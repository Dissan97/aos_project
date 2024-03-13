#pragma once
#include <linux/kprobes.h>

extern unsigned long audit_counter;
int vfs_open_wrapper(struct kprobe*, struct pt_regs *);
void vfs_open_post_wrapper(struct kprobe *, struct pt_regs *, unsigned long);

