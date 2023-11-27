#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h> // Added for shmget, semget
#include <sys/shm.h> // Added for shared memory
#include <sys/sem.h> // Added for semaphores
#include <time.h>

#define MAXLINE 1024
#define PORTNUM 3600
int pastp = 0;
typedef struct cal_string
{
    int left_num;
    int right_num;
    char op;
    int result;
    int sign;
    char str[MAXLINE];
    short int error;
} Data;
union semun
{
    int val;
};

int main(int argc, char **argv)

{
    Data rdata;

    int listen_fd, client_fd;
    struct sockaddr_in client_addr, server_addr;
    socklen_t addrlen;

    int shmid;
    Data *shmdata; // Pointer declaration

    pid_t pid;
    pid_t pastpid = -1;

    void *shared_memory = NULL;

    int semid;
    union semun sem_union;
    struct sembuf semopen = {0, -1, SEM_UNDO};
    struct sembuf semclose = {0, 1, SEM_UNDO};

    // 소켓 생성
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return 1;
    }
    memset((void *)&server_addr, 0x00, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORTNUM);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind error");
        return 1;
    }
    if (listen(listen_fd, 5) == -1)
    {
        perror("listen error");
        return 1;
    }
    // 각 클라이언트와 연결시 생성된 공유 메모리에서 사용할 세마포어 생성
    semid = semget((key_t)3477, 1, 0666 | IPC_CREAT); // Corrected semget call
    if (semid == -1)
    {
        perror("semget failed");
        return 1;
    }
    sem_union.val = 1;
    if (-1 == semctl(semid, 0, SETVAL, sem_union))
    {
        return 1;
    }
    signal(SIGCHLD, SIG_IGN);

    while (1) // 새로운 클라이언트의 반복적인 연결을 수행하는 부모 프로세스
    {
        client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);
        printf("New client connected %s(%d)\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
        if (client_fd == -1)
        {
            printf("accept error\n");
            break;
        }
        // 새로운 클라이언트에서 사용할 공유 메모리 생성
        shmid = shmget(IPC_PRIVATE, sizeof(Data), 0666 | IPC_CREAT); // Correct sizeof usage
        if (shmid == -1)
        {
            perror("shmget failed");
            return 1;
        }
        shared_memory = shmat(shmid, NULL, 0);
        if (shared_memory == (void *)-1)
        {
            perror("shmat failed");
            return 1;
        }
        shmdata = (Data *)shared_memory; // Corrected casting
        memset(shmdata, 0, sizeof(Data));
        shmdata->sign = 0;
        // 클라이언트의 연결을 기다린다.
        addrlen = sizeof(client_addr);

        pid = fork(); // 새로운 클라이언트가 연결되면, 연결과 통신을 분리
        if (pid == 0)
        {
            // 해당 클라이언트의 consumer 생성
            pid = fork(); // consumer 생성을 위한 fork()
            if (pid == 0)
            {
                while (1) // 요게 반복수행 된다 지금
                {
                    if (semop(semid, &semopen, 1) == -1)
                    {
                        return 1;
                    }

                    if (shmdata->sign != 1) // data.sign이 1이 아니란 것은 아직 입력을 안한 것이므로 임계 영역 탈출
                    {
                        semop(semid, &semclose, 1);
                    }
                    else
                    { // shmdata의 sign이 1일때만 임계영역 들어가서 공유 메모리 읽고 쓰기
                        switch (shmdata->op)
                        {
                        case '+':
                            printf("덧셈 연산\n");
                            shmdata->result = shmdata->left_num + shmdata->right_num;
                            break;
                        case '-':
                            printf("뺄셈 연산\n");
                            shmdata->result = shmdata->left_num - shmdata->right_num;
                            break;
                        case 'x':
                            printf("곱셈 연산\n");
                            shmdata->result = shmdata->left_num * shmdata->right_num;
                            break;
                        case '/':
                            printf("나눗셈 연산\n");
                            shmdata->result = shmdata->left_num / shmdata->right_num;
                            break;
                        case '%':
                            printf("나머지 연산\n");
                            shmdata->result = shmdata->left_num % shmdata->right_num;
                            break;
                        default:
                            break;
                        }
                        // 클라이언트한테 전송해야 함
                        rdata.result = htonl(shmdata->result);
                        rdata.left_num = htonl(shmdata->left_num);
                        rdata.right_num = htonl(shmdata->right_num);
                        rdata.op = shmdata->op;
                        if (pastpid != -1)
                        {
                            kill(pastpid, SIGTERM);
                        }
                        pastpid = pid = fork();
                        if (pid == 0)
                        {
                            while (1)
                            {
                                sleep(10);
                                write(client_fd, (Data *)&rdata, sizeof(rdata)); // 클라이언트에게 전송
                                time_t t = time(NULL);
                                struct tm *timeinfo = localtime(&t);

                                // tm 구조체를 클라이언트에게 전송
                                write(client_fd, timeinfo, sizeof(struct tm));
                            }
                        }
                        shmdata->sign = 0;
                        semop(semid, &semclose, 1);
                    }
                }
            }
            // 클라이언트와 연결된 자식 서버 프로세스
            close(listen_fd);                                                                                         // 연결 소켓은 닫는다.
            while (read(client_fd, (char *)&rdata, sizeof(rdata)) > 0)                                                // 연결된 클라이언트로부터 데이터 반복적으로 읽어오는 부분
            {                                                                                                         // client 입력이 계속 들어올 경우 반복
                printf("From client %s(%d): %s\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port, rdata.str); // 여기까지는 됐다.

                // 데이터 받았으니 이제 계산을 수행 해야 한다.
                pid = fork(); // producer 자식 생성을 위해 fork한다.
                if (pid == 0)
                {
                    while (1)
                    {
                        // 클라이언트로 부터 받은 데이터를 공유 메모리에 저장
                        if (semop(semid, &semopen, 1) == -1) // 잠구기
                        {
                            return 1;
                        }

                        if (shmdata->sign != 0)
                            semop(semid, &semclose, 1);
                        else // shmdata의 sign 값이 0 이라면 공유 메모리에 접근하여 값 전달
                        {
                            shmdata->left_num = ntohl(rdata.left_num);
                            shmdata->right_num = ntohl(rdata.right_num);
                            shmdata->op = rdata.op;
                            shmdata->sign = 1;          // 읽을 데이터가 있으니 1로 바꿔준다.
                            semop(semid, &semclose, 1); // 열기
                            break;
                        }
                    }
                }
            }

            // 부모 프로세스
        }
        else if (pid > 0)
        {
            close(client_fd);
        } // 통신 소켓을 닫는다.
    }
    return 0;
}