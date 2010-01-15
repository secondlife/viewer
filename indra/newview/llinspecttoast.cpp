/** 
 * @file llinspecttoast.cpp
 * @brief Toast inspector implementation.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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
private:
	void onToastDestroy(LLToast * toast);

private:
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

void LLInspectToast::onOpen(const LLSD& notification_id)
{
	LLInspect::onOpen(notification_id);
	LLToast* toast = mScreenChannel->getToastByNotificationID(notification_id);
	if (toast == NULL)
	{
		llwarns << "Could not get requested toast  from screen channel." << llendl;
		return;
	}
	toast->setOnToastDestroyedCallback(boost::bind(&LLInspectToast::onToastDestroy, this, _1));

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

void LLInspectToast::onToastDestroy(LLToast * toast)
{
	closeFloater(false);
}

void LLNotificationsUI::registerFloater()
{
	LLFloaterReg::add("inspect_toast", "inspect_toast.xml",
			&LLFloaterReg::build<LLInspectToast>);
}

