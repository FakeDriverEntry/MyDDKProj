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

extern	POBJECT_TYPE	IoDeviceObjectType;

NTSTATUS ObReferenceObjectByName(
		PUNICODE_STRING		ObjectName,
		ULONG				Attributes,
		PACCESS_STATE		AccessState,
		ACCESS_MASK			DesiredAccess,
		POBJECT_TYPE		ObjectType,
		KPROCESSOR_MODE		AccessMode,
		PVOID				ParseContext,
		PVOID				*Object	);

#endif
