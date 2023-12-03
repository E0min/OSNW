#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXLINE 1024
#define PORTNUM 3600
#define MAX_CLIENTS 1024

typedef struct
{
    int left_num;
    int right_num;
    char op;
    int result;
    char str[MAXLINE];
} Data;

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
    case 'x':
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

int main(int argc, char **argv)
{
    int listen_fd, client_fd;
    int maxfd = 0;
    fd_set readfds, allfds;
    socklen_t addrlen;
    char constr[MAXLINE];
    struct sockaddr_in client_addr, server_addr;
    Data *data[MAX_CLIENTS];
    int i, sockfd, fd_num;
    int address[MAX_CLIENTS] = {
        0,
    };

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        data[i] = NULL;
    }

    memset(constr, 0x00, sizeof(constr));

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
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

    FD_ZERO(&readfds);
    FD_SET(listen_fd, &readfds);
    maxfd = listen_fd;

    printf("Server is running on port %d\n", PORTNUM);
    int connected_clients = 0;

    while (1)
    {
        allfds = readfds;
        fd_num = select(maxfd + 1, &allfds, NULL, NULL, NULL);

        if (FD_ISSET(listen_fd, &allfds))
        {
            addrlen = sizeof(client_addr);
            client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);
            if (client_fd == -1)
            {
                perror("accept error");
                continue;
            }

            // 클라이언트 소켓과 데이터 배열의 인덱스는 짝지어 진다.
            data[client_fd] = malloc(sizeof(Data));
            if (data[client_fd] != NULL)
            {
                memset(data[client_fd], 0, sizeof(Data)); // 초기화
            }
            else
            {
                close(client_fd);
                continue;
            }

            FD_SET(client_fd, &readfds); // 클라이언트 소켓을 fd_set에 추가
            if (client_fd > maxfd)
                maxfd = client_fd;

            printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            continue;
        }
        for (i = 0; i <= maxfd; i++)
        {
            if (FD_ISSET(i, &allfds))
            {
                if (read(i, data[i], sizeof(Data)) <= 0)
                {
                    close(i);
                    FD_CLR(i, &readfds);
                    free(data[i]);
                    data[i] = NULL;
                }
                else
                {
                    data[i]->left_num = ntohl(data[i]->left_num);
                    data[i]->right_num = ntohl(data[i]->right_num);
                    if (strcmp(data[i]->str, "quit") == 0)
                    {
                        close(i);
                        FD_CLR(i, &readfds);
                        free(data[i]);
                        data[i] = NULL;
                    }
                    else
                    {
                        strcat(constr, data[i]->str);
                        calculate(data[i], constr);

                        strncpy(data[i]->str, constr, MAXLINE - 1);
                        data[i]->str[MAXLINE - 1] = '\0'; 
                        address[connected_clients] = i;
                        connected_clients++;
                        for (int k = 0; k < connected_clients; k++)
                        {
                            if (data[address[k]] != NULL)
                            {
                                strncpy(data[address[k]]->str, constr, MAXLINE - 1);
                                data[address[k]]->str[MAXLINE - 1] = '\0'; 
                                write(address[k], data[address[k]], sizeof(Data));
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}
