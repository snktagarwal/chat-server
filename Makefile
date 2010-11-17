all: sever client
server: server.c serv-client.h
	gcc -g -o server server.c -lpthread
client: client.c serv-client.h
	gcc -g -o client client.c -lpthread

