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


