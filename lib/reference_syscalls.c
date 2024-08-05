
#define EXPORT_SYMTAB

#include <linux/printk.h>
#include <linux/version.h>
#include <asm-generic/errno-base.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/cred.h>
#include <linux/uidgid.h>

#include "include/reference_types.h"
#include "include/reference_hooks.h"
#include "include/reference_syscalls.h"
#include "include/rcu_restrict_list.h"

#define REFERENCE_SYSCALLS "REFERENCE SYSCALLS"
/**
 * @param state: int it allow to change_the current state of the reference monitor
 * @param the_pwd: char* if is correc than the function is committed
 * @return: int, 0 if is al good -1 othrewise
 */

//todo not yet implemented

#define TEMP_PWD "password"

int check_pwd(char *pwd){
        return 0;
}
int do_change_state(char * the_pwd, int the_state){
        
        int i;
        unsigned long ret;
        char from_user_password[MAX_PATH_LEN];
        const struct cred *cred;
        
        cred = current_cred();

        if (likely(cred->euid.val != admin)){
                return -EACCES;
        }

        if (the_pwd == NULL || !(the_state & ON || the_state & OFF || the_state & REC_ON || the_state & REC_OFF)){
                return -EINVAL;
        }
        //TODO CHECK THIS

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
        ret = copy_from_user(from_user_password, the_pwd, MAX_PATH_LEN);
        if (ret < 0){
                pr_err("%s: cannot copy from the user the password\n", REFERENCE_SYSCALLS);
                return -EACCES;
        }
#else
        strcpy(from_user_password, the_pwd);
#endif


        if ((ret = check_pwd(from_user_password)) < 0){
                return ret;
        }; 
        
        spin_lock(&(reference_state_spinlock));
        //the_state = current_state;
        // this must be atomic
        //printk("%s: called sys_change_state old_state %ld\n", REFERENCE_SYSCALLS, current_reference_state);
        pr_info("%s: requested change_state previous state: %ld\n", REFERENCE_SYSCALLS, current_state);
        if (current_state == the_state){
                spin_unlock(&(reference_state_spinlock));
                return -ECANCELED;
        }
        /*
        if (the_state == REC_ON && the_state == OFF){
                the_state = REC_OFF;
        } else if (current_reference_state == REC_OFF && the_state == ON){
                the_state = REC_ON;
        }else {
                the_state = the_state;
        }*/
        current_state = the_state;

        if ((current_state & REC_ON) || (current_state & ON)){
                for(i=0;i<HOOKS_SIZE;i++){
                        enable_kprobe(&probes[i]);
                }
        }else {
                for(i=0;i<HOOKS_SIZE;i++){
                        disable_kprobe(&probes[i]);
                }
        }
        pr_info("%s: requested change_state new state: %ld\n", REFERENCE_SYSCALLS, current_state);
        spin_unlock(&(reference_state_spinlock));
        return 0;
}

/**
 * @param path: char* defined pathname that has to be added to restricted list or 
 *              deleted from restricted list
 * @param op: int ADD_PATH to add REMOVE_PATH to remove
 * @return : int ret = 0 all good ret < 0 reporting typeof error
 */

int do_change_path(char *the_pwd, char *the_path, int op){

        int ret = 0;
        long current_reference_state;
        char from_user_password[MAX_PATH_LEN];
        char from_user_path[MAX_PATH_LEN];
        const struct cred *cred;
        
        cred = current_cred();

        if (likely(cred->euid.val != admin)){
                return -EACCES;
        }

        if ((the_path == NULL) || !((op & ADD_PATH) || (op & REMOVE_PATH)) ||
                (op & REMOVE_PATH & ADD_PATH)){
                return -EINVAL;
        }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
        ret = copy_from_user(from_user_password, the_pwd, MAX_PATH_LEN);
        if (ret < 0){
                pr_err("%s: cannot copy from the user the password\n", REFERENCE_SYSCALLS);
                return -EACCES;
        }
        ret = copy_from_user(from_user_path, the_path, MAX_PATH_LEN);
        if (ret < 0){
                pr_err("%s: cannot copy from the user the path\n", REFERENCE_SYSCALLS);
                return -EACCES;
        }
#else
        from_user_password = the_pwd;
        from_user_path = the_path;
#endif
        if (forbitten_path(from_user_path) < 0){
                return -EINVAL;
        }
        if ((ret = check_pwd(from_user_password)) < 0){
                return ret;
        }; 

        

        spin_lock(&(reference_state_spinlock));
        current_reference_state = current_state;
        spin_unlock(&(reference_state_spinlock));

        if (!((current_reference_state &  REC_OFF) || (current_reference_state & REC_ON))){
                if (!((op & REC_ON) || (op & REC_OFF))){
                        return  -ECANCELED;
                }
                
                if ((ret = do_change_state(the_pwd, op))){
                        return ret;
                }
        }
        
        if (op & ADD_PATH){
                ret = add_path(from_user_path);
        }
        if (op & REMOVE_PATH){
                ret = del_path(from_user_path);
        }
        
        return ret;
}


