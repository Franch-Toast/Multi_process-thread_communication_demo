/*receive.c*/
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<errno.h>
#include<unistd.h>
#include<string.h>

#include<sys/msg.h> // 消息队列头文件

struct msgstru
{
    long msgtype;
    char msgtext[1024];
};

void main()
{
    struct msgstru msgs;
    char str[256];
    int ret_value;
    int msgid;// 用于存放消息队列的标识符
    int i = 3;// 剩余可失败次数
    key_t key;

    key = ftok(".",8);// key_t ftok(const char *pathname, int proj_id)
    printf("key = %x\n",key);// 获取到了不冲突的IPC键值

    // 消息队列获取连续失败三次则退出
    while(i > 0)
    {
        msgid = msgget(key,IPC_EXCL);// 获取IPC标识符，这里的第二个参数应该使用 IPC_CREAT | IPC_EXCL?

        if(msgid < 0)// 获取标识符失败
        {
            printf("msq not existed! errno=%d [%s]\n",errno,strerror(errno));
            i--; // 剩余可失败次数-1
            sleep(1);
            //continue;
        }
        else i = 3;// 获取成功，剩余可失败次数复位
        
        ret_value = msgrcv(msgid,&msgs,sizeof(struct msgstru),0,0);// type = 0获取消息队列的第一个消息，flag = 0默认为 非IPC_NOWAIT ，即队列为空时阻塞
        printf("type = [%ld],text = [%s]\n",msgs.msgtype,msgs.msgtext);
    }
}