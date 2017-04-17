//
//  header_win.h
//  producer-consumer
//
//  Created by Gerry on 27/03/2017.
//  Copyright © 2017 Gao Yue. All rights reserved.
//

#ifndef __HEADER_H
#define	__HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>

#define BUFF_SIZE 3             // total number of slots
#define PRO_NUM   2             // total number of producers
#define PRO_ITEM  6             // number of items produced per producer
#define CON_NUM   3             // total number of consumers
#define CON_ITEM  4             // number of items consumed per consumer

#define MAX_SEM_VALUE	3		// max value of sync semaphore
#define MAX_MUTEX_VAL	1		// max value of mutex semaphore

#define EMPTY	"EMPTY"
#define FULL	"FULL"
#define MUTEX	"MUTEX"
#define shmName "SharedMemory"

typedef struct {
	int buf[BUFF_SIZE];         // shared var
	int in;                     // buf[in%BUFF_SIZE] is the first empty slot
	int out;                    // buf[out%BUFF_SIZE] is the first full slot
} shared_data_st;

#endif