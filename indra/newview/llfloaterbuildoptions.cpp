/** 
 * @file llfloaterbuildoptions.cpp
 * @brief LLFloaterBuildOptions class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
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
		sInstance->open();	/*Flawfinder: ignore*/
	}
	else
	{
		LLFloaterBuildOptions* floater = new LLFloaterBuildOptions();

		gUICtrlFactory->buildFloater(floater, "floater_build_options.xml");
		floater->open();	/*Flawfinder: ignore*/
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
