/** 
 * @file llconversationview.h
 * @brief Implementation of conversations list widgets and views
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLCONVERSATIONVIEW_H
#define LL_LLCONVERSATIONVIEW_H

#include "llfolderviewitem.h"




#include "llstyle.h"
#include "llcallbackmap.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "lloutputmonitorctrl.h"

class LLIMFloaterContainer;

// Implementation of conversations list session widgets

class LLConversationViewSession : public LLFolderViewFolder
{
public:
	struct Params : public LLInitParam::Block<Params, LLFolderViewItem::Params>
	{
		Optional<LLIMFloaterContainer*>			container;

		Params();
	};
		
protected:
	friend class LLUICtrlFactory;
	LLConversationViewSession( const Params& p );
	
	LLIMFloaterContainer* mContainer;
	
public:
	virtual ~LLConversationViewSession( void ) { }
	virtual void selectItem();	
	void setVisibleIfDetached(BOOL visible);
};

// Implementation of conversations list participant (avatar) widgets

class LLAvatarIconCtrl;

class LLConversationViewParticipant : public LLFolderViewItem
{

public:

    struct Params : public LLInitParam::Block<Params, LLFolderViewItem::Params>
    {
        Optional<LLIMFloaterContainer*>			container;
		Optional<LLButton::Params>				info_button;
        Optional<LLOutputMonitorCtrl::Params>   output_monitor;
        
        Params();
    };

    virtual ~LLConversationViewParticipant( void ) { }
    virtual void draw();
    
protected:
    friend class LLUICtrlFactory;
    LLConversationViewParticipant( const Params& p );
	void initFromParams(const Params& params);
	BOOL postBuild();

	void onInfoBtnClick();
	
private:
	LLButton* createProfileButton();
	LLButton * mInfoBtn;
	

};

#endif // LL_LLCONVERSATIONVIEW_H
