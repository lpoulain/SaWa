#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include "sawa_client_interface.h"

int debug=1;

////////////////////////////////////////////////////////////////////////////

void print_usage(char *prg) {
    printf("Usage: %s info|read|write <parameters\n", prg);
    printf("- %s info\n", prg);
    printf("- %s read <offset> [<size>]\n", prg);
    printf("- %s write <offset> [<size>]\n", prg);
    printf("- %s test [nb threads]\n", prg);
    printf("- %s stop\n", prg);
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
//        if (argc >= 3) nb_requests = atoi(argv[2]); else nb_requests = 2000;
        if (argc >= 3) nb_threads = atoi(argv[2]);
        printf("Testing %d threads\n", nb_threads);
        sawaClient.testServer(nb_threads);
        return 0;
    }

    if (!strcmp(argv[1], "read")) op = SAWA_READ;
    else if (!strcmp(argv[1], "write")) op = SAWA_WRITE;
    else {
        printf("Unknown operation: %s\n", argv[1]);
        return -1;
    }
    
    if (argc < 3) {
        printf("Offset missing\n");
        print_usage(argv[0]);
        return -1;
    }

    offset = atoi(argv[2]);
    
    if (argc >= 4) size = atoi(argv[3]);
    if (size <= 0) {
        printf("Invalid size: %d\n", size);
        print_usage(argv[0]);
        return -1;
    }

    printf("Operation %d, offset %d, size: %d\n", op, offset, size);
    sawaClient.sendCommand(op, offset, size);
    
    return 0;
}
