#include "precomp.h"

VOID	DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT		pDevObj;
	PDEVICE_EXTENSION	pDevExt;
	UNICODE_STRING		ustrSymName;

	pDevObj = pDriverObject->DeviceObject;
	

	while(pDevObj != NULL)
	{
		pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
		ustrSymName = pDevExt->ustrSymName;
		IoDeleteSymbolicLink(&ustrSymName);
		pDevObj = pDevObj->NextDevice;
		IoDeleteDevice(pDevExt->pDevObj);
	}
}

NTSTATUS	CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT			pDevObj;
	PDEVICE_EXTENSION		pDevExt;
	UNICODE_STRING			ustrDevName;
	UNICODE_STRING			ustrSymName;
	NTSTATUS				ntStatus;

	RtlInitUnicodeString(&ustrDevName, wszDevName);

	ntStatus = IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&ustrDevName, 
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&pDevObj);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("IoCreateDevice failed ...\n"));
		return ntStatus;
	}

	//	设置Read/Write的通信方式
	pDevObj->Flags = DO_BUFFERED_IO;

	//	由于是在DriverEntry例程中创建Device，所以是由IO管理器来清空DO_DEVICE_INITIALIZING标志位
	//	如果是在其他例程中创建Device则需要由程序员来清空该标志位
	//	该标志位的作用是防止其他组件在设备创建的过程中向设备发送IRP
	
	//pDevObj->Flags ~= DO_DEVICE_INITIALIZING;

	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	pDevExt->pDevObj = pDevObj;
	pDevExt->ustrDevName = ustrDevName;

	RtlInitUnicodeString(&ustrSymName, wszSymName);

	ntStatus = IoCreateSymbolicLink(&ustrSymName, &ustrDevName);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("IoCreateSymbolicLink failed ...\n"));
		return ntStatus;
	}

	pDevExt->ustrSymName = ustrSymName;

	//	初始化定时器和DPC

	KeInitializeTimer(&pDevExt->Timer);
	KeInitializeDpc(&pDevExt->Dpc, CompletePendingIrpDpc, pDevObj);

	//	打印DeviceObject的地址
	KdPrint(("target device object address : %x\n", pDevObj));

	return ntStatus;
}

NTSTATUS	DefaultDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS	MyReadDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS			ntStatus;
	PDEVICE_EXTENSION	pDevExt;
	LARGE_INTEGER		liDueTime;

	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	KdPrint(("enter read dispatch routine ...\n"));

	IoMarkIrpPending(pIrp);

	pDevExt->pPendedIrp = pIrp;

	liDueTime = RtlConvertLongToLargeInteger(-50*1000*1000);

	KeSetTimer(&pDevExt->Timer, liDueTime, &pDevExt->Dpc);

	KdPrint(("leave read dispatch routine ...\n"));

	return STATUS_PENDING;
}

VOID	CompletePendingIrpDpc(PKDPC pDpc, 
							  PVOID DeferredContext, 
							  PVOID SystemArgument1, 
							  PVOID SystemArgument2)
{
	PDEVICE_OBJECT		pDevObj;
	PDEVICE_EXTENSION	pDevExt;
	PIRP				pPendedIrp;

	pDevObj = (PDEVICE_OBJECT)DeferredContext;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pPendedIrp = pDevExt->pPendedIrp;

	pPendedIrp->IoStatus.Status = STATUS_SUCCESS;
	pPendedIrp->IoStatus.Information = 0;
	IoCompleteRequest(pPendedIrp, IO_NO_INCREMENT);
}

NTSTATUS	DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	NTSTATUS		ntStatus;
	ULONG			ulIndex;

	pDriverObject->DriverUnload = DriverUnload;

	for (ulIndex = 0; ulIndex<IRP_MJ_MAXIMUM_FUNCTION; ulIndex++)
	{
		pDriverObject->MajorFunction[ulIndex] = DefaultDispatch;
	}

	pDriverObject->MajorFunction[IRP_MJ_READ] = MyReadDispatch;

	ntStatus = CreateDevice(pDriverObject);

	return ntStatus;
}