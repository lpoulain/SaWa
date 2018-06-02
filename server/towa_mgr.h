#ifndef TOWA_MGR_H
#define TOWA_MGR_H

#include <string>
#include "towa_ipc.h"

#define FAILURE_TOWA_INVALID_TYPE   0x60
#define FAILURE_TOWA_CANNOT_SPAWN   0x61

enum TowaType { namedpipe, tcpip, sharedmem };

class TowaMgr {
    pid_t pid;
    TowaIPC *ipc;
    
public:
    TowaMgr(TowaType);
    void start();
    void check();
    Message *sendMsg(string verb, string classname, string querystring);
};

#endif /* TOWA_MGR_H */
