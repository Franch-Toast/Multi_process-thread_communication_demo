
/*send.c*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<errno.h>

#include<sys/msg.h>

struct msgstru
{
    long msgtype;
    char msgtext[1024];
};

void main()
{
    struct msgstru msgs;
    int msg_type;
    char str[256];
    int ret_value;
    int msgid;
    key_t key;

    key = ftok(".",8);
    printf("key = %x\n",key);

    msgid = msgget(key,IPC_EXCL);// 获取IPC标识符，这里的第二个参数应该使用 IPC_CREAT | IPC_EXCL?
    if(msgid < 0)
    {
        msgid = msgget(key,IPC_CREAT|0777);//对于创建失败的消息队列重新创建，并给予权限

        if(msgid < 0)// 创建失败，直接推出
        {
            printf("failed to create msq | errno=%d [%s]\n",errno,strerror(errno));
            exit(-1);
        }
    }
    while(1)
    {
        printf("Input message type(end:0):");// 输入消息的优先级
        scanf("%d",&msg_type);
        if(msg_type == 0) break;// 以输入0结束
    
        printf("Input message to be sent:");// 输入消息本体
        scanf("%s",str);
        msgs.msgtype = msg_type;
        strcpy(msgs.msgtext,str);// 消息传递，其实这里很麻烦，可以直接写入

        ret_value = msgsnd(msgid,&msgs,sizeof(struct msgstru),IPC_NOWAIT);
        //对于队列已满的情况，不写入队列，并继续执行
        if(ret_value < 0)
        {
            printf("msgsnd() write msg failed,errno=%d[%s]\n",errno,strerror(errno));
            exit(-1);
        }
    }
    msgctl(msgid,IPC_RMID,0);// 删除对应的消息队列
}


