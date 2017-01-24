

.PHONY: clean

all: server client

server : udpserver.cpp
	g++ -o server udpserver.cpp -lpthread

client : udpclient.cpp
	g++ -o client udpclient.cpp -lpthread

clean:
	rm -f *.o *.exe