/** 
 * @file llwindebug.h
 * @brief LLWinDebug class header file
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLWINDEBUG_H
#define LL_LLWINDEBUG_H

#include "stdtypes.h"
#include <dbghelp.h>

class LLWinDebug
{
public:
	static BOOL setupExceptionHandler();

	static LONG WINAPI handleException(struct _EXCEPTION_POINTERS *pExceptionInfo);
	static void writeDumpToFile(MINIDUMP_TYPE type, MINIDUMP_EXCEPTION_INFORMATION *ExInfop, const char *filename);
};

#endif // LL_LLWINDEBUG_H
