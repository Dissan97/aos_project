#define EXPORT_SYMTAB


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>	
#include <linux/pid.h>		/* For pid types */
#include <linux/tty.h>		/* For the tty declarations */
#include <linux/version.h>	/* For LINUX_VERSION_CODE */
#include <linux/signal_types.h>
#include <linux/syscalls.h>
#include <linux/sched/mm.h>

#include "include/reference_path_based_syscall.h"
#include "include/reference_syscalls.h"
#include "include/reference_types.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dissan Uddin Ahmed <DissanAhmed@gmail.com>");


char *list_sys_call[3] = {CHANGE_STATE, CHANGE_PATH, NULL};
int check_avilable_sys_list(char *sys_name){
	
	int i;
	int len;
	int ret = -1;
	/*if (sys_name == NULL){
		return -EINVAL;
	}*/
    pr_info("%s: request for this: %s", DEVICE_SYS_VFS, sys_name);	
	len = strlen(sys_name);
	
	for (i = 0; list_sys_call[i] != NULL; i++){
		pr_info("%s: comparing %s - with %s\n", DEVICE_SYS_VFS, list_sys_call[i], sys_name);
		if (!strncmp(list_sys_call[i], sys_name, strlen(sys_name))){
			pr_info("%s: found %s on avilable syscall\n", DEVICE_SYS_VFS, sys_name);
			return i;	
                        
		}
		ret ++;
	}
	
	return -EBADMSG;
	
}

int parse_opt(char *op_str){

	int i;
    int j;
	char *argv[9];    
	size_t len;
	int format = 0;
	char c;
	int ret;
	len = strlen(op_str);
	
	for (i = 0; i < len; i++){
		c = op_str[i];
		
		if (c >= 'a' && c <= 'z'){
			op_str[i] -= 32;
		} 
	}
	pr_info("%s: start parsing this %s\n",DEVICE_SYS_VFS, op_str);
	j = 1;
	for(i=0;i<len;i++){
		if(op_str[i] == '|') {
			op_str[i] = '\0';
			argv[j++] = op_str + i + 1;
			format ++;
		}
	}
	argv[0] = op_str;
	argv[j] = NULL;
	ret = 0;
	for (i = 0; argv[i] != NULL; i++){
		len = strlen(argv[i]);
		if (!strncmp(argv[i], __on, len)){
			pr_info("%s: now in ON\n", DEVICE_SYS_VFS);
			ret |= ON;
		}
		else if (!strncmp(argv[i], __off, len)){
			pr_info("%s: now in OFF\n", DEVICE_SYS_VFS);
			ret |= OFF;		
		}
		else if (!strncmp(argv[i], __rec_on, len)){
			pr_info("%s: now in REC_ON\n", DEVICE_SYS_VFS);
			ret |= REC_ON;		
		}
		else if (!strncmp(argv[i], __rec_off, len)){
			pr_info("%s: now in REC_OFF\n", DEVICE_SYS_VFS);
			ret |= REC_OFF;		
		}
		else if (!strncmp(argv[i], __add_path, len)){
			pr_info("%s: now in ADD_PATH\n", DEVICE_SYS_VFS);
			ret |= ADD_PATH;		
		}
		else if (!strncmp(argv[i], __remove_path, len)){
			pr_info("%s: now in REMOVE_PATH\n", DEVICE_SYS_VFS);
			ret |= REMOVE_PATH;		
		}
		pr_info("%s: argv[i]=%s -> ret =%d\n", DEVICE_SYS_VFS, argv[i], ret);
	}
	
	
	return ret;
}

int change_state_vfs_sys_wrapper(char *argv[], int format){
	
	char *pwd = NULL;
	int op = -1;
	int i;

    pr_info("%s: got request for %s\n", DEVICE_SYS_VFS, CHANGE_STATE);

	if (format != 4){
		return -EBADMSG;
	}
		

	i = 0;
	while (argv[i] != NULL){
		if (!strncmp(argv[i], PWD, 4)){
			pwd = argv[i + 1];
		}
		if (!strncmp(argv[i], OPT, 4)){
			op = parse_opt(argv[i + 1]);
		}
		i++;
	}
        
    pr_info("%s: ready to call actual syscall sys_change_state with pwd=%s opt=%d\n", DEVICE_SYS_VFS, pwd, op);
	return do_change_state(pwd, op);	
	
}

int change_path_vfs_sys_wrapper(char *argv[], int format){

	char *pwd = NULL;
	char *pathname = NULL;
	int op = -1;
	int i;
	
	if (format != 6){
		return -EBADMSG;
	}

	i = 0;
	while (argv[i] != NULL){
		if (!strncmp(argv[i], PWD, 4)){
			pwd = argv[i + 1];
		}
		if (!strncmp(argv[i], PTH, 4)){
			pathname = argv[i + 1];
		}
		if (!strncmp(argv[i], OPT, 4)){
			op = parse_opt(argv[i + 1]);
		}
		i++;
	}
	pr_info("%s: calling the sys_change_path with pwd=%s, path=%s, op=%d\n", DEVICE_SYS_VFS,
	pwd, pathname, op);
	return do_change_path(pwd, pathname, op);
}


ssize_t vfs_sys_wrapper_wrt(struct file *filp, const char *buff, size_t len, loff_t *off)
{

	char buffer[LINE_SIZE];
	int i;
    int j;
	int ret;
	char *argv[9];
	int format = 0;
	
	//cngpth -pwd password -opt ADD_PATH -pth pathname
	//cngst -pwd password -opt OFF
        

        
	if(len >= LINE_SIZE) return -1;
	
	ret = copy_from_user(buffer,buff,len);
	buffer[len] = '\0';
	j = 1;

        pr_info("%s: got this: %s\n", DEVICE_SYS_VFS, buffer);
	
	for (i = 0; i < len; i++){
		if (buffer[i] == '\n'){
			buffer[i] = '\0';
		}
	}

	for(i=0;i<len;i++){
		if(buffer[i] == ' ') {
			buffer[i] = '\0';
			argv[j++] = buffer + i + 1;
			format++;
		}
	}

  	argv[j] = NULL;

  	argv[0] = buffer;
        
        pr_info("%s: requested syscall %s checking availability\n", DEVICE_SYS_VFS, argv[0]);

	ret = check_avilable_sys_list(argv[0]);

	if (ret < 0){
		return -EBADMSG;
	}
	
	if (ret == 0){
		ret = change_state_vfs_sys_wrapper(&argv[1], format);
		if (ret < 0){
			return ret;
		}
		
	}
	
	if (ret == 1){
		ret = change_path_vfs_sys_wrapper(&argv[1], format);
		 if(ret){
		 	return ret;
		 }
	}
	

	
	printk("%s: get this from user %s\n", DEVICE_SYS_VFS, buffer);

	return (ssize_t)len;
}

ssize_t vfs_sys_wrapper_rd(struct file *filp, char *buff, size_t len, loff_t *off)
{
        return -1;
}

int vfs_sys_wrapper_opn(struct inode* i_node, struct file* f)
{
	return 0;
}

int vsf_sys_wrapper_rls(struct inode* i_node, struct file* f)
{
	return 0;
}

