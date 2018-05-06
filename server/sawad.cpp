#include <iostream>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "sawa.h"
#include "thread_pool.h"
#include "display.h"
#include "sawa_admin.h"
#include "server.h"
#include "util.h"

using namespace std;

int debug_flag = 0;

AdminInterface *admin;

/////////////////////////////////////////////////////////////////////////////////////

void ctrl_c_handler(int s) {
    server->stop();

//    delete server;
    exit(0);
}

void set_ctrl_c_handler() {
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = ctrl_c_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    
    sigaction(SIGINT, &sigIntHandler, NULL);
}

void start_as_daemon() {
    pid_t pid, sid;
        
    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
            exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
            exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);

    /* Open any logs here */        

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
            /* Log the failure */
            exit(EXIT_FAILURE);
    }

    /* Change the current working directory */
/*    if ((chdir("/")) < 0) {
            exit(EXIT_FAILURE);
    }
*/
    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* Daemon-specific initialization goes here */

    /* The Big Loop */
    server->start();
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    int daemon = 0;
    int http = 0;
    int quiet = 0;
    int i, display_code;
    
    set_ctrl_c_handler();

    for (i=1; i<argc; i++) {
        if (!strcmp(argv[i], "-daemon")) daemon = 1;
        if (!strcmp(argv[i], "-debug")) debug_flag = 1;
        if (!strcmp(argv[i], "-quiet")) quiet = 1;
        if (!strcmp(argv[i], "-help")) {
            cout << "Usage: " << argv[0] << " [-daemon] [-http]\n" << endl;
            return 0;
        }
        if (!strcmp(argv[i], "-http")) http = 1;
    }

    try {
        if (daemon == 1)
            display_code = DISPLAY_DAEMON;
        else if (debug_flag == 1)
            display_code = DISPLAY_DEBUG;
        else if (quiet == 1)
            display_code = DISPLAY_QUIET;
        else
            display_code = DISPLAY_DEFAULT;
        select_display(display_code);
        
        if (http)
            server = new HTTPServer();
        else
            server = new SawaServer();

        pool = new ThreadPool();

        if (daemon) {
            start_as_daemon();
            return 0;
        }
    
        server->start();
    } catch(int failure_code) {
        Util::displayError(failure_code);
    }
    
    return 0;
}
