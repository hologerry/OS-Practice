//
//  memory_monitor.cpp
//  MemoryMonitor
//
//  Created by Gerry on 16/04/2017.
//  Copyright © 2017 Gao. All rights reserved.
//

#include <iostream>
#include <Windows.h>
#include <WinBase.h>
#include <Shlwapi.h>
#include <stdio.h>
#include <TlHelp32.h>
#include <Psapi.h>


using namespace std;

// for main() user operation type
#define EXT 0               // EXT for exit
#define SYS 1               // SYS for get system configuration info
#define MEM 2               // MEM for get memory info including status of use
#define PRO 3               // PRO for get running processes

// for mymonitor return value
#define OK   0
#define EXIT 1
#define ERR  2

// Use to convert bytes to KB
#define DIV 10

// cout or printf width
#define ADR_WIDTH 8
#define MEM_WIDTH 7
#define SZE_WIDTH 6

#define ERR_MAX 100
#define STR_MAX 50

// for print error message
char error_msg[ERR_MAX];

void oops(char *func_name, char *error)
{
    printf("In function: %s\n",func_name);
    perror(error);
    exit(1);
}

void format_size(DWORD numeric, char * buf, int buf_size)
{
	int pwr = 0;
	if (numeric < 0) {
		return;
	} else {
		while (numeric > (1<<DIV)) {
			numeric = numeric >> DIV;
			pwr ++;
		}
	}
	switch(pwr) {
	case 0:
		sprintf(buf, "%dB", numeric);
		break;
	case 1:
		sprintf(buf, "%dKB", numeric);
		break;
	case 2:
		sprintf(buf, "%dMB", numeric);
		break;
	case 3:
		sprintf(buf, "%dGB", numeric);
		break;
	}
	return;
}

void print_prompt()
{
    cout << "Please select operation type:" << endl;
    cout << EXT << " --- Exit program." << endl;
    cout << SYS << " --- Get system info." << endl;
    cout << MEM << " --- Get memory info." << endl;
    cout << PRO << " --- Get all running processes info." << endl;
}

void get_processor_arch(char * arch, int arch_num, int buf_size)
{
	switch (arch_num)
	{
	case 9:
		sprintf(arch, "x64 (AMD or Intel)");
		break;
	case 5:
		sprintf(arch, "ARM");
		break;
	case 6:
		sprintf(arch, "Intel Itanium-based");
		break;
	case 0:
		sprintf(arch, "x86");
		break;
	default:
		sprintf(arch, "Unknown architecture");
		break;
	}
}

void show_protection(DWORD protection) {
	switch (protection)
	{
	case PAGE_GUARD:
		cout << "PAGE_GUARD";
		break;
	case PAGE_EXECUTE:
		cout << "PAGE_EXECUTE";
		break;
	case PAGE_EXECUTE_READ:
		cout << "PAGE_EXECUTE_READ";
		break;
	case PAGE_EXECUTE_READWRITE:
		cout << "PAGE_EXECUTE_READWRITE";
		break;
	case PAGE_EXECUTE_WRITECOPY:
		cout << "PAGE_EXECUTE_WRITECOPY";
		break;
	case PAGE_NOACCESS:
		cout << "PAGE_NOACCESS";
		break;
	case PAGE_READONLY:
		cout << "PAGE_READONLY";
		break;
	case PAGE_READWRITE:
		cout << "PAGE_READWRITE";
		break;
	case PAGE_WRITECOPY:
		cout << "PAGE_WRITECOPY";
		break;
	case PAGE_NOCACHE:
		cout << "PAGE_NOCACHE";
		break;
	}
}

void get_system_info()
{
    // Get System info
    SYSTEM_INFO si;
	DWORD dwMemSize;
    
	ZeroMemory(&si, sizeof(si));
    GetSystemInfo(&si);

    // Converts a numeric value into a string (KB MB GB)
    char page_size[STR_MAX];
	format_size(si.dwPageSize, page_size, STR_MAX);

	// Convert numeric processor architecture to string
	char arch[STR_MAX];
	get_processor_arch(arch, si.wProcessorArchitecture, STR_MAX);
   
	char mem_size[STR_MAX];
	dwMemSize = (DWORD)si.lpMaximumApplicationAddress - (DWORD)si.lpMinimumApplicationAddress;
	format_size(dwMemSize, mem_size, STR_MAX);
    
	// Print info
	cout << endl << "System info:" << endl;
	cout << "Processor architecture: " << arch << endl;
	cout << "Number of processors: " << si.dwNumberOfProcessors << endl;
	cout << "Virtual memory page size: " << page_size << endl;
    cout << "Minimum application address: 0x";
    cout.fill('0');
	cout.width(ADR_WIDTH);
    cout << hex << (DWORD)si.lpMinimumApplicationAddress << endl;
    cout << "Maximum application address: 0x";
    cout << hex << (DWORD)si.lpMaximumApplicationAddress << endl;
    cout << "Total available virtual memory: " << mem_size << endl << endl << endl;
}

void get_memory_info()
{
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);

	GlobalMemoryStatusEx(&statex);
	cout << endl << "Memory info:" << endl;
	printf("There is  %*ld percent of memory in use.\n", 
		MEM_WIDTH, statex.dwMemoryLoad);
	printf("There are %*lld total KB of physical memory.\n", 
		MEM_WIDTH, statex.ullTotalPhys>>DIV);
	printf("There are %*lld free  KB of physical memory.\n", 
		MEM_WIDTH, statex.ullAvailPhys>>DIV);
	printf("There are %*lld total KB of paging file.\n", 
		MEM_WIDTH, statex.ullTotalPageFile>>DIV);
	printf("There are %*lld free  KB of paging file.\n", 
		MEM_WIDTH, statex.ullAvailPageFile>>DIV);
	printf("There are %*lld total KB of virtual memory.\n", 
		MEM_WIDTH, statex.ullTotalVirtual>>DIV);
	printf("There are %*lld free  KB of virtual memory.\n", 
		MEM_WIDTH, statex.ullAvailVirtual>>DIV);

	// Show the amount of extended memory available.
	printf("There are %*lld free  KB of extended memory.\n",
		MEM_WIDTH, statex.ullAvailExtendedVirtual>>DIV);

	cout << endl << endl;
}

void get_processes_info()
{
	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;
	DWORD dwPriorityClass;

	pe32.dwSize = sizeof(pe32);

	// if the first parameter is TH32CS_SNAPPROCESS, second is been ignored. via MSDN
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) {
		sprintf(error_msg, "CreateToolHelp32Snapshot error");
		oops("get_processes_info", error_msg);
	}
	// Retrieves information about the first process encountered in a system snapshot.
	if (!Process32First(hProcessSnap, &pe32))
	{
		sprintf(error_msg, "Process32First error");
		CloseHandle(hProcessSnap);          // clean the snapshot object
		oops("get_processes_info", error_msg);
	}

	// Now walk the snapshot of processes, and
	// display information about each process in turn
	cout << "\n\nAll Processes:";
	int count = 0;
	do
	{
		printf("\n=============================================================");
		printf("\n%d PROCESS NAME:  %S", count++, pe32.szExeFile);
		printf("\n-------------------------------------------------------------");

		printf("\n  Process ID        = %d", pe32.th32ProcessID);
		printf("\n  Thread count      = %d", pe32.cntThreads);
		printf("\n  Parent process ID = %d", pe32.th32ParentProcessID);
		printf("\n  Priority base     = %d", pe32.pcPriClassBase);

	} while (Process32Next(hProcessSnap, &pe32));
	cout << endl;
	CloseHandle(hProcessSnap);
}

void walk_vm(int processID)
{
	SYSTEM_INFO si;
	MEMORY_BASIC_INFORMATION mbi;
	LPVOID lpBaseAddress;
	LPVOID lpEndAddress;
	HANDLE hProcess;
	PSIZE_T lpMinimumWorkingSetSize = 0;
	PSIZE_T lpMaximumWorkingSetSize = 0;
	int n_bytes;
	char sizeStr[STR_MAX];
	TCHAR fileName[MAX_PATH];

	ZeroMemory(fileName, MAX_PATH);
	
	ZeroMemory(&si, sizeof(si));
	GetSystemInfo(&si);

	ZeroMemory(&mbi, sizeof(mbi));
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, processID);

	cout << "\n\n=============================================================\n";
	cout << "Whole memory address space info of process " << processID << ":";
	
	lpBaseAddress = si.lpMinimumApplicationAddress;
	while (lpBaseAddress < si.lpMaximumApplicationAddress) {
		if (! VirtualQueryEx(hProcess, lpBaseAddress, &mbi, sizeof(mbi)) ) {
			sprintf(error_msg, "VirtualQueryEx error");
			oops("walk_vm", error_msg);
		}
		lpEndAddress = (PBYTE)lpBaseAddress + mbi.RegionSize;
		format_size(mbi.RegionSize, sizeStr, STR_MAX);
		cout << "\n-------------------------------------------------------------\n";
		cout << "BLOCK INFO:\n";
		// Print base address, end address and block size
		cout << "0x";
		cout.fill('0');
		cout.width(ADR_WIDTH);
		cout << hex << (DWORD)lpBaseAddress;
		cout << " - ";
		cout << "0x";
		cout.fill('0');
		cout.width(ADR_WIDTH);
		cout << hex << (DWORD)lpEndAddress;
		printf("   Size: %*s", SZE_WIDTH, sizeStr);

		cout << "\nState: ";
		switch (mbi.State) {
		case MEM_COMMIT:
			cout << "Committed";
			break;
		case MEM_FREE:
			cout << "Free";
			break;
		case MEM_RESERVE:
			cout << "Reserved";
			break;
		}
		if (mbi.Protect == 0 && mbi.State != MEM_FREE) {
			mbi.Protect == PAGE_READONLY;
		}
		if (mbi.State != MEM_RESERVE) {
			cout << "   Protection: ";
			show_protection(mbi.Protect);
		}
		if (mbi.State != MEM_FREE) {
			cout << "   Type: ";
			switch (mbi.Type) {
			case MEM_IMAGE:
				cout << "Image";
				break;
			case MEM_MAPPED:
				cout << "Mapped";
				break;
			case MEM_PRIVATE:
				cout << "Private";
				break;
			}
		}
		if (GetModuleFileName((HMODULE)lpBaseAddress, fileName, MAX_PATH)) {
			printf("\nModule: %s", fileName);
		}

		lpBaseAddress = lpEndAddress;
	}
	cout << "\n-------------------------------------------------------------\n\n";
	CloseHandle(hProcess);
}

int mymonitor(int op_type)
{
    switch(op_type) {
	case EXT:
		return EXIT;
		break;
	case SYS:
		get_system_info();
		return OK;
		break;
	case MEM:
		get_memory_info();
		return OK;
		break;
	case PRO:
		get_processes_info();
		int pid;
		cout << endl << 
			"Please input process's PID to get its virtual address space:" << endl;
		cin >> pid;
		while (!cin.good()) {
			cin.clear();
			cin.ignore(10000, '\n');
			cout << "Please input valid value!" << endl;
			cin >> pid;
		}
		walk_vm(pid);
		return OK;
		break;
    }
	return ERR;
}

int main()
{
    int op_type = -1;
    while(true)
    {
        print_prompt();
        cin >> op_type;
		if (!cin.good()) {
			cin.clear();
			cin.ignore(10000, '\n');
			cout << "Please input valid value!" << endl;
			continue;
		}
        switch(mymonitor(op_type)) {
            case OK:
                break;
            case EXIT:
                return 0;
                break;
            case ERR:
                cout << "Please input valid value!" << endl;
                break;
        }
    }
	return 0;
}
