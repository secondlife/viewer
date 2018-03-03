/**
 * @file llpanelmediasettingsgeneral.h
 * @brief LLPanelMediaSettingsGeneral class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLPANELMEDIAMEDIASETTINGSGENERAL_H
#define LL_LLPANELMEDIAMEDIASETTINGSGENERAL_H

#include "llpanel.h"

class LLButton;
class LLCheckBoxCtrl;
class LLLineEditor;
class LLSpinCtrl;
class LLTextureCtrl;
class LLMediaCtrl;
class LLTextBox;
class LLFloaterMediaSettings;

class LLPanelMediaSettingsGeneral : public LLPanel
{
public:
	LLPanelMediaSettingsGeneral();
	~LLPanelMediaSettingsGeneral();
	
	// XXX TODO: put these into a common parent class?
	// Hook that the floater calls before applying changes from the panel
	void preApply();
	// Function that asks the panel to fill in values associated with the panel
	// 'include_tentative' means fill in tentative values as well, otherwise do not
	void getValues(LLSD &fill_me_in, bool include_tentative = true);
	// Hook that the floater calls after applying changes to the panel
	void postApply();
	
	BOOL postBuild();
	/*virtual*/ void draw();
	/*virtual*/ void onClose(bool app_quitting);

	void setParent( LLFloaterMediaSettings* parent );
	static void initValues( void* userdata, const LLSD& media_settings ,bool editable);
	static void clearValues( void* userdata, bool editable, bool update_preview = true);
	
	// Navigates the current selected face to the Home URL.
	// If 'only_if_current_is_empty' is "true", it only performs
	// the operation if: 1) the current URL is empty, and 2) auto play is true.
	bool navigateHomeSelectedFace(bool only_if_current_is_empty);
	
	void updateMediaPreview();

	const std::string getHomeUrl();
	
protected:
	LLFloaterMediaSettings* mParent;
	bool mMediaEditable;

private:
	void updateCurrentUrl();
	
	static void onBtnResetCurrentUrl(LLUICtrl* ctrl, void *userdata);
	static void onCommitHomeURL(LLUICtrl* ctrl, void *userdata );
	
	static bool isMultiple();

	void checkHomeUrlPassesWhitelist();

	LLCheckBoxCtrl* mAutoLoop;
	LLCheckBoxCtrl* mFirstClick;
	LLCheckBoxCtrl* mAutoZoom;
	LLCheckBoxCtrl* mAutoPlay;
	LLCheckBoxCtrl* mAutoScale;
	LLSpinCtrl* mWidthPixels;
	LLSpinCtrl* mHeightPixels;
	LLLineEditor* mHomeURL;
	LLTextBox* mCurrentURL;
	LLMediaCtrl* mPreviewMedia;
	LLTextBox* mFailWhiteListText;
};

#endif  // LL_LLPANELMEDIAMEDIASETTINGSGENERAL_H
