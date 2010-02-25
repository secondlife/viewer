/** 
 * @file llsyswellitem.cpp
 * @brief                                    // TODO
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#include "llsyswellitem.h"

#include "llwindow.h"
#include "v4color.h"
#include "lluicolortable.h"

//---------------------------------------------------------------------------------
LLSysWellItem::LLSysWellItem(const Params& p) : LLPanel(p),
												mTitle(NULL),
												mCloseBtn(NULL),
												mIcon(NULL)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_sys_well_item.xml");

	mTitle = getChild<LLTextBox>("title");
	mCloseBtn = getChild<LLButton>("close_btn");
	mIcon = getChild<LLIconCtrl>("icon");

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


