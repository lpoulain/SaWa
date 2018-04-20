// This is the TCP/IP layer which talks to the server

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

/*===============================================================================================*/
// TCP/IP layer
/*===============================================================================================*/

#define PORT 5000

u32 create_address(u8 *ip)
{
        u32 addr = 0;
        int i;

        for(i=0; i<4; i++)
        {
                addr += ip[i];
                if(i==3)
                        break;
                addr <<= 8;
        }
        return addr;
}

unsigned int inet_addr(char *str)
{
    int a, b, c, d;
    char arr[4];
    sscanf(str, "%d.%d.%d.%d", &a, &b, &c, &d);
    arr[0] = a; arr[1] = b; arr[2] = c; arr[3] = d;
    return *(unsigned int *)arr;
}

int tcp_client_connect(struct socket **conn_socket)
{
    struct sockaddr_in saddr;
    unsigned char destip[5] = {127,0,0,1,'\0'};
    int ret = -1;

    DECLARE_WAIT_QUEUE_HEAD(recv_wait);

    ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, conn_socket);
    if(ret < 0)
    {
            pr_info("SaWa Error: %d while creating first socket. | "
                    "setup_connection *** \n", ret);
            return -1;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT);
    saddr.sin_addr.s_addr = htonl(create_address(destip));

    ret = (*conn_socket)->ops->connect(*conn_socket, (struct sockaddr *)&saddr\
                    , sizeof(saddr), O_RDWR);
    if(ret && (ret != -EINPROGRESS))
    {
            pr_info("SaWa Error: %d while connecting using conn "
                    "socket. | setup_connection *** \n", ret);
            return -1;
    }

    return 0;
}

int tcp_client_send(struct socket *sock, const char *buf, const size_t length,\
                unsigned long flags)
{
        struct msghdr msg;
        //struct iovec iov;
        struct kvec vec;
        int len, written = 0, left = length;
        mm_segment_t oldmm;

        msg.msg_name    = 0;
        msg.msg_namelen = 0;
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags   = flags;

        oldmm = get_fs(); set_fs(KERNEL_DS);
repeat_send:
        vec.iov_len = left;
        vec.iov_base = (char *)buf + written;

        len = kernel_sendmsg(sock, &msg, &vec, left, left);
        if((len == -ERESTARTSYS) || (!(flags & MSG_DONTWAIT) &&\
                                (len == -EAGAIN)))
                goto repeat_send;
        if(len > 0)
        {
                written += len;
                left -= len;
                if(left)
                        goto repeat_send;
        }
        set_fs(oldmm);
        return written ? written:len;
}

int tcp_client_receive(struct socket *sock, char *str, unsigned long flags, int max_size)
{
        //mm_segment_t oldmm;
        struct msghdr msg;
        //struct iovec iov;
        struct kvec vec;
        int len;

        msg.msg_name    = 0;
        msg.msg_namelen = 0;
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags   = flags;
        vec.iov_len = max_size;
        vec.iov_base = str;

read_again:
        len = kernel_recvmsg(sock, &msg, &vec, max_size, max_size, flags);

        if(len == -EAGAIN || len == -ERESTARTSYS)
        {
//                pr_info(" *** mtp | error while reading: %d | "
//                        "tcp_client_receive *** \n", len);

                goto read_again;
        }


        return len;
}
