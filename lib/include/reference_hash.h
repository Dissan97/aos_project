

#include <linux/fs.h>




int generate_salt(char *);
int salt_text(char *, char *, char **);
int hash_plaintext(char *, char*, char*);
int hash_plain_file(char *,struct file*, struct file*);
