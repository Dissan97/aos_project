#pragma once
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/rculist.h>

struct rcu_restrict_path_list {
  char *pathname;
  struct list_head list;
};

#define init_restrict_list(name) LIST_HEAD(name)

int add_path(char *, struct list_head*);
int del_path(char *, struct list_head*);
int is_path_in(char *pathname, struct list_head*);

void safe_delele_list(struct list_head*);
