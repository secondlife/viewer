/** 
 * @file fmodwrapper.cpp
 * @brief dummy source file for building a shared library to wrap libfmod.a
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

extern "C"
{
	void FSOUND_Init(void);
}

void* fmodwrapper(void)
{
	// When building the fmodwrapper library, the linker doesn't seem to want to bring in libfmod.a unless I explicitly
	// reference at least one symbol in the library.  This seemed like the simplest way.
	return (void*)&FSOUND_Init;
}
