/** 
 * @file llfloatertools.cpp
 * @brief The edit tools, including move, position, land, etc.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloatertools.h"

#include "llfontgl.h"
#include "llcoord.h"
#include "llgl.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldraghandle.h"
#include "llfloaterbuildoptions.h"
#include "llfloateropenobject.h"
#include "llfocusmgr.h"
#include "llmenugl.h"
#include "llpanelcontents.h"
#include "llpanelface.h"
#include "llpanelland.h"
#include "llpanelinventory.h"
#include "llpanelobject.h"
#include "llpanelvolume.h"
#include "llpanelpermissions.h"
#include "llselectmgr.h"
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
#include "lltoolselect.h"
#include "lltoolselectland.h"
#include "llui.h"
#include "llviewermenu.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "viewer.h"

#include "llvieweruictrlfactory.h"

// Globals
LLFloaterTools *gFloaterTools = NULL;


const LLString PANEL_NAMES[LLFloaterTools::PANEL_COUNT] =
{
	LLString("General"), 	// PANEL_GENERAL,
	LLString("Object"), 	// PANEL_OBJECT,
	LLString("Features"),	// PANEL_FEATURES,
	LLString("Texture"),	// PANEL_FACE,
	LLString("Content"),	// PANEL_CONTENTS,
};

// Local prototypes
void commit_select_tool(LLUICtrl *ctrl, void *data);
void commit_select_component(LLUICtrl *ctrl, void *data);
void click_show_more(void*);
void click_popup_info(void*);
void click_popup_done(void*);
void click_popup_minimize(void*);
void click_popup_grab_drag(LLUICtrl *, void*);
void click_popup_grab_lift(LLUICtrl *, void*);
void click_popup_grab_spin(LLUICtrl *, void*);
void click_popup_rotate_left(void*);
void click_popup_rotate_reset(void*);
void click_popup_rotate_right(void*);
void click_popup_dozer_mode(LLUICtrl *, void *user);
void click_popup_dozer_size(LLUICtrl *, void *user);
void click_dozer_size(LLUICtrl *, void*);
void click_apply_to_selection(void*);
void commit_radio_zoom(LLUICtrl *, void*);
void commit_radio_orbit(LLUICtrl *, void*);
void commit_radio_pan(LLUICtrl *, void*);
void commit_grid_mode(LLUICtrl *, void*);
void commit_slider_zoom(LLUICtrl *, void*);


//static
void*	LLFloaterTools::createPanelPermissions(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelPermissions = new LLPanelPermissions("General");
	return floater->mPanelPermissions;
}
//static
void*	LLFloaterTools::createPanelObject(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelObject = new LLPanelObject("Object");
	return floater->mPanelObject;
}

//static
void*	LLFloaterTools::createPanelVolume(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelVolume = new LLPanelVolume("Features");
	return floater->mPanelVolume;
}

//static
void*	LLFloaterTools::createPanelFace(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelFace = new LLPanelFace("Texture");
	return floater->mPanelFace;
}

//static
void*	LLFloaterTools::createPanelContents(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelContents = new LLPanelContents("Contents");
	return floater->mPanelContents;
}

//static
void*	LLFloaterTools::createPanelContentsInventory(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelContents->mPanelInventory = new LLPanelInventory("ContentsInventory", LLRect());
	return floater->mPanelContents->mPanelInventory;
}

//static
void*	LLFloaterTools::createPanelLandInfo(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelLandInfo = new LLPanelLandInfo("land info panel");
	return floater->mPanelLandInfo;
}

BOOL	LLFloaterTools::postBuild()
{
	
	// Hide until tool selected
	setVisible(FALSE);

	// Since we constantly show and hide this during drags, don't
	// make sounds on visibility changes.
	setSoundFlags(LLView::SILENT);

	mDragHandle->setEnabled( !gSavedSettings.getBOOL("ToolboxAutoMove") );

	LLRect rect;
	mBtnFocus = LLUICtrlFactory::getButtonByName(this,"button focus");//btn;
	childSetAction("button focus",LLFloaterTools::setEditTool, (void*)gToolCamera);
	mBtnMove = LLUICtrlFactory::getButtonByName(this,"button move");
	childSetAction("button move",LLFloaterTools::setEditTool, (void*)gToolGrab);
	mBtnEdit = LLUICtrlFactory::getButtonByName(this,"button edit");
	childSetAction("button edit",LLFloaterTools::setEditTool, (void*)gToolTranslate);
	mBtnCreate = LLUICtrlFactory::getButtonByName(this,"button create");
	childSetAction("button create",LLFloaterTools::setEditTool, (void*)gToolCreate);
	mBtnLand = LLUICtrlFactory::getButtonByName(this, "button land" );
	childSetAction("button land",LLFloaterTools::setEditTool, (void*)gToolParcel);
	mTextStatus = LLUICtrlFactory::getTextBoxByName(this,"text status");
	mRadioZoom = LLUICtrlFactory::getCheckBoxByName(this,"radio zoom");
	childSetCommitCallback("slider zoom",commit_slider_zoom,this);
	mRadioOrbit = LLUICtrlFactory::getCheckBoxByName(this,"radio orbit");
	childSetCommitCallback("radio orbit",commit_radio_orbit,this);
	mRadioPan = LLUICtrlFactory::getCheckBoxByName(this,"radio pan");
	childSetCommitCallback("radio pan",commit_radio_pan,this);
	mRadioMove = LLUICtrlFactory::getCheckBoxByName(this,"radio move");
	childSetCommitCallback("radio move",click_popup_grab_drag,this);
	mRadioLift = LLUICtrlFactory::getCheckBoxByName(this,"radio lift");
	childSetCommitCallback("radio lift",click_popup_grab_lift,this);
	mRadioSpin = LLUICtrlFactory::getCheckBoxByName(this,"radio spin");
	childSetCommitCallback("radio spin",click_popup_grab_spin,NULL);
	mRadioPosition = LLUICtrlFactory::getCheckBoxByName(this,"radio position");
	childSetCommitCallback("radio position",commit_select_tool,gToolTranslate);
	mRadioRotate = LLUICtrlFactory::getCheckBoxByName(this,"radio rotate");
	childSetCommitCallback("radio rotate",commit_select_tool,gToolRotate);
	mRadioStretch = LLUICtrlFactory::getCheckBoxByName(this,"radio stretch");
	childSetCommitCallback("radio stretch",commit_select_tool,gToolStretch);
	mRadioSelectFace = LLUICtrlFactory::getCheckBoxByName(this,"radio select face");
	childSetCommitCallback("radio select face",commit_select_tool,gToolFace);
	mCheckSelectIndividual = LLUICtrlFactory::getCheckBoxByName(this,"checkbox edit linked parts");
	childSetValue("checkbox edit linked parts",(BOOL)gSavedSettings.getBOOL("EditLinkedParts"));
	childSetCommitCallback("checkbox edit linked parts",commit_select_component,this);
	mCheckSnapToGrid = LLUICtrlFactory::getCheckBoxByName(this,"checkbox snap to grid");
	childSetValue("checkbox snap to grid",(BOOL)gSavedSettings.getBOOL("SnapEnabled"));
	mBtnGridOptions = LLUICtrlFactory::getButtonByName(this,"Options...");
	childSetAction("Options...",onClickGridOptions, this);
	mCheckStretchUniform = LLUICtrlFactory::getCheckBoxByName(this,"checkbox uniform");
	childSetValue("checkbox uniform",(BOOL)gSavedSettings.getBOOL("ScaleUniform"));
	mCheckStretchTexture = LLUICtrlFactory::getCheckBoxByName(this,"checkbox stretch textures");
	childSetValue("checkbox stretch textures",(BOOL)gSavedSettings.getBOOL("ScaleStretchTextures"));
	mTextGridMode = LLUICtrlFactory::getTextBoxByName(this,"text ruler mode");
	mComboGridMode = LLUICtrlFactory::getComboBoxByName(this,"combobox grid mode");
	childSetCommitCallback("combobox grid mode",commit_grid_mode, this);
	//
	// Create Buttons
	//

	static	const LLString	toolNames[]={
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
	void*	toolData[]={
			&LLToolPlacerPanel::sCube,
			&LLToolPlacerPanel::sPrism,
			&LLToolPlacerPanel::sPyramid,
			&LLToolPlacerPanel::sTetrahedron,
			&LLToolPlacerPanel::sCylinder,
			&LLToolPlacerPanel::sCylinderHemi,
			&LLToolPlacerPanel::sCone,
			&LLToolPlacerPanel::sConeHemi,
			&LLToolPlacerPanel::sSphere,
			&LLToolPlacerPanel::sSphereHemi,
			&LLToolPlacerPanel::sTorus,
			&LLToolPlacerPanel::sSquareTorus,
			&LLToolPlacerPanel::sTriangleTorus,
			&LLToolPlacerPanel::sTree,
			&LLToolPlacerPanel::sGrass};
	for(size_t t=0; t<sizeof(toolNames)/sizeof(toolNames[0]); ++t)
	{
		LLButton *found = LLViewerUICtrlFactory::getButtonByName(this,toolNames[t]);
		if(found)
		{
			found->setClickedCallback(setObjectType,toolData[t]);
			mButtons.push_back( found );
		}else{
			llwarns << "Tool button not found! DOA Pending." << llendl;
		}
	}
	mCheckCopySelection = LLUICtrlFactory::getCheckBoxByName(this,"checkbox copy selection");
	childSetValue("checkbox copy selection",(BOOL)gSavedSettings.getBOOL("CreateToolCopySelection"));
	mCheckSticky = LLUICtrlFactory::getCheckBoxByName(this,"checkbox sticky");
	childSetValue("checkbox sticky",(BOOL)gSavedSettings.getBOOL("CreateToolKeepSelected"));
	mCheckCopyCenters = LLUICtrlFactory::getCheckBoxByName(this,"checkbox copy centers");
	childSetValue("checkbox copy centers",(BOOL)gSavedSettings.getBOOL("CreateToolCopyCenters"));
	mCheckCopyRotates = LLUICtrlFactory::getCheckBoxByName(this,"checkbox copy rotates");
	childSetValue("checkbox copy rotates",(BOOL)gSavedSettings.getBOOL("CreateToolCopyRotates"));
	mRadioSelectLand = LLUICtrlFactory::getCheckBoxByName(this,"radio select land");
	childSetCommitCallback("radio select land",commit_select_tool, gToolParcel);
	mRadioDozerFlatten = LLUICtrlFactory::getCheckBoxByName(this,"radio flatten");
	childSetCommitCallback("radio flatten",click_popup_dozer_mode,  (void*)0);
	mRadioDozerRaise = LLUICtrlFactory::getCheckBoxByName(this,"radio raise");
	childSetCommitCallback("radio raise",click_popup_dozer_mode,  (void*)1);
	mRadioDozerLower = LLUICtrlFactory::getCheckBoxByName(this,"radio lower");
	childSetCommitCallback("radio lower",click_popup_dozer_mode,  (void*)2);
	mRadioDozerSmooth = LLUICtrlFactory::getCheckBoxByName(this,"radio smooth");
	childSetCommitCallback("radio smooth",click_popup_dozer_mode,  (void*)3);
	mRadioDozerNoise = LLUICtrlFactory::getCheckBoxByName(this,"radio noise");
	childSetCommitCallback("radio noise",click_popup_dozer_mode,  (void*)4);
	mRadioDozerRevert = LLUICtrlFactory::getCheckBoxByName(this,"radio revert");
	childSetCommitCallback("radio revert",click_popup_dozer_mode,  (void*)5);
	mComboDozerSize = LLUICtrlFactory::getComboBoxByName(this,"combobox brush size");
	childSetCommitCallback("combobox brush size",click_dozer_size,  (void*)0);
	if(mComboDozerSize) mComboDozerSize->setCurrentByIndex(0);
	mBtnApplyToSelection = LLUICtrlFactory::getButtonByName(this,"button apply to selection");
	childSetAction("button apply to selection",click_apply_to_selection,  (void*)0);
	mCheckShowOwners = LLUICtrlFactory::getCheckBoxByName(this,"checkbox show owners");
	childSetValue("checkbox show owners",gSavedSettings.getBOOL("ShowParcelOwners"));
	childSetAction("button more", click_show_more, this);
	childSetAction("button less", click_show_more, this);
	mTab = LLUICtrlFactory::getTabContainerByName(this,"Object Info Tabs");
	if(mTab)
	{
		mTab->setVisible( gSavedSettings.getBOOL("ToolboxShowMore") );
		mTab->setFollows(FOLLOWS_TOP | FOLLOWS_LEFT);
		mTab->setVisible( gSavedSettings.getBOOL("ToolboxShowMore") );
		mTab->setBorderVisible(FALSE);
		mTab->selectFirstTab();
	}
	return TRUE;
}

// Create the popupview with a dummy center.  It will be moved into place
// during LLViewerWindow's per-frame hover processing.
LLFloaterTools::LLFloaterTools()
:	LLFloater("toolbox floater"),
	mBtnFocus(NULL),
	mBtnMove(NULL),
	mBtnEdit(NULL),
	mBtnCreate(NULL),
	mBtnLand(NULL),
	mTextStatus(NULL),

	mRadioOrbit(NULL),
	mRadioZoom(NULL),
	mRadioPan(NULL),

	mRadioMove(NULL),
	mRadioLift(NULL),
	mRadioSpin(NULL),

	mRadioPosition(NULL),
	mRadioRotate(NULL),
	mRadioStretch(NULL),
	mRadioSelectFace(NULL),
	mCheckSelectIndividual(NULL),

	mCheckSnapToGrid(NULL),
	mBtnGridOptions(NULL),
	mTextGridMode(NULL),
	mComboGridMode(NULL),
	mCheckStretchUniform(NULL),
	mCheckStretchTexture(NULL),

	mBtnRotateLeft(NULL),
	mBtnRotateReset(NULL),
	mBtnRotateRight(NULL),

	mBtnDelete(NULL),
	mBtnDuplicate(NULL),
	mBtnDuplicateInPlace(NULL),

	mCheckSticky(NULL),
	mCheckCopySelection(NULL),
	mCheckCopyCenters(NULL),
	mCheckCopyRotates(NULL),
	mRadioSelectLand(NULL),
	mRadioDozerFlatten(NULL),
	mRadioDozerRaise(NULL),
	mRadioDozerLower(NULL),
	mRadioDozerSmooth(NULL),
	mRadioDozerNoise(NULL),
	mRadioDozerRevert(NULL),
	mComboDozerSize(NULL),
	mBtnApplyToSelection(NULL),
	mCheckShowOwners(NULL),


	mTab(NULL),
	mPanelPermissions(NULL),
	mPanelObject(NULL),
	mPanelVolume(NULL),
	mPanelContents(NULL),
	mPanelFace(NULL),
	mPanelLandInfo(NULL),

	mTabLand(NULL),
	mDirty(TRUE)
{
	mAutoFocus = FALSE;
	LLCallbackMap::map_t factory_map;
	factory_map["General"] = LLCallbackMap(createPanelPermissions, this);//LLPanelPermissions
	factory_map["Object"] = LLCallbackMap(createPanelObject, this);//LLPanelObject
	factory_map["Features"] = LLCallbackMap(createPanelVolume, this);//LLPanelVolume
	factory_map["Texture"] = LLCallbackMap(createPanelFace, this);//LLPanelFace
	factory_map["Contents"] = LLCallbackMap(createPanelContents, this);//LLPanelContents
	factory_map["ContentsInventory"] = LLCallbackMap(createPanelContentsInventory, this);//LLPanelContents
	factory_map["land info panel"] = LLCallbackMap(createPanelLandInfo, this);//LLPanelLandInfo

	gUICtrlFactory->buildFloater(this,"floater_tools.xml",&factory_map,FALSE);

	mLargeHeight = getRect().getHeight();
	mSmallHeight = mLargeHeight;
	if (mTab) mSmallHeight -= mTab->getRect().getHeight();
	
	// force a toggle initially. seems to be needed to correctly initialize 
	// both "more" and "less" cases. it also seems to be important to begin
	// with the user's preference first so that it's initial position will
	// be correct (SL-51192) -MG
	BOOL show_more = gSavedSettings.getBOOL("ToolboxShowMore"); // get user's preference
	gSavedSettings.setBOOL("ToolboxShowMore", show_more); // sets up forced toggle below
	showMore( !show_more ); // does the toggle
	showMore(  show_more ); // reset the real user's preference
}

LLFloaterTools::~LLFloaterTools()
{
	// children automatically deleted
}

void LLFloaterTools::setStatusText(const LLString& text)
{
	mTextStatus->setText(text);
}

void LLFloaterTools::refresh()
{
	const S32 INFO_WIDTH = mRect.getWidth();
	const S32 INFO_HEIGHT = 384;
	LLRect object_info_rect(0, 0, INFO_WIDTH, -INFO_HEIGHT);
	BOOL all_volume = gSelectMgr->selectionAllPCode( LL_PCODE_VOLUME );

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

	mPanelPermissions->refresh();
	mPanelObject->refresh();
	mPanelVolume->refresh();
	mPanelFace->refresh();
	mPanelContents->refresh();
	mPanelLandInfo->refresh();
}

void LLFloaterTools::draw()
{
	if (mDirty)
	{
		refresh();
		mDirty = FALSE;
	}

	//	mCheckSelectIndividual->set(gSavedSettings.getBOOL("EditLinkedParts"));
	LLFloater::draw();
}

void LLFloaterTools::dirty()
{
	mDirty = TRUE; 
	LLFloaterOpenObject::dirty();
}

// Clean up any tool state that should not persist when the
// floater is closed.
void LLFloaterTools::resetToolState()
{
	gCameraBtnOrbit = FALSE;
	gCameraBtnPan = FALSE;

	gGrabBtnSpin = FALSE;
	gGrabBtnVertical = FALSE;
}

void LLFloaterTools::updatePopup(LLCoordGL center, MASK mask)
{
	LLTool *tool = gToolMgr->getCurrentTool();

	// HACK to allow seeing the buttons when you have the app in a window.
	// Keep the visibility the same as it 
	if (tool == gToolNull)
	{
		return;
	}
	
	// Focus buttons
	BOOL focus_visible = (	tool == gToolCamera );

	mBtnFocus	->setToggleState( focus_visible );

	mRadioZoom	->setVisible( focus_visible );
	mRadioOrbit	->setVisible( focus_visible );
	mRadioPan	->setVisible( focus_visible );
	childSetVisible("slider zoom", focus_visible);
	
	mRadioZoom	->set(	!gCameraBtnOrbit &&
						!gCameraBtnPan &&
						!(mask == MASK_ORBIT) &&
						!(mask == (MASK_ORBIT | MASK_ALT)) &&
						!(mask == MASK_PAN) &&
						!(mask == (MASK_PAN | MASK_ALT)) );

	mRadioOrbit	->set(	gCameraBtnOrbit || 
						(mask == MASK_ORBIT) ||
						(mask == (MASK_ORBIT | MASK_ALT)) );

	mRadioPan	->set(	gCameraBtnPan ||
						(mask == MASK_PAN) ||
						(mask == (MASK_PAN | MASK_ALT)) );

	// multiply by correction factor because volume sliders go [0, 0.5]
	childSetValue( "slider zoom", gAgent.getCameraZoomFraction() * 0.5f);

	// Move buttons
	BOOL move_visible = (tool == gToolGrab);

	if (mBtnMove) mBtnMove	->setToggleState( move_visible );

	// HACK - highlight buttons for next click
	if (mRadioMove)
	{
		mRadioMove	->setVisible( move_visible );
		mRadioMove	->set(	!gGrabBtnSpin && 
							!gGrabBtnVertical &&
							!(mask == MASK_VERTICAL) && 
							!(mask == MASK_SPIN) );
	}

	if (mRadioLift)
	{
		mRadioLift	->setVisible( move_visible );
		mRadioLift	->set(	gGrabBtnVertical || 
							(mask == MASK_VERTICAL) );
	}

	if (mRadioSpin)
	{
		mRadioSpin	->setVisible( move_visible );
		mRadioSpin	->set(	gGrabBtnSpin || 
							(mask == MASK_SPIN) );
	}

	// Edit buttons
	BOOL edit_visible = tool == gToolTranslate ||
						tool == gToolRotate ||
						tool == gToolStretch ||
						tool == gToolFace ||
						tool == gToolIndividual ||
						tool == gToolPipette;

	mBtnEdit	->setToggleState( edit_visible );

	mRadioPosition	->setVisible( edit_visible );
	mRadioRotate	->setVisible( edit_visible );
	mRadioStretch	->setVisible( edit_visible );
	if (mRadioSelectFace)
	{
		mRadioSelectFace->setVisible( edit_visible );
		mRadioSelectFace->set( tool == gToolFace );
	}

	if (mCheckSelectIndividual)
	{
		mCheckSelectIndividual->setVisible(edit_visible);
		//mCheckSelectIndividual->set(gSavedSettings.getBOOL("EditLinkedParts"));
	}

	mRadioPosition	->set( tool == gToolTranslate );
	mRadioRotate	->set( tool == gToolRotate );
	mRadioStretch	->set( tool == gToolStretch );

	if (mComboGridMode) 
	{
		mComboGridMode		->setVisible( edit_visible );
		S32 index = mComboGridMode->getCurrentIndex();
		mComboGridMode->removeall();

		switch (mObjectSelection->getSelectType())
		{
		case SELECT_TYPE_HUD:
			mComboGridMode->add("Screen");
			mComboGridMode->add("Local");
			//mComboGridMode->add("Reference");
			break;
		case SELECT_TYPE_WORLD:
			mComboGridMode->add("World");
			mComboGridMode->add("Local");
			mComboGridMode->add("Reference");
			break;
		case SELECT_TYPE_ATTACHMENT:
			mComboGridMode->add("Attachment");
			mComboGridMode->add("Local");
			mComboGridMode->add("Reference");
			break;
		}

		mComboGridMode->setCurrentByIndex(index);
	}
	if (mTextGridMode) mTextGridMode->setVisible( edit_visible );

	// Snap to grid disabled for grab tool - very confusing
	if (mCheckSnapToGrid) mCheckSnapToGrid->setVisible( edit_visible /* || tool == gToolGrab */ );
	if (mBtnGridOptions) mBtnGridOptions->setVisible( edit_visible /* || tool == gToolGrab */ );

	//mCheckSelectLinked	->setVisible( edit_visible );
	if (mCheckStretchUniform) mCheckStretchUniform->setVisible( edit_visible );
	if (mCheckStretchTexture) mCheckStretchTexture->setVisible( edit_visible );

	// Create buttons
	BOOL create_visible = (tool == gToolCreate);

	mBtnCreate	->setToggleState(	tool == gToolCreate );

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
		for( std::vector<LLButton*>::size_type i = 0; i < mButtons.size(); i++ )
		{
			LLPCode pcode = LLToolPlacer::getObjectType();
			void *userdata = mButtons[i]->getCallbackUserData();
			LLPCode *cur = (LLPCode*) userdata;

			BOOL state = (pcode == *cur);
			mButtons[i]->setToggleState( state );
			mButtons[i]->setVisible( create_visible );
		}
	}

	if (mCheckSticky) mCheckSticky		->setVisible( create_visible );
	if (mCheckCopySelection) mCheckCopySelection	->setVisible( create_visible );
	if (mCheckCopyCenters) mCheckCopyCenters	->setVisible( create_visible );
	if (mCheckCopyRotates) mCheckCopyRotates	->setVisible( create_visible );

	if (mCheckCopyCenters) mCheckCopyCenters->setEnabled( mCheckCopySelection->get() );
	if (mCheckCopyRotates) mCheckCopyRotates->setEnabled( mCheckCopySelection->get() );

	// Land buttons
	BOOL land_visible = (tool == gToolLand || tool == gToolParcel );

	if (mBtnLand)	mBtnLand	->setToggleState( land_visible );

	//	mRadioEditLand	->set( tool == gToolLand );
	if (mRadioSelectLand)	mRadioSelectLand->set( tool == gToolParcel );

	//	mRadioEditLand	->setVisible( land_visible );
	if (mRadioSelectLand)	mRadioSelectLand->setVisible( land_visible );

	S32 dozer_mode = gSavedSettings.getS32("RadioLandBrushAction");
	S32 dozer_size = gSavedSettings.getS32("RadioLandBrushSize");

	if (mRadioDozerFlatten)
	{
		mRadioDozerFlatten	->set( tool == gToolLand && dozer_mode == 0);
		mRadioDozerFlatten	->setVisible( land_visible );
	}
	if (mRadioDozerRaise)
	{
		mRadioDozerRaise	->set( tool == gToolLand && dozer_mode == 1);
		mRadioDozerRaise	->setVisible( land_visible );
	}
	if (mRadioDozerLower)
	{
		mRadioDozerLower	->set( tool == gToolLand && dozer_mode == 2);
		mRadioDozerLower	->setVisible( land_visible );
	}
	if (mRadioDozerSmooth)
	{
		mRadioDozerSmooth	->set( tool == gToolLand && dozer_mode == 3);
		mRadioDozerSmooth	->setVisible( land_visible );
	}
	if (mRadioDozerNoise)
	{
		mRadioDozerNoise	->set( tool == gToolLand && dozer_mode == 4);
		mRadioDozerNoise	->setVisible( land_visible );
	}
	if (mRadioDozerRevert)
	{
		mRadioDozerRevert	->set( tool == gToolLand && dozer_mode == 5);
		mRadioDozerRevert	->setVisible( land_visible );
	}
	if (mComboDozerSize)
	{
		mComboDozerSize		->setCurrentByIndex(dozer_size);
		mComboDozerSize 	->setVisible( land_visible );
		mComboDozerSize 	->setEnabled( tool == gToolLand );
	}
	if (mBtnApplyToSelection)
	{
		mBtnApplyToSelection->setVisible( land_visible );
		mBtnApplyToSelection->setEnabled( land_visible && !gParcelMgr->selectionEmpty() && tool != gToolParcel);
	}
	if (mCheckShowOwners)
	{
		mCheckShowOwners	->setVisible( land_visible );
	}

	//
	// More panel visibility
	//
	BOOL show_more = gSavedSettings.getBOOL("ToolboxShowMore");

	mTab->setVisible(show_more && tool != gToolLand && tool != gToolParcel);
	mPanelLandInfo->setVisible(show_more && (tool == gToolLand || tool == gToolParcel));
}


// virtual
BOOL LLFloaterTools::canClose()
{
	// don't close when quitting, so camera will stay put
	return !gQuit;
}

// virtual
void LLFloaterTools::onOpen()
{
	mParcelSelection = gParcelMgr->getFloatingParcelSelection();
	mObjectSelection = gSelectMgr->getEditSelection();
}

// virtual
void LLFloaterTools::onClose(bool app_quitting)
{
	setMinimized(FALSE);
	setVisible(FALSE);
	mTab->setVisible(FALSE);

    // Different from handle_reset_view in that it doesn't actually 
	//   move the camera if EditCameraMovement is not set.
	gAgent.resetView(gSavedSettings.getBOOL("EditCameraMovement"));
	
	// exit component selection mode
	gSelectMgr->promoteSelectionToRoot();
	gSavedSettings.setBOOL("EditLinkedParts", FALSE);

	gViewerWindow->showCursor();

	resetToolState();

	mParcelSelection = NULL;
	mObjectSelection = NULL;

	// Switch back to basic toolset
	gToolMgr->setCurrentToolset(gBasicToolset);
	// we were already in basic toolset, using build tools
	// so manually reset tool to default (pie menu tool)
	gToolMgr->getCurrentToolset()->selectFirstTool();
}

void LLFloaterTools::showMore(BOOL show_more)
{
	BOOL showing_more = gSavedSettings.getBOOL("ToolboxShowMore");
	if (show_more == showing_more)
	{
		return;
	}
	
	gSavedSettings.setBOOL("ToolboxShowMore", show_more);

	// Visibility updated next frame - JC
	// mTab->setVisible(show_more);

	if (show_more)
	{
		reshape( mRect.getWidth(), mLargeHeight, TRUE);
		translate( 0, mSmallHeight - mLargeHeight );
	}
	else
	{
		reshape( mRect.getWidth(), mSmallHeight, TRUE);
		translate( 0, mLargeHeight - mSmallHeight );
	}
	childSetVisible("button less",  show_more);
	childSetVisible("button more", !show_more);
}

void LLFloaterTools::showPanel(EInfoPanel panel)
{
	llassert(panel >= 0 && panel < PANEL_COUNT);
	mTab->selectTabByName(PANEL_NAMES[panel]);
}

void click_show_more(void *userdata)
{
	LLFloaterTools *f = (LLFloaterTools *)userdata;
	BOOL show_more = !gSavedSettings.getBOOL("ToolboxShowMore");
	f->showMore( show_more );
}

void click_popup_info(void*)
{
//	gBuildView->setPropertiesPanelOpen(TRUE);
}

void click_popup_done(void*)
{
	handle_reset_view();
}

void click_popup_grab_drag(LLUICtrl*, void*)
{
	gGrabBtnVertical = FALSE;
	gGrabBtnSpin = FALSE;
}

void click_popup_grab_lift(LLUICtrl*, void*)
{
	gGrabBtnVertical = TRUE;
	gGrabBtnSpin = FALSE;
}

void click_popup_grab_spin(LLUICtrl*, void*)
{
	gGrabBtnVertical = FALSE;
	gGrabBtnSpin = TRUE;
}

void commit_radio_zoom(LLUICtrl *, void*)
{
	gCameraBtnOrbit = FALSE;
	gCameraBtnPan = FALSE;
}

void commit_radio_orbit(LLUICtrl *, void*)
{
	gCameraBtnOrbit = TRUE;
	gCameraBtnPan = FALSE;
}

void commit_radio_pan(LLUICtrl *, void*)
{
	gCameraBtnOrbit = FALSE;
	gCameraBtnPan = TRUE;
}

void commit_slider_zoom(LLUICtrl *ctrl, void*)
{
	// renormalize value, since max "volume" level is 0.5 for some reason
	F32 zoom_level = (F32)ctrl->getValue().asReal() * 2.f; // / 0.5f;
	gAgent.setCameraZoomFraction(zoom_level);
}

void click_popup_rotate_left(void*)
{
	gSelectMgr->selectionRotateAroundZ( 45.f );
	dialog_refresh_all();
}

void click_popup_rotate_reset(void*)
{
	gSelectMgr->selectionResetRotation();
	dialog_refresh_all();
}

void click_popup_rotate_right(void*)
{
	gSelectMgr->selectionRotateAroundZ( -45.f );
	dialog_refresh_all();
}


void click_popup_dozer_mode(LLUICtrl *, void *user)
{
	S32 show_owners = gSavedSettings.getBOOL("ShowParcelOwners");
	S32 mode = (S32)(intptr_t) user;
	gFloaterTools->setEditTool( gToolLand );
	gSavedSettings.setS32("RadioLandBrushAction", mode);
	gSavedSettings.setBOOL("ShowParcelOwners", show_owners);
}

void click_popup_dozer_size(LLUICtrl *, void *user)
{
	S32 size = (S32)(intptr_t) user;
	gSavedSettings.setS32("RadioLandBrushSize", size);
}

void click_dozer_size(LLUICtrl *ctrl, void *user)
{
	S32 size = ((LLComboBox*) ctrl)->getCurrentIndex();
	gSavedSettings.setS32("RadioLandBrushSize", size);
}

void click_apply_to_selection(void* user)
{
	gToolLand->modifyLandInSelectionGlobal();
}

void commit_select_tool(LLUICtrl *ctrl, void *data)
{
	S32 show_owners = gSavedSettings.getBOOL("ShowParcelOwners");
	gFloaterTools->setEditTool(data);
	gSavedSettings.setBOOL("ShowParcelOwners", show_owners);
}

void commit_select_component(LLUICtrl *ctrl, void *data)
{
	LLFloaterTools* floaterp = (LLFloaterTools*)data;

	//forfeit focus
	if (gFocusMgr.childHasKeyboardFocus(floaterp))
	{
		gFocusMgr.setKeyboardFocus(NULL, NULL);
	}

	BOOL select_individuals = floaterp->mCheckSelectIndividual->get();
	gSavedSettings.setBOOL("EditLinkedParts", select_individuals);
	floaterp->dirty();

	if (select_individuals)
	{
		gSelectMgr->demoteSelectionToIndividuals();
	}
	else
	{
		gSelectMgr->promoteSelectionToRoot();
	}
}

void commit_grid_mode(LLUICtrl *ctrl, void *data)   
{   
	LLComboBox* combo = (LLComboBox*)ctrl;   
    
	gSelectMgr->setGridMode((EGridMode)combo->getCurrentIndex());
} 

// static 
void LLFloaterTools::setObjectType( void* data )
{
	LLPCode pcode = *(LLPCode*) data;
	LLToolPlacer::setObjectType( pcode );
	gSavedSettings.setBOOL("CreateToolCopySelection", FALSE);
	gViewerWindow->setMouseCapture(NULL);
}

// static
void LLFloaterTools::onClickGridOptions(void* data)
{
	//LLFloaterTools* floaterp = (LLFloaterTools*)data;
	LLFloaterBuildOptions::show(NULL);
	// RN: this makes grid options dependent on build tools window
	//floaterp->addDependentFloater(LLFloaterBuildOptions::getInstance(), FALSE);
}

void LLFloaterTools::setEditTool(void* tool_pointer)
{
	select_tool(tool_pointer);
}

void LLFloaterTools::onFocusReceived()
{
	gToolMgr->setCurrentToolset(gBasicToolset);
}
