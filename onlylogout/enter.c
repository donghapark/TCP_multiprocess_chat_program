#include "P_server.h"

int enter(int c_num, user* data, int pfd)
{
    perror("enter");
    char c_flag[2]; // 클라에게 보내는 flag값
    c_flag[0] = '2';
    c_flag[1] = '\0';
    write(c_num, c_flag, strlen(c_flag));

    // 메시지 받는 놈(c_num꺼만) + 보내는 놈(방 모두한테)
    while (1)
    {
        int pid = fork();
        
        // 메세지 보내버리기(서버에 -> 서버가 돌면서 방 번호 같은 애한테 전송)
        if (pid == 0)
        {
            // 포맷: (플래그):(소켓 번호):(방 번호):(아이디):(내용);
            // 플래그: 0 이상 = 채팅, 0 미만 = 데이터 저장
            char format[100];
            char mesg[100];
            while (1)
            {
                memset(format, 0, sizeof(format));
                sprintf(format, "%d:%d:%d:%s:", 0, data->fd, data->rn, data->name);

                memset(mesg, 0, sizeof(mesg));
                int n = read(c_num, mesg, sizeof(mesg));

                strcat(format, mesg);

                write(pfd, format, strlen(format));
            }
        }
        else
        {
            while(1)
            {

            }
        }
    }
}