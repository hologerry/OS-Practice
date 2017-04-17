//
//  mytime_win.cpp
//  mytime
//
//  Created by Gerry on 16/03/2017.
//  Copyright © 2017 Gao Yue. All rights reserved.
//

#include <iostream>
#include <windows.h>
#include <cstdio>

using namespace std;


void print_time(SYSTEMTIME start, SYSTEMTIME end)
{
	int hour = end.wHour - start.wHour,
		minute = end.wMinute - start.wMinute,
		second = end.wSecond - start.wSecond,
		milliSecond = end.wMilliseconds - start.wMilliseconds;
	second = milliSecond<0 ? second - 1 : second;
	milliSecond = milliSecond<0 ? milliSecond + 1000 : milliSecond;

	minute = second<0 ? minute - 1 : minute;
	second = second<0 ? second + 60 : second;

	hour = minute<0 ? hour - 1 : hour;
	minute = minute<0 ? minute + 60 : minute;

	cout << "times: " << hour << "h" << minute << "m" << second << "s" << milliSecond << "ms" << endl;
}

void mytime(string cmd)
{
	SYSTEMTIME start, end;

	TCHAR szFilename[MAX_PATH];

	copy(cmd.begin(), cmd.end(), szFilename);
	szFilename[cmd.size()] = 0;

	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi;

	GetSystemTime(&start);

	// Create subprocess
	BOOL bCreateOK = CreateProcess(szFilename, NULL, NULL, NULL, FALSE,
		CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);

	// Close process and thread handles.
	if (bCreateOK) {               // Create success
		WaitForSingleObject(pi.hProcess, INFINITE);

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		GetSystemTime(&end);
		print_time(start, end);
	} else {
		cout << "Create child process failed." << endl;
	}
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		printf("Usage: %s [cmdline]\n", argv[0]);
		return 0;
	}
	mytime(argv[1]);
	
	return 0;
}