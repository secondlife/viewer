/** 
 * @file llfloaterbuildoptions.h
 * @brief LLFloaterBuildOptions class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/**
 * Panel for setting global object-editing options, specifically
 * grid size and spacing.
 */

#ifndef LL_LLFLOATERBUILDOPTIONS_H
#define LL_LLFLOATERBUILDOPTIONS_H

#include "llfloater.h"


class LLFloaterBuildOptions
:	public LLFloater
{
protected:
	LLFloaterBuildOptions();
	~LLFloaterBuildOptions();

public:
	static void		show(void*);
	static LLFloaterBuildOptions* getInstance();
	static BOOL		visible(void*);

protected:
	static LLFloaterBuildOptions*	sInstance;
};

#endif
