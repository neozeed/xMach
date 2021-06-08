#ifndef _SYS_DEVFS_H
#define _SYS_DEVFS_H

void devfs_register(char* name, int is_block, dev_t dev);
void devfs_unregister(char* name);

#endif /* _SYS_DEVFS_H */
