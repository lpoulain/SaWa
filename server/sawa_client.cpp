#include <cstdint>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include "sawa_client_interface.h"

using namespace std;
int debug=1;

////////////////////////////////////////////////////////////////////////////

void print_usage(char *prg) {
    cout << "Usage: " << prg << " info|read|write|test|stop [parameters]" << endl;
    cout << "- " << prg << " info" << endl;
    cout << "- " << prg << " read <offset> [<size>]" << endl;
    cout << "- " << prg << " write <offset> [<size>]" << endl;
    cout << "- " << prg << " test [# of threads]" << endl;
    cout << "- " << prg << " stop" << endl;
}

int main(int argc, char *argv[]) {
    unsigned int op, offset, size = 4096;
    int socket_fd, nb_requests, nb_threads = 1;

    SawaClient sawaClient = SawaClient();
    
    if (argc < 2)
    {
        print_usage(argv[0]);
        return -1;
    }
    
    if (!strcmp(argv[1], "info")) {
        sawaClient.sendInfoCmd();
        return 0;
    }
    
    if (!strcmp(argv[1], "stop")) {
        sawaClient.sendStopCmd();
        return 0;
    }

    if (!strcmp(argv[1], "stat")) {
        sawaClient.sendStatCmd();
        return 0;
    }

    if (!strcmp(argv[1], "test")) {
        if (argc >= 3) nb_threads = atoi(argv[2]);
        cout << "Testing " << nb_threads << " threads" << endl;
        sawaClient.testServer(nb_threads);
        return 0;
    }

    if (!strcmp(argv[1], "read")) op = SAWA_READ;
    else if (!strcmp(argv[1], "write")) op = SAWA_WRITE;
    else {
        cerr << "Unknown operation: " << argv[1] << endl;
        print_usage(argv[0]);
        return -1;
    }
    
    if (argc < 3) {
        cerr << "Offset missing" << endl;
        print_usage(argv[0]);
        return -1;
    }

    offset = atoi(argv[2]);
    
    if (argc >= 4) size = atoi(argv[3]);
    if (size <= 0) {
        cerr << "Invalid size: " << size << endl;
        print_usage(argv[0]);
        return -1;
    }

    cout << "Operation: " << op << "   offset: " << offset << "   size: " << size << endl;
    sawaClient.sendCommand(op, offset, size);
    
    return 0;
}
