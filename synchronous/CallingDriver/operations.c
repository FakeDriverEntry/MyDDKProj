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

VOID	UsingSymbolicLinkOpenDevice()
{
	UNICODE_STRING			ustrSymName;
	UNICODE_STRING			ustrDevName;
	OBJECT_ATTRIBUTES		oaSym;
	OBJECT_ATTRIBUTES		oaDev;
	HANDLE					hDevice;
	HANDLE					hSymbolicLink;
	NTSTATUS				ntStatus;
	ULONG					ulDevNameLen;
	IO_STATUS_BLOCK			ioStatus;
	LARGE_INTEGER			liOffset;

	RtlInitUnicodeString(&ustrSymName, L"\\??\\TargetDevice");

	InitializeObjectAttributes(&oaSym, &ustrSymName, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);

	ntStatus = ZwOpenSymbolicLinkObject(&hSymbolicLink, FILE_ALL_ACCESS, &oaSym);

	if (ntStatus != STATUS_SUCCESS)
	{
		KdPrint(("ZwOpenSymbolicLinkObject failed : %x\n", ntStatus));
		return;
	}

	ustrDevName.Buffer = ExAllocatePool(PagedPool, DEV_NAME_LEN);
	ustrDevName.Length = ustrSymName.MaximumLength = DEV_NAME_LEN;

	ntStatus = ZwQuerySymbolicLinkObject(hSymbolicLink, &ustrDevName, &ulDevNameLen);

	if (ntStatus != STATUS_SUCCESS)
	{
		KdPrint(("ZwQuerySymbolicLinkObject failed : %x\n"));
		return;
	}

	InitializeObjectAttributes(&oaDev, &ustrDevName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	ntStatus = ZwCreateFile(&hDevice, 
		FILE_READ_ATTRIBUTES|SYNCHRONIZE,
		&oaDev,
		&ioStatus,
		NULL, 
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN_IF,
		FILE_SYNCHRONOUS_IO_NONALERT, 
		NULL, 
		0);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("ZwCreateFile failed : %x\n", ntStatus));
		return;
	}

	liOffset = RtlConvertLongToLargeInteger(0);

	KdPrint(("begin to synchronously read target device ...\n"));

	ntStatus = ZwReadFile(hDevice, NULL, NULL, NULL, &ioStatus, NULL, 0, &liOffset, NULL);

	KdPrint(("synchronously read target has finished ...\n"));

	return;
	
}

NTSTATUS	BuildSyncIrp()
{
	PIRP				pNewIrp;
	PDEVICE_OBJECT		pDevObj;
	PFILE_OBJECT		pFileObject;
	UNICODE_STRING		ustrDevName;
	NTSTATUS			ntStatus;
	KEVENT				event;
	IO_STATUS_BLOCK		ioStatus;
	LARGE_INTEGER		liOffset;
	PIO_STACK_LOCATION	pNextStack;

	RtlInitUnicodeString(&ustrDevName, L"\\Device\\TargetDevice");

	ntStatus = IoGetDeviceObjectPointer(&ustrDevName, FILE_ALL_ACCESS, &pFileObject, &pDevObj);

	if (ntStatus != STATUS_SUCCESS)
	{
		KdPrint(("IoGetDeviceObjectPointer failed : %x\n", ntStatus));
		return ntStatus;
	}

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	liOffset = RtlConvertLongToLargeInteger(0);

	pNewIrp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
		pDevObj,
		NULL,
		0,
		&liOffset,
		&event,
		&ioStatus);

	if (pNewIrp == NULL)
	{
		KdPrint(("IoBuildSynchronousFsdRequest failed ...\n"));
		return STATUS_UNSUCCESSFUL;
	}

	pNextStack = IoGetNextIrpStackLocation(pNewIrp);

	pNextStack->FileObject = pFileObject;

	ntStatus = IoCallDriver(pDevObj, pNewIrp);

	if (ntStatus == STATUS_PENDING)
	{
		KdPrint(("IRP_MJ_READ is being pended ...\n"));

		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

		KdPrint(("IRP_MJ_READ has been completed ...\n"));

		ntStatus = ioStatus.Status;
	}

	ObDereferenceObject(pFileObject);

	return ntStatus;
}

VOID	BuildAsyncIrp()
{
	NTSTATUS			ntStatus;
	PIRP				pNewIrp;
	PFILE_OBJECT		pFileObject;
	PDEVICE_OBJECT		pDeviceObject;
	LARGE_INTEGER		liOffset;
	KEVENT				my_event;
	PIO_STACK_LOCATION	pNextStack;
	UNICODE_STRING		ustrDevName;
	IO_STATUS_BLOCK		ioStatus;

	RtlInitUnicodeString(&ustrDevName, L"\\Device\\TargetDevice");

	ntStatus = IoGetDeviceObjectPointer(&ustrDevName, FILE_ALL_ACCESS, &pFileObject, &pDeviceObject);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("IoGetDeviceObjectPointer failed : %x\n", ntStatus));
		return;
	}

	KeInitializeEvent(&my_event, NotificationEvent, FALSE);

	liOffset = RtlConvertLongToLargeInteger(0);

	pNewIrp = IoBuildAsynchronousFsdRequest(IRP_MJ_READ, pDeviceObject, NULL, 0, &liOffset, &ioStatus);

	if (pNewIrp == NULL)
	{
		KdPrint(("IoBuildAsynchronousFsdRequest failed ...\n"));
		return;
	}

	pNewIrp->UserEvent = &my_event;

	pNextStack = IoGetNextIrpStackLocation(pNewIrp);

	pNextStack->FileObject = pFileObject;

	ntStatus = IoCallDriver(pDeviceObject, pNewIrp);

	if (ntStatus == STATUS_PENDING)
	{
		KdPrint(("IoCallDriver read irp is being pended ...\n"));
		
		KeWaitForSingleObject(&my_event, Executive, KernelMode, FALSE, NULL);

		KdPrint(("IoCallDriver read irp has returned ...\n"));
	}

	ZwClose(pFileObject);

	ObDereferenceObject(pFileObject);

	return;
}

VOID	AllocateIrp()
{
	NTSTATUS			ntStatus;
	PFILE_OBJECT		pFileObject;
	PDEVICE_OBJECT		pDeviceObject;
	IO_STATUS_BLOCK		ioStatus;
	PIRP				pNewIrp;
	KEVENT				my_event;
	UNICODE_STRING		ustrDevName;
	PIO_STACK_LOCATION	pNextStack;

	RtlInitUnicodeString(&ustrDevName, L"\\Device\\TargetDevice");

	ntStatus = IoGetDeviceObjectPointer(&ustrDevName, FILE_ALL_ACCESS, &pFileObject, &pDeviceObject);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("IoGetDeviceObjectPointer failed ...\n"));
		return;
	}

	KeInitializeEvent(&my_event, NotificationEvent, FALSE);

	pNewIrp = IoAllocateIrp(pDeviceObject->StackSize, FALSE);

	pNewIrp->UserEvent = &my_event;

	pNewIrp->IoStatus = &ioStatus;

	pNewIrp->Tail.Overlay.Thread = PsGetCurrentThread();

	pNewIrp->AssociatedIrp.SystemBuffer = NULL;

	pNextStack = IoGetNextIrpStackLocation(pNewIrp);

	pNextStack->MajorFunction = IRP_MJ_READ;

	pNextStack->MinorFunction = IRP_MN_NORMAL;

	pNextStack->FileObject = pFileObject;

	ntStatus = IoCallDriver(pDeviceObject, pNewIrp);

	if (ntStatus == STATUS_PENDING)
	{
		KdPrint(("IoCallDriver read irp is being pended ...\n"));

		KeWaitForSingleObject(&my_event, Executive, KernelMode, FALSE, NULL);

		KdPrint(("IoCallDriver read irp has returned ...\n"));
	}

	ObDereferenceObject(pFileObject);

	return;
}