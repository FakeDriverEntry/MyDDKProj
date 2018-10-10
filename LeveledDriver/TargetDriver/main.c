#include "precomp.h"

NTSTATUS		CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING			ustrDevName;
	UNICODE_STRING			ustrSymName;
	PDEVICE_OBJECT			pDevObj;
	PDEVICE_EXTENSION		pDevExt;
	NTSTATUS				ntStatus;


	RtlInitUnicodeString(&ustrDevName, wszDevName);

	ntStatus = IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&ustrDevName,
		FILE_DEVICE_UNKNOWN,
		0,
		TRUE,
		&pDevObj);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("IoCreateDevice failed : %x\n", ntStatus));
		return ntStatus;
	}

	pDevObj->Flags |= DO_BUFFERED_IO;

	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	pDevExt->pDevObj = pDevObj;

	pDevExt->ustrDevName = ustrDevName;

	RtlInitUnicodeString(&ustrSymName, wszSymName);

	ntStatus = IoCreateSymbolicLink(&ustrSymName, &ustrDevName);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("IoCreateSymbolicLink failed : %x\n", ntStatus));
		return ntStatus;
	}

	pDevExt->ustrSymName = ustrSymName;

	return ntStatus;
}

VOID	DpcRoutine(PKDPC pDpc, PVOID pContext, PVOID SysArg1, PVOID SysArg2)
{
	PDEVICE_OBJECT			pDevObj;
	PDEVICE_EXTENSION		pDevExt;
	PIRP					pPendedIrp;

	
	pDevObj = (PDEVICE_OBJECT)pContext;
	
	pDevExt	 = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	pPendedIrp = pDevExt->pPendedIrp;

	IoCompleteRequest(pPendedIrp, IO_NO_INCREMENT);

	KdPrint(("count-down time has been reached and the pended irp has been complete ...\n"));

}

VOID	DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_EXTENSION			pDevExt;
	PDEVICE_OBJECT				pDevObj;
	UNICODE_STRING				ustrSymName;

	pDevObj = pDriverObject->DeviceObject;

	while(pDevObj != NULL)
	{
		pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
		ustrSymName = pDevExt->ustrSymName;
		IoDeleteSymbolicLink(&ustrSymName);
		pDevObj = pDevObj->NextDevice;
		IoDetachDevice(pDevExt->pDevObj);
	}
}

NTSTATUS	CreateDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	KdPrint(("Entering TargetDevice IRP_MJ_CREATE routine ...\n"));
	
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leaving TargetDevice IRP_MJ_CREATE routine ...\n"));

	return STATUS_SUCCESS;
}

NTSTATUS	CloseDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	KdPrint(("Entering TargetDevice IRP_MJ_CLOSE routine ...\n"));

	pIrp->IoStatus.Status = STATUS_SUCCESS;

	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leaving TargetDevice IRP_MJ_CLOSE routine ...\n"));

	return STATUS_SUCCESS;
}

NTSTATUS	CommonDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	KdPrint(("Entering TargetDevice common routine ...\n"));

	pIrp->IoStatus.Status = STATUS_SUCCESS;

	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leaving TargetDevice common routine ...\n"));

	return STATUS_SUCCESS;
}

NTSTATUS	ReadDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	PDEVICE_EXTENSION		pDevExt;
	LARGE_INTEGER			liWaitTime;
	

	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

}