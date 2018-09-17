#ifndef		_PRECOMP_H_
#define		_PRECOMP_H_

#include <ntddk.h>
#include "TargetDriver.h"

typedef	struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT		pDevObj;
	UNICODE_STRING		ustrDevName;
	UNICODE_STRING		ustrSymName;
	KTIMER				Timer;
	KDPC				Dpc;
	PIRP				pPendedIrp;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

const	WCHAR		wszDevName[] = L"\\Device\\TargetDevice";
const	WCHAR		wszSymName[] = L"\\DosDevices\\TargetDevice";

#endif
