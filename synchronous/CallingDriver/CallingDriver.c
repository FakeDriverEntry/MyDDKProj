#include "precomp.h"

const	WCHAR		wszDevName[] = L"\\Device\\CallingDevice";
const	WCHAR		wszSymName[] = L"\\DosDevices\\CallingDevice";


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
	NTSTATUS		ntStatus = STATUS_SUCCESS;

	UsingIoCompletion();

	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return ntStatus;
}

NTSTATUS	MyDeviceIoControl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS			ntStatus = STATUS_SUCCESS;
	ULONG				ulCtrlCode;
	PIO_STACK_LOCATION	irpSp;

	irpSp = IoGetCurrentIrpStackLocation(pIrp);

	ulCtrlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;

	switch(ulCtrlCode)
	{
	
	case IOCTL_USING_SYNCHRONIZE:
		SynchronizeRead();
		break;

	case IOCTL_USING_IO_COMPLETION:
		UsingIoCompletion();
		break;

	case IOCTL_USING_FILE_POINTER:
		UsingFilePointer();
		break;
	}

	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return ntStatus;
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

	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyDeviceIoControl;

	pDriverObject->MajorFunction[IRP_MJ_READ] = MyReadDispatch;

	ntStatus = CreateDevice(pDriverObject);

	return ntStatus;
}