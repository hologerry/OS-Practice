//
//  mycp.c
//  mycp
//
//  Created by Gerry on 26/03/2017.
//  Copyright © 2017 Gao. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <string.h>
#include <Aclapi.h>				// for SECURITY_DESCRIPTOR

#define DEBUG 0
//#define PATH_MAX _MAX_PATH
#define BUFF_SIZE 1024
#define ERROR_MAX 100

#define F2F 0
#define F2D 1
#define D2D 2

#define SUCCESS 1
#define FAIL   -1

char error_msg[ERROR_MAX];

const DWORD MASK = 0xFFFFFFFF;    // used to get all SECURITY_INFORMATION bit flag


void oops(char *func_name, char *error)
{
	printf("In function: %s\n", func_name);
	perror(error);
	exit(1);
}

void copy_time(HANDLE hIn, HANDLE hOut)
{
	FILETIME creationTime,lastAccessTime,lastWriteTime;
	if (!GetFileTime(hIn, &creationTime, &lastAccessTime, &lastWriteTime)) {
		sprintf(error_msg, "get file time %s error", hIn);
		oops("copy_time", error_msg);
	}
	if (!SetFileTime(hOut, &creationTime, &lastAccessTime, &lastWriteTime)) {
		sprintf(error_msg, "set file time %s error", hOut);
		oops("copy_time", error_msg);
	}
}


int copy_security_info(const char* source, const char* dest_file)
{
//	GetSecurityInfo(hIn, lpFindFileData.dwFileAttributes, MASK, NULL, NULL, NULL, NULL, pSD);
//	SetSecurityInfo(hOut, lpFindFileData.dwFileAttributes, pSD->Control, pSD->Owner, pSD->Group, pSD->Dacl, pSD->Sacl);
}

int copy_file(const char* source, const char* destination)
{
	HANDLE hSrc, hDest;
	DWORD dw_read, dw_write;
	SECURITY_DESCRIPTOR *pSD;
	char buffer[BUFF_SIZE];
	char src_file[PATH_MAX], dest_file[PATH_MAX];
	
	strcpy(src_file, source);
	strcpy(dest_file, destination);

	WIN32_FIND_DATA lpFindFileData;
	hSrc = CreateFile(src_file, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hSrc == NULL) {
		sprintf(error_msg, "open file %s error", src_file);
		oops("copy_file", error_msg);
	}
	hDest = CreateFile(dest_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDest == NULL) {
		sprintf(error_msg, "create file %s error", dest_file);
		oops("copy_file", error_msg);
	}
	
	memset(buffer, 0, sizeof(buffer));
	while (ReadFile(hSrc, buffer, BUFF_SIZE, &dw_read, NULL) && dw_read > 0) {
		WriteFile(hDest, buffer, dw_read, &dw_write, NULL);
		if (dw_write != dw_read) {
			sprintf(error_msg, "write file %s error", dest_file);
			oops("copy_file", error_msg);
		}
	}

	copy_time(hSrc, hDest);
	
	//copy_security_info(src_file, dest_file);
	
	CloseHandle(hSrc);
	CloseHandle(hDest);

	return SUCCESS;
}

int copy_dir(const char* source, const char* destination)
{
	char src_dir[PATH_MAX], dest_dir[PATH_MAX];
	strcpy(src_dir, source);
	strcpy(dest_dir, destination);
	
	BOOL creat_retval = CreateDirectory(dest_dir, NULL);
	if (!creat_retval) {
		sprintf(error_msg, "creat directory %s error", dest_dir);
		oops("copy_dir", error_msg);
	}
	
	//
	copy_security_info(src_dir, dest_dir);
	
	return creat_retval;
}

void work_path_recursion(const char* source, const char* destination)
{
	WIN32_FIND_DATA lpFindFileData;
	HANDLE hFind;

	char current_dest[PATH_MAX];
	char current_src[PATH_MAX];
	strcpy(current_src, source);
	strcpy(current_dest, destination);
	strcat(current_src, "\\*.*");
	strcat(current_dest, "\\");

	hFind = FindFirstFile(current_src, &lpFindFileData);
	if (hFind == NULL) {
		sprintf(error_msg,"mycp: cannot find %s for copying\n", current_src);
		oops("work_path_recursion", error_msg);
	}
	
	while (FindNextFile(hFind, &lpFindFileData) != 0) {
		if (lpFindFileData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
			if (strcmp(lpFindFileData.cFileName, ".") == 0 || strcmp(lpFindFileData.cFileName, "..") == 0)
				continue;
			if (DEBUG) printf("handling directory: %s\n", lpFindFileData.cFileName);
			strcpy(current_src, source);
			strcat(current_src, "\\");
			strcat(current_src, lpFindFileData.cFileName);
			strcat(current_dest, lpFindFileData.cFileName);
			copy_dir(current_src, current_dest);
			if (DEBUG) printf("entering directory: %s\n", lpFindFileData.cFileName);
			work_path_recursion(current_src, current_dest);
			HANDLE hSrc = CreateFile(current_src, GENERIC_READ, FILE_SHARE_READ, 0, 
							OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
			if (hSrc == INVALID_HANDLE_VALUE) {
				sprintf(error_msg, "open directory %s error", current_src);
				oops("copy_dir", error_msg);
			}
			HANDLE hDest = CreateFile(current_dest, GENERIC_READ|GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
			if (hDest == INVALID_HANDLE_VALUE) {
				sprintf(error_msg, "open created directory %s error", current_dest);
				oops("copy_dir", error_msg);
			}
			copy_time(hSrc, hDest);				// copy time only when get out directory
		} else {
			if (DEBUG) printf("handling file: %s\n", lpFindFileData.cFileName);
			strcpy(current_src, source);
			strcat(current_src, "\\");
			strcat(current_src, lpFindFileData.cFileName);
			strcat(current_dest, lpFindFileData.cFileName);
			if (DEBUG) printf("copying file: %s\n", lpFindFileData.cFileName);
			copy_file(current_src, current_dest);
		}
		strcpy(current_src, source);
		strcat(current_src, "\\");
		strcpy(current_dest, destination);
		strcat(current_dest, "\\");
	}
	return;
}

void mycp(const char* source, const char* destination, int op_type)
{
	WIN32_FIND_DATA lpFindFileDataSrc, lpFindFileDataDest;
	char src[PATH_MAX];
	char dest[PATH_MAX];
	strcpy(src, source);
	strcpy(dest, destination);
	
	HANDLE hSrc = FindFirstFile(src, &lpFindFileDataSrc);
	HANDLE hDest = FindFirstFile(dest, &lpFindFileDataDest);

	if (op_type == F2F) {
		if (copy_file(src, dest)) {
			printf("Copy file %s to file %s succeed\n", src, dest);
		}
		
	}
	else if (op_type == F2D) {
		strcat(dest, "\\");
		strcat(dest, lpFindFileDataSrc.cFileName);
		if (copy_file(src, dest)) {
			printf("Copy file %s to directory %s\\ succeed\n", src, destination);
		}
	}
	else if (op_type == D2D) {
		if (hDest == INVALID_HANDLE_VALUE) {
			printf("copying dir\n");
			copy_dir(src, dest);
		}
		work_path_recursion(src, dest);
		// After all sub- files directorys are copied copy the directory's time
		HANDLE hSrc = CreateFile(src, GENERIC_READ, FILE_SHARE_READ, 0, 
							OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (hSrc == INVALID_HANDLE_VALUE) {
			sprintf(error_msg, "open directory %s error", src);
			oops("mycp", error_msg);
		}
		HANDLE hDest = CreateFile(dest, GENERIC_READ|GENERIC_WRITE, 
			FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (hDest == INVALID_HANDLE_VALUE) {
			sprintf(error_msg, "open created directory %s error", dest);
			oops("mycp", error_msg);
		}
		copy_time(hSrc, hDest);				// copy time only when get out directory
		printf("Copy directory %s\\ to directory %s\\ succeed\n", source, destination);
	}
}

int main(int argc, char* argv[])
{
	if (argc != 3) {
        printf("Usage: mycp source destination\n");
        return 0;
    }
	char source[PATH_MAX];
	char destination[PATH_MAX];
	strcpy(source, argv[1]);
	strcpy(destination, argv[2]);

	WIN32_FIND_DATA lpFindFileDataSrc, lpFindFileDataDest;
	HANDLE hSrc = FindFirstFile(source, &lpFindFileDataSrc);
	HANDLE hDest = FindFirstFile(destination, &lpFindFileDataDest);

	if (hSrc == INVALID_HANDLE_VALUE) {
		oops("main", "Source file(directory) does not exist!");
	}

	if (lpFindFileDataSrc.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
		if (hDest == INVALID_HANDLE_VALUE) {
			printf("Copy File to File.\n");
			mycp(source, destination, F2F);
		} else if (lpFindFileDataDest.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
			printf("Copy File to Directory.\n");
			mycp(source, destination, F2D);
		}
	} else {
		printf("Copy Directory to Directory\n");
		mycp(source, destination, D2D);
	}

	return 0;
}
