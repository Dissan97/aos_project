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
#include <linux/uaccess.h>
#include <linux/namei.h>
#include <linux/rculist.h>
#include <linux/rcupdate.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/dcache.h>
#include <linux/cred.h>
#include <linux/uidgid.h>


/* *****************************************************
 *            CUSTOM LIBRARIES
 * *****************************************************
 */ 

#include "lib/include/reference_hooks.h"
#include "lib/include/reference_defer.h"
#include "lib/include/reference_syscalls.h"
#include "lib/include/rcu_restrict_list.h"
#include "lib/include/scth.h"
#include "lib/include/reference_types.h"
#include "lib/include/reference_path_based_syscall.h"
#include "lib/include/reference_hash.h"

#define unload_syscall\
        unprotect_memory();\
        for(i=0;i<HACKED_ENTRIES;i++){\
                ((unsigned long *)the_syscall_table)[restore[i]] = the_ni_syscall;\
        }\
        protect_memory()



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

unsigned int admin = 0xFFFFFFFF;
module_param(admin,uint,0000);

unsigned long the_syscall_table = 0x0;
module_param(the_syscall_table, ulong, 0444);

unsigned long current_state = OFF;
module_param(current_state,ulong,0444);

unsigned long audit_counter = 0x0;
module_param(audit_counter, ulong, 0444);

unsigned long path_syscall_device_major = -1;
module_param(path_syscall_device_major,ulong,0444);




/**
* *****************************************************
*                 GLOBALS ARGS
* *****************************************************
*/


unsigned long the_ni_syscall;
char salt[(SALT_LENGTH + 1)];

/* 
  * *****************************************************
  *              STATE HANDLING SPINLOCK
  * *****************************************************
*/
//state_t reference_state;
spinlock_t reference_state_spinlock;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
long sys_change_state = (unsigned long) __x64_sys_change_state;       
long sys_change_paths = (unsigned long) __x64_sys_change_paths;       
#else
#endif

/* 
  * *****************************************************
  *              SYSTEM CALLS
  * *****************************************************
*/

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

/**
  * *****************************************************
  *             KERNEL DAEMON HOUSEKEEPER
  * *****************************************************
 */
struct task_struct *daemon_task;
#define DAEMON_NAME "RM DAEMON HOUSEKEEPER"


/* 
  * *****************************************************
  *             KPROBES' STRUCT
  *             target_functions={vfs_open, (vfs_mkdir, vfs_rmdir, vfs_unlink)}
  * *****************************************************
*/

struct kprobe probes[HOOKS_SIZE];

/*
struct kprobe vfs_open_kp;
struct kprobe vfs_mkdir_kp;
struct kprobe vfs_rmdir_kp;
struct kprobe vfs_unlink_kp;
*/

/* 
  * *****************************************************
  *             PATH BASED SYSCALL DEVICE 
  * *****************************************************
*/

static struct file_operations fops;
/* 
  * *****************************************************
  *             APPEND ONLY DEVICE & DEFER
  * *****************************************************
*/
struct file_operations append_fops;
unsigned long append_defer_device_major = -1;
struct workqueue_struct *deferred_queue;
char *file_appendonly;

//kernel house_keeper daemon
static int daemon_housekeepr_restrict_list(void *data)
{
        struct path path;
        struct rcu_restrict *entry;
        int error;

        AUDIT
        pr_info("%s: daemon alive\n", MODNAME);
        while (!kthread_should_stop()) {
                
                msleep(60000); // Sleep for 1 minute
                AUDIT
                pr_info("%s: daemon wake up looking for path if it still exists\n", MODNAME);
                spin_lock(&restrict_path_lock);
                list_for_each_entry(entry, &restrict_path_list, node){
                        error = kern_path(entry->path, LOOKUP_FOLLOW, &path);
                        if (error){
                                list_del_rcu(&entry->node);
                                synchronize_rcu();
                                spin_unlock(&restrict_path_lock);
                                kfree(entry->path);
                                kfree(entry);
                                return 0;
                        }
                }
                spin_unlock(&restrict_path_lock);

                synchronize_rcu(); // Ensure all RCU callbacks have completed
        }

    return 0;
}


static int __init reference_monitor_init(void) 
{

        int i;
        int j; 
        int ret;
        kuid_t kuid;
        
        const char target_functions [HOOKS_SIZE][12] = {
                "vfs_open",
                "vfs_mkdir",
                "vfs_rmdir",
                "vfs_unlink"
        };
        const kprobe_pre_handler_t pre_handlers [HOOKS_SIZE] = {
                vfs_open_wrapper,
                vfs_mkrmdir_unlink_wrapper,
                vfs_mkrmdir_unlink_wrapper,
                vfs_mkrmdir_unlink_wrapper
        };
        
        if (admin == 0xFFFFFFFF){
                pr_err("%s: pass security admin uid\n", MODNAME);
                return -EINVAL;
        }
        kuid.val = admin;
        if (!uid_valid(kuid)){
                pr_err("%s: not valid admin %d\n", MODNAME, admin);
                return -EINVAL;
        }
        AUDIT
        pr_info("%s: the reference monitor admin is (uid=%d)\n",MODNAME, admin);
        if (the_syscall_table == 0x0){
                pr_err("%s: cannot manage sys_call_table address set to 0x0\n",MODNAME);
                return -EINVAL;
        }

        if (strlen(pwd) <=8){
                pr_err("%s: password to short\n", MODNAME);
                return -EINVAL;
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
        //SYSCALL INSTALLATION
        AUDIT{
           pr_info("%s: queuing received sys_call_table address %px\n",MODNAME,(void*)the_syscall_table);
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
        AUDIT
        pr_info("%s: all new system-calls correctly installed on sys-call table\n",MODNAME);
        //KPROBE INSTALLATION
        for (i = 0; i < HOOKS_SIZE; i++){
                
                probes[i].pre_handler = pre_handlers[i];
                probes[i].symbol_name = target_functions[i];
                ret = register_kprobe(&probes[i]);
                if (ret < 0){
                        pr_err("%s: unbale to register kprobe for %s unloading previous probes\n", MODNAME, target_functions[i]);
                        for (j = 0; j < i; j++){
                                pr_err("%s: probe for target func=%s unrestired\n", MODNAME, target_functions[j]);
                                unregister_kprobe(&probes[j]);
                        }
                        goto error_syscall_installed;
                }
                disable_kprobe(&probes[i]);
                AUDIT
                pr_info("%s: kprobe for target_fun -> %s installed but disbled\n", MODNAME, target_functions[i]);
        }
        
        AUDIT
	pr_info("%s: read hook module correctly loaded\n", MODNAME);

        //PATH BASED SYSTEM CALL INSTALLATION
        fops.owner = THIS_MODULE;
        fops.write = vfs_sys_wrapper_wrt;
        fops.read = vfs_sys_wrapper_rd;
        fops.open =  vfs_sys_wrapper_opn;
        fops.release = vsf_sys_wrapper_rls;

        path_syscall_device_major = register_chrdev(0, DEVICE_SYS_VFS, &fops);

	if (path_syscall_device_major < 0) {
	        pr_err("%s: registering device failed\n",MODNAME);
                ret = path_syscall_device_major;
                goto error_kprobe_installed;
        }
        AUDIT
        pr_info("%s: char device driver for syscall registered\n", MODNAME);

        //APPEND DEFER WORK INSTALLATION
        append_fops.owner = THIS_MODULE;
        append_fops.write = append_defer_wrt;
        append_fops.read = append_defer_rd;
        append_fops.open =  append_defer_opn;
        append_fops.release = append_defer_rls;

        append_defer_device_major = register_chrdev(0, DEVICE_WORK_DEFER_APPEND, &append_fops);
	if (append_defer_device_major < 0) {
	        pr_err("%s: registering device failed\n",MODNAME);
                ret = append_defer_device_major;
                goto error_sys_path_based_char_device_installed;
        }

        deferred_queue = create_singlethread_workqueue(DEVICE_WORK_DEFER_APPEND);
        if (!deferred_queue){
                pr_err("%s: cannot create the work queue=%s\n", MODNAME, DEVICE_WORK_DEFER_APPEND);
                ret = -ENOMEM;
                goto error_defer_append_device_installed;
        }
        AUDIT
        pr_info("%s: workqueue created\n", MODNAME);
        /*
        daemon_task = kthread_run(daemon_housekeepr_restrict_list, NULL, DAEMON_NAME);
        if ((ret = IS_ERR(daemon_task))){
                pr_err("Failed to create daemon thread\n");
                goto error_work_queue_installed;
        }*/
        return ret;
error_work_queue_installed:
        destroy_workqueue(deferred_queue);
error_defer_append_device_installed:
        unregister_chrdev(append_defer_device_major, DEVICE_WORK_DEFER_APPEND);        
error_sys_path_based_char_device_installed:
        unregister_chrdev(path_syscall_device_major, DEVICE_SYS_VFS);
error_kprobe_installed:
        for (i = 0; i < HOOKS_SIZE; i++){
                unregister_kprobe(&probes[i]);
        }        
error_syscall_installed:
        unload_syscall;

        return ret;
}


static void __exit reference_monitor_cleanup(void) 
{

        int i;
        struct rcu_restrict *p;
        AUDIT
        pr_info("%s: shutting down\n",MODNAME);
        for (i = 0; i < HOOKS_SIZE; i++){
                unregister_kprobe(&probes[i]);
        }
        AUDIT
        pr_info("%s: unregistered all kprobes\n", MODNAME);
        unregister_chrdev(path_syscall_device_major, DEVICE_SYS_VFS);
        AUDIT
        pr_info("%s: unregistered the device name=%s with path_syscall_device_major=%ld", MODNAME, DEVICE_SYS_VFS, path_syscall_device_major);
        unregister_chrdev(append_defer_device_major, DEVICE_WORK_DEFER_APPEND);  
        AUDIT{
        pr_info("%s: unregistered the device name=%s with path_syscall_device_major=%ld", MODNAME, DEVICE_WORK_DEFER_APPEND, append_defer_device_major);
        pr_info("%s: safe releasing nodes in restrict_path_list\n",MODNAME);
        }
        
        spin_lock(&restrict_path_lock);
        list_for_each_entry(p, &restrict_path_list, node){      
                list_del_rcu(&p->node);
                synchronize_rcu();
                spin_unlock(&restrict_path_lock);
                kfree(p);            
        }
        spin_unlock(&restrict_path_lock);
        AUDIT
        pr_info("%s:  deleted the_rcu_list\n", MODNAME);
	unload_syscall;
        AUDIT
        pr_info("%s: sys-call table restored to its original content\n",MODNAME);
        destroy_workqueue(deferred_queue);
        AUDIT
        pr_info("%s: work queue destryed\n", MODNAME);
        //kthread_stop(daemon_task);
        AUDIT
        pr_info("%s: stopped %s\n", MODNAME, DAEMON_NAME);
}


module_init(reference_monitor_init);
module_exit(reference_monitor_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dissan Uddin Ahmed <DissanAhmed@gmail.com>");
MODULE_DESCRIPTION("Reference Monitor that protects updates on files or directories based on some pathname");