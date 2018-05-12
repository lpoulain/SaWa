CC=g++
CFLAGS=-I. -std=gnu++11
DEPS = 
SAWAD_OBJ = sawad.o http_srv.o sawa_srv.o thread_pool.o display.o sawa_admin.o server.o util.o
SAWA_CLIENT_OBJ = sawa_client.o sawa_client_interface.o
TEST_OBJ = test.o test_helper.o http_srv.o sawa_srv.o thread_pool.o display.o sawa_admin.o server.o util.o

%.o: %.cpp $(DEPS)
	$(CC) -c -g -o $@ $< $(CFLAGS)

all: sawad sawa-client test

sawa-client: $(SAWA_CLIENT_OBJ)
	g++ -pthread -o $@ $^ $(CFLAGS)

sawad: $(SAWAD_OBJ)
	g++ -pthread -std=gnu++11 -o $@ $^ $(CFLAGS) -lncurses

test: $(TEST_OBJ)
	g++ -pthread -std=gnu++11 -o $@ $^ $(CFLAGS) -lncurses

clean:
	rm *.o
	rm sawad
	rm sawa-client
	rm test
