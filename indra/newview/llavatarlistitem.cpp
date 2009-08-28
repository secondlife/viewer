/** 
 * @file llavatarlistitem.cpp
 * @avatar list item source file
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include "llfloaterreg.h"
#include "llavatarlistitem.h"
#include "llagent.h"



//---------------------------------------------------------------------------------
LLAvatarListItem::LLAvatarListItem(const Params& p) : LLPanel()
{
	mNeedsArrange = false;
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_avatar_list_item.xml");

	mStatus = NULL;
	mInfo = NULL;
	mProfile = NULL;
	mInspector = NULL;

	mAvatar = getChild<LLAvatarIconCtrl>("avatar_icon");
	//mAvatar->setValue(p.avatar_icon);
	mName = getChild<LLTextBox>("name"); 
	//mName->setText(p.user_name);
	
	init(p);

	
}

//---------------------------------------------------------------------------------
void LLAvatarListItem::init(const Params& p)
{
	mLocator = getChild<LLIconCtrl>("locator");

	mStatus = getChild<LLTextBox>("user_status");

	mInfo = getChild<LLButton>("info_btn");
	mInfo->setVisible(false);
	
	mProfile = getChild<LLButton>("profile_btn");
	mProfile->setVisible(false);

	if(!p.buttons.locator)
	{
		mLocator->setVisible(false);
		delete mLocator;
		mLocator = NULL;
	}

	if(!p.buttons.status)
	{
		mStatus->setVisible(false);
		delete mStatus;
		mStatus = NULL;
	}

	if(!p.buttons.info)
	{
		delete mInfo;
		mInfo = NULL;        
	}
	else
	{
		mInfo->setClickedCallback(boost::bind(&LLAvatarListItem::onInfoBtnClick, this));
	}

	if(!p.buttons.profile)
	{
		delete mProfile;
		mProfile = NULL;

		LLRect rect;

		rect.setLeftTopAndSize(mName->getRect().mLeft, mName->getRect().mTop, mName->getRect().getWidth() + 30, mName->getRect().getHeight());
		mName->setRect(rect);

		if(mStatus)
		{
			rect.setLeftTopAndSize(mStatus->getRect().mLeft + 30, mStatus->getRect().mTop, mStatus->getRect().getWidth(), mStatus->getRect().getHeight());
			mStatus->setRect(rect);
		}

		if(mLocator)
		{
			rect.setLeftTopAndSize(mLocator->getRect().mLeft + 30, mLocator->getRect().mTop, mLocator->getRect().getWidth(), mLocator->getRect().getHeight());
			mLocator->setRect(rect);
		}

		if(mInfo)
		{
			rect.setLeftTopAndSize(mInfo->getRect().mLeft + 30, mInfo->getRect().mTop, mInfo->getRect().getWidth(), mInfo->getRect().getHeight());
			mInfo->setRect(rect);
		}
	}
	
}

//---------------------------------------------------------------------------------
void LLAvatarListItem::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	if(!mNeedsArrange)
	{
		LLView::reshape(width, height, called_from_parent);
		return;
	}

	LLRect  rect;
	S32     profile_delta = 0;
	S32     width_delta = getRect().getWidth() - width; 

	if(!mProfile)
	{
		profile_delta = 30;
	}
	else
	{
		rect.setLeftTopAndSize(mProfile->getRect().mLeft - width_delta, mProfile->getRect().mTop, mProfile->getRect().getWidth(), mProfile->getRect().getHeight());
		mProfile->setRect(rect);
	}

	width_delta += profile_delta;

	if(mInfo)
	{
		rect.setLeftTopAndSize(mInfo->getRect().mLeft - width_delta, mInfo->getRect().mTop, mInfo->getRect().getWidth(), mInfo->getRect().getHeight());
		mInfo->setRect(rect);
	}

	if(mLocator)
	{
		rect.setLeftTopAndSize(mLocator->getRect().mLeft - width_delta, mLocator->getRect().mTop, mLocator->getRect().getWidth(), mLocator->getRect().getHeight());
		mLocator->setRect(rect);
	}

	if(mStatus)
	{
		rect.setLeftTopAndSize(mStatus->getRect().mLeft - width_delta, mStatus->getRect().mTop, mStatus->getRect().getWidth(), mStatus->getRect().getHeight());
		mStatus->setRect(rect);
	}

	mNeedsArrange = false;
	LLView::reshape(width, height, called_from_parent);
}

//---------------------------------------------------------------------------------
LLAvatarListItem::~LLAvatarListItem()
{
}
//---------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
BOOL LLAvatarListItem::handleHover(S32 x, S32 y, MASK mask)
{
	mYPos = y;
	mXPos = x;

	return true;
}

//---------------------------------------------------------------------------------
void LLAvatarListItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	setTransparentColor( *(new LLColor4((F32)0.4, (F32)0.4, (F32)0.4)) );

	if(mInfo)
		mInfo->setVisible(true);

	if(mProfile)
		mProfile->setVisible(true);
}

//---------------------------------------------------------------------------------
void LLAvatarListItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	if(mInfo)
	{
		if( mInfo->getRect().pointInRect(x, y) )
			return;

		mInfo->setVisible(false);
	}

	if(mProfile)
	{
		if( mProfile->getRect().pointInRect(x, y) )
			return;
		
		mProfile->setVisible(false);
	}

	setTransparentColor( *(new LLColor4((F32)0.3, (F32)0.3, (F32)0.3)) );
}

//---------------------------------------------------------------------------------
void LLAvatarListItem::setStatus(int status)
{
}

//---------------------------------------------------------------------------------
void LLAvatarListItem::setName(std::string name)
{
}

//---------------------------------------------------------------------------------
void LLAvatarListItem::setAvatar(LLSD& data)
{
}

//---------------------------------------------------------------------------------
void LLAvatarListItem::onInfoBtnClick()
{
	mInspector = LLFloaterReg::showInstance("inspect_avatar", gAgent.getID());

	if (!mInspector)
		return;

	LLRect rect;
	localPointToScreen(mXPos, mYPos, &mXPos, &mYPos);
	

	// *TODO Vadim: rewrite this. "+= -" looks weird.
	S32 delta = mYPos - mInspector->getRect().getHeight();
	if(delta < 0)
	{
		mYPos += -delta;
	}

	rect.setLeftTopAndSize(mXPos, mYPos,
		mInspector->getRect().getWidth(), mInspector->getRect().getHeight()); 
	mInspector->setRect(rect);
	mInspector->setFrontmost(true);
	mInspector->setVisible(true);

}

//---------------------------------------------------------------------------------
void LLAvatarListItem::onProfileBtnClick()
{
}

//---------------------------------------------------------------------------------
