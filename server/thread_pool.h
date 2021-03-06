#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

class ConnectionThread {
    pthread_cond_t cv;
    pthread_mutex_t mp;
    
public:
    int nb;
    pthread_t thread;
    int client_sock;
    int nb_connections;
    int info[3];
    struct ConnectionThread *next;
    struct ConnectionThread *next_all;
    
    ConnectionThread(int client_sock);
};

class ThreadStat {
    int nb_connections;
    int info[3];
    char active;
    
public:
    void loadFrom(ConnectionThread *thread_info);
};

class ThreadPool {
    pthread_mutex_t idle_threads_lock;
    pthread_mutex_t all_threads_lock;
    
    ConnectionThread *idle_threads;
    ConnectionThread *all_threads;
    int thread_nb;

    ConnectionThread *newThread(int client_sock);
    ConnectionThread *reuseThread(int client_sock);
    void serializeThreadStats(uint8_t *buffer);
    
public:
    ThreadPool();
    void handleNewConnection(int client_sock);
    void releaseThread(struct ConnectionThread *thread_info);
    uint8_t *getThreadStatistics();
    ~ThreadPool();
};

extern ThreadPool *pool;

#endif
