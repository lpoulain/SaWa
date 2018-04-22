extern void handle_new_connection(int client_sock);
extern int thread_pool_init();
extern int thread_pool_cleanup();

struct connection_thread {
    int nb;
    struct connection_thread *next;
    pthread_t thread;
    pthread_cond_t cv;
    pthread_mutex_t mp;
    int client_sock;
    int nb_connections;
    int info[3];
};
