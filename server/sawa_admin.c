#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "display.h"
#include "sawa.h"

#define ADMIN_PORT 5001
int admin_socket_desc;

void process_admin_command(int socket_fd, unsigned char *buffer_in, int size) {
    unsigned char op = buffer_in[0];
    
    switch(op) {
        case SAWA_STOP:
            ctrl_c_handler();
            exit(0);
        default:
            break;
    }
}

void read_admin_command(int socket_fd) {
    int n, expected_size;
    unsigned char *buffer_in;
    
    for (;;) {
    
        n = read(socket_fd, (char*)(&expected_size), 4);
        if (n < 4) return;

        if (expected_size > 1024) return;

        if (expected_size > 0) {
            buffer_in = malloc(expected_size);
            n = read(socket_fd, buffer_in, expected_size);
    
            if (n < 0) {
                free(buffer_in);
                return;
            }

            process_admin_command(socket_fd, buffer_in, expected_size);
            free(buffer_in);
        }
    }
}

// Starts a TCP connection on port 5001 and listens to it
static void *admin_connection_handler(void *dummy)
{
    int client_sock , c , *new_sock;
    struct connection *conn;
    struct sockaddr_in server , client;
    int option = 1;
     
    //Create socket
    admin_socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (admin_socket_desc == -1)
    {
        screen.error("Could not create socket");
    }
    setsockopt(admin_socket_desc, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(ADMIN_PORT);
     
    //Bind
    if( bind(admin_socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        screen.error("bind failed. Error");
        return 0;
    }
     
    //Listen
    listen(admin_socket_desc , 3);

    c = sizeof(struct sockaddr_in);
     
    while( (client_sock = accept(admin_socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        screen.debug("[Socket %d] Admin request\n", client_sock);
        read_admin_command(client_sock);
    }
     
    return 0;
}

// Creates a dedicated thread to process administrative commands
int sawa_start_admin_interface() {
    pthread_t *thread = malloc(sizeof(pthread_t));
    
    if( pthread_create(thread, NULL, admin_connection_handler, NULL) < 0)
    {
        screen.error("could not create admin thread");
        return -1;
    }    
    
    return 0;
}
