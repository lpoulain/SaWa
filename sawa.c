////////////////////////////////////////////////////////////////////////////
// The SaWa interface with the file server
////////////////////////////////////////////////////////////////////////////

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
#include "tcp.h"

#define SAWA_INFO  0x01
#define SAWA_READ  0x02
#define SAWA_WRITE 0x03

unsigned int nb_sectors;

void dump_mem(unsigned char *addr, int size) {
    int i, j=0, col=0;
    char buffer[64];
//    printk(KERN_INFO "%d bytes\n", size);
    while (1) {
        if (j >= size) return;
        sprintf(buffer, "%04x  ", j);
        col += 6;
//        printk(KERN_INFO "%04x ", j);
        for (i=0; i<16; i++) {
            if (j >= size) {
                buffer[col++] = '\n';
                buffer[col] = 0;
                printk(KERN_INFO "%s", buffer);
//                printk(KERN_INFO "\n");
                return;
            }
            sprintf(&buffer[col], " %02x", addr[j]);
            col += 3;
//            printk(KERN_INFO " %02x", addr[j]);
            j++;
        }
        buffer[col++] = '\n';
        buffer[col] = 0;
        printk(KERN_INFO "%s", buffer);
//        printk(KERN_INFO "\n");
        col = 0;
    }
}

int sawa_send_info(void) {
    struct socket *conn_socket;
    unsigned int tmp = 0xFFFFFFFF;
    unsigned char buffer_out[12];
    unsigned char buffer_in[4];
    int *int_ptr = (int*)(buffer_out);
    
    buffer_in[0] = 0x42;
    
    *int_ptr = 1;
    buffer_out[sizeof(int)] = SAWA_INFO;

    if (tcp_client_connect(&conn_socket) < 0) {
        printk(KERN_INFO "SaWa: could not connect\n");
        return -1;
    }

    tcp_client_send(conn_socket, (const char *)&buffer_out, 5, MSG_WAITALL);
    
    printk("SaWa: %x %x %x\n", buffer_in[0], tmp, *((int*)&buffer_out));
    
    tcp_client_receive(conn_socket, (unsigned char *)&buffer_in, MSG_DONTWAIT);
    nb_sectors = *(unsigned int *)buffer_in;
    printk("SaWa: Number of sectors: %d\n", nb_sectors);
    
    sock_release(conn_socket);
    return 0;
}

int sawa_write_data(sector_t sector, unsigned long nb_sectors, unsigned char *buffer) {
    struct socket *conn_socket;
    unsigned int offset = sector * 512;
    unsigned int payload_size = nb_sectors * 512;
    unsigned char *buffer_out = (unsigned char *)kmalloc(1 + sizeof(int)*2 + payload_size, GFP_KERNEL);
    int *int_ptr = (int*)(buffer_out);
    int buffer_start = 1 + 2*sizeof(int);
    
    if (tcp_client_connect(&conn_socket) < 0) {
        printk(KERN_INFO "SaWa: could not connect\n");
        return -1;
    }
    printk(KERN_INFO "SaWa: Write data - Offset: %d, size: %d\n", offset, payload_size);
    
    *int_ptr = payload_size + 1 +sizeof(int);
    buffer_out[sizeof(int)] = SAWA_WRITE;
    int_ptr = (int*)(buffer_out + 1 + sizeof(int));
    *int_ptr = offset;
    
    memcpy(buffer_out+buffer_start, buffer, payload_size);
    
    tcp_client_send(conn_socket, (const char *)buffer_out, buffer_start + payload_size, MSG_WAITALL);    
    
    kfree(buffer_out);
    sock_release(conn_socket);
    printk(KERN_INFO "SaWa: Wrote %lu sectors\n", nb_sectors); 
    return 0;
}

int sawa_read_data(sector_t sector, unsigned long nb_sectors, unsigned char *buffer_in) {
    struct socket *conn_socket;
    unsigned char buffer_out[13];
    unsigned int offset = sector * 512;
    unsigned int payload_size = nb_sectors * 512;
    int *int_ptr = (int*)(&buffer_out);
    memset(buffer_out, 0xFF, 13);

    if (tcp_client_connect(&conn_socket) < 0) {
        printk(KERN_INFO "SaWa: could not connect\n");
        return -1;
    }
    printk(KERN_INFO "SaWa: Read data - Offset: %d, size: %d\n", offset, payload_size);
    *int_ptr = 1 +sizeof(int)*2;
    buffer_out[sizeof(int)] = SAWA_READ;
    int_ptr = (int*)((char*)&buffer_out + 1 + sizeof(int));
    *int_ptr = offset;
    int_ptr = (int*)((char*)&buffer_out + 1 + 2*sizeof(int));
    *int_ptr = payload_size;

    tcp_client_send(conn_socket, (const char *)&buffer_out, sizeof(int)*3 + 1, MSG_WAITALL);
    tcp_client_receive(conn_socket, (unsigned char *)buffer_in, MSG_DONTWAIT);
    
    sock_release(conn_socket);
    printk(KERN_INFO "SaWa: Read %lu sectors\n", nb_sectors);    
    return 0;
}
