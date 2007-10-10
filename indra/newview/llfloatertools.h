/** 
 * @file llfloatertools.h
 * @brief The edit tools, including move, position, land, etc.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

class LLButton;
class LLTextBox;
class LLTool;
class LLCheckBoxCtrl;
class LLTabContainer;
class LLPanelPermissions;
class LLPanelObject;
class LLPanelVolume;
class LLPanelContents;
class LLPanelFace;
class LLPanelLandInfo;
class LLComboBox;
class LLParcelSelection;
class LLObjectSelection;

typedef LLHandle<LLParcelSelection> LLParcelSelectionHandle;
typedef LLHandle<LLObjectSelection> LLObjectSelectionHandle;

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
	static	void*	createPanelContentsInventory(void*	vdata);
	static	void*	createPanelLandInfo(void*	vdata);

	LLFloaterTools();
	virtual ~LLFloaterTools();

	virtual void onOpen();
	virtual void onClose(bool app_quitting);
	virtual BOOL canClose();

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

	/*virtual*/  void draw();

	void dirty();
	void showMore(BOOL show_more);
	void showPanel(EInfoPanel panel);

	void setStatusText(const std::string& text);
	virtual void onFocusReceived();
	static void setEditTool(void* data);
	void saveLastTool();
private:
	static void setObjectType( void* data );
	
	void refresh();

	static void onClickGridOptions(void* data);

public:

	LLButton		*mBtnFocus;
	LLButton		*mBtnMove;
	LLButton		*mBtnEdit;
	LLButton		*mBtnCreate;
	LLButton		*mBtnLand;

	LLTextBox		*mTextStatus;

	// Focus buttons
	LLCheckBoxCtrl	*mRadioOrbit;
	LLCheckBoxCtrl	*mRadioZoom;
	LLCheckBoxCtrl	*mRadioPan;

	// Move buttons
	LLCheckBoxCtrl	*mRadioMove;
	LLCheckBoxCtrl	*mRadioLift;
	LLCheckBoxCtrl	*mRadioSpin;

	// Edit buttons
	LLCheckBoxCtrl	*mRadioPosition;
	LLCheckBoxCtrl	*mRadioRotate;
	LLCheckBoxCtrl	*mRadioStretch;
	LLCheckBoxCtrl	*mRadioSelectFace;

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
//	LLCheckBoxCtrl	*mRadioEditLand;
	LLCheckBoxCtrl	*mRadioSelectLand;

	LLCheckBoxCtrl	*mRadioDozerFlatten;
	LLCheckBoxCtrl	*mRadioDozerRaise;
	LLCheckBoxCtrl	*mRadioDozerLower;
	LLCheckBoxCtrl	*mRadioDozerSmooth;
	LLCheckBoxCtrl	*mRadioDozerNoise;
	LLCheckBoxCtrl	*mRadioDozerRevert;

	LLComboBox		*mComboDozerSize;
	LLButton		*mBtnApplyToSelection;
	LLCheckBoxCtrl	*mCheckShowOwners;

	std::vector<LLButton*>	mButtons;//[ 15 ];

	LLTabContainerCommon	*mTab;
	LLPanelPermissions		*mPanelPermissions;
	LLPanelObject			*mPanelObject;
	LLPanelVolume			*mPanelVolume;
	LLPanelContents			*mPanelContents;
	LLPanelFace				*mPanelFace;
	LLPanelLandInfo			*mPanelLandInfo;

	LLTabContainer*			mTabLand;

	LLParcelSelectionHandle	mParcelSelection;
	LLObjectSelectionHandle	mObjectSelection;

private:
	BOOL					mDirty;
	S32						mSmallHeight;
	S32						mLargeHeight;

	std::map<std::string, std::string> mStatusText;
};

extern LLFloaterTools *gFloaterTools;

#endif
