#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
struct cal_data
{
        int left_num;
        int right_num;
        char op;
        int result;
        int sign;
        short int error;
};
void print_data_hex(const char* prefix, const char* data, int len) {
        printf("%s: ", prefix);
        for (int i = 0; i < len; i++) {
                printf("%02X ", (unsigned char)data[i]);
        }
        printf("\n");
}
union semun
{
        int val;
};
int main(int argc, char **argv)
{
        int shmid;
        int semid;

        struct cal_data *cal;
        void *shared_memory = NULL;
        union semun sem_union;
        struct sembuf semopen = {0, -1, SEM_UNDO};
        struct sembuf semclose = {0, 1, SEM_UNDO};
        struct cal_data sdata;
        shmid = shmget((key_t)1234, sizeof(struct cal_data), 0666);
        if (shmid == -1)
        {
                perror("shmget failed : ");
                exit(0);
        }

        semid = semget((key_t)3477, 1, IPC_CREAT|0666);
        if(semid == -1)
        {
                perror("semget failed : ");
                return 1;
        }

        shared_memory = shmat(shmid, NULL, 0);
        if (shared_memory == (void *)-1)
        {
                perror("shmat failed : ");
                exit(0);
        }

        cal = (struct cal_data *)shared_memory;
        memset(cal,0x00,sizeof(struct cal_data));
        cal->sign = 0;
        sem_union.val = 1;

        if ( -1 == semctl( semid, 0, SETVAL, sem_union))
        {
                return 1;
        }
        while(1){
                if(semop(semid, &semopen, 1) == -1) // 잠구기
                {
                        return 1;
                }

                if (cal->sign != 0)
                        semop(semid, &semclose , 1);
                else{
                        printf("입력 형식: num1 num2 op\n");
                        scanf("%d %d %c", &sdata.left_num, &sdata.right_num, &sdata.op);
                        if (sdata.op == '$')
                                break;

                        cal->left_num = sdata.left_num;
                        cal->right_num = sdata.right_num;
                        cal->op = sdata.op;
                        cal->sign  = 1; // 읽을 데이터가 있으니 1로 바꿔준다.
                        print_data_hex("cal_data", (const char*)cal, sizeof(struct cal_data));
                        semop(semid, &semclose, 1); // 열기
                }

                if(semop(semid, &semopen, 1) == -1) // 잠구기
                {
                        return 1;
                }

                if (cal->sign != 2) // sign값이 2가 아니면 임계 구역 탈출
                        semop(semid , &semclose , 1);
                else{
                        sdata.error = cal->error;
                        sdata.result = cal->result;
                        cal->sign = 0;


                        if (sdata.error == 1)
                                printf("나머지 혹은 나누기  연산시 0으로 나누면 안됩니다\n");
                        else
                                printf("%d %c %d = %d\n", sdata.left_num, sdata.op, sdata.right_num, sdata.result);

                        print_data_hex("cal_data", (const char*)cal, sizeof(struct cal_data));
                        semop(semid, &semclose, 1); // 열기
                }

        }

        return 1;
}