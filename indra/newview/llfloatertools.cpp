/** 
 * @file llfloatertools.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llfloatertools.h"

#include "llfontgl.h"
#include "llcoord.h"
//#include "llgl.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldraghandle.h"
#include "llerror.h"
#include "llfloaterbuildoptions.h"
#include "llfloatermediasettings.h"
#include "llfloateropenobject.h"
#include "llfloaterobjectweights.h"
#include "llfloaterreg.h"
#include "llfocusmgr.h"
#include "llmediaentry.h"
#include "llmediactrl.h"
#include "llmenugl.h"
#include "llnotificationsutil.h"
#include "llpanelcontents.h"
#include "llpanelface.h"
#include "llpanelland.h"
#include "llpanelobjectinventory.h"
#include "llpanelobject.h"
#include "llpanelvolume.h"
#include "llpanelpermissions.h"
#include "llparcel.h"
#include "llradiogroup.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llslider.h"
#include "llstatusbar.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltoolbrush.h"
#include "lltoolcomp.h"
#include "lltooldraganddrop.h"
#include "lltoolface.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolgrab.h"
#include "lltoolindividual.h"
#include "lltoolmgr.h"
#include "lltoolpie.h"
#include "lltoolpipette.h"
#include "lltoolplacer.h"
#include "lltoolselectland.h"
#include "lltrans.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llviewerjoystick.h"
#include "llviewerregion.h"
#include "llviewermenu.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"
#include "llvovolume.h"
#include "lluictrlfactory.h"
#include "llmeshrepository.h"

// Globals
LLFloaterTools *gFloaterTools = NULL;
bool LLFloaterTools::sShowObjectCost = true;

const std::string PANEL_NAMES[LLFloaterTools::PANEL_COUNT] =
{
	std::string("General"), 	// PANEL_GENERAL,
	std::string("Object"), 	// PANEL_OBJECT,
	std::string("Features"),	// PANEL_FEATURES,
	std::string("Texture"),	// PANEL_FACE,
	std::string("Content"),	// PANEL_CONTENTS,
};


// Local prototypes
void commit_grid_mode(LLUICtrl *ctrl);
void commit_select_component(void *data);
void click_show_more(void*);
void click_popup_info(void*);
void click_popup_done(void*);
void click_popup_minimize(void*);
void commit_slider_dozer_force(LLUICtrl *);
void click_apply_to_selection(void*);
void commit_radio_group_focus(LLUICtrl* ctrl);
void commit_radio_group_move(LLUICtrl* ctrl);
void commit_radio_group_edit(LLUICtrl* ctrl);
void commit_radio_group_land(LLUICtrl* ctrl);
void commit_slider_zoom(LLUICtrl *ctrl);

/**
 * Class LLLandImpactsObserver
 *
 * An observer class to monitor parcel selection and update
 * the land impacts data from a parcel containing the selected object.
 */
class LLLandImpactsObserver : public LLParcelObserver
{
public:
	virtual void changed()
	{
		LLFloaterTools* tools_floater = LLFloaterReg::getTypedInstance<LLFloaterTools>("build");
		if(tools_floater)
		{
			tools_floater->updateLandImpacts();
			tools_floater->dirty();
		}
	}
};

//static
void*	LLFloaterTools::createPanelPermissions(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelPermissions = new LLPanelPermissions();
	return floater->mPanelPermissions;
}
//static
void*	LLFloaterTools::createPanelObject(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelObject = new LLPanelObject();
	return floater->mPanelObject;
}

//static
void*	LLFloaterTools::createPanelVolume(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelVolume = new LLPanelVolume();
	return floater->mPanelVolume;
}

//static
void*	LLFloaterTools::createPanelFace(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelFace = new LLPanelFace();
	return floater->mPanelFace;
}

//static
void*	LLFloaterTools::createPanelContents(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelContents = new LLPanelContents();
	return floater->mPanelContents;
}

//static
void*	LLFloaterTools::createPanelLandInfo(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelLandInfo = new LLPanelLandInfo();
	return floater->mPanelLandInfo;
}

static	const std::string	toolNames[]={
	"ToolCube",
	"ToolPrism",
	"ToolPyramid",
	"ToolTetrahedron",
	"ToolCylinder",
	"ToolHemiCylinder",
	"ToolCone",
	"ToolHemiCone",
	"ToolSphere",
	"ToolHemiSphere",
	"ToolTorus",
	"ToolTube",
	"ToolRing",
	"ToolTree",
	"ToolGrass"};
LLPCode toolData[]={
	LL_PCODE_CUBE,
	LL_PCODE_PRISM,
	LL_PCODE_PYRAMID,
	LL_PCODE_TETRAHEDRON,
	LL_PCODE_CYLINDER,
	LL_PCODE_CYLINDER_HEMI,
	LL_PCODE_CONE,
	LL_PCODE_CONE_HEMI,
	LL_PCODE_SPHERE,
	LL_PCODE_SPHERE_HEMI,
	LL_PCODE_TORUS,
	LLViewerObject::LL_VO_SQUARE_TORUS,
	LLViewerObject::LL_VO_TRIANGLE_TORUS,
	LL_PCODE_LEGACY_TREE,
	LL_PCODE_LEGACY_GRASS};

BOOL	LLFloaterTools::postBuild()
{	
	// Hide until tool selected
	setVisible(FALSE);

	// Since we constantly show and hide this during drags, don't
	// make sounds on visibility changes.
	setSoundFlags(LLView::SILENT);

	getDragHandle()->setEnabled( !gSavedSettings.getBOOL("ToolboxAutoMove") );

	LLRect rect;
	mBtnFocus			= getChild<LLButton>("button focus");//btn;
	mBtnMove			= getChild<LLButton>("button move");
	mBtnEdit			= getChild<LLButton>("button edit");
	mBtnCreate			= getChild<LLButton>("button create");
	mBtnLand			= getChild<LLButton>("button land" );
	mTextStatus			= getChild<LLTextBox>("text status");
	mRadioGroupFocus	= getChild<LLRadioGroup>("focus_radio_group");
	mRadioGroupMove		= getChild<LLRadioGroup>("move_radio_group");
	mRadioGroupEdit		= getChild<LLRadioGroup>("edit_radio_group");
	mBtnGridOptions		= getChild<LLButton>("Options...");
	mTitleMedia			= getChild<LLMediaCtrl>("title_media");
	mBtnLink			= getChild<LLButton>("link_btn");
	mBtnUnlink			= getChild<LLButton>("unlink_btn");
	
	mCheckSelectIndividual	= getChild<LLCheckBoxCtrl>("checkbox edit linked parts");	
	getChild<LLUICtrl>("checkbox edit linked parts")->setValue((BOOL)gSavedSettings.getBOOL("EditLinkedParts"));
	mCheckSnapToGrid		= getChild<LLCheckBoxCtrl>("checkbox snap to grid");
	getChild<LLUICtrl>("checkbox snap to grid")->setValue((BOOL)gSavedSettings.getBOOL("SnapEnabled"));
	mCheckStretchUniform	= getChild<LLCheckBoxCtrl>("checkbox uniform");
	getChild<LLUICtrl>("checkbox uniform")->setValue((BOOL)gSavedSettings.getBOOL("ScaleUniform"));
	mCheckStretchTexture	= getChild<LLCheckBoxCtrl>("checkbox stretch textures");
	getChild<LLUICtrl>("checkbox stretch textures")->setValue((BOOL)gSavedSettings.getBOOL("ScaleStretchTextures"));
	mComboGridMode			= getChild<LLComboBox>("combobox grid mode");
	mCheckStretchUniformLabel = getChild<LLTextBox>("checkbox uniform label");

	//
	// Create Buttons
	//

	for(size_t t=0; t<LL_ARRAY_SIZE(toolNames); ++t)
	{
		LLButton *found = getChild<LLButton>(toolNames[t]);
		if(found)
		{
			found->setClickedCallback(boost::bind(&LLFloaterTools::setObjectType, toolData[t]));
			mButtons.push_back( found );
		}else{
			llwarns << "Tool button not found! DOA Pending." << llendl;
		}
	}
	mCheckCopySelection = getChild<LLCheckBoxCtrl>("checkbox copy selection");
	getChild<LLUICtrl>("checkbox copy selection")->setValue((BOOL)gSavedSettings.getBOOL("CreateToolCopySelection"));
	mCheckSticky = getChild<LLCheckBoxCtrl>("checkbox sticky");
	getChild<LLUICtrl>("checkbox sticky")->setValue((BOOL)gSavedSettings.getBOOL("CreateToolKeepSelected"));
	mCheckCopyCenters = getChild<LLCheckBoxCtrl>("checkbox copy centers");
	getChild<LLUICtrl>("checkbox copy centers")->setValue((BOOL)gSavedSettings.getBOOL("CreateToolCopyCenters"));
	mCheckCopyRotates = getChild<LLCheckBoxCtrl>("checkbox copy rotates");
	getChild<LLUICtrl>("checkbox copy rotates")->setValue((BOOL)gSavedSettings.getBOOL("CreateToolCopyRotates"));

	mRadioGroupLand			= getChild<LLRadioGroup>("land_radio_group");
	mBtnApplyToSelection	= getChild<LLButton>("button apply to selection");
	mSliderDozerSize		= getChild<LLSlider>("slider brush size");
	getChild<LLUICtrl>("slider brush size")->setValue(gSavedSettings.getF32("LandBrushSize"));
	mSliderDozerForce		= getChild<LLSlider>("slider force");
	// the setting stores the actual force multiplier, but the slider is logarithmic, so we convert here
	getChild<LLUICtrl>("slider force")->setValue(log10(gSavedSettings.getF32("LandBrushForce")));

	mCostTextBorder = getChild<LLViewBorder>("cost_text_border");

	mTab = getChild<LLTabContainer>("Object Info Tabs");
	if(mTab)
	{
		mTab->setFollows(FOLLOWS_TOP | FOLLOWS_LEFT);
		mTab->setBorderVisible(FALSE);
		mTab->selectFirstTab();
	}

	mStatusText["rotate"] = getString("status_rotate");
	mStatusText["scale"] = getString("status_scale");
	mStatusText["move"] = getString("status_move");
	mStatusText["modifyland"] = getString("status_modifyland");
	mStatusText["camera"] = getString("status_camera");
	mStatusText["grab"] = getString("status_grab");
	mStatusText["place"] = getString("status_place");
	mStatusText["selectland"] = getString("status_selectland");

	sShowObjectCost = gSavedSettings.getBOOL("ShowObjectRenderingCost");
	
	return TRUE;
}

// Create the popupview with a dummy center.  It will be moved into place
// during LLViewerWindow's per-frame hover processing.
LLFloaterTools::LLFloaterTools(const LLSD& key)
:	LLFloater(key),
	mBtnFocus(NULL),
	mBtnMove(NULL),
	mBtnEdit(NULL),
	mBtnCreate(NULL),
	mBtnLand(NULL),
	mTextStatus(NULL),

	mRadioGroupFocus(NULL),
	mRadioGroupMove(NULL),
	mRadioGroupEdit(NULL),

	mCheckSelectIndividual(NULL),

	mCheckSnapToGrid(NULL),
	mBtnGridOptions(NULL),
	mTitleMedia(NULL),
	mComboGridMode(NULL),
	mCheckStretchUniform(NULL),
	mCheckStretchTexture(NULL),
	mCheckStretchUniformLabel(NULL),

	mBtnRotateLeft(NULL),
	mBtnRotateReset(NULL),
	mBtnRotateRight(NULL),

	mBtnLink(NULL),
	mBtnUnlink(NULL),

	mBtnDelete(NULL),
	mBtnDuplicate(NULL),
	mBtnDuplicateInPlace(NULL),

	mCheckSticky(NULL),
	mCheckCopySelection(NULL),
	mCheckCopyCenters(NULL),
	mCheckCopyRotates(NULL),
	mRadioGroupLand(NULL),
	mSliderDozerSize(NULL),
	mSliderDozerForce(NULL),
	mBtnApplyToSelection(NULL),

	mTab(NULL),
	mPanelPermissions(NULL),
	mPanelObject(NULL),
	mPanelVolume(NULL),
	mPanelContents(NULL),
	mPanelFace(NULL),
	mPanelLandInfo(NULL),

	mCostTextBorder(NULL),
	mTabLand(NULL),

	mLandImpactsObserver(NULL),

	mDirty(TRUE),
	mNeedMediaTitle(TRUE)
{
	gFloaterTools = this;

	setAutoFocus(FALSE);
	mFactoryMap["General"] = LLCallbackMap(createPanelPermissions, this);//LLPanelPermissions
	mFactoryMap["Object"] = LLCallbackMap(createPanelObject, this);//LLPanelObject
	mFactoryMap["Features"] = LLCallbackMap(createPanelVolume, this);//LLPanelVolume
	mFactoryMap["Texture"] = LLCallbackMap(createPanelFace, this);//LLPanelFace
	mFactoryMap["Contents"] = LLCallbackMap(createPanelContents, this);//LLPanelContents
	mFactoryMap["land info panel"] = LLCallbackMap(createPanelLandInfo, this);//LLPanelLandInfo
	
	mCommitCallbackRegistrar.add("BuildTool.setTool",			boost::bind(&LLFloaterTools::setTool,this, _2));
	mCommitCallbackRegistrar.add("BuildTool.commitZoom",		boost::bind(&commit_slider_zoom, _1));
	mCommitCallbackRegistrar.add("BuildTool.commitRadioFocus",	boost::bind(&commit_radio_group_focus, _1));
	mCommitCallbackRegistrar.add("BuildTool.commitRadioMove",	boost::bind(&commit_radio_group_move,_1));
	mCommitCallbackRegistrar.add("BuildTool.commitRadioEdit",	boost::bind(&commit_radio_group_edit,_1));

	mCommitCallbackRegistrar.add("BuildTool.gridMode",			boost::bind(&commit_grid_mode,_1));
	mCommitCallbackRegistrar.add("BuildTool.selectComponent",	boost::bind(&commit_select_component, this));
	mCommitCallbackRegistrar.add("BuildTool.gridOptions",		boost::bind(&LLFloaterTools::onClickGridOptions,this));
	mCommitCallbackRegistrar.add("BuildTool.applyToSelection",	boost::bind(&click_apply_to_selection, this));
	mCommitCallbackRegistrar.add("BuildTool.commitRadioLand",	boost::bind(&commit_radio_group_land,_1));
	mCommitCallbackRegistrar.add("BuildTool.LandBrushForce",	boost::bind(&commit_slider_dozer_force,_1));
	mCommitCallbackRegistrar.add("BuildTool.AddMedia",			boost::bind(&LLFloaterTools::onClickBtnAddMedia,this));
	mCommitCallbackRegistrar.add("BuildTool.DeleteMedia",		boost::bind(&LLFloaterTools::onClickBtnDeleteMedia,this));
	mCommitCallbackRegistrar.add("BuildTool.EditMedia",			boost::bind(&LLFloaterTools::onClickBtnEditMedia,this));

	mCommitCallbackRegistrar.add("BuildTool.LinkObjects",		boost::bind(&LLSelectMgr::linkObjects, LLSelectMgr::getInstance()));
	mCommitCallbackRegistrar.add("BuildTool.UnlinkObjects",		boost::bind(&LLSelectMgr::unlinkObjects, LLSelectMgr::getInstance()));

	mLandImpactsObserver = new LLLandImpactsObserver();
	LLViewerParcelMgr::getInstance()->addObserver(mLandImpactsObserver);
}

LLFloaterTools::~LLFloaterTools()
{
	// children automatically deleted
	gFloaterTools = NULL;

	LLViewerParcelMgr::getInstance()->removeObserver(mLandImpactsObserver);
	delete mLandImpactsObserver;
}

void LLFloaterTools::setStatusText(const std::string& text)
{
	std::map<std::string, std::string>::iterator iter = mStatusText.find(text);
	if (iter != mStatusText.end())
	{
		mTextStatus->setText(iter->second);
	}
	else
	{
		mTextStatus->setText(text);
	}
}

void LLFloaterTools::refresh()
{
	const S32 INFO_WIDTH = getRect().getWidth();
	const S32 INFO_HEIGHT = 384;
	LLRect object_info_rect(0, 0, INFO_WIDTH, -INFO_HEIGHT);
	BOOL all_volume = LLSelectMgr::getInstance()->selectionAllPCode( LL_PCODE_VOLUME );

	S32 idx_features = mTab->getPanelIndexByTitle(PANEL_NAMES[PANEL_FEATURES]);
	S32 idx_face = mTab->getPanelIndexByTitle(PANEL_NAMES[PANEL_FACE]);
	S32 idx_contents = mTab->getPanelIndexByTitle(PANEL_NAMES[PANEL_CONTENTS]);

	S32 selected_index = mTab->getCurrentPanelIndex();

	if (!all_volume && (selected_index == idx_features || selected_index == idx_face ||
		selected_index == idx_contents))
	{
		mTab->selectFirstTab();
	}

	mTab->enableTabButton(idx_features, all_volume);
	mTab->enableTabButton(idx_face, all_volume);
	mTab->enableTabButton(idx_contents, all_volume);

	// Refresh object and prim count labels
	LLLocale locale(LLLocale::USER_LOCALE);
#if 0
	if (!gMeshRepo.meshRezEnabled())
	{		
		std::string obj_count_string;
		LLResMgr::getInstance()->getIntegerString(obj_count_string, LLSelectMgr::getInstance()->getSelection()->getRootObjectCount());
		getChild<LLUICtrl>("selection_count")->setTextArg("[OBJ_COUNT]", obj_count_string);
		std::string prim_count_string;
		LLResMgr::getInstance()->getIntegerString(prim_count_string, LLSelectMgr::getInstance()->getSelection()->getObjectCount());
		getChild<LLUICtrl>("selection_count")->setTextArg("[PRIM_COUNT]", prim_count_string);

		// calculate selection rendering cost
		if (sShowObjectCost)
		{
			std::string prim_cost_string;
			S32 render_cost = LLSelectMgr::getInstance()->getSelection()->getSelectedObjectRenderCost();
			LLResMgr::getInstance()->getIntegerString(prim_cost_string, render_cost);
			getChild<LLUICtrl>("RenderingCost")->setTextArg("[COUNT]", prim_cost_string);
		}
		
		// disable the object and prim counts if nothing selected
		bool have_selection = ! LLSelectMgr::getInstance()->getSelection()->isEmpty();
		getChildView("obj_count")->setEnabled(have_selection);
		getChildView("prim_count")->setEnabled(have_selection);
		getChildView("RenderingCost")->setEnabled(have_selection && sShowObjectCost);
	}
	else
#endif
	{
		F32 link_cost  = LLSelectMgr::getInstance()->getSelection()->getSelectedLinksetCost();
		S32 link_count = LLSelectMgr::getInstance()->getSelection()->getRootObjectCount();

		LLCrossParcelFunctor func;
		if (LLSelectMgr::getInstance()->getSelection()->applyToRootObjects(&func, true))
		{
			// Selection crosses parcel bounds.
			// We don't display remaining land capacity in this case.
			const LLStringExplicit empty_str("");
			childSetTextArg("remaining_capacity", "[CAPACITY_STRING]", empty_str);
		}
		else
		{
			LLViewerObject* selected_object = mObjectSelection->getFirstObject();
			if (selected_object)
			{
				// Select a parcel at the currently selected object's position.
				LLViewerParcelMgr::getInstance()->selectParcelAt(selected_object->getPositionGlobal());
			}
			else
			{
				llwarns << "Failed to get selected object" << llendl;
			}
		}

		LLStringUtil::format_map_t selection_args;
		selection_args["OBJ_COUNT"] = llformat("%.1d", link_count);
		selection_args["LAND_IMPACT"] = llformat("%.1d", (S32)link_cost);

		std::ostringstream selection_info;

		selection_info << getString("status_selectcount", selection_args);

		getChild<LLTextBox>("selection_count")->setText(selection_info.str());

		bool have_selection = !LLSelectMgr::getInstance()->getSelection()->isEmpty();
		childSetVisible("selection_count",  have_selection);
		childSetVisible("remaining_capacity", have_selection);
		childSetVisible("selection_empty", !have_selection);
	}


	// Refresh child tabs
	mPanelPermissions->refresh();
	mPanelObject->refresh();
	mPanelVolume->refresh();
	mPanelFace->refresh();
	refreshMedia();
	mPanelContents->refresh();
	mPanelLandInfo->refresh();

	// Refresh the advanced weights floater
	LLFloaterObjectWeights* object_weights_floater = LLFloaterReg::findTypedInstance<LLFloaterObjectWeights>("object_weights");
	if(object_weights_floater && object_weights_floater->getVisible())
	{
		object_weights_floater->refresh();
	}
}

void LLFloaterTools::draw()
{
	if (mDirty)
	{
		refresh();
		mDirty = FALSE;
	}

	// grab media name/title and update the UI widget
	updateMediaTitle();

	//	mCheckSelectIndividual->set(gSavedSettings.getBOOL("EditLinkedParts"));
	LLFloater::draw();
}

void LLFloaterTools::dirty()
{
	mDirty = TRUE; 
	LLFloaterOpenObject* instance = LLFloaterReg::findTypedInstance<LLFloaterOpenObject>("openobject");
	if (instance) instance->dirty();
}

// Clean up any tool state that should not persist when the
// floater is closed.
void LLFloaterTools::resetToolState()
{
	gCameraBtnZoom = TRUE;
	gCameraBtnOrbit = FALSE;
	gCameraBtnPan = FALSE;

	gGrabBtnSpin = FALSE;
	gGrabBtnVertical = FALSE;
}

void LLFloaterTools::updatePopup(LLCoordGL center, MASK mask)
{
	LLTool *tool = LLToolMgr::getInstance()->getCurrentTool();

	// HACK to allow seeing the buttons when you have the app in a window.
	// Keep the visibility the same as it 
	if (tool == gToolNull)
	{
		return;
	}

	if ( isMinimized() )
	{	// SL looks odd if we draw the tools while the window is minimized
		return;
	}
	
	// Focus buttons
	BOOL focus_visible = (	tool == LLToolCamera::getInstance() );

	mBtnFocus	->setToggleState( focus_visible );

	mRadioGroupFocus->setVisible( focus_visible );
	getChildView("slider zoom")->setVisible( focus_visible);
	getChildView("slider zoom")->setEnabled(gCameraBtnZoom);

	if (!gCameraBtnOrbit &&
		!gCameraBtnPan &&
		!(mask == MASK_ORBIT) &&
		!(mask == (MASK_ORBIT | MASK_ALT)) &&
		!(mask == MASK_PAN) &&
		!(mask == (MASK_PAN | MASK_ALT)) )
	{
		mRadioGroupFocus->setValue("radio zoom");
	}
	else if (	gCameraBtnOrbit || 
				(mask == MASK_ORBIT) ||
				(mask == (MASK_ORBIT | MASK_ALT)) )
	{
		mRadioGroupFocus->setValue("radio orbit");
	}
	else if (	gCameraBtnPan ||
				(mask == MASK_PAN) ||
				(mask == (MASK_PAN | MASK_ALT)) )
	{
		mRadioGroupFocus->setValue("radio pan");
	}

	// multiply by correction factor because volume sliders go [0, 0.5]
	getChild<LLUICtrl>("slider zoom")->setValue(gAgentCamera.getCameraZoomFraction() * 0.5f);

	// Move buttons
	BOOL move_visible = (tool == LLToolGrab::getInstance());

	if (mBtnMove) mBtnMove	->setToggleState( move_visible );

	// HACK - highlight buttons for next click
	mRadioGroupMove->setVisible(move_visible);
	if (!gGrabBtnSpin && 
		!gGrabBtnVertical &&
		!(mask == MASK_VERTICAL) && 
		!(mask == MASK_SPIN) )
	{
		mRadioGroupMove->setValue("radio move");
	}
	else if (gGrabBtnVertical || 
			 (mask == MASK_VERTICAL) )
	{
		mRadioGroupMove->setValue("radio lift");
	}
	else if (gGrabBtnSpin || 
			 (mask == MASK_SPIN) )
	{
		mRadioGroupMove->setValue("radio spin");
	}

	// Edit buttons
	BOOL edit_visible = tool == LLToolCompTranslate::getInstance() ||
						tool == LLToolCompRotate::getInstance() ||
						tool == LLToolCompScale::getInstance() ||
						tool == LLToolFace::getInstance() ||
						tool == LLToolIndividual::getInstance() ||
						tool == LLToolPipette::getInstance();

	mBtnEdit	->setToggleState( edit_visible );
	mRadioGroupEdit->setVisible( edit_visible );
	bool linked_parts = gSavedSettings.getBOOL("EditLinkedParts");
	getChildView("RenderingCost")->setVisible( !linked_parts && (edit_visible || focus_visible || move_visible) && sShowObjectCost);

	mBtnLink->setVisible(edit_visible);
	mBtnUnlink->setVisible(edit_visible);

	mBtnLink->setEnabled(LLSelectMgr::instance().enableLinkObjects());
	mBtnUnlink->setEnabled(LLSelectMgr::instance().enableUnlinkObjects());

	if (mCheckSelectIndividual)
	{
		mCheckSelectIndividual->setVisible(edit_visible);
		//mCheckSelectIndividual->set(gSavedSettings.getBOOL("EditLinkedParts"));
	}

	if ( tool == LLToolCompTranslate::getInstance() )
	{
		mRadioGroupEdit->setValue("radio position");
	}
	else if ( tool == LLToolCompRotate::getInstance() )
	{
		mRadioGroupEdit->setValue("radio rotate");
	}
	else if ( tool == LLToolCompScale::getInstance() )
	{
		mRadioGroupEdit->setValue("radio stretch");
	}
	else if ( tool == LLToolFace::getInstance() )
	{
		mRadioGroupEdit->setValue("radio select face");
	}

	if (mComboGridMode) 
	{
		mComboGridMode->setVisible( edit_visible );
		S32 index = mComboGridMode->getCurrentIndex();
		mComboGridMode->removeall();

		switch (mObjectSelection->getSelectType())
		{
			case SELECT_TYPE_HUD:
				mComboGridMode->add(getString("grid_screen_text"));
				mComboGridMode->add(getString("grid_local_text"));
				break;
			case SELECT_TYPE_WORLD:
				mComboGridMode->add(getString("grid_world_text"));
				mComboGridMode->add(getString("grid_local_text"));
				mComboGridMode->add(getString("grid_reference_text"));
				break;
			case SELECT_TYPE_ATTACHMENT:
				mComboGridMode->add(getString("grid_attachment_text"));
				mComboGridMode->add(getString("grid_local_text"));
				mComboGridMode->add(getString("grid_reference_text"));
				break;
		}

		mComboGridMode->setCurrentByIndex(index);
	}

	// Snap to grid disabled for grab tool - very confusing
	if (mCheckSnapToGrid) mCheckSnapToGrid->setVisible( edit_visible /* || tool == LLToolGrab::getInstance() */ );
	if (mBtnGridOptions) mBtnGridOptions->setVisible( edit_visible /* || tool == LLToolGrab::getInstance() */ );

	//mCheckSelectLinked	->setVisible( edit_visible );
	if (mCheckStretchUniform) mCheckStretchUniform->setVisible( edit_visible );
	if (mCheckStretchTexture) mCheckStretchTexture->setVisible( edit_visible );
	if (mCheckStretchUniformLabel) mCheckStretchUniformLabel->setVisible( edit_visible );

	// Create buttons
	BOOL create_visible = (tool == LLToolCompCreate::getInstance());

	mBtnCreate	->setToggleState(	tool == LLToolCompCreate::getInstance() );

	if (mCheckCopySelection
		&& mCheckCopySelection->get())
	{
		// don't highlight any placer button
		for (std::vector<LLButton*>::size_type i = 0; i < mButtons.size(); i++)
		{
			mButtons[i]->setToggleState(FALSE);
			mButtons[i]->setVisible( create_visible );
		}
	}
	else
	{
		// Highlight the correct placer button
		for( S32 t = 0; t < (S32)mButtons.size(); t++ )
		{
			LLPCode pcode = LLToolPlacer::getObjectType();
			LLPCode button_pcode = toolData[t];
			BOOL state = (pcode == button_pcode);
			mButtons[t]->setToggleState( state );
			mButtons[t]->setVisible( create_visible );
		}
	}

	if (mCheckSticky) mCheckSticky		->setVisible( create_visible );
	if (mCheckCopySelection) mCheckCopySelection	->setVisible( create_visible );
	if (mCheckCopyCenters) mCheckCopyCenters	->setVisible( create_visible );
	if (mCheckCopyRotates) mCheckCopyRotates	->setVisible( create_visible );

	if (mCheckCopyCenters && mCheckCopySelection) mCheckCopyCenters->setEnabled( mCheckCopySelection->get() );
	if (mCheckCopyRotates && mCheckCopySelection) mCheckCopyRotates->setEnabled( mCheckCopySelection->get() );

	// Land buttons
	BOOL land_visible = (tool == LLToolBrushLand::getInstance() || tool == LLToolSelectLand::getInstance() );

	mCostTextBorder->setVisible(!land_visible);

	if (mBtnLand)	mBtnLand	->setToggleState( land_visible );

	mRadioGroupLand->setVisible( land_visible );
	if ( tool == LLToolSelectLand::getInstance() )
	{
		mRadioGroupLand->setValue("radio select land");
	}
	else if ( tool == LLToolBrushLand::getInstance() )
	{
		S32 dozer_mode = gSavedSettings.getS32("RadioLandBrushAction");
		switch(dozer_mode)
		{
		case 0:
			mRadioGroupLand->setValue("radio flatten");
			break;
		case 1:
			mRadioGroupLand->setValue("radio raise");
			break;
		case 2:
			mRadioGroupLand->setValue("radio lower");
			break;
		case 3:
			mRadioGroupLand->setValue("radio smooth");
			break;
		case 4:
			mRadioGroupLand->setValue("radio noise");
			break;
		case 5:
			mRadioGroupLand->setValue("radio revert");
			break;
		default:
			break;
		}
	}

	if (mBtnApplyToSelection)
	{
		mBtnApplyToSelection->setVisible( land_visible );
		mBtnApplyToSelection->setEnabled( land_visible && !LLViewerParcelMgr::getInstance()->selectionEmpty() && tool != LLToolSelectLand::getInstance());
	}
	if (mSliderDozerSize)
	{
		mSliderDozerSize	->setVisible( land_visible );
		getChildView("Bulldozer:")->setVisible( land_visible);
		getChildView("Dozer Size:")->setVisible( land_visible);
	}
	if (mSliderDozerForce)
	{
		mSliderDozerForce	->setVisible( land_visible );
		getChildView("Strength:")->setVisible( land_visible);
	}

	bool have_selection = !LLSelectMgr::getInstance()->getSelection()->isEmpty();

	getChildView("selection_count")->setVisible(!land_visible && have_selection);
	getChildView("remaining_capacity")->setVisible(!land_visible && have_selection);
	getChildView("selection_empty")->setVisible(!land_visible && !have_selection);
	
	mTab->setVisible(!land_visible);
	mPanelLandInfo->setVisible(land_visible);
}


// virtual
BOOL LLFloaterTools::canClose()
{
	// don't close when quitting, so camera will stay put
	return !LLApp::isExiting();
}

// virtual
void LLFloaterTools::onOpen(const LLSD& key)
{
	mParcelSelection = LLViewerParcelMgr::getInstance()->getFloatingParcelSelection();
	mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();
	
	std::string panel = key.asString();
	if (!panel.empty())
	{
		mTab->selectTabByName(panel);
	}
	
	//gMenuBarView->setItemVisible("BuildTools", TRUE);
}

// virtual
void LLFloaterTools::onClose(bool app_quitting)
{
	mTab->setVisible(FALSE);

	LLViewerJoystick::getInstance()->moveAvatar(false);

	// destroy media source used to grab media title
	if( mTitleMedia )
		mTitleMedia->unloadMediaSource();

    // Different from handle_reset_view in that it doesn't actually 
	//   move the camera if EditCameraMovement is not set.
	gAgentCamera.resetView(gSavedSettings.getBOOL("EditCameraMovement"));
	
	// exit component selection mode
	LLSelectMgr::getInstance()->promoteSelectionToRoot();
	gSavedSettings.setBOOL("EditLinkedParts", FALSE);

	gViewerWindow->showCursor();

	resetToolState();

	mParcelSelection = NULL;
	mObjectSelection = NULL;

	// Switch back to basic toolset
	LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
	// we were already in basic toolset, using build tools
	// so manually reset tool to default (pie menu tool)
	LLToolMgr::getInstance()->getCurrentToolset()->selectFirstTool();

	//gMenuBarView->setItemVisible("BuildTools", FALSE);
	LLFloaterReg::hideInstance("media_settings");

	// hide the advanced object weights floater
	LLFloaterReg::hideInstance("object_weights");
}

void click_popup_info(void*)
{
}

void click_popup_done(void*)
{
	handle_reset_view();
}

void commit_radio_group_move(LLUICtrl* ctrl)
{
	LLRadioGroup* group = (LLRadioGroup*)ctrl;
	std::string selected = group->getValue().asString();
	if (selected == "radio move")
	{
		gGrabBtnVertical = FALSE;
		gGrabBtnSpin = FALSE;
	}
	else if (selected == "radio lift")
	{
		gGrabBtnVertical = TRUE;
		gGrabBtnSpin = FALSE;
	}
	else if (selected == "radio spin")
	{
		gGrabBtnVertical = FALSE;
		gGrabBtnSpin = TRUE;
	}
}

void commit_radio_group_focus(LLUICtrl* ctrl)
{
	LLRadioGroup* group = (LLRadioGroup*)ctrl;
	std::string selected = group->getValue().asString();
	if (selected == "radio zoom")
	{
		gCameraBtnZoom = TRUE;
		gCameraBtnOrbit = FALSE;
		gCameraBtnPan = FALSE;
	}
	else if (selected == "radio orbit")
	{
		gCameraBtnZoom = FALSE;
		gCameraBtnOrbit = TRUE;
		gCameraBtnPan = FALSE;
	}
	else if (selected == "radio pan")
	{
		gCameraBtnZoom = FALSE;
		gCameraBtnOrbit = FALSE;
		gCameraBtnPan = TRUE;
	}
}

void commit_slider_zoom(LLUICtrl *ctrl)
{
	// renormalize value, since max "volume" level is 0.5 for some reason
	F32 zoom_level = (F32)ctrl->getValue().asReal() * 2.f; // / 0.5f;
	gAgentCamera.setCameraZoomFraction(zoom_level);
}

void commit_slider_dozer_force(LLUICtrl *ctrl)
{
	// the slider is logarithmic, so we exponentiate to get the actual force multiplier
	F32 dozer_force = pow(10.f, (F32)ctrl->getValue().asReal());
	gSavedSettings.setF32("LandBrushForce", dozer_force);
}

void click_apply_to_selection(void*)
{
	LLToolBrushLand::getInstance()->modifyLandInSelectionGlobal();
}

void commit_radio_group_edit(LLUICtrl *ctrl)
{
	S32 show_owners = gSavedSettings.getBOOL("ShowParcelOwners");

	LLRadioGroup* group = (LLRadioGroup*)ctrl;
	std::string selected = group->getValue().asString();
	if (selected == "radio position")
	{
		LLFloaterTools::setEditTool( LLToolCompTranslate::getInstance() );
	}
	else if (selected == "radio rotate")
	{
		LLFloaterTools::setEditTool( LLToolCompRotate::getInstance() );
	}
	else if (selected == "radio stretch")
	{
		LLFloaterTools::setEditTool( LLToolCompScale::getInstance() );
	}
	else if (selected == "radio select face")
	{
		LLFloaterTools::setEditTool( LLToolFace::getInstance() );
	}
	gSavedSettings.setBOOL("ShowParcelOwners", show_owners);
}

void commit_radio_group_land(LLUICtrl* ctrl)
{
	LLRadioGroup* group = (LLRadioGroup*)ctrl;
	std::string selected = group->getValue().asString();
	if (selected == "radio select land")
	{
		LLFloaterTools::setEditTool( LLToolSelectLand::getInstance() );
	}
	else
	{
		LLFloaterTools::setEditTool( LLToolBrushLand::getInstance() );
		S32 dozer_mode = gSavedSettings.getS32("RadioLandBrushAction");
		if (selected == "radio flatten")
			dozer_mode = 0;
		else if (selected == "radio raise")
			dozer_mode = 1;
		else if (selected == "radio lower")
			dozer_mode = 2;
		else if (selected == "radio smooth")
			dozer_mode = 3;
		else if (selected == "radio noise")
			dozer_mode = 4;
		else if (selected == "radio revert")
			dozer_mode = 5;
		gSavedSettings.setS32("RadioLandBrushAction", dozer_mode);
	}
}

void commit_select_component(void *data)
{
	LLFloaterTools* floaterp = (LLFloaterTools*)data;

	//forfeit focus
	if (gFocusMgr.childHasKeyboardFocus(floaterp))
	{
		gFocusMgr.setKeyboardFocus(NULL);
	}

	BOOL select_individuals = floaterp->mCheckSelectIndividual->get();
	gSavedSettings.setBOOL("EditLinkedParts", select_individuals);
	floaterp->dirty();

	if (select_individuals)
	{
		LLSelectMgr::getInstance()->demoteSelectionToIndividuals();
	}
	else
	{
		LLSelectMgr::getInstance()->promoteSelectionToRoot();
	}
}

// static 
void LLFloaterTools::setObjectType( LLPCode pcode )
{
	LLToolPlacer::setObjectType( pcode );
	gSavedSettings.setBOOL("CreateToolCopySelection", FALSE);
	gFocusMgr.setMouseCapture(NULL);
}

void commit_grid_mode(LLUICtrl *ctrl)
{
	LLComboBox* combo = (LLComboBox*)ctrl;

	LLSelectMgr::getInstance()->setGridMode((EGridMode)combo->getCurrentIndex());
}


void LLFloaterTools::onClickGridOptions()
{
	LLFloaterReg::showInstance("build_options");
	// RN: this makes grid options dependent on build tools window
	//floaterp->addDependentFloater(LLFloaterBuildOptions::getInstance(), FALSE);
}

// static
void LLFloaterTools::setEditTool(void* tool_pointer)
{
	LLTool *tool = (LLTool *)tool_pointer;
	LLToolMgr::getInstance()->getCurrentToolset()->selectTool( tool );
}

void LLFloaterTools::setTool(const LLSD& user_data)
{
	std::string control_name = user_data.asString();
	if(control_name == "Focus")
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool((LLTool *) LLToolCamera::getInstance() );
	else if (control_name == "Move" )
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool( (LLTool *)LLToolGrab::getInstance() );
	else if (control_name == "Edit" )
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool( (LLTool *) LLToolCompTranslate::getInstance());
	else if (control_name == "Create" )
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool( (LLTool *) LLToolCompCreate::getInstance());
	else if (control_name == "Land" )
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool( (LLTool *) LLToolSelectLand::getInstance());
	else
		llwarns<<" no parameter name "<<control_name<<" found!! No Tool selected!!"<< llendl;
}

void LLFloaterTools::onFocusReceived()
{
	LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
	LLFloater::onFocusReceived();
}

// Media stuff
void LLFloaterTools::refreshMedia()
{
	getMediaState();	
}

bool LLFloaterTools::selectedMediaEditable()
{
	U32 owner_mask_on;
	U32 owner_mask_off;
	U32 valid_owner_perms = LLSelectMgr::getInstance()->selectGetPerm( PERM_OWNER, 
																	  &owner_mask_on, &owner_mask_off );
	U32 group_mask_on;
	U32 group_mask_off;
	U32 valid_group_perms = LLSelectMgr::getInstance()->selectGetPerm( PERM_GROUP, 
																	  &group_mask_on, &group_mask_off );
	U32 everyone_mask_on;
	U32 everyone_mask_off;
	S32 valid_everyone_perms = LLSelectMgr::getInstance()->selectGetPerm( PERM_EVERYONE, 
																		 &everyone_mask_on, &everyone_mask_off );
	
	bool selected_Media_editable = false;
	
	// if perms we got back are valid
	if ( valid_owner_perms &&
		valid_group_perms && 
		valid_everyone_perms )
	{
		
		if ( ( owner_mask_on & PERM_MODIFY ) ||
			( group_mask_on & PERM_MODIFY ) || 
			( group_mask_on & PERM_MODIFY ) )
		{
			selected_Media_editable = true;
		}
		else
			// user is NOT allowed to press the RESET button
		{
			selected_Media_editable = false;
		};
	};
	
	return selected_Media_editable;
}

void LLFloaterTools::updateLandImpacts()
{
	LLParcel *parcel = mParcelSelection->getParcel();
	if (!parcel)
	{
		return;
	}

	S32 rezzed_prims = parcel->getSimWidePrimCount();
	S32 total_capacity = parcel->getSimWideMaxPrimCapacity();

	std::string remaining_capacity_str = "";

	bool show_mesh_cost = gMeshRepo.meshRezEnabled();
	if (show_mesh_cost)
	{
		LLStringUtil::format_map_t remaining_capacity_args;
		remaining_capacity_args["LAND_CAPACITY"] = llformat("%d", total_capacity - rezzed_prims);
		remaining_capacity_str = getString("status_remaining_capacity", remaining_capacity_args);
	}

	childSetTextArg("remaining_capacity", "[CAPACITY_STRING]", remaining_capacity_str);

	// Update land impacts info in the weights floater
	LLFloaterObjectWeights* object_weights_floater = LLFloaterReg::findTypedInstance<LLFloaterObjectWeights>("object_weights");
	if(object_weights_floater)
	{
		object_weights_floater->updateLandImpacts(parcel);
	}
}

void LLFloaterTools::getMediaState()
{
	LLObjectSelectionHandle selected_objects =LLSelectMgr::getInstance()->getSelection();
	LLViewerObject* first_object = selected_objects->getFirstObject();
	LLTextBox* media_info = getChild<LLTextBox>("media_info");
	
	if( !(first_object 
		  && first_object->getPCode() == LL_PCODE_VOLUME
		  &&first_object->permModify() 
	      ))
	{
		getChildView("Add_Media")->setEnabled(FALSE);
		media_info->clear();
		clearMediaSettings();
		return;
	}
	
	std::string url = first_object->getRegion()->getCapability("ObjectMedia");
	bool has_media_capability = (!url.empty());
	
	if(!has_media_capability)
	{
		getChildView("Add_Media")->setEnabled(FALSE);
		LL_WARNS("LLFloaterTools: media") << "Media not enabled (no capability) in this region!" << LL_ENDL;
		clearMediaSettings();
		return;
	}
	
	BOOL is_nonpermanent_enforced = (LLSelectMgr::getInstance()->getSelection()->getFirstRootNode() 
		&& LLSelectMgr::getInstance()->selectGetRootsNonPermanentEnforced())
		|| LLSelectMgr::getInstance()->selectGetNonPermanentEnforced();
	bool editable = is_nonpermanent_enforced && (first_object->permModify() || selectedMediaEditable());

	// Check modify permissions and whether any selected objects are in
	// the process of being fetched.  If they are, then we're not editable
	if (editable)
	{
		LLObjectSelection::iterator iter = selected_objects->begin(); 
		LLObjectSelection::iterator end = selected_objects->end();
		for ( ; iter != end; ++iter)
		{
			LLSelectNode* node = *iter;
			LLVOVolume* object = dynamic_cast<LLVOVolume*>(node->getObject());
			if (NULL != object)
			{
				if (!object->permModify())
				{
					LL_INFOS("LLFloaterTools: media")
						<< "Selection not editable due to lack of modify permissions on object id "
						<< object->getID() << LL_ENDL;
					
					editable = false;
					break;
				}
				// XXX DISABLE this for now, because when the fetch finally 
				// does come in, the state of this floater doesn't properly
				// update.  Re-selecting fixes the problem, but there is 
				// contention as to whether this is a sufficient solution.
//				if (object->isMediaDataBeingFetched())
//				{
//					LL_INFOS("LLFloaterTools: media")
//						<< "Selection not editable due to media data being fetched for object id "
//						<< object->getID() << LL_ENDL;
//						
//					editable = false;
//					break;
//				}
			}
		}
	}

	// Media settings
	bool bool_has_media = false;
	struct media_functor : public LLSelectedTEGetFunctor<bool>
	{
		bool get(LLViewerObject* object, S32 face)
		{
			LLTextureEntry *te = object->getTE(face);
			if (te)
			{
				return te->hasMedia();
			}
			return false;
		}
	} func;
	
	
	// check if all faces have media(or, all dont have media)
	LLFloaterMediaSettings::getInstance()->mIdenticalHasMediaInfo = selected_objects->getSelectedTEValue( &func, bool_has_media );
	
	const LLMediaEntry default_media_data;
	
	struct functor_getter_media_data : public LLSelectedTEGetFunctor< LLMediaEntry>
    {
		functor_getter_media_data(const LLMediaEntry& entry): mMediaEntry(entry) {}	

        LLMediaEntry get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return *(object->getTE(face)->getMediaData());
			return mMediaEntry;
        };
		
		const LLMediaEntry& mMediaEntry;
		
    } func_media_data(default_media_data);

	LLMediaEntry media_data_get;
    LLFloaterMediaSettings::getInstance()->mMultipleMedia = !(selected_objects->getSelectedTEValue( &func_media_data, media_data_get ));
	
	std::string multi_media_info_str = LLTrans::getString("Multiple Media");
	std::string media_title = "";
	mNeedMediaTitle = false;
	// update UI depending on whether "object" (prim or face) has media
	// and whether or not you are allowed to edit it.
	
	getChildView("Add_Media")->setEnabled(editable);
	// IF all the faces have media (or all dont have media)
	if ( LLFloaterMediaSettings::getInstance()->mIdenticalHasMediaInfo )
	{
		// TODO: get media title and set it.
		media_info->clear();
		// if identical is set, all faces are same (whether all empty or has the same media)
		if(!(LLFloaterMediaSettings::getInstance()->mMultipleMedia) )
		{
			// Media data is valid
			if(media_data_get!=default_media_data)
			{
				// initial media title is the media URL (until we get the name)
				media_title = media_data_get.getHomeURL();

				// kick off a navigate and flag that we need to update the title
				navigateToTitleMedia( media_data_get.getHomeURL() );
				mNeedMediaTitle = true;
			}
			// else all faces might be empty. 
		}
		else // there' re Different Medias' been set on on the faces.
		{
			media_title = multi_media_info_str;
			mNeedMediaTitle = false;
		}
		
		getChildView("media_tex")->setEnabled(bool_has_media && editable);
		getChildView("edit_media")->setEnabled(bool_has_media && LLFloaterMediaSettings::getInstance()->mIdenticalHasMediaInfo && editable );
		getChildView("delete_media")->setEnabled(bool_has_media && editable );
		getChildView("add_media")->setEnabled(( ! bool_has_media ) && editable );
			// TODO: display a list of all media on the face - use 'identical' flag
	}
	else // not all face has media but at least one does.
	{
		// seleted faces have not identical value
		LLFloaterMediaSettings::getInstance()->mMultipleValidMedia = selected_objects->isMultipleTEValue(&func_media_data, default_media_data );
	
		if(LLFloaterMediaSettings::getInstance()->mMultipleValidMedia)
		{
			media_title = multi_media_info_str;
			mNeedMediaTitle = false;
		}
		else
		{
			// Media data is valid
			if(media_data_get!=default_media_data)
			{
				// initial media title is the media URL (until we get the name)
				media_title = media_data_get.getHomeURL();

				// kick off a navigate and flag that we need to update the title
				navigateToTitleMedia( media_data_get.getHomeURL() );
				mNeedMediaTitle = true;
			}
		}
		
		getChildView("media_tex")->setEnabled(TRUE);
		getChildView("edit_media")->setEnabled(LLFloaterMediaSettings::getInstance()->mIdenticalHasMediaInfo);
		getChildView("delete_media")->setEnabled(TRUE);
		getChildView("add_media")->setEnabled(FALSE );
	}
	media_info->setText(media_title);
	
	// load values for media settings
	updateMediaSettings();
	
	LLFloaterMediaSettings::initValues(mMediaSettings, editable );
}


//////////////////////////////////////////////////////////////////////////////
// called when a user wants to add media to a prim or prim face
void LLFloaterTools::onClickBtnAddMedia()
{
	// check if multiple faces are selected
	if(LLSelectMgr::getInstance()->getSelection()->isMultipleTESelected())
	{
		LLNotificationsUtil::add("MultipleFacesSelected", LLSD(), LLSD(), multipleFacesSelectedConfirm);
	}
	else
	{
		onClickBtnEditMedia();
	}
}

// static
bool LLFloaterTools::multipleFacesSelectedConfirm(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch( option )
	{
		case 0:  // "Yes"
			gFloaterTools->onClickBtnEditMedia();
			break;
		case 1:  // "No"
		default:
			break;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////
// called when a user wants to edit existing media settings on a prim or prim face
// TODO: test if there is media on the item and only allow editing if present
void LLFloaterTools::onClickBtnEditMedia()
{
	refreshMedia();
	LLFloaterReg::showInstance("media_settings");	
}

//////////////////////////////////////////////////////////////////////////////
// called when a user wants to delete media from a prim or prim face
void LLFloaterTools::onClickBtnDeleteMedia()
{
	LLNotificationsUtil::add("DeleteMedia", LLSD(), LLSD(), deleteMediaConfirm);
}


// static
bool LLFloaterTools::deleteMediaConfirm(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch( option )
	{
		case 0:  // "Yes"
			LLSelectMgr::getInstance()->selectionSetMedia( 0, LLSD() );
			if(LLFloaterReg::instanceVisible("media_settings"))
			{
				LLFloaterReg::hideInstance("media_settings");
			}
			break;
			
		case 1:  // "No"
		default:
			break;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////
//
void LLFloaterTools::clearMediaSettings()
{
	LLFloaterMediaSettings::clearValues(false);
}

//////////////////////////////////////////////////////////////////////////////
//
void LLFloaterTools::navigateToTitleMedia( const std::string url )
{
	if ( mTitleMedia )
	{
		LLPluginClassMedia* media_plugin = mTitleMedia->getMediaPlugin();
		if ( media_plugin )
		{
			// if it's a movie, we don't want to hear it
			media_plugin->setVolume( 0 );
		};
		mTitleMedia->navigateTo( url );
	};
}

//////////////////////////////////////////////////////////////////////////////
//
void LLFloaterTools::updateMediaTitle()
{
	// only get the media name if we need it
	if ( ! mNeedMediaTitle )
		return;

	// get plugin impl
	LLPluginClassMedia* media_plugin = mTitleMedia->getMediaPlugin();
	if ( media_plugin )
	{
		// get the media name (asynchronous - must call repeatedly)
		std::string media_title = media_plugin->getMediaName();

		// only replace the title if what we get contains something
		if ( ! media_title.empty() )
		{
			// update the UI widget
			LLTextBox* media_title_field = getChild<LLTextBox>("media_info");
			if ( media_title_field )
			{
				media_title_field->setText( media_title );

				// stop looking for a title when we get one
				// FIXME: check this is the right approach
				mNeedMediaTitle = false;
			};
		};
	};
}

//////////////////////////////////////////////////////////////////////////////
//
void LLFloaterTools::updateMediaSettings()
{
    bool identical( false );
    std::string base_key( "" );
    std::string value_str( "" );
    int value_int = 0;
    bool value_bool = false;
	LLObjectSelectionHandle selected_objects =LLSelectMgr::getInstance()->getSelection();
    // TODO: (CP) refactor this using something clever or boost or both !!

    const LLMediaEntry default_media_data;

    // controls 
    U8 value_u8 = default_media_data.getControls();
    struct functor_getter_controls : public LLSelectedTEGetFunctor< U8 >
    {
		functor_getter_controls(const LLMediaEntry &entry) : mMediaEntry(entry) {}
		
        U8 get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return object->getTE(face)->getMediaData()->getControls();
            return mMediaEntry.getControls();
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_controls(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_controls, value_u8 );
    base_key = std::string( LLMediaEntry::CONTROLS_KEY );
    mMediaSettings[ base_key ] = value_u8;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // First click (formerly left click)
    value_bool = default_media_data.getFirstClickInteract();
    struct functor_getter_first_click : public LLSelectedTEGetFunctor< bool >
    {
		functor_getter_first_click(const LLMediaEntry& entry): mMediaEntry(entry) {}		
		
        bool get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return object->getTE(face)->getMediaData()->getFirstClickInteract();
            return mMediaEntry.getFirstClickInteract();
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_first_click(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_first_click, value_bool );
    base_key = std::string( LLMediaEntry::FIRST_CLICK_INTERACT_KEY );
    mMediaSettings[ base_key ] = value_bool;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // Home URL
    value_str = default_media_data.getHomeURL();
    struct functor_getter_home_url : public LLSelectedTEGetFunctor< std::string >
    {
		functor_getter_home_url(const LLMediaEntry& entry): mMediaEntry(entry) {}		
		
        std::string get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return object->getTE(face)->getMediaData()->getHomeURL();
            return mMediaEntry.getHomeURL();
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_home_url(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_home_url, value_str );
    base_key = std::string( LLMediaEntry::HOME_URL_KEY );
    mMediaSettings[ base_key ] = value_str;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // Current URL
    value_str = default_media_data.getCurrentURL();
    struct functor_getter_current_url : public LLSelectedTEGetFunctor< std::string >
    {
		functor_getter_current_url(const LLMediaEntry& entry): mMediaEntry(entry) {}
        
		std::string get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return object->getTE(face)->getMediaData()->getCurrentURL();
            return mMediaEntry.getCurrentURL();
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_current_url(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_current_url, value_str );
    base_key = std::string( LLMediaEntry::CURRENT_URL_KEY );
    mMediaSettings[ base_key ] = value_str;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // Auto zoom
    value_bool = default_media_data.getAutoZoom();
    struct functor_getter_auto_zoom : public LLSelectedTEGetFunctor< bool >
    {
		
		functor_getter_auto_zoom(const LLMediaEntry& entry)	: mMediaEntry(entry) {}	
		
        bool get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return object->getTE(face)->getMediaData()->getAutoZoom();
            return mMediaEntry.getAutoZoom();
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_auto_zoom(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_auto_zoom, value_bool );
    base_key = std::string( LLMediaEntry::AUTO_ZOOM_KEY );
    mMediaSettings[ base_key ] = value_bool;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // Auto play
    //value_bool = default_media_data.getAutoPlay();
	// set default to auto play TRUE -- angela  EXT-5172
	value_bool = true;
    struct functor_getter_auto_play : public LLSelectedTEGetFunctor< bool >
    {
		functor_getter_auto_play(const LLMediaEntry& entry)	: mMediaEntry(entry) {}	
			
        bool get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return object->getTE(face)->getMediaData()->getAutoPlay();
            //return mMediaEntry.getAutoPlay(); set default to auto play TRUE -- angela  EXT-5172
			return true;
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_auto_play(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_auto_play, value_bool );
    base_key = std::string( LLMediaEntry::AUTO_PLAY_KEY );
    mMediaSettings[ base_key ] = value_bool;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
	
    // Auto scale
	// set default to auto scale TRUE -- angela  EXT-5172
    //value_bool = default_media_data.getAutoScale();
	value_bool = true;
    struct functor_getter_auto_scale : public LLSelectedTEGetFunctor< bool >
    {
		functor_getter_auto_scale(const LLMediaEntry& entry): mMediaEntry(entry) {}	

        bool get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return object->getTE(face)->getMediaData()->getAutoScale();
           // return mMediaEntry.getAutoScale();  set default to auto scale TRUE -- angela  EXT-5172
			return true;
		};
		
		const LLMediaEntry &mMediaEntry;
		
    } func_auto_scale(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_auto_scale, value_bool );
    base_key = std::string( LLMediaEntry::AUTO_SCALE_KEY );
    mMediaSettings[ base_key ] = value_bool;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // Auto loop
    value_bool = default_media_data.getAutoLoop();
    struct functor_getter_auto_loop : public LLSelectedTEGetFunctor< bool >
    {
		functor_getter_auto_loop(const LLMediaEntry& entry)	: mMediaEntry(entry) {}	

        bool get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return object->getTE(face)->getMediaData()->getAutoLoop();
            return mMediaEntry.getAutoLoop();
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_auto_loop(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_auto_loop, value_bool );
    base_key = std::string( LLMediaEntry::AUTO_LOOP_KEY );
    mMediaSettings[ base_key ] = value_bool;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // width pixels (if not auto scaled)
    value_int = default_media_data.getWidthPixels();
    struct functor_getter_width_pixels : public LLSelectedTEGetFunctor< int >
    {
		functor_getter_width_pixels(const LLMediaEntry& entry): mMediaEntry(entry) {}		

        int get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return object->getTE(face)->getMediaData()->getWidthPixels();
            return mMediaEntry.getWidthPixels();
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_width_pixels(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_width_pixels, value_int );
    base_key = std::string( LLMediaEntry::WIDTH_PIXELS_KEY );
    mMediaSettings[ base_key ] = value_int;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // height pixels (if not auto scaled)
    value_int = default_media_data.getHeightPixels();
    struct functor_getter_height_pixels : public LLSelectedTEGetFunctor< int >
    {
		functor_getter_height_pixels(const LLMediaEntry& entry)	: mMediaEntry(entry) {}
        
		int get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return object->getTE(face)->getMediaData()->getHeightPixels();
            return mMediaEntry.getHeightPixels();
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_height_pixels(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_height_pixels, value_int );
    base_key = std::string( LLMediaEntry::HEIGHT_PIXELS_KEY );
    mMediaSettings[ base_key ] = value_int;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // Enable Alt image
    value_bool = default_media_data.getAltImageEnable();
    struct functor_getter_enable_alt_image : public LLSelectedTEGetFunctor< bool >
    {
		functor_getter_enable_alt_image(const LLMediaEntry& entry): mMediaEntry(entry) {}
        
		bool get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return object->getTE(face)->getMediaData()->getAltImageEnable();
            return mMediaEntry.getAltImageEnable();
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_enable_alt_image(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_enable_alt_image, value_bool );
    base_key = std::string( LLMediaEntry::ALT_IMAGE_ENABLE_KEY );
    mMediaSettings[ base_key ] = value_bool;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // Perms - owner interact
    value_bool = 0 != ( default_media_data.getPermsInteract() & LLMediaEntry::PERM_OWNER );
    struct functor_getter_perms_owner_interact : public LLSelectedTEGetFunctor< bool >
    {
		functor_getter_perms_owner_interact(const LLMediaEntry& entry): mMediaEntry(entry) {}
        
		bool get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return (0 != (object->getTE(face)->getMediaData()->getPermsInteract() & LLMediaEntry::PERM_OWNER));
            return 0 != ( mMediaEntry.getPermsInteract() & LLMediaEntry::PERM_OWNER );
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_perms_owner_interact(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_perms_owner_interact, value_bool );
    base_key = std::string( LLPanelContents::PERMS_OWNER_INTERACT_KEY );
    mMediaSettings[ base_key ] = value_bool;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // Perms - owner control
    value_bool = 0 != ( default_media_data.getPermsControl() & LLMediaEntry::PERM_OWNER );
    struct functor_getter_perms_owner_control : public LLSelectedTEGetFunctor< bool >
    {
		functor_getter_perms_owner_control(const LLMediaEntry& entry)	: mMediaEntry(entry) {}
        
        bool get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return (0 != (object->getTE(face)->getMediaData()->getPermsControl() & LLMediaEntry::PERM_OWNER));
            return 0 != ( mMediaEntry.getPermsControl() & LLMediaEntry::PERM_OWNER );
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_perms_owner_control(default_media_data);
    identical = selected_objects ->getSelectedTEValue( &func_perms_owner_control, value_bool );
    base_key = std::string( LLPanelContents::PERMS_OWNER_CONTROL_KEY );
    mMediaSettings[ base_key ] = value_bool;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // Perms - group interact
    value_bool = 0 != ( default_media_data.getPermsInteract() & LLMediaEntry::PERM_GROUP );
    struct functor_getter_perms_group_interact : public LLSelectedTEGetFunctor< bool >
    {
		functor_getter_perms_group_interact(const LLMediaEntry& entry): mMediaEntry(entry) {}
        
        bool get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return (0 != (object->getTE(face)->getMediaData()->getPermsInteract() & LLMediaEntry::PERM_GROUP));
            return 0 != ( mMediaEntry.getPermsInteract() & LLMediaEntry::PERM_GROUP );
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_perms_group_interact(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_perms_group_interact, value_bool );
    base_key = std::string( LLPanelContents::PERMS_GROUP_INTERACT_KEY );
    mMediaSettings[ base_key ] = value_bool;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // Perms - group control
    value_bool = 0 != ( default_media_data.getPermsControl() & LLMediaEntry::PERM_GROUP );
    struct functor_getter_perms_group_control : public LLSelectedTEGetFunctor< bool >
    {
		functor_getter_perms_group_control(const LLMediaEntry& entry): mMediaEntry(entry) {}
        
        bool get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return (0 != (object->getTE(face)->getMediaData()->getPermsControl() & LLMediaEntry::PERM_GROUP));
            return 0 != ( mMediaEntry.getPermsControl() & LLMediaEntry::PERM_GROUP );
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_perms_group_control(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_perms_group_control, value_bool );
    base_key = std::string( LLPanelContents::PERMS_GROUP_CONTROL_KEY );
    mMediaSettings[ base_key ] = value_bool;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // Perms - anyone interact
    value_bool = 0 != ( default_media_data.getPermsInteract() & LLMediaEntry::PERM_ANYONE );
    struct functor_getter_perms_anyone_interact : public LLSelectedTEGetFunctor< bool >
    {
		functor_getter_perms_anyone_interact(const LLMediaEntry& entry): mMediaEntry(entry) {}
        
        bool get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return (0 != (object->getTE(face)->getMediaData()->getPermsInteract() & LLMediaEntry::PERM_ANYONE));
            return 0 != ( mMediaEntry.getPermsInteract() & LLMediaEntry::PERM_ANYONE );
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_perms_anyone_interact(default_media_data);
    identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func_perms_anyone_interact, value_bool );
    base_key = std::string( LLPanelContents::PERMS_ANYONE_INTERACT_KEY );
    mMediaSettings[ base_key ] = value_bool;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // Perms - anyone control
    value_bool = 0 != ( default_media_data.getPermsControl() & LLMediaEntry::PERM_ANYONE );
    struct functor_getter_perms_anyone_control : public LLSelectedTEGetFunctor< bool >
    {
		functor_getter_perms_anyone_control(const LLMediaEntry& entry)	: mMediaEntry(entry) {}
        
        bool get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return (0 != (object->getTE(face)->getMediaData()->getPermsControl() & LLMediaEntry::PERM_ANYONE));
            return 0 != ( mMediaEntry.getPermsControl() & LLMediaEntry::PERM_ANYONE );
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_perms_anyone_control(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_perms_anyone_control, value_bool );
    base_key = std::string( LLPanelContents::PERMS_ANYONE_CONTROL_KEY );
    mMediaSettings[ base_key ] = value_bool;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // security - whitelist enable
    value_bool = default_media_data.getWhiteListEnable();
    struct functor_getter_whitelist_enable : public LLSelectedTEGetFunctor< bool >
    {
		functor_getter_whitelist_enable(const LLMediaEntry& entry)	: mMediaEntry(entry) {}
        
        bool get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return object->getTE(face)->getMediaData()->getWhiteListEnable();
            return mMediaEntry.getWhiteListEnable();
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_whitelist_enable(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_whitelist_enable, value_bool );
    base_key = std::string( LLMediaEntry::WHITELIST_ENABLE_KEY );
    mMediaSettings[ base_key ] = value_bool;
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
	
    // security - whitelist URLs
    std::vector<std::string> value_vector_str = default_media_data.getWhiteList();
    struct functor_getter_whitelist_urls : public LLSelectedTEGetFunctor< std::vector<std::string> >
    {
		functor_getter_whitelist_urls(const LLMediaEntry& entry): mMediaEntry(entry) {}
        
        std::vector<std::string> get( LLViewerObject* object, S32 face )
        {
            if ( object )
                if ( object->getTE(face) )
                    if ( object->getTE(face)->getMediaData() )
                        return object->getTE(face)->getMediaData()->getWhiteList();
            return mMediaEntry.getWhiteList();
        };
		
		const LLMediaEntry &mMediaEntry;
		
    } func_whitelist_urls(default_media_data);
    identical = selected_objects->getSelectedTEValue( &func_whitelist_urls, value_vector_str );
    base_key = std::string( LLMediaEntry::WHITELIST_KEY );
	mMediaSettings[ base_key ].clear();
    std::vector< std::string >::iterator iter = value_vector_str.begin();
    while( iter != value_vector_str.end() )
    {
        std::string white_list_url = *iter;
        mMediaSettings[ base_key ].append( white_list_url );
        ++iter;
    };
	
    mMediaSettings[ base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX ) ] = ! identical;
}

