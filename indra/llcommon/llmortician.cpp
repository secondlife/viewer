/** 
 * @file llmortician.cpp
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llmortician.h"

#include <list>

std::list<LLMortician*> gGraveyard;

BOOL LLMortician::sDestroyImmediate = FALSE;

LLMortician::~LLMortician() 
{
	gGraveyard.remove(this);
}

void LLMortician::updateClass() 
{
	while (!gGraveyard.empty()) 
	{
		LLMortician* dead = gGraveyard.front();
		delete dead;
	}
}

void LLMortician::die() 
{
	// It is valid to call die() more than once on something that hasn't died yet
	if (sDestroyImmediate)
	{
		//HACK: we need to do this to ensure destruction order on shutdown
		mIsDead = TRUE;
		delete this;
		return;
	}
	else if (!mIsDead)
	{
		mIsDead = TRUE;
		gGraveyard.push_back(this);
	}
}

// static
void LLMortician::setZealous(BOOL b)
{
	sDestroyImmediate = b;
}
