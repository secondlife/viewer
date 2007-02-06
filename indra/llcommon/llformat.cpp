/** 
 * @file llformat.cpp
 * @date   January 2007
 * @brief string formatting utility
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llformat.h"

#include <stdarg.h>

std::string llformat(const char *fmt, ...)
{
	char tstr[1024];	/* Flawfinder: ignore */
	va_list va;
	va_start(va, fmt);
#if LL_WINDOWS
	_vsnprintf(tstr, 1024, fmt, va);
#else
	vsnprintf(tstr, 1024, fmt, va);	/* Flawfinder: ignore */
#endif
	va_end(va);
	return std::string(tstr);
}
