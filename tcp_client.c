#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define TCP_PORT 5100         // 사용할 서버의 포트 번호
#define MAX_NAME_LEN 128      // 최대 이름 길이
#define MAX_MSG_LEN 896       // 최대 메시지 길이

// 서버로부터 데이터를 수신하는 스레드 함수
void *receive_thread(void *socket) {
    int sock = *(int *)socket;  // 소켓 파일 디스크립터
    char buffer[1024];          // 수신 데이터를 저장할 버퍼

    while (1) {
        int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {   // 연결이 종료되거나 오류 발생 시
            printf("Connection closed or error occurred.\n");
            break;
        }
        buffer[received] = '\0';  // 문자열의 끝을 나타내는 널 문자 추가
        printf("%s", buffer);     // 수신된 메시지 출력
    }

    return NULL;  // 스레드 종료
}

int main(int argc, char **argv) {
    int sock;  // 소켓 파일 디스크립터
    struct sockaddr_in servaddr;  // 서버 주소 구조체
    char send_msg[MAX_MSG_LEN], recv_msg[1024];
    pthread_t recv_thread_id;  // 수신 스레드의 ID

    // IP 주소 입력이 제대로 되었는지 확인
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

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(TCP_PORT);
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    // 서버에 연결
    if (connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connection failed");
        close(sock);
        return -1;
    }
    printf("채팅방에 입장되셨습니다.\n");
    printf("이름 설정: ");
    fgets(send_msg, MAX_NAME_LEN, stdin);  // 이름 입력 받기
    send_msg[strcspn(send_msg, "\n")] = '\0';  // 줄바꿈 문자 제거

    // 서버에 이름 전송
    if (send(sock, send_msg, strlen(send_msg), 0) < 0) {
        perror("Failed to send name");
        close(sock);
        return -1;
    }
    printf("채팅방 입장이 완료되었습니다\n");

    // 수신 스레드 시작
    pthread_create(&recv_thread_id, NULL, receive_thread, &sock);

    // 서버로 메시지 전송
    while (fgets(send_msg, MAX_MSG_LEN, stdin) != NULL) {
        if (send(sock, send_msg, strlen(send_msg), 0) < 0) {
            perror("메세지 전송실패");
            break;
        }
    }

    // 자원 정리
    pthread_join(recv_thread_id, NULL);
    close(sock);
    return 0;
}
