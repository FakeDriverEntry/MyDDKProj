#ifndef _OPERATIONS_H_
#define _OPERATIONS_H_

#define		DEV_NAME_LEN		32*sizeof(WCHAR)

VOID		SynchronizeRead();

VOID		UsingIoCompletion();

VOID		UsingFilePointer();

VOID		UsingSymbolicLinkOpenDevice();

NTSTATUS	BuildSyncIrp();

NTSTATUS	BuildAsyncIrp();


#endif