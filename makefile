CC = gcc
#CC = aarch64-linux-gnu-gcc-12

TARGETS = tcp_server tcp_client

TCP_SERVER_SRC = tcp_server.c
TCP_CLIENT_SRC = tcp_client.c

TCP_SERVER_OBJ = $(TCP_SERVER_SRC:.c=.o)
TCP_CLIENT_OBJ = $(TCP_CLIENT_SRC:.c=.o)

all: $(TARGETS)

tcp_server: $(TCP_SERVER_OBJ)
	$(CC) -o $@ $(TCP_SERVER_OBJ)

tcp_client: $(TCP_CLIENT_OBJ)
	$(CC) -o $@ $(TCP_CLIENT_OBJ)

%.o: %.c %.h
	$(CC) -c $< -o $@

clean:
	rm -f $(TARGETS) *.o
