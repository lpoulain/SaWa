#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "sawa.h"

int server_port = 5000;
int admin_port = 5001;
int debug=1;

typedef struct {
	volatile int counter;
} atomic_t;

struct thread_stat {
    int nb_connections;
    int info[3];
    char active;
};

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

unsigned char *send_read_cmd(int socket_fd, int offset, int payload_size) {
    char response;
    int n;
    unsigned char buffer_out[12];
    unsigned char *buffer_in = malloc(payload_size + 1);
    int *int_ptr = (int*)(buffer_out);

    *int_ptr = 1 +sizeof(int)*2;
    buffer_out[sizeof(int)] = SAWA_READ;
    int_ptr = (int*)(buffer_out + 1 + sizeof(int));
    *int_ptr = offset;
    int_ptr = (int*)(buffer_out + 1 + 2*sizeof(int));
    *int_ptr = payload_size;

    write(socket_fd, &buffer_out, 1 + sizeof(int)*3);
    n = read(socket_fd, buffer_in, payload_size);

    // Read result
    if (n > 1) return buffer_in;

    // If the message is just one byte long, it's an error code
    switch(buffer_in[0]) {
        case SAWA_MSG_ERR:
            printf("Error!\n");
            break;
        case SAWA_MSG_NOAUTH:
            printf("Not authorized!\n");
            break;
        default:
            printf("Unknown return code: %x\n", buffer_in[0]);
    }
    return NULL;
}

int send_write_cmd_random(int socket_fd, int offset, int payload_size) {
    char response;
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
    read(socket_fd, &response, 1);
   
    return response;
}

int send_write_cmd(int socket_fd, int offset, int payload_size, unsigned char *payload) {
    char response;
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
    read(socket_fd, &response, 1);
    
    return response;
}

void send_info_cmd() {
    unsigned char buffer_out[12];
    int *int_ptr = (int*)(buffer_out);
    int result;

    int socket_fd = sawa_client_init(server_port);
    *int_ptr = 1;
    buffer_out[sizeof(int)] = SAWA_INFO;
    
    write(socket_fd, buffer_out, 1 + sizeof(int));
    read(socket_fd, (unsigned char*)&result, sizeof(int));
    printf("Number of sectors: %d\n", result);
}

void send_stop_cmd() {
    unsigned char buffer_out[12];
    int *int_ptr = (int*)(buffer_out);
    unsigned char result[4];

    int socket_fd = sawa_client_init(admin_port);
    *int_ptr = 1;
    buffer_out[sizeof(int)] = SAWA_STOP;
    
    write(socket_fd, buffer_out, 1 + sizeof(int));
}

void send_stat_cmd() {
    unsigned char buffer_out[12];
    unsigned char *buffer_in;
    int size, nb_threads, i;
    int *int_ptr = (int*)(buffer_out);
    unsigned char result[4];
    struct thread_stat *thread_stats;

    int socket_fd = sawa_client_init(admin_port);
    *int_ptr = 1;
    buffer_out[sizeof(int)] = SAWA_STAT;
    
    write(socket_fd, buffer_out, 1 + sizeof(int));
    
    read(socket_fd, &size, 4);
    nb_threads = size / sizeof(struct thread_stat);
    
    printf("There are %d threads\n", nb_threads);
    printf("Thread# Active? #Conn   Info    Reads   Writes\n");
    buffer_in = malloc(size);
    thread_stats = (struct thread_stat *)buffer_in;

    read(socket_fd, buffer_in, size);
    
    for (i=0; i<nb_threads; i++) {
        printf("%d\t%d\t%d\t%d\t%d\t%d\n",
               i,
               thread_stats[i].active,
               thread_stats[i].nb_connections,
               thread_stats[i].info[0],
               thread_stats[i].info[1],
               thread_stats[i].info[2]);
    }
    
    free(buffer_in);
}

void sawa_send_command(int socket_fd, int op, int offset, int nb_bytes) {
    unsigned char *buffer;
    int response;
    
    if (op == SAWA_READ) {
        buffer = send_read_cmd(socket_fd, offset, nb_bytes);
        if (buffer != NULL) {
            dump_mem(buffer, nb_bytes);
            free(buffer);
        }
    }
    else if (op == SAWA_WRITE) {
        response = send_write_cmd_random(socket_fd, offset, nb_bytes);

        if (response == SAWA_MSG_OK) printf("Success!\n");
        else if (response == SAWA_MSG_ERR) printf("Error!\n");
        else if (response == SAWA_MSG_NOAUTH) printf("Not authorized!\n");
    }
}

int sawa_client_init(int port) {
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
    server_addr.sin_port = htons(port);
    
    if (connect(socket_fd, (struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
        printf("Could not connect to port %d\n", port);
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

    socket_fd = sawa_client_init(server_port);
    
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

int timeval_subtract (result, x, y)
     struct timeval *result, *x, *y;
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

void sawa_test(int nb_threads) {
    pthread_t *tid = malloc(sizeof(pthread_t)*nb_threads);
    int i;
    debug = 0;
    struct timeval starttime, endtime, timediff;
    
    if (nb_threads > 10) nb_threads = 10;
    printf("Staring test...\n");
    
    gettimeofday(&starttime,0x0);
    atomic_set(&test_counter, nb_threads);

    for (i=0; i<nb_threads; i++) {
        pthread_create(&(tid[i]), NULL, &sawa_thread_test, (void*)(long)i);
    }
    
    while (atomic_read(&test_counter) > 0) {
    }
    gettimeofday(&endtime,0x0);
    
    timeval_subtract(&timediff,&endtime,&starttime);
    printf("Test completed in %d seconds and %d ms\n", (int)timediff.tv_sec, (int)timediff.tv_usec / 1000);
    
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

    if (!strcmp(argv[1], "stat")) {
        send_stat_cmd();
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
    socket_fd = sawa_client_init(server_port);
    sawa_send_command(socket_fd, op, offset, size);
    
    return 0;
}
