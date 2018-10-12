#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#include <ntddk.h>

typedef	struct	_DEVICE_EXTENSION
{
	PDEVICE_OBJECT			pDevObj;
	PDEVICE_OBJECT			pTargetDevice;
	UNICODE_STRING			ustrDevName;
	UNICODE_STRING			ustrSymName;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

const	WCHAR		wszDevName[] = L"\\Device\\FilterDevice";
const	WCHAR		wszSymName[] = L"\\DosDevices\\FilterDevice";

#endif
