/** 
 * @file llvectorperfoptions.h
 * @brief Control of vector performance options
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_VECTORPERFOPTIONS_H
#define LL_VECTORPERFOPTIONS_H

namespace LLVectorPerformanceOptions
{
	void initClass(); // Run after configuration files are read.
	void cleanupClass();
};

#endif
