CC=gcc
CFLAGS=-I.
DEPS = 
SAWAD_OBJ = sawad.o http_srv.o sawa_srv.o thread_pool.o display.o sawa_admin.o
SAWA_CLIENT_OBJ = sawa_client.o

%.o: %.c $(DEPS)
	$(CC) -c -g -o $@ $< $(CFLAGS)

all: sawad sawa-client

sawa-client: $(SAWA_CLIENT_OBJ)
	gcc -pthread -o $@ $^ $(CFLAGS)

sawad: $(SAWAD_OBJ)
	gcc -pthread -o $@ $^ $(CFLAGS) -lncurses

clean:
	rm *.o
	rm sawad
	rm sawa-client
