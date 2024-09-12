#include "P_server.h"

#define PATH "client_info.txt"

int login(int c_num, user* data)
{   
    perror("login\n");
    int flag = 0; // 아이디 검증
    char c_flag[2]; // 클라에게 보내는 flag값
    c_flag[0] = '1';
    c_flag[1] = '\0';
    sleep(2);
    write(c_num, c_flag, strlen(c_flag));

    FILE* fd = fopen(PATH, "r");
    if (fd == NULL)
    {
        perror("fopen()");
        return -1;
    }
    
    char buf[BUFSIZ]; // 사용자 입력 저장
    char format[30]; // id:pw 포맷 저장
    char id[11]; // 아이디만 저장
    char pw[11]; // 비번만 저장
    
    while (1)
    {
        int n = read(c_num, buf, BUFSIZ);
        while (fgets(format, 30, fd) != NULL)
        {   
            strcpy(id, strtok(format, ":"));
            if (!strncmp(buf, id, strlen(buf)))
            {
                memset(buf, 0, BUFSIZ);
                strcpy(buf, "yes");
                write(c_num, buf, strlen(buf));
                strcpy(pw, strtok(NULL, ":"));
                flag = 1;
                break;
            }
        }
        if (!flag)
        {
            memset(buf, 0, BUFSIZ);
            strcpy(buf, "no");
            write(c_num, buf, strlen(buf));
        }
        else
            break;

        fseek(fd, 0, SEEK_SET);
    }

    // 세 번 틀리면 강제로 종료.
    while (1)
    {
        int n = read(c_num, buf, BUFSIZ);
        if (!strncmp(buf, pw, strlen(buf)))
        {
            memset(buf, 0, BUFSIZ);
            strcpy(buf, "yes");
            write(c_num, buf, strlen(buf));
            break;
        }
        memset(buf, 0, BUFSIZ);
        strcpy(buf, "no");
        write(c_num, buf, strlen(buf));
    }

    strcpy(data->name, id);
    fclose(fd);
    return 0;
}