/** 
 * @file llpanelnearbymedia.h
 * @brief Management interface for muting and controlling nearby media
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#ifndef LL_LLPANELNEARBYMEDIA_H
#define LL_LLPANELNEARBYMEDIA_H

#include "llpanel.h"

class LLPanelNearbyMedia;
class LLButton;
class LLScrollListCtrl;
class LLSliderCtrl;
class LLCheckBoxCtrl;
class LLTextBox;
class LLComboBox;
class LLViewerMediaImpl;

class LLPanelNearByMedia : public LLPanel
{
public:
	
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();
	/*virtual*/ void onMouseEnter(S32 x, S32 y, MASK mask);
	/*virtual*/ void onMouseLeave(S32 x, S32 y, MASK mask);
	/*virtual*/ void handleVisibilityChange ( BOOL new_visibility );
	/*virtual*/ void onTopLost ();

	// this is part of the nearby media *dialog* so we can track whether
	// the user *implicitly* wants audio on or off via their *explicit*
	// interaction with our buttons.
	bool getParcelAudioAutoStart();

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
	void addMediaItem(const LLUUID &id);
	void updateMediaItem(LLScrollListItem* item, LLViewerMediaImpl* impl);
	void removeMediaItem(const LLUUID &id);
	
	// Refresh the list in the UI
	void refreshList();
	
	void refreshParcelMediaUI();
	void refreshParcelAudioUI();

	// UI Callbacks 
	void onClickEnableAll();
	void onClickDisableAll();
	void onClickEnableParcelMedia();
	void onClickDisableParcelMedia();
	void onClickMuteParcelMedia();
	void onParcelMediaVolumeSlider();
	void onClickParcelMediaPlay();
	void onClickParcelMediaStop();
	void onClickParcelMediaPause();
	void onClickParcelAudioPlay();
	void onClickParcelAudioStop();
	void onClickParcelAudioPause();
	void onCheckAutoPlay();
	
	void onCheckItem(LLUICtrl* ctrl, const LLUUID &row_id);
	
	static void onZoomMedia(void* user_data);
	
private:
	bool setDisabled(const LLUUID &id, bool disabled);
	
	static void getNameAndUrlHelper(LLViewerMediaImpl* impl, std::string& name, std::string & url, const std::string &defaultName);

	static std::string getParcelAudioURL();
	
	void updateColumns();
	
	void enableAllHelper(bool val);

	bool shouldShow(LLViewerMediaImpl* impl);
	
	LLScrollListCtrl*	mMediaList;
	LLUICtrl*			mEnableAllCtrl;
	LLUICtrl*			mDisableAllCtrl;
	LLSliderCtrl*		mParcelMediaVolumeSlider;
	LLButton*			mParcelMediaMuteCtrl;
	LLUICtrl*			mEnableParcelMediaCtrl;
	LLUICtrl*			mDisableParcelMediaCtrl;
	LLTextBox*			mParcelMediaText;
	LLTextBox*			mItemCountText;
	LLUICtrl*			mParcelMediaCtrl;
	LLUICtrl*			mParcelMediaPlayCtrl;
	LLUICtrl*			mParcelMediaPauseCtrl;
	LLUICtrl*			mParcelAudioCtrl;
	LLUICtrl*			mParcelAudioPlayCtrl;
	LLUICtrl*			mParcelAudioPauseCtrl;
	LLComboBox*			mShowCtrl;
	
	bool				mAllMediaDisabled;
	bool				mDebugInfoVisible;
	bool				mParcelAudioAutoStart;
	std::string			mEmptyNameString;
	std::string			mDefaultParcelMediaName;
	std::string			mDefaultParcelMediaToolTip;

	LLFrameTimer		mHoverTimer;
};


#endif // LL_LLPANELNEARBYMEDIA_H
