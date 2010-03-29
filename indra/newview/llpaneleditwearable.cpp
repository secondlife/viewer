/** 
 * @file llpaneleditwearable.cpp
 * @brief UI panel for editing of a particular wearable item.
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

#include "llviewerprecompiledheaders.h"

#include "llpaneleditwearable.h"
#include "llpanel.h"
#include "llwearable.h"
#include "lluictrl.h"
#include "llscrollingpanellist.h"
#include "llvisualparam.h"
#include "lltoolmorph.h"
#include "llviewerjointmesh.h"
#include "lltrans.h"
#include "llbutton.h"
#include "llsliderctrl.h"
#include "llagent.h"
#include "llvoavatarself.h"
#include "lltexteditor.h"
#include "lltextbox.h"
#include "llaccordionctrltab.h"
#include "llagentwearables.h"
#include "llscrollingpanelparam.h"

// register panel with appropriate XML
static LLRegisterPanelClassWrapper<LLPanelEditWearable> t_edit_wearable("panel_edit_wearable");

// subparts of the UI for focus, camera position, etc.
enum ESubpart {
	SUBPART_SHAPE_HEAD = 1, // avoid 0
	SUBPART_SHAPE_EYES,
	SUBPART_SHAPE_EARS,
	SUBPART_SHAPE_NOSE,
	SUBPART_SHAPE_MOUTH,
	SUBPART_SHAPE_CHIN,
	SUBPART_SHAPE_TORSO,
	SUBPART_SHAPE_LEGS,
	SUBPART_SHAPE_WHOLE,
	SUBPART_SHAPE_DETAIL,
	SUBPART_SKIN_COLOR,
	SUBPART_SKIN_FACEDETAIL,
	SUBPART_SKIN_MAKEUP,
	SUBPART_SKIN_BODYDETAIL,
	SUBPART_HAIR_COLOR,
	SUBPART_HAIR_STYLE,
	SUBPART_HAIR_EYEBROWS,
	SUBPART_HAIR_FACIAL,
	SUBPART_EYES,
	SUBPART_SHIRT,
	SUBPART_PANTS,
	SUBPART_SHOES,
	SUBPART_SOCKS,
	SUBPART_JACKET,
	SUBPART_GLOVES,
	SUBPART_UNDERSHIRT,
	SUBPART_UNDERPANTS,
	SUBPART_SKIRT,
	SUBPART_ALPHA,
	SUBPART_TATTOO
 };

typedef std::vector<ESubpart> subpart_vec_t;

// Locally defined classes

class LLEditWearableDictionary : public LLSingleton<LLEditWearableDictionary>
{
	//--------------------------------------------------------------------
	// Constructors and Destructors
	//--------------------------------------------------------------------
public:
	LLEditWearableDictionary();
	virtual ~LLEditWearableDictionary();
	
	//--------------------------------------------------------------------
	// Wearable Types
	//--------------------------------------------------------------------
public:
	struct WearableEntry : public LLDictionaryEntry
	{
		WearableEntry(EWearableType type,
					  const std::string &title,
					  const std::string &desc_title,
					  U8 num_subparts, ... ); // number of subparts followed by a list of ESubparts


		const EWearableType mWearableType;
		const std::string   mTitle;
		const std::string	mDescTitle;
		subpart_vec_t		mSubparts;

	};

	struct Wearables : public LLDictionary<EWearableType, WearableEntry>
	{
		Wearables();
	} mWearables;

	const WearableEntry*	getWearable(EWearableType type) const { return mWearables.lookup(type); }

	//--------------------------------------------------------------------
	// Subparts
	//--------------------------------------------------------------------
public:
	struct SubpartEntry : public LLDictionaryEntry
	{
		SubpartEntry(ESubpart part,
					 const std::string &joint,
					 const std::string &edit_group,
					 const std::string &param_list,
					 const std::string &accordion_tab,
					 const LLVector3d  &target_offset,
					 const LLVector3d  &camera_offset,
					 const ESex 	   &sex);

		ESubpart			mSubpart;
		std::string			mTargetJoint;
		std::string			mEditGroup;
		std::string			mParamList;
		std::string			mAccordionTab;
		LLVector3d			mTargetOffset;
		LLVector3d			mCameraOffset;
		ESex				mSex;
	};

	struct Subparts : public LLDictionary<ESubpart, SubpartEntry>
	{
		Subparts();
	} mSubparts;

	const SubpartEntry*  getSubpart(ESubpart subpart) const { return mSubparts.lookup(subpart); }
};

LLEditWearableDictionary::LLEditWearableDictionary()
{

}

//virtual 
LLEditWearableDictionary::~LLEditWearableDictionary()
{
}

LLEditWearableDictionary::Wearables::Wearables()
{
	addEntry(WT_SHAPE, new WearableEntry(WT_SHAPE,"edit_shape_title","shape_desc_text",9,	SUBPART_SHAPE_HEAD,	SUBPART_SHAPE_EYES,	SUBPART_SHAPE_EARS,	SUBPART_SHAPE_NOSE,	SUBPART_SHAPE_MOUTH, SUBPART_SHAPE_CHIN, SUBPART_SHAPE_TORSO, SUBPART_SHAPE_LEGS, SUBPART_SHAPE_WHOLE));
	addEntry(WT_SKIN, new WearableEntry(WT_SKIN,"edit_skin_title","skin_desc_text",4, SUBPART_SKIN_COLOR, SUBPART_SKIN_FACEDETAIL, SUBPART_SKIN_MAKEUP, SUBPART_SKIN_BODYDETAIL));
	addEntry(WT_HAIR, new WearableEntry(WT_HAIR,"edit_hair_title","hair_desc_text",4, SUBPART_HAIR_COLOR,	SUBPART_HAIR_STYLE,	SUBPART_HAIR_EYEBROWS, SUBPART_HAIR_FACIAL));
	addEntry(WT_EYES, new WearableEntry(WT_EYES,"edit_eyes_title","eyes_desc_text",1, SUBPART_EYES));
	addEntry(WT_SHIRT, new WearableEntry(WT_SHIRT,"edit_shirt_title","shirt_desc_text",1, SUBPART_SHIRT));
	addEntry(WT_PANTS, new WearableEntry(WT_PANTS,"edit_pants_title","pants_desc_text",1, SUBPART_PANTS));
	addEntry(WT_SHOES, new WearableEntry(WT_SHOES,"edit_shoes_title","shoes_desc_text",1, SUBPART_SHOES));
	addEntry(WT_SOCKS, new WearableEntry(WT_SOCKS,"edit_socks_title","socks_desc_text",1, SUBPART_SOCKS));
	addEntry(WT_JACKET, new WearableEntry(WT_JACKET,"edit_jacket_title","jacket_desc_text",1, SUBPART_JACKET));
	addEntry(WT_GLOVES, new WearableEntry(WT_GLOVES,"edit_gloves_title","gloves_desc_text",1, SUBPART_GLOVES));
	addEntry(WT_UNDERSHIRT, new WearableEntry(WT_UNDERSHIRT,"edit_undershirt_title","undershirt_desc_text",1, SUBPART_UNDERSHIRT));
	addEntry(WT_UNDERPANTS, new WearableEntry(WT_UNDERPANTS,"edit_underpants_title","underpants_desc_text",1, SUBPART_UNDERPANTS));
	addEntry(WT_SKIRT, new WearableEntry(WT_SKIRT,"edit_skirt_title","skirt_desc_text",1, SUBPART_SKIRT));
	addEntry(WT_ALPHA, new WearableEntry(WT_ALPHA,"edit_alpha_title","alpha_desc_text",1, SUBPART_ALPHA));
	addEntry(WT_TATTOO, new WearableEntry(WT_TATTOO,"edit_tattoo_title","tattoo_desc_text",1, SUBPART_TATTOO));
}

LLEditWearableDictionary::WearableEntry::WearableEntry(EWearableType type,
					  const std::string &title,
					  const std::string &desc_title,
					  U8 num_subparts, ... ) :
	LLDictionaryEntry(title),
	mWearableType(type),
	mTitle(title),
	mDescTitle(desc_title)
{
	va_list argp;
	va_start(argp, num_subparts);

	for (U8 i = 0; i < num_subparts; ++i)
	{
		ESubpart part = (ESubpart)va_arg(argp,int);
		mSubparts.push_back(part);
	}
}

LLEditWearableDictionary::Subparts::Subparts()
{
	addEntry(SUBPART_SHAPE_WHOLE, new SubpartEntry(SUBPART_SHAPE_WHOLE, "mPelvis", "shape_body","shape_body_param_list", "shape_body_tab", LLVector3d(0.f, 0.f, 0.1f), LLVector3d(-2.5f, 0.5f, 0.8f),SEX_BOTH));
	addEntry(SUBPART_SHAPE_HEAD, new SubpartEntry(SUBPART_SHAPE_HEAD, "mHead", "shape_head", "shape_head_param_list", "shape_head_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
	addEntry(SUBPART_SHAPE_EYES, new SubpartEntry(SUBPART_SHAPE_EYES, "mHead", "shape_eyes", "shape_eyes_param_list", "shape_eyes_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
	addEntry(SUBPART_SHAPE_EARS, new SubpartEntry(SUBPART_SHAPE_EARS, "mHead", "shape_ears", "shape_ears_param_list", "shape_ears_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
	addEntry(SUBPART_SHAPE_NOSE, new SubpartEntry(SUBPART_SHAPE_NOSE, "mHead", "shape_nose", "shape_nose_param_list", "shape_nose_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
	addEntry(SUBPART_SHAPE_MOUTH, new SubpartEntry(SUBPART_SHAPE_MOUTH, "mHead", "shape_mouth", "shape_mouth_param_list", "shape_mouth_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
	addEntry(SUBPART_SHAPE_CHIN, new SubpartEntry(SUBPART_SHAPE_CHIN, "mHead", "shape_chin", "shape_chin_param_list", "shape_chin_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
	addEntry(SUBPART_SHAPE_TORSO, new SubpartEntry(SUBPART_SHAPE_TORSO, "mTorso", "shape_torso", "shape_torso_param_list", "shape_torso_tab", LLVector3d(0.f, 0.f, 0.3f), LLVector3d(-1.f, 0.15f, 0.3f),SEX_BOTH));
	addEntry(SUBPART_SHAPE_LEGS, new SubpartEntry(SUBPART_SHAPE_LEGS, "mPelvis", "shape_legs", "shape_legs_param_list", "shape_legs_tab", LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f),SEX_BOTH));

	addEntry(SUBPART_SKIN_COLOR, new SubpartEntry(SUBPART_SKIN_COLOR, "mHead", "skin_color", "skin_color_param_list", "skin_color_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
	addEntry(SUBPART_SKIN_FACEDETAIL, new SubpartEntry(SUBPART_SKIN_FACEDETAIL, "mHead", "skin_facedetail", "skin_face_param_list", "skin_face_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
	addEntry(SUBPART_SKIN_MAKEUP, new SubpartEntry(SUBPART_SKIN_MAKEUP, "mHead", "skin_makeup", "skin_makeup_param_list", "skin_makeup_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
	addEntry(SUBPART_SKIN_BODYDETAIL, new SubpartEntry(SUBPART_SKIN_BODYDETAIL, "mPelvis", "skin_bodydetail", "skin_body_param_list", "skin_body_tab", LLVector3d(0.f, 0.f, -0.2f), LLVector3d(-2.5f, 0.5f, 0.5f),SEX_BOTH));

	addEntry(SUBPART_HAIR_COLOR, new SubpartEntry(SUBPART_HAIR_COLOR, "mHead", "hair_color", "hair_color_param_list", "hair_color_tab", LLVector3d(0.f, 0.f, 0.10f), LLVector3d(-0.4f, 0.05f, 0.10f),SEX_BOTH));
	addEntry(SUBPART_HAIR_STYLE, new SubpartEntry(SUBPART_HAIR_STYLE, "mHead", "hair_style", "hair_style_param_list", "hair_style_tab", LLVector3d(0.f, 0.f, 0.10f), LLVector3d(-0.4f, 0.05f, 0.10f),SEX_BOTH));
	addEntry(SUBPART_HAIR_EYEBROWS, new SubpartEntry(SUBPART_HAIR_EYEBROWS, "mHead", "hair_eyebrows", "hair_eyebrows_param_list", "hair_eyebrows_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
	addEntry(SUBPART_HAIR_FACIAL, new SubpartEntry(SUBPART_HAIR_FACIAL, "mHead", "hair_facial", "hair_facial_param_list", "hair_facial_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_MALE));

	addEntry(SUBPART_EYES, new SubpartEntry(SUBPART_EYES, "mHead", "eyes", "eyes_main_param_list", "eyes_main_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));

	addEntry(SUBPART_SHIRT, new SubpartEntry(SUBPART_SHIRT, "mTorso", "shirt", "shirt_main_param_list", "shirt_main_tab", LLVector3d(0.f, 0.f, 0.3f), LLVector3d(-1.f, 0.15f, 0.3f),SEX_BOTH));
	addEntry(SUBPART_PANTS, new SubpartEntry(SUBPART_PANTS, "mPelvis", "pants", "pants_main_param_list", "pants_main_tab", LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f),SEX_BOTH));
	addEntry(SUBPART_SHOES, new SubpartEntry(SUBPART_SHOES, "mPelvis", "shoes", "shoes_main_param_list", "shoes_main_tab", LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f),SEX_BOTH));
	addEntry(SUBPART_SOCKS, new SubpartEntry(SUBPART_SOCKS, "mPelvis", "socks", "socks_main_param_list", "socks_main_tab", LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f),SEX_BOTH));
	addEntry(SUBPART_JACKET, new SubpartEntry(SUBPART_JACKET, "mTorso", "jacket", "jacket_main_param_list", "jacket_main_tab", LLVector3d(0.f, 0.f, 0.f), LLVector3d(-2.f, 0.1f, 0.3f),SEX_BOTH));
	addEntry(SUBPART_SKIRT, new SubpartEntry(SUBPART_SKIRT, "mPelvis", "skirt", "skirt_main_param_list", "skirt_main_tab", LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f),SEX_BOTH));
	addEntry(SUBPART_GLOVES, new SubpartEntry(SUBPART_GLOVES, "mTorso", "gloves", "gloves_main_param_list", "gloves_main_tab", LLVector3d(0.f, 0.f, 0.f), LLVector3d(-1.f, 0.15f, 0.f),SEX_BOTH));
	addEntry(SUBPART_UNDERSHIRT, new SubpartEntry(SUBPART_UNDERSHIRT, "mTorso", "undershirt", "undershirt_main_param_list", "undershirt_main_tab", LLVector3d(0.f, 0.f, 0.3f), LLVector3d(-1.f, 0.15f, 0.3f),SEX_BOTH));
	addEntry(SUBPART_UNDERPANTS, new SubpartEntry(SUBPART_UNDERPANTS, "mPelvis", "underpants", "underpants_main_param_list", "underpants_main_tab", LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f),SEX_BOTH));
	addEntry(SUBPART_ALPHA, new SubpartEntry(SUBPART_ALPHA, "mPelvis", "alpha", "alpha_main_param_list", "alpha_main_tab", LLVector3d(0.f, 0.f, 0.1f), LLVector3d(-2.5f, 0.5f, 0.8f),SEX_BOTH));
	addEntry(SUBPART_TATTOO, new SubpartEntry(SUBPART_TATTOO, "mPelvis", "tattoo", "tattoo_main_param_list", "tattoo_main_tab", LLVector3d(0.f, 0.f, 0.1f), LLVector3d(-2.5f, 0.5f, 0.8f),SEX_BOTH));
}

LLEditWearableDictionary::SubpartEntry::SubpartEntry(ESubpart part,
					 const std::string &joint,
					 const std::string &edit_group,
					 const std::string &param_list,
					 const std::string &accordion_tab,
					 const LLVector3d  &target_offset,
					 const LLVector3d  &camera_offset,
					 const ESex 	   &sex) :
	LLDictionaryEntry(edit_group),
	mSubpart(part),
	mTargetJoint(joint),
	mEditGroup(edit_group),
	mParamList(param_list),
	mAccordionTab(accordion_tab),
	mTargetOffset(target_offset),
	mCameraOffset(camera_offset),
	mSex(sex)
{
}


// LLPanelEditWearable

LLPanelEditWearable::LLPanelEditWearable()
	: LLPanel()
{
}

//virtual
LLPanelEditWearable::~LLPanelEditWearable()
{

}

// virtual 
BOOL LLPanelEditWearable::postBuild()
{
	// buttons
	mBtnRevert = getChild<LLButton>("revert_button");
	mBtnRevert->setClickedCallback(boost::bind(&LLPanelEditWearable::onRevertButtonClicked, this));

	mBtnBack = getChild<LLButton>("back_btn");
	// handled at appearance panel level?
	//mBtnBack->setClickedCallback(boost::bind(&LLPanelEditWearable::onBackButtonClicked, this));

	mTextEditor = getChild<LLTextEditor>("description");

	mPanelTitle = getChild<LLTextBox>("edit_wearable_title");
	mDescTitle = getChild<LLTextBox>("description_text");

	// The following panels will be shown/hidden based on what wearable we're editing
	// body parts
	mPanelShape = getChild<LLPanel>("edit_shape_panel");
	mPanelSkin = getChild<LLPanel>("edit_skin_panel");
	mPanelEyes = getChild<LLPanel>("edit_eyes_panel");
	mPanelHair = getChild<LLPanel>("edit_hair_panel");

	//clothes
	mPanelShirt = getChild<LLPanel>("edit_shirt_panel");
	mPanelPants = getChild<LLPanel>("edit_pants_panel");
	mPanelShoes = getChild<LLPanel>("edit_shoes_panel");
	mPanelSocks = getChild<LLPanel>("edit_socks_panel");
	mPanelJacket = getChild<LLPanel>("edit_jacket_panel");
	mPanelGloves = getChild<LLPanel>("edit_gloves_panel");
	mPanelUndershirt = getChild<LLPanel>("edit_undershirt_panel");
	mPanelUnderpants = getChild<LLPanel>("edit_underpants_panel");
	mPanelSkirt = getChild<LLPanel>("edit_skirt_panel");
	mPanelAlpha = getChild<LLPanel>("edit_alpha_panel");
	mPanelTattoo = getChild<LLPanel>("edit_tattoo_panel");

	mWearablePtr = NULL;

	return TRUE;
}

// virtual 
// LLUICtrl
BOOL LLPanelEditWearable::isDirty() const
{
	BOOL isDirty = FALSE;
	if (mWearablePtr)
	{
		if (mWearablePtr->isDirty() ||
			mWearablePtr->getName().compare(mTextEditor->getText()) != 0)
		{
			isDirty = TRUE;
		}
	}
	return isDirty;
}
//virtual
void LLPanelEditWearable::draw()
{
	BOOL is_dirty = isDirty();
	mBtnRevert->setEnabled(is_dirty);

	LLPanel::draw();
}

void LLPanelEditWearable::setWearable(LLWearable *wearable)
{
	showWearable(mWearablePtr, FALSE);
	mWearablePtr = wearable;
	showWearable(mWearablePtr, TRUE);

	initializePanel();
}

//static 
void LLPanelEditWearable::onRevertButtonClicked(void* userdata)
{
	LLPanelEditWearable *panel = (LLPanelEditWearable*) userdata;
	panel->revertChanges();
}


void LLPanelEditWearable::saveChanges()
{
	if (!mWearablePtr || !isDirty())
	{
		// do nothing if no unsaved changes
		return;
	}

	U32 index = gAgentWearables.getWearableIndex(mWearablePtr);
	
	if (mWearablePtr->getName().compare(mTextEditor->getText()) != 0)
	{
		// the name of the wearable has changed, re-save wearable with new name
		gAgentWearables.saveWearableAs(mWearablePtr->getType(), index, mTextEditor->getText(), FALSE);
	}
	else
	{
		gAgentWearables.saveWearable(mWearablePtr->getType(), index);
	}
}

void LLPanelEditWearable::revertChanges()
{
	if (!mWearablePtr || !isDirty())
	{
		// no unsaved changes to revert
		return;
	}

	mWearablePtr->revertValues();
	mTextEditor->setText(mWearablePtr->getName());
}

void LLPanelEditWearable::showWearable(LLWearable* wearable, BOOL show)
{
	if (!wearable)
	{
		return;
	}

	EWearableType type = wearable->getType();
	LLPanel *targetPanel = NULL;
	std::string title;
	std::string description_title;

	const LLEditWearableDictionary::WearableEntry *entry = LLEditWearableDictionary::getInstance()->getWearable(type);
	if (!entry)
	{
		llwarns << "called LLPanelEditWearable::showWearable with an invalid wearable type! (" << type << ")" << llendl;
		return;
	}

	targetPanel = getPanel(type);
	title = getString(entry->mTitle);
	description_title = getString(entry->mDescTitle);

	targetPanel->setVisible(show);
	if (show)
	{
		mPanelTitle->setText(title);
		mDescTitle->setText(description_title);
	}

}

void LLPanelEditWearable::initializePanel()
{
	if (!mWearablePtr)
	{
		// cannot initialize with a null reference.
		return;
	}

	EWearableType type = mWearablePtr->getType();

	// set name
	mTextEditor->setText(mWearablePtr->getName());

	// clear and rebuild visual param list
	const LLEditWearableDictionary::WearableEntry *wearable_entry = LLEditWearableDictionary::getInstance()->getWearable(type);
	if (!wearable_entry)
	{
		llwarns << "could not get wearable dictionary entry for wearable of type: " << type << llendl;
		return;
	}
	U8 num_subparts = wearable_entry->mSubparts.size();

	for (U8 index = 0; index < num_subparts; ++index)
	{
		// dive into data structures to get the panel we need
		ESubpart subpart_e = wearable_entry->mSubparts[index];
		const LLEditWearableDictionary::SubpartEntry *subpart_entry = LLEditWearableDictionary::getInstance()->getSubpart(subpart_e);

		if (!subpart_entry)
		{
			llwarns << "could not get wearable subpart dictionary entry for subpart: " << subpart_e << llendl;
			continue;
		}

		const std::string scrolling_panel = subpart_entry->mParamList;
		const std::string accordion_tab = subpart_entry->mAccordionTab;

		LLScrollingPanelList *panel_list = getChild<LLScrollingPanelList>(scrolling_panel);
		LLAccordionCtrlTab *tab = getChild<LLAccordionCtrlTab>(accordion_tab);

		if (!panel_list)
		{
			llwarns << "could not get scrolling panel list: " << scrolling_panel << llendl;
			continue;
		}

		if (!tab)
		{
			llwarns << "could not get llaccordionctrltab from UI with name: " << accordion_tab << llendl;
			continue;
		}

		// what edit group do we want to extract params for?
		const std::string edit_group = subpart_entry->mEditGroup;

		// storage for ordered list of visual params
		value_map_t sorted_params;
		getSortedParams(sorted_params, edit_group);

		buildParamList(panel_list, sorted_params, tab);

		updateScrollingPanelUI();
	}
	
}

void LLPanelEditWearable::updateScrollingPanelUI()
{
	// do nothing if we don't have a valid wearable we're editing
	if (mWearablePtr == NULL)
	{
		return;
	}

	EWearableType type = mWearablePtr->getType();
	LLPanel *panel = getPanel(type);

	if(panel && (mWearablePtr->getItemID().notNull()))
	{
		const LLEditWearableDictionary::WearableEntry *wearable_entry = LLEditWearableDictionary::getInstance()->getWearable(type);
		U8 num_subparts = wearable_entry->mSubparts.size();

		LLScrollingPanelParam::sUpdateDelayFrames = 0;
		for (U8 index = 0; index < num_subparts; ++index)
		{
			// dive into data structures to get the panel we need
			ESubpart subpart_e = wearable_entry->mSubparts[index];
			const LLEditWearableDictionary::SubpartEntry *subpart_entry = LLEditWearableDictionary::getInstance()->getSubpart(subpart_e);

			const std::string scrolling_panel = subpart_entry->mParamList;

			LLScrollingPanelList *panel_list = getChild<LLScrollingPanelList>(scrolling_panel);
	
			if (!panel_list)
			{
				llwarns << "could not get scrolling panel list: " << scrolling_panel << llendl;
				continue;
			}
			
			panel_list->updatePanels(TRUE);
		}
	}
}

LLPanel* LLPanelEditWearable::getPanel(EWearableType type)
{
	switch (type)
	{
		case WT_SHAPE:
			return mPanelShape;
			break;

		case WT_SKIN:
			return mPanelSkin;
			break;

		case WT_HAIR:
			return mPanelHair;
			break;

		case WT_EYES:
			return mPanelEyes;
			break;

		case WT_SHIRT:
			return mPanelShirt;
			break;

		case WT_PANTS:
			return mPanelPants;
			break;

		case WT_SHOES:
			return mPanelShoes;
			break;

		case WT_SOCKS:
			return mPanelSocks;
			break;

		case WT_JACKET:
			return mPanelJacket;
			break;

		case WT_GLOVES:
			return mPanelGloves;
			break;

		case WT_UNDERSHIRT:
			return mPanelUndershirt;
			break;

		case WT_UNDERPANTS:
			return mPanelUnderpants;
			break;

		case WT_SKIRT:
			return mPanelSkirt;
			break;

		case WT_ALPHA:
			return mPanelAlpha;
			break;

		case WT_TATTOO:
			return mPanelTattoo;
			break;
		default:
			break;
	}
	return NULL;
}

void LLPanelEditWearable::getSortedParams(value_map_t &sorted_params, const std::string &edit_group)
{
	LLWearable::visual_param_vec_t param_list;
	ESex avatar_sex = gAgentAvatar->getSex();

	mWearablePtr->getVisualParams(param_list);

	for (LLWearable::visual_param_vec_t::iterator iter = param_list.begin();
		iter != param_list.end();
		++iter)
	{
		LLViewerVisualParam *param = (LLViewerVisualParam*) *iter;

		if (param->getID() == -1
			|| param->getGroup() != VISUAL_PARAM_GROUP_TWEAKABLE 
			|| param->getEditGroup() != edit_group 
			|| !(param->getSex() & avatar_sex))
		{
			continue;
		}

		value_map_t::value_type vt(-param->getDisplayOrder(), param);
		llassert( sorted_params.find(-param->getDisplayOrder()) == sorted_params.end() ); //check for duplicates
		sorted_params.insert(vt);
	}
}

void LLPanelEditWearable::buildParamList(LLScrollingPanelList *panel_list, value_map_t &sorted_params, LLAccordionCtrlTab *tab)
{
	// sorted_params is sorted according to magnitude of effect from
	// least to greatest.  Adding to the front of the child list
	// reverses that order.
	if( panel_list )
	{
		panel_list->clearPanels();
		value_map_t::iterator end = sorted_params.end();
		S32 height = 0;
		for(value_map_t::iterator it = sorted_params.begin(); it != end; ++it)
		{
			LLPanel::Params p;
			p.name("LLScrollingPanelParam");
			p.rect(LLRect(0, LLScrollingPanelParam::PARAM_PANEL_HEIGHT, LLScrollingPanelParam::PARAM_PANEL_WIDTH, 0 ));
			LLScrollingPanelParam* panel_param = new LLScrollingPanelParam( p, NULL, (*it).second, TRUE, this->getWearable());
			height = panel_list->addPanel( panel_param );
		}
	
		S32 width = tab->getRect().getWidth();
	
		tab->reshape(width,height + tab->getHeaderHeight()+10,FALSE);
	}
}





