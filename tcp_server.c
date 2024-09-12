#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define TCP_PORT 5100
#define MAX_CLIENTS 10
#define BUFSIZE 2048

int client_sockets[MAX_CLIENTS];  // 클라이언트 소켓 배열
int pipefd[MAX_CLIENTS][2];       // 부모 -> 자식
int pipefd_back[MAX_CLIENTS][2];  // 자식 -> 부모

void sigchld_handler(int sig) {
    int client_id, status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (read(pipefd_back[i][0], &client_id, sizeof(client_id)) > 0) {
                client_sockets[client_id] = -1;
                printf("Client %d disconnected and cleared from array.\n", client_id);

                // Inform other child processes of the disconnection
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (client_sockets[j] != -1) {
                        write(pipefd[j][1], &client_id, sizeof(client_id));
                    }
                }
            }
        }
    }
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
                client_sockets[i] = -1;  // 연결이 끊긴 클라이언트 제거
            }
        }
    }
    if (active_clients == 0) {
        printf("No active clients to receive the message.\n");
    } else {
        printf("Message sent to %d clients.\n", active_clients);
    }
    print_client_sockets();
}

void handle_client(int client_sock, int client_id, int pipe_read, int pipe_write_back) {
    char message[BUFSIZE];
    char client_name[BUFSIZE];
    int n;

    n = recv(client_sock, client_name, BUFSIZE, 0);
    if (n <= 0) {
        printf("Failed to get client name\n");
        close(client_sock);
        write(pipe_write_back, &client_id, sizeof(client_id));
        exit(0);
    }
    client_name[n] = '\0';
    printf("Client '%s' connected.\n", client_name);

    while (1) {
        memset(message, 0, BUFSIZE);
        n = recv(client_sock, message, BUFSIZE, 0);
        if (n <= 0) {
            if (n == 0) {
                printf("Client '%s' disconnected.\n", client_name);
            } else {
                perror("recv() error");
            }
            close(client_sock);
            write(pipe_write_back, &client_id, sizeof(client_id));
            exit(0);
        }
        message[n] = '\0';
        printf("=============================================================\n");
        printf("Received from %s: %s\n", client_name, message);
        broadcast_message(client_sock, message);
        printf("=============================================================\n");
    }
}

int main() {
    int listen_sock, client_sock;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clen = sizeof(cliaddr);
    pid_t pid;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = -1;
        if (pipe(pipefd[i]) == -1 || pipe(pipefd_back[i]) == -1) {
            perror("Error initializing pipe");
            exit(1);
        }
    }

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
            pid = fork();
            if (pid == 0) {// 자식프로세스일때 
                close(listen_sock);
                handle_client(client_sock, client_id, pipefd[client_id][0], pipefd_back[client_id][1]);
                exit(0);
            }
        }
    }

    close(listen_sock);
    return 0;
}
