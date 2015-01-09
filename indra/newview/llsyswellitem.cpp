/** 
 * @file llsyswellitem.cpp
 * @brief                                    // TODO
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


#include "llviewerprecompiledheaders.h" // must be first include

#include "llsyswellitem.h"

#include "llwindow.h"
#include "v4color.h"
#include "lltrans.h"
#include "lluicolortable.h"

//---------------------------------------------------------------------------------
LLSysWellItem::LLSysWellItem(const Params& p) : LLPanel(p),
												mTitle(NULL),
												mCloseBtn(NULL)
{
	buildFromFile( "panel_sys_well_item.xml");

	mTitle = getChild<LLTextBox>("title");
	mCloseBtn = getChild<LLButton>("close_btn");

	mTitle->setValue(p.title);
	mCloseBtn->setClickedCallback(boost::bind(&LLSysWellItem::onClickCloseBtn,this));

	mID = p.notification_id;
}

//---------------------------------------------------------------------------------
LLSysWellItem::~LLSysWellItem()
{
}

//---------------------------------------------------------------------------------
void LLSysWellItem::setTitle( std::string title )
{
	mTitle->setValue(title);
}

//---------------------------------------------------------------------------------
void LLSysWellItem::onClickCloseBtn()
{
	mOnItemClose(this);
}

//---------------------------------------------------------------------------------
BOOL LLSysWellItem::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL res = LLPanel::handleMouseDown(x, y, mask);
	if(!mCloseBtn->getRect().pointInRect(x, y))
		mOnItemClick(this);

	return res;
}

//---------------------------------------------------------------------------------
void LLSysWellItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	setTransparentColor(LLUIColorTable::instance().getColor( "SysWellItemSelected" ));
}

//---------------------------------------------------------------------------------
void LLSysWellItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	setTransparentColor(LLUIColorTable::instance().getColor( "SysWellItemUnselected" ));
}

//---------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
LLNotificationTabbedItem::LLNotificationTabbedItem(const Params& p) : LLPanel(p),
    mTitle(NULL),
    mSender(NULL),
    mCloseBtn(NULL)
{
    buildFromFile( "panel_notification_tabbed_item.xml");

    mTitle = getChild<LLTextBox>("GroupName_NoticeTitle");
    mSender = getChild<LLTextBox>("Sender_Resident");
    mTimeBox = getChild<LLTextBox>("Time_Box");
    mGroupIcon = getChild<LLIconCtrl>("group_icon_small");
    mCloseBtn = getChild<LLButton>("close_btn");

    mTitle->setValue(p.title);
    mSender->setValue("Sender: " + p.sender);
    mTimeBox->setValue(buildNotificationDate(p.time_stamp));
    mGroupIcon->setValue(p.group_id);

    mCloseBtn->setClickedCallback(boost::bind(&LLNotificationTabbedItem::onClickCloseBtn,this));

    mID = p.notification_id;
}

//---------------------------------------------------------------------------------
LLNotificationTabbedItem::~LLNotificationTabbedItem()
{
}

//---------------------------------------------------------------------------------
//static
std::string LLNotificationTabbedItem::buildNotificationDate(const LLDate& time_stamp)
{
    std::string timeStr = "[" + LLTrans::getString("LTimeMthNum") + "]/["
        +LLTrans::getString("LTimeDay")+"]/["
        +LLTrans::getString("LTimeYear")+"] ["
        +LLTrans::getString("LTimeHour")+"]:["
        +LLTrans::getString("LTimeMin")+"]";

    LLSD substitution;
    substitution["datetime"] = time_stamp;
    LLStringUtil::format(timeStr, substitution);
    return timeStr;
}

//---------------------------------------------------------------------------------
void LLNotificationTabbedItem::setTitle( std::string title )
{
    mTitle->setValue(title);
}

//---------------------------------------------------------------------------------
void LLNotificationTabbedItem::onClickCloseBtn()
{
    mOnItemClose(this);
}

//---------------------------------------------------------------------------------
BOOL LLNotificationTabbedItem::handleMouseDown(S32 x, S32 y, MASK mask)
{
    BOOL res = LLPanel::handleMouseDown(x, y, mask);
    if(!mCloseBtn->getRect().pointInRect(x, y))
    //if(!mCloseBtn->getRect().pointInRect(x, y))
    //if(!mCloseBtn->getLocalRect().pointInRect(x, y))
        mOnItemClick(this);

    return res;
}

//---------------------------------------------------------------------------------
void LLNotificationTabbedItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
    //setTransparentColor(LLUIColorTable::instance().getColor( "SysWellItemSelected" ));
}

//---------------------------------------------------------------------------------
void LLNotificationTabbedItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
    //setTransparentColor(LLUIColorTable::instance().getColor( "SysWellItemUnselected" ));
}
