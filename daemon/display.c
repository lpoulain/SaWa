#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sawa.h"

void display_init() {
    if (debug) return;
    
    char buffer[32];
    sprintf(buffer, "Server started on port %d\n", server_port);
    
    initscr();
    cbreak();
    noecho();
    clear();
    
    mvaddstr(0, 0, buffer);
    mvaddstr(1, 0, "Thread #   :");
    mvaddstr(2, 0, "Connections:");
    
    refresh();
}

void display_new_thread(int thread_no) {
    if (debug) return;
    
    char buffer[6];
    memset(buffer, 6, 0);
    sprintf(buffer, "%d", thread_no);
    
    mvaddstr(1, thread_no * 6 + 14, buffer);
    refresh();
}

void display(int thread_no, int nb_connections) {
    if (debug) return;
    
    char buffer[6];
    memset(buffer, 6, 0);
    sprintf(buffer, "%d", nb_connections);
    
    mvaddstr(2, thread_no * 6 + 14, buffer);
    refresh();
}

void display_cleanup() {
    if (debug) return;
    
    endwin();
}