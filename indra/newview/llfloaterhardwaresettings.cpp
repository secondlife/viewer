/** 
 * @file llfloaterhardwaresettings.cpp
 * @brief Menu of all the different graphics hardware settings
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
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
#include "llspinctrl.h"
#include "llstartup.h"
#include "lltextbox.h"
#include "llcombobox.h"
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

	getChild<LLUICtrl>("fsaa")->setValue((LLSD::Integer) mFSAASamples);
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
		getChildView("vbo")->setEnabled(FALSE);
	}

	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderCompressTextures") ||
		!gGLManager.mHasVertexBufferObject)
	{
		getChildView("texture compression")->setEnabled(FALSE);
	}

	// if no windlight shaders, turn off nighttime brightness, gamma, and fog distance
	LLSpinCtrl* gamma_ctrl = getChild<LLSpinCtrl>("gamma");
	gamma_ctrl->setEnabled(!gPipeline.canUseWindLightShaders());
	getChildView("(brightness, lower is brighter)")->setEnabled(!gPipeline.canUseWindLightShaders());
	getChildView("fog")->setEnabled(!gPipeline.canUseWindLightShaders());

	// anti-aliasing
	{
		LLUICtrl* fsaa_ctrl = getChild<LLUICtrl>("fsaa");
		LLTextBox* fsaa_text = getChild<LLTextBox>("antialiasing label");
		LLView* fsaa_restart = getChildView("antialiasing restart");
		
		// Enable or disable the control, the "Antialiasing:" label and the restart warning
		// based on code support for the feature on the current hardware.

		if (gPipeline.canUseAntiAliasing())
		{
			fsaa_ctrl->setEnabled(TRUE);
			
			// borrow the text color from the gamma control for consistency
			fsaa_text->setColor(gamma_ctrl->getEnabledTextColor());

			fsaa_restart->setVisible(!gSavedSettings.getBOOL("RenderDeferred"));
		}
		else
		{
			fsaa_ctrl->setEnabled(FALSE);
			fsaa_ctrl->setValue((LLSD::Integer) 0);
			
			// borrow the text color from the gamma control for consistency
			fsaa_text->setColor(gamma_ctrl->getDisabledTextColor());
			
			fsaa_restart->setVisible(FALSE);
		}
	}
}

//============================================================================

BOOL LLFloaterHardwareSettings::postBuild()
{
	childSetAction("OK", onBtnOK, this);

	if (gGLManager.mIsIntel || gGLManager.mGLVersion < 3.f)
	{ //remove FSAA settings above "4x"
		LLComboBox* combo = getChild<LLComboBox>("fsaa");
		combo->remove("8x");
		combo->remove("16x");
	}

	refresh();
	center();

	// load it up
	initCallbacks();
	return TRUE;
}


void LLFloaterHardwareSettings::apply()
{
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


