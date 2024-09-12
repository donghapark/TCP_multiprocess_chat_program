#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#define TCP_PORT 5100
#define MAX_NAME_LEN 128
#define MAX_MSG_LEN 896

// 수신 데이터 처리 함수
void receive_data(int sock) {
    char buffer[1024];
    while (1) {
        int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 데이터가 없으므로 잠시 대기 후 다시 시도
                usleep(100000); // 0.1초 대기
                continue;
            } else {
                perror("recv error");
                break;
            }
        } else if (received == 0) {
            printf("Connection closed.\n");
            break;
        }

        buffer[received] = '\0';
        printf("Received: %s\n", buffer);
    }
}

int main(int argc, char **argv) {
    int sock;
    struct sockaddr_in servaddr;
    char send_msg[MAX_MSG_LEN];
    pid_t pid;

    if (argc < 2) {
        printf("Usage: %s IP_ADDRESS\n", argv[0]);
        return -1;
    }

    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // 서버 주소 설정
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(TCP_PORT);
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    // 서버 연결
    if (connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connection failed");
        close(sock);
        return -1;
    }

    // 논블록 소켓 설정
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    // 이름 입력
    printf("Enter your name: ");
    fgets(send_msg, MAX_NAME_LEN, stdin);
    send_msg[strcspn(send_msg, "\n")] = '\0'; // 줄바꿈 제거

    // 이름 서버로 전송
    if (send(sock, send_msg, strlen(send_msg), 0) < 0) {
        perror("Failed to send name");
        close(sock);
        return -1;
    }

    pid = fork();
    if (pid == 0) {
        // 자식 프로세스: 서버로부터 데이터 수신
        receive_data(sock);
        _exit(0);
    } else {
        // 부모 프로세스: 사용자로부터 메시지 입력 받아 서버로 전송
        while (fgets(send_msg, MAX_MSG_LEN, stdin) != NULL) {
            if (send(sock, send_msg, strlen(send_msg), 0) < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    usleep(100000); // 0.1초 대기
                } else {
                    perror("Non-blocking send");
                    break;
                }
            }
        }
    }

    // 자원 정리
    if (pid != 0) {
        int status;
        waitpid(pid, &status, 0);
        close(sock);
    }

    return 0;
}
