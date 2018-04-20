#define SAWA_INFO  0x01
#define SAWA_READ  0x02
#define SAWA_WRITE 0x03

struct connection_thread;

extern void HTTP_init();
extern void HTTP_cleanup();
extern void sawa_init();
extern int server_port;

extern void (*op_listen) (struct connection_thread *);
extern int debug;
