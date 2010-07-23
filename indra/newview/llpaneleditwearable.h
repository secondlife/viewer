/** 
 * @file llpaneleditwearable.h
 * @brief A LLPanel dedicated to the editing of wearables.
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
#include "llvoavatardefines.h"
#include "llwearabletype.h"

class LLAccordionCtrl;
class LLCheckBoxCtrl;
class LLWearable;
class LLTextBox;
class LLViewerInventoryItem;
class LLViewerVisualParam;
class LLVisualParamHint;
class LLViewerJointMesh;
class LLAccordionCtrlTab;
class LLJoint;
class LLLineEditor;

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

	void				saveChanges(bool force_save_as = false);
	void				revertChanges();

	void				showDefaultSubpart();
	void				onTabExpandedCollapsed(const LLSD& param, U8 index);

	void 				updateScrollingPanelList();

	static void			onRevertButtonClicked(void* userdata);
	void				onCommitSexChange();
	void				onSaveAsButtonClicked();
	void				saveAsCallback(const LLSD& notification, const LLSD& response);


private:
	typedef std::map<F32, LLViewerVisualParam*> value_map_t;

	void				showWearable(LLWearable* wearable, BOOL show);
	void				updateScrollingPanelUI();
	LLPanel*			getPanel(LLWearableType::EType type);
	void				getSortedParams(value_map_t &sorted_params, const std::string &edit_group);
	void				buildParamList(LLScrollingPanelList *panel_list, value_map_t &sorted_params, LLAccordionCtrlTab *tab, LLJoint* jointp);
	// update bottom bar buttons ("Save", "Revert", etc)
	void				updateVerbs();

	void				onColorSwatchCommit(const LLUICtrl*);
	void				onTexturePickerCommit(const LLUICtrl*);
	void				updatePanelPickerControls(LLWearableType::EType type);
	void				toggleTypeSpecificControls(LLWearableType::EType type);
	void				updateTypeSpecificControls(LLWearableType::EType type);

	// changes camera angle to default for selected subpart
	void				changeCamera(U8 subpart);

	//alpha mask checkboxes
	void configureAlphaCheckbox(LLVOAvatarDefines::ETextureIndex te, const std::string& name);
	void onInvisibilityCommit(LLCheckBoxCtrl* checkbox_ctrl, LLVOAvatarDefines::ETextureIndex te);
	void updateAlphaCheckboxes();
	void initPreviousAlphaTextures();
	void initPreviousAlphaTextureEntry(LLVOAvatarDefines::ETextureIndex te);

	// callback for HeightUnits parameter.
	bool changeHeightUnits(const LLSD& new_value);

	// updates current metric and replacemet metric label text
	void updateMetricLayout(BOOL new_value);

	// updates avatar height label
	void updateAvatarHeightLabel();

	void onWearablePanelVisibilityChange(const LLSD &in_visible_chain, LLAccordionCtrl* accordion_ctrl);

	void setWearablePanelVisibilityChangeCallback(LLPanel* bodypart_panel);

	// the pointer to the wearable we're editing. NULL means we're not editing a wearable.
	LLWearable *mWearablePtr;
	LLViewerInventoryItem* mWearableItem;

	// these are constant no matter what wearable we're editing
	LLButton *mBtnRevert;
	LLButton *mBtnBack;
	std::string mBackBtnLabel;

	LLTextBox *mPanelTitle;
	LLTextBox *mDescTitle;
	LLTextBox *mTxtAvatarHeight;


	// localized and parametrized strings that used to build avatar_height_label
	std::string mMeters;
	std::string mFeet;
	std::string mHeigth;
	LLUIString  mHeigthValue;
	LLUIString  mReplacementMetricUrl;

	// color for mHeigth string
	LLUIColor mAvatarHeigthLabelColor;
	// color for mHeigthValue string
	LLUIColor mAvatarHeigthValueLabelColor;

	// This text editor reference will change each time we edit a new wearable - 
	// it will be grabbed from the currently visible panel
	LLLineEditor *mNameEditor;

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

	typedef std::map<std::string, LLVOAvatarDefines::ETextureIndex> string_texture_index_map_t;
	string_texture_index_map_t mAlphaCheckbox2Index;

	typedef std::map<LLVOAvatarDefines::ETextureIndex, LLUUID> s32_uuid_map_t;
	s32_uuid_map_t mPreviousAlphaTexture;
};

#endif
