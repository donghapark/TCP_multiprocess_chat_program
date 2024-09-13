#include "tcp_server.h"

#define TCP_PORT 5100          // 서버가 사용할 포트 번호
#define MAX_CLIENTS 10          // 최대 클라이언트 수
#define BUFSIZE 2048            // 버퍼 크기

int client_sockets[MAX_CLIENTS];  // 클라이언트 소켓을 저장할 배열
int pipefd[MAX_CLIENTS][2];       // 부모 프로세스에서 자식 프로세스로 데이터를 보내는 파이프
int pipefd_back[MAX_CLIENTS][2];  // 자식 프로세스에서 부모 프로세스로 데이터를 보내는 파이프

int main() {
    int listen_sock, client_sock;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clen = sizeof(cliaddr);
    pid_t pid;

    // 모든 클라이언트 소켓을 -1로 초기화 (사용되지 않는 상태)
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = -1;
        if (pipe(pipefd[i]) == -1 || pipe(pipefd_back[i]) == -1) {
            perror("파이프 초기화 실패");
            exit(1);
        }
    }

    // 서버 소켓 생성
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("socket()");
        exit(1);
    }

    // 주소 재사용 옵션 설정
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 서버 주소 구조체 설정
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TCP_PORT);

    // 서버 소켓 바인딩
    if (bind(listen_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind()");
        close(listen_sock);
        exit(1);
    }

    // 클라이언트의 연결 요청을 기다림
    if (listen(listen_sock, MAX_CLIENTS) < 0) {
        perror("listen()");
        close(listen_sock);
        exit(1);
    }

    printf("서버 포트: %d  실행 중...\n", TCP_PORT);

    // 자식 프로세스 종료 시그널 처리
    signal(SIGCHLD, sigchld_handler);

    // 클라이언트 연결 처리 루프
    while (1) {
        client_sock = accept(listen_sock, (struct sockaddr *)&cliaddr, &clen);
        if (client_sock < 0) {
            perror("accept()");
            continue;
        }

        // 사용 가능한 슬롯 찾기
        int client_id = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == -1) {
                client_sockets[i] = client_sock;
                client_id = i;
                break;
            }
        }

        // 모든 슬롯이 찼는지 확인
        if (client_id == -1) {
            printf("최대 클라이언트 수에 도달하여 연결 거부됨.\n");
            close(client_sock);
        } else {
            // 클라이언트 처리를 위해 자식 프로세스 생성
            pid = fork();
            if (pid == 0) { // 자식 프로세스에서
                close(listen_sock);
                handle_client(client_sock, client_id, pipefd[client_id][0], pipefd_back[client_id][1]);
                exit(0);
            }
        }
    }

    // 서버 소켓 닫기
    close(listen_sock);
    return 0;
}
