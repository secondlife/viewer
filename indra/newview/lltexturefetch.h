/** 
 * @file lltexturefetch.h
 * @brief Object for managing texture fetches.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTEXTUREFETCH_H
#define LL_LLTEXTUREFETCH_H

#include "llworkerthread.h"

class LLViewerImage;

// Interface class
class LLTextureFetch
{
public:
	static void initClass();
	static void updateClass();
	static void cleanupClass();

	static LLWorkerClass::handle_t addRequest(LLImageFormatted* image, S32 discard);
	static bool getRequestFinished(LLWorkerClass::handle_t handle);
};

#endif LL_LLTEXTUREFETCH_H
