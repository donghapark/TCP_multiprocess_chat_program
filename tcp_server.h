#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_CLIENTS 10
#define BUFSIZE 2048

extern int client_sockets[MAX_CLIENTS];
extern int pipefd[MAX_CLIENTS][2];
extern int pipefd_back[MAX_CLIENTS][2];

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