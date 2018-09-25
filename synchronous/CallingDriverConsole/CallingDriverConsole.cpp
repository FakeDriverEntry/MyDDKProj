// CallingDriverConsole.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE		hDevice;
	DWORD		dwRet;
	BOOL		bRet;

	printf("Press Any Key to Load Target Driver ...\n");
	_getch();

	bRet = LoadNTDriver("TargetDriver", ".\\TargetDriver.sys");


	printf("Press Any Key to Load Calling Driver ...\n");
	_getch();

	bRet = LoadNTDriver("CallingDriver", ".\\CallingDriver.sys");

	hDevice = CreateFile(L"\\\\.\\CallingDevice",
		GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	printf("Press Any Key to test synchronize read ...\n");
	_getch();

	bRet = DeviceIoControl(hDevice, IOCTL_USING_SYNCHRONIZE, NULL, 0, NULL, 0, &dwRet, NULL);

	printf("Press Any Key to test using io completion read ...\n");
	_getch();

	bRet = DeviceIoControl(hDevice, IOCTL_USING_IO_COMPLETION, NULL, 0, NULL, 0, &dwRet, NULL);

	printf("Press Any Key to test using file pointer read ...\n");
	_getch();

	bRet = DeviceIoControl(hDevice, IOCTL_USING_FILE_POINTER, NULL, 0, NULL, 0, &dwRet, NULL);

	printf("Press Any Key to unload Calling driver ...\n");
	_getch();

	CloseHandle(hDevice);

	UnloadNTDriver("CallingDriver");

	printf("Press Any Key to unload Target Driver ...\n");
	_getch();

	UnloadNTDriver("TargetDriver");

	return 0;
}

