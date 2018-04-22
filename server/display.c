#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "sawa.h"
#include "display.h"
#include "thread_pool.h"

struct display screen;

static char *info_label[3];

void display_init() {
    char buffer[32];
    int i;
    
    sprintf(buffer, "Server started on port %d\n", server_port);
    
    initscr();
    cbreak();
    noecho();
    clear();
    
    mvaddstr(0, 0, buffer);
    mvaddstr(1, 0, "Thread #   :");
    mvaddstr(2, 0, "Active?    :");
    mvaddstr(3, 0, "Connections:");
    for (i=0; i<3; i++) {
        if (info_label[i] != NULL) mvaddstr(4+i, 0, info_label[i]);
    }
    
    refresh();
}

void display_refresh(struct connection_thread *thread_info, int i) {
    char buffer[6];
        
    if (info_label[i] == NULL) return;
        
    memset(buffer, 6, 0);
    sprintf(buffer, "%d", thread_info->info[i]);
    mvaddstr(4+i, thread_info->nb * 6 + 14, buffer);

    refresh();
}

// Displays the thread statistics
void display_thread_update(struct connection_thread *thread_info) {
    char buffer[6];
    int i;
        
    memset(buffer, 6, 0);
    sprintf(buffer, "%d", thread_info->nb_connections);
    mvaddstr(3, thread_info->nb * 6 + 14, buffer);

    refresh();
}

void display_status(struct connection_thread *thread_info, int is_on) {
    if (is_on)
        mvaddstr(2, thread_info->nb * 6 + 14, "Y");
    else
        mvaddstr(2, thread_info->nb * 6 + 14, ".");
    refresh();
}

// Displays the thread number
void display_new_thread(struct connection_thread *thread_info) {
    char buffer[6];
    memset(buffer, 6, 0);
    sprintf(buffer, "%d", thread_info->nb);
    
    mvaddstr(1, thread_info->nb * 6 + 14, buffer);
    
    display_thread_update(thread_info);
    display_status(thread_info, 1);
}

void display_cleanup() {
    endwin();
}

// No-op
void display_debug(char *msg, ...) {
    
}

void select_ncurses_display() {
    screen.init = display_init;
    screen.new_thread = display_new_thread;
    screen.update = display_thread_update;
    screen.cleanup = display_cleanup;
    screen.debug = display_debug;
    screen.refresh = display_refresh;
    screen.status = display_status;
    
    info_label[0] = "Info       :";
    info_label[1] = "Reads      :";
    info_label[2] = "Writes     :";
}

///////////////////////////////////////////////////////////////////

void debug_init() {
    printf("Server started on port %d\n", server_port);
}

void debug_new_thread(struct connection_thread *thread_info) {
    printf("Created thread %d\n", thread_info->nb);    
}

void debug_print(char *format, ...) {
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
}

void debug_cleanup() { }
void debug_refresh() { }
void debug_thread_update(struct connection_thread *thread_info) { }

void debug_status(struct connection_thread *thread_info, int is_on) {
    if (is_on)
        printf("Resumed thread %d\n", thread_info->nb);
    else
        printf("Release thread %d\n", thread_info->nb);
}

void select_debug_display() {
    screen.init = debug_init;
    screen.new_thread = debug_new_thread;
    screen.update = debug_thread_update;
    screen.cleanup = debug_cleanup;
    screen.debug = debug_print;
    screen.refresh = debug_refresh;
    screen.status = debug_status;
}

///////////////////////////////////////////////////////////////////

void daemon_init() { }
void daemon_new_thread(struct connection_thread *thread_info) { }
void daemon_print(char *format, ...) { }
void daemon_cleanup() { }
void daemon_refresh() { }
void daemon_thread_update(struct connection_thread *thread_info) { }
void daemon_status(struct connection_thread *thread_info, int is_on) { }

void select_daemon_display() {
    screen.init = daemon_init;
    screen.new_thread = daemon_new_thread;
    screen.update = daemon_thread_update;
    screen.cleanup = daemon_cleanup;
    screen.debug = daemon_print;
    screen.refresh = daemon_refresh;
    screen.status = daemon_status;
}
