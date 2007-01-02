/** 
 * @file llfloaterabout.h
 * @brief The about box from Help -> About
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERABOUT_H
#define LL_LLFLOATERABOUT_H

#include "llfloater.h"

class LLFloaterAbout 
: public LLFloater
{
public:
	LLFloaterAbout();
	virtual ~LLFloaterAbout();

	static void show(void*);

private:
	static LLFloaterAbout* sInstance;
};


#endif // LL_LLFLOATERABOUT_H
