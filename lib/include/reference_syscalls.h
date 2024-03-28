#pragma once

#include "reference_rcu_restrict_list.h"

extern struct list_head restrict_path_list;

int do_change_state(char*, int);
int do_change_path(char *,char *, int);
