//
//  consumer_win.c
//  producer-consumer
//
//  Created by Gerry on 27/03/2017.
//  Copyright © 2017 Gao Yue. All rights reserved.
//

#include "header_win.h"

int main(int argc, char* argv[]) {
	HANDLE SEM_FULL;
	HANDLE SEM_EMPTY;
	HANDLE SEM_MUTEX;
	HANDLE hMapFile;
	shared_data_st* shared_data;
	SYSTEMTIME currenttime;
	int consumer_id = 0;

    srand(time(NULL));

	if (argc > 1) {
		sscanf(argv[1], "%d", &consumer_id);
	}

	// Get MapFile
	hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, shmName);
	if (NULL == hMapFile) {
		perror("OpenFileMapping error.");
		exit(-1);
	}
	LPVOID pBuffer = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(shared_data_st));
	shared_data = (shared_data_st*)pBuffer;

	// Get Semaphore handles
	SEM_EMPTY = OpenSemaphore(SEMAPHORE_ALL_ACCESS, NULL, EMPTY);
	SEM_FULL = OpenSemaphore(SEMAPHORE_ALL_ACCESS, NULL, FULL);
	SEM_MUTEX = OpenSemaphore(SEMAPHORE_ALL_ACCESS, NULL, MUTEX);

	for (int i = 0; i < CON_ITEM; i++) {
		WaitForSingleObject(SEM_FULL, INFINITE);
		WaitForSingleObject(SEM_MUTEX, INFINITE);

		// Get product
		int product = shared_data->buf[shared_data->out];
		shared_data->buf[shared_data->out] = -1;
		int get_pos = shared_data->out;
		shared_data->out = (shared_data->out + 1) % BUFF_SIZE;

		GetSystemTime(&currenttime);
		int pr_time = currenttime.wSecond;
		printf("time %d: Consumer %d get %d at buffer[%d] for its %dth operation\n",
			pr_time, consumer_id, product, get_pos, i + 1);
		printf("current buffer: ");
        for(int i = 0; i < BUFF_SIZE; i++)
            printf("[%d]%s",shared_data->buf[i], i==BUFF_SIZE-1 ? "\n":" ");

		// ReleaseSemaphore
		ReleaseSemaphore(SEM_MUTEX, 1, NULL);
		ReleaseSemaphore(SEM_EMPTY, 1, NULL);

		int sleeptime = rand() % 2000 + 3000;
		Sleep(sleeptime);
	}
}
