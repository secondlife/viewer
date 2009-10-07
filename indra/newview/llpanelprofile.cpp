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

#include "llagent.h"
#include "llavataractions.h"
#include "llfloaterreg.h"
#include "llcommandhandler.h"
#include "llpanelpicks.h"
#include "lltabcontainer.h"

static const std::string PANEL_PICKS = "panel_picks";
static const std::string PANEL_PROFILE = "panel_profile";

class LLAgentHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLAgentHandler() : LLCommandHandler("agent", UNTRUSTED_THROTTLE) { }

	bool handle(const LLSD& params, const LLSD& query_map,
		LLMediaCtrl* web)
	{
		if (params.size() < 2) return false;
		LLUUID avatar_id;
		if (!avatar_id.set(params[0], FALSE))
		{
			return false;
		}

		if (params[1].asString() == "about")
		{
			LLAvatarActions::showProfile(avatar_id);
			return true;
		}

		if (params[1].asString() == "inspect")
		{
			LLFloaterReg::showInstance("inspect_avatar", LLSD().insert("avatar_id", avatar_id));
			return true;
		}
		return false;
	}
};
LLAgentHandler gAgentHandler;


LLPanelProfile::LLPanelProfile()
 : LLPanel()
 , mTabCtrl(NULL)
 , mAvatarId(LLUUID::null)
{
}

BOOL LLPanelProfile::postBuild()
{
	mTabCtrl = getChild<LLTabContainer>("tabs");

	getTabCtrl()->setCommitCallback(boost::bind(&LLPanelProfile::onTabSelected, this, _2));

	LLPanelPicks* panel_picks = getChild<LLPanelPicks>(PANEL_PICKS);
	panel_picks->setProfilePanel(this);

	getTabContainer()[PANEL_PICKS] = panel_picks;
	getTabContainer()[PANEL_PROFILE] = getChild<LLPanelAvatarProfile>(PANEL_PROFILE);

	return TRUE;
}

void LLPanelProfile::onOpen(const LLSD& key)
{
	if (key.has("open_tab_name"))
	{
		getTabContainer()[PANEL_PICKS]->onClosePanel();

		// onOpen from selected panel will be called from onTabSelected callback
		getTabCtrl()->selectTabByName(key["open_tab_name"]);
	}
	else
	{
		getTabCtrl()->getCurrentPanel()->onOpen(getAvatarId());
	}
}

//*TODO redo panel toggling
void LLPanelProfile::togglePanel(LLPanel* panel)
{
	// TRUE - we need to open/expand "panel"
	bool expand = getChildList()->back() != panel;  // mTabCtrl->getVisible();

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
		panel->setVisible(FALSE);
		if (panel->getParent() == this) 
		{
			removeChild(panel);
		}
		sendChildToBack(getTabCtrl());
		getTabCtrl()->getCurrentPanel()->onOpen(getAvatarId());
	}
}

void LLPanelProfile::onTabSelected(const LLSD& param)
{
	std::string tab_name = param.asString();
	if (NULL != getTabContainer()[tab_name])
	{
		getTabContainer()[tab_name]->onOpen(getAvatarId());
	}
}

void LLPanelProfile::setAllChildrenVisible(BOOL visible)
{
	const child_list_t* child_list = getChildList();
	child_list_const_iter_t child_it = child_list->begin();
	for (; child_it != child_list->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		viewp->setVisible(visible);
	}
}

