/** 
 * @file llfloatertools.h
 * @brief The edit tools, including move, position, land, etc.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLFLOATERTOOLS_H
#define LL_LLFLOATERTOOLS_H

#include "llfloater.h"
#include "llcoord.h"
#include "llparcelselection.h"

class LLButton;
class LLComboBox;
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

	LLCheckBoxCtrl*	mCheckSnapToGrid;
	LLButton*		mBtnGridOptions;
	LLTextBox*		mTextGridMode;
	LLComboBox*		mComboGridMode;
	LLCheckBoxCtrl*	mCheckStretchUniform;
	LLCheckBoxCtrl*	mCheckStretchTexture;

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
