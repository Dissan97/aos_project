#include "include/rcu_restrict_list.h"
#include <linux/slab.h>
#include <linux/rculist.h>
#include <linux/rcupdate.h>
#include <asm-generic/errno-base.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/dcache.h>
#include <linux/path.h>

#define RCU_RESTRICT_LIST "RCU RESTRICT LIST"

/**
 * 
struct restrict_path_list {
  char *path;
  struct dentry *dentry;
  struct list_head list;
  struct rcu_head rcu;
};
 */

int add_path(char *the_path)
{

        struct rcu_restrict *p;
        struct path dummy;
        int err;
        long len;

        if (!the_path){
                return -EINVAL;
        }

        if (!is_path_in(the_path)){
                return -ECANCELED;
        }

        p = kzalloc(sizeof(struct rcu_restrict), GFP_KERNEL);
        if (!p){
                pr_warn("%s: cannot alloc memory for struct path\n", RCU_RESTRICT_LIST);
                return -ENOMEM;
        }
        
        
        //TODO add dentry search
        len = strlen(the_path);
        p->path = kzalloc(sizeof(char)*(len + 1), GFP_KERNEL);
        
        if (!p->path){
                pr_warn("%s: cannot alloc memory to insert new path\n", RCU_RESTRICT_LIST);
                kfree(p);
                return -ENOMEM;
        }
        strncpy(p->path, the_path, len);

        err = kern_path(the_path, LOOKUP_FOLLOW, &dummy);
        if (err){
                return err;
        }
        p->dentry = dummy.dentry;
        spin_lock(&restrict_path_lock);
        list_add_rcu(&p->node, &restrict_path_list);
        spin_unlock(&restrict_path_lock);
        return 0;

        /*
        struct rcu_restrict_path_list *entry;
        size_t the_path_len;
        
        if (!the_path || !the_rcu_list) return -EINVAL;
        if (!is_path_in(the_path, the_rcu_list)){
                pr_info("%s: ALREDY EXIST\n", RCU_RESTRICT_LIST);
                return -EINVAL;
        }
        the_path_len = strlen(the_path);
        
        entry = kmalloc(sizeof(struct rcu_restrict_path_list), GFP_KERNEL);
        if (!entry) return -ENOMEM;
        

        entry->path = kmalloc(sizeof(char) * (the_path_len + 1) , GFP_KERNEL);
        if (!entry->path) {
                kfree(entry);
                return -ENOMEM;
        }


        strncpy(entry->path, the_path, the_path_len);
        INIT_LIST_HEAD(&entry->list);
        list_add_rcu(&entry->list, the_rcu_list);
        pr_info("%s: inserted %s\n", RCU_RESTRICT_LIST, the_path);
        return 0;
        */
}


int del_path(char *the_path)
{
        struct rcu_restrict *p;
        long len;
        if (!the_path){
                return -EINVAL;
        }
        len = strlen(the_path);
        spin_lock(&restrict_path_lock);
        list_for_each_entry(p, &restrict_path_list, node){
                if (!strncmp(the_path, p->path, len)){
                        list_del_rcu(&p->node);
                        spin_unlock(&restrict_path_lock);
                        synchronize_rcu();
                        kfree(p);
                        return 0;
                }
        }
        spin_unlock(&restrict_path_lock);
        /*
        struct rcu_restrict_path_list *entry;
        struct rcu_restrict_path_list *tmp;

        if (!the_path || !the_rcu_list) return -EINVAL;

        list_for_each_entry_safe(entry, tmp, the_rcu_list, list){
                if (!strncmp(entry->path, the_path, strlen(the_path))){
                        list_del_rcu(&entry->list);
                        kfree(entry);
                }
        }
        */

   return -ENOKEY;   
}


int is_path_in(char *the_path)
{
        
        struct rcu_restrict *p;
        long len;
        
        if (!the_path){
                return -1;
        }

        len = strlen(the_path);

        rcu_read_lock();
        list_for_each_entry_rcu(p, &restrict_path_list, node) {
		if(!strncmp(the_path, p->path, len)) {
			rcu_read_unlock();
			return 0;
		}
	}
	rcu_read_unlock();

	pr_err("%s: path does not exists\n", RCU_RESTRICT_LIST);
        return -1;

 /*       int ret = -1;
        if (!the_path || !the_rcu_list) return -EINVAL;
        pr_info("%s: Searching for %s in the list\n", RCU_RESTRICT_LIST, the_path);
        rcu_read_lock();
        list_for_each_entry_rcu(entry, the_rcu_list, list){
                pr_info("%s: entry found %s\n",RCU_RESTRICT_LIST, entry->path);
                if (!strncmp(the_path, entry->path, strlen(the_path))){
                        pr_info("%s: path: %s\n", RCU_RESTRICT_LIST, entry->path);
                        ret = 0;
                        rcu_read_unlock();
                        return ret;
                }
        }
      
        rcu_read_unlock();
        return ret;
        */

}

