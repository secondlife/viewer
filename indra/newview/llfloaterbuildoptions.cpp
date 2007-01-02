/** 
 * @file llfloaterbuildoptions.cpp
 * @brief LLFloaterBuildOptions class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/**
 * Panel for setting global object-editing options, specifically
 * grid size and spacing.
 */ 

#include "llviewerprecompiledheaders.h"

#include "llfloaterbuildoptions.h"
#include "llvieweruictrlfactory.h"

// library includes
#include "llfontgl.h"
#include "llcheckboxctrl.h"
#include "llspinctrl.h"
#include "llsliderctrl.h"

// newview includes
#include "llresmgr.h"
#include "llviewercontrol.h"

//
// Globals
//
LLFloaterBuildOptions	*LLFloaterBuildOptions::sInstance = NULL;

//
// Methods
//
LLFloaterBuildOptions::LLFloaterBuildOptions( )
: LLFloater("build options floater")
{
	sInstance = this;
}

LLFloaterBuildOptions::~LLFloaterBuildOptions()
{
	sInstance = NULL;
}

// static
void LLFloaterBuildOptions::show(void*)
{
	if (sInstance)
	{
		sInstance->open();
	}
	else
	{
		LLFloaterBuildOptions* floater = new LLFloaterBuildOptions();

		gUICtrlFactory->buildFloater(floater, "floater_build_options.xml");
		floater->open();
	}
}

LLFloaterBuildOptions* LLFloaterBuildOptions::getInstance()
{
	return sInstance;
}

// static
BOOL LLFloaterBuildOptions::visible(void*)
{
	return (sInstance != NULL);
}
