#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>// fcntl.h头文件定义了文件操作等所用到的相关宏。
#include<unistd.h>
#include<string.h>
#include<errno.h>// 存放了系统调用的错误信息

#include<sys/stat.h> // 有名管道创建头文件,轻松获取文件属性

/*
#include <sys/stat.h>

int mkfifo(const char *pathname, mode_t mode);  //返回值：成功返回0，出错返回-1
*/


int main()
{
    int fd;
    int len = 0;
    pid_t pid;
    char buff[1000];

    printf("我是读进程，我的进程ID是：%d\n",getpid());

    if(mkfifo("./fifo",0666) < 0 && errno != EEXIST)    //创建FIFO管道
    {
        perror("管道创建失败。\n");
    }

    if((fd = open("./fifo",O_RDONLY)) < 0) return -1;
    // 以 只读 的形式创建管道

    while(1)
    {
        if((len = read(fd,buff,1024)) > 0)   //读取FIFO管道
        {
            printf("我是读进程，我收到的消息的长度是 %d bytes.\n",len);
            printf("我是读进程，我收到的消息是:%s\n",buff);
        }
        else
        {
            break;
        }
    }
    close(fd);
    return 0;
}