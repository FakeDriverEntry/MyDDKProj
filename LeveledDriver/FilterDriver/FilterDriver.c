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
		if (pDevExt->pTargetDevice != NULL)
		{
			IoDetachDevice(pDevExt->pTargetDevice);
		}
		pDevObj = pDevObj->NextDevice;
		IoDetachDevice(pDevExt->pDevObj);
	}
}

NTSTATUS	CreateDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	PDEVICE_EXTENSION		pDevExt;
	NTSTATUS				ntStatus;

	
	KdPrint(("Entering Filter Device's read dispatch routine ...\n"));

	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	IoSkipCurrentIrpStackLocation(pIrp);
	
	ntStatus = IoCallDriver(pDevExt->pTargetDevice, pIrp);

	KdPrint(("Leaving Filter Device's read dispatch routine ...\n"));

	return ntStatus;
}

NTSTATUS	CloseDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	PDEVICE_EXTENSION		pDevExt;
	NTSTATUS				ntStatus;


	KdPrint(("Entering Filter Device's close dispatch routine ...\n"));

	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	IoSkipCurrentIrpStackLocation(pIrp);

	ntStatus = IoCallDriver(pDevExt->pTargetDevice, pIrp);

	KdPrint(("Leaving Filter Device's close dispatch routine ...\n"));

	return ntStatus;
}

NTSTATUS	CommonDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	KdPrint(("Entering Filter Device common routine ...\n"));

	pIrp->IoStatus.Status = STATUS_SUCCESS;

	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leaving Filter Device common routine ...\n"));

	return STATUS_SUCCESS;
}

NTSTATUS	ReadDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	PDEVICE_EXTENSION		pDevExt;
	NTSTATUS				ntStatus;


	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	KdPrint(("entering filter device's read dispatch routine ...\n"));

	IoSkipCurrentIrpStackLocation(pIrp);

	ntStatus = IoCallDriver(pDevExt->pTargetDevice, pIrp);

	KdPrint(("filter device has passed current read irp to the target device and ready to leave read dispatch routine ...\n"));

	return ntStatus;
}

NTSTATUS	DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pustrRegPath)
{
	NTSTATUS			ntStatus;
	ULONG				ulIndex;
	PDEVICE_EXTENSION	pDevExt;
	PDEVICE_OBJECT		pDevObj;
	PDEVICE_OBJECT		pTargetDevice;
	PDEVICE_OBJECT		pReturnedDevice;
	PFILE_OBJECT		pFileObject;
	UNICODE_STRING		ustrTargetDeviceName;

	for (ulIndex = 0; ulIndex<IRP_MJ_MAXIMUM_FUNCTION; ulIndex++)
	{
		pDriverObject->MajorFunction[ulIndex] = CommonDispatchRoutine;
	}

	pDriverObject->MajorFunction[IRP_MJ_CREATE] = CreateDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_READ] = ReadDispatchRoutine;
	pDriverObject->DriverUnload = DriverUnload;

	ntStatus = CreateDevice(pDriverObject);

	if (!NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}

	pDevObj = pDriverObject->DeviceObject;

	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	RtlInitUnicodeString(&ustrTargetDeviceName, L"\\Device\\TargetDevice");

	ntStatus = IoGetDeviceObjectPointer(&ustrTargetDeviceName, FILE_ALL_ACCESS, &pFileObject, &pReturnedDevice);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("IoGetDeviceObjectPointer failed : %x\n", ntStatus));
		return ntStatus;
	}

	pTargetDevice = IoAttachDeviceToDeviceStack(pDevExt->pDevObj, pReturnedDevice);

	if (pTargetDevice == NULL)
	{
		KdPrint(("IoAttachDeviceToDeviceStack failed ...\n"));

		ObDereferenceObject(pFileObject);

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	pDevExt->pTargetDevice = pTargetDevice;

	pDevExt->pDevObj->Characteristics = pTargetDevice->Characteristics;
	pDevExt->pDevObj->DeviceType = pTargetDevice->DeviceType;
	pDevExt->pDevObj->Flags &= ~DO_DEVICE_INITIALIZING;
	pDevExt->pDevObj->Flags |= (pTargetDevice->Flags&(DO_BUFFERED_IO|DO_DIRECT_IO));

	ObDereferenceObject(pFileObject);

	KdPrint(("filter device has attached to the target device successfully ...\n"));

	return ntStatus;
}