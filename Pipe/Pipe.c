
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>// 是Unix/Linux系统的基本系统数据类型的头文件，含有size_t，time_t，pid_t等类型。

#include<unistd.h> 
/*
unistd.h是unix std的意思，
是POSIX标准定义的unix类系统定义符号常量的头文件，
包含了许多UNIX系统服务的函数原型，
例如read函数、write函数和getpid函数。
*/

// int pipe(int fd[2]); // 返回值：若成功返回0，失败返回-1


int main()
{
    int fd[2]; // 定义两个文件描述符
    pid_t pid;
    char buff[40] = {0};

    if(pipe(fd) < 0) return -1;// 管道创建失败

    if((pid = fork()) < 0) return -1;// 创建子进程，获取父子进程的pid

    if(pid > 0) // 父进程
    {
        sleep(3);
        
        printf("父进程的进程ID为：%d\n",getpid());

        close(fd[0]);// 关闭读端
        write(fd[1],"从遥远的父进程发来的信息！\n",strlen("从遥远的父进程发来的信息！\n"));

    }
    else
    {
        printf("子进程的进程ID为：%d\n",getpid());

        close(fd[1]);// 关闭写端
        read(fd[0],buff,40);
        printf("收到父进程的消息：%s",buff);
        exit(0);
    }
    return 0;
}
