

#include <linux/fs.h>

#define SHA512_LENGTH 64 
#define SALT_LENGTH 16 // Define the length of the salt



int generate_salt(char *);
int salt_text(char *, char *, char **);
int hash_plaintext(char *, char*, char*);
int hash_plain_file(char *,struct file*, struct file*);
