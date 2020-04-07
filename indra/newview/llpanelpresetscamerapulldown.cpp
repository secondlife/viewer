/** 
 * @file llpanelpresetscamerapulldown.cpp
 * @brief A panel showing a quick way to pick camera presets
 *
 * $LicenseInfo:firstyear=2017&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2017, Linden Research, Inc.
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

#include "llpanelpresetscamerapulldown.h"

#include "llviewercontrol.h"
#include "llstatusbar.h"

#include "llbutton.h"
#include "lltabcontainer.h"
#include "llfloatercamera.h"
#include "llfloaterreg.h"
#include "llfloaterpreference.h"
#include "llpresetsmanager.h"
#include "llsliderctrl.h"
#include "llscrolllistctrl.h"
#include "lltrans.h"

/* static */ const F32 LLPanelPresetsCameraPulldown::sAutoCloseFadeStartTimeSec = 2.0f;
/* static */ const F32 LLPanelPresetsCameraPulldown::sAutoCloseTotalTimeSec = 3.0f;

///----------------------------------------------------------------------------
/// Class LLPanelPresetsCameraPulldown
///----------------------------------------------------------------------------

// Default constructor
LLPanelPresetsCameraPulldown::LLPanelPresetsCameraPulldown()
{
	mHoverTimer.stop();

	mCommitCallbackRegistrar.add("Presets.toggleCameraFloater", boost::bind(&LLPanelPresetsCameraPulldown::onViewButtonClick, this, _2));
	mCommitCallbackRegistrar.add("PresetsCamera.RowClick", boost::bind(&LLPanelPresetsCameraPulldown::onRowClick, this, _2));

	buildFromFile( "panel_presets_camera_pulldown.xml");
}

BOOL LLPanelPresetsCameraPulldown::postBuild()
{
	LLPresetsManager* presetsMgr = LLPresetsManager::getInstance();
	if (presetsMgr)
	{
		// Make sure there is a default preference file
		presetsMgr->createMissingDefault(PRESETS_CAMERA);

		presetsMgr->startWatching(PRESETS_CAMERA);

		presetsMgr->setPresetListChangeCameraCallback(boost::bind(&LLPanelPresetsCameraPulldown::populatePanel, this));
	}

	populatePanel();

	return LLPanel::postBuild();
}

void LLPanelPresetsCameraPulldown::populatePanel()
{
	LLPresetsManager::getInstance()->loadPresetNamesFromDir(PRESETS_CAMERA, mPresetNames, DEFAULT_BOTTOM);

	LLScrollListCtrl* scroll = getChild<LLScrollListCtrl>("preset_camera_list");

	if (scroll && mPresetNames.begin() != mPresetNames.end())
	{
		scroll->clearRows();

		std::string active_preset = gSavedSettings.getString("PresetCameraActive");
		if (active_preset == PRESETS_DEFAULT)
		{
			active_preset = LLTrans::getString(PRESETS_DEFAULT);
		}

		for (std::list<std::string>::const_iterator it = mPresetNames.begin(); it != mPresetNames.end(); ++it)
		{
			const std::string& name = *it;
            LL_DEBUGS() << "adding '" << name << "'" << LL_ENDL;
            
			LLSD row;
			row["columns"][0]["column"] = "preset_name";
			row["columns"][0]["value"] = name;

			bool is_selected_preset = false;
			if (name == active_preset)
			{
				row["columns"][1]["column"] = "icon";
				row["columns"][1]["type"] = "icon";
				row["columns"][1]["value"] = "Check_Mark";

				is_selected_preset = true;
			}

			LLScrollListItem* new_item = scroll->addElement(row);
			new_item->setSelected(is_selected_preset);
		}
	}
}

/*virtual*/
void LLPanelPresetsCameraPulldown::onMouseEnter(S32 x, S32 y, MASK mask)
{
	mHoverTimer.stop();
	LLPanel::onMouseEnter(x,y,mask);
}

/*virtual*/
void LLPanelPresetsCameraPulldown::onTopLost()
{
	setVisible(FALSE);
}

/*virtual*/
BOOL LLPanelPresetsCameraPulldown::handleMouseDown(S32 x, S32 y, MASK mask)
{
    LLPanel::handleMouseDown(x,y,mask);
    return TRUE;
}

/*virtual*/
BOOL LLPanelPresetsCameraPulldown::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    LLPanel::handleRightMouseDown(x, y, mask);
    return TRUE;
}

/*virtual*/
BOOL LLPanelPresetsCameraPulldown::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    LLPanel::handleDoubleClick(x, y, mask);
    return TRUE;
}

/*virtual*/
void LLPanelPresetsCameraPulldown::onMouseLeave(S32 x, S32 y, MASK mask)
{
	mHoverTimer.start();
	LLPanel::onMouseLeave(x,y,mask);
}

/*virtual*/ 
void LLPanelPresetsCameraPulldown::onVisibilityChange ( BOOL new_visibility )
{
	if (new_visibility)	
	{
		mHoverTimer.start(); // timer will be stopped when mouse hovers over panel
	}
	else
	{
		mHoverTimer.stop();

	}
}

void LLPanelPresetsCameraPulldown::onRowClick(const LLSD& user_data)
{
	LLScrollListCtrl* scroll = getChild<LLScrollListCtrl>("preset_camera_list");

	if (scroll)
	{
		LLScrollListItem* item = scroll->getFirstSelected();
		if (item)
		{
			std::string name = item->getColumn(1)->getValue().asString();

            LL_DEBUGS() << "selected '" << name << "'" << LL_ENDL;
			LLFloaterCamera::switchToPreset(name);

			setVisible(FALSE);
		}
        else
        {
            LL_DEBUGS() << "none selected" << LL_ENDL;
        }
	}
    else
    {
        LL_DEBUGS() << "no scroll" << LL_ENDL;
    }
}

void LLPanelPresetsCameraPulldown::onViewButtonClick(const LLSD& user_data)
{
	// close the minicontrol, we're bringing up the big one
	setVisible(FALSE);

	LLFloaterReg::toggleInstanceOrBringToFront("camera");
}

//virtual
void LLPanelPresetsCameraPulldown::draw()
{
	F32 alpha = mHoverTimer.getStarted() 
		? clamp_rescale(mHoverTimer.getElapsedTimeF32(), sAutoCloseFadeStartTimeSec, sAutoCloseTotalTimeSec, 1.f, 0.f)
		: 1.0f;
	LLViewDrawContext context(alpha);

	LLPanel::draw();

	if (alpha == 0.f)
	{
		setVisible(FALSE);
	}
}
