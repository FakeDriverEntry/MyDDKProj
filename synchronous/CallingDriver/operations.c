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
		FILE_READ_ATTRIBUTES|SYNCHRONIZE, 
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
		KdPrint(("begin to read target device synchronously ...\n"));

		ZwReadFile(hDevice, NULL, NULL, NULL, &isb, NULL, 0, NULL, NULL);

		KdPrint(("ZwReadFile synchronously returned ...\n"));
	}

	ZwClose(hDevice);
}

VOID	CompleteTargetDriver_Read(PVOID Context, PIO_STATUS_BLOCK pStatus_Block, ULONG ul)
{
	KdPrint(("TargetDriver has completed the read irp ...\n"));
	KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
}

VOID	UsingIoCompletion()
{
	NTSTATUS			ntStatus;
	HANDLE				hDevice;
	IO_STATUS_BLOCK		isb;
	OBJECT_ATTRIBUTES	oa;
	KEVENT				sync_event;
	UNICODE_STRING		ustrDevName;
	LARGE_INTEGER		liOffset;

	RtlInitUnicodeString(&ustrDevName, L"\\Device\\TargetDevice");

	InitializeObjectAttributes(&oa, &ustrDevName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	liOffset = RtlConvertLongToLargeInteger(0);

	ntStatus = ZwCreateFile(&hDevice,
		FILE_READ_ATTRIBUTES,
		&oa,
		&isb,
		&liOffset,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN_IF,
		0,
		NULL,
		0);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("ZwCreateFile failed ...\n"));
		return;
	}
	
	//	SynchronizationEvent:��һ���ȴ����¼����̷߳��ָ��¼��������״̬ʱ���̻߳����ִ�У��Զ������¼����óɷ�����״̬������Ҳ��֮Ϊ�Զ������¼�
	//	NotificationEvent:��һ��NotificationEvent�¼��������״̬ʱ�����еȴ����¼����߳̽�����ִ�У��¼�״̬��һֱ����Ϊ����״ֱ̬����ȷ����KeResetEvent
	//	����KeClearEvent��Ҳ��Ϊ�ֶ������¼�
	KeInitializeEvent(&sync_event,
		SynchronizationEvent,		
		FALSE);

	ntStatus = ZwReadFile(hDevice,
		NULL, 
		CompleteTargetDriver_Read, 
		&sync_event,
		&isb,
		NULL, 
		0,
		&liOffset, 
		NULL);

	if (ntStatus == STATUS_PENDING)
	{
		KdPrint(("Calling Driver's read irp is being pended...\n"));

		KeWaitForSingleObject(&sync_event, Executive, KernelMode, FALSE, NULL);

		KdPrint(("ZwReadFile's APC routine has set the SynchronizationEvent to signaled status ...\n"));
	}

	ZwClose(hDevice);

}

VOID	UsingFilePointer()
{
	UNICODE_STRING			ustrDevName;
	OBJECT_ATTRIBUTES		oa;
	IO_STATUS_BLOCK			io_status;
	HANDLE					hDevice;
	PFILE_OBJECT			pFileObject;
	NTSTATUS				ntStatus;
	LARGE_INTEGER			liOffset;

	RtlInitUnicodeString(&ustrDevName, L"\\Device\\TargetDevice");

	InitializeObjectAttributes(&oa, &ustrDevName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	liOffset = RtlConvertLongToLargeInteger(0);

	ntStatus = ZwCreateFile(&hDevice,
		FILE_READ_ATTRIBUTES,
		&oa,
		&io_status,
		&liOffset,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN_IF,
		0,
		NULL,
		0);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("failed to get device handle ...\n"));
		return;
	}

	ntStatus = ZwReadFile(hDevice, NULL, NULL, NULL, &io_status, NULL, 0, &liOffset, NULL);

	if (STATUS_PENDING == ntStatus)
	{
		ntStatus = ObReferenceObjectByHandle(hDevice, 
			FILE_READ_DATA,
			*IoFileObjectType,
			KernelMode,
			(PVOID*)&pFileObject,
			NULL);

		if (!NT_SUCCESS(ntStatus))
		{
			KdPrint(("obtain device object by device handle failed : %x\n", ntStatus));
			return;
		}

		KdPrint(("wait for asynchronous read return ...\n"));

		KeWaitForSingleObject(&pFileObject->Event, Executive, KernelMode, FALSE, NULL);

		KdPrint(("asynchronous read has return ...\n"));

		ObDereferenceObject(pFileObject);
	}

	ZwClose(hDevice);

}