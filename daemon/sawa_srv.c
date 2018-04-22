#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "sawa.h"
#include "display.h"
#include "thread_pool.h"

static const char *filesystem = "./filesystem";
static unsigned int nb_sectors = 2048;
static int fd;

void dump_mem(unsigned char *addr, int size) {
    int i, j=0;
    screen.debug("Received %d bytes\n", size);
    while (1) {
        printf("%04x ", j);
        for (i=0; i<16; i++) {
            if (j >= size) {
                printf("\n");
                return;
            }
            printf(" %02x", addr[j]);
            j++;
        }
        printf("\n");
    }
}

void send_info(int socket_fd) {
    char buffer_out[4];
    screen.debug("INFO command received\n");
    write(socket_fd, &nb_sectors, sizeof(int));
    screen.debug("INFO sent\n");
}

void read_file(int socket_fd, unsigned int offset, unsigned int size) {
    screen.debug("[%d] Read %d bytes (0x%x) starting at offset %d\n", socket_fd, size, size, offset);
    if (size + offset > nb_sectors * 512) size = nb_sectors * 512 - offset;
    lseek(fd, offset, SEEK_SET);
    unsigned char *buffer = malloc(size);
    read(fd, buffer, size);
    screen.debug("[%d] Sending data...\n", socket_fd);
//    dump_mem(buffer, 32);
    write(socket_fd, buffer, size);
    screen.debug("[%d] ...%d bytes sent\n", socket_fd, size);
    free(buffer);
}

void write_file(int socket_fd, unsigned char *addr, unsigned int offset, unsigned int size) {
    screen.debug("[%d] Write %d bytes starting at offset %d\n", socket_fd, size, offset);
//    dump_mem(addr, 32);
    lseek(fd, offset, SEEK_SET);
    write(fd, addr, (ssize_t)size);
    sync();
    screen.debug("[%d]... done\n", socket_fd);
}

void process_request(int socket_fd, struct connection_thread *thread_info, unsigned char *addr, unsigned int size) {
    unsigned int offset;
    unsigned char op;
    
//    dump_mem(addr, size);
    
//    op = addr[sizeof(int)];
    op = addr[0];
    
    if (op == SAWA_INFO) {
        send_info(socket_fd);
        thread_info->info[0]++;
        screen.refresh(thread_info, 0);
        return;
    }
    
//    offset = *((unsigned int*)(addr+1+sizeof(int)));
    offset = *((unsigned int*)(addr+1));

    switch(op) {
        case SAWA_READ:
            size = *((unsigned int*)(addr+1+sizeof(int)));
            read_file(socket_fd, offset, size);
            thread_info->info[1]++;
            screen.refresh(thread_info, 1);
            break;
        case SAWA_WRITE:
            size -= (1 + sizeof(int));
            write_file(socket_fd, addr + 1 + sizeof(int), offset, size);
            thread_info->info[2]++;
            screen.refresh(thread_info, 2);
            break;
        case SAWA_STOP:
            ctrl_c_handler();
            exit(0);
    }
}

void sawa_listen(struct connection_thread *thread_info) {
    int socket_fd = thread_info->client_sock;
    int n, size=0;
    unsigned int expected_size;
    unsigned char *buffer_in;
    
    for (;;) {
    
        n = read(socket_fd, (char*)(&expected_size), 4);
        if (n < 4) return;

        if (expected_size > 32768) return;

        if (expected_size > 0) {
            buffer_in = malloc(expected_size);
            n = read(socket_fd, buffer_in, expected_size);
    
            if (n < 0) {
                free(buffer_in);
                return;
            }

            process_request(socket_fd, thread_info, buffer_in, expected_size);
            free(buffer_in);
        }
    }
}

int get_fs_file() {
    int nb_bytes;
    int fd;
    
    fd= open(filesystem, O_RDWR, 0700);
    if (fd > 0) return fd;
    
    screen.debug("Filesystem file does not exist, creating it...\n");
    fd = open(filesystem, O_RDWR|O_CREAT, 0700);
    
    nb_bytes = nb_sectors * 512;
    unsigned char *buffer = malloc(nb_bytes);
    memset(buffer, 0, nb_bytes);
    write(fd, buffer, nb_bytes);
    free(buffer);
    
    return fd;
}

void sawa_init() {
    fd = get_fs_file();

    op_listen = sawa_listen;
    server_port = 5000;
}
