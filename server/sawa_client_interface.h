#define SAWA_INFO  0x01
#define SAWA_READ  0x02
#define SAWA_WRITE 0x03
#define SAWA_STOP  0x04
#define SAWA_STAT  0x05

#define SAWA_MSG_OK     0x42
#define SAWA_MSG_ERR    0x43
#define SAWA_MSG_NOAUTH 0x44

class ThreadStat {
public:
    int nb_connections;
    int info[3];
    char active;
};

// Direct TCP/IP connection to the SaWa Server
class SawaInterface {
    int socket_fd;
    uint8_t *buffer_out;
    int total_size;
    
public:
    // Used to send a message to the server
    uint8_t *prepareCommand(uint8_t op, int payload_size);
    void sendCmdToServer();
    void sendSingleCmdToServer(uint8_t op);
    
    // Used to read a response from the server
    int readFromServer(uint8_t *buffer_in, int size);
    int readFromServer(uint8_t **buffer_in);
    
    SawaInterface(int port);
    ~SawaInterface();
};

// The high-level SaWa Client commands
class SawaClient {
    uint8_t *sendReadCmd(int offset, int payload_size, SawaInterface *sawa=nullptr);
    int sendWriteCmd(int offset, int payload_size, SawaInterface *sawa=nullptr);
    int sendWriteCmd(uint8_t *buffer_out, int offset, int payload_size, SawaInterface *sawa=nullptr);
    
public:
    int sendInfoCmd();
    void sendStopCmd();
    void sendStatCmd();
    static void *threadTest(void *arg);
    void testServer(int nb_threads);
    void sendCommand(uint8_t op, int offset, int size);

    SawaClient();
    ~SawaClient();
};
