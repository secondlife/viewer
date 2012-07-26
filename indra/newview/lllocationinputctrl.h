/** 
 * @file lllocationinputctrl.h
 * @brief Combobox-like location input control
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

#ifndef LL_LLLOCATIONINPUTCTRL_H
#define LL_LLLOCATIONINPUTCTRL_H

#include "llcombobox.h"
#include "lliconctrl.h"		// Params
#include "lltextbox.h"		// Params
#include "lllocationhistory.h"
#include "llpathfindingnavmesh.h"

class LLLandmark;

// internals
class LLAddLandmarkObserver;
class LLRemoveLandmarkObserver;
class LLParcelChangeObserver;
class LLMenuGL;
class LLTeleportHistoryItem;
class LLPathfindingNavMeshStatus;

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
											icon_maturity_moderate,
											add_landmark_image_enabled,
											add_landmark_image_disabled,
											add_landmark_image_hover,
											add_landmark_image_selected;
		Optional<std::string>				maturity_help_topic;
		Optional<S32>						icon_hpad,
											add_landmark_hpad;
		Optional<LLButton::Params>			maturity_button,
											add_landmark_button,
											for_sale_button,
											info_button;
		Optional<LLIconCtrl::Params>		voice_icon,
											fly_icon,
											push_icon,
											build_icon,
											scripts_icon,
											damage_icon,
											see_avatars_icon,
											pathfinding_dirty_icon,
											pathfinding_disabled_icon;
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
		FLY_ICON,			      // 1
		PUSH_ICON,			      // 2
		BUILD_ICON,			      // 3
		SCRIPTS_ICON,		      // 4
		DAMAGE_ICON,		      // 5
		SEE_AVATARS_ICON,         // 6
		PATHFINDING_DIRTY_ICON,   // 7
		PATHFINDING_DISABLED_ICON,// 8
		ICON_COUNT			      // 9 total
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
	void					refreshMaturityButton();
	void					positionMaturityButton();
	
	void					rebuildLocationHistory(const std::string& filter = LLStringUtil::null);
	bool 					findTeleportItemsByTitle(const LLTeleportHistoryItem& item, const std::string& filter);
	void					setText(const LLStringExplicit& text);
	void					updateAddLandmarkButton();
	void 					updateAddLandmarkTooltip();
	void 					updateContextMenu();
	void					updateWidgetlayout();
	void					changeLocationPresentation();

	void					onInfoButtonClicked();
	void					onLocationHistoryChanged(LLLocationHistory::EChangeType event);
	void					onLocationPrearrange(const LLSD& data);
	void 					onTextEditorRightClicked(S32 x, S32 y, MASK mask);
	void					onLandmarkLoaded(LLLandmark* lm);
	void					onForSaleButtonClicked();
	void					onAddLandmarkButtonClicked();
	void					onAgentParcelChange();
	void					onMaturityButtonClicked();
	void                    onRegionBoundaryCrossed();
	void                    onNavMeshStatusChange(const LLPathfindingNavMeshStatus &pNavMeshStatus);
	// callbacks
	bool					onLocationContextMenuItemEnabled(const LLSD& userdata);
	void 					onLocationContextMenuItemClicked(const LLSD& userdata);
	void					onParcelIconClick(EParcelIcon icon);

	void                    createNavMeshStatusListenerForCurrentRegion();

	LLMenuGL*				mLocationContextMenu;
	LLButton*				mAddLandmarkBtn;
	LLButton*				mForSaleBtn;
	LLButton*				mInfoBtn;
	S32						mIconHPad;			// pad between all icons
	S32						mAddLandmarkHPad;	// pad to left of landmark star

	LLButton*	mMaturityButton;
	LLIconCtrl*	mParcelIcon[ICON_COUNT];
	LLTextBox* mDamageText;

	LLAddLandmarkObserver*		mAddLandmarkObserver;
	LLRemoveLandmarkObserver*	mRemoveLandmarkObserver;
	LLParcelChangeObserver*		mParcelChangeObserver;

	boost::signals2::connection	mCoordinatesControlConnection;
	boost::signals2::connection	mParcelPropertiesControlConnection;
	boost::signals2::connection	mParcelMgrConnection;
	boost::signals2::connection	mLocationHistoryConnection;
	boost::signals2::connection	mRegionCrossingSlot;
	LLPathfindingNavMesh::navmesh_slot_t mNavMeshSlot;
	bool mIsNavMeshDirty;
	LLUIImage* mLandmarkImageOn;
	LLUIImage* mLandmarkImageOff;
	LLPointer<LLUIImage> mIconMaturityGeneral;
	LLPointer<LLUIImage> mIconMaturityAdult;
	LLPointer<LLUIImage> mIconMaturityModerate;
	LLPointer<LLUIImage> mIconPathfindingDynamic;

	std::string mAddLandmarkTooltip;
	std::string mEditLandmarkTooltip;
	// this field holds a human-readable form of the location string, it is needed to be able to compare copy-pated value and real location
	std::string mHumanReadableLocation;
	bool isHumanReadableLocationVisible;
	std::string mMaturityHelpTopic;
};

#endif
