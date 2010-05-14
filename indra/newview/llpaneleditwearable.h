/** 
 * @file llfloatercustomize.h
 * @brief The customize avatar floater, triggered by "Appearance..."
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009-2009, Linden Research, Inc.
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

#ifndef LL_LLPANELEDITWEARABLE_H
#define LL_LLPANELEDITWEARABLE_H

#include "llpanel.h"
#include "llscrollingpanellist.h"
#include "llmodaldialog.h"
#include "llwearabletype.h"

class LLWearable;
class LLTextEditor;
class LLTextBox;
class LLViewerInventoryItem;
class LLViewerVisualParam;
class LLVisualParamHint;
class LLViewerJointMesh;
class LLAccordionCtrlTab;

class LLPanelEditWearable : public LLPanel
{
public:
	LLPanelEditWearable( );
	virtual ~LLPanelEditWearable();

	/*virtual*/ BOOL 		postBuild();
	/*virtual*/ BOOL		isDirty() const;	// LLUICtrl
	/*virtual*/ void		draw();	

	LLWearable* 		getWearable() { return mWearablePtr; }
	void				setWearable(LLWearable *wearable);

	void				saveChanges();
	void				revertChanges();

	static void			onRevertButtonClicked(void* userdata);

private:
	typedef std::map<F32, LLViewerVisualParam*> value_map_t;

	void				showWearable(LLWearable* wearable, BOOL show);
	void				initializePanel();
	void				updateScrollingPanelUI();
	LLPanel*			getPanel(LLWearableType::EType type);
	void				getSortedParams(value_map_t &sorted_params, const std::string &edit_group);
	void				buildParamList(LLScrollingPanelList *panel_list, value_map_t &sorted_params, LLAccordionCtrlTab *tab);
	// update bottom bar buttons ("Save", "Revert", etc)
	void				updateVerbs();

	void				onColorSwatchCommit(const LLUICtrl*);
	void				onTexturePickerCommit(const LLUICtrl*);
	void				updatePanelPickerControls(LLWearableType::EType type);
	void				toggleTypeSpecificControls(LLWearableType::EType type);
	void				updateTypeSpecificControls(LLWearableType::EType type);

	// the pointer to the wearable we're editing. NULL means we're not editing a wearable.
	LLWearable *mWearablePtr;
	LLViewerInventoryItem* mWearableItem;

	// these are constant no matter what wearable we're editing
	LLButton *mBtnRevert;
	LLButton *mBtnBack;

	LLTextBox *mPanelTitle;
	LLTextBox *mDescTitle;
	LLTextBox *mTxtAvatarHeight;


	// This text editor reference will change each time we edit a new wearable - 
	// it will be grabbed from the currently visible panel
	LLTextEditor *mTextEditor;

	// The following panels will be shown/hidden based on what wearable we're editing
	// body parts
	LLPanel *mPanelShape;
	LLPanel *mPanelSkin;
	LLPanel *mPanelEyes;
	LLPanel *mPanelHair;

	//clothes
	LLPanel *mPanelShirt;
	LLPanel *mPanelPants;
	LLPanel *mPanelShoes;
	LLPanel *mPanelSocks;
	LLPanel *mPanelJacket;
	LLPanel *mPanelGloves;
	LLPanel *mPanelUndershirt;
	LLPanel *mPanelUnderpants;
	LLPanel *mPanelSkirt;
	LLPanel *mPanelAlpha;
	LLPanel *mPanelTattoo;
};

#endif
