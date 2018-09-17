#ifndef		_PRECOMP_H_
#define		_PRECOMP_H_

#include <ntddk.h>
#include "CallingDriver.h"
#include "ioctrl_code.h"
#include "operations.h"


typedef	struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT		pDevObj;
	UNICODE_STRING		ustrDevName;
	UNICODE_STRING		ustrSymName;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

const	WCHAR		wszDevName[] = L"\\Device\\CallingDevice";
const	WCHAR		wszSymName[] = L"\\DosDevices\\CallingDevice";

#endif
