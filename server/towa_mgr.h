#ifndef TOWA_MGR_H
#define TOWA_MGR_H

#include <string>
#include <vector>
#include "towa_ipc.h"

#define FAILURE_TOWA_INVALID_TYPE   0x60
#define FAILURE_TOWA_CANNOT_SPAWN   0x61

enum TowaType { namedpipe, tcpip, sharedmem };

class TowaServlet {
    string name;
    string path;
    
public:
    TowaServlet(const char *, const char *);
};

class TowaMgr {
    pid_t pid;
    TowaIPC *ipc;
    vector<TowaServlet*> *scanClasspath();
    void launchAppPool();
    void uncompressWar(string);
    TowaServlet *readWebXML(string war);
    
public:
    TowaMgr(TowaType);
    void start();
    void check();
    Message *sendMsg(string verb, string classname, string querystring);
};

#endif /* TOWA_MGR_H */
