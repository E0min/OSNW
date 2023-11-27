
// 소켓 만들기
// 세마포어 생성
while (1) // 새로운 클라이언트의 반복적인 연결을 수행
    {
        accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);
        // 새로운 클라이언트에서 사용할 공유 메모리 생성
        pid = fork(); // 새로운 클라이언트가 연결되면, 연결과 통신을 분리
        if (pid == 0)
        {
            // 해당 클라이언트의 consumer 생성
            pid = fork(); 
            if (pid == 0)
            {
                while (1) 
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
                    { 
                        
                    }
                }
            }
            // 클라이언트와 연결된 자식 서버 프로세스
            close(listen_fd);                                                                                         
            while (read(client_fd, (char *)&rdata, sizeof(rdata)) > 0)                                                
            {                                                                                                        
                printf("From client %s(%d): %s\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port, rdata.str);

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
/*
먼저 각 클라이언트 마다 컨슈머, 프로듀서가 있어야 한다.

클라이언트에게서 새로운 값이 입력되기 전까지 기존 값을 10초마다 보내야 한다.

*/