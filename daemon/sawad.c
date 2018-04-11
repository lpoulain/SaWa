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
#include "thread_pool.h"

int server_port = 5000;
int debug = 0;

/////////////////////////////////////////////////////////////////////////////////////

struct winsize w;

void display() {
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    printf ("lines %d\n", w.ws_row);
    printf ("columns %d\n", w.ws_col);    
}

void error(const char *msg) {
    printf("Error: %s\n", msg);
}

int socket_desc;

void ctrl_c_handler(int s) {
    int nb_conn = thread_pool_cleanup();
    
    shutdown(socket_desc, SHUT_RDWR);
    printf("Shutting down...(%d connections dropped)\n", nb_conn);
    
    exit(1);
}

void set_ctrl_c_handler() {
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = ctrl_c_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    
    sigaction(SIGINT, &sigIntHandler, NULL);
}

int sawa_server_start() {
    int client_sock , c , *new_sock;
    struct connection *conn;
    struct sockaddr_in server , client;
    int option = 1;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(server_port);
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
     
    //Listen
    c = listen(socket_desc , 3);
     
    //Accept and incoming connection
    printf("Server started on port %d\n", server_port);
    c = sizeof(struct sockaddr_in);
     
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        handle_new_connection(client_sock);
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;    
}

void start_as_daemon() {
    pid_t pid, sid;
        
    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
            exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
            exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);

    /* Open any logs here */        

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
            /* Log the failure */
            exit(EXIT_FAILURE);
    }



    /* Change the current working directory */
    if ((chdir("/")) < 0) {
            /* Log the failure */
            exit(EXIT_FAILURE);
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* Daemon-specific initialization goes here */

    /* The Big Loop */
/*    while (1) {

        if (pthread_mutex_init(&lock, NULL) != 0)
        {
            printf("\n mutex init has failed\n");
            return;
        }

        sawa_server_start();
    }*/
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    int daemon = 0;
    int http = 0;
    int i;
    
    set_ctrl_c_handler();

    for (i=1; i<argc; i++) {
        if (!strcmp(argv[i], "-daemon")) daemon = 1;
        if (!strcmp(argv[i], "-debug")) debug = 1;
        if (!strcmp(argv[i], "-help")) {
            printf("Usage: %s [-deamon] [-http]\n", argv[0]);
            return 0;
        }
        if (!strcmp(argv[i], "-http")) http = 1;
    }

    if (http)
        HTTP_init();
    else
        sawa_init();
    
    // Currently not working
    if (daemon) {
        start_as_daemon();
        return 0;
    }
    
    if (thread_pool_init()) return 1;

    sawa_server_start();
    
    return 0;
}
