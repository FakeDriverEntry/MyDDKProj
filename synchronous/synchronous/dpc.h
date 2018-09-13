#ifndef _DPC_H_
#define _DPC_H_

void	CustomDpc(KDPC			*pDpc,
				  PVOID			DeferredContext,
				  PVOID			SystemArgument1,
				  PVOID			SystemArgument2);

#endif
