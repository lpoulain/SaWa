#include <iostream>
#include <cstddef>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "sawa.h"
#include "display.h"
#include "thread_pool.h"

using namespace std;

static int thread_nb = 0;

void (*op_listen) (struct connection_thread *);
sigset_t fSigSet;

/////////////////////////////////////////////////////////////////////////////////////
// Thread startup function
/////////////////////////////////////////////////////////////////////////////////////

void release_thread(struct connection_thread *thread_info);

void *connection_handler(void *ti)
{
    struct connection_thread *thread_info = (struct connection_thread *)ti;
    int nSig;

    while (1) {
        op_listen(thread_info);
        close(thread_info->client_sock);
        release_thread(thread_info);
        
        sigwait(&fSigSet, &nSig);
        screen->status(thread_info, 1);
    }
     
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////
// Threads handling TCP connections
/////////////////////////////////////////////////////////////////////////////////////

pthread_mutex_t idle_threads_lock;
pthread_mutex_t all_threads_lock;

struct connection_thread *idle_threads = nullptr;
struct connection_thread *all_threads = nullptr;

// There is no idle thread, so we create a new one
struct connection_thread *new_thread(int client_sock) {
    int i;
    struct connection_thread *thread_info = (struct connection_thread *)malloc(sizeof(struct connection_thread));
    
    thread_info->next = nullptr;
    thread_info->client_sock = client_sock;
    thread_info->nb_connections = 1;
    for (i=0; i<3; i++) thread_info->info[i] = 0;
    
    pthread_cond_init(&(thread_info->cv), NULL);
    pthread_mutex_init(&(thread_info->mp), NULL);

    pthread_mutex_lock(&all_threads_lock);
        thread_info->nb = thread_nb++;
        thread_info->next = all_threads;
        thread_info->next_all = all_threads;
        all_threads = thread_info;
    pthread_mutex_unlock(&all_threads_lock);
    
    if( pthread_create( &thread_info->thread, NULL, connection_handler, (void*)thread_info) < 0)
    {
        screen->error("could not create thread");
        return 0;
    }    

    screen->new_thread(thread_info);
    
    return thread_info;
}

// A thread is done executing, put it back in the unassigned pool
void release_thread(struct connection_thread *thread_info) {
    thread_info->client_sock = -1;

    pthread_mutex_lock(&idle_threads_lock);
        thread_info->next = idle_threads;
        idle_threads = thread_info;
    pthread_mutex_unlock(&idle_threads_lock);

    screen->status(thread_info, 0);
}

// Reuse an idle thread
struct connection_thread *reuse_thread(int client_sock) {
    struct connection_thread *thread_info = nullptr;

    // Is there any idle thread?
    if (idle_threads == nullptr) return nullptr;
    
    // If there is, remove it from the idle queue
    pthread_mutex_lock(&idle_threads_lock);
        thread_info = idle_threads;
        idle_threads = thread_info->next;
    pthread_mutex_unlock(&idle_threads_lock);   
    
    // Assign it to the connection
    thread_info->client_sock = client_sock;
    thread_info->nb_connections++;
    
    // Wake up the thread
    screen->thread_update(thread_info);
    pthread_kill(thread_info->thread, SIGUSR1);
    
    return thread_info;
}

////////////////////////////////////////////////////////
void handle_new_connection(int client_sock) {
    // Try to reuse an idle thread
    struct connection_thread *thread_info = reuse_thread(client_sock);
    // If there is no idle thread, create a new one
    if (thread_info == nullptr) new_thread(client_sock);
}

////////////////////////////////////////////////////////

int thread_pool_init() {
    if (pthread_mutex_init(&idle_threads_lock, NULL) != 0 ||
        pthread_mutex_init(&all_threads_lock, NULL) != 0)
    {
        cerr << "\n mutex init has failed" << endl;
        return 1;
    }

    sigemptyset(&fSigSet);
    sigaddset(&fSigSet, SIGUSR1);
    sigaddset(&fSigSet, SIGSEGV);

    pthread_sigmask(SIG_BLOCK, &fSigSet, NULL);
    
    return 0;
}

int thread_pool_cleanup() {
    struct connection *conn_tmp;
    struct connection_thread *thread_info;
    int nb_conn = 0;

    while (all_threads != nullptr) {
        thread_info = all_threads;
        
        pthread_detach(thread_info->thread);
        pthread_kill(thread_info->thread, SIGALRM);

        if (thread_info->client_sock >= 0) {
            shutdown(thread_info->client_sock, SHUT_RDWR);
            nb_conn++;
        }
        
        all_threads = all_threads->next;
        free(thread_info);
    }
    
    return nb_conn;
}

////////////////////////////////////////////////////////////
// Thread serialization
////////////////////////////////////////////////////////////

int get_nb_threads() {
    int nb_threads = 0;
    struct connection_thread *thread_info = all_threads;

    while (thread_info != nullptr) {
        nb_threads++;
        thread_info = thread_info->next;
    }

    return nb_threads;
}

void serialize_thread_stats(unsigned char *buffer) {
    struct thread_stat *thread_st = (struct thread_stat *)buffer + thread_nb - 1;
    struct connection_thread *thread_info = all_threads;
    int i;
    
    while (thread_info != nullptr) {
        thread_st->nb_connections = thread_info->nb_connections;
        for (i=0; i<3; i++)
            thread_st->info[i] = thread_info->info[i];
        thread_st->active = (thread_info->client_sock >= 0);
        thread_st--;
        thread_info = thread_info->next_all;
    }
}

unsigned char *get_thread_statistics() {
    int *size;
    unsigned char *buffer_out;
    struct connection_thread *thread_info;
    struct thread_stat *thread_st;
    
    pthread_mutex_lock(&all_threads_lock);
        buffer_out = (unsigned char *)malloc(4 + thread_nb*sizeof(struct thread_stat));
        memset(buffer_out, 0, 4 + thread_nb*sizeof(struct thread_stat));
        size = (int *)buffer_out;
        *size = thread_nb * sizeof(struct thread_stat);

        serialize_thread_stats(buffer_out + 4);
    pthread_mutex_unlock(&all_threads_lock);
    
    return buffer_out;
}