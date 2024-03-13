#include "include/rcu_restrict_list.h"
#include <linux/slab.h>
#include <linux/slab.h>
#include <linux/rculist.h>
#include <linux/rcupdate.h>
#include <asm-generic/errno-base.h>

#define RCU_RESTRICT_LIST "RCU RESTRICT LIST"

int add_path(char *pathname, struct list_head *the_rcu_list)
{
        struct rcu_restrict_path_list *entry;
        size_t pathname_len;
        
        if (!pathname || !the_rcu_list) return -EINVAL;

        pathname_len = strlen(pathname);
        
        entry = kmalloc(sizeof(struct rcu_restrict_path_list), GFP_KERNEL);
        if (!entry) return -ENOMEM;
        

        entry->pathname = kmalloc(sizeof(char) * (pathname_len + 1) , GFP_KERNEL);
        if (!entry->pathname) {
                kfree(entry);
                return -ENOMEM;
        }


        strncpy(entry->pathname, pathname, pathname_len);
        INIT_LIST_HEAD(&entry->list);
        list_add_rcu(&entry->list, the_rcu_list);
        
        return 0;
}


int del_path(char *pathname, struct list_head *the_rcu_list)
{
        
        struct rcu_restrict_path_list *entry;
        struct rcu_restrict_path_list *tmp;

        if (!pathname || !the_rcu_list) return -EINVAL;

        list_for_each_entry_safe(entry, tmp, the_rcu_list, list){
                if (!strncmp(entry->pathname, pathname, strlen(pathname))){
                        list_del_rcu(&entry->list);
                        kfree(entry);
                }
        }

   return 0;   
}


int is_path_in(char *pathname, struct list_head *the_rcu_list)
{
        
        struct rcu_restrict_path_list *entry;
        int ret = -1;
        if (!pathname || !the_rcu_list) return -EINVAL;
        rcu_read_lock();
        list_for_each_entry_rcu(entry, the_rcu_list, list){
        
                if (!strncmp(pathname, entry->pathname, strlen(pathname))){
                        printk(KERN_INFO "%s: path: %s\n", RCU_RESTRICT_LIST, entry->pathname);
                        ret = 0;
                        goto ret_rcu_read;
                }
        }

ret_rcu_read:       

        rcu_read_unlock();
        return ret;

}

void safe_delele_list(struct list_head * the_rcu_list){
        struct rcu_restrict_path_list *entry;
        struct rcu_restrict_path_list *tmp;
        
        if (!the_rcu_list) return;

        list_for_each_entry_safe(entry, tmp, the_rcu_list, list){
                list_del_rcu(&entry->list);
                kfree(entry);
        }

        printk(KERN_INFO "%s: safe deleted the_rcu_list\n", RCU_RESTRICT_LIST);
 
}
