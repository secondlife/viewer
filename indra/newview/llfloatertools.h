/** 
 * @file llfloatertools.h
 * @brief The edit tools, including move, position, land, etc.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLFLOATERTOOLS_H
#define LL_LLFLOATERTOOLS_H

#include "llfloater.h"
#include "llcoord.h"
#include "llparcelselection.h"

class LLButton;
class LLCheckBoxCtrl;
class LLPanelPermissions;
class LLPanelObject;
class LLPanelVolume;
class LLPanelContents;
class LLPanelFace;
class LLPanelLandInfo;
class LLRadioGroup;
class LLSlider;
class LLTabContainer;
class LLTextBox;
class LLMediaCtrl;
class LLTool;
class LLParcelSelection;
class LLObjectSelection;

typedef LLSafeHandle<LLObjectSelection> LLObjectSelectionHandle;

class LLFloaterTools
: public LLFloater
{
public:
	virtual	BOOL	postBuild();
	static	void*	createPanelPermissions(void*	vdata);
	static	void*	createPanelObject(void*	vdata);
	static	void*	createPanelVolume(void*	vdata);
	static	void*	createPanelFace(void*	vdata);
	static	void*	createPanelContents(void*	vdata);
	static	void*	createPanelLandInfo(void*	vdata);

	LLFloaterTools(const LLSD& key);
	virtual ~LLFloaterTools();

	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ BOOL canClose();
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ void draw();
	/*virtual*/ void onFocusReceived();

	// call this once per frame to handle visibility, rect location,
	// button highlights, etc.
	void updatePopup(LLCoordGL center, MASK mask);

	// When the floater is going away, reset any options that need to be 
	// cleared.
	void resetToolState();

	enum EInfoPanel
	{
		PANEL_GENERAL=0,
		PANEL_OBJECT,
		PANEL_FEATURES,
		PANEL_FACE,
		PANEL_CONTENTS,
		PANEL_COUNT
	};

	void dirty();
	void showPanel(EInfoPanel panel);

	void setStatusText(const std::string& text);
	static void setEditTool(void* data);
	void setTool(const LLSD& user_data);
	void saveLastTool();
	void onClickBtnDeleteMedia();
	void onClickBtnAddMedia();
	void onClickBtnEditMedia();
	void clearMediaSettings();
	void updateMediaTitle();
	void navigateToTitleMedia( const std::string url );
	bool selectedMediaEditable();

private:
	void refresh();
	void refreshMedia();
	void getMediaState();
	void updateMediaSettings();
	static bool deleteMediaConfirm(const LLSD& notification, const LLSD& response);
	static bool multipleFacesSelectedConfirm(const LLSD& notification, const LLSD& response);
	static void setObjectType( LLPCode pcode );
	void onClickGridOptions();
	S32 calcRenderCost();

public:
	LLButton		*mBtnFocus;
	LLButton		*mBtnMove;
	LLButton		*mBtnEdit;
	LLButton		*mBtnCreate;
	LLButton		*mBtnLand;

	LLTextBox		*mTextStatus;

	// Focus buttons
	LLRadioGroup*	mRadioGroupFocus;

	// Move buttons
	LLRadioGroup*	mRadioGroupMove;

	// Edit buttons
	LLRadioGroup*	mRadioGroupEdit;

	LLCheckBoxCtrl	*mCheckSelectIndividual;
	LLButton*		mBtnLink;
	LLButton*		mBtnUnlink;

	LLCheckBoxCtrl*	mCheckSnapToGrid;
	LLButton*		mBtnGridOptions;
	LLCheckBoxCtrl*	mCheckStretchUniform;
	LLCheckBoxCtrl*	mCheckStretchTexture;

	// !HACK! Replacement of mCheckStretchUniform label because LLCheckBoxCtrl
	//  doesn't support word_wrap of its label. Need to fix truncation bug EXT-6658
	LLTextBox*		mCheckStretchUniformLabel;

	LLButton	*mBtnRotateLeft;
	LLButton	*mBtnRotateReset;
	LLButton	*mBtnRotateRight;

	LLButton	*mBtnDelete;
	LLButton	*mBtnDuplicate;
	LLButton	*mBtnDuplicateInPlace;

	// Create buttons
	LLCheckBoxCtrl	*mCheckSticky;
	LLCheckBoxCtrl	*mCheckCopySelection;
	LLCheckBoxCtrl	*mCheckCopyCenters;
	LLCheckBoxCtrl	*mCheckCopyRotates;

	// Land buttons
	LLRadioGroup*	mRadioGroupLand;
	LLSlider		*mSliderDozerSize;
	LLSlider		*mSliderDozerForce;

	LLButton		*mBtnApplyToSelection;

	std::vector<LLButton*>	mButtons;//[ 15 ];

	LLTabContainer	*mTab;
	LLPanelPermissions		*mPanelPermissions;
	LLPanelObject			*mPanelObject;
	LLPanelVolume			*mPanelVolume;
	LLPanelContents			*mPanelContents;
	LLPanelFace				*mPanelFace;
	LLPanelLandInfo			*mPanelLandInfo;

	LLViewBorder*			mCostTextBorder;

	LLTabContainer*			mTabLand;

	LLParcelSelectionHandle	mParcelSelection;
	LLObjectSelectionHandle	mObjectSelection;

	LLMediaCtrl				*mTitleMedia;
	bool					mNeedMediaTitle;

private:
	BOOL					mDirty;

	std::map<std::string, std::string> mStatusText;

protected:
	LLSD				mMediaSettings;

public:
	static bool		sShowObjectCost;
	
};

extern LLFloaterTools *gFloaterTools;

#endif
