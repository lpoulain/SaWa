class AdminInterface {
    int socket_desc;
    pthread_t *thread;
    
    void statCommand(int);
    void processCommand(int, unsigned char *, int);
    void readCommand(int);
    static void *connectionHandler(void *);
    
public:
    AdminInterface();
    ~AdminInterface();
};

extern AdminInterface *admin;
