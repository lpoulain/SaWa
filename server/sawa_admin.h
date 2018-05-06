class AdminInterface {
    int socket_desc;
    pthread_t *thread;
    
    void statCommand(int);
    void processCommand(int, uint8_t *, int);
    void readCommand(int);
    static void *connectionHandler(void *);
    
public:
    AdminInterface();
    ~AdminInterface();
};

extern AdminInterface *admin;
