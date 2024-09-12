all : P_server P_client

P_server : P_server.o login.o enter.o
	gcc -o P_server P_server.o login.o enter.o

P_client : P_client.o
	gcc -o P_client P_client.c

P_server.o : P_server.c P_server.h
	gcc -c P_server.c

login.o : login.c P_server.h
	gcc -c login.c

enter.o : enter.c P_server.h
	gcc -c enter.c

P_client.o : P_client.c
	gcc -c P_client.c