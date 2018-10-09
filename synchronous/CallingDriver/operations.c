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

	// 获得设备对象句柄
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

	//	将完成例程的地址作为ZwReadFile的APC Routine参数，当下一层设备对象完成IRP后，该完成例程会被调用
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

	// 获得设备对象的句柄
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

	// 异步读操作，会立即返回
	ntStatus = ZwReadFile(hDevice, NULL, NULL, NULL, &io_status, NULL, 0, &liOffset, NULL);

	if (STATUS_PENDING == ntStatus)
	{
		// 通过设备对象句柄获得该设备的文件对象指针
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

		// 在文件对象指针的Event成员进行等待，当底层IRP完成后会将该事件设置为受信状态，程序继续运行
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

	//	创建符号链接的字符串
	RtlInitUnicodeString(&ustrSymName, L"\\??\\TargetDevice");

	//	初始化对象属性
	InitializeObjectAttributes(&oaSym, &ustrSymName, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);

	//	获得符号链接的句柄
	ntStatus = ZwOpenSymbolicLinkObject(&hSymbolicLink, FILE_ALL_ACCESS, &oaSym);

	if (ntStatus != STATUS_SUCCESS)
	{
		KdPrint(("ZwOpenSymbolicLinkObject failed : %x\n", ntStatus));
		return;
	}

	ustrDevName.Buffer = ExAllocatePool(PagedPool, DEV_NAME_LEN);
	ustrDevName.Length = ustrSymName.MaximumLength = DEV_NAME_LEN;

	//	通过符号链接句柄获得设备对象名的字符串
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

	//	创建同步IRP，将通知事件作为参数传入该函数，当底层设备对象完成IRP后将设置该事件为受信状态
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

	//	将通知事件的地址作为新创建的IRP的UserEvent成员，当底层IRP完成后会将该事件设置为受信状态
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

	//ZwClose(pFileObject);

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

	//	将通知事件的地址作为新创建的IRP的UserEvent成员，当底层IRP完成后会将该事件设置为受信状态
	pNewIrp->UserEvent = &my_event;

	pNewIrp->UserIosb = &ioStatus;

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

VOID	GetDevObjByName()
{
	UNICODE_STRING			ustrSymName;
	PDEVICE_OBJECT			pDevObj;
	NTSTATUS				ntStatus;


	
	RtlInitUnicodeString(&ustrSymName, L"\\??\\TargetDevice");

	ntStatus = ObReferenceObjectByName(&ustrSymName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		FILE_ALL_ACCESS,
		IoDeviceObjectType,
		KernelMode,
		NULL,
		(PVOID*)&pDevObj);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("ObReferenceObjectByName failed : %x\n", ntStatus));
		return;
	}

	KdPrint(("target device's device object address : %x\n", pDevObj));
}