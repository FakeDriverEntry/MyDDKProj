#include <ntddk.h>

#include "ioctrl_code.h"
#include "mutex.h"
#include "dpc.h"

typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT		pDevObj;
	UNICODE_STRING		ustrDevName;
	UNICODE_STRING		ustrSymName;
	LIST_ENTRY			PendingIrpHead;
	KSPIN_LOCK			spinLock;
	KDPC				Dpc;
	KTIMER				Timer;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;