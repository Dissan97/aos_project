#include "include/reference_syscalls.h"
#include "include/rcu_restrict_list.h"
#include "include/reference_header.h"
#include <linux/printk.h>
#include <asm-generic/errno-base.h>
#include <linux/string.h>
#include "include/reference_types.h"
#include "include/reference_hooks.h"

#define REFERENCE_SYSCALLS "REFERENCE SYSCALLS"
/**
 * @param state: int it allow to change_the current state of the reference monitor
 * @param the_password: char* if is correc than the function is committed
 * @return: int, 0 if is al good -1 othrewise
 */

//todo not yet implemented

#define TEMP_PWD "password"



int do_change_state(char * the_password, int the_state){
        
        int i;
        if (the_password == NULL || !(the_state & ON || the_state & OFF || the_state & REC_ON || the_state & REC_OFF)){
                return -EINVAL;
        }
        //TODO CHECK THIS
        if (strncmp(TEMP_PWD, the_password, strlen(TEMP_PWD))){
                return -EINVAL;
        } 
        
        spin_lock(&(reference_state.write_lock));
        //the_state = reference_state.state;
        // this must be atomic
        //printk("%s: called sys_change_state old_state %ld\n", REFERENCE_SYSCALLS, current_reference_state);
        pr_info("%s: requested change_state previous state: %ld\n", REFERENCE_SYSCALLS, reference_state.state);
        if (reference_state.state == the_state){
                spin_unlock(&(reference_state.write_lock));
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
        reference_state.state = the_state;

        if ((reference_state.state & REC_ON) || (reference_state.state & ON)){
                for(i=0;i<HOOKS_SIZE;i++){
                        enable_kprobe(&probes[i]);
                }
        }else {
                for(i=0;i<HOOKS_SIZE;i++){
                        disable_kprobe(&probes[i]);
                }
        }
        pr_info("%s: requested change_state new state: %ld\n", REFERENCE_SYSCALLS, reference_state.state);
        spin_unlock(&(reference_state.write_lock));
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
        if ((the_path == NULL) || !((op & ADD_PATH) || (op & REMOVE_PATH)) ||
                (op & REMOVE_PATH & ADD_PATH)){
                return -EINVAL;
        }
        pr_info("%s: called sys_change_path path=%s, op=%d\n",REFERENCE_SYSCALLS, the_path, op);
        //TODO check password
        spin_lock(&(reference_state.write_lock));
        current_reference_state = reference_state.state;
        spin_unlock(&(reference_state.write_lock));

        if (!((current_reference_state &  REC_OFF) || (current_reference_state & REC_ON))){
                if (!((op & REC_ON) || (op & REC_OFF))){
                        return  -ECANCELED;
                }
                
                if ((ret = do_change_state(the_pwd, op))){
                        return ret;
                }
        }
        
        if (op & ADD_PATH){
                ret = add_path(the_path);
        }
        if (op & REMOVE_PATH){
                ret = del_path(the_path);
        }
        
        return ret;
}


