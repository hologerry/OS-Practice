//
//  main_unix.c
//  producer-consumer
//
//  Created by Gerry on 26/03/2017.
//  Copyright © 2017 Gao Yue. All rights reserved.
//

#include "header_unix.h"

char* producer = "producer";
char* consumer = "consumer";
char* accesser;

void init_data(shared_data_st * shared_data) {
    for (int i = 0; i < BUFF_SIZE; i++)
        shared_data->buf[i] = -1;
    shared_data->in = 0;
    shared_data->out = 0;
}

int main(int argc, char* argv[]) {
    char parameters[3]; // put accesser id;
    
    pid_t pid;

    int shmid, semid;
    key_t shm_key, sem_key;
    size_t shm_size;
    char *shm;
    int sem_val;
    
    // Semaphore
    sem_key = (key_t)SEM_KEY;
    // Create 3 semaphores SEM_EMPTY SEM_FULL SEM_MUTEX
    if ((semid = semget(sem_key, 3, SXM_ACCESS_RIGHT|IPC_CREAT|IPC_EXCL)) < 0) {
        perror("semget error");
        exit(1);
    }
    printf("semid: %d\n",semid);
    sem_val = 3;
    if (semctl(semid, SEM_EMPTY, SETVAL, sem_val) < 0) {
        perror("semctl for SEM_EMPTY error");
        exit(1);
    }
    sem_val = 0;
    if (semctl(semid, SEM_FULL, SETVAL, sem_val) < 0) {
        perror("semctl fot SEM_FULL error");
        exit(1);
    }
    sem_val = 1;
    if (semctl(semid, SEM_MUTEX, SETVAL, sem_val) < 0 ) {
        perror("semctl fot SEM_MUTEX error");
        exit(1);
    }

    // Shared Memory
    shm_key = (key_t)SHM_KEY;
    shm_size = sizeof(shared_data_st);
    // Create the shared memory segment
    if ((shmid = shmget(shm_key, shm_size, SXM_ACCESS_RIGHT|IPC_CREAT|IPC_EXCL)) < 0) {
        perror("shmget error");
        exit(1);
    }
    printf("shmid: %d\n",shmid);
    // Attach the shared memory segment to the address space
    if ((shm = shmat(shmid, NULL, 0)) < 0) {
        perror("shmat error");
        exit(1);
    }

    // Init shared memory segment data
    shared_data_st * shared_data = (shared_data_st *) shm;
    init_data(shared_data);

    // Accesser is the producer or consumer
    // Define ID 0-1 is producer, ID 2-4 is consumer, total_ers are the sum
    int total_ers = PRO_NUM + CON_NUM;
    for (int i = 0; i < total_ers; i++) {
        int accesser_id = i;
        accesser = accesser_id < PRO_NUM ? producer : consumer;
        sprintf(parameters, "%d", accesser_id);

        if ((pid = fork()) < 0) {
            perror("fork error");
            exit(1);
        } else if (pid == 0) {
            if (execlp(accesser, parameters) < 0) {
                perror("execute accesser error");
                exit(1);
            }
        }
    }

    // Put at outside of for-loop wait for all process done
    for (int i = 0; i < total_ers; i++) {
        if (wait(NULL) < 0) {
            perror("wait accesser error");
                exit(1);
        }
    }

    if (shmdt(shm) < 0) {
        perror("shmdt error");
        exit(1);
    }
    if (shmctl(shmid, IPC_RMID, 0)) {
        perror("shmctl error");
        exit(1);
    }
    if (semctl(semid, 0, IPC_RMID) < 0) {
        perror("shmctl error");
        exit(1);
    }
    exit(0);
}
