/** 
 * @file llpanelnearbymedia.h
 * @brief Management interface for muting and controlling nearby media
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#ifndef LL_LLPANELNEARBYMEDIA_H
#define LL_LLPANELNEARBYMEDIA_H

#include "llpanelpulldown.h"

class LLPanelNearbyMedia;
class LLButton;
class LLScrollListCtrl;
class LLSlider;
class LLSliderCtrl;
class LLCheckBoxCtrl;
class LLTextBox;
class LLToggleableMenu;
class LLComboBox;
class LLViewerMediaImpl;

class LLPanelNearByMedia : public LLPanelPulldown
{
public:
	
	bool postBuild() override;
	void draw() override;
	void reshape(S32 width, S32 height, bool called_from_parent) override;
	bool handleHover(S32 x, S32 y, MASK mask) override;
	bool handleRightMouseDown(S32 x, S32 y, MASK mask) override;
    void onVisibilityChange(bool new_visibility) override;

	// this is part of the nearby media *dialog* so we can track whether
	// the user *implicitly* wants audio on or off via their *explicit*
	// interaction with our buttons.
	bool getParcelAudioAutoStart();

	// callback for when the auto play media preference changes
	// to update mParcelAudioAutoStart
	void handleMediaAutoPlayChanged(const LLSD& newvalue);

	LLPanelNearByMedia();
	virtual ~LLPanelNearByMedia();

private:
	
	enum ColumnIndex {
		CHECKBOX_COLUMN = 0,
		PROXIMITY_COLUMN = 1,
		VISIBILITY_COLUMN = 2,
		CLASS_COLUMN = 3,
		NAME_COLUMN = 4,
		DEBUG_COLUMN = 5
	};
	
	// Media "class" enumeration
	enum MediaClass {
		MEDIA_CLASS_ALL = 0,
		MEDIA_CLASS_FOCUSED = 1,
		MEDIA_CLASS_WITHIN_PARCEL = 2,
		MEDIA_CLASS_OUTSIDE_PARCEL = 3,
		MEDIA_CLASS_ON_OTHERS = 4
	};
		
	// Add/remove an LLViewerMediaImpl to/from the list
	LLScrollListItem* addListItem(const LLUUID &id);
	void updateListItem(LLScrollListItem* item, LLViewerMediaImpl* impl);
	void updateListItem(LLScrollListItem* item,
						const std::string &item_name,
						const std::string &item_tooltip,
						S32 proximity,
						bool is_disabled,
						bool has_media,
						bool is_time_based_and_playing,
						MediaClass media_class,
						const std::string &debug_str);
	void removeListItem(const LLUUID &id);
	
	// Refresh the list in the UI
	void refreshList();
	
	void refreshParcelItems();

	// UI Callbacks 
	void onClickEnableAll();
	void onClickDisableAll();
	void onClickEnableParcelMedia();
	void onClickDisableParcelMedia();
	void onClickParcelMediaPlay();
	void onClickParcelMediaStop();
	void onClickParcelMediaPause();
	void onClickParcelAudioPlay();
	void onClickParcelAudioStop();
	void onClickParcelAudioPause();
	void onAdvancedButtonClick();	
	void onMoreLess();	
	
	void onCheckItem(LLUICtrl* ctrl, const LLUUID &row_id);
	
	static void onZoomMedia(void* user_data);
	
private:
	bool setDisabled(const LLUUID &id, bool disabled);

	static void getNameAndUrlHelper(LLViewerMediaImpl* impl, std::string& name, std::string & url, const std::string &defaultName);
    std::string getSelectedUrl();
	
	void updateColumns();
	
	bool shouldShow(LLViewerMediaImpl* impl);
	
	void showBasicControls(bool playing, bool include_zoom, bool is_zoomed, bool muted, F32 volume);
	void showTimeBasedControls(bool playing, bool include_zoom, bool is_zoomed, bool muted, F32 volume);
	void showDisabledControls();
	void updateControls();
	
	void onClickSelectedMediaStop();
	void onClickSelectedMediaPlay();
	void onClickSelectedMediaPause();
	void onClickSelectedMediaMute();
	void onCommitSelectedMediaVolume();
	void onClickSelectedMediaZoom();
	void onClickSelectedMediaUnzoom();
    void onMenuAction(const LLSD& userdata);
    bool onMenuVisible(const LLSD& userdata);
	
	LLUICtrl*			mNearbyMediaPanel;
	LLScrollListCtrl*		mMediaList;
	LLUICtrl*			mEnableAllCtrl;
	LLUICtrl*			mDisableAllCtrl;
	LLComboBox*			mShowCtrl;
	
	// Dynamic (selection-dependent) controls
	LLUICtrl*			mStopCtrl;
	LLUICtrl*			mPlayCtrl;
	LLUICtrl*			mPauseCtrl;
	LLUICtrl*			mMuteCtrl;
	LLUICtrl*			mVolumeSliderCtrl;
	LLUICtrl*			mZoomCtrl;
	LLUICtrl*			mUnzoomCtrl;
	LLSlider*			mVolumeSlider;
	LLButton*			mMuteBtn;
	LLButton*			mMoreLessBtn;
	
	bool				mDebugInfoVisible;
	bool				mParcelAudioAutoStart;
	std::string			mEmptyNameString;
	std::string			mPlayingString;
	std::string			mParcelMediaName;
	std::string			mParcelAudioName;
	
	LLRect				mMoreRect;
	LLRect				mLessRect;
	LLScrollListItem*	mParcelMediaItem;
	LLScrollListItem*	mParcelAudioItem;
	LLToggleableMenu*	mContextMenu;
};


#endif // LL_LLPANELNEARBYMEDIA_H
