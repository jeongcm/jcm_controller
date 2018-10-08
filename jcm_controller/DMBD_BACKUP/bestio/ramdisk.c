#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/hdreg.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <asm/uaccess.h>
#include "ramdisk.h"

#define VRD_DEV_NAME           "vrd"
#define VRD_DEV_MAJOR          240

#define VRD_SECTOR_SIZE        512
#define VRD_SIZE              device.size 
#define VRD_SECTOR_TOTAL     (VRD_SIZE/VRD_SECTOR_SIZE)
#define VRD_MAX_PARTITON       5

typedef struct
{
	unsigned char             *data;
	struct request_queue    *queue;
	struct gendisk              *gd;

	unsigned long long size;
} vrd_device;
static void vrd_default_hook_request(struct bio* bio)
{
}

vrd_device                    device;
static vrd_hook_fn vrd_hook_request = vrd_default_hook_request;

unsigned char *vrd_get_bio_address(struct bio *bio)
{
	return device.data + (bio->bi_iter.bi_sector * VRD_SECTOR_SIZE);
}
EXPORT_SYMBOL(vrd_get_bio_address);

void vrd_set_hook_request(vrd_hook_fn func) 
{
	vrd_hook_request = func;
}

static blk_qc_t  vrd_make_request(struct request_queue *q, struct bio *bio)
{
	vrd_device 		*pdevice;
	char       		*pVHDDData;
	char       		*pBuffer;
	struct bio_vec        	bvec;
	struct bvec_iter 	b_iter;

	if ( ( (bio->bi_iter.bi_sector * VRD_SECTOR_SIZE) + bio->bi_iter.bi_size ) > VRD_SIZE )
		goto fail;

	pdevice = (vrd_device *)bio->bi_bdev->bd_disk->private_data;

	pVHDDData = pdevice->data + (bio->bi_iter.bi_sector*VRD_SECTOR_SIZE);
	//vrd_hook_request(bio);
	bio_for_each_segment(bvec, bio, b_iter)
	{
		pBuffer = kmap(bvec.bv_page) + bvec.bv_offset;
		switch(bio_data_dir(bio))
		{
		//case READA :
		case READ :
			memcpy(pBuffer, pVHDDData, bvec.bv_len);
			break;
		case WRITE :
			memcpy(pVHDDData, pBuffer, bvec.bv_len);
			break;
		default :
			kunmap(bvec.bv_page);
			goto fail;
		}
		kunmap(bvec.bv_page);
		pVHDDData += bvec.bv_len;
	}

	bio_endio(bio);
	return 0;
fail :
	bio_io_error(bio);
	return 0;
}

static int vrd_open(struct block_device *inode, fmode_t filp)
{
	return 0;
}

static void vrd_release(struct gendisk *inode, fmode_t filp)
{
}

static int vrd_ioctl(struct block_device *inode, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	return -ENOTTY;
}

static struct block_device_operations vrd_fops =
{
	.owner    = THIS_MODULE,
	.open      = vrd_open,
	.release   = vrd_release,
	.ioctl        = vrd_ioctl,
};

int vrd_init(char *name, unsigned long long size)
{
	printk("ramdisk name : %s, size %llu\n", name, size);
	device.size = size;
	register_blkdev(VRD_DEV_MAJOR, VRD_DEV_NAME);

	device.data    = vmalloc(VRD_SIZE);
	device.gd      = alloc_disk(VRD_MAX_PARTITON);
	device.queue = blk_alloc_queue(GFP_KERNEL);
	blk_queue_make_request(device.queue, &vrd_make_request);

	device.gd->major         = VRD_DEV_MAJOR;
	device.gd->first_minor   = 0;
	device.gd->fops           = &vrd_fops;
	device.gd->queue        = device.queue;
	device.gd->private_data = &device;
	sprintf(device.gd->disk_name, "%s%c", name, 'a');
	set_capacity(device.gd, VRD_SECTOR_TOTAL);

	add_disk(device.gd);
	printk("vrd_init done\n");
	return 0;
}

void vrd_exit(void)
{
	del_gendisk(device.gd);
	put_disk(device.gd);
	blk_cleanup_queue(device.queue);
	unregister_blkdev(VRD_DEV_MAJOR, VRD_DEV_NAME);
	printk("vrd_Exit done\n");

	vfree(device.data);
}
MODULE_LICENSE("GPL");
EXPORT_SYMBOL(device);
