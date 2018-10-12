// ReadTargetDevice.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE		hDevice = NULL;
	BOOL		bRet = FALSE;


	printf("press any key to install target driver ...\n");
	_getch();

	bRet = LoadNTDriver("TargetDriver", ".\\TargetDriver.sys");

	printf("press any key to install filter driver ...\n");
	_getch();

	bRet = LoadNTDriver("FilterDriver", ".\\FilterDriver.sys");

	printf("press any key to transfer IRP_MJ_CREATE to target device ...\n");
	_getch();

	hDevice = CreateFileA("\\\\.\\TargetDevice",
		GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("CreateFile failed : %d\n", GetLastError());
		return 1;
	}

	printf("press any key to transfer IRP_MJ_READ to target device ...\n");
	_getch();

	bRet = ReadFile(hDevice, NULL, 0, NULL, NULL);

	printf("press any key to transfer IRP_MJ_CLOSE to target device ...\n");
	_getch();

	CloseHandle(hDevice);

	printf("press any key to uninstall filter deriver ...\n");
	_getch();

	UnloadNTDriver("FilterDriver");

	printf("press any key to uninstall target deriver ...\n");
	_getch();

	UnloadNTDriver("TargetDriver");

	
	return 0;
}

