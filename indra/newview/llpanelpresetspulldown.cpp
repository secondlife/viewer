/** 
 * @file llpanelpresetspulldown.cpp
 * @brief A panel showing a quick way to pick presets
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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

#include "llpanelpresetspulldown.h"

#include "llviewercontrol.h"
#include "llstatusbar.h"

#include "llbutton.h"
#include "lltabcontainer.h"
#include "llfloaterreg.h"
#include "llfloaterpreference.h"
#include "llpresetsmanager.h"
#include "llsliderctrl.h"
#include "llscrolllistctrl.h"
#include "lltrans.h"

/* static */ const F32 LLPanelPresetsPulldown::sAutoCloseFadeStartTimeSec = 2.0f;
/* static */ const F32 LLPanelPresetsPulldown::sAutoCloseTotalTimeSec = 3.0f;

///----------------------------------------------------------------------------
/// Class LLPanelPresetsPulldown
///----------------------------------------------------------------------------

// Default constructor
LLPanelPresetsPulldown::LLPanelPresetsPulldown()
{
	mHoverTimer.stop();

	mCommitCallbackRegistrar.add("Presets.GoGraphicsPrefs", boost::bind(&LLPanelPresetsPulldown::onGraphicsButtonClick, this, _2));
	mCommitCallbackRegistrar.add("Presets.RowClick", boost::bind(&LLPanelPresetsPulldown::onRowClick, this, _2));

	buildFromFile( "panel_presets_pulldown.xml");
}

BOOL LLPanelPresetsPulldown::postBuild()
{
	LLPresetsManager* presetsMgr = LLPresetsManager::getInstance();
    presetsMgr->setPresetListChangeCallback(boost::bind(&LLPanelPresetsPulldown::populatePanel, this));
	// Make sure there is a default preference file
    presetsMgr->createMissingDefault();

	populatePanel();

	return LLPanel::postBuild();
}

void LLPanelPresetsPulldown::populatePanel()
{
	std::string presets_dir = LLPresetsManager::getInstance()->getPresetsDir(PRESETS_GRAPHIC);
	LLPresetsManager::getInstance()->loadPresetNamesFromDir(presets_dir, mPresetNames, DEFAULT_TOP);

	LLScrollListCtrl* scroll = getChild<LLScrollListCtrl>("preset_list");

	if (scroll && mPresetNames.begin() != mPresetNames.end())
	{
		scroll->clearRows();

		std::string active_preset = gSavedSettings.getString("PresetGraphicActive");
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
void LLPanelPresetsPulldown::onMouseEnter(S32 x, S32 y, MASK mask)
{
	mHoverTimer.stop();
	LLPanel::onMouseEnter(x,y,mask);
}

/*virtual*/
void LLPanelPresetsPulldown::onTopLost()
{
	setVisible(FALSE);
}

/*virtual*/
BOOL LLPanelPresetsPulldown::handleMouseDown(S32 x, S32 y, MASK mask)
{
    LLPanel::handleMouseDown(x,y,mask);
    return TRUE;
}

/*virtual*/
BOOL LLPanelPresetsPulldown::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    LLPanel::handleRightMouseDown(x, y, mask);
    return TRUE;
}

/*virtual*/
BOOL LLPanelPresetsPulldown::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    LLPanel::handleDoubleClick(x, y, mask);
    return TRUE;
}

/*virtual*/
void LLPanelPresetsPulldown::onMouseLeave(S32 x, S32 y, MASK mask)
{
	mHoverTimer.start();
	LLPanel::onMouseLeave(x,y,mask);
}

/*virtual*/ 
void LLPanelPresetsPulldown::onVisibilityChange ( BOOL new_visibility )
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

void LLPanelPresetsPulldown::onRowClick(const LLSD& user_data)
{
	LLScrollListCtrl* scroll = getChild<LLScrollListCtrl>("preset_list");

	if (scroll)
	{
		LLScrollListItem* item = scroll->getFirstSelected();
		if (item)
		{
			std::string name = item->getColumn(1)->getValue().asString();

            LL_DEBUGS() << "selected '" << name << "'" << LL_ENDL;
			LLPresetsManager::getInstance()->loadPreset(PRESETS_GRAPHIC, name);

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

void LLPanelPresetsPulldown::onGraphicsButtonClick(const LLSD& user_data)
{
	// close the minicontrol, we're bringing up the big one
	setVisible(FALSE);

	// bring up the prefs floater
	LLFloater* prefsfloater = LLFloaterReg::showInstance("preferences");
	if (prefsfloater)
	{
		// grab the 'graphics' panel from the preferences floater and
		// bring it the front!
		LLTabContainer* tabcontainer = prefsfloater->getChild<LLTabContainer>("pref core");
		LLPanel* graphicspanel = prefsfloater->getChild<LLPanel>("display");
		if (tabcontainer && graphicspanel)
		{
			tabcontainer->selectTabPanel(graphicspanel);
		}
	}
}

//virtual
void LLPanelPresetsPulldown::draw()
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
