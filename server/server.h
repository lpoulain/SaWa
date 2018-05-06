#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <map>

class ConnectionThread;
struct request_message;
struct web_file;

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
    unsigned int nb_sectors;
    int fd;
    
    void dumpMem(unsigned char *addr, int size);
    void sendInfo(int socket_fd);
    int checkValidRequest(int socket_fd, unsigned int offset, unsigned int size);
    void readFile(int socket_fd, unsigned int offset, unsigned int size);
    void writeFile(int socket_fd, unsigned char *addr, unsigned int offset, unsigned int size);
    void processRequest(int socket_fd, ConnectionThread *thread_info, unsigned char *addr, unsigned int size);
    int getFilesystemFile();
    
public:
    SawaServer();
    ~SawaServer();
    virtual void readData(ConnectionThread *thread_info);
};

class HTTPServer : public Server {
    std::map<std::string, struct web_file *> cache;
    
    int strnCaseStr(const char *s, const char *find, const int max);
    int processRequest(int socket_fd, struct request_message *msg, unsigned int size);
    struct web_file *getFile(char *path);
    
public:
    HTTPServer();
    ~HTTPServer();
    virtual void readData(ConnectionThread *thread_info);
};

extern Server *server;

#endif /* SERVER_H */
