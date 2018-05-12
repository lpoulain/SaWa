#include <iostream>
#include <fstream>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>

#include "sawa.h"
#include "display.h"
#include "thread_pool.h"
#include "server.h"
#include "util.h"

using namespace std;

const char *HTTP_200 = "HTTP/1.1 200 OK\nKeep-Alive: timeout=15, max=95\nConnection: Keep-Alive\n";
const char *HTTP_404 = "HTTP/1.1 404 Not Found\nContent-Type: text/html; charset=UTF-8\nContent-Length: 12\n\nError 404 - Not found\n\n";
const char *HTTP_500 = "HTTP/1.1 500 Server Error\n\n";
const char *root_dir = "./wwwroot";
const char *index_html = "index.html";
int HTTP_200_len;
int HTTP_404_len;
int HTTP_500_len;
int root_dir_len;
int index_html_len;

#define request_message_len 1016

struct request_message {
    uint8_t data[request_message_len];
    struct request_message *next;
};

class WebFile {
    char *content;
    int size;
    
public:
    char *getContent() { return content; }
    int getSize() { return size; }
    
    WebFile(char *path, int file_size) {
        this->size = file_size;
        this->content = new char[file_size];
    
        FILE *fp = fopen(path, "r");
        fseek(fp, 0, SEEK_SET);
        fread(this->content, sizeof(char), this->size, fp);
        fclose(fp);        
    }
    
    ~WebFile() {
        delete [] content;
    }
};

// Used to cache the files read from the disk
// It is a very simplistic cache which does not release anything *ever*
map<string, WebFile *> cache;

WebFile *HTTPServer::getFile(char* path) {
    WebFile *the_file;
    int file_size;

    // Check if the file is cached. If it is, return it
    the_file = cache[path];
    if (the_file != nullptr) return the_file;

    // If not, find the file size
    ifstream in(path, std::ifstream::ate | std::ifstream::binary);
    file_size = in.tellg(); 

    // The file doesn't exist
    if (file_size <= 0) return nullptr;

    // The file exists, loads it in memory
    the_file = new WebFile(path, file_size);

    // Then save it in the cache
    cache[path] = the_file;
    
    return the_file;
}

bool HTTPServer::isRequestValid(char *buffer_in) {
    return strncmp(buffer_in, "GET /", 5) == 0;
}

char *HTTPServer::parseUrl(char *buffer_in) {
    char *url;
    int first_line_end = strlen(buffer_in), url_start, url_end, url_length;
    
    url_start = 4;
    url_end = 5;
    while (url_end < first_line_end && buffer_in[url_end] != ' ' && buffer_in[url_end] != '?')
        url_end++;

    url_length = url_end - url_start;

    if (buffer_in[url_end-1] == '/')
        url = new char[url_length + root_dir_len + index_html_len + 1];
    else
        url = new char[url_length + root_dir_len + 1];
    
    strcpy(url, root_dir);
    strncpy(url + root_dir_len, buffer_in + url_start, url_length);

    if (buffer_in[url_end-1] == '/') {
        strcpy(url + root_dir_len + url_length, index_html);
        url_length += index_html_len;
    }
        
    url[url_length + root_dir_len] = 0;
    
    return url;
}

// Process the HTTP request
// Request 0 if close connection
// Request 1 if keep alive
int HTTPServer::processRequest(int socket_fd, ConnectionThread *thread_info, request_message* msg, uint32_t size) {
    struct request_message *tmp_msg = msg;
    char *buffer_out, *buffer_in = (char*)msg;
    char *url;
    int header_size;
    int keep_alive = 0;
    WebFile *the_file;

    // If the HTTP request is not valid, return an HTTP error 500
    if (!this->isRequestValid(buffer_in)) {
        Util::dumpMem((uint8_t*)msg, 32);
        write(socket_fd, HTTP_500, HTTP_500_len);
        delete msg;
        return 0;
    }
    
    // Retrieves the URL from the request
    url = this->parseUrl(buffer_in);
    
    // Checks whether the connection should be kept alive
    keep_alive = Util::strnCaseStr(buffer_in, "Connection: Keep-Alive", request_message_len);
    
    // Analysis over. Processing the request
    screen->debug("[Socket %d] requested file: [%s]. Keep-alive=%d\n", socket_fd, url, keep_alive);
    
    the_file = this->getFile(url);
    
    // We are done reading the HTTP request, free the resource
    delete [] url;
    while (tmp_msg != nullptr) {
        tmp_msg = tmp_msg->next;
        delete msg;
        msg = tmp_msg;
    }
    
    // If the file doesn't exist, return an HTTP 404 message
    if (!the_file) {
        write(socket_fd, HTTP_404, HTTP_404_len);
        return 0;
    }

    thread_info->info[1]++;
    screen->refresh_thread(thread_info, 1);
    
    // Otherwise, write the HTTP headers
    buffer_out = new char[200];
    sprintf(buffer_out, "%sContent-Length: %d\nContent-Type: text/html;\n\n", HTTP_200, the_file->getSize());
    header_size = strlen(buffer_out);
    
    write(socket_fd, buffer_out, header_size);
    write(socket_fd, the_file->getContent(), the_file->getSize());
    
    delete [] buffer_out;
    
    return keep_alive;
}

void HTTPServer::readData(ConnectionThread* thread_info) {
    int socket_fd = thread_info->client_sock;
    int n = 1;
    struct request_message *top_msg = new request_message();
    memset(top_msg, 0, sizeof(struct request_message));
    struct request_message *msg = top_msg, *tmp_msg;

    while (1) {
        n = read(socket_fd, (char *)msg, request_message_len);
        
        if (n <= 0) {
            delete top_msg;
            return;
        }
        screen->debug("[Socket %d] %d bytes request\n", socket_fd, n);

        // If this is the end of the message, send it
        if (n < request_message_len || strcmp((char*)msg + request_message_len - 4, "\r\n\r\n")) {
            // If the connection is not Keep-Alive, break the connection
            if (!this->processRequest(socket_fd, thread_info, top_msg, n)) return;
            
            // Otherwise be ready for another message
            top_msg = new request_message();
            top_msg->next = nullptr;
            msg = top_msg;
        }
        
        // If the message is too large to fit in a request_message structure
        // Create another one and keep listening
        else {
            tmp_msg = new request_message();
            msg->next = tmp_msg;
            msg = tmp_msg;
        }
    }
}

HTTPServer::HTTPServer() : Server(8080) {
    HTTP_200_len = strlen(HTTP_200);
    HTTP_404_len = strlen(HTTP_404);
    HTTP_500_len = strlen(HTTP_500);
    root_dir_len = strlen(root_dir);
    index_html_len = strlen(index_html);    
}

HTTPServer::~HTTPServer() {
    cache.clear();
}
