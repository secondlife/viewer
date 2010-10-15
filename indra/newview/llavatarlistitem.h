/** 
 * @file llavatarlistitem.h
 * @brief avatar list item header file
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

#ifndef LL_LLAVATARLISTITEM_H
#define LL_LLAVATARLISTITEM_H

#include "llpanel.h"
#include "lloutputmonitorctrl.h"
#include "llbutton.h"
#include "lltextbox.h"
#include "llstyle.h"

#include "llcallingcard.h" // for LLFriendObserver

class LLAvatarIconCtrl;
class LLAvatarName;
class LLIconCtrl;

class LLAvatarListItem : public LLPanel, public LLFriendObserver
{
public:
	struct Params : public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<LLStyle::Params>	default_style,
									voice_call_invited_style,
									voice_call_joined_style,
									voice_call_left_style,
									online_style,
									offline_style;

		Params();
	};

	typedef enum e_item_state_type {
		IS_DEFAULT,
		IS_VOICE_INVITED,
		IS_VOICE_JOINED,
		IS_VOICE_LEFT,
		IS_ONLINE,
		IS_OFFLINE,
	} EItemState;

	/**
	 * Creates an instance of LLAvatarListItem.
	 *
	 * It is not registered with LLDefaultChildRegistry. It is built via LLUICtrlFactory::buildPanel
	 * or via registered LLCallbackMap depend on passed parameter.
	 * 
	 * @param not_from_ui_factory if true instance will be build with LLUICtrlFactory::buildPanel 
	 * otherwise it should be registered via LLCallbackMap before creating.
	 */
	LLAvatarListItem(bool not_from_ui_factory = true);
	virtual ~LLAvatarListItem();

	virtual BOOL postBuild();

	/**
	 * Processes notification from speaker indicator to update children when indicator's visibility is changed.
	 */
	virtual S32	notifyParent(const LLSD& info);
	virtual void onMouseLeave(S32 x, S32 y, MASK mask);
	virtual void onMouseEnter(S32 x, S32 y, MASK mask);
	virtual void setValue(const LLSD& value);
	virtual void changed(U32 mask); // from LLFriendObserver

	void setOnline(bool online);
	void updateAvatarName(); // re-query the name cache
	void setAvatarName(const std::string& name);
	void setAvatarToolTip(const std::string& tooltip);
	void setHighlight(const std::string& highlight);
	void setState(EItemState item_style);
	void setAvatarId(const LLUUID& id, const LLUUID& session_id, bool ignore_status_changes = false, bool is_resident = true);
	void setLastInteractionTime(U32 secs_since);
	//Show/hide profile/info btn, translating speaker indicator and avatar name coordinates accordingly
	void setShowProfileBtn(bool show);
	void setShowInfoBtn(bool show);
	void showSpeakingIndicator(bool show);
	void setShowPermissions(bool show) { mShowPermissions = show; };
	void showLastInteractionTime(bool show);
	void setAvatarIconVisible(bool visible);
	
	const LLUUID& getAvatarId() const;
	std::string getAvatarName() const;
	std::string getAvatarToolTip() const;

	void onInfoBtnClick();
	void onProfileBtnClick();

	/*virtual*/ BOOL handleDoubleClick(S32 x, S32 y, MASK mask);

protected:
	/**
	 * Contains indicator to show voice activity. 
	 */
	LLOutputMonitorCtrl* mSpeakingIndicator;

	LLAvatarIconCtrl* mAvatarIcon;

	/// Indicator for permission to see me online.
	LLIconCtrl* mIconPermissionOnline;
	/// Indicator for permission to see my position on the map.
	LLIconCtrl* mIconPermissionMap;
	/// Indicator for permission to edit my objects.
	LLIconCtrl* mIconPermissionEditMine;
	/// Indicator for permission to edit their objects.
	LLIconCtrl* mIconPermissionEditTheirs;

private:

	typedef enum e_online_status {
		E_OFFLINE,
		E_ONLINE,
		E_UNKNOWN,
	} EOnlineStatus;

	/**
	 * Enumeration of item elements in order from right to left.
	 * 
	 * updateChildren() assumes that indexes are in the such order to process avatar icon easier.
	 *
	 * @see updateChildren()
	 */
	typedef enum e_avatar_item_child {
		ALIC_SPEAKER_INDICATOR,
		ALIC_PROFILE_BUTTON,
		ALIC_INFO_BUTTON,
		ALIC_PERMISSION_ONLINE,
		ALIC_PERMISSION_MAP,
		ALIC_PERMISSION_EDIT_MINE,
		ALIC_PERMISSION_EDIT_THEIRS,
		ALIC_INTERACTION_TIME,
		ALIC_NAME,
		ALIC_ICON,
		ALIC_COUNT,
	} EAvatarListItemChildIndex;

	void setNameInternal(const std::string& name, const std::string& highlight);
	void onAvatarNameCache(const LLAvatarName& av_name);

	std::string formatSeconds(U32 secs);

	typedef std::map<EItemState, LLColor4> icon_color_map_t;
	static icon_color_map_t& getItemIconColorMap();

	/**
	 * Initializes widths of all children to use them while changing visibility of any of them.
	 *
	 * @see updateChildren()
	 */
	static void initChildrenWidths(LLAvatarListItem* self);

	/**
	 * Updates position and rectangle of visible children to fit all available item's width.
	 */
	void updateChildren();

	/**
	 * Update visibility of active permissions icons.
	 *
	 * Need to call updateChildren() afterwards to sort out their layout.
	 */
	bool showPermissions(bool visible);

	/**
	 * Gets child view specified by index.
	 *
	 * This method implemented via switch by all EAvatarListItemChildIndex values.
	 * It is used to not store children in array or vector to avoid of increasing memory usage.
	 */
	LLView* getItemChildView(EAvatarListItemChildIndex child_index);

	LLTextBox* mAvatarName;
	LLTextBox* mLastInteractionTime;
	LLStyle::Params mAvatarNameStyle;
	
	LLButton* mInfoBtn;
	LLButton* mProfileBtn;

	LLUUID mAvatarId;
	std::string mHighlihtSubstring; // substring to highlight
	EOnlineStatus mOnlineStatus;
	//Flag indicating that info/profile button shouldn't be shown at all.
	//Speaker indicator and avatar name coords are translated accordingly
	bool mShowInfoBtn;
	bool mShowProfileBtn;

	/// indicates whether to show icons representing permissions granted
	bool mShowPermissions;

	/// true when the mouse pointer is hovering over this item
	bool mHovered;

	static bool	sStaticInitialized; // this variable is introduced to improve code readability
	static S32  sLeftPadding; // padding to first left visible child (icon or name)
	static S32  sRightNamePadding; // right padding from name to next visible child

	/**
	 * Contains widths of each child specified by EAvatarListItemChildIndex
	 * including padding to the next right one.
	 *
	 * @see initChildrenWidths()
	 */
	static S32 sChildrenWidths[ALIC_COUNT];

};

#endif //LL_LLAVATARLISTITEM_H
