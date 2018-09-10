// synchronous_test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE			hDevice = INVALID_HANDLE_VALUE;
	DWORD			dwRet = 0;
	BOOL			bRet = FALSE;
	OVERLAPPED		overlap;

	printf("press any key to install driver ...\n");
	_getch();

	bRet = LoadNTDriver("synchronous", ".\\synchronous.sys");

	if (!bRet)
	{
		printf("LoadNTDriver failed ...\n");
		return -1;
	}

	KernelMutexObjectTest();

	AsynchronousTest();

	printf("press any key to uninstall device...\n");
	_getch();

	UnloadNTDriver("synchronous");
	
	return 0;
}

