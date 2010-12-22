/** 
 * @file llinspecttoast.cpp
 * @brief Toast inspector implementation.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h" // must be first include

#include "llinspecttoast.h"
#include "llinspect.h"
#include "llfloaterreg.h"
#include "llscreenchannel.h"
#include "llchannelmanager.h"
#include "lltransientfloatermgr.h"

using namespace LLNotificationsUI;

/**
 * Represents inspectable toast .
 */
class LLInspectToast: public LLInspect
{
public:

	LLInspectToast(const LLSD& notification_idl);
	virtual ~LLInspectToast();

	/*virtual*/ void onOpen(const LLSD& notification_id);
	/*virtual*/ BOOL handleToolTip(S32 x, S32 y, MASK mask);
private:
	void onToastDestroy(LLToast * toast);

	boost::signals2::scoped_connection mConnection;
	LLPanel* mPanel;
	LLScreenChannel* mScreenChannel;
};

LLInspectToast::LLInspectToast(const LLSD& notification_id) :
	LLInspect(LLSD()), mPanel(NULL)
{
	LLScreenChannelBase* channel = LLChannelManager::getInstance()->findChannelByID(
																LLUUID(gSavedSettings.getString("NotificationChannelUUID")));
	mScreenChannel = dynamic_cast<LLScreenChannel*>(channel);
	if(NULL == mScreenChannel)
	{
		llwarns << "Could not get requested screen channel." << llendl;
		return;
	}

	LLTransientFloaterMgr::getInstance()->addControlView(this);
}
LLInspectToast::~LLInspectToast()
{
	LLTransientFloaterMgr::getInstance()->removeControlView(this);
}

// virtual
void LLInspectToast::onOpen(const LLSD& notification_id)
{
	LLInspect::onOpen(notification_id);
	LLToast* toast = mScreenChannel->getToastByNotificationID(notification_id);
	if (toast == NULL)
	{
		llwarns << "Could not get requested toast  from screen channel." << llendl;
		return;
	}
	mConnection = toast->setOnToastDestroyedCallback(boost::bind(&LLInspectToast::onToastDestroy, this, _1));

	LLPanel * panel = toast->getPanel();
	panel->setVisible(TRUE);
	panel->setMouseOpaque(FALSE);
	if(mPanel != NULL && mPanel->getParent() == this)
	{
		removeChild(mPanel);
	}
	addChild(panel);
	panel->setFocus(TRUE);
	mPanel = panel;


	LLRect panel_rect;
	panel_rect = panel->getRect();
	reshape(panel_rect.getWidth(), panel_rect.getHeight());

	LLUI::positionViewNearMouse(this);
}

// virtual
BOOL LLInspectToast::handleToolTip(S32 x, S32 y, MASK mask)
{
	// We don't like the way LLInspect handles tooltips
	// (black tooltips look weird),
	// so force using the default implementation (STORM-511).
	return LLFloater::handleToolTip(x, y, mask);
}

void LLInspectToast::onToastDestroy(LLToast * toast)
{
	closeFloater(false);
}

void LLNotificationsUI::registerFloater()
{
	LLFloaterReg::add("inspect_toast", "inspect_toast.xml",
			&LLFloaterReg::build<LLInspectToast>);
}

