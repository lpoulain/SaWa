#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <iterator>
#include <regex>

#include "sawa.h"
#include "display.h"
#include "thread_pool.h"
#include "server.h"
#include "util.h"
#include "towa_ipc.h"
#include "towa_mgr.h"

using namespace std;

string HTTP_200 = string("HTTP/1.1 200 OK\nKeep-Alive: timeout=15, max=95\nConnection: Keep-Alive\n");
string HTTP_404 = string("HTTP/1.1 404 Not Found\nContent-Type: text/html; charset=UTF-8\nContent-Length: 12\n\nError 404 - Not found\n\n");
string HTTP_500 = string("HTTP/1.1 500 Server Error\n\n");
const char *root_dir = "./wwwroot";
const char *index_html = "index.html";
int HTTP_200_len;
int HTTP_404_len;
int HTTP_500_len;
int root_dir_len;
int index_html_len;

#define HTTP_200_COL    0
#define HTTP_404_COL    1
#define HTTP_500_COL    2

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
    
    WebFile(string path, int file_size) {
        this->size = file_size;
        this->content = new char[file_size];
    
        FILE *fp = fopen(path.c_str(), "r");
        fseek(fp, 0, SEEK_SET);
        fread(this->content, sizeof(char), this->size, fp);
        fclose(fp);        
    }
    
    WebFile(Message *msg) {
        this->size = msg->getSize();
        this->content = (char*)msg->getContent();
    }
    
    ~WebFile() {
        delete [] content;
    }
};

class HTTPRequest {
    string method;
    string path;
    string query_string;
    vector<string> headers;
    bool valid;
    bool keep_alive;
    
public:
    HTTPRequest(char *buffer_in) {
        int idx = 0;
        istringstream ss(buffer_in);
        string s;
        std::getline(ss, s);
        method = "GET";
        query_string = "";
        keep_alive = 0;
        smatch m;
//        try {
        regex e("^GET\\s+([\\w\\.\\-/]+)(\\?(.*))?\\s+HTTP");
        while (std::regex_search (s,m,e)) {
            for (auto x:m) {
                switch(idx) {
                    case 1:
                        path = x;
                    case 3:
                        query_string = x;
                }

                idx++;
            }

            s = m.suffix().str();
        }            

        cout << "Path=" << path << endl;
        
        // Checks that the regex successfully parsed the request
        valid = !path.empty();
        // Checks whether the connection should be kept alive
        keep_alive = Util::strnCaseStr(buffer_in, "Connection: Keep-Alive", request_message_len);
    }
    
    string getMethod() { return method; }
    string getPath() { return path; }
    string getQueryString() { return query_string; }
    bool isValid() { return valid; }
    bool isKeepAlive() { return keep_alive; }
};

// Used to cache the files read from the disk
// It is a very simplistic cache which does not release anything *ever*
map<string, WebFile *> cache;

inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

WebFile *HTTPServer::getFile(string path) {
    WebFile *the_file;
    int file_size;
    struct stat st;
    
    path = root_dir + path;
    
    // Check if the file is cached. If it is, return it
    the_file = cache[path];
    if (the_file != nullptr) return the_file;
    cout << "Path: " << path << endl;
    // If not, check if the file exists on the filesystem
    if (stat(path.c_str(), &st) != 0) return nullptr;
    
    // If this is a folder, check for <path>/index.html
    if (st.st_mode & S_IFDIR != 0) {
        if (ends_with(path, "/"))
            path = path + index_html;
        else
            path = path + "/" + index_html;
    
        cout << "Path (again): " << path << endl;
        // Check again if the file exists
        if (stat(path.c_str(), &st) != 0) return nullptr;
    }
    
    // The file exists, loads it in memory
    file_size = st.st_size;
    the_file = new WebFile(path, file_size);

    // Then save it in the cache
    cache[path] = the_file;
    
    return the_file;
}

string HTTPServer::generate500Response(string desc) {
    string content = "<h1>500 Internal Server Error</h1>\n" + desc;
    string msg500 = "HTTP/1.1 500 Internal Server Error\nContent-Type: text/html; charset=UTF-8\nContent-Length: " + to_string(content.size()) + "\n\n" + content;
    
    return msg500;
}

// Process the HTTP request
// Request 0 if close connection
// Request 1 if keep alive
int HTTPServer::processRequest(int socket_fd, ConnectionThread *thread_info, request_message* req_msg, uint32_t size) {
    struct request_message *tmp_msg = req_msg;
    char *buffer_in = (char*)req_msg;
    string buffer_out;
    char *url;
    int header_size;
    WebFile *the_file;
    HTTPRequest request(buffer_in);
    
    // If the HTTP request is not valid, return an HTTP error 500
    if (!request.isValid()) {
        updateScreen(thread_info, HTTP_500_COL);
        Util::dumpMem((uint8_t*)req_msg, 32);
        buffer_out = generate500Response("Invalid request");
        write(socket_fd, buffer_out.c_str(), buffer_out.size());
        delete req_msg;
        return 0;
    }
    
    // Analysis over. Processing the request
    screen->debug("[Socket %d] requested file: [%s]. Keep-alive=%d\n", socket_fd, request.getPath().c_str(), request.isKeepAlive());
    
    if (towa_flag) {
        Message *msg_in = towaMgr->sendMsg(request.getMethod(), request.getPath().substr(1), request.getQueryString());
        uint8_t *content = msg_in->getContent();
        switch(msg_in->getHeader()) {
            case TOWA_HTTP_200:
                the_file = new WebFile(msg_in);
                break;
            case TOWA_HTTP_404:
                break;
            default:
                updateScreen(thread_info, HTTP_500_COL);
//                Util::dumpMem((uint8_t*)msg, 32);
                string msg500 = generate500Response(string((char*)content, msg_in->getSize()));
                write(socket_fd, msg500.c_str(), msg500.size());
                delete req_msg;
                return 0;                
        }
        
        the_file = new WebFile(msg_in);
    }
    else {
        the_file = this->getFile(request.getPath());
    }
    
    // We are done reading the HTTP request, free the resource
    while (tmp_msg != nullptr) {
        tmp_msg = tmp_msg->next;
        delete req_msg;
        req_msg = tmp_msg;
    }
    
    // If the file doesn't exist, return an HTTP 404 message
    if (!the_file) {
        updateScreen(thread_info, HTTP_404_COL);
        write(socket_fd, HTTP_404.c_str(), HTTP_404_len);
        return 0;
    }

    updateScreen(thread_info, HTTP_200_COL);
    
    // Otherwise, write the HTTP headers
    buffer_out = HTTP_200 + "Content-Length: " + to_string(the_file->getSize()) + "\nContent-Type: text/html;\n\n";
    header_size = buffer_out.length();
    
    write(socket_fd, buffer_out.c_str(), header_size);
    write(socket_fd, the_file->getContent(), the_file->getSize());
    
    return request.isKeepAlive();
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
    HTTP_200_len = HTTP_200.length();
    HTTP_404_len = HTTP_404.length();
    HTTP_500_len = HTTP_500.length();
    root_dir_len = strlen(root_dir);
    index_html_len = strlen(index_html);    
    
    if (towa_flag) {
        towaMgr = new TowaMgr(namedpipe);
        towaMgr->start();
    }
}

HTTPServer::~HTTPServer() {
    cache.clear();
}
