CC=g++
CFLAGS=-I. -std=gnu++11
DEPS = 
SAWAD_OBJ = sawad.o http_srv.o sawa_srv.o thread_pool.o display.o sawa_admin.o server.o util.o
SAWA_CLIENT_OBJ = sawa_client.o sawa_client_interface.o util.o
TEST_OBJ = test.o test_helper.o http_srv.o sawa_srv.o thread_pool.o display.o sawa_admin.o server.o util.o

all: sawad sawa-client test towa

#app_pool.o: app_pool.cpp
#	g++ -I /usr/lib/jvm/java-1.6.0-openjdk-amd64/include -std=gnu++11 -o $@ $^ $(CFLAGS) -L/usr/lib/jvm/java-1.6.0-openjdk-amd64/bin/java -L/usr/lib/jvm/java-1.6.0-openjdk-amd64/jre/lib/amd64/server/

%.o: %.cpp $(DEPS)
	$(CC) -c -g -o $@ $< $(CFLAGS)

sawa-client: $(SAWA_CLIENT_OBJ)
	g++ -pthread -std=gnu++11 -o $@ $^ $(CFLAGS)

sawad: $(SAWAD_OBJ)
	g++ -pthread -std=gnu++11 -o $@ $^ $(CFLAGS) -lncurses

test: $(TEST_OBJ)
	g++ -pthread -std=gnu++11 -o $@ $^ $(CFLAGS) -lncurses

towa: towa.cpp
	g++ -g -o towa -I /usr/lib/jvm/java-1.6.0-openjdk-amd64/include -std=gnu++11 towa.cpp -I. -std=gnu++11 -L/usr/lib/jvm/java-1.6.0-openjdk-amd64/bin/java -L/usr/lib/jvm/java-1.6.0-openjdk-amd64/jre/lib/amd64/server/ -ljvm

clean:
	rm *.o
	rm sawad
	rm sawa-client
	rm towa
	rm test
