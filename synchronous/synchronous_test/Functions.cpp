#include "stdafx.h"

void	KernelMutexObjectTest()
{
	BOOL	bRet;
	HANDLE	hDevice;
	DWORD	dwRet;
	
	printf("press any key to test driver ...\n");
	_getch();

	hDevice = CreateFile(L"\\\\.\\synchronous",
		GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (INVALID_HANDLE_VALUE == hDevice)
	{
		printf("CreateFile failed : %d\n", GetLastError());
		return;
	}

	bRet = DeviceIoControl(hDevice, IOCTRL_FAST_MUTEX, NULL, 0, NULL, 0, &dwRet, NULL);

	if (!bRet)
	{
		printf("DeviceIoControl failed : %d\n", GetLastError());
		return ;
	}

	CloseHandle(hDevice);
}

DWORD	WINAPI		MyPendingReadFile(LPVOID pContext)
{
	HANDLE			hDevice = *(PHANDLE)pContext;
	DWORD			dwRet;
	BOOL			bRet;
	OVERLAPPED		overlap = {0};
	char			szBur[10] = {0};

	overlap.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	bRet = ReadFile(hDevice, szBur, 10, &dwRet, &overlap);

	if (!bRet && GetLastError() == ERROR_IO_PENDING)
	{
		printf("read operation is being pended...\n");
		WaitForSingleObject(overlap.hEvent, INFINITE);
	}

	return 0;
}

void	AsynchronousTest()
{
	BOOL	bRet;
	HANDLE	hDevice;
	DWORD	dwRet;
	HANDLE	hThread[10];
	DWORD	dwThreadId[10];
	int		i = 0;

	printf("press any key to test driver ...\n");
	_getch();

	hDevice = CreateFile(L"\\\\.\\synchronous",
		GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,
		NULL);

	if (INVALID_HANDLE_VALUE == hDevice)
	{
		printf("CreateFile failed : %d\n", GetLastError());
		return;
	}

	for (i=0; i<10; i++)
	{
		hThread[i] = CreateThread(NULL, 0, MyPendingReadFile, &hDevice, 0, &dwThreadId[i]);
	}

	printf("press any key to complete pended irp...\n");
	_getch();
	CloseHandle(hDevice);
}

void	DpcTest()
{
	BOOL	bRet;
	HANDLE	hDevice;
	DWORD	dwRet;

	printf("press any key to start dpc timer ...\n");
	_getch();

	hDevice = CreateFile(L"\\\\.\\synchronous",
		GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (INVALID_HANDLE_VALUE == hDevice)
	{
		printf("CreateFile failed : %d\n", GetLastError());
		return;
	}

	bRet = DeviceIoControl(hDevice, IOCTRL_START_TIMER, NULL, 0, NULL, 0, &dwRet, NULL);

	if (!bRet)
	{
		printf("DeviceIoControl failed : %d\n", GetLastError());
		return ;
	}

	printf("press any key to stop dpc timer ...\n");
	_getch();

	bRet = DeviceIoControl(hDevice, IOCTRL_STOP_TIMER, NULL, 0, NULL, 0, &dwRet, NULL);

	if (!bRet)
	{
		printf("DeviceIoControl failed : %d\n", GetLastError());
		return ;
	}

	CloseHandle(hDevice);
}