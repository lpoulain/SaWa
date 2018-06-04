#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <map>
#include "towa_mgr.h"

class ConnectionThread;
struct request_message;
class WebFile;

class Server {
protected:
    int socket_desc;
    
public:
    int port;
    
    Server(int port);
    virtual ~Server();
    virtual void readData(ConnectionThread *thread_info) = 0;
    void updateScreen(ConnectionThread *thread_info, int idx);
    void start();
    void stop();
};

class SawaServer : public Server {
    std::string filesystem;
    uint32_t nb_sectors;
    FILE *fp;
    
    void dumpMem(uint8_t *addr, int size);
    void sendInfo(int socket_fd);
    int checkValidRequest(int socket_fd, uint32_t offset, uint32_t size);
    void readFile(int socket_fd, uint32_t offset, uint32_t size);
    void writeFile(int socket_fd, uint8_t *addr, uint32_t offset, uint32_t size);
    void processRequest(int socket_fd, ConnectionThread *thread_info, uint8_t *addr, uint32_t size);
    FILE *getFilesystemFile();
    
public:
    SawaServer();
    ~SawaServer();
    virtual void readData(ConnectionThread *thread_info);
};

class HTTPServer : public Server {
    TowaMgr *towaMgr;
    std::map<std::string, WebFile *> cache;
    
    int processRequest(int socket_fd, ConnectionThread *thread_info, struct request_message *msg, uint32_t size);
    struct WebFile *getFile(string path);
    string generate500Response(string desc);
    
public:
    HTTPServer();
    ~HTTPServer();
    virtual void readData(ConnectionThread *thread_info);
};

extern Server *server;

#endif /* SERVER_H */
