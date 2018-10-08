#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gfp.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <linux/blk_types.h>
#include <linux/blkdev.h>  //블록 디바이스 관련 자료 구조와 함수
#include <asm/uaccess.h>
#include <linux/fcntl.h>
#include <linux/types.h>
#include <linux/bio.h>
#include <linux/list.h>

#define DEVICE_SECTOR_SIZE 512
char first_dev[16];
char second_dev[16];
module_param_string(dev1,first_dev,sizeof(first_dev),0);
module_param_string(dev2,second_dev,sizeof(second_dev),0);


struct root_device_t{
	struct request_queue *queue;
	struct gendisk *gd;
	unsigned int total_size;
	struct list_head total_list;
}root_device;

struct dev_list{
	unsigned long dev_size;
	struct block_device *bdev;
	struct list_head list;
};

struct root_device_t root_device;
struct bio_set *bs=NULL; 
struct dev_list *dlist, *back_list,*jcm_list;
struct dev_list *lh;
static blk_qc_t device_make_request(struct request_queue *q,struct bio *bio)
{
	struct bio *split=NULL;
	struct bio *split_tmp=NULL;
	sector_t before_sector=0;

	if((bio->bi_iter.bi_sector*DEVICE_SECTOR_SIZE)+bio->bi_iter.bi_size> root_device.total_size){
		goto fail;
	}	

	// select device
	list_for_each_entry(lh, &root_device.total_list,list){
		if(bio->bi_iter.bi_sector >=lh->dev_size+before_sector)
		{
			before_sector+=lh->dev_size;
			continue;
		}
		else if(bio->bi_iter.bi_sector < lh->dev_size+before_sector)
		{
			if (bio_end_sector(bio)>lh->dev_size+before_sector)
			{
				split=bio_split(bio,lh->dev_size+before_sector,GFP_NOIO,bs);
				bio->bi_bdev = lh->bdev;
				bio->bi_iter.bi_sector-= before_sector;
				generic_make_request(bio);
				bio=split;
				before_sector+=lh->dev_size;
				continue;
			}
			else
			{
				bio->bi_iter.bi_sector-= before_sector;
				bio->bi_bdev = lh->bdev;
				generic_make_request(bio);
				break;
			}
		}
	}
	
	// request

	
	return 0;
fail:
	bio_io_error(bio);
	return 0;

}


static int device_open(struct block_device *inode, fmode_t filp)
{
	return 0;
}

static void device_release(struct gendisk *inode, fmode_t filp)
{


}

static int device_ioctl(struct block_device *inode, fmode_t filp, unsigned cmd, unsigned long arg)
{
	return -EIO;
}


static struct block_device_operations bdops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_release,
	.ioctl = device_ioctl
};

static int device_init(void)
{
	//dlist = kmalloc(sizeof(struct dev_list), GFP_ATOMIC);
	back_list = kmalloc(sizeof(struct dev_list), GFP_ATOMIC);
	jcm_list = kmalloc(sizeof(struct dev_list), GFP_ATOMIC);
	//dlist->bdev = blkdev_get_by_path("/dev/sdc", FMODE_READ|FMODE_WRITE,NULL);
	//dlist->dev_size = get_capacity(dlist->bdev->bd_disk);
	back_list->bdev = blkdev_get_by_path("/dev/nbd0", FMODE_READ|FMODE_WRITE,NULL);
	back_list->dev_size = get_capacity(back_list->bdev->bd_disk);
	jcm_list->bdev = blkdev_get_by_path("/dev/bestioa", FMODE_READ|FMODE_WRITE,NULL);
	jcm_list->dev_size = get_capacity(jcm_list->bdev->bd_disk);
	INIT_LIST_HEAD(&root_device.total_list);
	//list_add_tail(&dlist->list,&root_device.total_list);
	list_add_tail(&jcm_list->list,&root_device.total_list);	
	list_add_tail(&back_list->list,&root_device.total_list);	
	root_device.total_size =0;
	list_for_each_entry(lh, &root_device.total_list, list){
		root_device.total_size += (lh->dev_size*DEVICE_SECTOR_SIZE);
	}
	register_blkdev(250, "vrd");

	root_device.gd = alloc_disk(1);
	root_device.queue = blk_alloc_queue(GFP_KERNEL);
	blk_queue_make_request(root_device.queue, &device_make_request);

	root_device.gd->major = 250;
	root_device.gd->first_minor=0;
	root_device.gd->fops = &bdops;
	root_device.gd->queue = root_device.queue;
	root_device.gd->private_data = &root_device;
	sprintf(root_device.gd->disk_name,"%s%c","jcm",'a');
	set_capacity(root_device.gd, root_device.total_size/DEVICE_SECTOR_SIZE);
	bs= bioset_create(BIO_POOL_SIZE,0,BIOSET_NEED_BVECS);
	//bs= bioset_create(BIO_POOL_SIZE,0);//,BIOSET_NEED_BVECS);
	printk("vrd init\n");
	add_disk(root_device.gd);


	return 0;
}
void device_exit(void)
{
	del_gendisk(root_device.gd);
	put_disk(root_device.gd);
	unregister_blkdev(250, "vrd");
	//kfree(dlist);
	printk("vrd exit");
	return;
}

module_init(device_init);
module_exit(device_exit);
MODULE_LICENSE("GPL");

