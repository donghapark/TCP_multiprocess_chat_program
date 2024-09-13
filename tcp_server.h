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
#define MAX_NAME_LEN 128

extern int client_sockets[MAX_CLIENTS];
extern int pipefd[MAX_CLIENTS][2];
extern int pipefd_back[MAX_CLIENTS][2];

// 자식 프로세스 종료 시그널 핸들러
void sigchld_handler(int sig) {
    int client_id, status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (read(pipefd_back[i][0], &client_id, sizeof(client_id)) > 0) {
                client_sockets[client_id] = -1;
                printf("클라이언트 %d 연결 해제 및 배열에서 제거됨.\n", client_id);

                // 다른 자식 프로세스에 연결 해제 정보 전달
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (client_sockets[j] != -1) {
                        write(pipefd[j][1], &client_id, sizeof(client_id));
                    }
                }
            }
        }
    }
}

// 클라이언트 소켓 상태 출력
void print_client_sockets() {
    printf("현재 클라이언트 소켓: ");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        printf("%d ", client_sockets[i]);
    }
    printf("\n");
}

// 메시지를 모든 클라이언트에 브로드캐스트
void broadcast_message(int sender_sock, char *message, char *sender_name) {
    char full_message[BUFSIZE + MAX_NAME_LEN];
    snprintf(full_message, sizeof(full_message), "[%s]: %s", sender_name, message);  // 송신자 이름과 메시지 포맷 지정

    int active_clients = 0;
    printf("브로드캐스트: %s\n", full_message);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != -1 && client_sockets[i] != sender_sock) {// 본인 빼고 브로드케스트 
            active_clients++;
            if (send(client_sockets[i], full_message, strlen(full_message), 0) < 0) {
                perror("send()");
                close(client_sockets[i]);
                client_sockets[i] = -1;  // 전송 실패 시 클라이언트 제거
            }
        }
    }
    if (active_clients == 0) {
        printf("메시지를 받을 활성 클라이언트가 없습니다.\n");
    } else {
        printf("%d개의 클라이언트에게 메시지를 보냈습니다.\n", active_clients);
    }
    print_client_sockets();
}

// 클라이언트 처리 함수
void handle_client(int client_sock, int client_id, int pipe_read, int pipe_write_back) {
    char message[BUFSIZE];
    char client_name[BUFSIZE];
    int n;

    // 처음으로 클라이언트 이름 수신
    n = recv(client_sock, client_name, BUFSIZE, 0);
    if (n <= 0) {
        printf("클라이언트 이름을 받는 데 실패했습니다\n");
        close(client_sock);
        write(pipe_write_back, &client_id, sizeof(client_id));
        exit(0);
    }
    client_name[n] = '\0';  // 받은 이름을 널 문자로 종료
    printf("클라이언트 '%s'가 연결되었습니다.\n", client_name);

    while (1) {
        memset(message, 0, BUFSIZE);
        n = recv(client_sock, message, BUFSIZE, 0);
        if (n <= 0) {
            if (n == 0) {
                printf("클라이언트 '%s'가 연결을 종료했습니다.\n", client_name);
            } else {
                perror("recv()");
            }
            close(client_sock);
            write(pipe_write_back, &client_id, sizeof(client_id));
            exit(0);
        }
        message[n] = '\0';
        printf("[%s]: %s\n", client_name, message);
        broadcast_message(client_sock, message, client_name);  // 메시지와 함께 클라이언트 이름을 브로드캐스트 함수로 전달
    }
}
