#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>// fcntl.h头文件定义了文件操作等所用到的相关宏。
#include<unistd.h>
#include<string.h>

#include<sys/stat.h> // 有名管道创建头文件,轻松获取文件属性

/*
#include <sys/stat.h>

int mkfifo(const char *pathname, mode_t mode);  //返回值：成功返回0，出错返回-1
*/


int main()
{
    int fd;
    
    char *buff = "我是写文件，向你致以我真诚的问候。";

    printf("我是写进程，我的进程ID是：%d\n",getpid());

    if((fd = open("./fifo",O_WRONLY)) < 0) 
    {
        printf("我没找到管道，所以我滚了。\n");
        return -1;
    }
    // 以 只写 的形式创建管道

    for(int i = 0; i < 5; i++)
    {
        printf("我是写进程，我现在准备发送消息了。\n",buff);
        if(write(fd,buff,strlen(buff) + 1) < 0)
        {
            perror("写管道失败！\n");
            close(fd);
            return -1;
        }
        sleep(1);
    }
    close(fd);
    return 0;
}