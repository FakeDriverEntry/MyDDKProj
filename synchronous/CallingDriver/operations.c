#include "precomp.h"

//	使用ZwCreateFile同步和异步打开设备时参数的区别：
//	同步：DesiredAccess设置为SYNCHRONIZE并且CreateOption指定为FILE_SYNCHRONOUS_IO_NOALERT或者FILE_SYNCHRONOUS_IO_ALERT
//	异步：DesiredAccess不能设置为SYNCHRONIZE并且CreateOption不能指定为FILE_SYNCHRONOUS_IO_NOALERT和FILE_SYNCHRONOUS_IO_ALERT

VOID	SynchronizeRead()
{
	NTSTATUS			ntStatus;
	UNICODE_STRING		ustrDevName;
	OBJECT_ATTRIBUTES	oa;
	IO_STATUS_BLOCK		isb;
	HANDLE				hDevice;

	RtlInitUnicodeString(&ustrDevName, L"\\Device\\TargetDevice");

	InitializeObjectAttributes(&oa, &ustrDevName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	ntStatus = ZwCreateFile(&hDevice, 
		FILE_SHARE_READ|SYNCHRONIZE, 
		&oa, 
		&isb, 
		NULL, 
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ, 
		FILE_OPEN_IF,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);

	if (NT_SUCCESS(ntStatus))
	{
		ZwReadFile(hDevice, NULL, NULL, NULL, &isb, NULL, 0, NULL, NULL);
	}

	ZwClose(hDevice);
}