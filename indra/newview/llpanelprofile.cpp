/** 
* @file llpanelprofile.cpp
* @brief Profile panel implementation
*
* $LicenseInfo:firstyear=2009&license=viewergpl$
* 
* Copyright (c) 2009, Linden Research, Inc.
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
#include "llpanelprofile.h"
#include "lltabcontainer.h"
#include "llpanelpicks.h"
#include "llagent.h"

static const std::string PANEL_PICKS = "panel_picks";
static const std::string PANEL_NOTES = "panel_notes";
static const std::string PANEL_PROFILE = "panel_profile";

static LLRegisterPanelClassWrapper<LLPanelAvatarProfile> t_panel_profile(PANEL_PROFILE);
static LLRegisterPanelClassWrapper<LLPanelPicks> t_panel_picks(PANEL_PICKS);


LLPanelProfile::LLPanelProfile()
:	LLPanel(),
	mTabContainer(NULL)
{
}

LLPanelProfile::~LLPanelProfile()
{
}

BOOL LLPanelProfile::postBuild()
{
	mTabContainer = getChild<LLTabContainer>("tabs");
	mTabContainer->setCommitCallback(boost::bind(&LLPanelProfile::onTabSelected, this, _2));

	LLPanelPicks* panel_picks = getChild<LLPanelPicks>(PANEL_PICKS);
	panel_picks->setProfilePanel(this);
	mTabs[PANEL_PICKS] = panel_picks;

	mTabs[PANEL_PROFILE] = getChild<LLPanelAvatarProfile>(PANEL_PROFILE);

	return TRUE;
}

void LLPanelProfile::onOpen(const LLSD& key)
{
	//*NOTE LLUUID::null in this context means Agent related stuff
	LLUUID id(key.has("id") ? key["id"].asUUID() : gAgentID);
	if (key.has("open_tab_name"))
		mTabContainer->selectTabByName(key["open_tab_name"]);

	if(id.notNull() && mAvatarId.notNull() && mAvatarId != id)
	{
		mTabs[PANEL_PROFILE]->clear();
		mTabs[PANEL_PICKS]->clear();
		mTabs[PANEL_NOTES]->clear();
	}

	mAvatarId = id;

	mTabContainer->getCurrentPanel()->onOpen(mAvatarId);
}

//*TODO redo panel toggling
void LLPanelProfile::togglePanel(LLPanel* panel)
{
	// TRUE - we need to open/expand "panel"
	BOOL expand = this->getChildList()->back() != panel;  // mTabContainer->getVisible();

	if (expand)
	{
		//*NOTE on view profile panel along with tabcontainer there is 
		// a backbutton that will be shown when there will be a panel over it even 
		//if that panel has visible backgroud
		setAllChildrenVisible(FALSE);
		
		panel->setVisible(TRUE);
		if (panel->getParent() != this)
		{
			addChildInBack(panel);
		}
		else
		{
			sendChildToBack(panel);
		}

		LLRect new_rect = getRect();
		panel->reshape(new_rect.getWidth(), new_rect.getHeight());
		new_rect.setLeftTopAndSize(0, new_rect.getHeight(), new_rect.getWidth(), new_rect.getHeight());
		panel->setRect(new_rect);
	}
	else 
	{
		this->setAllChildrenVisible(TRUE);
		if (panel->getParent() == this) removeChild(panel);
		sendChildToBack(mTabContainer);
		mTabContainer->getCurrentPanel()->onOpen(mAvatarId);
	}
}


void LLPanelProfile::onTabSelected(const LLSD& param)
{
	std::string tab_name = param.asString();
	if (NULL != mTabs[tab_name])
	{
		mTabs[tab_name]->onOpen(mAvatarId);
	}
}

void LLPanelProfile::setAllChildrenVisible(BOOL visible)
{
	const child_list_t* child_list = getChildList();
	for (child_list_const_iter_t child_it = child_list->begin(); child_it != child_list->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		viewp->setVisible(visible);
	}
}

