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

#include "sawa.h"
#include "display.h"
#include "thread_pool.h"
#include "server.h"

void SawaServer::sendInfo(int socket_fd) {
    char buffer_out[4];
    screen->debug("INFO command received\n");
    write(socket_fd, &nb_sectors, sizeof(int));
    screen->debug("INFO sent\n");
}

// Checks that the bound .
// If not, sends an SAWA_MSG_ERR error
int SawaServer::checkValidRequest(int socket_fd, unsigned int offset, unsigned int size) {
    char response;
    
    if (size + offset <= nb_sectors * 512) return 1;
    
    response = SAWA_MSG_NOAUTH;
    write(socket_fd, &response, 1);
    return 0;
}

void SawaServer::readFile(int socket_fd, unsigned int offset, unsigned int size) {
    unsigned char *buffer;
    
    screen->debug("[%d] Read %d bytes (0x%x) starting at offset %d\n", socket_fd, size, size, offset);
    
    // Check that the request is valid
    if (!this->checkValidRequest(socket_fd, offset, size)) return;

    lseek(fd, offset, SEEK_SET);
    buffer = new unsigned char[size];
    read(fd, buffer, size);
    
    screen->debug("[%d] Sending data...\n", socket_fd);
//    dump_mem(buffer, 32);
    write(socket_fd, buffer, size);
    screen->debug("[%d] ...%d bytes sent\n", socket_fd, size);
    delete [] buffer;
}

void SawaServer::writeFile(int socket_fd, unsigned char* addr, unsigned int offset, unsigned int size) {
    char response = SAWA_MSG_OK;
    unsigned char *buffer;
    screen->debug("[%d] Write %d bytes starting at offset %d\n", socket_fd, size, offset);

    // Check that the request is valid
    if (!this->checkValidRequest(socket_fd, offset, size)) return;
    
    if (lseek(fd, offset, SEEK_SET) >= 0) {
        if (write(fd, addr, (ssize_t)size) >= 0) {
            sync();
            
            // Sends OK message
            write(socket_fd, &response, 1);
            screen->debug("[%d]... done\n", socket_fd);
            return;
        }
    }
    
    response = SAWA_MSG_ERR;
    write(socket_fd, &response, 1);
    screen->error("[%d]... Error %d\n", socket_fd, errno);
}

void SawaServer::processRequest(int socket_fd, ConnectionThread* thread_info, unsigned char* addr, unsigned int size) {
    unsigned int offset;
    unsigned char op;
    
    op = addr[0];
    
    if (op == SAWA_INFO) {
        this->sendInfo(socket_fd);
        thread_info->info[0]++;
        screen->refresh_thread(thread_info, 0);
        return;
    }
    
    offset = *((unsigned int*)(addr+1));

    switch(op) {
        case SAWA_READ:
            size = *((unsigned int*)(addr+1+sizeof(int)));
            this->readFile(socket_fd, offset, size);
            thread_info->info[1]++;
            screen->refresh_thread(thread_info, 1);
            break;
        case SAWA_WRITE:
            size -= (1 + sizeof(int));
            this->writeFile(socket_fd, addr + 1 + sizeof(int), offset, size);
            thread_info->info[2]++;
            screen->refresh_thread(thread_info, 2);
            break;
    }
}

void SawaServer::readData(ConnectionThread* thread_info) {
    int socket_fd = thread_info->client_sock;
    int n;
    unsigned int expected_size;
    unsigned char *buffer_in;
    
    for (;;) {
    
        n = read(socket_fd, (char*)(&expected_size), 4);
        if (n < 4) return;

        if (expected_size > 32768) return;

        if (expected_size > 0) {
            buffer_in = new unsigned char[expected_size];
            n = read(socket_fd, buffer_in, expected_size);
    
            if (n < 0) {
                delete [] buffer_in;
                return;
            }

            this->processRequest(socket_fd, thread_info, buffer_in, expected_size);
            delete [] buffer_in;
        }
    }
}

int SawaServer::getFilesystemFile() {
    int nb_bytes;
    int fd;
    
    fd= open(filesystem.c_str(), O_RDWR, 0700);
    if (fd > 0) return fd;
    
    screen->debug("Filesystem file does not exist, creating it...\n");
    fd = open(filesystem.c_str(), O_RDWR|O_CREAT, 0700);
    
    nb_bytes = nb_sectors * 512;
    unsigned char *buffer = new unsigned char[nb_bytes];
    memset(buffer, 0, nb_bytes);
    write(fd, buffer, nb_bytes);
    delete [] buffer;
    
    return fd;
}

SawaServer::SawaServer() : Server(5000) {
    this->nb_sectors = 2048;
    this->filesystem = "./filesystem";
    
    fd = this->getFilesystemFile();
}

SawaServer::~SawaServer() {
    close(fd);
}