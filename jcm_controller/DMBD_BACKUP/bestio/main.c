#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/wait.h>
#include "ramdisk.h"
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#endif
#define BRK printk("bestio FILE : %s,  FUNCTION : %s, LINE : %d\n", __FILE__, __func__ , __LINE__)

struct backup_device {
	struct block_device *bdev;
	unsigned int major;
	unsigned int minor;
	unsigned long dev_size;
};

struct backup_list {
	struct bio *bio;
	struct list_head list;
};

struct backup {
	struct backup_device device;
	struct backup_list *queue;
	struct task_struct *thread_task;

	struct semaphore q_sem_mutex;

	wait_queue_head_t kq_wait_bestio;
	unsigned long bestio_condition;
};


struct backup backup;

static unsigned long memory_size = 0;
static unsigned int back_major = 0;
static unsigned int back_minor = 0;
module_param(memory_size, long, 0);
module_param(back_major, int, 0);
module_param(back_minor, int, 0);

static void bestio_backup_end_request(struct bio *bio)
{
	int i, nr_page;
	complete((struct completion *)bio->bi_private);

	nr_page = bio->bi_vcnt;
	for(i=0;i < nr_page; i++){
		if(bio->bi_io_vec[i].bv_page != NULL)
			__free_page(bio->bi_io_vec[i].bv_page);
	}           
	bio_put(bio);
}

// hook and producer
void bestio_backup_bio(struct bio *bio)
{
	struct   bio  *clone_bio;
	unsigned char *bio_address;
	struct bio_vec bvec;
	struct backup_list *q_data;
	struct backup_list *back_q = backup.queue;
	struct bvec_iter b_iter;
	int i =0;

	bio_address = vrd_get_bio_address(bio);
	if (bio_data_dir(bio) == WRITE) {
		clone_bio = bio_alloc(GFP_NOIO, bio->bi_vcnt);
		clone_bio->bi_iter.bi_sector = bio->bi_iter.bi_sector;
		clone_bio->bi_bdev = backup.device.bdev;
		clone_bio->bi_opf = bio->bi_opf;
		clone_bio->bi_flags |= 1 << 0;
		clone_bio->bi_vcnt = bio->bi_vcnt;
		clone_bio->bi_iter.bi_idx = bio->bi_iter.bi_idx;
		clone_bio->bi_iter.bi_size = bio->bi_iter.bi_size;
		clone_bio->bi_end_io = bestio_backup_end_request;     
		bio_for_each_segment(bvec, bio, b_iter){
			clone_bio->bi_io_vec[i].bv_len = bvec.bv_len;
			clone_bio->bi_io_vec[i].bv_offset = bvec.bv_offset;
			clone_bio->bi_io_vec[i].bv_page = (struct page*) bio_address;
			bio_address += bvec.bv_len;
			i++;
		}

		q_data = kmalloc(sizeof(struct backup_list), GFP_ATOMIC);

		q_data->bio = clone_bio;
		down(&backup.q_sem_mutex);
		list_add_tail(&q_data->list, &back_q->list);
		up(&backup.q_sem_mutex);
		
		if(!test_and_set_bit(0x01, &backup.bestio_condition)) {
			wake_up_interruptible(&backup.kq_wait_bestio);
		}
	}

}

int bestio_q_bio(struct bio *bio, struct backup *meta_data)
{
	int i = 0;
	struct page *page = NULL;
	char *mem_address;
	int nr_page;
	char *kmap_address;
	//unsigned long dflags = 0;
	struct completion *complete;
	complete = kmalloc(sizeof(struct completion), GFP_ATOMIC);
	init_completion(complete);

	nr_page = bio->bi_vcnt;

	for(;i < nr_page; i++){
		mem_address = (char *)bio->bi_io_vec[i].bv_page;
		if(mem_address == NULL){
			printk("[BESTIO] bio address fail\n");
			return 1;
		}
		page = alloc_page(GFP_NOIO);
		bio->bi_io_vec[i].bv_page = page;
		if(bio->bi_io_vec[i].bv_page  == NULL){
			printk("[BESTIO] backup page alloc fail\n");
			return 1;
		}
		kmap_address = kmap(bio->bi_io_vec[i].bv_page)
			+ bio->bi_io_vec[i].bv_offset;

		if(kmap_address == NULL){
			printk("[BESTIO] kmap fail\n");
			return 1;
		}
		memcpy(kmap_address, mem_address, bio->bi_io_vec[i].bv_len);
		kunmap(bio->bi_io_vec[i].bv_page);
	}

	//    spin_lock_irqsave(&meta_data->end_req_lock, dflags);
	//  meta_data->req_count++;
	//   spin_unlock_irqrestore(&meta_data->end_req_lock, dflags);

	bio->bi_private = complete;
	submit_bio(bio);
	wait_for_completion(complete);

	kfree(complete);

	return 0;
}

// consumer
static int bestio_backup_thread(void * data)
{
	struct backup *meta_data = (struct backup*)data;
	struct backup_list *back_q = meta_data->queue;
	struct backup_list *q_data, *tmp;

	allow_signal(SIGKILL);
	set_current_state(TASK_INTERRUPTIBLE);
	do{
		down(&backup.q_sem_mutex);
		if (list_empty(&back_q->list)) {
			up(&backup.q_sem_mutex);
			wait_event_interruptible_timeout(meta_data->kq_wait_bestio, meta_data->bestio_condition, HZ);
			clear_bit(0x01, &meta_data->bestio_condition);
		} else {

			list_for_each_entry_safe(q_data, tmp,
						 &back_q->list, list){
				if(q_data != NULL) {
					break;
				}
			}

			if(q_data != NULL) {
				list_del(&q_data->list);
			}
			up(&backup.q_sem_mutex);

			bestio_q_bio(q_data->bio, meta_data);
			kfree(q_data);
		}
	}while(!kthread_should_stop());
	return 0;
}

static void backup_init(void)
{
	dev_t dev;
	struct backup_device *device = &backup.device;
	//FIXME error when not settinged
	device->major = back_major;
	device->minor = back_minor;
	dev = MKDEV(back_major, back_minor);
	device->bdev  = blkdev_get_by_dev(dev, FMODE_READ|FMODE_WRITE, NULL);
	device->dev_size  = device->bdev->bd_inode->i_size;

	backup.queue = kmalloc(sizeof(struct backup_list), GFP_KERNEL);
	INIT_LIST_HEAD(&backup.queue->list);

	init_waitqueue_head(&backup.kq_wait_bestio);

	//sema_init(&backup.q_sem_empty, 0);
	sema_init(&backup.q_sem_mutex, 1);

	backup.thread_task = kthread_run(bestio_backup_thread, &backup, "krtbackd");

	if(IS_ERR(backup.thread_task)) {
		printk("THREAD ERROR\n");
		return ;
	}

	vrd_set_hook_request(bestio_backup_bio);
	return;
}

static int my_init(void)
{
	printk("memory size %lu\n", memory_size);
	vrd_init("bestio", memory_size*1024*1024);
	//backup_init();
	return  0;
}

int backup_clear_queue(struct backup_list *queue)
{
	struct backup_list *tmp;
	struct list_head *pos, *q;
	list_for_each_safe(pos, q, &queue->list){
		tmp = list_entry(pos, struct backup_list, list);
		//TODO hello comments
		//bestio_q_bio(q_data->clone_bio, meta_data);
		list_del(pos);
		kfree(tmp);
	}
	kfree(queue);
	return 0;
}

int backup_exit(void)
{
	if(!IS_ERR(backup.thread_task)) {
		kthread_stop(backup.thread_task);
	}

	backup_clear_queue(backup.queue);

	blkdev_put(backup.device.bdev, 0); 

	return 0;
}

static void my_exit(void)
{
	//backup_exit();
	vrd_exit();
	return;
}

module_init(my_init);
module_exit(my_exit);
