#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstddef>
#include <sys/time.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "sawa_client_interface.h"

using namespace std;

int server_port = 5000;
int admin_port = 5001;
extern int debug;

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

/////////////////////////////////////////////////////////////////////////////

uint8_t *SawaInterface::prepareCommand(uint8_t op, int payload_size) {
    int *int_ptr;
    this->total_size = sizeof(int) + 1 + payload_size;
    buffer_out = new uint8_t[this->total_size];
    buffer_out[sizeof(int)] = op;
    int_ptr = (int *)buffer_out;
    *int_ptr = (payload_size + 1);
    
    return buffer_out + sizeof(int) + 1;
}

void SawaInterface::sendCmdToServer() {
    write(socket_fd, buffer_out, this->total_size);
    delete [] buffer_out;
}

void SawaInterface::sendSingleCmdToServer(uint8_t op) {
    uint8_t buffer_out[12];
    int *int_ptr = (int*)(buffer_out);
    *int_ptr = 1;
    buffer_out[sizeof(int)] = op;
    
    write(this->socket_fd, buffer_out, 1 + sizeof(int));
}

int SawaInterface::readFromServer(uint8_t *buffer_in, int size) {
    return read(socket_fd, buffer_in, size);
}

int SawaInterface::readFromServer(uint8_t **buffer_in) {
    unsigned int payload_size;
    int n;
    
    // Read the payload size
    n = read(socket_fd, (char*)&payload_size, 4);

    // If there is less than that, the operation failed
    if (n < 4) {
        return 0;
    }

    // Otherwise, read the payload
    *buffer_in = new uint8_t[payload_size];
    n = read(socket_fd, *buffer_in, payload_size);

    dump_mem(*buffer_in, n);
    
    return payload_size;
}

SawaInterface::SawaInterface(int port) {
    struct sockaddr_in server_addr;
    
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
}

SawaInterface::~SawaInterface() {
    close(socket_fd);
}

////////////////////////////////////////////////////////////////////////////

uint8_t *SawaClient::sendReadCmd(int offset, int payload_size, SawaInterface *sawa) {
    int n;
    bool needsSawaDelete = false;
    int *int_ptr;
    uint8_t *buffer_in = new uint8_t[payload_size];
    if (sawa == nullptr) {
        sawa = new SawaInterface(server_port);
        needsSawaDelete = true;
    }
    int_ptr = (int*)sawa->prepareCommand(SAWA_READ, 2 * sizeof(int));

    *int_ptr = offset;
    int_ptr++;
    *int_ptr = payload_size;

    // Sends the READ command
    sawa->sendCmdToServer();
    // Reads the result
    n = sawa->readFromServer(buffer_in, payload_size);

    if (needsSawaDelete) delete sawa;
    
    // If the message is one byte-long => error
    if (n == 1) {
        n = buffer_in[0];
        delete [] buffer_in;
        throw n;
    }

    return buffer_in;
}

int SawaClient::sendWriteCmd(uint8_t *payload, int offset, int payload_size, SawaInterface *sawa) {
    char response = 0xFF;
    bool needsSawaDelete = false;
    uint8_t *buffer_out;
    int *int_ptr;
    int i;

    if (sawa == nullptr) {
        sawa = new SawaInterface(server_port);
        needsSawaDelete = true;
    }
    
    buffer_out = sawa->prepareCommand(SAWA_WRITE, sizeof(int) + 4096);
    int_ptr = (int*)(buffer_out);
    *int_ptr = offset;
    
    memcpy(buffer_out + sizeof(int), payload, payload_size);

    sawa->sendCmdToServer();
    sawa->readFromServer((uint8_t*)&response, 1);
    
    if (needsSawaDelete) delete sawa;
    
    return response;
}

int SawaClient::sendWriteCmd(int offset, int payload_size, SawaInterface *sawa) {
    char response = 0xFF;
    bool needsSawaDelete = false;
    uint8_t *buffer_out;
    int *int_ptr;
    int i;
    unsigned char c = 0xFF;

    if (sawa == nullptr) {
        sawa = new SawaInterface(server_port);
        needsSawaDelete = true;
    }
    
    buffer_out = sawa->prepareCommand(SAWA_WRITE, sizeof(int) + 4096);
    int_ptr = (int*)(buffer_out);
    *int_ptr = offset;
    
    memset(buffer_out + sizeof(int), c, payload_size);

    sawa->sendCmdToServer();
    sawa->readFromServer((uint8_t*)&response, 1);

    if (needsSawaDelete) delete sawa;
    
    return response;
}

int SawaClient::sendInfoCmd() {
    int result = 0, n;

    SawaInterface sawa = SawaInterface(server_port);
    sawa.sendSingleCmdToServer(SAWA_INFO);
    n = sawa.readFromServer((uint8_t*)&result, 4);
    
    if (n == 1) {
        throw result;
    }
    
    printf("Number of sectors: %d\n", result);
    
    return result;
}

void SawaClient::sendStopCmd() {
    SawaInterface sawa = SawaInterface(admin_port);
    sawa.sendSingleCmdToServer(SAWA_STOP);
}

void SawaClient::sendStatCmd() {
    uint8_t *buffer_in;
    int n, i, nb_threads;
    ThreadStat *thread_stats;
    
    SawaInterface sawa = SawaInterface(admin_port);
    sawa.sendSingleCmdToServer(SAWA_STAT);
    n = sawa.readFromServer(&buffer_in);
    
    nb_threads = n / sizeof(ThreadStat);
    thread_stats = (ThreadStat*)buffer_in;
    
    printf("There are %d threads\n", nb_threads);
    printf("Thread# Active? #Conn   Info    Reads   Writes\n");
    
    for (i=0; i<nb_threads; i++) {
        printf("%d\t%d\t%d\t%d\t%d\t%d\n",
               i,
               thread_stats[i].active,
               thread_stats[i].nb_connections,
               thread_stats[i].info[0],
               thread_stats[i].info[1],
               thread_stats[i].info[2]);
    }
    
    delete [] buffer_in;
}

SawaClient::SawaClient() {
}

SawaClient::~SawaClient() {
}

////////////////////////////////////////////////////////////////////////////

static atomic_t test_counter;
static int test_nb_requests;

void *SawaClient::threadTest(void *arg) {
    uint8_t *buffer = new uint8_t[4096];
    int i, j, n;
    long thread_id = (long)arg;
    int socket_fd, result;
    SawaClient sawaClient = SawaClient();
    SawaInterface sawa = SawaInterface(server_port);

    // Sends INFO message
    sawa.sendSingleCmdToServer(SAWA_INFO);
    n = sawa.readFromServer((uint8_t*)&result, 4);
    
    uint8_t *buffer_out = new uint8_t[4096];
    uint8_t *buffer_in;
    
    for (i=0; i<255; i++) {
        for (j=0; j<4096; j++) {
            buffer_out[j] = (i + j) % 256;
        }
        result = sawaClient.sendWriteCmd(buffer_out, i*4096, 4096, &sawa);
        if (result != SAWA_MSG_OK) {
            cout << "Error code " << result << " at page " << i << endl;
        }
        buffer_in = sawaClient.sendReadCmd(i*4096, 4096, &sawa);
        
        if (strncmp((const char *)buffer_in, (const char *)buffer_out, 4096) != 0)
            printf("Error page %d, read/write operation failed\n", i);
        
        delete [] buffer_in;
    }

    delete [] buffer;
    
    printf("End thread %ld\n", thread_id);
    atomic_dec(&test_counter);
}

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
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

void SawaClient::testServer(int nb_threads) {
    pthread_t *tid = new pthread_t[nb_threads];
    int i;
    debug = 0;
    struct timeval starttime, endtime, timediff;
    
    if (nb_threads > 10) nb_threads = 10;
    printf("Staring test...\n");
    
    gettimeofday(&starttime,0x0);
    atomic_set(&test_counter, nb_threads);

    for (i=0; i<nb_threads; i++) {
        pthread_create(&(tid[i]), NULL, &SawaClient::threadTest, (void*)(long)i);
    }
    
    while (atomic_read(&test_counter) > 0) {
    }
    gettimeofday(&endtime,0x0);
    
    timeval_subtract(&timediff,&endtime,&starttime);
    printf("Test completed in %d seconds and %d ms\n", (int)timediff.tv_sec, (int)timediff.tv_usec / 1000);
    
    delete [] tid;
}

void SawaClient::sendCommand(uint8_t op, int offset, int size) {
    uint8_t *buffer;
    int response;
    
    if (op == SAWA_READ) {
        buffer = sendReadCmd(offset, size);
        if (buffer != NULL) {
            dump_mem(buffer, size);
            delete [] buffer;
        }
    }
    else if (op == SAWA_WRITE) {
        response = sendWriteCmd(offset, size);

        if (response == SAWA_MSG_OK) cout << "Success!" << endl;
        else if (response == SAWA_MSG_ERR) cout << "Error!" << endl;
        else if (response == SAWA_MSG_NOAUTH) cout << "Not authorized!" << endl;
        else cout << "Unknown response: " << response << endl;
    }
    else {
        cout << "Unknown operation: " << op << endl;
    }
}
