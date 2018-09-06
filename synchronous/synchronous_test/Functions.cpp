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