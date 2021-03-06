#define _BSD_SOURCE

#include <curses.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include "sawa.h"
#include "display.h"
#include "thread_pool.h"
#include "server.h"

using namespace std;

Display *screen;

class DisplayDefault : public Display {
    string info_label[3];
    // In order to avoid display issues, only one thread can update the screen at a time
    // Hence the need of a mutex
    // This of course slows down the overall performance
    // But the primary goal of the default display mode is not performance
    // The -daemon or -quiet options are better suited for that
    pthread_mutex_t display_lock;
    
public:    
    void init() override {
        string buffer("Server started on port ");
        int i;

        buffer += to_string(server->port);

        initscr();
        cbreak();
        noecho();
        clear();

        mvaddstr(0, 0, buffer.c_str());
        mvaddstr(1, 0, "Thread# Active? #Conn");
        for (i=0; i<3; i++) {
            if (!info_label[i].empty()) mvaddstr(1, 24 + i*8, info_label[i].c_str());
        }

        refresh();
    }

    void refresh_thread(ConnectionThread *thread_info, int i) override {
        string buffer = to_string(thread_info->info[i]);

        if (info_label[i].empty()) return;

        pthread_mutex_lock(&display_lock);
            mvaddstr(thread_info->nb + 2, 24 + i*8, buffer.c_str());
            refresh();
        pthread_mutex_unlock(&display_lock);
    }

    // Displays the thread statistics
    void thread_update(ConnectionThread *thread_info) override {
        string buffer = to_string(thread_info->nb_connections);

        pthread_mutex_lock(&display_lock);
            mvaddstr(thread_info->nb + 2, 16, buffer.c_str());
            refresh();
        pthread_mutex_unlock(&display_lock);
    }

    void status(ConnectionThread *thread_info, int is_on) override {
        pthread_mutex_lock(&display_lock);
        if (is_on)
            mvaddstr(thread_info->nb + 2, 8, "Y");
        else
            mvaddstr(thread_info->nb + 2, 8, ".");
        refresh();
        pthread_mutex_unlock(&display_lock);
    }

    // Displays the thread number
    void new_thread(ConnectionThread *thread_info) override {
        string buffer = to_string(thread_info->nb);

        mvaddstr(thread_info->nb + 2, 0, buffer.c_str());

        this->thread_update(thread_info);
        this->status(thread_info, 1);
    }

    void cleanup() override {
        endwin();
    }

    // No-op
    void debug(const char *format, ...) override {

    }

    void error(const char *format, ...) override {
        char buffer[256];

        va_list argptr;
        va_start(argptr, format);
        vsprintf(buffer, format, argptr);
        va_end(argptr);

        pthread_mutex_lock(&display_lock);
        mvaddstr(6, 0, buffer);
        refresh();
        pthread_mutex_unlock(&display_lock);
    }
    
    DisplayDefault(bool http) {
        if (http) {
            info_label[0] = "200";
            info_label[1] = "404";
            info_label[2] = "500";
        }
        else {
            info_label[0] = "Info";
            info_label[1] = "Reads";
            info_label[2] = "Writes";
        }
        
        if (pthread_mutex_init(&display_lock, NULL) != 0)
        {
            throw FAILURE_MUTEX_INIT;
        }
    }
};

///////////////////////////////////////////////////////////////////

class DisplayDebug : public Display {
public:

    void init() override {
        cout << "Server started on port " << server->port << endl;
    }

    void new_thread(ConnectionThread *thread_info) override {
        cout << "Created thread " << thread_info->nb << endl;
    }

    void debug(const char *format, ...) override {
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

    void error(const char *format, ...) override {
        va_list argptr;
        va_start(argptr, format);
        vfprintf(stderr, format, argptr);
        va_end(argptr);
    }

    void cleanup() override { }
    void refresh_thread(ConnectionThread *thread_info, int i) override { }
    void thread_update(ConnectionThread *thread_info) override { }

    void status(ConnectionThread *thread_info, int is_on) override {
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

///////////////////////////////////////////////////////////////////

static FILE *fd_log;

class DisplayDaemon : public Display {
public:
    void init() override {
        fd_log = fopen("./sawa.log", "a+");
    }

    void new_thread(ConnectionThread *thread_info) override { }

    void debug(const char *format, ...) override {
        if (!debug_flag) return;

        va_list argptr;
        va_start(argptr, format);
        vfprintf(fd_log, format, argptr);
        va_end(argptr);

        fflush(fd_log);
    }

    void cleanup() override {
        fclose(fd_log);
    }

    void refresh_thread(ConnectionThread *thread_info, int i) override { }
    void thread_update(ConnectionThread *thread_info) override { }
    void status(ConnectionThread *thread_info, int is_on) override { }
    
    void error(const char *format, ...) {
        va_list argptr;
        va_start(argptr, format);
        vfprintf(fd_log, format, argptr);
        va_end(argptr);

        fflush(fd_log);
    }
};

//////////////////////////////////////////////////////////////////////////////////

class DisplayQuiet : public Display {
public:

    void init() override {
        cout << "Server started on port " << server->port << endl;
    }

    void error(const char *format, ...) override {
        va_list argptr;
        va_start(argptr, format);
        vfprintf(stderr, format, argptr);
        va_end(argptr);
    }

    void new_thread(ConnectionThread *thread_info) override { }
    void debug(const char *format, ...) override { }
    void cleanup() override { }
    void refresh_thread(ConnectionThread *thread_info, int i) override { }
    void thread_update(ConnectionThread *thread_info) override { }
    void status(ConnectionThread *thread_info, int is_on) override { }
};

//////////////////////////////////////////////////////////////////////////////////

void select_display(int display_code, bool http) {
    switch(display_code) {
        case DISPLAY_DEFAULT:
            screen = new DisplayDefault(http);
            break;
        case DISPLAY_DEBUG:
            screen = new DisplayDebug();
            break;
        case DISPLAY_DAEMON:
            screen = new DisplayDaemon();
            break;
        case DISPLAY_QUIET:
            screen = new DisplayQuiet();
            break;
        default:
            screen = new DisplayDefault(http);
    }
}
