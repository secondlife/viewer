/** 
 * @file llfloatertools.h
 * @brief The edit tools, including move, position, land, etc.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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

	void setStatusText(const LLString& text);
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
};

extern LLFloaterTools *gFloaterTools;

#endif
