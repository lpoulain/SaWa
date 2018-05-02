void select_ncurses_display();
void select_debug_display();
void select_daemon_display();

struct connection_thread;

class Display {
public:
    virtual void init() = 0;
    virtual void new_thread(struct connection_thread *) = 0;
    virtual void thread_update(struct connection_thread *) = 0;
    virtual void cleanup() = 0;
    virtual void debug(const char *, ...) = 0;
    virtual void error(const char *, ...) = 0;
    virtual void refresh_thread(struct connection_thread *, int) = 0;
    virtual void status(struct connection_thread *, int)  = 0;
/*    void (*init)();
    void (*new_thread)(struct connection_thread *);
    void (*update)(struct connection_thread *);
    void (*cleanup)();
    void (*debug)(const char *, ...);
    void (*error)(const char *, ...);
    void (*refresh)(struct connection_thread *, int);
    void (*status)(struct connection_thread *, int);*/
};

extern Display *screen;
