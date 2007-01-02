/** 
 * @file llfloaterbump.h
 * @brief Floater showing recent bumps, hits with objects, pushes, etc.
 * @author Cory Ondrejka, James Cook
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERBUMP_H
#define LL_LLFLOATERBUMP_H

#include "llfloater.h"

class LLMeanCollisionData;
class LLScrollListCtrl;

class LLFloaterBump 
: public LLFloater
{
public:
	static void show(void *);

private:
	LLFloaterBump();
	virtual ~LLFloaterBump();
	static void add(LLScrollListCtrl* list, LLMeanCollisionData *mcd);
	
private:
	static LLFloaterBump* sInstance;
};

#endif
