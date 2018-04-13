void select_ncurses_display();
void select_debug_display();

struct display {
    void (*init)();
    void (*new_thread)(int thread_no);
    void (*update)(int thread_no, int nb_connections);
    void (*cleanup)();
};

extern struct display screen;
