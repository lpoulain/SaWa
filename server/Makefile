CC=g++
CFLAGS=-I. -std=gnu++11
DEPS = 
SAWAD_OBJ = sawad.o http_srv.o sawa_srv.o thread_pool.o display.o sawa_admin.o server.o util.o
SAWA_CLIENT_OBJ = sawa_client.o

%.o: %.c $(DEPS)
	g++ -c -g -o $@ $< $(CFLAGS)

%.o: %.cpp $(DEPS)
	$(CC) -c -g -o $@ $< $(CFLAGS)

all: sawad sawa-client

sawa_client.o: sawa_client.c
	gcc -c -g -o sawa_client.o sawa_client.c

sawa-client: $(SAWA_CLIENT_OBJ)
	gcc -pthread -o $@ $^ $(CFLAGS)

sawad: $(SAWAD_OBJ)
	g++ -pthread -std=gnu++11 -o $@ $^ $(CFLAGS) -lncurses

clean:
	rm *.o
	rm sawad
	rm sawa-client
