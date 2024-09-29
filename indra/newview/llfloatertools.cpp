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
bool LLFloaterTools::sPreviousFocusOnAvatar = false;

const std::string PANEL_NAMES[LLFloaterTools::PANEL_COUNT] =
{
    std::string("General"),     // PANEL_GENERAL,
    std::string("Object"),  // PANEL_OBJECT,
    std::string("Features"),    // PANEL_FEATURES,
    std::string("Texture"), // PANEL_FACE,
    std::string("Content"), // PANEL_CONTENTS,
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
        }
    }
};

//static
void*   LLFloaterTools::createPanelPermissions(void* data)
{
    LLFloaterTools* floater = (LLFloaterTools*)data;
    floater->mPanelPermissions = new LLPanelPermissions();
    return floater->mPanelPermissions;
}
//static
void*   LLFloaterTools::createPanelObject(void* data)
{
    LLFloaterTools* floater = (LLFloaterTools*)data;
    floater->mPanelObject = new LLPanelObject();
    return floater->mPanelObject;
}

//static
void*   LLFloaterTools::createPanelVolume(void* data)
{
    LLFloaterTools* floater = (LLFloaterTools*)data;
    floater->mPanelVolume = new LLPanelVolume();
    return floater->mPanelVolume;
}

//static
void*   LLFloaterTools::createPanelFace(void* data)
{
    LLFloaterTools* floater = (LLFloaterTools*)data;
    floater->mPanelFace = new LLPanelFace();
    return floater->mPanelFace;
}

//static
void*   LLFloaterTools::createPanelContents(void* data)
{
    LLFloaterTools* floater = (LLFloaterTools*)data;
    floater->mPanelContents = new LLPanelContents();
    return floater->mPanelContents;
}

//static
void*   LLFloaterTools::createPanelLandInfo(void* data)
{
    LLFloaterTools* floater = (LLFloaterTools*)data;
    floater->mPanelLandInfo = new LLPanelLandInfo();
    return floater->mPanelLandInfo;
}

static  const std::string   toolNames[]={
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

bool    LLFloaterTools::postBuild()
{
    // Hide until tool selected
    setVisible(false);

    // Since we constantly show and hide this during drags, don't
    // make sounds on visibility changes.
    setSoundFlags(LLView::SILENT);

    getDragHandle()->setEnabled( !gSavedSettings.getBOOL("ToolboxAutoMove") );

    LLRect rect;
    mBtnFocus           = getChild<LLButton>("button focus");//btn;
    mBtnMove            = getChild<LLButton>("button move");
    mBtnEdit            = getChild<LLButton>("button edit");
    mBtnCreate          = getChild<LLButton>("button create");
    mBtnLand            = getChild<LLButton>("button land" );
    mTextStatus         = getChild<LLTextBox>("text status");
    mRadioGroupFocus    = getChild<LLRadioGroup>("focus_radio_group");
    mRadioGroupMove     = getChild<LLRadioGroup>("move_radio_group");
    mRadioGroupEdit     = getChild<LLRadioGroup>("edit_radio_group");
    mBtnGridOptions     = getChild<LLButton>("Options...");
    mBtnLink            = getChild<LLButton>("link_btn");
    mBtnUnlink          = getChild<LLButton>("unlink_btn");

    mCheckSelectIndividual  = getChild<LLCheckBoxCtrl>("checkbox edit linked parts");
    getChild<LLUICtrl>("checkbox edit linked parts")->setValue((bool)gSavedSettings.getBOOL("EditLinkedParts"));
    mCheckSnapToGrid        = getChild<LLCheckBoxCtrl>("checkbox snap to grid");
    getChild<LLUICtrl>("checkbox snap to grid")->setValue((bool)gSavedSettings.getBOOL("SnapEnabled"));
    mCheckStretchUniform    = getChild<LLCheckBoxCtrl>("checkbox uniform");
    getChild<LLUICtrl>("checkbox uniform")->setValue((bool)gSavedSettings.getBOOL("ScaleUniform"));
    mCheckStretchTexture    = getChild<LLCheckBoxCtrl>("checkbox stretch textures");
    getChild<LLUICtrl>("checkbox stretch textures")->setValue((bool)gSavedSettings.getBOOL("ScaleStretchTextures"));
    mComboGridMode          = getChild<LLComboBox>("combobox grid mode");

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
            LL_WARNS() << "Tool button not found! DOA Pending." << LL_ENDL;
        }
    }
    mCheckCopySelection = getChild<LLCheckBoxCtrl>("checkbox copy selection");
    getChild<LLUICtrl>("checkbox copy selection")->setValue((bool)gSavedSettings.getBOOL("CreateToolCopySelection"));
    mCheckSticky = getChild<LLCheckBoxCtrl>("checkbox sticky");
    getChild<LLUICtrl>("checkbox sticky")->setValue((bool)gSavedSettings.getBOOL("CreateToolKeepSelected"));
    mCheckCopyCenters = getChild<LLCheckBoxCtrl>("checkbox copy centers");
    getChild<LLUICtrl>("checkbox copy centers")->setValue((bool)gSavedSettings.getBOOL("CreateToolCopyCenters"));
    mCheckCopyRotates = getChild<LLCheckBoxCtrl>("checkbox copy rotates");
    getChild<LLUICtrl>("checkbox copy rotates")->setValue((bool)gSavedSettings.getBOOL("CreateToolCopyRotates"));

    mRadioGroupLand         = getChild<LLRadioGroup>("land_radio_group");
    mBtnApplyToSelection    = getChild<LLButton>("button apply to selection");
    mSliderDozerSize        = getChild<LLSlider>("slider brush size");
    getChild<LLUICtrl>("slider brush size")->setValue(gSavedSettings.getF32("LandBrushSize"));
    mSliderDozerForce       = getChild<LLSlider>("slider force");
    // the setting stores the actual force multiplier, but the slider is logarithmic, so we convert here
    getChild<LLUICtrl>("slider force")->setValue(log10(gSavedSettings.getF32("LandBrushForce")));

    mTextBulldozer = getChild<LLTextBox>("Bulldozer:");
    mTextDozerSize = getChild<LLTextBox>("Dozer Size:");
    mTextDozerStrength = getChild<LLTextBox>("Strength:");
    mSliderZoom = getChild<LLSlider>("slider zoom");

    mTextSelectionCount = getChild<LLTextBox>("selection_count");
    mTextSelectionEmpty = getChild<LLTextBox>("selection_empty");
    mTextSelectionFaces = getChild<LLTextBox>("selection_faces");

    mCostTextBorder = getChild<LLViewBorder>("cost_text_border");

    mTab = getChild<LLTabContainer>("Object Info Tabs");
    if(mTab)
    {
        mTab->setFollows(FOLLOWS_TOP | FOLLOWS_LEFT);
        mTab->setBorderVisible(false);
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

    return true;
}

// Create the popupview with a dummy center.  It will be moved into place
// during LLViewerWindow's per-frame hover processing.
LLFloaterTools::LLFloaterTools(const LLSD& key)
:   LLFloater(key),
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

    mDirty(true),
    mHasSelection(true)
{
    gFloaterTools = this;

    setAutoFocus(false);
    mFactoryMap["General"] = LLCallbackMap(createPanelPermissions, this);//LLPanelPermissions
    mFactoryMap["Object"] = LLCallbackMap(createPanelObject, this);//LLPanelObject
    mFactoryMap["Features"] = LLCallbackMap(createPanelVolume, this);//LLPanelVolume
    mFactoryMap["Texture"] = LLCallbackMap(createPanelFace, this);//LLPanelFace
    mFactoryMap["Contents"] = LLCallbackMap(createPanelContents, this);//LLPanelContents
    mFactoryMap["land info panel"] = LLCallbackMap(createPanelLandInfo, this);//LLPanelLandInfo

    mCommitCallbackRegistrar.add("BuildTool.setTool",           { boost::bind(&LLFloaterTools::setTool,this, _2) });
    mCommitCallbackRegistrar.add("BuildTool.commitZoom",        { boost::bind(&commit_slider_zoom, _1), cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("BuildTool.commitRadioFocus",  { boost::bind(&commit_radio_group_focus, _1), cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("BuildTool.commitRadioMove",   { boost::bind(&commit_radio_group_move,_1), cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("BuildTool.commitRadioEdit",   { boost::bind(&commit_radio_group_edit,_1), cb_info::UNTRUSTED_BLOCK });

    mCommitCallbackRegistrar.add("BuildTool.gridMode",          { boost::bind(&commit_grid_mode,_1), cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("BuildTool.selectComponent",   { boost::bind(&commit_select_component, this), cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("BuildTool.gridOptions",       { boost::bind(&LLFloaterTools::onClickGridOptions,this) });
    mCommitCallbackRegistrar.add("BuildTool.applyToSelection",  { boost::bind(&click_apply_to_selection, this), cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("BuildTool.commitRadioLand",   { boost::bind(&commit_radio_group_land,_1), cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("BuildTool.LandBrushForce",    { boost::bind(&commit_slider_dozer_force,_1), cb_info::UNTRUSTED_BLOCK });

    mCommitCallbackRegistrar.add("BuildTool.LinkObjects",       { boost::bind(&LLSelectMgr::linkObjects, LLSelectMgr::getInstance()) });
    mCommitCallbackRegistrar.add("BuildTool.UnlinkObjects",     { boost::bind(&LLSelectMgr::unlinkObjects, LLSelectMgr::getInstance()) });

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
    bool all_volume = LLSelectMgr::getInstance()->selectionAllPCode( LL_PCODE_VOLUME );

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
        mTextSelectionCount->setTextArg("[OBJ_COUNT]", obj_count_string);
        std::string prim_count_string;
        LLResMgr::getInstance()->getIntegerString(prim_count_string, LLSelectMgr::getInstance()->getSelection()->getObjectCount());
        mTextSelectionCount->setTextArg("[PRIM_COUNT]", prim_count_string);

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
        LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
        F32 link_cost = selection->getSelectedLinksetCost();
        S32 link_count = selection->getRootObjectCount();
        S32 object_count = selection->getObjectCount();

        LLCrossParcelFunctor func;
        if (!LLSelectMgr::getInstance()->getSelection()->applyToRootObjects(&func, true))
        {
            // Unless multiple parcels selected, higlight parcel object is at.
            LLViewerObject* selected_object = mObjectSelection->getFirstObject();
            if (selected_object)
            {
                // Select a parcel at the currently selected object's position.
                LLViewerParcelMgr::getInstance()->selectParcelAt(selected_object->getPositionGlobal());
            }
            else
            {
                LL_WARNS() << "Failed to get selected object" << LL_ENDL;
            }
        }

        if (object_count == 1)
        {
            // "selection_faces" shouldn't be visible if not LLToolFace::getInstance()
            // But still need to be populated in case user switches

            std::string faces_str = "";

            for (LLObjectSelection::iterator iter = selection->begin(); iter != selection->end();)
            {
                LLObjectSelection::iterator nextiter = iter++; // not strictly needed, we have only one object
                LLSelectNode* node = *nextiter;
                LLViewerObject* object = (*nextiter)->getObject();
                if (!object)
                    continue;
                S32 num_tes = llmin((S32)object->getNumTEs(), (S32)object->getNumFaces());
                for (S32 te = 0; te < num_tes; ++te)
                {
                    if (node->isTESelected(te))
                    {
                        if (!faces_str.empty())
                        {
                            faces_str += ", ";
                        }
                        faces_str += llformat("%d", te);
                    }
                }
            }
            mTextSelectionFaces->setTextArg("[FACES_STRING]", faces_str);
        }

        bool show_faces = (object_count == 1)
                          && LLToolFace::getInstance() == LLToolMgr::getInstance()->getCurrentTool();
        mTextSelectionFaces->setVisible(show_faces);

        LLStringUtil::format_map_t selection_args;
        selection_args["OBJ_COUNT"] = llformat("%.1d", link_count);
        selection_args["LAND_IMPACT"] = llformat("%.1d", (S32)link_cost);

        mTextSelectionCount->setText(getString("status_selectcount", selection_args));
    }


    // Refresh child tabs
    mPanelPermissions->refresh();
    mPanelObject->refresh();
    mPanelVolume->refresh();
    mPanelFace->refresh();
    mPanelFace->refreshMedia();
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
    bool has_selection = !LLSelectMgr::getInstance()->getSelection()->isEmpty();
    if(!has_selection && (mHasSelection != has_selection))
    {
        mDirty = true;
    }
    mHasSelection = has_selection;

    if (mDirty)
    {
        refresh();
        mDirty = false;
    }

    //  mCheckSelectIndividual->set(gSavedSettings.getBOOL("EditLinkedParts"));
    LLFloater::draw();
}

void LLFloaterTools::dirty()
{
    mDirty = true;
    LLFloaterOpenObject* instance = LLFloaterReg::findTypedInstance<LLFloaterOpenObject>("openobject");
    if (instance) instance->dirty();
}

// Clean up any tool state that should not persist when the
// floater is closed.
void LLFloaterTools::resetToolState()
{
    gCameraBtnZoom = true;
    gCameraBtnOrbit = false;
    gCameraBtnPan = false;

    gGrabBtnSpin = false;
    gGrabBtnVertical = false;
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
    {   // SL looks odd if we draw the tools while the window is minimized
        return;
    }

    // Focus buttons
    bool focus_visible = (  tool == LLToolCamera::getInstance() );

    mBtnFocus   ->setToggleState( focus_visible );

    mRadioGroupFocus->setVisible( focus_visible );

    mSliderZoom->setVisible( focus_visible);
    mSliderZoom->setEnabled(gCameraBtnZoom);

    if (!gCameraBtnOrbit &&
        !gCameraBtnPan &&
        !(mask == MASK_ORBIT) &&
        !(mask == (MASK_ORBIT | MASK_ALT)) &&
        !(mask == MASK_PAN) &&
        !(mask == (MASK_PAN | MASK_ALT)) )
    {
        mRadioGroupFocus->setValue("radio zoom");
    }
    else if (   gCameraBtnOrbit ||
                (mask == MASK_ORBIT) ||
                (mask == (MASK_ORBIT | MASK_ALT)) )
    {
        mRadioGroupFocus->setValue("radio orbit");
    }
    else if (   gCameraBtnPan ||
                (mask == MASK_PAN) ||
                (mask == (MASK_PAN | MASK_ALT)) )
    {
        mRadioGroupFocus->setValue("radio pan");
    }

    // multiply by correction factor because volume sliders go [0, 0.5]
    mSliderZoom->setValue(gAgentCamera.getCameraZoomFraction() * 0.5f);

    // Move buttons
    bool move_visible = (tool == LLToolGrab::getInstance());

    if (mBtnMove) mBtnMove  ->setToggleState( move_visible );

    // HACK - highlight buttons for next click
    mRadioGroupMove->setVisible(move_visible);
    if (!(gGrabBtnSpin ||
        gGrabBtnVertical ||
        (mask == MASK_VERTICAL) ||
        (mask == MASK_SPIN)))
    {
        mRadioGroupMove->setValue("radio move");
    }
    else if ((mask == MASK_VERTICAL) ||
             (gGrabBtnVertical && (mask != MASK_SPIN)))
    {
        mRadioGroupMove->setValue("radio lift");
    }
    else if ((mask == MASK_SPIN) ||
             (gGrabBtnSpin && (mask != MASK_VERTICAL)))
    {
        mRadioGroupMove->setValue("radio spin");
    }

    // Edit buttons
    bool edit_visible = tool == LLToolCompTranslate::getInstance() ||
                        tool == LLToolCompRotate::getInstance() ||
                        tool == LLToolCompScale::getInstance() ||
                        tool == LLToolFace::getInstance() ||
                        tool == LLToolIndividual::getInstance() ||
                        tool == LLToolPipette::getInstance();

    mBtnEdit    ->setToggleState( edit_visible );
    mRadioGroupEdit->setVisible( edit_visible );
    //bool linked_parts = gSavedSettings.getBOOL("EditLinkedParts");
    //getChildView("RenderingCost")->setVisible( !linked_parts && (edit_visible || focus_visible || move_visible) && sShowObjectCost);

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

    //mCheckSelectLinked    ->setVisible( edit_visible );
    if (mCheckStretchUniform) mCheckStretchUniform->setVisible( edit_visible );
    if (mCheckStretchTexture) mCheckStretchTexture->setVisible( edit_visible );
    if (mCheckStretchUniformLabel) mCheckStretchUniformLabel->setVisible( edit_visible );

    // Create buttons
    bool create_visible = (tool == LLToolCompCreate::getInstance());

    mBtnCreate  ->setToggleState(   tool == LLToolCompCreate::getInstance() );

    if (mCheckCopySelection
        && mCheckCopySelection->get())
    {
        // don't highlight any placer button
        for (std::vector<LLButton*>::size_type i = 0; i < mButtons.size(); i++)
        {
            mButtons[i]->setToggleState(false);
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
            bool state = (pcode == button_pcode);
            mButtons[t]->setToggleState( state );
            mButtons[t]->setVisible( create_visible );
        }
    }

    if (mCheckSticky) mCheckSticky      ->setVisible( create_visible );
    if (mCheckCopySelection) mCheckCopySelection    ->setVisible( create_visible );
    if (mCheckCopyCenters) mCheckCopyCenters    ->setVisible( create_visible );
    if (mCheckCopyRotates) mCheckCopyRotates    ->setVisible( create_visible );

    if (mCheckCopyCenters && mCheckCopySelection) mCheckCopyCenters->setEnabled( mCheckCopySelection->get() );
    if (mCheckCopyRotates && mCheckCopySelection) mCheckCopyRotates->setEnabled( mCheckCopySelection->get() );

    // Land buttons
    bool land_visible = (tool == LLToolBrushLand::getInstance() || tool == LLToolSelectLand::getInstance() );

    mCostTextBorder->setVisible(!land_visible);

    if (mBtnLand)   mBtnLand    ->setToggleState( land_visible );

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
        mSliderDozerSize->setVisible( land_visible );
        mTextBulldozer->setVisible( land_visible);
        mTextDozerSize->setVisible( land_visible);
    }
    if (mSliderDozerForce)
    {
        mSliderDozerForce->setVisible( land_visible );
        mTextDozerStrength->setVisible( land_visible);
    }

    bool have_selection = !LLSelectMgr::getInstance()->getSelection()->isEmpty();

    mTextSelectionCount->setVisible(!land_visible && have_selection);
    mTextSelectionFaces->setVisible(LLToolFace::getInstance() == LLToolMgr::getInstance()->getCurrentTool()
                                                && LLSelectMgr::getInstance()->getSelection()->getObjectCount() == 1);
    mTextSelectionEmpty->setVisible(!land_visible && !have_selection);

    mTab->setVisible(!land_visible);
    mPanelLandInfo->setVisible(land_visible);
}


// virtual
bool LLFloaterTools::canClose()
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

    LLTool* tool = LLToolMgr::getInstance()->getCurrentTool();
    if (tool == LLToolCompInspect::getInstance()
        || tool == LLToolDragAndDrop::getInstance())
    {
        // Something called floater up while it was supressed (during drag n drop, inspect),
        // so it won't be getting any layout or visibility updates, update once
        // further updates will come from updateLayout()
        LLCoordGL select_center_screen;
        MASK    mask = gKeyboard->currentMask(true);
        updatePopup(select_center_screen, mask);
    }

    //gMenuBarView->setItemVisible("BuildTools", true);
}

// virtual
void LLFloaterTools::onClose(bool app_quitting)
{
    mTab->setVisible(false);

    LLViewerJoystick::getInstance()->moveAvatar(false);

    // destroy media source used to grab media title
    mPanelFace->unloadMedia();

    // Different from handle_reset_view in that it doesn't actually
    //   move the camera if EditCameraMovement is not set.
    gAgentCamera.resetView(gSavedSettings.getBOOL("EditCameraMovement"));

    // exit component selection mode
    LLSelectMgr::getInstance()->promoteSelectionToRoot();
    gSavedSettings.setBOOL("EditLinkedParts", false);

    gViewerWindow->showCursor();

    resetToolState();

    mParcelSelection = NULL;
    mObjectSelection = NULL;

    // Switch back to basic toolset
    LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
    // we were already in basic toolset, using build tools
    // so manually reset tool to default (pie menu tool)
    LLToolMgr::getInstance()->getCurrentToolset()->selectFirstTool();

    //gMenuBarView->setItemVisible("BuildTools", false);
    LLFloaterReg::hideInstance("media_settings");

    // hide the advanced object weights floater
    LLFloaterReg::hideInstance("object_weights");

    // hide gltf material editor
    LLFloaterReg::hideInstance("live_material_editor");

    // prepare content for next call
    mPanelContents->clearContents();

    if(sPreviousFocusOnAvatar)
    {
        sPreviousFocusOnAvatar = false;
        gAgentCamera.setAllowChangeToFollow(true);
    }
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
        gGrabBtnVertical = false;
        gGrabBtnSpin = false;
    }
    else if (selected == "radio lift")
    {
        gGrabBtnVertical = true;
        gGrabBtnSpin = false;
    }
    else if (selected == "radio spin")
    {
        gGrabBtnVertical = false;
        gGrabBtnSpin = true;
    }
}

void commit_radio_group_focus(LLUICtrl* ctrl)
{
    LLRadioGroup* group = (LLRadioGroup*)ctrl;
    std::string selected = group->getValue().asString();
    if (selected == "radio zoom")
    {
        gCameraBtnZoom = true;
        gCameraBtnOrbit = false;
        gCameraBtnPan = false;
    }
    else if (selected == "radio orbit")
    {
        gCameraBtnZoom = false;
        gCameraBtnOrbit = true;
        gCameraBtnPan = false;
    }
    else if (selected == "radio pan")
    {
        gCameraBtnZoom = false;
        gCameraBtnOrbit = false;
        gCameraBtnPan = true;
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

    bool select_individuals = floaterp->mCheckSelectIndividual->get();
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
    gSavedSettings.setBOOL("CreateToolCopySelection", false);
    gFocusMgr.setMouseCapture(NULL);
}

void commit_grid_mode(LLUICtrl *ctrl)
{
    LLComboBox* combo = (LLComboBox*)ctrl;

    LLSelectMgr::getInstance()->setGridMode((EGridMode)combo->getCurrentIndex());
}

// static
void LLFloaterTools::setGridMode(S32 mode)
{
    LLFloaterTools* tools_floater = LLFloaterReg::getTypedInstance<LLFloaterTools>("build");
    if (!tools_floater || !tools_floater->mComboGridMode)
    {
        return;
    }

    tools_floater->mComboGridMode->setCurrentByIndex(mode);
}

void LLFloaterTools::onClickGridOptions()
{
    LLFloater* floaterp = LLFloaterReg::showInstance("build_options");
    // position floater next to build tools, not over
    floaterp->setShape(gFloaterView->findNeighboringPosition(this, floaterp), true);
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
        LL_WARNS()<<" no parameter name "<<control_name<<" found!! No Tool selected!!"<< LL_ENDL;
}

void LLFloaterTools::onFocusReceived()
{
    LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
    LLFloater::onFocusReceived();
}

void LLFloaterTools::updateLandImpacts()
{
    LLParcel *parcel = mParcelSelection->getParcel();
    if (!parcel)
    {
        return;
    }

    // Update land impacts info in the weights floater
    LLFloaterObjectWeights* object_weights_floater = LLFloaterReg::findTypedInstance<LLFloaterObjectWeights>("object_weights");
    if(object_weights_floater)
    {
        object_weights_floater->updateLandImpacts(parcel);
    }
}

