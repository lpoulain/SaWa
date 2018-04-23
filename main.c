/////////////////////////////////////////////////////////////////////
// The part which implements the Block Driver
/////////////////////////////////////////////////////////////////////

#include <linux/init.h>       /* module_init, module_exit */
#include <linux/module.h> /* version info, MODULE_LICENSE, MODULE_AUTHOR, printk() */
#include <linux/fs.h> 	     /* file stuff */
#include <linux/kernel.h>    /* printk() */
#include <linux/errno.h>     /* error codes */
#include <linux/module.h>  /* THIS_MODULE */
#include <linux/cdev.h>      /* char device stuff */
#include <linux/uaccess.h>  /* copy_to_user() */
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/socket.h>
#include <linux/netdevice.h>
#include <linux/tcp.h>
#include <linux/in.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include "sawa.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Laurent Poulain");

int blockdev_getgeo(struct block_device * block_device, struct hd_geometry * geo);

static unsigned int major_number;
static const char *device_name = "SaWa";

static struct net_dev {
    int nb_sectors;
    spinlock_t lock;
    struct request_queue *queue;
    struct gendisk *gd;
} sawa_dev;

static struct block_device_operations fops = 
{
    .owner   = THIS_MODULE,
    .getgeo  = blockdev_getgeo
};


/* ===================================================================*/
static struct request_queue *the_queue = NULL;
static struct request *current_req = NULL;

static int blockdev_transfer(sector_t sector,
		unsigned long nsect, char *buffer, int write) {
//    printk(KERN_INFO "SaWa: Request for %lu sectors starting at sector #%lu (write=%d)\n", nsect, sector, write);
    
    if (write == 0) {
        return sawa_read_data(sector, nsect, buffer);
    }
    else  {
        return sawa_write_data(sector, nsect, buffer);
    }
}

static int set_next_request(void)
{
    if (!the_queue) {
        current_req = 0;
        return 0;
    }
    
    while (1) {
        current_req = blk_fetch_request(the_queue);
    
        if (current_req == NULL) return 0;
    
        if (current_req->cmd_type != REQ_TYPE_FS) {
			printk (KERN_NOTICE "Skip non-CMD request\n");
			__blk_end_request_all(current_req, -EIO);
			continue;
		}

        return 1;
    }
}

static void work_handler(struct work_struct *work) {
    unsigned long saved_flags;
    int ret;
    
    while (1) {
        ret = blockdev_transfer(blk_rq_pos(current_req), blk_rq_cur_sectors(current_req),
                    current_req->buffer, rq_data_dir(current_req));

        spin_lock_irqsave(&sawa_dev.lock, saved_flags);
            // After __blk_end_request_cur(), the next request
            // might be on the same object
            if (!__blk_end_request_cur(current_req, ret)) set_next_request();
        spin_unlock_irqrestore(&sawa_dev.lock, saved_flags);
        if (current_req == NULL) return;
    }
}

static DECLARE_DELAYED_WORK(work, work_handler);

static void blockdev_request(struct request_queue *q) {
    the_queue = q;
    
    // There is already a request being worked on, quit
    if (current_req) return;
    
    // Get the next request
    set_next_request();
//	current_req = blk_fetch_request(q);
	if (current_req == NULL) return;
//    printk(KERN_NOTICE "New request %px\n", current_req);

	schedule_delayed_work(&work, 0);
}

int blockdev_getgeo(struct block_device * block_device, struct hd_geometry * geo) {
	long size;

    printk(KERN_INFO "GetGeo called\n");
	/* We have no real geometry, of course, so make something up. */
	size = nb_sectors;
	geo->cylinders = 64; //(size & ~0x3f) >> 6;
	geo->heads = 1;
	geo->sectors = nb_sectors;
	geo->start = 0;
	return 0;
}

/*===============================================================================================*/
static int sawa_driver_init(void)
{
    int ret;
    
    printk( KERN_NOTICE "SaWa: Initialization started" );
    
    spin_lock_init(&sawa_dev.lock);
    
    ret = sawa_send_info();

    if (ret < 0) {
        printk(KERN_WARNING "SaWa: Did not receive the nubmer of sectors from SaWa\n");
        return -1;
    }
    
    memset (&sawa_dev, 0, sizeof (struct net_dev));
    sawa_dev.nb_sectors = nb_sectors;
    sawa_dev.queue = blk_init_queue(blockdev_request, &sawa_dev.lock);
    //sawa_dev.queue = blk_init_queue(ramdisk_request, &sawa_dev.lock);
    
    if (sawa_dev.queue == NULL) return -ENOMEM;
    blk_queue_logical_block_size(sawa_dev.queue, 512);
    major_number = register_blkdev( 0, device_name);
    if (major_number < 0) {
		printk(KERN_WARNING "SaWa: unable to get major number\n");
		return -ENOMEM;
	}
    
    sawa_dev.gd = alloc_disk(nb_sectors);
    
    if (!sawa_dev.gd) {
        printk (KERN_NOTICE "alloc_disk failure\n");
        unregister_blkdev(major_number, device_name);
        return -ENOMEM;
    }
    sawa_dev.gd->major = major_number;
    sawa_dev.gd->first_minor = 0;
    sawa_dev.gd->fops = &fops;
    sawa_dev.gd->private_data = &sawa_dev;
    strcpy(sawa_dev.gd->disk_name, "sawa0");
    set_capacity(sawa_dev.gd, nb_sectors);
    sawa_dev.gd->queue = sawa_dev.queue;
    add_disk(sawa_dev.gd);
    
    return 0;
}
/*-----------------------------------------------------------------------------------------------*/
static void sawa_driver_exit(void)
{
    printk( KERN_NOTICE "SaWa: Exiting" );
    
    if (sawa_dev.gd != NULL) {
        del_gendisk(sawa_dev.gd);
        put_disk(sawa_dev.gd);
    }
    unregister_blkdev(major_number, device_name);
	if (sawa_dev.queue != NULL) blk_cleanup_queue(sawa_dev.queue);
//    if (sock != NULL) sock_release(sock);
}
/*===============================================================================================*/

module_init(sawa_driver_init);
module_exit(sawa_driver_exit);
