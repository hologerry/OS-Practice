//
//  consumer_unix.c
//  producer-consumer
//
//  Created by Gerry on 26/03/2017.
//  Copyright © 2017 Gao Yue. All rights reserved.
//

#include "header_unix.h"

int main(int argc, char* argv[]) {
    int shmid, semid;
    key_t shm_key, sem_key;
    size_t shm_size;
    char *shm;
    struct timeval currenttime;

    srand(time(NULL));

    sem_key = (key_t)SEM_KEY;

    // Get the semaphore
    if ((semid = semget(sem_key, 3, SXM_ACCESS_RIGHT)) < 0) {
        perror("consumer semget error");
        exit(1);
    }
    shm_key = (key_t)SHM_KEY;
    shm_size = sizeof(shared_data_st);
    // Get the shared memory segment
    if ((shmid = shmget(shm_key, shm_size, SXM_ACCESS_RIGHT)) < 0) {
        perror("shmget error");
        exit(1);
    }
    // Attach the shared memory segment to the address space
    if ((shm = shmat(shmid, NULL, 0)) < 0) {
        perror("shmat error");
        exit(1);
    }

    shared_data_st* shared_data = (shared_data_st*) shm;

    for (int i = 0; i < CON_ITEM; i++) {
        if (p(semid, SEM_FULL) < 0 ) {
            perror("consumer p operation error SEM_FULL");
            exit(1);
        }
        if (p(semid, SEM_MUTEX) < 0) {
            perror("consumer p operation error SEM_MUTEX");
            exit(1);
        }
        
        int product = shared_data->buf[shared_data->out];
        shared_data->buf[shared_data->out] = -1;
        int get_pos = shared_data->out;
        shared_data->out = (shared_data->out+1)%BUFF_SIZE;
        
        if (gettimeofday(&currenttime, NULL) < 0) {
            perror("get current time error");
            exit(1);
        }
        
        int pr_time = currenttime.tv_sec%100;
        char* consumer_id = argv[0];
        printf("time %d: Consumer %s get %d at buffer[%d] for its %dth operation\n",
            pr_time, consumer_id, product, get_pos, i+1);
        printf("current buffer: ");
        for(int i = 0; i < BUFF_SIZE; i++)
            printf("[%d]%s",shared_data->buf[i], i==BUFF_SIZE-1 ? "\n":" ");
        if (v(semid, SEM_MUTEX) < 0) {
            perror("consumer v operation error SEM_MUTEX");
            exit(1);
        }
        if (v(semid, SEM_EMPTY) < 0) {
            perror("consumer v operation error SEM_EMPTY");
            exit(1);
        }

        int sleeptime = rand()%5;
        sleep(sleeptime);
    }
}
