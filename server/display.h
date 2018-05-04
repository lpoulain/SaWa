void select_ncurses_display();
void select_debug_display();
void select_daemon_display();

class ConnectionThread;

class Display {
public:
    virtual void init() = 0;
    virtual void new_thread(ConnectionThread *) = 0;
    virtual void thread_update(ConnectionThread *) = 0;
    virtual void cleanup() = 0;
    virtual void debug(const char *, ...) = 0;
    virtual void error(const char *, ...) = 0;
    virtual void refresh_thread(ConnectionThread *, int) = 0;
    virtual void status(ConnectionThread *, int)  = 0;
};

extern Display *screen;
