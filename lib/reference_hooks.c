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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dissan Uddin Ahmed <dissanahmed@gmail.com>");

#define VFS_WRAPPER "VFS OPEN WRAPPER"
#define bad_dir "prova.aoo"

#define MAX_LEN 256


int vfs_open_wrapper(struct kprobe *ri, struct pt_regs * regs)
{
        struct path* path = (struct path*) (regs->di);
        struct dentry *dentry;
        struct file *file = (struct file *)regs->si;
        int flags;
        char dir[MAX_LEN];
        char *p;
        
        if (path != NULL){
                if (path->dentry != NULL){
	               dentry = path->dentry;
                	if (!strncmp(dentry->d_name.name, bad_dir, strlen(bad_dir))){
                        	//printk("%s: found vfs_open on dir=foo\n", bad_dir);
                        	atomic_inc((atomic_t*)&audit_counter);
                        	p=dentry_path_raw(dentry, dir, MAX_LEN);
                        	if (p){
                          		printk("%s: path=%s chiamato=%ld\n", bad_dir, p, audit_counter);
                          		if (dentry->d_inode->i_mode & S_IRUSR){
                          			printk("foo: possiamo leggere\n");
                          		}if (dentry->d_inode->i_mode & S_IWUSR){
                          			printk("Possiamo: scrivere\n");
                          			dentry->d_inode->i_mode ^= S_IWUSR;
                          		}
					//regs->si = (unsigned long)NULL;
							return 0;
                         
					}		
				}

        	}
        }
        
        return 0;


        return 0;
}

void vfs_open_post_wrapper(struct kprobe *ri, struct pt_regs *regs, unsigned long l_param)
{
        return;
}
