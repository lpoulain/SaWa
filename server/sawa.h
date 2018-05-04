#define SAWA_INFO  0x01
#define SAWA_READ  0x02
#define SAWA_WRITE 0x03
#define SAWA_STOP  0x04
#define SAWA_STAT  0x05

#define SAWA_MSG_OK     0x42
#define SAWA_MSG_ERR    0x43
#define SAWA_MSG_NOAUTH 0x44

class ConnectionThread;

extern void HTTP_init();
extern void HTTP_cleanup();
extern void sawa_init();
extern int server_port;
extern void ctrl_c_handler();

extern void (*op_listen) (ConnectionThread *);
extern int debug_flag;
