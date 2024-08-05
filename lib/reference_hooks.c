#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
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

#include "include/reference_hooks.h"
#include "include/rcu_restrict_list.h"
#include "include/reference_defer.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dissan Uddin Ahmed <dissanahmed@gmail.com>");

#define REFERENCE_HOOK "REFERENCE HOOK"
#define MAX_LEN 256
#define FORBITTEN_MASK (O_RDWR | O_WRONLY | O_TRUNC | O_APPEND)

int vfs_open_wrapper(struct kprobe *ri, struct pt_regs * regs)
{
        struct path* path = (struct path*) (regs->di);
        struct dentry *dentry;
		struct dentry *d_parent;
        struct file *filp = (struct file *)regs->si;
		unsigned int f_flags;
		if (likely(filp)){
			f_flags = filp->f_flags;
			if (
				((f_flags & O_RDWR) != 0) && 
				((f_flags & O_WRONLY) != 0) && 
				((f_flags & O_TRUNC) != 0) && 
				((f_flags & O_APPEND) != 0)
			)
			/*if (!(filp->f_flags & FORBITTEN_MASK)){
				return 0;
			}*/
			return 0;
		}
	
        if (likely(path)){
                if (path->dentry){
					dentry = path->dentry;
				   				  
		/*		   	p=dentry_path_raw(dentry, dir, MAX_LEN);
					if (forbitten_path(p) < 0){
						return 0;
				   }
		*/
					//if (p){
						//pr_info("Dissan dice: %s\n", p);

					if (!is_black_listed(dentry)){
						atomic_inc((atomic_t*)&audit_counter);
						//inode = dentry -> d_inode;
						if (filp != NULL){
							//if (filp->f_flags & (O_RDWR | O_WRONLY | O_TRUNC | O_APPEND) ){
								pr_err("%s: cannot open this file=%s in write|append|trunc\n", REFERENCE_HOOK, dentry->d_name.name);
								regs->si = (unsigned long)(NULL);
								regs->di = (unsigned long)(NULL);								
								goto store_thread_info_vfs_open;
							//}
						}
					}
					
					// if is a directory then
					d_parent = dentry->d_parent;
					if (!d_parent) return 0;

					//memset(dir, 0, MAX_LEN);
					//p = dentry_path_raw(d_parent, dir, MAX_LEN);

					if (!is_black_listed(d_parent)){
						atomic_inc((atomic_t*)&audit_counter);
						if (filp != NULL){
							if (filp->f_flags & (O_CREAT | O_TRUNC | O_APPEND)){
								pr_err("%s: file cannot be created on this directory=%s cannot be modified\n", REFERENCE_HOOK, d_parent->d_name.name);
								regs->si = (unsigned long)(NULL);
								regs->di = (unsigned long)(NULL);	
								goto store_thread_info_vfs_open;	
							}
						}
					}		          	

        	}

	}
	return 0;

	store_thread_info_vfs_open:
	AUDIT
	pr_info("hook: calling work queue");
	defer_top_half(defer_bottom_half);
	return 0;
}



int vfs_mkrmdir_unlink_wrapper(struct kprobe *ri, struct pt_regs * regs)
{
    //struct inode *d = (struct inode *)regs->di;
	struct dentry *dentry = (struct dentry *)regs->si;
	struct dentry *d_parent;

	if (dentry){
		d_parent = dentry->d_parent;
			if (!is_black_listed(d_parent)){
				atomic_inc((atomic_t*)&audit_counter);
				pr_err("%s: cannot modify directory in this for directory %s\n", REFERENCE_HOOK, d_parent->d_name.name);
				regs->si = (unsigned long)(NULL);
				regs->di = (unsigned long)(NULL);	
			}
	}
	
	return 0;
}


