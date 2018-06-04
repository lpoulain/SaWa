#include <iostream>
#include <cstdlib>
#include <jni.h>
#include "towa_ipc.h"
#include "towa_app_pool.h"
/*
 * ToWa: Tomcat Wannabe
 */

using namespace std;

int debug_flag = 1;
TowaAppPool *app;

Message *callback(Message *msg) {
    return app->processRequest(msg);
}

int main(int argc, char** argv) {
    try {
        app = new TowaAppPool();
    
        TowaIPC *ipc = new TowaPipe(false);
        while (1) {
            ipc->listenToMsg(callback);
        }
    }
    catch (int e) {
        cout << "Error " << e << endl;
        cout << errno << endl;
    }
    
    return 0;
}
