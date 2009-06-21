/** 
 * @file llavatarlistitem.h
 * @avatar list item header file
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

#include "llavatariconctrl.h"
#include <llview.h>
#include <llpanel.h>
#include <llfloater.h>
#include <lltextbox.h>
#include <llbutton.h>
#include <lluuid.h>

#include "llfloaterminiinspector.h"

class LLAvatarListItem : public LLPanel 
{
public:
	struct Params :	public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<LLUUID>        	avatar_icon;
		Optional<std::string>		user_name;
		struct avatar_list_item_buttons
		{
			bool    status;
			bool    info;
			bool    profile;
			bool    locator;
			avatar_list_item_buttons() : status(true), info(true), profile(true), locator(true)
			{};
		} buttons;

        Params() : avatar_icon("avatar_icon",LLUUID()), user_name("user_name","")
        {};
	};


	LLAvatarListItem(const Params& p);
	virtual	~LLAvatarListItem();

    void reshape(S32 width, S32 height, BOOL called_from_parent);

	//interface
	void setStatus(int status);
	void setName(std::string name);
	void setAvatar(LLSD& data);
    void needsArrange( void ) {mNeedsArrange = true;} 


	//event handlers
	//mouse
	virtual BOOL handleHover(S32 x, S32 y, MASK mask);
	virtual void onMouseLeave(S32 x, S32 y, MASK mask);
	virtual void onMouseEnter(S32 x, S32 y, MASK mask);
	//buttons
	void onInfoBtnClick();
	void onProfileBtnClick();

private:
    LLAvatarIconCtrl* mAvatar;
    LLIconCtrl* mLocator;
	LLTextBox*	mName;
	LLTextBox*	mStatus;
	LLButton*	mInfo;
	LLButton*	mProfile;

	S32 mYPos;
	S32 mXPos;

	LLFloater*	mini_inspector;
    bool        mNeedsArrange;

    //
    void init(const Params& p);
};
