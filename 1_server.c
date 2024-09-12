#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>

#define TCP_PORT 5181
#define MAX_CLIENTS 100

int main()
{
    int ssock, csock, csocks[MAX_CLIENTS];
    socklen_t clen;
    struct sockaddr_in servaddr, cliaddr;
    int c_count = 0; // 유저 수

    // 서버 소켓 생성
    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if (ssock == -1) {
        perror("socket()");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(TCP_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 소켓 재사용 설정
    int on = 1;
    if (setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        perror("setsockopt()");
        return -1;
    }

    // 소켓 바인드
    if (bind(ssock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind()");
        return -1;
    }

    // 소켓 청취
    if (listen(ssock, 50) < 0) {
        perror("listen()");
        return -1;
    }

    clen = sizeof(cliaddr);

    while (1) {
        if (c_count < MAX_CLIENTS) {
            csock = accept(ssock, (struct sockaddr*)&cliaddr, &clen);
            if (csock != -1) {
                csocks[c_count++] = csock;
                // 메시지 수신 및 전달을 처리하는 자식 프로세스 생성
                if (fork() == 0) {
                    char buf[BUFSIZ];
                    int read_len;

                    while ((read_len = read(csock, buf, BUFSIZ)) > 0) {
                        // 클라이언트로부터 메시지를 받고 모든 클라이언트에게 브로드캐스트
                        for (int i = 0; i < c_count; i++) {
                            if (csocks[i] != csock) { // 메시지를 보낸 클라이언트를 제외하고 메시지 전송
                                write(csocks[i], buf, read_len);
                            }
                        }
                    }

                    // 클라이언트 연결 종료 시 처리
                    close(csock);
                    _exit(0);
                }
            }
        }
    }

    // 서버 종료 시 자원 해제
    for (int i = 0; i < c_count; i++) {
        close(csocks[i]);
    }
    close(ssock);

    return 0;
}
