#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define TCP_PORT 5181

int clogin(int s_num);
int cEnter(int s_num);
void waitS();

int main(int argc, char** argv)
{
    int ssock;
    struct sockaddr_in servaddr;

    // 입력 할 문자
    char buf[BUFSIZ];
    ssock = socket(AF_INET, SOCK_STREAM, 0);

    // 서버 정보
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(TCP_PORT);
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr.s_addr);

    if (connect(ssock, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
    {
        perror("connect()");
        return -1;
    }

    char select[2] = {'0', '\0'}; // 1: 로그인 창, 2. 채팅
    while (1)
    {   
        waitS();
        read(ssock, select, 2);
        //write(1, select, strlen(select));
        if (!strcmp(select, "1"))
        {
            if (clogin(ssock))
                break;
        }
        else if (!strcmp(select, "2"))
        {
            if (cEnter(ssock))
                break;
        }
        else
        {
            // printf("pass\n");
            continue;
        }
    }

    close(ssock);
    return 0;
}

int clogin(int s_num)
{
    char print[BUFSIZ];
    char buf[BUFSIZ];
    int flag = 0;

    while (1)
    {
        // 로그인에 대한 프로세스.
        // 아이디 존재하면 break.
        // 아이디 확인이 안되면 무한 반복.
        memset(print, 0, BUFSIZ);
        strcpy(print, "아이디를 입력해주세요: ");
        write(1, print, strlen(print));

        // 아이디 전송
        memset(buf, 0, BUFSIZ);
        int n = read(1, buf, BUFSIZ);
        buf[strlen(buf) - 1] = '\0';
        write(s_num, buf, n);

        // 결과 반환
        memset(buf, 0, BUFSIZ);
        n = read(s_num, buf, BUFSIZ);
        
        if (!strcmp(buf, "no"))
        {
            strcpy(print, "아이디가 존재하지 않습니다.\n");
            write(1, print, strlen(print));
            continue;
        }
        else if (!strcmp(buf, "yes"))
        {
            break;
        }
        else
        {
            perror("else");
        }
    }
    
    while (1)
    {
        // 비밀번호 프로세스.
        strcpy(print, "비밀번호를 입력해주세요: ");
        write(1, print, strlen(print));

        // 비밀번호 전송
        memset(buf, 0, BUFSIZ);
        int n = read(1, buf, BUFSIZ);
        buf[strlen(buf) - 1] = '\0';
        write(s_num, buf, n);

        // 결과 반환
        memset(buf, 0, BUFSIZ);
        n = read(s_num, buf, BUFSIZ);
        
        if (!strcmp(buf, "no"))
        {
            strcpy(print, "비밀번호 입력이 틀렸습니다.\n");
            write(1, print, strlen(print));
            continue;
        }
        else if (!strcmp(buf, "yes"))
        {
            return 0;
        }
        else
        {
            perror("not 'yes' or 'no");
            continue;
        }
    }
    return 1;
}

int cEnter(int s_num)
{
    pid_t pid = fork();
    char print[BUFSIZ];
    char buf[BUFSIZ];

    // 부모는 계속 출력 대기
    if (pid != 0)
    {
        memset(print, 0, BUFSIZ);
        strcpy(print, "\n방에 입장하셨습니다\n");
        write(1, print, strlen(print));
        while (1)
        {   
            memset(print, 0, BUFSIZ);
            write(1, print, strlen(print));
            
            memset(buf, 0, BUFSIZ);
            int n = read(1, buf, BUFSIZ);
            write(s_num, buf, n);
        }
    }
    // 자식은 입력 대기하면서 화면에 출력만 하면 됨
    else
    {
        while (1)
        {
            memset(buf, 0, BUFSIZ);
            int n = read(s_num, buf, BUFSIZ);
            write(1, buf, n);
        }
    }
}

void waitS()
{
    char mesg[100];
    strcpy(mesg, "서버 기다리는중...\n");
    write(1, mesg, strlen(mesg));
}