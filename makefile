all:server client
server: server.o
	gcc -Wall -o server server.o
client: client.o
	gcc -Wall -o client client.o
clean:
	rm -f *.o
	rm -f server client
