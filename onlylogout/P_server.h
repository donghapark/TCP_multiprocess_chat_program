#ifndef _P_SERVER_H
#define _P_SERVER_H
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <wait.h>
typedef struct
{
    int fd; // client socket number
    int rn; // room number
    int count; // user's number
    char name[20];
} user;
int login(int c_num, user* data);
int enter(int c_num, user* data, int pfd);
#endif