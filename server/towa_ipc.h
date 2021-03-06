#ifndef TOWA_IPC_H
#define TOWA_IPC_H

#include <string>

#define FAILURE_TOWA_IPC_CANNOT_CREATE_PIPE     0x30
#define FAILURE_TOWA_IPC_CANNOT_OPEN_PIPE     0x31

#define TOWA_HTTP_200   0x01
#define TOWA_HTTP_404   0x02
#define TOWA_HTTP_500   0x03


using namespace std;

class Message {
    uint8_t *content;
    int size;
    char header;
    
public:
    Message(int, uint8_t *);
    Message(FILE *, bool);
    Message(string);
    ~Message();
    void sendTo(FILE *);
    uint8_t *getContent();
    int getSize();
    string getString();
    char getHeader();
};

class TowaIPC {
public:
    virtual Message *sendMsg(string verb, string classname, string querystring) = 0;
    virtual void listenToMsg(Message *(*callback)(Message *)) = 0;
};

class TowaPipe : public TowaIPC {
    FILE *fp_ping;
    FILE *fp_pong;
public:
    virtual Message *sendMsg(string verb, string classname, string querystring);
    virtual void listenToMsg(Message *(*callback)(Message *));
    
    TowaPipe(bool);
    ~TowaPipe();
};

class TowaSharedMem : public TowaIPC {
    virtual Message *sendMsg(string verb, string classname, string querystring);
    virtual void listenToMsg(Message *(*callback)(Message *));
};

class TowaTCP : public TowaIPC {
    virtual Message *sendMsg(string verb, string classname, string querystring);
    virtual void listenToMsg(Message *(*callback)(Message *));    
};

#endif /* TOWA_IPC_H */
