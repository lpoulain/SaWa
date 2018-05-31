#ifndef TOWA_IPC_H
#define TOWA_IPC_H

#include <string>

#define FAILURE_TOWA_IPC_CANNOT_CREATE_PIPE     0x30
#define FAILURE_TOWA_IPC_CANNOT_OPEN_PIPE     0x31

using namespace std;

class Message {
    uint8_t *content;
    int size;
    
public:
    Message(int, uint8_t *);
    Message(FILE *);
    Message(string);
    ~Message();
    void sendTo(FILE *);
    uint8_t *getContent();
    int getSize();
};

class TowaIPC {
public:
    virtual Message *sendMsg(Message *msg) = 0;
    virtual void listenToMsg(Message *(*callback)(Message *)) = 0;
};

class TowaPipe : public TowaIPC {
    FILE *fp_ping;
    FILE *fp_pong;
public:
    virtual Message *sendMsg(Message *msg);
    virtual void listenToMsg(Message *(*callback)(Message *));
    
    TowaPipe(bool);
    ~TowaPipe();
};

class TowaSharedMem : public TowaIPC {
    
};

#endif /* TOWA_IPC_H */
