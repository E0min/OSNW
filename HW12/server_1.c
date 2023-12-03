#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXLINE 1024
#define PORTNUM 3600

typedef struct
{
    int left_num;
    int right_num;
    char op;
    int result;
    char str[MAXLINE];
}Data;
void calculate(Data *data, char *constr);

int main(int argc, char **argv)
{
    int listen_fd, client_fd;
    int maxfd = 0;
    fd_set readfds, allfds;
    socklen_t addrlen;
    char constr[MAXLINE];
    struct sockaddr_in client_addr, server_addr;
    Data *data = malloc(sizeof(Data));
    int i, sockfd, fd_num;

    memset(constr, 0x00, sizeof(constr));

    // 소켓 생성
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORTNUM);

    // 소켓을 포트에 바인딩
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind error");
        return 1;
    }

    // 소켓을 듣기 모드로 설정
    if (listen(listen_fd, 5) == -1)
    {
        perror("listen error");
        return 1;
    }

    // fd_set 초기화
    FD_ZERO(&readfds);

    // 듣기 소켓을 fd_set에 추가
    FD_SET(listen_fd, &readfds);
    maxfd = listen_fd;

    printf("Server is running on port %d\n", PORTNUM);

    while (1)
    {
        allfds = readfds;
        // select 함수로 입출력 이벤트 대기
        fd_num = select(maxfd + 1, &allfds, NULL, NULL, NULL);

        // 듣기 소켓으로부터 데이터가 있다면 (즉, 클라이언트로부터 연결요청)
        if (FD_ISSET(listen_fd, &allfds))
        {
            addrlen = sizeof(client_addr);
            client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);
            if (client_fd == -1)
            {
                perror("accept error");
                continue;
            }

            FD_SET(client_fd, &readfds); // 클라이언트 소켓을 fd_set에 추가
            if (client_fd > maxfd)
                maxfd = client_fd;

            printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            continue;
        }

        // 모든 소켓에 대해 검사
        for (i = 0; i <= maxfd; i++)
        {
            sockfd = i;
            if (FD_ISSET(sockfd, &allfds))
            {
                if (read(sockfd, data, sizeof(Data)) <= 0)
                {
                    close(sockfd);
                    FD_CLR(sockfd, &readfds); // 소켓을 fd_set에서 제거
                }
                else
                {
                    data->left_num = ntohl(data->left_num);
                    data->right_num = ntohl(data->right_num);
                    if (strcmp(data->str, "quit") == 0)
                    {
                        FD_CLR(sockfd, &readfds); // 클라이언트를 fd_set에서 제거
                    }
                    else
                    {
                        strcat(constr, data->str);
                        calculate(data, constr);
                        
                        // constr의 내용을 data->str에 안전하게 복사합니다.
                        strncpy(data->str, constr, MAXLINE - 1);
                        data->str[MAXLINE - 1] = '\0';     // NULL 문자로 끝을 보장합니다.
                        write(sockfd, data, sizeof(Data)); // 클라이언트에게 에코
                        
                    }
                    if (--fd_num <= 0)
                        break; // 모든 이벤트 처리 완료
                }
            }
        }
    }
    return 0;
}

void calculate(Data *data, char *constr)
{
    switch (data->op)
    {
    case '+':
        data->result = data->left_num + data->right_num;
        break;
    case '-':
        data->result = data->left_num - data->right_num;
        break;
    case '*':
        data->result = data->left_num * data->right_num;
        break;
    case '/':
        if (data->right_num != 0)
        {
            data->result = data->left_num / data->right_num;
        }
        else
        {
            data->result = 0; // 나누기 오류 처리
        }
        break;
    default:
        data->result = 0; // 잘못된 연산자 처리
    }
    data->result = htonl(data->result);
    data->left_num = htonl(data->left_num);
    data->right_num = htonl(data->right_num);
    printf("Received string data: %s\n", data->str);
    printf("Received sumstring data: %s\n", constr);
}

