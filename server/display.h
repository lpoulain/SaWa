void select_ncurses_display();
void select_debug_display();
void select_daemon_display();

struct connection_thread;

struct display {
    void (*init)();
    void (*new_thread)(struct connection_thread *);
    void (*update)(struct connection_thread *);
    void (*cleanup)();
    void (*debug)(char *, ...);
    void (*refresh)(struct connection_thread *, int);
    void (*status)(struct connection_thread *, int);
};

extern struct display screen;
