#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h> /* for fork */
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h> /* for wait */
#include "towa_mgr.h"

using namespace std;

TowaMgr::TowaMgr(TowaType t) {
    switch(t) {
        case TowaType::namedpipe:
            ipc = new TowaPipe(true);
            break;
        case TowaType::tcpip:
            ipc = new TowaTCP();
            break;
        case TowaType::sharedmem:
            ipc = new TowaSharedMem();
            break;
        default:
            throw FAILURE_TOWA_INVALID_TYPE;
    }
}

void TowaMgr::start() {
    pid=fork();
    if (pid==0) {    // Only executed by the chile process
        static char *argv[]={(char*)"towa"};
        execv("./towa" ,argv);
        exit(127); /* only if execv fails */
    }
    
    pid_t res = waitpid(pid, 0, WNOHANG);
    if (res != 0) throw FAILURE_TOWA_CANNOT_SPAWN;
}

void TowaMgr::check() {
    pid_t res = waitpid(pid, 0, WNOHANG);
    
    if (res != 0) this->start();
//    cout << "Towa process: " << res << endl;
}

Message *TowaMgr::sendMsg(string verb, string classname, string querystring) {
    this->check();
    return ipc->sendMsg(verb, classname, querystring);
}
