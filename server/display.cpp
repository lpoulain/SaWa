#define _BSD_SOURCE

#include <curses.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include "sawa.h"
#include "display.h"
#include "thread_pool.h"

using namespace std;

Display *screen;

static const char *info_label[3];

class DisplayDefault : public Display {
public:    
    void init() {
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

    void refresh_thread(struct connection_thread *thread_info, int i) {
        char buffer[6];

        if (info_label[i] == NULL) return;

        memset(buffer, 6, 0);
        sprintf(buffer, "%d", thread_info->info[i]);
        mvaddstr(4+i, thread_info->nb * 6 + 14, buffer);

        refresh();
    }

    // Displays the thread statistics
    void thread_update(struct connection_thread *thread_info) {
        char buffer[6];
        int i;

        memset(buffer, 6, 0);
        sprintf(buffer, "%d", thread_info->nb_connections);
        mvaddstr(3, thread_info->nb * 6 + 14, buffer);

        refresh();
    }

    void status(struct connection_thread *thread_info, int is_on) {
        if (is_on)
            mvaddstr(2, thread_info->nb * 6 + 14, "Y");
        else
            mvaddstr(2, thread_info->nb * 6 + 14, ".");
        refresh();
    }

    // Displays the thread number
    void new_thread(struct connection_thread *thread_info) {
        char buffer[6];
        memset(buffer, 6, 0);
        sprintf(buffer, "%d", thread_info->nb);

        mvaddstr(1, thread_info->nb * 6 + 14, buffer);

        this->thread_update(thread_info);
        this->status(thread_info, 1);
    }

    void cleanup() {
        endwin();
    }

    // No-op
    void debug(const char *format, ...) {

    }

    void error(const char *format, ...) {
        char buffer[256];

        va_list argptr;
        va_start(argptr, format);
        vsprintf(buffer, format, argptr);
        va_end(argptr);

        mvaddstr(6, 0, buffer);
    }
    
    DisplayDefault() {
        info_label[0] = "Info       :";
        info_label[1] = "Reads      :";
        info_label[2] = "Writes     :";
    }
};

void select_ncurses_display() {
    screen = new DisplayDefault();
}

///////////////////////////////////////////////////////////////////

class DisplayDebug : public Display {
public:

    void init() {
        cout << "Server started on port " << server_port << endl;
    }

    void new_thread(struct connection_thread *thread_info) {
        cout << "Created thread " << thread_info->nb << endl;
    }

    void debug(const char *format, ...) {
        struct timeval tv;
        int sec, msec;
        gettimeofday(&tv, NULL);
        sec = tv.tv_sec;
        msec = tv.tv_usec / 1000;
        cout << "[" << msec << "] ";

        va_list argptr;
        va_start(argptr, format);
        vfprintf(stdout, format, argptr);
        va_end(argptr);
    }

    void error(const char *format, ...) {
        va_list argptr;
        va_start(argptr, format);
        vfprintf(stderr, format, argptr);
        va_end(argptr);
    }

    void cleanup() { }
    void refresh_thread(struct connection_thread *thread_info, int i) { }
    void thread_update(struct connection_thread *thread_info) { }

    void status(struct connection_thread *thread_info, int is_on) {
        struct timeval tv;
        int sec, msec;
        gettimeofday(&tv, NULL);
        sec = tv.tv_sec;
        msec = tv.tv_usec / 1000;

        if (is_on)
            cout << "[" << msec << "] Resumed thread " << thread_info->nb << std::endl;
        else
            cout << "[" << msec << "] Release thread " << thread_info->nb << std::endl;
    }
};

void select_debug_display() {
    screen = new DisplayDebug();
}

///////////////////////////////////////////////////////////////////

static FILE *fd_log;

class DisplayDaemon : public Display {
public:
    void init() {
        fd_log = fopen("./sawa.log", "a+");
    }

    void new_thread(struct connection_thread *thread_info) { }

    void debug(const char *format, ...) {
        if (!debug_flag) return;

        va_list argptr;
        va_start(argptr, format);
        vfprintf(fd_log, format, argptr);
        va_end(argptr);

        fflush(fd_log);
    }

    void cleanup() {
        fclose(fd_log);
    }

    void refresh_thread(struct connection_thread *thread_info, int i) { }
    void thread_update(struct connection_thread *thread_info) { }
    void status(struct connection_thread *thread_info, int is_on) { }
    
    void error(const char *format, ...) {
        va_list argptr;
        va_start(argptr, format);
        vfprintf(fd_log, format, argptr);
        va_end(argptr);

        fflush(fd_log);
    }
};

void select_daemon_display() {
    screen = new DisplayDaemon();
}
