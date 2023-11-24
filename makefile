all: client server

client: client.o
	gcc client.o -o client
client.o: client.c
	gcc -c client.c 
server: server.o 
	gcc server.o -o server 
server.o: server.c 
	gcc -c server.c 
clean:
	rm -f client.o client server.o server