#define DISPLAY_DEFAULT 0x01
#define DISPLAY_DEBUG   0x02
#define DISPLAY_DAEMON  0x03
#define DISPLAY_QUIET   0x04

void select_display(int);

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
