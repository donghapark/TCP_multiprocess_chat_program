#include "tcp_server.h"

#define TCP_PORT 5100
#define MAX_CLIENTS 10
#define BUFSIZE 2048

int client_sockets[MAX_CLIENTS];  // 클라이언트 소켓 배열
int pipefd[MAX_CLIENTS][2];       // 부모 -> 자식
int pipefd_back[MAX_CLIENTS][2];  // 자식 -> 부모

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