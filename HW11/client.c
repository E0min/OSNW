#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define MAXLINE 1024

typedef struct
{
    int left_num;
    int right_num;
    char op;
    int result;
    int client_fd;
    char str[MAXLINE];
    short int error;
} Data;

void *send_thread(void *arg);
void *receive_thread(void *arg);

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in serveraddr;
    pthread_t snd_thread, rcv_thread;
    

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket error");
        return 1;
    }

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serveraddr.sin_port = htons(3600);

    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
    {
        perror("connect error");
        return 1;
    }

    // 스레드 생성
    if (pthread_create(&snd_thread, NULL, send_thread, &sockfd) != 0)
    {
        perror("Thread create error");
        close(sockfd);
        return 1;
    }
    if (pthread_create(&rcv_thread, NULL, receive_thread, &sockfd) != 0)
    {
        perror("Thread create error");
        close(sockfd);
        return 1;
    }

    pthread_join(snd_thread, NULL);
    pthread_join(rcv_thread, NULL);

    close(sockfd);
    return 0;
}

void *send_thread(void *arg)
{
    int sockfd = *(int *)arg;
    Data sdata;
    int sbyte;
    printf("입력 형식: num1 num2 op string\n");
    while (1)
    {
        
        scanf("%d %d %c %s", &sdata.left_num, &sdata.right_num, &sdata.op, sdata.str);

        if (strcmp(sdata.str, "quit") == 0)
        {
            break;
        }

        sdata.left_num = htonl(sdata.left_num);
        sdata.right_num = htonl(sdata.right_num);

        write(sockfd, (char *)&sdata, sizeof(sdata));
    }

    return NULL;
}

void *receive_thread(void *arg)
{
    int sockfd = *(int *)arg;
    struct tm timeinfo;
    Data rdata;
    int rbyte;

    while (1)
    {
        rbyte = read(sockfd, &rdata, sizeof(Data));
        if (rbyte <= 0)
        {
            break;
        }

        printf("결과: %d %c %d = %d ", ntohl(rdata.left_num), rdata.op, ntohl(rdata.right_num), ntohl(rdata.result));

        // 서버로부터 현재 시간 정보 읽기
        rbyte = read(sockfd, &timeinfo, sizeof(struct tm));
        if (rbyte <= 0)
        {
            break;
        }

        printf("현재 시간: %d-%02d-%02d %02d:%02d:%02d\n",
               timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
               timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }

    return NULL;
}
