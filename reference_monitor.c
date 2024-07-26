/*
* 
* This is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; either version 3 of the License, or (at your option) any later
* version.
* 
* This module is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
* 
* @file reference_monitor.c
* @brief This is the main source for the Linux Kernel Module which implements
*       a Reference Monitor for locking writing operation on files or directories
*
* @author Dissan Uddin Ahmed
*
* @date 2024, March, 11
*/


#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/apic.h>
#include <asm/io.h>
#include <linux/syscalls.h>
#include <linux/rculist.h>
#include "lib/include/reference_header.h"
#include <linux/rculist.h>
#include <linux/rcupdate.h>
#include <linux/list.h>

/* *****************************************************
 *            CUSTOM LIBRARIES
 * *****************************************************
 */ 

#include "lib/include/reference_hooks.h"
#include "lib/include/defer.h"
#include "lib/include/reference_syscalls.h"
#include "lib/include/rcu_restrict_list.h"
#include "lib/include/scth.h"
#include "lib/include/reference_types.h"
#include "lib/include/reference_path_based_syscall.h"
#include "lib/include/reference_hash.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dissan Uddin Ahmed <DissanAhmed@gmail.com>");
MODULE_DESCRIPTION("Reference Monitor that protects updates on files or directories based on some pathname");

#define unload_syscall\
        unprotect_memory();\
        for(i=0;i<HACKED_ENTRIES;i++){\
                ((unsigned long *)the_syscall_table)[restore[i]] = the_ni_syscall;\
        }\
        protect_memory()


#define target_func "vfs_open"
#define target_vfs_open_kp "vfs_open"

/**
*  *****************************************************
*       SYSCALL DEFINITIONS
*  *****************************************************
*/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(2, _change_state, char *, pwd, int, state){
#else
asmlinkage long sys_change_state(char *pwd, int state){
#endif
        return do_change_state(pwd, state);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(3, _change_paths, char *, pwd, char *, path, int, op){
#else
asmlinkage long sys_change_paths(char * pwd, char *path, int op){
#endif
	return do_change_path(pwd, path, op);
}

/**
* *****************************************************
*                 MODULE PARAMS
* *****************************************************
*/ 

char pwd[SHA512_LENGTH * 2 + 1] = {0};
module_param_string(pwd, pwd, sizeof(pwd), 0000);

unsigned long the_syscall_table = 0x0;
module_param(the_syscall_table, ulong, 0444);

unsigned long locked_path __attribute__((aligned(8)));//this is used to audit how many threads are still sleeping onto the sleep/wakeup queue
module_param(locked_path,ulong,0660);

unsigned long current_state = REC_OFF;
module_param(current_state,ulong,0444);

unsigned long audit_counter = 0x0;
module_param(audit_counter, ulong, 0444);

unsigned long Major = -1;
module_param(Major,ulong,0444);


/**
* *****************************************************
*                 GLOBALS ARGS
* *****************************************************
*/


unsigned long the_ni_syscall;
char salt[SALT_LENGTH ];

/* 
  * *****************************************************
  *             STRUCT FOR RCU STATE HANDLING
  * *****************************************************
*/
state_t reference_state;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
long sys_change_state = (unsigned long) __x64_sys_change_state;       
long sys_change_paths = (unsigned long) __x64_sys_change_paths;       
#else
#endif

unsigned long new_sys_call_array[] = {0x0,0x0};//please set to sys_got_sleep and sys_change_paths at startup
#define HACKED_ENTRIES (int)(sizeof(new_sys_call_array)/sizeof(unsigned long))
int restore[HACKED_ENTRIES] = {[0 ... (HACKED_ENTRIES-1)] -1};

/**
* restrict_path_list
*/

//static struct kretprobe retprobe;  

/* 
  * *****************************************************
  *             RESTRICT PATH RCU_LIST
  * *****************************************************
*/

LIST_HEAD(restrict_path_list);
spinlock_t restrict_path_lock;

//todo Add kretprobe to block wfs_write
/* 
  * *****************************************************
  *             KPROBES' STRUCT
  *             target_functions={vfs_open, vfs_mkdir, vfs_something}
  * *****************************************************
*/

struct kprobe probes[HOOKS_SIZE];
struct kprobe vfs_open_kp;

/* 
  * *****************************************************
  *             PATH BASED SYSCALL DEVICE
  * *****************************************************
*/

static struct file_operations fops;


int init_module(void) {

        int i;
        int ret;

        /*if (LINUX_VERSION_CODE < KERNEL_VERSION(5,11,0)){
	 	pr_err("%s: unsupported kernel version", MODNAME);
        		return -1; 	
        };
        */

        if (the_syscall_table == 0x0){
                pr_err("%s: cannot manage sys_call_table address set to 0x0\n",MODNAME);
                return -1;
        }

        if (strlen(pwd) <=8){
                pr_err("%s: password to short\n", MODNAME);
                return -1;
        }

        ret = generate_salt(salt);

        if (ret){
                pr_err("%s: salt generation problem\n", MODNAME);
                return ret;
        }

        ret = hash_plaintext(salt, pwd, pwd);

        if (ret){
                pr_err("%s: hash password problem\n", MODNAME);
                return ret;
        }
        
        AUDIT{
           pr_info("%s: queuing example received sys_call_table address %px\n",MODNAME,(void*)the_syscall_table);
           pr_info("%s: initializing - hacked entries %d\n",MODNAME,HACKED_ENTRIES);
        }

        new_sys_call_array[0] = (unsigned long)sys_change_state;
        new_sys_call_array[1] = (unsigned long)sys_change_paths;
        ret = get_entries(restore,HACKED_ENTRIES,(unsigned long*)the_syscall_table,&the_ni_syscall);

        if (ret != HACKED_ENTRIES){
                pr_err("%s: could not hack %d entries (just %d)\n",MODNAME,HACKED_ENTRIES,ret);

                return -1;
        }

        unprotect_memory();

        for(i=0;i<HACKED_ENTRIES;i++){
                ((unsigned long *)the_syscall_table)[restore[i]] = (unsigned long)new_sys_call_array[i];
        }

        protect_memory();

        probes[0].pre_handler = vfs_open_wrapper;
        probes[0].post_handler = vfs_open_post_wrapper;
        probes[0].symbol_name = target_vfs_open_kp;

        ret = register_kprobe(&probes[0]);

        if (ret < 0){
                pr_err("%s: unbale to register kprobe for %s\n", MODNAME, target_vfs_open_kp);
                goto error_syscall_installed;
        }
        AUDIT
        pr_info("%s: all new system-calls correctly installed on sys-call table\n",MODNAME);
        
	if (ret < 0) {
		pr_err("%s: hook init failed, returned %d\n", MODNAME, ret);
                goto error_syscall_installed;
	}
        AUDIT
	pr_info("%s: read hook module correctly loaded\n", MODNAME);

         
        fops.owner = THIS_MODULE;
        fops.write = vfs_sys_wrapper_wrt;
        fops.read = vfs_sys_wrapper_rd;
        fops.open =  vfs_sys_wrapper_opn;
        fops.release = vsf_sys_wrapper_rls;

        Major = register_chrdev(0, DEVICE_NAME, &fops);

	if (Major < 0) {
	        pr_err("%s: registering device failed\n",MODNAME);
                ret = Major;
                goto error_kprobe_installed;
        }
        AUDIT
        pr_info("%s: char device driver registered\n", MODNAME);
        reference_state.state = OFF;
        disable_kprobe(&probes[0]);
        return ret;

error_kprobe_installed:
        unregister_kprobe(&vfs_open_kp);
error_syscall_installed:
        unload_syscall;

        return ret;
}


void cleanup_module(void) {

        int i;
        struct rcu_restrict *p;

        pr_info("%s: shutting down\n",MODNAME);
        unregister_kprobe(&probes[0]);
        pr_info("%s: unregistered the kprobe for %s\n", MODNAME, target_vfs_open_kp);
        unregister_chrdev(Major, DEVICE_NAME);
        pr_info("%s: unregistered the device name=%s with Major=%ld", MODNAME, DEVICE_NAME, Major);
        pr_info("%s: safe releasing nodes in restrict_path_list\n",MODNAME);
        
        spin_lock(&restrict_path_lock);
        list_for_each_entry(p, &restrict_path_list, node){      
                list_del_rcu(&p->node);
                spin_unlock(&restrict_path_lock);
                synchronize_rcu();
                kfree(p);            
        }
        spin_unlock(&restrict_path_lock);

        pr_info("%s:  deleted the_rcu_list\n", MODNAME);
	unload_syscall;
        pr_info("%s: sys-call table restored to its original content\n",MODNAME);

}


