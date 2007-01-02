/** 
 * @file llmortician.h
 * @brief Base class for delayed deletions.
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LLMORTICIAN_H
#define LLMORTICIAN_H

#include "stdtypes.h"

class LLMortician 
{
public:
	LLMortician() { mIsDead = FALSE; }
	static void updateClass();
	virtual ~LLMortician();
	void die();
	BOOL isDead() { return mIsDead; }

	// sets destroy immediate true
	static void setZealous(BOOL b);

private:
	static BOOL sDestroyImmediate;

	BOOL mIsDead;
};

#endif

