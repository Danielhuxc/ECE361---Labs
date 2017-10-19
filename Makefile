all: server deliver

server: server.c
	gcc -o server server.c
deliver: deliver.c
	gcc -o deliver deliver.c