#ifndef _LOAD_DRIVER_H_
#define _LOAD_DRIVER_H_

#include "stdafx.h"

BOOL LoadNTDriver(char* lpszDriverName,char* lpszDriverPath);

BOOL UnloadNTDriver( char * szSvrName )  ;
#endif
