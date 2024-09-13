#include "tcp_server.h"
#include <sys/stat.h>  // umask 함수를 위한 헤더 파일 추가

#define TCP_PORT 5100
#define MAX_CLIENTS 10
#define BUFSIZE 2048

int client_sockets[MAX_CLIENTS];
int pipefd[MAX_CLIENTS][2];
int pipefd_back[MAX_CLIENTS][2];

void daemonize() {
    pid_t pid;

    // 첫 번째 fork
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE); // fork 실패
    if (pid > 0) exit(EXIT_SUCCESS); // 부모 프로세스 종료

    // 새 세션 생성
    if (setsid() < 0) exit(EXIT_FAILURE);

    // 두 번째 fork
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    // 작업 디렉토리 변경
    chdir("/");

    // 파일 모드 마스크 초기화
    umask(0);

    // 표준 파일 디스크립터를 /dev/null로 리다이렉트
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open("/dev/null", O_RDWR); // stdin
    dup(0); // stdout
    dup(0); // stderr
}

int main() {
    daemonize();

    int listen_sock, client_sock;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clen = sizeof(cliaddr);
    pid_t pid;

    // 클라이언트 소켓 초기화 및 파이프 설정
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = -1;
        if (pipe(pipefd[i]) == -1 || pipe(pipefd_back[i]) == -1) {
            perror("파이프 초기화 실패");
            exit(1);
        }
    }

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("socket()");
        exit(1);
    }

    // 주소 재사용 설정
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 서버 주소 설정 및 바인딩
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TCP_PORT);
    if (bind(listen_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind()");
        close(listen_sock);
        exit(1);
    }

    if (listen(listen_sock, MAX_CLIENTS) < 0) {
        perror("listen()");
        close(listen_sock);
        exit(1);
    }

    signal(SIGCHLD, sigchld_handler);

    while (1) {
        client_sock = accept(listen_sock, (struct sockaddr *)&cliaddr, &clen);
        if (client_sock < 0) {
            perror("accept()");
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
            printf("최대 클라이언트 수 도달.\n");
            close(client_sock);
        } else {
            pid = fork();
            if (pid == 0) {
                close(listen_sock);
                handle_client(client_sock, client_id, pipefd[client_id][0], pipefd_back[client_id][1]);
                exit(0);
            }
        }
    }

    close(listen_sock);
    return 0;
}
