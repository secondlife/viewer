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

#include "llavatariconctrl.h"
#include "llbutton.h"
#include "lloutputmonitorctrl.h"

class LLTextBox;
class LLFloaterIMContainer;
class LLConversationViewSession;
class LLConversationViewParticipant;

class LLVoiceClientStatusObserver;

// Implementation of conversations list session widgets

class LLConversationViewSession : public LLFolderViewFolder
{
public:
	struct Params : public LLInitParam::Block<Params, LLFolderViewItem::Params>
	{
		Optional<LLFloaterIMContainer*>			container;

		Params();
	};
		
protected:
	friend class LLUICtrlFactory;
	LLConversationViewSession( const Params& p );
	
	LLFloaterIMContainer* mContainer;
	
public:
	virtual ~LLConversationViewSession();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();
	/*virtual*/ BOOL handleMouseDown( S32 x, S32 y, MASK mask );

	/*virtual*/ S32 arrange(S32* width, S32* height);

	/*virtual*/ void toggleOpen();

	/*virtual*/	bool isMinimized() { return mMinimizedMode; }

	/*virtual*/ void drawOpenFolderArrow(const LLFolderViewItem::Params& default_params, const LLUIColor& fg_color);

	void toggleMinimizedMode(bool is_minimized);

	void setVisibleIfDetached(BOOL visible);
	LLConversationViewParticipant* findParticipant(const LLUUID& participant_id);

	void showVoiceIndicator();

	virtual void refresh();

private:

	void onCurrentVoiceSessionChanged(const LLUUID& session_id);

	LLPanel*				mItemPanel;
	LLPanel*				mCallIconLayoutPanel;
	LLTextBox*				mSessionTitle;
	LLOutputMonitorCtrl*	mSpeakingIndicator;

	bool					mMinimizedMode;

	LLVoiceClientStatusObserver* mVoiceClientObserver;

	boost::signals2::connection mActiveVoiceChannelConnection;
};

// Implementation of conversations list participant (avatar) widgets

class LLConversationViewParticipant : public LLFolderViewItem
{

public:

	struct Params : public LLInitParam::Block<Params, LLFolderViewItem::Params>
	{
        Optional<LLFloaterIMContainer*>			container;
		Optional<LLUUID>	                    participant_id;
        Optional<LLAvatarIconCtrl::Params>	    avatar_icon;
		Optional<LLButton::Params>				info_button;
        Optional<LLOutputMonitorCtrl::Params>   output_monitor;
		
		Params();
	};
	
    virtual ~LLConversationViewParticipant( void ) { }

    bool hasSameValue(const LLUUID& uuid) { return (uuid == mUUID); }
    virtual void refresh();
    void addToFolder(LLFolderViewFolder* folder);
	void addToSession(const LLUUID& session_id);

    /*virtual*/ BOOL handleMouseDown( S32 x, S32 y, MASK mask );

    void onMouseEnter(S32 x, S32 y, MASK mask);
    void onMouseLeave(S32 x, S32 y, MASK mask);

    /*virtual*/ S32 getLabelXPos();

protected:
	friend class LLUICtrlFactory;
	LLConversationViewParticipant( const Params& p );
	void initFromParams(const Params& params);
	BOOL postBuild();
    /*virtual*/ void draw();
    /*virtual*/ S32 arrange(S32* width, S32* height);
	
	void onInfoBtnClick();

private:
    LLAvatarIconCtrl* mAvatarIcon;
	LLButton * mInfoBtn;
    LLOutputMonitorCtrl* mSpeakingIndicator;
	LLUUID mUUID;		// UUID of the participant

    typedef enum e_avatar_item_child {
        ALIC_SPEAKER_INDICATOR,
        ALIC_INFO_BUTTON,
        ALIC_COUNT,
    } EAvatarListItemChildIndex;

    static bool	sStaticInitialized; // this variable is introduced to improve code readability
    static S32 sChildrenWidths[ALIC_COUNT];
    static void initChildrenWidths(LLConversationViewParticipant* self);
    void updateChildren();
    LLView* getItemChildView(EAvatarListItemChildIndex child_view_index);
};

#endif // LL_LLCONVERSATIONVIEW_H
