#pragma once
#include <linux/fs.h>
#define ON 0x01             // 0000 0001
#define OFF 0x02            // 0000 0010 
#define REC_ON 0x04         // 0000 0100
#define REC_OFF 0x08        // 0000 1000
#define ADD_PATH 0x10       // 0001 0000
#define REMOVE_PATH 0x20    // 0010 0000
#define HOOKS_SIZE 4
#define MAX_PATH_LEN PAGE_SIZE
#define SHA512_LENGTH (1 << 6) 
#define SALT_LENGTH (1 << 4) // Define the length of the salt
#pragma once

#define MODNAME "REFERENCE MONITOR"
#define PATH_DEVICE_NAME "REFERENCE PATH DEVICE"
#define CACHE_DEVICE_NAME "REFERENCE CACHE DEVICE"
#ifndef AUDIT
#define AUDIT if(1)
#endif


extern spinlock_t reference_state_spinlock;
extern struct file *filp_dummy;

extern unsigned long current_state;
extern unsigned int admin;
extern char salt[(SALT_LENGTH + 1)];

