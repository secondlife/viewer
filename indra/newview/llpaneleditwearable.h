/** 
 * @file llpaneleditwearable.h
 * @brief A LLPanel dedicated to the editing of wearables.
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

#ifndef LL_LLPANELEDITWEARABLE_H
#define LL_LLPANELEDITWEARABLE_H

#include "llpanel.h"
#include "llscrollingpanellist.h"
#include "llmodaldialog.h"
#include "llavatarappearancedefines.h"
#include "llwearabletype.h"

class LLAccordionCtrl;
class LLCheckBoxCtrl;
class LLViewerWearable;
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

	// changes camera angle to default for selected subpart
	void				changeCamera(U8 subpart);

	LLViewerWearable*	getWearable() { return mWearablePtr; }
	void				setWearable(LLViewerWearable *wearable, BOOL disable_camera_switch = FALSE);

	void				saveChanges(bool force_save_as = false);
	void				revertChanges();

	void				showDefaultSubpart();
	void				onTabExpandedCollapsed(const LLSD& param, U8 index);

	void 				updateScrollingPanelList();

	static void			onRevertButtonClicked(void* userdata);
	static void			onBackButtonClicked(void* userdata); 
	void				onCommitSexChange();
	void				onSaveAsButtonClicked();
	void				saveAsCallback(const LLSD& notification, const LLSD& response);

	virtual void		setVisible(BOOL visible);


private:
	typedef std::map<F32, LLViewerVisualParam*> value_map_t;

	void				showWearable(LLViewerWearable* wearable, BOOL show, BOOL disable_camera_switch = FALSE);
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

	//alpha mask checkboxes
	void configureAlphaCheckbox(LLAvatarAppearanceDefines::ETextureIndex te, const std::string& name);
	void onInvisibilityCommit(LLCheckBoxCtrl* checkbox_ctrl, LLAvatarAppearanceDefines::ETextureIndex te);
	void updateAlphaCheckboxes();
	void initPreviousAlphaTextures();
	void initPreviousAlphaTextureEntry(LLAvatarAppearanceDefines::ETextureIndex te);

	// callback for HeightUnits parameter.
	bool changeHeightUnits(const LLSD& new_value);

	// updates current metric and replacemet metric label text
	void updateMetricLayout(BOOL new_value);

	// updates avatar height label
	void updateAvatarHeightLabel();

	void onWearablePanelVisibilityChange(const LLSD &in_visible_chain, LLAccordionCtrl* accordion_ctrl);

	void setWearablePanelVisibilityChangeCallback(LLPanel* bodypart_panel);

	// the pointer to the wearable we're editing. NULL means we're not editing a wearable.
	LLViewerWearable *mWearablePtr;
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
	LLPanel *mPanelPhysics;

	typedef std::map<std::string, LLAvatarAppearanceDefines::ETextureIndex> string_texture_index_map_t;
	string_texture_index_map_t mAlphaCheckbox2Index;

	typedef std::map<LLAvatarAppearanceDefines::ETextureIndex, LLUUID> s32_uuid_map_t;
	s32_uuid_map_t mPreviousAlphaTexture;
};

#endif
