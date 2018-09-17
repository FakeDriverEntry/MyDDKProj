#ifndef _TARGETDRIVER_H_
#define _TARGETDRIVER_H_

VOID		DriverUnload(PDRIVER_OBJECT pDriverObject);
NTSTATUS	DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath);
NTSTATUS	CreateDevice(PDRIVER_OBJECT pDriverObject);
NTSTATUS	DefaultDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS	MyReadDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp);
VOID		CompletePendingIrpDpc(PKDPC pDpc, 
								  PVOID DeferredContext,
								  PVOID SystemArgument1,
								  PVOID SystemArgument2);

#endif
