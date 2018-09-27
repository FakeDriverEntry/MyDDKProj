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
	
	//	SynchronizationEvent:当一个等待该事件的线程发现该事件变成受信状态时，线程会继续执行，自动将该事件设置成非受信状态，所以也称之为自动重置事件
	//	NotificationEvent:当一个NotificationEvent事件变成受信状态时，所有等待该事件的线程将继续执行，事件状态将一直保持为受信状态直到明确调用KeResetEvent
	//	或者KeClearEvent，也称为手动重置事件
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