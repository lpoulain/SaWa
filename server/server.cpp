#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>

#include "server.h"
#include "sawa_admin.h"
#include "display.h"
#include "thread_pool.h"
#include "sawa.h"
#include "util.h"

using namespace std;

Server::Server(int port) {
    struct sockaddr_in server;
    int option = 1;
    this->port = port;
    
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        Util::setDebugInfo(port);
        throw FAILURE_CREATE_SOCKET;
    }
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        Util::setDebugInfo(port);
        throw FAILURE_BIND;
    }
    
    admin = new AdminInterface();
}

void Server::start() {
    int c, client_sock;
    struct sockaddr_in client;
    
    //Listen
    c = listen(socket_desc , 3);
     
    //Accept and incoming connection
    screen->init();

    c = sizeof(struct sockaddr_in);
     
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        screen->debug("[Socket %d] New request\n", client_sock);
        pool->handleNewConnection(client_sock);
    }
     
    if (client_sock < 0)
    {
        throw FAILURE_ACCEPT;
    }    
}

void Server::stop() {
    shutdown(socket_desc, SHUT_RDWR);
    
    delete admin;
    delete pool;
    
    screen->cleanup();
    delete screen;    
}

Server::~Server() {
}

void Server::updateScreen(ConnectionThread* thread_info, int idx) {
    thread_info->info[idx]++;
    screen->refresh_thread(thread_info, idx);
}

Server *server;
