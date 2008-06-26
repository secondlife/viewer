/** 
 * @file llfloaterhardwaresettings.cpp
 * @brief Menu of all the different graphics hardware settings
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llfloaterhardwaresettings.h"
#include "llfloaterpreference.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "llviewerimagelist.h"
#include "llfeaturemanager.h"
#include "llstartup.h"

#include "llradiogroup.h"
#include "lluictrlfactory.h"

#include "llimagegl.h"
#include "pipeline.h"

LLFloaterHardwareSettings* LLFloaterHardwareSettings::sHardwareSettings = NULL;

LLFloaterHardwareSettings::LLFloaterHardwareSettings() : LLFloater(std::string("Hardware Settings Floater"))
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_hardware_settings.xml");
	
	// load it up
	initCallbacks();
}

LLFloaterHardwareSettings::~LLFloaterHardwareSettings()
{
}

void LLFloaterHardwareSettings::onClickHelp(void* data)
{
	const char* xml_alert = "HardwareSettingsHelpButton";
	gViewerWindow->alertXml(xml_alert);
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
	S32 min_tex_mem = LLViewerImageList::getMinVideoRamSetting();
	S32 max_tex_mem = LLViewerImageList::getMaxVideoRamSetting();
	childSetMinValue("GrapicsCardTextureMemory", min_tex_mem);
	childSetMaxValue("GrapicsCardTextureMemory", max_tex_mem);

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

// static instance of it
LLFloaterHardwareSettings* LLFloaterHardwareSettings::instance()
{
	if (!sHardwareSettings)
	{
		sHardwareSettings = new LLFloaterHardwareSettings();
		sHardwareSettings->close();
	}
	return sHardwareSettings;
}
void LLFloaterHardwareSettings::show()
{
	LLFloaterHardwareSettings* hardSettings = instance();
	hardSettings->refresh();
	hardSettings->center();

	// comment in if you want the menu to rebuild each time
	//LLUICtrlFactory::getInstance()->buildFloater(hardSettings, "floater_hardware_settings.xml");
	//hardSettings->initCallbacks();

	hardSettings->open();
}

bool LLFloaterHardwareSettings::isOpen()
{
	if (sHardwareSettings != NULL) 
	{
		return true;
	}
	return false;
}

// virtual
void LLFloaterHardwareSettings::onClose(bool app_quitting)
{
	if (sHardwareSettings)
	{
		sHardwareSettings->setVisible(FALSE);
	}
}


//============================================================================

BOOL LLFloaterHardwareSettings::postBuild()
{
	childSetAction("OK", onBtnOK, this);

	refresh();

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

	close();
}

// static 
void LLFloaterHardwareSettings::onBtnOK( void* userdata )
{
	LLFloaterHardwareSettings *fp =(LLFloaterHardwareSettings *)userdata;
	fp->apply();
	fp->close(false);
}

