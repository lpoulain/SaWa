#include <fstream>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <search.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "display.h"
#include "sawa.h"
#include "thread_pool.h"
#include "sawa_admin.h"

#define ADMIN_PORT 5001

extern void ctrl_c_handler(int s);

void AdminInterface::statCommand(int socket_fd) {
    unsigned char *buffer = pool->getThreadStatistics();
    int *buffer_size = (int*)buffer;
    
    write(socket_fd, buffer, *buffer_size);
    delete [] buffer;
}

void AdminInterface::processCommand(int socket_fd, unsigned char *buffer_in, int size) {
    unsigned char op = buffer_in[0];
    
    switch(op) {
        case SAWA_STOP:
            ctrl_c_handler(0);
            exit(0);
        case SAWA_STAT:
            this->statCommand(socket_fd);
            break;
        default:
            break;
    }
}

void AdminInterface::readCommand(int socket_fd) {
    int n, expected_size;
    unsigned char *buffer_in;
    
    for (;;) {
    
        n = read(socket_fd, (char*)(&expected_size), 4);
        if (n < 4) return;

        if (expected_size > 1024) return;

        if (expected_size > 0) {
            buffer_in = new unsigned char[expected_size];
            n = read(socket_fd, buffer_in, expected_size);
    
            if (n < 0) {
                delete [] buffer_in;
                return;
            }

            this->processCommand(socket_fd, buffer_in, expected_size);
            delete [] buffer_in;
        }
    }
}

void *AdminInterface::connectionHandler(void *ptr) {
    int client_sock , c , *new_sock;
    AdminInterface *admin = (AdminInterface*)ptr;
    struct sockaddr_in server , client;
    int option = 1;
     
    //Create socket
    admin->socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (admin->socket_desc == -1)
    {
        screen->error("Could not create socket");
    }
    setsockopt(admin->socket_desc, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(ADMIN_PORT);
     
    //Bind
    if( bind(admin->socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        screen->error("bind failed. Error");
        return 0;
    }
     
    //Listen
    listen(admin->socket_desc , 3);

    c = sizeof(struct sockaddr_in);
     
    while( (client_sock = accept(admin->socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        screen->debug("[Socket %d] Admin request\n", client_sock);
        admin->readCommand(client_sock);
    }
     
    return 0;    
}

AdminInterface::AdminInterface() {
    this->thread = new pthread_t();

    if(pthread_create(thread, NULL, AdminInterface::connectionHandler, this) < 0)
    {
        screen->error("could not create admin thread");
    }    

}

AdminInterface::~AdminInterface() {
    screen->debug("Shutting down admin interface\n");

    shutdown(this->socket_desc, SHUT_RDWR);
    delete this->thread;
}
