/** 
 * @file llfloaterhardwaresettings.cpp
 * @brief Menu of all the different graphics hardware settings
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "llviewerprecompiledheaders.h"

#include "llfloaterhardwaresettings.h"

// Viewer includes
#include "llfloaterpreference.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "llviewertexturelist.h"
#include "llfeaturemanager.h"
#include "llstartup.h"
#include "pipeline.h"

// Linden library includes
#include "llradiogroup.h"
#include "lluictrlfactory.h"
#include "llwindow.h"
#include "llsliderctrl.h"

LLFloaterHardwareSettings::LLFloaterHardwareSettings(const LLSD& key)
	: LLFloater(key),

	  // these should be set on imminent refresh() call,
	  // but init them anyway
	  mUseVBO(0),
	  mUseAniso(0),
	  mFSAASamples(0),
	  mGamma(0.0),
	  mVideoCardMem(0),
	  mFogRatio(0.0),
	  mProbeHardwareOnStartup(FALSE)
{
	//LLUICtrlFactory::getInstance()->buildFloater(this, "floater_hardware_settings.xml");
}

LLFloaterHardwareSettings::~LLFloaterHardwareSettings()
{
}

void LLFloaterHardwareSettings::initCallbacks(void) 
{
}

// menu maintenance functions

void LLFloaterHardwareSettings::refresh()
{
	LLPanel::refresh();

	mUseVBO = gSavedSettings.getBOOL("RenderVBOEnable");
	mUseAniso = gSavedSettings.getBOOL("RenderAnisotropic");
	mFSAASamples = gSavedSettings.getU32("RenderFSAASamples");
	mGamma = gSavedSettings.getF32("RenderGamma");
	mVideoCardMem = gSavedSettings.getS32("TextureMemory");
	mFogRatio = gSavedSettings.getF32("RenderFogRatio");
	mProbeHardwareOnStartup = gSavedSettings.getBOOL("ProbeHardwareOnStartup");

	childSetValue("fsaa", (LLSD::Integer) mFSAASamples);
	refreshEnabledState();
}

void LLFloaterHardwareSettings::refreshEnabledState()
{
	S32 min_tex_mem = LLViewerTextureList::getMinVideoRamSetting();
	S32 max_tex_mem = LLViewerTextureList::getMaxVideoRamSetting();
	getChild<LLSliderCtrl>("GraphicsCardTextureMemory")->setMinValue(min_tex_mem);
	getChild<LLSliderCtrl>("GraphicsCardTextureMemory")->setMaxValue(max_tex_mem);

	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderVBOEnable") ||
		!gGLManager.mHasVertexBufferObject)
	{
		childSetEnabled("vbo", FALSE);
	}

	// if no windlight shaders, turn off nighttime brightness, gamma, and fog distance
	childSetEnabled("gamma", !gPipeline.canUseWindLightShaders());
	childSetEnabled("(brightness, lower is brighter)", !gPipeline.canUseWindLightShaders());
	childSetEnabled("fog", !gPipeline.canUseWindLightShaders());

}

//============================================================================

BOOL LLFloaterHardwareSettings::postBuild()
{
	childSetAction("OK", onBtnOK, this);

	refresh();
	center();

	// load it up
	initCallbacks();
	return TRUE;
}


void LLFloaterHardwareSettings::apply()
{
	// Anisotropic rendering
	BOOL old_anisotropic = LLImageGL::sGlobalUseAnisotropic;
	LLImageGL::sGlobalUseAnisotropic = childGetValue("ani");

	U32 fsaa = (U32) childGetValue("fsaa").asInteger();
	U32 old_fsaa = gSavedSettings.getU32("RenderFSAASamples");

	BOOL logged_in = (LLStartUp::getStartupState() >= STATE_STARTED);

	if (old_fsaa != fsaa)
	{
		gSavedSettings.setU32("RenderFSAASamples", fsaa);
		LLWindow* window = gViewerWindow->getWindow();
		LLCoordScreen size;
		window->getSize(&size);
		gViewerWindow->changeDisplaySettings(window->getFullscreen(), 
														size,
														gSavedSettings.getBOOL("DisableVerticalSync"),
														logged_in);
	}
	else if (old_anisotropic != LLImageGL::sGlobalUseAnisotropic)
	{
		gViewerWindow->restartDisplay(logged_in);
	}

	refresh();
}


void LLFloaterHardwareSettings::cancel()
{
	gSavedSettings.setBOOL("RenderVBOEnable", mUseVBO);
	gSavedSettings.setBOOL("RenderAnisotropic", mUseAniso);
	gSavedSettings.setU32("RenderFSAASamples", mFSAASamples);
	gSavedSettings.setF32("RenderGamma", mGamma);
	gSavedSettings.setS32("TextureMemory", mVideoCardMem);
	gSavedSettings.setF32("RenderFogRatio", mFogRatio);
	gSavedSettings.setBOOL("ProbeHardwareOnStartup", mProbeHardwareOnStartup );

	closeFloater();
}

// static 
void LLFloaterHardwareSettings::onBtnOK( void* userdata )
{
	LLFloaterHardwareSettings *fp =(LLFloaterHardwareSettings *)userdata;
	fp->apply();
	fp->closeFloater(false);
}


