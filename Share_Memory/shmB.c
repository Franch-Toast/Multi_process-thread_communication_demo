/*shmB.c*/
#include<stdio.h>
#include<stdlib.h>
#include<sys/msg.h>  // 消息队列
#include<string.h>
#include<unistd.h>

#include<sys/shm.h>  // 共享内存
#include<sys/sem.h>  // 信号量

int main()
{
    key_t key = ftok(".",1);// 创建相同的IPC键

    int shmid = shmget(key,1024,0);// 获取共享内存

    char *str = (char *)shmat(shmid,NULL,0);// 建立连接

    while(1)
    {
        printf("%s",str);

        if(strncmp(str,"end",3) == 0)
        {
            break;
        }
        sleep(1);
    }

    shmdt(str);// 断开连接，但是没有删除
    printf("Disconnect shared memory.\n");

    return 0;
}