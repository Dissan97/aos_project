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
#include <linux/syscalls.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Francesco Quaglia <quaglia@dis.uniroma1.it>");
MODULE_DESCRIPTION("system call table hacker");

#define DEFER "DEFER"


int change_state(unsigned long state, unsigned long *the_state){
	int ret = 0;
	
	printk("%s: change_state system call actual %ld -requested %ld\n", DEFER, *the_state, state);

	if (state == *the_state) goto exit_change_state;

	if (state < 0 || state > 3){
		printk("%s: requsted bad state - not available\n", DEFER);
		ret = -1;
		goto exit_change_state;
	}
		
	*the_state = state;

exit_change_state:
	return ret;
}
int sys_two(void){
	printk("%s: two", DEFER);
	return 0;
}
