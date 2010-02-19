/** 
 * @file lllocationinputctrl.h
 * @brief Combobox-like location input control
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_LLLOCATIONINPUTCTRL_H
#define LL_LLLOCATIONINPUTCTRL_H

#include "llcombobox.h"
#include "lliconctrl.h"		// Params
#include "lltextbox.h"		// Params

class LLLandmark;

// internals
class LLAddLandmarkObserver;
class LLRemoveLandmarkObserver;
class LLParcelChangeObserver;
class LLMenuGL;
class LLTeleportHistoryItem;

/**
 * Location input control.
 * 
 * @see LLNavigationBar
 */
class LLLocationInputCtrl
:	public LLComboBox
{
	LOG_CLASS(LLLocationInputCtrl);
	friend class LLAddLandmarkObserver;
	friend class LLRemoveLandmarkObserver;
	friend class LLParcelChangeObserver;

public:
	struct Params 
	:	public LLInitParam::Block<Params, LLComboBox::Params>
	{
		Optional<LLUIImage*>				icon_maturity_general,
											icon_maturity_adult,
											add_landmark_image_enabled,
											add_landmark_image_disabled,
											add_landmark_image_hover,
											add_landmark_image_selected;
		Optional<S32>						icon_hpad,
											add_landmark_hpad;
		Optional<LLButton::Params>			add_landmark_button,
											for_sale_button,
											info_button;
		Optional<LLIconCtrl::Params>		maturity_icon,
											voice_icon,
											fly_icon,
											push_icon,
											build_icon,
											scripts_icon,
											damage_icon;
		Optional<LLTextBox::Params>			damage_text;
		Params();
	};

	// LLView interface
	/*virtual*/ void		setEnabled(BOOL enabled);
	/*virtual*/ BOOL		handleToolTip(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL		handleKeyHere(KEY key, MASK mask);
	/*virtual*/ void		onFocusReceived();
	/*virtual*/ void		onFocusLost();
	/*virtual*/ void		draw();
	/*virtual*/ void		reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	//========================================================================

	// LLUICtrl interface
	/*virtual*/ void		setFocus(BOOL b);
	//========================================================================

	// LLComboBox interface
	void					hideList();
	void					onTextEntry(LLLineEditor* line_editor);
	//========================================================================

	LLLineEditor*			getTextEntry() const { return mTextEntry; }
	void					handleLoginComplete();

private:

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

	friend class LLUICtrlFactory;
	LLLocationInputCtrl(const Params&);
	virtual ~LLLocationInputCtrl();

	void					focusTextEntry();
	/**
	 * Changes the "Add landmark" button image
	 * depending on whether current parcel has been landmarked.
	 */
	void					enableAddLandmarkButton(bool val);
	void					refresh();
	void					refreshLocation();
	void					refreshParcelIcons();
	// Refresh the value in the health percentage text field
	void					refreshHealth();
	void					positionMaturityIcon();
	
	void					rebuildLocationHistory(std::string filter = "");
	bool 					findTeleportItemsByTitle(const LLTeleportHistoryItem& item, const std::string& filter);
	void					setText(const LLStringExplicit& text);
	void					updateAddLandmarkButton();
	void 					updateAddLandmarkTooltip();
	void 					updateContextMenu();
	void					updateWidgetlayout();
	void					changeLocationPresentation();

	void					onInfoButtonClicked();
	void					onLocationHistoryLoaded();
	void					onLocationPrearrange(const LLSD& data);
	void 					onTextEditorRightClicked(S32 x, S32 y, MASK mask);
	void					onLandmarkLoaded(LLLandmark* lm);
	void					onForSaleButtonClicked();
	void					onAddLandmarkButtonClicked();
	void					onAgentParcelChange();
	// callbacks
	bool					onLocationContextMenuItemEnabled(const LLSD& userdata);
	void 					onLocationContextMenuItemClicked(const LLSD& userdata);
	void					onParcelIconClick(EParcelIcon icon);

	LLMenuGL*				mLocationContextMenu;
	LLButton*				mAddLandmarkBtn;
	LLButton*				mForSaleBtn;
	LLButton*				mInfoBtn;
	S32						mIconHPad;			// pad between all icons
	S32						mAddLandmarkHPad;	// pad to left of landmark star

	LLIconCtrl*	mMaturityIcon;
	LLIconCtrl*	mParcelIcon[ICON_COUNT];
	LLTextBox* mDamageText;

	LLAddLandmarkObserver*		mAddLandmarkObserver;
	LLRemoveLandmarkObserver*	mRemoveLandmarkObserver;
	LLParcelChangeObserver*		mParcelChangeObserver;

	boost::signals2::connection	mCoordinatesControlConnection;
	boost::signals2::connection	mParcelPropertiesControlConnection;
	boost::signals2::connection	mParcelMgrConnection;
	boost::signals2::connection	mLocationHistoryConnection;
	LLUIImage* mLandmarkImageOn;
	LLUIImage* mLandmarkImageOff;
	LLUIImage* mIconMaturityGeneral;
	LLUIImage* mIconMaturityAdult;

	std::string mAddLandmarkTooltip;
	std::string mEditLandmarkTooltip;
	// this field holds a human-readable form of the location string, it is needed to be able to compare copy-pated value and real location
	std::string mHumanReadableLocation;
	bool isHumanReadableLocationVisible;
};

#endif
