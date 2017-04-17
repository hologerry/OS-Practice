//
//  header_unix.h
//  producer-consumer
//
//  Created by Gerry on 26/03/2017.
//  Copyright © 2017 Gao Yue. All rights reserved.
//

#ifndef __HEADER_H
#define __HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define BUFF_SIZE 3             // total number of slots
#define PRO_NUM   2             // total number of producers
#define PRO_ITEM  6             // number of items produced per producer
#define CON_NUM   3             // total number of consumers
#define CON_ITEM  4             // number of items consumed per consumer

#define SHM_KEY 555
#define SEM_KEY 333
#define SXM_ACCESS_RIGHT 0666   // shm sem access permissions

#define SEM_EMPTY 0             // SEM_EMPTY id keep track of the number of empty spots
#define SEM_FULL  1             // SEM_FULL id keep track of the number of full spots
#define SEM_MUTEX 2             // SEM_MUTEX id

typedef struct {
    int buf[BUFF_SIZE];         // shared var
    int in;                     // buf[in%BUFF_SIZE] is the first empty slot
    int out;                    // buf[out%BUFF_SIZE] is the first full slot
} shared_data_st;

int p(int sem_id, int sem_num) {
    int status;
    struct sembuf sem_buf;
    sem_buf.sem_num = sem_num;
    sem_buf.sem_op = -1;
    sem_buf.sem_flg = 0;
    if ((status = semop(sem_id, &sem_buf, 1)) < 0) {
        perror("p operation semop error");
    }
    return status;
}

int v(int sem_id, int sem_num) {
    int status;
    struct sembuf sem_buf;
    sem_buf.sem_num = sem_num;
    sem_buf.sem_op = 1;
    sem_buf.sem_flg = 0;
    if ((status = semop(sem_id, &sem_buf, 1)) < 0) {
        perror("v operation semop error");
    }
    return status;
}

#endif
