#include "precomp.h"

//	ʹ��ZwCreateFileͬ�����첽���豸ʱ����������
//	ͬ����DesiredAccess����ΪSYNCHRONIZE����CreateOptionָ��ΪFILE_SYNCHRONOUS_IO_NOALERT����FILE_SYNCHRONOUS_IO_ALERT
//	�첽��DesiredAccess��������ΪSYNCHRONIZE����CreateOption����ָ��ΪFILE_SYNCHRONOUS_IO_NOALERT��FILE_SYNCHRONOUS_IO_ALERT

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