/**
 * @file llpaneltopinfobar.h
 * @brief Coordinates and Parcel Settings information panel definition
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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

#ifndef LLPANELTOPINFOBAR_H_
#define LLPANELTOPINFOBAR_H_

#include "llpanel.h"

class LLButton;
class LLTextBox;
class LLIconCtrl;
class LLParcelChangeObserver;

class LLPanelTopInfoBar : public LLPanel, public LLSingleton<LLPanelTopInfoBar>
{
	LOG_CLASS(LLPanelTopInfoBar);

public:
	LLPanelTopInfoBar();
	~LLPanelTopInfoBar();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();

	/**
	 * Updates location and parcel icons on login complete
	 */
	void handleLoginComplete();

private:
	class LLParcelChangeObserver;

	friend class LLParcelChangeObserver;

	enum EParcelIcon
	{
		VOICE_ICON = 0,
		FLY_ICON,
		PUSH_ICON,
		BUILD_ICON,
		SCRIPTS_ICON,
		DAMAGE_ICON,
		ICON_COUNT
	};

	/**
	 * Initializes parcel icons controls. Called from the constructor.
	 */
	void initParcelIcons();

	BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

	/**
	 * Handles clicks on the parcel icons.
	 */
	void onParcelIconClick(EParcelIcon icon);

	/**
	 * Handles clicks on the info buttons.
	 */
	void onInfoButtonClicked();

	/**
	 * Called when agent changes the parcel.
	 */
	void onAgentParcelChange();

	/**
	 * Called when context menu item is clicked.
	 */
	void onContextMenuItemClicked(const LLSD::String& userdata);

	/**
	 * Called when user checks/unchecks Show Coordinates menu item.
	 */
	void onNavBarShowParcelPropertiesCtrlChanged();

	/**
	 * Shorthand to call updateParcelInfoText() and updateParcelIcons().
	 */
	void update();

	/**
	 * Updates parcel info text (mParcelInfoText).
	 */
	void updateParcelInfoText();

	/**
	 * Updates parcel icons (mParcelIcon[]).
	 */
	void updateParcelIcons();

	/**
	 * Updates health information (mDamageText).
	 */
	void updateHealth();

	/**
	 * Lays out all parcel icons starting from right edge of the mParcelInfoText + 11px
	 * (see screenshots in EXT-5808 for details).
	 */
	void layoutParcelIcons();

	/**
	 * Lays out a widget. Widget's rect mLeft becomes equal to the 'left' argument.
	 */
	S32 layoutWidget(LLUICtrl* ctrl, S32 left);

	/**
	 * Generates location string and returns it in the loc_str parameter.
	 */
	void buildLocationString(std::string& loc_str, bool show_coords);

	/**
	 * Sets new value to the mParcelInfoText and updates the size of the top bar.
	 */
	void setParcelInfoText(const std::string& new_text);

	LLButton* 				mInfoBtn;
	LLTextBox* 				mParcelInfoText;
	LLTextBox* 				mDamageText;
	LLIconCtrl*				mParcelIcon[ICON_COUNT];
	LLParcelChangeObserver*	mParcelChangedObserver;

	boost::signals2::connection	mParcelPropsCtrlConnection;
	boost::signals2::connection	mShowCoordsCtrlConnection;
	boost::signals2::connection	mParcelMgrConnection;
};

#endif /* LLPANELTOPINFOBAR_H_ */
