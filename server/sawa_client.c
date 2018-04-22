#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "sawa.h"

int server_port = 5000;
int debug=1;

typedef struct {
	volatile int counter;
} atomic_t;

//#define ATOMIC_INIT(i)	{ (i) }

#define atomic_read(v) ((v)->counter)
#define atomic_set(v,i) (((v)->counter) = (i))

static inline void atomic_dec( atomic_t *v )
{
	(void)__sync_fetch_and_sub(&v->counter, 1);
}

void dump_mem(unsigned char *addr, int size) {
    if (!debug) return;
    
    int i, j=0;
    printf("Received %d bytes\n", size);
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

void read_result(int socket_fd, int expected_size, int debug, unsigned char *buffer_in) {
    int n, size=0;
    
    for (;;) {
//        printf("Before reading...\n");
        n = read(socket_fd, buffer_in + size, 1024);
//        if (debug)
//            printf("[%d] 0x%02x %02x %02x %02x\n", n, buffer_in[0], buffer_in[1], buffer_in[2], buffer_in[3]);
        if (n <= 0) {
            if (size > 0) dump_mem(buffer_in, size);
            return;
        }
        size += n;
        if (size == expected_size) {
            dump_mem(buffer_in, size);
            return;
        }
//        printf("0x%02x (%d bytes)\n", buffer_in[0], n);
    }    
}

unsigned char *send_read_cmd(int socket_fd, int offset, int payload_size) {
    unsigned char buffer_out[12];
    unsigned char *buffer_in = malloc(payload_size);
    int *int_ptr = (int*)(buffer_out);

    *int_ptr = 1 +sizeof(int)*2;
    buffer_out[sizeof(int)] = SAWA_READ;
    int_ptr = (int*)(buffer_out + 1 + sizeof(int));
    *int_ptr = offset;
    int_ptr = (int*)(buffer_out + 1 + 2*sizeof(int));
    *int_ptr = payload_size;

    write(socket_fd, &buffer_out, 1 + sizeof(int)*3);
    read_result(socket_fd, payload_size, 0, buffer_in);
    
    return buffer_in;
}

void send_write_cmd_random(int socket_fd, int offset, int payload_size) {
    unsigned char buffer_out[5000];
    int *int_ptr = (int*)(buffer_out);
    int i, buffer_start = 1 + 2*sizeof(int);
    unsigned char c = 0xFF;

    *int_ptr = payload_size + 1 +sizeof(int);
    buffer_out[sizeof(int)] = SAWA_WRITE;
    int_ptr = (int*)(buffer_out + 1 + sizeof(int));
    *int_ptr = offset;
    
    for (i=0; i<payload_size; i++) {
        buffer_out[buffer_start+i] = c;
    }

    write(socket_fd, &buffer_out, payload_size + buffer_start);
}

void send_write_cmd(int socket_fd, int offset, int payload_size, unsigned char *payload) {
    unsigned char buffer_out[5000];
    int *int_ptr = (int*)(buffer_out);
    int i, buffer_start = 1 + 2*sizeof(int);
    unsigned char c = (unsigned char)(payload_size % 256);

    *int_ptr = payload_size + 1 +sizeof(int);
    buffer_out[sizeof(int)] = SAWA_WRITE;
    int_ptr = (int*)(buffer_out + 1 + sizeof(int));
    *int_ptr = offset;
    
    memcpy(buffer_out + buffer_start, payload, payload_size);

    write(socket_fd, &buffer_out, payload_size + buffer_start);
}

void send_info_cmd() {
    unsigned char buffer_out[12];
    int *int_ptr = (int*)(buffer_out);
    unsigned char result[4];

    int socket_fd = sawa_client_init();
    *int_ptr = 1;
    buffer_out[sizeof(int)] = SAWA_INFO;
    
    write(socket_fd, buffer_out, 1 + sizeof(int));
    read_result(socket_fd, sizeof(int), 1, (unsigned char*)&result);
}

void send_stop_cmd() {
    unsigned char buffer_out[12];
    int *int_ptr = (int*)(buffer_out);
    unsigned char result[4];

    int socket_fd = sawa_client_init();
    *int_ptr = 1;
    buffer_out[sizeof(int)] = SAWA_STOP;
    
    write(socket_fd, buffer_out, 1 + sizeof(int));
}

void sawa_send_command(int socket_fd, int op, int offset, int nb_bytes) {
    if (op == SAWA_READ) free(send_read_cmd(socket_fd, offset, nb_bytes));
    else if (op == SAWA_WRITE) send_write_cmd_random(socket_fd, offset, nb_bytes);
}

int sawa_client_init() {
    struct sockaddr_in server_addr;
    int socket_fd;
    
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, '0', sizeof(server_addr));
    
    server_addr.sin_family = AF_INET;
    if(inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        exit(1);
    } 
    server_addr.sin_port = htons(server_port);
    
    if (connect(socket_fd, (struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
        printf("Could not connect to port 5000\n");
        exit(1);
    }
    
    return socket_fd;
}

////////////////////////////////////////////////////////////////////////////

static atomic_t test_counter;
static int test_nb_requests;

void *sawa_thread_test(void *arg) {
    unsigned char *buffer = malloc(4096);
    int i, j;
    long thread_id = (long)arg;
    int socket_fd;

    socket_fd = sawa_client_init();
    
    sawa_send_command(socket_fd, SAWA_INFO, 0, 0);
    
    unsigned char *buffer_out = malloc(4096);
    unsigned char *buffer_in;
    
    for (i=0; i<256; i++) {
        for (j=0; j<4096; j++) {
            buffer_out[j] = (i + j) % 256;
        }
        send_write_cmd(socket_fd, i*4096, 4096, buffer_out);
        buffer_in = send_read_cmd(socket_fd, i*4096, 4096);
        
        if (strncmp((const char *)buffer_in, (const char *)buffer_out, 4096) != 0)
            printf("Error page %d, read/write operation failed\n", i);
        
        free(buffer_in);
    }

    close(socket_fd);
    free(buffer);
    
    printf("End thread %ld\n", thread_id);
    atomic_dec(&test_counter);
}

void sawa_test(int nb_threads) {
    pthread_t *tid = malloc(sizeof(pthread_t)*nb_threads);
    int i;
    debug = 0;
    
    if (nb_threads > 10) nb_threads = 10;
    atomic_set(&test_counter, nb_threads);

    for (i=0; i<nb_threads; i++) {
        pthread_create(&(tid[i]), NULL, &sawa_thread_test, (void*)(long)i);
    }
    
    while (atomic_read(&test_counter) > 0) {
    }
    
    free(tid);
}

////////////////////////////////////////////////////////////////////////////

void print_usage(char *prg) {
    printf("Usage: %s info|read|write <parameters\n", prg);
    printf("- %s info\n", prg);
    printf("- %s read <offset> [<size>]\n", prg);
    printf("- %s write <offset> [<size>]\n", prg);
    printf("- %s test [nb threads]\n", prg);
    printf("- %s stop\n", prg);
}

int main(int argc, char *argv[]) {
    unsigned int op, offset, size = 4096;
    int socket_fd, nb_requests, nb_threads = 1;

    if (argc < 2)
    {
        print_usage(argv[0]);
        return -1;
    }
    
    if (!strcmp(argv[1], "info")) {
        send_info_cmd();
        return 0;
    }
    
    if (!strcmp(argv[1], "stop")) {
        send_stop_cmd();
        return 0;
    }
    
    if (!strcmp(argv[1], "test")) {
//        if (argc >= 3) nb_requests = atoi(argv[2]); else nb_requests = 2000;
        if (argc >= 3) nb_threads = atoi(argv[2]);
        printf("Testing %d threads\n", nb_threads);
        sawa_test(nb_threads);
        return 0;
    }

    if (!strcmp(argv[1], "read")) op = SAWA_READ;
    else if (!strcmp(argv[1], "write")) op = SAWA_WRITE;
    else {
        printf("Unknown operation: %s\n", argv[1]);
        return -1;
    }
    
    if (argc < 3) {
        printf("Offset missing\n");
        print_usage(argv[0]);
        return -1;
    }

    offset = atoi(argv[2]);
    
    if (argc >= 4) size = atoi(argv[3]);
    if (size <= 0) {
        printf("Invalid size: %d\n", size);
        print_usage(argv[0]);
        return -1;
    }

    printf("Operation %d, offset %d, size: %d\n", op, offset, size);
    socket_fd = sawa_client_init();
    sawa_send_command(socket_fd, op, offset, size);
    
    return 0;
}
