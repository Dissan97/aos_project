#pragma once
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/rculist.h>
#include <linux/fs.h>

struct rcu_restrict {
  char *path;
  struct dentry *dentry;
  struct list_head node;
  struct rcu_head rcu;
};

extern spinlock_t restrict_path_lock;
extern struct list_head restrict_path_list;
int add_path(char *);
int del_path(char *);
int is_path_in(char *);

