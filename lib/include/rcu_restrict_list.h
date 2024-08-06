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
  unsigned long i_ino;
  struct list_head node;
  struct rcu_head rcu;
};


extern spinlock_t restrict_path_lock;
extern struct list_head restrict_path_list;
//wait free

int forbitten_path(const char *);
int add_path(char *);
int del_path(char *);
int is_black_listed(const char *path, unsigned long);



