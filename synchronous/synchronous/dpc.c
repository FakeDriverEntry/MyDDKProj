#include "precomp.h"

void CustomDpc(PKDPC pDpc, 
			   PVOID DeferredContext,
			   PVOID SystemArgument1,
			   PVOID SystemArgument2)
{
	PDEVICE_OBJECT		pDevObj;
	PDEVICE_EXTENSION	pDevExt;
	LARGE_INTEGER		liDueTime;

	pDevObj = (PDEVICE_OBJECT)DeferredContext;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	liDueTime = RtlConvertLongToLargeInteger(-2*10*1000*1000);

	KdPrint(("elapse time arrive ... \n"));

	KeSetTimer(&pDevExt->Timer, liDueTime, &pDevExt->Dpc);

}