#include "include/reference_syscalls.h"
#include "include/rcu_restrict_list.h"
#include "include/reference_header.h"
#include <linux/printk.h>
#include <asm-generic/errno-base.h>
#include <linux/string.h>
#include "include/reference_types.h"

#define REFERENCE_SYSCALLS "REFERENCE SYSCALLS"
/**
 * @param state: int it allow to change_the current state of the reference monitor
 * @param the_password: char* if is correc than the function is committed
 * @return: int, 0 if is al good -1 othrewise
 */

//todo not yet implemented

#define TEMP_PWD "password"


int do_change_state(char * the_password, int op){

        if (the_password == NULL || !(op & ON || op & OFF || op & REC_ON || op & REC_OFF)){
                return -EINVAL;
        }

        if (strncmp(TEMP_PWD, the_password, strlen(TEMP_PWD))){
                return -1;
        } 
                
        // this must be atomic
        printk("%s: called sys_change_state old_state %ld\n", REFERENCE_SYSCALLS, current_state);
        
        if (current_state == REC_ON && op == OFF){
                current_state = REC_OFF;
        }

        if (current_state == REC_OFF && op == ON){
                current_state = REC_ON;
        }

        current_state = op;

        printk("%s: called sys_change_state current_state=%ld, password=%s\n", REFERENCE_SYSCALLS, current_state, the_password);
        return 0;
}

/**
 * @param path: char* defined pathname that has to be added to restricted list or 
 *              deleted from restricted list
 * @param op: int ADD_PATH to add REMOVE_PATH to remove
 * @return : int 0 all good -1 reporting typeof error
 */

int do_change_path(char *the_pwd, char *pathname, int op){

        int ret;

        if ((pathname == NULL) || !((op & ADD_PATH) || (op & REMOVE_PATH))){
                return -EINVAL;
        }


        if (!((current_state &  REC_OFF) || (current_state & REC_OFF))){
                if (!((op & REC_ON) || (op & REC_OFF))){
                        return  -EINVAL;
                }
                
                if (do_change_state(the_pwd, op)){
                        return -1;
                }
                goto do_change_path_label;
        }

        if (ret) {
                return ret;
        }
        
do_change_path_label:
        if (op == ADD_PATH)
                ret = add_path(pathname, &restrict_path_list);
        if (op == REMOVE_PATH)
                ret = del_path(pathname, &restrict_path_list);

        if (ret) return  ret;

        ret = is_path_in(pathname, &restrict_path_list);
        if (ret) return ret;
        printk("%s: called sys_change_path path=%s, op=%d\n",REFERENCE_SYSCALLS, pathname, op);
        return ret;
}

