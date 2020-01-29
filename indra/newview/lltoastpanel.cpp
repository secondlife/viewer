/** 
 * @file lltoastpanel.cpp
 * @brief Creates a panel of a specific kind for a toast
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "lldbstrings.h"
#include "llcheckboxctrl.h"
#include "llpanelgenerictip.h"
#include "llpanelonlinestatus.h"
#include "llnotifications.h"
#include "lltoastnotifypanel.h"
#include "lltoastpanel.h"
#include "lltoastscriptquestion.h"

#include <boost/algorithm/string.hpp>

//static
const S32 LLToastPanel::MIN_PANEL_HEIGHT = 40; // VPAD(4)*2 + ICON_HEIGHT(32)
// 'magic numbers', consider initializing (512+20) part from xml/notifications
const S32 LLToastPanel::MAX_TEXT_LENGTH = 512 + 20 + DB_FIRST_NAME_BUF_SIZE + DB_LAST_NAME_BUF_SIZE + DB_INV_ITEM_NAME_BUF_SIZE;

LLToastPanel::LLToastPanel(const LLNotificationPtr& notification)
{
	mNotification = notification;
}

LLToastPanel::~LLToastPanel() 
{
}

//virtual
std::string LLToastPanel::getTitle()
{
	// *TODO: create Title and localize it. If it will be required.
	return mNotification->getMessage();
}

//virtual
const std::string& LLToastPanel::getNotificationName()
{
	return mNotification->getName();
}

//virtual
const LLUUID& LLToastPanel::getID()
{
	return mNotification->id();
}

S32 LLToastPanel::computeSnappedToMessageHeight(LLTextBase* message, S32 maxLineCount)
{
	S32 heightDelta = 0;
	S32 maxTextHeight = message->getFont()->getLineHeight() * maxLineCount;

	LLRect messageRect = message->getRect();
	S32 oldTextHeight = messageRect.getHeight();

	//Knowing the height is set to max allowed, getTextPixelHeight returns needed text height
	//Perhaps we need to pass maxLineCount as parameter to getTextPixelHeight to avoid previous reshape.
	S32 requiredTextHeight = message->getTextBoundingRect().getHeight();
	S32 newTextHeight = llmin(requiredTextHeight, maxTextHeight);

	heightDelta = newTextHeight - oldTextHeight;
	S32 new_panel_height = llmax(getRect().getHeight() + heightDelta, MIN_PANEL_HEIGHT);

	return new_panel_height;
}

//snap to the message height if it is visible
void LLToastPanel::snapToMessageHeight(LLTextBase* message, S32 maxLineCount)
{
	if(!message)
	{
		return;
	}

	//Add message height if it is visible
	if (message->getVisible())
	{
		S32 new_panel_height = computeSnappedToMessageHeight(message, maxLineCount);

		//reshape the panel with new height
		if (new_panel_height != getRect().getHeight())
		{
			reshape( getRect().getWidth(), new_panel_height);
		}
	}
}

// static
LLToastPanel* LLToastPanel::buidPanelFromNotification(
		const LLNotificationPtr& notification)
{
	LLToastPanel* res = NULL;

	//process tip toast panels
	if ("notifytip" == notification->getType())
	{
		// if it is online/offline notification
		if ("FriendOnlineOffline" == notification->getName())
		{
			res = new LLPanelOnlineStatus(notification);
		}
		// in all other case we use generic tip panel
		else
		{
			res = new LLPanelGenericTip(notification);
		}
	}
	else if("notify" == notification->getType())
	{
		if (notification->getPriority() == NOTIFICATION_PRIORITY_CRITICAL)
		{
			res = new LLToastScriptQuestion(notification);
		}
		else
		{
			res = new LLToastNotifyPanel(notification);
		}
	}
	/*
	 else if(...)
	 create all other specific non-public toast panel
	 */

	return res;
}

LLCheckBoxToastPanel::LLCheckBoxToastPanel(const LLNotificationPtr& p_ntf)
: LLToastPanel(p_ntf),
mCheck(NULL)
{

}

void LLCheckBoxToastPanel::setCheckBoxes(const S32 &h_pad, const S32 &v_pad, LLView *parent_view)
{
    std::string ignore_label;
    LLNotificationFormPtr form = mNotification->getForm();

    if (form->getIgnoreType() == LLNotificationForm::IGNORE_CHECKBOX_ONLY)
    {
        // Normally text is only used to describe notification in preferences, 
        // but this one is not displayed in preferences and works on case by case
        // basis.
        // Display text if present, display 'always chose' if not.
        std::string ignore_message = form->getIgnoreMessage();
        if (ignore_message.empty())
        {
            ignore_message = LLNotifications::instance().getGlobalString("alwayschoose");
        }
        setCheckBox(ignore_message, ignore_label, boost::bind(&LLCheckBoxToastPanel::onCommitCheckbox, this, _1), h_pad, v_pad, parent_view);
    }
    else if (form->getIgnoreType() == LLNotificationForm::IGNORE_WITH_DEFAULT_RESPONSE)
    {
        setCheckBox(LLNotifications::instance().getGlobalString("skipnexttime"), ignore_label, boost::bind(&LLCheckBoxToastPanel::onCommitCheckbox, this, _1), h_pad, v_pad, parent_view);
    }
    if (form->getIgnoreType() == LLNotificationForm::IGNORE_WITH_DEFAULT_RESPONSE_SESSION_ONLY)
    {
        setCheckBox(LLNotifications::instance().getGlobalString("skipnexttimesessiononly"), ignore_label, boost::bind(&LLCheckBoxToastPanel::onCommitCheckbox, this, _1), h_pad, v_pad, parent_view);
    }
    else if (form->getIgnoreType() == LLNotificationForm::IGNORE_WITH_LAST_RESPONSE)
    {
        setCheckBox(LLNotifications::instance().getGlobalString("alwayschoose"), ignore_label, boost::bind(&LLCheckBoxToastPanel::onCommitCheckbox, this, _1), h_pad, v_pad, parent_view);
    }
}

bool LLCheckBoxToastPanel::setCheckBox(const std::string& check_title,
                                       const std::string& check_control,
                                       const commit_signal_t::slot_type& cb,
                                       const S32 &h_pad,
                                       const S32 &v_pad,
                                       LLView *parent_view)
{
    mCheck = LLUICtrlFactory::getInstance()->createFromFile<LLCheckBoxCtrl>("alert_check_box.xml", this, LLPanel::child_registry_t::instance());

    if (!mCheck)
    {
        return false;
    }

    const LLFontGL* font = mCheck->getFont();
    const S32 LINE_HEIGHT = font->getLineHeight();

    std::vector<std::string> lines;
    boost::split(lines, check_title, boost::is_any_of("\n"));

    // Extend dialog for "check next time"
    S32 max_msg_width = LLToastPanel::getRect().getWidth() - 2 * h_pad;
    S32 check_width = S32(font->getWidth(lines[0]) + 0.99f) + 16; // use width of the first line
    max_msg_width = llmax(max_msg_width, check_width);
    S32 dialog_width = max_msg_width + 2 * h_pad;

    S32 dialog_height = LLToastPanel::getRect().getHeight();
    dialog_height += LINE_HEIGHT * lines.size();
    dialog_height += LINE_HEIGHT / 2;

    LLToastPanel::reshape(dialog_width, dialog_height, FALSE);

    S32 msg_x = (LLToastPanel::getRect().getWidth() - max_msg_width) / 2;

    // set check_box's attributes
    LLRect check_rect;
    // if we are part of the toast, we need to leave space for buttons
    S32 msg_y = v_pad + (parent_view ? 0 : (BTN_HEIGHT + LINE_HEIGHT / 2));
    mCheck->setRect(check_rect.setOriginAndSize(msg_x, msg_y, max_msg_width, LINE_HEIGHT*lines.size()));
    mCheck->setLabel(check_title);
    mCheck->setCommitCallback(cb);

    if (parent_view)
    {
        // assume that width and height autoadjusts to toast
        parent_view->addChild(mCheck);
    }
    else
    {
        LLToastPanel::addChild(mCheck);
    }

    return true;
}

void LLCheckBoxToastPanel::onCommitCheckbox(LLUICtrl* ctrl)
{
    BOOL check = ctrl->getValue().asBoolean();
    if (mNotification->getForm()->getIgnoreType() == LLNotificationForm::IGNORE_SHOW_AGAIN)
    {
        // question was "show again" so invert value to get "ignore"
        check = !check;
    }
    mNotification->setIgnored(check);
}
