#include <stdio.h>
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
        
/*        pthread_mutex_lock(&(thread_info->mp));
        pthread_cond_wait(&(thread_info->cv), &(thread_info->mp));
        pthread_mutex_unlock(&(thread_info->mp));*/
        
        sigwait(&fSigSet, &nSig);
        screen.debug("Resumed thread %d\n", thread_info->nb);
    }
     
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////
// Threads handling TCP connections
/////////////////////////////////////////////////////////////////////////////////////

pthread_mutex_t idle_threads_lock;
pthread_mutex_t all_threads_lock;

struct connection_thread *idle_threads = NULL;
struct connection_thread *all_threads = NULL;

// There is no idle thread, so we create a new one
struct connection_thread *new_thread(int client_sock) {
    int i;
    struct connection_thread *thread_info = malloc(sizeof(struct connection_thread));
    
    thread_info->next = NULL;
    thread_info->client_sock = client_sock;
    thread_info->nb_connections = 1;
    for (i=0; i<3; i++) thread_info->info[i] = 0;
    
    pthread_cond_init(&(thread_info->cv), NULL);
    pthread_mutex_init(&(thread_info->mp), NULL);

    pthread_mutex_lock(&all_threads_lock);
        thread_info->nb = thread_nb++;
        thread_info->next = all_threads;
        all_threads = thread_info;
    pthread_mutex_unlock(&all_threads_lock);
    
    if( pthread_create( &thread_info->thread, NULL, connection_handler, (void*)thread_info) < 0)
    {
        perror("could not create thread");
        return 0;
    }    

    screen.new_thread(thread_info);
    
    return thread_info;
}

// A thread is done executing, put it back in the unassigned pool
void release_thread(struct connection_thread *thread_info) {
    thread_info->client_sock = -1;

    pthread_mutex_lock(&idle_threads_lock);
        thread_info->next = idle_threads;
        idle_threads = thread_info;
//        pthread_cond_init(&(thread_info->cv), NULL);
//        pthread_mutex_init(&(thread_info->mp), NULL);
    pthread_mutex_unlock(&idle_threads_lock);

    screen.debug("Release thread %d\n", thread_info);
}

// Reuse an idle thread
struct connection_thread *reuse_thread(int client_sock) {
    struct connection_thread *thread_info = NULL;

    // Is there any idle thread?
    if (idle_threads == NULL) return NULL;
    
    // If there is, remove it from the idle queue
    pthread_mutex_lock(&idle_threads_lock);
        thread_info = idle_threads;
        idle_threads = thread_info->next;
    pthread_mutex_unlock(&idle_threads_lock);   
    
    // Assign it to the connection
    thread_info->client_sock = client_sock;
    thread_info->nb_connections++;
    
    // Wake up the thread
    screen.update(thread_info);
    pthread_kill(thread_info->thread, SIGUSR1);
    
    return thread_info;
}

////////////////////////////////////////////////////////
void handle_new_connection(int client_sock) {
    // Try to reuse an idle thread
    struct connection_thread *thread_info = reuse_thread(client_sock);
    // If there is no idle thread, create a new one
    if (thread_info == NULL) new_thread(client_sock);
}

////////////////////////////////////////////////////////

int thread_pool_init() {
    if (pthread_mutex_init(&idle_threads_lock, NULL) != 0 ||
        pthread_mutex_init(&all_threads_lock, NULL) != 0)
    {
        printf("\n mutex init has failed\n");
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

    while (all_threads != NULL) {
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