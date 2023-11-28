#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
void print_data_hex(const char* prefix, const char* data, int len) {
printf("%s: ", prefix);
for (int i = 0; i < len; i++) {
printf("%02X ", (unsigned char)data[i]);
}
printf("\n");
}
struct cal_data {
        int left_num;
        int right_num;
        char op;
        int result;
        int sign;
        short int error;
};
int main(int argc, char **argv)
{
        int shmid;
        int semid;

        struct cal_data *cal;
        void *shared_memory = NULL;

        struct cal_data data;

        struct sembuf semopen = {0, -1, SEM_UNDO};
        struct sembuf semclose = {0, 1, SEM_UNDO};

        shmid = shmget((key_t)1234, sizeof(struct cal_data), 0666|IPC_CREAT);
        if (shmid == -1)
        {
                return 1;
        }
        semid = semget((key_t)3477, 0, 0666);
        if(semid == -1)
        {
                return 1;
        }

        shared_memory = shmat(shmid, NULL, 0);
        if (shared_memory == (void *)-1)
        {
                return 1;
        }

        cal = (struct cal_data *)shared_memory;
        memset(cal,0x00,sizeof(struct cal_data));
        cal->sign = 0;

        while(1)
        {


                if(semop(semid, &semopen, 1) == -1)
                {
                        return 1;
                }

                if (cal->sign != 1) // data.sign이 1이 아니란 것은 아직 입력을 안한 것이므로 임계 영역 탈출
                        semop(semid,&semclose, 1);
                else { // data.sign이 1일때만 임계영역 들어가서 공유 메모리 읽고 쓰기

                        switch (cal->op) {
                                case '+':
                                        printf("덧셈 연산\n");
                                        cal->result = cal->left_num + cal->right_num;
                                        break;
                                case '-':
                                        printf("뺄셈 연산\n");
                                        cal->result = cal->left_num - cal->right_num;
                                        break;
                                case 'x':
                                        printf("곱셈 연산\n");
                                        cal->result = cal->left_num * cal->right_num;
                                        break;
                                case '/':
                                        printf("나눗셈 연산\n");
                                        if (cal->right_num == 0) {
                                                cal->error = 1; // 나누기 에러
                                        } else {
                                                cal->result = cal->left_num / cal->right_num;
                                        }
                                        break;
                                case '%':
                                        printf("나머지 연산\n");
                                        if (cal->right_num == 0) {
                                                cal->error = 1; // 나머지 연산 에러
                                        } else {
                                                cal->result = cal->left_num % cal->right_num;
                                        }
                                        break;
                                default:
                                        printf(" 올바른 연산자를 사용 하세요\n");
                                        cal->error = 1; // 연산자 에러
                                        break;
                        }
                        cal->sign = 2; // 소비자가 읽을 차례
                        print_data_hex("cal_data", (const char*)cal, sizeof(struct cal_data));
                        semop(semid, &semclose, 1);
                }
        }
        return 1;
}