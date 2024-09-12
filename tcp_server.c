#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#define TCP_PORT 5100
#define MAX_CLIENTS 10
#define BUFSIZ 2048

int client_sockets[MAX_CLIENTS];  // 클라이언트 소켓 배열

void sigchld_handler(int sig) {
    // 자식 프로세스 종료 처리
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void print_client_sockets() {
    printf("Current client sockets: ");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        printf("%d ", client_sockets[i]);
    }
    printf("\n");
}

void broadcast_message(int sender_sock, char *message) {
    print_client_sockets();
    int active_clients = 0;
    printf("Broadcasting message: %s\n", message);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != -1 && client_sockets[i] != sender_sock) {
            active_clients++;
            if (send(client_sockets[i], message, strlen(message), 0) < 0) {
                perror("send() error");
                close(client_sockets[i]);
                client_sockets[i] = -1;
            }
        }
    }
    if (active_clients == 0) {
        printf("No active clients to receive the message.\n");
    } else {
        printf("Message sent to %d clients.\n", active_clients);
    }
}

void handle_client(int client_sock) {
    char message[BUFSIZ];
    char client_name[BUFSIZ];
    int n;

    if ((n = recv(client_sock, client_name, BUFSIZ, 0)) <= 0) {
        perror("Failed to receive client name");
        close(client_sock);
        return;
    }
    client_name[n] = '\0';
    printf("Client '%s' connected.\n", client_name);

    while (1) {
        if ((n = recv(client_sock, message, BUFSIZ, 0)) <= 0) {
            if (n == 0) {
                printf("Client '%s' disconnected.\n", client_name);
            } else {
                perror("recv() error");
            }
            break;
        }
        message[n] = '\0';
        printf("Received from %s: %s\n", client_name, message);
        broadcast_message(client_sock, message);
    }
    close(client_sock);
}

int main() {
    int listen_sock, client_sock;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clen = sizeof(cliaddr);

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("socket() error");
        exit(1);
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TCP_PORT);

    if (bind(listen_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind() error");
        close(listen_sock);
        exit(1);
    }

    if (listen(listen_sock, MAX_CLIENTS) < 0) {
        perror("listen() error");
        close(listen_sock);
        exit(1);
    }

    printf("Server is running on port %d...\n", TCP_PORT);

    signal(SIGCHLD, sigchld_handler);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = -1; // Initialize client sockets array
    }

    while (1) {
        client_sock = accept(listen_sock, (struct sockaddr *)&cliaddr, &clen);
        if (client_sock < 0) {
            perror("accept() error");
            continue;
        }

        int client_id = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == -1) {
                client_sockets[i] = client_sock;
                client_id = i;
                break;
            }
        }

        if (client_id == -1) {
            printf("Maximum clients connected. Connection refused.\n");
            close(client_sock);
        } else {
            print_client_sockets(); // Print the status of client sockets
            if (fork() == 0) { // Child process
                close(listen_sock);
                handle_client(client_sock);
                exit(0);
            }
        }
    }

    close(listen_sock);
    return 0;
}
