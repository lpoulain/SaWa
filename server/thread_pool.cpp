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
#include "server.h"

using namespace std;

sigset_t fSigSet;

/////////////////////////////////////////////////////////////////////////////////////
// Thread startup function
/////////////////////////////////////////////////////////////////////////////////////

ThreadPool *pool;

void *connection_handler(void *ti)
{
    struct ConnectionThread *thread_info = (struct ConnectionThread *)ti;
    int nSig;

    while (1) {
        server->readData(thread_info);
        close(thread_info->client_sock);
        pool->releaseThread(thread_info);
        
        sigwait(&fSigSet, &nSig);
        screen->status(thread_info, 1);
    }
     
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////
// Threads handling TCP connections
/////////////////////////////////////////////////////////////////////////////////////

ConnectionThread::ConnectionThread(int client_sock) {
    int i;
    
    this->next = nullptr;
    this->client_sock = client_sock;
    this->nb_connections = 1;
    for (i=0; i<3; i++) this->info[i] = 0;
    
    pthread_cond_init(&(this->cv), NULL);
    pthread_mutex_init(&(this->mp), NULL);    
}

// There is no idle thread, so we create a new one
struct ConnectionThread *ThreadPool::newThread(int client_sock) {
    ConnectionThread *thread_info = new ConnectionThread(client_sock);
    
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
void ThreadPool::releaseThread(ConnectionThread* thread_info) {
    thread_info->client_sock = -1;

    pthread_mutex_lock(&idle_threads_lock);
        thread_info->next = idle_threads;
        idle_threads = thread_info;
    pthread_mutex_unlock(&idle_threads_lock);

    screen->status(thread_info, 0);
}

// Reuse an idle thread
ConnectionThread *ThreadPool::reuseThread(int client_sock) {
    struct ConnectionThread *thread_info = nullptr;

    // Is there any idle thread?
    if (this->idle_threads == nullptr) return nullptr;
    
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
void ThreadPool::handleNewConnection(int client_sock) {
    // Try to reuse an idle thread
    struct ConnectionThread *thread_info = this->reuseThread(client_sock);
    // If there is no idle thread, create a new one
    if (thread_info == nullptr) this->newThread(client_sock);
}

////////////////////////////////////////////////////////

ThreadPool::ThreadPool() {
    if (pthread_mutex_init(&idle_threads_lock, NULL) != 0 ||
        pthread_mutex_init(&all_threads_lock, NULL) != 0)
    {
        throw FAILURE_MUTEX_INIT;
    }

    sigemptyset(&fSigSet);
    sigaddset(&fSigSet, SIGUSR1);
    sigaddset(&fSigSet, SIGSEGV);

    pthread_sigmask(SIG_BLOCK, &fSigSet, NULL);
}

ThreadPool::~ThreadPool() {
    struct ConnectionThread *thread_info;
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
    
    screen->debug("Shutting down...(%d connections dropped)\n", nb_conn);
}

////////////////////////////////////////////////////////////
// Thread serialization
////////////////////////////////////////////////////////////

void ThreadPool::serializeThreadStats(unsigned char* buffer) {
    ThreadStat *thread_st = (ThreadStat *)buffer + thread_nb - 1;
    ConnectionThread *thread_info = all_threads;
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

unsigned char *ThreadPool::getThreadStatistics() {
    int *size;
    unsigned char *buffer_out;
    
    pthread_mutex_lock(&all_threads_lock);
        buffer_out = (unsigned char *)malloc(4 + thread_nb*sizeof(ThreadStat));
        memset(buffer_out, 0, 4 + thread_nb*sizeof(ThreadStat));
        size = (int *)buffer_out;
        *size = thread_nb * sizeof(ThreadStat);

        serializeThreadStats(buffer_out + 4);
    pthread_mutex_unlock(&all_threads_lock);
    
    return buffer_out;
}
