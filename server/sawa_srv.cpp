#include <fstream>
#include <string.h>
#include <fcntl.h>
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
#include "util.h"

void SawaServer::sendInfo(int socket_fd) {
    screen->debug("INFO command received\n");
    write(socket_fd, &nb_sectors, sizeof(int));
    screen->debug("INFO sent\n");
}

// Checks that the bound .
// If not, sends an SAWA_MSG_ERR error
int SawaServer::checkValidRequest(int socket_fd, uint32_t offset, uint32_t size) {
    char response;
    
    if (size + offset <= nb_sectors * 512) return 1;
    
    response = SAWA_MSG_NOAUTH;
    write(socket_fd, &response, 1);
    return 0;
}

void SawaServer::readFile(int socket_fd, uint32_t offset, uint32_t size) {
    uint8_t *buffer;
    
    screen->debug("[%d] Read %d bytes (0x%x) starting at offset %d\n", socket_fd, size, size, offset);
    
    // Check that the request is valid
    if (!this->checkValidRequest(socket_fd, offset, size)) return;

    fseek(fp, offset, SEEK_SET);
    buffer = new uint8_t[size];
    fread(buffer, sizeof(char), size, fp);
    
    screen->debug("[%d] Sending data...\n", socket_fd);
//    dump_mem(buffer, 32);
    write(socket_fd, buffer, size);
    screen->debug("[%d] ...%d bytes sent\n", socket_fd, size);
    delete [] buffer;
}

void SawaServer::writeFile(int socket_fd, uint8_t* addr, uint32_t offset, uint32_t size) {
    char response = SAWA_MSG_OK;
    screen->debug("[%d] Write %d bytes starting at offset %d\n", socket_fd, size, offset);

    // Check that the request is valid
    if (!this->checkValidRequest(socket_fd, offset, size)) return;
    
    if (fseek(fp, offset, SEEK_SET) >= 0) {
        if (fwrite(addr, sizeof(char), (ssize_t)size, fp) >= 0) {
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

void SawaServer::processRequest(int socket_fd, ConnectionThread* thread_info, uint8_t* addr, uint32_t size) {
    uint32_t offset;
    uint8_t op;
    
    op = addr[0];
    
    if (op == SAWA_INFO) {
        // Processs the INFO command
        this->sendInfo(socket_fd);
        // Updates the thread statistics and update the screen (if appropriate)
        thread_info->info[0]++;
        screen->refresh_thread(thread_info, 0);
        return;
    }
    
    offset = *((uint32_t*)(addr+1));

    switch(op) {
        case SAWA_READ:
            // Retrieve the message size and process the READ command
            size = *((uint32_t*)(addr+1+sizeof(int)));
            this->readFile(socket_fd, offset, size);
            // Updates the thread statistics and update the screen (if appropriate)
            thread_info->info[1]++;
            screen->refresh_thread(thread_info, 1);
            break;
        case SAWA_WRITE:
            // Retrieve the message size and process the WRITE command
            size -= (1 + sizeof(int));
            this->writeFile(socket_fd, addr + 1 + sizeof(int), offset, size);
            // Updates the thread statistics and update the screen (if appropriate)
            thread_info->info[2]++;
            screen->refresh_thread(thread_info, 2);
            break;
    }
}

// Reads data from the network
void SawaServer::readData(ConnectionThread* thread_info) {
    int socket_fd = thread_info->client_sock;
    int n;
    uint32_t expected_size;
    uint8_t *buffer_in;
    
    for (;;) {
    
        n = read(socket_fd, (char*)(&expected_size), 4);
        if (n < 4) return;

        if (expected_size > 32768) return;

        if (expected_size > 0) {
            buffer_in = new uint8_t[expected_size];
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

// Opens the filesystem file, creates it if need be
FILE *SawaServer::getFilesystemFile() {
    int nb_bytes;
    FILE *fp;
    
    fp= fopen(filesystem.c_str(), "r+");
    if (fp != NULL) return fp;
    
    screen->debug("Filesystem file does not exist, creating it...\n");
    fp = fopen(filesystem.c_str(), "w+");
    
    if (fp == NULL) {
        Util::setDebugInfo(errno);
        Util::setDebugInfo(filesystem);
        throw FAILURE_OPEN_FILE;
    }
    
    nb_bytes = nb_sectors * 512;
    uint8_t *buffer = new uint8_t[nb_bytes];
    memset(buffer, 0, nb_bytes);
    fwrite(buffer, sizeof(char), nb_bytes, fp);
    delete [] buffer;
    
    return fp;
}

SawaServer::SawaServer() : Server(5000) {
    this->nb_sectors = 2048;
    this->filesystem = "./filesystem";
    
    fp = this->getFilesystemFile();
}

SawaServer::~SawaServer() {
    fclose(fp);
}