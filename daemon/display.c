#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sawa.h"
#include "display.h"

struct display screen;

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

void display_thread_update(int thread_no, int nb_connections) {
    if (debug) return;
    
    char buffer[6];
    memset(buffer, 6, 0);
    sprintf(buffer, "%d", nb_connections);
    
    mvaddstr(2, thread_no * 6 + 14, buffer);
    refresh();
}

void display_new_thread(int thread_no) {
    if (debug) return;
    
    char buffer[6];
    memset(buffer, 6, 0);
    sprintf(buffer, "%d", thread_no);
    
    mvaddstr(1, thread_no * 6 + 14, buffer);
    
    display_thread_update(thread_no, 1);
}

void display_cleanup() {
    if (debug) return;
    
    endwin();
}

void select_ncurses_display() {
    screen.init = display_init;
    screen.new_thread = display_new_thread;
    screen.update = display_thread_update;
    screen.cleanup = display_cleanup;
}

///////////////////////////////////////////////////////////////////

void debug_init() {
    printf("Server started on port %d\n", server_port);
}

void debug_new_thread(int thread_no) {
    printf("Created thread %d\n", thread_no);    
}

void debug_thread_update(int thread_no, int nb_connections) {
    
}

void debug_cleanup() {
  
}

void select_debug_display() {
    screen.init = debug_init;
    screen.new_thread = debug_new_thread;
    screen.update = debug_thread_update;
    screen.cleanup = debug_cleanup;
}
