#ifndef __RAMDISK__
#define __RAMDISK__
#include <linux/bio.h>

int vrd_init(char *name, unsigned long long size);
void vrd_exit(void);
typedef void (*vrd_hook_fn)(struct bio *bio);
void vrd_set_hook_request(vrd_hook_fn func);
unsigned char *vrd_get_bio_address(struct bio *bio);

#endif
