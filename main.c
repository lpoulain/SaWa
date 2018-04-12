// This part does not work properly yet.
// The block device driver works fine, but it hangs the whole OS
// when tring to perform some TCP/IP operation in kernel mode

#include <linux/init.h>       /* module_init, module_exit */
#include <linux/module.h> /* version info, MODULE_LICENSE, MODULE_AUTHOR, printk() */
#include <linux/fs.h>
#include <linux/netdevice.h>
#include <linux/pci.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/socket.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <linux/slab.h>
#include <linux/interrupt.h>

MODULE_LICENSE("MIT");
MODULE_AUTHOR("Laurent Poulain");


/*===============================================================================================*/
// TCP/IP layer
/*===============================================================================================*/

#define SAWA_INFO  0x01
#define SAWA_READ  0x02
#define SAWA_WRITE 0x03


static unsigned int nb_sectors;
static int server_port = 5000;
//static int socket_fd, new_socket_fd;
static struct socket *sock = NULL;

void dump_mem(unsigned char *addr, int size) {
    int i, j=0;
    printk(KERN_INFO "Received %d bytes\n", size);
    while (1) {
        printk(KERN_INFO "%04x ", j);
        for (i=0; i<16; i++) {
            if (j >= size) {
                printk(KERN_INFO "\n");
                return;
            }
            printk(KERN_INFO " %02x", addr[j]);
            j++;
        }
        printk(KERN_INFO "\n");
    }
}

unsigned int inet_addr(char *str)
{
    int a, b, c, d;
    char arr[4];
    sscanf(str, "%d.%d.%d.%d", &a, &b, &c, &d);
    arr[0] = a; arr[1] = b; arr[2] = c; arr[3] = d;
    return *(unsigned int *)arr;
}

int sawa_connect(void) {
    struct sockaddr_in server_addr;
    int ret;
    
    ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);

    memset(&server_addr, '0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(server_port);

    if (sock->ops->connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr), O_RDWR) < 0) {
        printk(KERN_INFO "Could not connect to port 5000");
        return -1;
    }
    
    return 0;
}

void sawa_read_result(unsigned char *buffer_in, int expected_size) {
    int n, size=0, nb_bytes=-1, left = expected_size, read = 0;
//    unsigned char buffer_in[1025];
    struct msghdr msg;
    struct iovec iov;
    mm_segment_t oldfs;

    iov.iov_base=buffer_in + read;
    iov.iov_len=left;
    
	msg.msg_control=NULL;
	msg.msg_controllen=0;
	msg.msg_flags=0;
	msg.msg_name=0;
	msg.msg_namelen=0;
	msg.msg_iov=&iov;
	msg.msg_iovlen=1;
    
    oldfs = get_fs(); set_fs(KERNEL_DS);
    
//    while (left > 0 && nb_bytes != 0) {
        iov.iov_base=buffer_in + read;
	    iov.iov_len=left;
        
        nb_bytes = sock_recvmsg(sock, &msg, 1024, msg.msg_flags);
//        printk(KERN_INFO "SaWa: received %d bytes\n", nb_bytes);
        
        left -= nb_bytes;
        read += nb_bytes;
//    }
    
    set_fs(oldfs);
/*    for (;;) {
        iov.iov_base=buffer_in + size;
	    iov.iov_len=left;

        n = sock_recvmsg(sock, &msg, 160, 0);
        if (n == 0) {
//            dump_mem(buffer_in, size);
            return;
        }
        size += n;
        left -= n;
        if (size == expected_size) {
//            dump_mem(buffer_in, size);
            return;
        }
//        printf("0x%02x (%d bytes)\n", buffer_in[0], n);

    }
    */
}

int send_sync_buf(struct socket *sock, const char *buf, const size_t length, unsigned long flags)
{
    struct msghdr msg;
    struct iovec iov;
    int nb_bytes=0, written = 0, left = length;
    mm_segment_t oldmm;
    
    msg.msg_name     = 0;
    msg.msg_namelen  = 0;
    msg.msg_iov      = &iov;
    msg.msg_iovlen   = 1;
    msg.msg_control  = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags    = flags;

    oldmm = get_fs(); set_fs(KERNEL_DS);

//repeat_send:
    while (left > 0) {
        msg.msg_iov->iov_len = left;
        msg.msg_iov->iov_base = (char *) buf + written;

        nb_bytes = sock_sendmsg(sock, &msg, left);
        left -= nb_bytes;
        written += nb_bytes;
    }
    
    return written ? written : nb_bytes;
}

int sawa_send_info(void) {
    unsigned char buffer_out[12];
    unsigned char buffer_in[sizeof(int)];
    
    int *int_ptr = (int*)(buffer_out);

    *int_ptr = 1;
    buffer_out[sizeof(int)] = SAWA_INFO;
    
    send_sync_buf(sock,(unsigned char *)&buffer_out, sizeof(int) + 1, MSG_WAITALL);
    sawa_read_result((unsigned char *)&buffer_in, sizeof(int));
    nb_sectors = *(unsigned int *)buffer_in;
    printk("SaWa: Number of sectors: %d\n", nb_sectors);
    
    return 0;
}

void sawa_write_data(sector_t sector, unsigned long nb_sectors, unsigned char *buffer) {
    unsigned int offset = sector * 512;
    unsigned int payload_size = nb_sectors * 512;
    unsigned char *buffer_out = (unsigned char *)kmalloc(1 + sizeof(int)*2 + payload_size, GFP_KERNEL);
    int *int_ptr = (int*)(&buffer_out);
    int buffer_start = 1 + 2*sizeof(int);

    return;
    
    *int_ptr = payload_size + 1 +sizeof(int);
    buffer_out[sizeof(int)] = SAWA_WRITE;
    int_ptr = (int*)((char*)&buffer_out + 1 + sizeof(int));
    *int_ptr = offset;
    
    strncpy((char*)&buffer_out+buffer_start, buffer, payload_size);
    
    send_sync_buf(sock,(unsigned char *)&buffer_out, buffer_start + payload_size, MSG_WAITALL);
    
    kfree(buffer_out);
    printk(KERN_INFO "SaWa: Wrote %lu sectors\n", nb_sectors); 
}

void sawa_read_data(sector_t sector, unsigned long nb_sectors, unsigned char *buffer_in) {
    unsigned char buffer_out[13];
    unsigned int offset = sector * 512;
    unsigned int payload_size = nb_sectors * 512;
    int *int_ptr = (int*)(&buffer_out);
    memset(buffer_out, 0xFF, 13);
    
    printk(KERN_INFO "Offset: %d, size: %d\n", offset, payload_size);
    *int_ptr = 1 +sizeof(int)*2;
    buffer_out[sizeof(int)] = SAWA_READ;
    int_ptr = (int*)((char*)&buffer_out + 1 + sizeof(int));
    *int_ptr = offset;
    int_ptr = (int*)((char*)&buffer_out + 1 + 2*sizeof(int));
    *int_ptr = payload_size;
    
    printk(KERN_INFO "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
          buffer_out[0], buffer_out[1], buffer_out[2], buffer_out[3],
          buffer_out[4], buffer_out[5], buffer_out[6], buffer_out[7],
          buffer_out[8], buffer_out[9], buffer_out[10], buffer_out[11],
          buffer_out[12]);
    
    send_sync_buf(sock,(unsigned char *)&buffer_out, sizeof(int)*3 + 1, MSG_WAITALL);
    sawa_read_result((unsigned char *)&buffer_in, payload_size);
    
    printk(KERN_INFO "SaWa: Read %lu sectors\n", nb_sectors);    
}

/*===============================================================================================*/

static void blockdev_transfer(sector_t sector,
		unsigned long nsect, char *buffer, int write) {
    printk(KERN_INFO "SaWa: Request for %lu sectors starting at sector #%lu (write=%d)\n", nsect, sector, write);
    
    if (write == 0)
        sawa_read_data(sector, nsect, buffer);
    else
        sawa_write_data(sector, nsect, buffer);
}

static void tasklet_handler(unsigned long data) {
    struct request *req = (struct request *)data;
    
    blockdev_transfer(blk_rq_pos(req), blk_rq_cur_sectors(req),
				req->buffer, rq_data_dir(req));
    
    __blk_end_request_cur(req, 0);
}

static void blockdev_request(struct request_queue *q) {
    struct request *req;
    struct tasklet_struct *tasklet;

	req = blk_fetch_request(q);
	while (req != NULL) {
		if (req == NULL || (req->cmd_type != REQ_TYPE_FS)) {
			printk (KERN_NOTICE "Skip non-CMD request\n");
			__blk_end_request_all(req, -EIO);
			continue;
		}
        
        tasklet = kmalloc(sizeof(struct tasklet_struct), GFP_KERNEL);
        tasklet_init(tasklet, tasklet_handler, (unsigned long)req);
        tasklet_hi_schedule(tasklet);

        
//        blk_dequeue_request(req);
        
//        req = blk_fetch_request(q);
        
                return;

        /*
		blockdev_transfer(blk_rq_pos(req), blk_rq_cur_sectors(req),
				req->buffer, rq_data_dir(req));
		if ( ! __blk_end_request_cur(req, 0) ) {
			req = blk_fetch_request(q);
		}
        */
	}    
}

int blockdev_getgeo(struct block_device * block_device, struct hd_geometry * geo) {
	long size;

    printk(KERN_INFO "GetGeo called\n");
	/* We have no real geometry, of course, so make something up. */
	size = nb_sectors;
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = nb_sectors;
	geo->start = 0;
	return 0;
}

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

/*===============================================================================================*/
static int sawa_driver_init(void)
{
    int ret;
    
    printk( KERN_NOTICE "SaWa: Initialization started" );
    
    spin_lock_init(&sawa_dev.lock);
    
    ret = sawa_connect();
    if (ret < 0) {
        printk(KERN_WARNING "SaWa: Could not communicate to the SaWa Daemon\n");
        return -1;
    }
    ret = sawa_send_info();

    if (ret < 0) {
        printk(KERN_WARNING "SaWa: Did not receive the nubmer of sectors from SaWa\n");
        return -1;
    }
    
    memset (&sawa_dev, 0, sizeof (struct net_dev));
    sawa_dev.nb_sectors = nb_sectors;
    sawa_dev.queue = blk_init_queue(blockdev_request, &sawa_dev.lock);
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
    if (sock != NULL) sock_release(sock);
}
/*===============================================================================================*/

module_init(sawa_driver_init);
module_exit(sawa_driver_exit);
