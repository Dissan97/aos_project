
#include <linux/slab.h>
#include <linux/rculist.h>
#include <linux/rcupdate.h>
#include <asm-generic/errno-base.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/dcache.h>
#include <linux/path.h>

#include "include/reference_types.h"
#include "include/rcu_restrict_list.h"
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
const char *proc = "/proc";
const char *sys = "/sys";
const char *run = "/run";
const char *usr = "/usr";
const char *var = "/var";
const char *bin = "/bin";


int is_path_in(const char *the_path)
{
        
        struct rcu_restrict *p;
        
        if (!the_path){
                return -EINVAL;
        }


        rcu_read_lock();
        list_for_each_entry_rcu(p, &restrict_path_list, node) {
		if(!strcmp(the_path, p->path)) {
			rcu_read_unlock();
			return 0;
		}
	}
	rcu_read_unlock();

	//pr_err("%s: path does not exists\n", RCU_RESTRICT_LIST);
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


int add_path(char *the_path)
{

        struct rcu_restrict *entry;
        struct path dummy_path;
        int err;
        long len;

        if (!the_path){
                return -EINVAL;
        }

        if (!is_path_in(the_path)){
                return -ECANCELED;
        }

        entry = kzalloc(sizeof(struct rcu_restrict), GFP_KERNEL);
        if (!entry){
                pr_warn("%s: cannot alloc memory for struct path\n", RCU_RESTRICT_LIST);
                return -ENOMEM;
        }
        
        
        //TODO add dentry search
        len = strlen(the_path);
        entry->path = kzalloc(sizeof(char)*(len + 1), GFP_KERNEL);
        
        if (!entry->path){
                pr_warn("%s: cannot alloc memory to insert new path\n", RCU_RESTRICT_LIST);
                kfree(entry);
                return -ENOMEM;
        }
        strncpy(entry->path, the_path, len);

        err = kern_path(the_path, LOOKUP_FOLLOW, &dummy_path);
        if (err){
                kfree(entry->path);
                kfree(entry);
                return err;
        }

        if (!dummy_path.dentry->d_inode){
                kfree(entry->path);
                kfree(entry);
                return -ECANCELED;
        }
        entry->i_ino = dummy_path.dentry->d_inode->i_ino;   
        
        spin_lock(&restrict_path_lock);
        list_add_rcu(&entry->node, &restrict_path_list);
        spin_unlock(&restrict_path_lock);
        return 0;
}



int del_path(char *the_path)
{
        struct rcu_restrict *entry;
        long len;
        if (!the_path){
                return -EINVAL;
        }
        len = strlen(the_path);
        spin_lock(&restrict_path_lock);
        list_for_each_entry(entry, &restrict_path_list, node){
                if (!strncmp(the_path, entry->path, len)){
                        list_del_rcu(&entry->node);
                        spin_unlock(&restrict_path_lock);
                        synchronize_rcu();
                        kfree(entry->path);
                        kfree(entry);
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
int forbitten_path(const char *the_path){
        if (!the_path) {
                return -1;
        }
       return (strncmp(the_path, proc, 5) == 0 ||
        strncmp(the_path, sys, 4) == 0 ||
        strncmp(the_path, run, 4) == 0 ||
        strncmp(the_path, usr, 4) == 0 ||
        strncmp(the_path, var, 4) == 0 ||
        strncmp(the_path, bin, 4) == 0 ) ? -1 : 0;
}


int is_black_listed(const char *path, unsigned long i_ino){

        struct rcu_restrict *entry;
        if (!path){
                return -EINVAL;
        }
        
        rcu_read_lock();
        list_for_each_entry_rcu(entry, &restrict_path_list, node) {
		if(!strcmp(path, entry->path) || entry->i_ino == i_ino) {
			rcu_read_unlock();
			return 0;
		}
                
	}
	rcu_read_unlock();
        return -EEXIST;


}




/*
int is_inode_in(struct inode *the_inode){
        struct rcu_restrict *p;
        
        if (!the_inode){
                return -EINVAL;
        }

        rcu_read_lock();
        list_for_each_entry_rcu(p, &restrict_path_list, node) {
		
                if ((the_inode->i_ino == p->target_inode->i_ino) && (the_inode->i_sb == p->target_inode->i_sd)){
                        rcu_read_unlock();
                        return 0;
                }
                
	}
	rcu_read_unlock();
        return -EEXIST;
}

*/