obj-m += the_reference_monitor.o
the_reference_monitor-objs += reference_monitor.o lib/scth.o lib/defer.o lib/hook.o lib/reference_syscalls.o lib/rcu_restrict_list.o lib/reference_hash.o lib/reference_path_based_syscall.o

hacked_sys_table_addr = $(shell cat /sys/module/the_usctm/parameters/sys_call_table_address)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

mount:
	insmod the_reference_monitor.ko the_syscall_table=$(hacked_sys_table_addr) pwd='DummyPwd123!!!'

unmount:
	rmmod the_reference_monitor

test:
	gcc ./user/user.c -o ./user/test
	./user/test
