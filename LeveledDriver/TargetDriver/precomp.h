#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#include <ntddk.h>

typedef	struct	_DEVICE_EXTENSION
{
	PDEVICE_OBJECT			pDevObj;
	KDPC					readRoutineDpc;
	KTIMER					readRoutineTimer;
	UNICODE_STRING			ustrDevName;
	UNICODE_STRING			ustrSymName;
	PIRP					pPendedIrp;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

const	WCHAR		wszDevName[] = L"\\Device\\TargetDevice";
const	WCHAR		wszSymName[] = L"\\DosDevices\\TargetDevice";

#endif
