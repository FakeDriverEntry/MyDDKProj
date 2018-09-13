#include "precomp.h"

typedef	struct _PENDING_IRP_ENTRY
{
	PIRP		pIrp;
	LIST_ENTRY	LinkList;
}PENDING_IRP_ENTRY, *PPENDING_IRP_ENTRY;

WCHAR		wszDevName[] = L"\\Device\\synchronous";
WCHAR		wszSymName[] = L"\\??\\synchronous";

VOID	DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT			pNextObj = NULL;
	UNICODE_STRING			ustrSymName;
	PDEVICE_EXTENSION		pDevExt = NULL;

	pNextObj = pDriverObject->DeviceObject;

	while(pNextObj != NULL)
	{
		pDevExt = pNextObj->DeviceExtension;
		ustrSymName = pDevExt->ustrSymName;
		IoDeleteSymbolicLink(&ustrSymName);
		pNextObj = pNextObj->NextDevice;
		IoDeleteDevice(pDevExt->pDevObj);
	}
}

NTSTATUS	CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	PDEVICE_OBJECT		pDevObj = NULL;
	PDEVICE_EXTENSION	pDevExt = NULL;
	UNICODE_STRING		ustrDevName;
	UNICODE_STRING		ustrSymName;

	RtlInitUnicodeString(&ustrDevName, wszDevName);

	status = IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&ustrDevName,
		FILE_DEVICE_UNKNOWN,
		0,
		TRUE,
		&pDevObj);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("IoCreateDevice failed ...\n"));
		return status;
	}

	pDevObj->Flags |= DO_BUFFERED_IO;

	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevObj = pDevObj;
	pDevExt->ustrDevName = ustrDevName;

	RtlInitUnicodeString(&ustrSymName, wszSymName);

	status = IoCreateSymbolicLink(&ustrSymName, &ustrDevName);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("IoCreateSymbolicLink failed ...\n"));
		return status;
	}

	pDevExt->ustrSymName = ustrSymName;

	InitializeListHead(&pDevExt->PendingIrpHead);

	KeInitializeSpinLock(&pDevExt->spinLock);

	KeInitializeTimer(&pDevExt->Timer);
	
	KeInitializeDpc(&pDevExt->Dpc, CustomDpc, pDevObj);

	return status;
}

NTSTATUS	MyDefaultDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS	MyCleanupDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	PDEVICE_EXTENSION			pDevExt = NULL;
	PLIST_ENTRY					pLinkEntry;
	PPENDING_IRP_ENTRY			pIrpEntry;

	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	while(!IsListEmpty(&pDevExt->PendingIrpHead))
	{
		pLinkEntry = RemoveTailList(&pDevExt->PendingIrpHead);
		pIrpEntry = CONTAINING_RECORD(pLinkEntry, PENDING_IRP_ENTRY, LinkList);

		pIrpEntry->pIrp->IoStatus.Status = STATUS_SUCCESS;
		pIrpEntry->pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrpEntry->pIrp, IO_NO_INCREMENT);
		ExFreePool(pIrpEntry);
	}

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS	MyDevIoCtrlDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	PIO_STACK_LOCATION			irpSp;
	ULONG						ulCtrlCode;
	PDEVICE_EXTENSION			pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	LARGE_INTEGER				liDueTime;

	irpSp = IoGetCurrentIrpStackLocation(pIrp);
	ulCtrlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;

	switch(ulCtrlCode)
	{
	case IOCTRL_FAST_MUTEX:
		FastMutexTest();
		break;
	case IOCTRL_START_TIMER:
		liDueTime = RtlConvertLongToLargeInteger(-2*10*1000*1000);
		KeSetTimer(&pDevExt->Timer, liDueTime, &pDevExt->Dpc);
		break;
	case IOCTRL_STOP_TIMER:
		KeCancelTimer(&pDevExt->Timer);
		break;
	}

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS	MyReadDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	PDEVICE_EXTENSION		pDevExt;
	PPENDING_IRP_ENTRY		pIrpEntry = NULL;
	KSPIN_LOCK				spinLock;
	KIRQL					oldIrql;

	pDevExt = pDevObj->DeviceExtension;

	spinLock = pDevExt->spinLock;

	pIrpEntry = ExAllocatePool(PagedPool, sizeof(PENDING_IRP_ENTRY));
	RtlZeroMemory(pIrpEntry, sizeof(PENDING_IRP_ENTRY));

	pIrpEntry->pIrp = pIrp;

	KeAcquireSpinLock(&pDevExt->spinLock, &oldIrql);

	InsertTailList(&pDevExt->PendingIrpHead, &pIrpEntry->LinkList);

	KeReleaseSpinLock(&pDevExt->spinLock, oldIrql);

	IoMarkIrpPending(pIrp);

	return STATUS_PENDING;

}

NTSTATUS	DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	NTSTATUS		status = STATUS_UNSUCCESSFUL;
	ULONG			ulIndex;

	pDriverObject->DriverUnload = DriverUnload;

	for (ulIndex=0; ulIndex<IRP_MJ_MAXIMUM_FUNCTION; ulIndex++)
	{
		pDriverObject->MajorFunction[ulIndex] = MyDefaultDispatch;
	}

	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyDevIoCtrlDispatch;
	pDriverObject->MajorFunction[IRP_MJ_READ] = MyReadDispatch;
	pDriverObject->MajorFunction[IRP_MJ_CLEANUP] = MyCleanupDispatch;

	status = CreateDevice(pDriverObject);

	return status;

}