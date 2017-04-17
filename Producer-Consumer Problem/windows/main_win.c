//
//  main_win.c
//  producer-consumer
//
//  Created by Gerry on 27/03/2017.
//  Copyright © 2017 Gao Yue. All rights reserved.
//

#include "header_win.h"

char* producer = "producer.exe";
char* consumer = "consumer.exe";

void init_data(shared_data_st* shared_data) {
	for (int i = 0; i < BUFF_SIZE; i++) {
		shared_data->buf[i] = -1;
	}
	shared_data->in = 0;
	shared_data->out = 0;
}

PROCESS_INFORMATION StartClone(int count)
{
	char* accesser = count < PRO_NUM ? producer : consumer;
	TCHAR szCmdLine[MAX_PATH];
	sprintf(szCmdLine, "%s %d", accesser, count);

	STARTUPINFO si;			
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));

	// Create process
	BOOL bCreateOK = CreateProcess(NULL, szCmdLine, 
		NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi);
	return pi;
}

int main(int argc, char* argv[]) {
	PROCESS_INFORMATION subProcesses[PRO_NUM+CON_NUM];

	HANDLE SEM_EMPTY;					// keep track of the number of empty spots
	HANDLE SEM_FULL;					// keep track of the number of full spots
	HANDLE SEM_MUTEX;					// mutex

	// Create semaphores
	SEM_EMPTY = CreateSemaphore(NULL, BUFF_SIZE, MAX_SEM_VALUE, EMPTY);
	SEM_FULL = CreateSemaphore(NULL, 0, MAX_SEM_VALUE, FULL);
	SEM_MUTEX = CreateSemaphore(NULL, 1, MAX_MUTEX_VAL, MUTEX);

	HANDLE hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,			// A handle to the file from which to create a file mapping object.
		NULL,							// A pointer to a SECURITY_ATTRIBUTES structure
		PAGE_READWRITE,					// Specifies the page protection of the file mapping object. 
		0,								// The high-order DWORD of the maximum size of the file mapping object.
		sizeof(shared_data_st),			// The low-order DWORD of the maximum size of the file mapping object.
		shmName);						// The name of the file mapping object.
	if (hMapFile == NULL) {
		perror("CreateFileMapping error.\n");
		exit(-1);
	}

	LPVOID pBuffer = MapViewOfFile(
		hMapFile,						// A handle to a file mapping object.
		FILE_MAP_ALL_ACCESS,			// The type of access to a file mapping object
		0,								// A high-order DWORD of the file offset where the view begins.
		0,								// A low-order DWORD of the file offset where the view is to begin.
		sizeof(shared_data_st));		// The number of bytes of a file mapping to map to the view.

	shared_data_st* shared_data = (shared_data_st*) pBuffer;
	init_data(shared_data);

	// Accesser is the producer or consumer
	// Define ID 0-1 is producer, ID 2-4 is consumer, total_ers are the sum
	int total_ers = PRO_NUM + CON_NUM;
	for (int i = 0; i < total_ers; i++) {
		subProcesses[i] = StartClone(i);
	}
	// Wait for all sub process
	for (int i = 0; i < total_ers; i++) {
		WaitForSingleObject(subProcesses[i].hProcess, INFINITE);
	}
	// Close sub process handle
	for (int i = 0; i < total_ers; i++)
	{
		CloseHandle(subProcesses[i].hProcess);
		CloseHandle(subProcesses[i].hThread);
	}
	// Semaphore handle
	CloseHandle(SEM_MUTEX);
	CloseHandle(SEM_EMPTY);
	CloseHandle(SEM_FULL);
	CloseHandle(pBuffer);

	return 0;
}