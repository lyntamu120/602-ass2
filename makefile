all:server client
server: server.o
	gcc -Wall -o server server.o -std=gnu99
client: client.o
	gcc -Wall -o client client.o -std=gnu99
clean:
	rm -f *.o
	rm -f server client
