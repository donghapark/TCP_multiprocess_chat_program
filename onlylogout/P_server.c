#include "P_server.h"

// 가용 인원 최소 50 - 무제한 시 가산점

#define TCP_PORT 5181

int main()
{
    // 기본 데이터
    int ssock, csock, pfd[2];
    socklen_t clen;
    struct sockaddr_in servaddr, cliaddr;

    // 서버 정보
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(TCP_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 클라 정보
    int c_count = 0; // 유저 수
    int roomn = 0; // 서버에 생성된 방
    user* users[100]; // 유저 구조체
    user* newUser; // 유저 포인터

    // 유저 수 공유하기 위해 공유 메모리 대신 파일 사용
    int fd = open("user_count.txt", O_RDWR | O_TRUNC | O_CREAT, S_IRWXU);
    char dum_count[2] = "0\0";
    if (write(fd, dum_count, strlen(dum_count)) == -1)
    {
        perror("file write()");
        return -1;
    }
    close(fd);

    // 서버 소켓 생성
    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if (ssock == -1)
    {
        perror("socket()");
        return -1;
    }

    // 포트 중복 문제 해결
    int on = 1;
    if (setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        perror("setsockopt()");
        return -1;
    }

    // 서버 nonblocking
    int flag = fcntl(ssock, F_GETFL, 0);
    fcntl(ssock, F_SETFL, flag | O_NONBLOCK);

    if (bind(ssock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind()");
        return -1;
    }

    if (listen(ssock, 50) < 0)
    {
        perror("listen()");
        return -1;
    }

    clen = sizeof(cliaddr);
    char buf[BUFSIZ];
    pid_t pid = -1;

    if (pipe(pfd) < 0)
    {
        perror("pipe()");
        return -1;
    }

    while (1)
    {   
        // nonblocking으로 구현하면 부모도 비동기적으로 작동 가능
        //sleep(3);
        if (pid != 0)
        {
            csock = accept(ssock, (struct sockaddr*)&cliaddr, &clen);
            // char temp[100];
            // sprintf(temp, "%d", csock);
            // write(1, temp, strlen(temp));
            if (csock != -1)
            {
                pid = fork();
            }
        }

        if (pid == 0)
        {
            char cbuf[10];
            char pbuf[10];
            // sprintf(cbuf, "%d\n", csock);
            // write(1, cbuf, sizeof(cbuf));
            // 뮤텍스
            // int c_count = 0;
            // fd = open("user_count.txt", O_RDONLY , S_IRWXU);
            // char temp[2];
            // int n = read(fd, temp, sizeof(temp));
            // c_count = atoi(temp);
            // close(fd);
            // // 뮤텍스

            newUser = (void*)malloc(sizeof(user));
            newUser->fd = csock;
            newUser->count = c_count;
            newUser->rn = 0;

            // // 뮤텍스
            // fd = open("user_count.txt", O_WRONLY | O_TRUNC, S_IRWXU);
            // sprintf(temp, "%d", c_count);
            // write(fd, temp, strlen(temp));
            // close(fd);
            // // 뮤텍스

            // 로그인
            login(csock, newUser);

            pid = fork();
            if (pid == 0)
            {
                char format[100];
                memset(format, 0, sizeof(format));
                sprintf(format, "%d:%d:%d:%s:%s", -1, newUser->fd, newUser->rn, newUser->name, newUser->name);
                write(pfd[1], format, sizeof(format));
                sleep(3);
                _exit(0);
            }

            sleep(1);
            // 방 입장 및 채팅 시작
            enter(csock, newUser, pfd[1]);
            perror("out of enter");
            free(newUser);
            //_exit(0);
        }

        // 전체에게 전송
        if (pid != 0)
        {
            // char temp[100];
            // strcpy(temp, "send\n");
            // write(1, temp, strlen(temp));
            int flag = fcntl(ssock, F_GETFL, 0);
            fcntl(pfd[0], F_SETFL, flag | O_NONBLOCK);

            // 유저의 디스크립터, 방 번호, 이름을 추출
            int c_fd, c_rn, c_flag;
            char b_flag[2];
            char b_fd[3], b_rn[3];
            char c_name[100];
            char mesg[100];
            char format[BUFSIZ];

            memset(c_name, 0, 100);
            memset(mesg, 0, BUFSIZ);
            memset(format, 0, BUFSIZ);

            int n = read(pfd[0], format, BUFSIZ);

            // char temp[100];
            // sprintf(temp, "%d", n);
            // write(1, temp, strlen(temp));
            if (n > 0)
            {   
                strcpy(b_flag, strtok(format, ":"));
                strcpy(b_fd, strtok(NULL, ":"));
                strcpy(b_rn, strtok(NULL, ":"));
                strcpy(c_name, strtok(NULL, ":"));
                strcpy(mesg, strtok(NULL, ":"));
                
                c_flag = atoi(b_flag);
                // 채팅
                if (c_flag >= 0)
                {
                    write(1, mesg, strlen(mesg));
                    for (int i = 0; i < c_count; i++)
                    {
                        // 자기 자신 제외하고 전달
                        if (atoi(b_fd) != users[i]->fd)
                        {
                            char sendMesg[1000];
                            memset(sendMesg, 0, sizeof(sendMesg));
                            sprintf(sendMesg, "[%s]: %s", c_name, mesg);
                            write(users[i]->fd, sendMesg, strlen(sendMesg));
                        }
                            
                    }
                }
                // 유저 배열에 저장
                else
                {
                    users[c_count] = (void*)malloc(sizeof(user));
                    users[c_count]->count = c_count;
                    users[c_count]->fd = atoi(b_fd);
                    users[c_count]->rn = atoi(b_rn);
                    strcpy(users[c_count]->name, c_name);

                    char temp[3];
                    sprintf(temp, "%d", c_count);

                    c_count++;
                }
            }
        }
    }
    
    perror("close");
    for (int i = 0; i < c_count; i++)
    {
        free(users[i]);
    }
    close(fd);
    close(pfd[0]);
    close(pfd[1]);
    close(csock);
    close(ssock);

    return 0;

}