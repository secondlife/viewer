/** 
 * @file llerror.cpp
 * @brief Function to crash.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#include "linden_common.h"

#include "llerror.h"

LLErrorBuffer gErrorBuffer;
LLErrorStream gErrorStream(&gErrorBuffer);


void _llcrash_and_loop()
{
	// Now, we go kaboom!
	U32* crash = NULL;

	*crash = 0;

	while(TRUE)
	{
	
		// Loop forever, in case the crash didn't work?
	}
}

LLScopedErrorLevel::LLScopedErrorLevel(LLErrorBuffer::ELevel error_level)
{
	mOrigErrorLevel = gErrorStream.getErrorLevel();
	gErrorStream.setErrorLevel(error_level);
}


LLScopedErrorLevel::~LLScopedErrorLevel()
{
	gErrorStream.setErrorLevel(mOrigErrorLevel);
}
