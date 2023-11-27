#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAXLINE 1024
#define PORTNUM 3600
pthread_mutex_t m_lock;
void *timer_thread(void *arg);
void *producer(void *arg);
void *consumer(void *arg);

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

int main(int argc, char **argv)
{
    int listen_fd, client_fd;
    socklen_t addrlen;
    struct sockaddr_in client_addr, server_addr;

    pthread_mutex_init(&m_lock, NULL);

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

    printf("Server is running on port %d\n", PORTNUM);

    while (1)
    {
        addrlen = sizeof(client_addr);
        client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (client_fd == -1)
        {
            perror("accept error");
            continue;
        }

        printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        int *new_sock = malloc(sizeof(int));
        if (new_sock == NULL)
        {
            perror("malloc error");
            close(client_fd);
            continue;
        }
        *new_sock = client_fd;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, producer, (void *)new_sock) < 0)
        {
            perror("could not create thread");
            free(new_sock);
            close(client_fd);
            continue;
        }
        pthread_detach(thread_id);
    }

    close(listen_fd);
    return 0;
}

void *producer(void *socket_desc)
{
    int sock = *(int *)socket_desc;
    int read_size;
    Data *rdata = malloc(sizeof(Data));

    if (rdata == NULL)
    {
        perror("malloc error");
        free(socket_desc);
        close(sock);
        return NULL;
    }

    while ((read_size = read(sock, rdata, sizeof(Data))) > 0)
    {
        rdata->left_num = ntohl(rdata->left_num);
        rdata->right_num = ntohl(rdata->right_num);
        rdata->client_fd = sock;
        printf("Received string data: %s\n", rdata->str);

        pthread_t consumer_thread;
        if (pthread_create(&consumer_thread, NULL, consumer, rdata) != 0)
        {
            perror("Could not create consumer thread");
            continue;
        }

        pthread_join(consumer_thread, NULL);
    }

    if (read_size == 0)
    {
        puts("Client disconnected");
    }
    else if (read_size == -1)
    {
        perror("read failed");
    }

    free(rdata);
    free(socket_desc);
    close(sock);
    return NULL;
}

void *consumer(void *data_arg)
{
    Data *data_ptr = (Data *)data_arg;

    pthread_mutex_lock(&m_lock);
    // 데이터 처리 로직 (예시: 사칙연산)
    switch (data_ptr->op)
    {
    case '+':
        data_ptr->result = htonl(data_ptr->left_num + data_ptr->right_num);
        break;
    case '-':
        data_ptr->result = htonl(data_ptr->left_num - data_ptr->right_num);
        break;
    case '*':
        data_ptr->result = htonl(data_ptr->left_num * data_ptr->right_num);
        break;
    case '/':
        data_ptr->result = htonl(data_ptr->left_num / data_ptr->right_num);
        break;
        // 기타 연산자 처리...
    }
    data_ptr->left_num = htonl(data_ptr->left_num);
    data_ptr->right_num = htonl(data_ptr->right_num);

    pthread_mutex_unlock(&m_lock);


    // 결과 전송
    pthread_t timer_tid;
    if (pthread_create(&timer_tid, NULL, timer_thread, data_ptr) != 0) {
        perror("Could not create timer thread");
        // 오류 처리
    }
    
    return NULL;
}

void *timer_thread(void * data_arg) {
    Data *data_ptr = (Data *)data_arg;
    struct tm *timeinfo;
    time_t rawtime;

    while (1) {
        sleep(10);  // 10초 동안 대기

        time(&rawtime);
        timeinfo = localtime(&rawtime);
        write(data_ptr->client_fd, data_ptr,sizeof(Data));
        write(data_ptr->client_fd, timeinfo, sizeof(struct tm));
    }

    return NULL;
}