#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/select.h>
#define MAXLINE 1024

typedef struct cal_string
{
    int left_num;
    int right_num;
    char op;
    int result;
    char str[MAXLINE];
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
    fd_set readfds, tempfds;
    int maxfd;

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

    FD_ZERO(&readfds);
    FD_SET(server_sockfd, &readfds);
    FD_SET(STDIN_FILENO, &readfds);
    
    if (server_sockfd > STDIN_FILENO)
    {
        maxfd = server_sockfd;
    }
    else
    {
        maxfd = STDIN_FILENO;
    }

    while (1)
    {
        tempfds = readfds;
        if (select(maxfd + 1, &tempfds, NULL, NULL, NULL) == -1)
        {
            perror("select error");
            return 1;
        }

        if (FD_ISSET(STDIN_FILENO, &tempfds))
        {
            // 키보드 입력 처리
            scanf("%d %d %c %s", &sdata.left_num, &sdata.right_num, &sdata.op, sdata.str);
            if (sdata.op != '+' && sdata.op != '-' && sdata.op != 'x' && sdata.op != '/' && sdata.op != '%')
            {
                continue;
            }
            if (strcmp(sdata.str, "quit") == 0)
            {
                close(server_sockfd);
                return 0;
            }

            sdata.left_num = htonl(sdata.left_num);
            sdata.right_num = htonl(sdata.right_num);
            sbyte = write(server_sockfd, (char *)&sdata, sizeof(Data));
        }

        if (FD_ISSET(server_sockfd, &tempfds))
        {
            // 서버로부터 데이터 수신
            rbyte = read(server_sockfd, (char *)&rdata, sizeof(Data));
            if (rbyte > 0)
            {
                printf("%d %c %d = %d %s\n", ntohl(rdata.left_num), rdata.op, ntohl(rdata.right_num), ntohl(rdata.result), rdata.str);
            }
        }
    }

    close(server_sockfd);
    return 0;
}