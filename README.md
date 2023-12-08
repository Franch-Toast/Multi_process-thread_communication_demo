# 多进程通信与多线程通信

## 多进程通信

在开始前需要了解：

我们把信号量、消息和共享内存统称 **System V IPC 的对象**，每一个对象都具有同样类型的接口，即系统调用。就像每个文件都有一个打开文件号一样，每个对象也都有唯一的识别号，进程可以通过系统调用传递的识别号来存取这些对象，与文件的存取一样，对这些对象的存取也要验证存取权限，System V IPC 可以通过系统调用对对象的创建者设置这些对象的存取权限。

在 Linux 内核中，System V IPC 的所有对象有一个公共的数据结构 pc_perm 结构，它是IPC 对象的权限描述，在 linux/ipc.h 中定义如下：

```c
struct ipc_perm
{
	__kernel_key_t	key;// 键
	__kernel_uid_t	uid;// 对象拥有者对应进程的有效用户识别号和有效组识别号
	__kernel_gid_t	gid;
	__kernel_uid_t	cuid;// 对象创建者对应进程的有效用户识别号和有效组织识别号
	__kernel_gid_t	cgid;
	__kernel_mode_t	mode; // 存储模式
	unsigned short	seq;// 序列号
};
```

在这个结构中，要进一步说明的是键（key）。键和识别号指的是不同的东西。系统支持两种键：公有和私有。

- 如果键是公有的，则系统中所有的进程通过权限检查后，均可以找到 System V IPC 对象的识别号。
- 如果键是私有的，则键值为 0，说明每个进程都可以用键值0 建立一个专供其私用的对象。注意，对 System V IPC 对象的引用是通过识别号而不是通过键，从后面的系统调用中可了解这一点。



### Pipe

#### 特点

1. 它是半双工的（即数据只能在一个方向上流动），具有固定的读端和写端。
2. 它只能用于具有亲缘关系的进程之间的通信（也是父子进程或者兄弟进程之间）。
3. 它可以看成是一种特殊的文件，对于它的读写也可以使用普通的read、write 等函数。但是它不是普通的文件，并不属于其他任何文件系统，并且只存在于内存中。

#### 原型

```c
#include <unistd.h>

int pipe(int fd[2]);    // 返回值：若成功返回0，失败返回-1
```

当一个管道建立时，它会创建两个文件描述符：`fd[0]`为读而打开，`fd[1]`为写而打开。

![img](https://pic1.zhimg.com/80/v2-e99bb77084831e65d70642eaf30ea2a4_720w.webp)

要关闭管道只需将这两个文件描述符关闭即可。

#### 示例

单个进程中的管道几乎没有任何用处。所以，通常调用 pipe 的进程接着调用 fork，这样就创建了父进程与子进程之间的 IPC 通道。若要数据流从父进程流向子进程，则关闭父进程的读端（`fd[0]`）与子进程的写端（`fd[1]`）；反之，则可以使数据流从子进程流向父进程。

```c

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
```



### FIFO

FIFO( First Input First Output)简单说就是指先进先出，也称为命名管道，它是一种文件类型。

#### 特点

1. FIFO可以在无关的进程之间交换数据，与无名管道不同。
2. FIFO有路径名与之相关联，它以一种特殊设备文件形式存在于文件系统中。

#### 原型

```c
#include <sys/stat.h>

int mkfifo(const char *pathname, mode_t mode);  //返回值：成功返回0，出错返回-1
```

其中的 mode 参数与open函数中的 mode 相同。一旦创建了一个 FIFO，就可以用一般的文件I/O函数操作它。

当 open 一个FIFO时，是否设置非阻塞标志（O_NONBLOCK）的区别：

- 若没有指定O_NONBLOCK（默认），只读 open 要阻塞到某个其他进程为写而打开此 FIFO。类似的，只写 open 要阻塞到某个其他进程为读而打开它。
- 若指定了O_NONBLOCK，则只读 open 立即返回。而只写 open 将出错返回 -1 如果没有进程已经为读而打开该 FIFO，其errno置ENXIO。

#### 示例

FIFO的通信方式类似于在进程中使用文件来传输数据，只不过FIFO类型文件同时具有管道的特性。在数据读出时，FIFO管道中同时清除数据，并且“先进先出”。

```c
// read.c
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
```



```c
// write.c
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
```









### 消息队列

#### 特点

1. 消息队列是面向记录的，其中的消息具有特定的格式以及特定的优先级。
2. 消息队列独立于发送与接收进程。进程终止时，消息队列及其内容并不会被删除。
3. 消息队列可以实现消息的随机查询,消息不一定要以先进先出的次序读取,也可以按消息的类型读取。

#### 原型

```c
#include <sys/msg.h>

// 创建或打开消息队列：成功返回队列ID，失败返回-1
int msgget(key_t key, int flag);    

// 添加消息：成功返回0，失败返回-1
int msgsnd(int msqid, const void *ptr, size_t size, int flag);  

// 读取消息：成功返回消息数据的长度，失败返回-1
int msgrcv(int msqid, void *ptr, size_t size, long type,int flag);  

// 控制消息队列：成功返回0，失败返回-1
int msgctl(int msqid, int cmd, struct msqid_ds *buf);   
```

在以下两种情况下，msgget将创建一个新的消息队列：

- 如果没有与键值key相对应的消息队列，并且flag中包含了IPC_CREAT标志位。
- key参数为IPC_PRIVATE。

```c
/* Special key values.  */
#define IPC_PRIVATE	((__key_t) 0)	/* Private key.  */
```



函数msgrcv在读取消息队列时，type参数有下面几种情况：

- type == 0，返回队列中的第一个消息；
- type > 0，返回队列中消息类型为 type 的第一个消息；
- type < 0，返回队列中消息类型值小于或等于 type 绝对值的消息，如果有多个，则取类型值最小的消息。

可以看出，type值非 0 时用于以非先进先出次序读消息。也可以把 type 看做优先级的权值。



#### 示例

```c
/*receive.c*/
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<errno.h>
#include<unistd.h>
#include<string.h>

#include<sys/msg.h> // 消息队列头文件

struct msgstru// 自定义了消息队列中传输的数据格式
{
    long msgtype;// 消息优先级
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
        else i = 0;// 获取成功，剩余可失败次数复位
        
        ret_value = msgrcv(msgid,&msgs,sizeof(struct msgstru),0,0);// type = 0获取消息队列的第一个消息，flag = 0默认为 非IPC_NOWAIT ，即队列为空时阻塞
        printf("type = [%ld],text = [%s]\n",msgs.msgtype,msgs.msgtext);
    }
}
```

关于IPC的 ftok函数 和 IPC键值(key_t)可以参考[博文]([linux进程间通信--消息队列相关函数（ftok）详解-CSDN博客](https://blog.csdn.net/andylauren/article/details/78821655))。



```c
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
```





### 共享内存

共享内存（Shared Memory），指两个或多个进程共享一个给定的存储区。

#### 特点

1. 共享内存是最快的一种 IPC，因为进程是直接对内存进行存取。
2. 因为多个进程可以同时操作，所以需要进行同步。
3. 信号量+共享内存通常结合在一起使用，信号量用来同步对共享内存的访问。

#### 原型

```c
#include <sys/shm.h>

int shmget(key_t key, size_t size, int flag);   
// 创建或获取一个共享内存：成功返回共享内存ID，失败返回-1

void *shmat(int shm_id, const void *addr, int flag);    
// 连接共享内存到当前进程的地址空间：成功返回指向共享内存的指针，失败返回-1

int shmdt(void *addr);  
// 断开与共享内存的连接：成功返回0，失败返回-1

int shmctl(int shm_id, int cmd, struct shmid_ds *buf);  
// 控制共享内存的相关信息：成功返回0，失败返回-1
```

当用shmget 函数创建一段共享内存时，必须指定其 size；而如果引用一个已存在的共享内存，则将 size 指定为0 。

当一段共享内存被创建以后，它并不能被任何进程访问。必须使用shmat 函数连接该共享内存到当前进程的地址空间，连接成功后把共享内存区对象映射到调用进程的地址空间，随后可像本地空间一样访问。

shmdt 函数是用来断开shmat 建立的连接的。注意，这并不是从系统中删除该共享内存，只是当前进程不能再访问该共享内存而已。

shmct 函数可以对共享内存执行多种操作，根据参数 cmd 执行相应的操作。常用的是IPC_RMID（从系统中删除该共享内存）。

```c
/* Control commands for `msgctl', `semctl', and `shmctl'.  */
#define IPC_RMID	0		/* Remove identifier.  */
#define IPC_SET		1		/* Set `ipc_perm' options.  */
#define IPC_STAT	2		/* Get `ipc_perm' options.  */
#ifdef __USE_GNU
# define IPC_INFO	3		/* See ipcs.  */
#endif
```





#### 示例

```c
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
```



```c
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
```





### 信号量

信号量（semaphore）与已经介绍过的 IPC 结构不同，它是一个计数器。信号量用于实现进程间的互斥与同步，而不是用于存储进程间通信数据。

#### 特点

1. 信号量用于进程间同步，若要在进程间传递数据需要结合共享内存。
2. 信号量基于操作系统的 PV 操作，程序对信号量的操作都是原子操作。
3. 每次对信号量的 PV 操作不仅限于对信号量值加 1 或减 1，而且可以加减任意正整数。
4. 支持信号量组。

#### 原型

最简单的信号量是只能取 0 和 1 的变量，这也是信号量最常见的一种形式，叫做二值信号量（Binary Semaphore）。而可以取多个正整数的信号量被称为通用信号量。

Linux 下的信号量函数都是在通用的信号量数组上进行操作，而不是在一个单一的二值信号量上进行操作。

```c
#include <sys/sem.h>

int semget(key_t key, int num_sems, int sem_flags);     
// 创建或获取一个信号量组：若成功返回信号量集ID，失败返回-1

int semop(int semid, struct sembuf semoparray[], size_t numops);    
// 对信号量组进行操作，改变信号量的值：成功返回0，失败返回-1

int semctl(int semid, int sem_num, int cmd, ...);       
// 控制信号量的相关信息
```



当semget创建新的信号量集合时，必须指定集合中信号量的个数（即num_sems），通常为1； 如果是引用一个现有的集合，则将num_sems指定为 0 。

```c
/* Mode bits for `msgget', `semget', and `shmget'.  */
#define IPC_CREAT	01000		/* Create key if key does not exist. */
#define IPC_EXCL	02000		/* Fail if key exists.  */
#define IPC_NOWAIT	04000		/* Return error on wait.  */
```

在semop函数中，sembuf结构的定义如下：

```c
/* 来自 sys/sem.h */
/* Structure used for argument to `semop' to describe operations.  */
struct sembuf
{
  unsigned short int sem_num;	// 信号量组中对应的序号，0～sem_nums-1
  short int sem_op;		// 信号量值在一次操作中的改变量
  short int sem_flg;		// IPC_NOWAIT, SEM_UNDO
};


/* 来自 bits/sem.h */
/* Flags for `semop'.  */
#define SEM_UNDO	0x1000		
//SEM_UNDO用于将修改的信号量值在进程正常退出（调用exit退出或main执行完）或异常退出（如段异常、除0异常、收到KILL信号等）时归还给信号量。

```



其中 sem_op 是一次操作中的信号量的改变量：

semop系统调用改变信号量的值，即执行P，V操作。一般的信号量的P,V操作会修改内核中的这些变量：

```cpp
  unsigned short int semval;  // 信号量的值                                                
  unsigned short int semzcnt; // 等待信号量变为0的进程数量                                 
  unsigned short int semncnt;  // 等待信号量值增加的进程数量                               
  pid_t sempid; // 上一次执行semop操作的进程id 
```



1. 若sem_op > 0，表示进程释放相应的资源数，将 sem_op 的值加到信号量的值上。如果有进程正在休眠等待此信号量，则唤醒它们。

2. 若sem_op < 0，请求 sem_op 的绝对值的资源：


- 如果相应的资源数可以满足请求，则将该信号量的值减去sem_op的绝对值，函数成功返回。

- 当相应的资源数不能满足请求时，这个操作与sem_flg有关：
  - sem_flg 指定IPC_NOWAIT，则semop函数出错返回EAGAIN。
  - sem_flg 没有指定IPC_NOWAIT，则将该信号量的semncnt值加1，然后进程挂起直到下述情况发生：
    - 当相应的资源数可以满足请求，此信号量的semncnt值减1，该信号量的值减去sem_op的绝对值。成功返回；
    - 此信号量被删除，函数smeop出错返回EIDRM；
    - 进程捕捉到信号，并从信号处理函数返回，此情况下将此信号量的semncnt值减1，函数semop出错返回EINTR


3. 若sem_op == 0，进程阻塞直到信号量的相应值为0：

- 当信号量已经为0，函数立即返回。

- 如果信号量的值不为0，则依据sem_flg决定函数动作：

  - sem_flg指定IPC_NOWAIT，则出错返回EAGAIN。
  - sem_flg没有指定IPC_NOWAIT，则将该信号量的semncnt值加1，然后进程挂起直到下述情况发生：
    - 信号量值为0，将信号量的semzcnt的值减1，函数semop成功返回；
    - 此信号量被删除，函数smeop出错返回EIDRM；
    - 进程捕捉到信号，并从信号处理函数返回，在此情况将此信号量的semncnt值减1，函数semop出错返回EINTR

在semctl函数中的命令有多种，这里就说两个常用的：

- SETVAL：用于初始化信号量为一个已知的值。所需要的值作为联合semun的val成员来传递。在信号量第一次使用之前需要设置信号量。
- IPC_RMID：删除一个信号量集合。如果不删除信号量，它将继续在系统中存在，即使程序已经退出，它可能在你下次运行此程序时引发问题，而且信号量是一种有限的资源。

```c
/* Commands for `semctl'.  */
#define GETPID		11		/* get sempid */
#define GETVAL		12		/* get semval */
#define GETALL		13		/* get all semval's */
#define GETNCNT		14		/* get semncnt */
#define GETZCNT		15		/* get semzcnt */
#define SETVAL		16		/* set semval */
#define SETALL		17		/* set all semval's */


/* Control commands for `msgctl', `semctl', and `shmctl'.  */
#define IPC_RMID	0		/* Remove identifier.  */
#define IPC_SET		1		/* Set `ipc_perm' options.  */
#define IPC_STAT	2		/* Get `ipc_perm' options.  */
#ifdef __USE_GNU
# define IPC_INFO	3		/* See ipcs.  */
#endif
```

其实可以添加第四个参数，在<sys/sem.h>中定义了它的推荐格式。

```c
union semun {                                                                            
  int val;     // 用于SETVAL命令                                                         
  struct semid_ids* buf;   // 用于IPC_STAT和IPC_SET命令                                  
  unsigned short* array;   // 用于GETALL和SETALL命令                                     
  struct seminfo* _buf;    // 用于IPC_INFO命令                                           
};                                                                                       
                                                                                         
struct seminfo {                                                                         
  int semmap;   // Linux 内核没有使用                                                    
  int semmni;   // 系统最多可以拥有的信号量集数目                                        
  int semmns;   // 系统最多可以拥有的信号量的数目                                        
  int semmnu;   // Linux 内核没有使用                                                    
  int semmsl;   // 一个信号量集最多允许包含的信号量数目                                  
  int semopm;   // semop一次最多能执行的sem_op操作数目                                   
  int semume;   // Linux内核没有使用                                                     
  int semusz;   // sem_undo 结构体的大小                                                 
  int semvmx;   // 最大允许的信号量值                                                    
  int semaem;   // 最多允许的UNDO的次数                                                  
}; 
```



#### 示例

```c
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<errno.h>
#include <unistd.h>

#include<sys/sem.h> // 信号量头文件

union semun     // 联合体，用于semctl初始化
{
    int val;             /*for SETVAL*/
    struct semid_ds *buf;
    unsigned short  *array;
};

int init_sem(int sem_id, int value)     // 初始化信号量
{
    union semun tmp;
    tmp.val = value;
    if(semctl(sem_id, 0, SETVAL, tmp) == -1)// SETVAL初始化信号量为一个已知的值,需要连同联合semun一起传递
    {
        perror("Init Semaphore Error");
        return -1;
    }
    return 0;
}

// P操作:
//    若信号量值为1，获取资源并将信号量值-1
//    若信号量值为0，进程挂起等待
int sem_p(int sem_id)
{
    struct sembuf sbuf;
    sbuf.sem_num = 0; /*序号*/
    sbuf.sem_op = -1; /*P操作*/
    sbuf.sem_flg = SEM_UNDO;

    if(semop(sem_id, &sbuf, 1) == -1)
    {
        perror("P operation Error");
        return -1;
    }
    return 0;
}

// V操作：
//    释放资源并将信号量值+1
//    如果有进程正在挂起等待，则唤醒它们
int sem_v(int sem_id)
{
    struct sembuf sbuf;
    sbuf.sem_num = 0; /*序号*/
    sbuf.sem_op = 1;  /*V操作*/
    sbuf.sem_flg = SEM_UNDO;

    if(semop(sem_id, &sbuf, 1) == -1)
    {
        perror("V operation Error");
        return -1;
    }
    return 0;
}

int del_sem(int sem_id)     //删除信号量集
{
    union semun tmp;
    if(semctl(sem_id, 0, IPC_RMID, tmp) == -1)
    {
        perror("Delete Semaphore Error");
        return -1;
    }
    return 0;
}


int main()
{
    int sem_id;  //信号量集ID
    key_t key;
    pid_t pid;

    if((key = ftok(".", 'z')) < 0)  //获取key值
    {
        perror("ftok error");
        exit(1);
    }

    if((sem_id = semget(key, 1, IPC_CREAT|0666)) == -1)  //创建信号量集，其中只有一个信号量
    {
        perror("semget error");
        exit(1);
    }

    init_sem(sem_id, 0);    //初始化：初值设为0资源被占用

    if((pid = fork()) == -1)
        perror("Fork Error");
    else if(pid == 0) /*子进程*/
    {
        sleep(2);
        printf("This is the child process. (PID=%d)\n", getpid());
        sem_v(sem_id);  /*释放资源*/
    }
    else  /*父进程*/
    {
        sem_p(sem_id);   /*等待资源*/
        printf("This is the parent process. (PID=%d)\n", getpid());
        sem_v(sem_id);   /*释放资源*/
        del_sem(sem_id); /*删除信号量集*/
    }
    return 0;
}
```





