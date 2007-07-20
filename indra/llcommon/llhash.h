/** 
 * @file llhash.h
 * @brief Wrapper for a hash function.
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLHASH_H
#define LL_LLHASH_H

#include "llpreprocessor.h" // for GCC_VERSION

#if (LL_WINDOWS)
#include <hash_map>
#include <algorithm>
#elif LL_DARWIN || LL_LINUX
#  if GCC_VERSION >= 30400 // gcc 3.4 and up
#    include <ext/hashtable.h>
#  elif __GNUC__ >= 3
#    include <ext/stl_hashtable.h>
#  else
#    include <hashtable.h>
#  endif
#elif LL_SOLARIS
#include <ext/hashtable.h>
#else
#error Please define your platform.
#endif

template<class T> inline size_t llhash(T value) 
{ 
#if LL_WINDOWS
	return stdext::hash_value<T>(value);
#elif ( (defined _STLPORT_VERSION) || ((LL_LINUX) && (__GNUC__ <= 2)) )
	std::hash<T> H;
	return H(value);
#elif LL_DARWIN || LL_LINUX || LL_SOLARIS
	__gnu_cxx::hash<T> H;
	return H(value);
#else
#error Please define your platform.
#endif
}

#endif

