/*shmA.c*/
#include<stdio.h>
#include<stdlib.h>
#include<sys/msg.h>  // 消息队列
#include<string.h>

#include<sys/shm.h>  // 共享内存
#include<sys/sem.h>  // 信号量

int main()
{
    key_t key = ftok(".",1);// 获取IPC键

    int shmid = shmget(key,1024,IPC_CREAT|0666);// 创建一个1024Bytes大小的共享内存空间

    if(shmid == -1)
    {
        printf("Failed to create shared memory.\n");
        exit(-1);
    }

    char *str = (char *)shmat(shmid,NULL,0);
    // 连接共享内存到当前进程的地址空间：成功返回指向共享内存的指针，失败返回-1

    if(*str == -1)
    {
        printf("Failed to create shared memory.\n");
        exit(-1);
    }
    else printf("Shared memory created successfully.\n");

    while(1)
    {
        printf("Please enter:");
        fgets(str,127,stdin);// 从标准输入中读取127个字符存放入str中

        if(strncmp(str,"end",3) == 0)// 输入的如果是end则退出
        {
            break;
        }
    }

    shmdt(str);// 断开连接
    printf("Disconnect shared memory.\n");
    shmctl(shmid,IPC_RMID,NULL);// 删除共享内存
    printf("Delete shared memory.\n");

    return 0;
}