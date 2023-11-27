#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#define MAXLINE 1024

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

int main(int argc, char **argv)
{
    struct sockaddr_in serveraddr;
    int server_sockfd;
    int client_len;
    int sbyte;
    int rbyte;
    Data sdata;
    Data rdata;
    struct tm timeinfo;
    pid_t pid;
    int len;

    // 소켓 생성
    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("error :");
        return 1;
    }

    // 서버 주소 설정
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serveraddr.sin_port = htons(3600);

    client_len = sizeof(serveraddr);

    // 서버에 연결
    if (connect(server_sockfd, (struct sockaddr *)&serveraddr, client_len) == -1)
    {
        perror("connect error :");
        return 1;
    }
    printf("입력 형식: num1 num2 op string\n");
    pid = fork();
    if (pid == 0)
    { // 자식 프로세스
        while (1)
        { // client에게 반복적으로 입력받는다.
            scanf("%d %d %c %s", &sdata.left_num, &sdata.right_num, &sdata.op, sdata.str);
            if (sdata.op != '+' && sdata.op != '-' && sdata.op != 'x' && sdata.op != '/' && sdata.op != '%')
            {
                continue;
            }
            if (strcmp(sdata.str, "quit") == 0)
            {
                close(server_sockfd);
                kill(getppid(),SIGTERM);
                return 0;
            }

            sdata.left_num = htonl(sdata.left_num);
            sdata.right_num = htonl(sdata.right_num);
            sbyte = write(server_sockfd, (char *)&sdata, sizeof(sdata)); // 이거는 수행 된다.

            
        }
    }
    else if (pid > 0)
    { // 부모 프로세스
        while (1)
        {
            rbyte = read(server_sockfd, (char *)&rdata, sizeof(rdata));
            read(server_sockfd, &timeinfo, sizeof(struct tm));

            printf("%d %c %d = %d ", ntohl(rdata.left_num), rdata.op, ntohl(rdata.right_num), ntohl(rdata.result));
            printf(" %d-%d-%d %d:%d:%d\n",
                   timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                   timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        }
    }
    close(server_sockfd);
    return 0;
}