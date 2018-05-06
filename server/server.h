#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <map>

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
    void start();
    void stop();
};

class SawaServer : public Server {
    std::string filesystem;
    uint32_t nb_sectors;
    int fd;
    
    void dumpMem(uint8_t *addr, int size);
    void sendInfo(int socket_fd);
    int checkValidRequest(int socket_fd, uint32_t offset, uint32_t size);
    void readFile(int socket_fd, uint32_t offset, uint32_t size);
    void writeFile(int socket_fd, uint8_t *addr, uint32_t offset, uint32_t size);
    void processRequest(int socket_fd, ConnectionThread *thread_info, uint8_t *addr, uint32_t size);
    int getFilesystemFile();
    
public:
    SawaServer();
    ~SawaServer();
    virtual void readData(ConnectionThread *thread_info);
};

class HTTPServer : public Server {
    std::map<std::string, WebFile *> cache;
    
    int strnCaseStr(const char *s, const char *find, const int max);
    int processRequest(int socket_fd, struct request_message *msg, uint32_t size);
    struct WebFile *getFile(char *path);
    
public:
    HTTPServer();
    ~HTTPServer();
    virtual void readData(ConnectionThread *thread_info);
};

extern Server *server;

#endif /* SERVER_H */
