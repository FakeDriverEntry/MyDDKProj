#include "precomp.h"

VOID	FastMutexThread1(PVOID pContext)
{
	PKMUTEX			pMutex = (PKMUTEX)pContext;

	KeWaitForSingleObject(pMutex, Executive, KernelMode, FALSE, NULL);

	KdPrint(("begin execute thread 1\n"));

	KeStallExecutionProcessor(50000);

	KdPrint(("end of executing thread 1\n"));

	KeReleaseMutex(pMutex, FALSE);

	PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID	FastMutexThread2(PVOID pContext)
{
	PKMUTEX			pMutex = (PKMUTEX)pContext;

	KeWaitForSingleObject(pMutex, Executive, KernelMode, FALSE, NULL);

	KdPrint(("beging to execute thread 2\n"));

	KdPrint(("thread 2 begin doing something\n"));

	KdPrint(("end of executing thread 2\n"));

	KeReleaseMutex(pMutex, FALSE);

	PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID	FastMutexTest()
{
	KMUTEX		Mutex;
	HANDLE		hThread[2];
	PVOID		pThreadObject[2];

	KeInitializeMutex(&Mutex, 0);

	PsCreateSystemThread(&hThread[0], 0, NULL, NtCurrentProcess(), NULL, FastMutexThread1, &Mutex);
	PsCreateSystemThread(&hThread[1], 0, NULL, NtCurrentProcess(), NULL, FastMutexThread2, &Mutex);

	ObReferenceObjectByHandle(hThread[0], 0, *PsThreadType, KernelMode, &pThreadObject[0], NULL);
	ObReferenceObjectByHandle(hThread[1], 0, *PsThreadType, KernelMode, &pThreadObject[1], NULL);

	KeWaitForMultipleObjects(2, pThreadObject, WaitAll, Executive, KernelMode, FALSE, NULL, NULL);

	ObDereferenceObject(pThreadObject[0]);
	ObDereferenceObject(pThreadObject[1]);
}