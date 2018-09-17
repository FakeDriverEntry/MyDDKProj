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

	//	����Read/Write��ͨ�ŷ�ʽ
	pDevObj->Flags = DO_BUFFERED_IO;

	//	��������DriverEntry�����д���Device����������IO�����������DO_DEVICE_INITIALIZING��־λ
	//	����������������д���Device����Ҫ�ɳ���Ա����ոñ�־λ
	//	�ñ�־λ�������Ƿ�ֹ����������豸�����Ĺ��������豸����IRP
	
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

NTSTATUS	MyDeviceIoControl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS			ntStatus;
	ULONG				ulCtrlCode;
	PIO_STACK_LOCATION	irpSp;

	ulCtrlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;
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

	ntStatus = CreateDevice(pDriverObject);

	return ntStatus;
}