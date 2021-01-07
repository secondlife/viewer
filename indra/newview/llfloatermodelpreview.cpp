/**
 * @file llfloatermodelpreview.cpp
 * @brief LLFloaterModelPreview class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include "llmodelloader.h"
#include "llmodelpreview.h"

#include "llfloatermodelpreview.h"

#include "llfilepicker.h"
#include "llimagebmp.h"
#include "llimagetga.h"
#include "llimagejpeg.h"
#include "llimagepng.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llfocusmgr.h"
#include "llmeshrepository.h"
#include "llnotificationsutil.h"
#include "llsdutil_math.h"
#include "llskinningutil.h"
#include "lltextbox.h"
#include "lltoolmgr.h"
#include "llui.h"
#include "llviewerwindow.h"
#include "pipeline.h"
#include "llviewercontrol.h"
#include "llviewermenufile.h" //LLFilePickerThread
#include "llstring.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "lltabcontainer.h"
#include "lltrans.h"
#include "llcallbacklist.h"
#include "llviewertexteditor.h"
#include "llviewernetwork.h"


//static
S32 LLFloaterModelPreview::sUploadAmount = 10;
LLFloaterModelPreview* LLFloaterModelPreview::sInstance = NULL;

// "Retain%" decomp parameter has values from 0.0 to 1.0 by 0.01
// But according to the UI spec for upload model floater, this parameter
// should be represented by Retain spinner with values from 1 to 100 by 1.
// To achieve this, RETAIN_COEFFICIENT is used while creating spinner
// and when value is requested from spinner.
const double RETAIN_COEFFICIENT = 100;

// "Cosine%" decomp parameter has values from 0.9 to 1 by 0.001
// But according to the UI spec for upload model floater, this parameter
// should be represented by Smooth combobox with only 10 values.
// So this const is used as a size of Smooth combobox list.
const S32 SMOOTH_VALUES_NUMBER = 10;
const S32 PREVIEW_RENDER_SIZE = 1024;
const F32 PREVIEW_CAMERA_DISTANCE = 16.f;

class LLMeshFilePicker : public LLFilePickerThread
{
public:
    LLMeshFilePicker(LLModelPreview* mp, S32 lod);
    virtual void notify(const std::vector<std::string>& filenames);

private:
    LLModelPreview* mMP;
    S32 mLOD;
};

LLMeshFilePicker::LLMeshFilePicker(LLModelPreview* mp, S32 lod)
: LLFilePickerThread(LLFilePicker::FFLOAD_COLLADA)
	{
		mMP = mp;
		mLOD = lod;
	}

void LLMeshFilePicker::notify(const std::vector<std::string>& filenames)
{
	if (filenames.size() > 0)
	{
		mMP->loadModel(filenames[0], mLOD);
	}
	else
	{
		//closes floater
		mMP->loadModel(std::string(), mLOD);
	}
}

//-----------------------------------------------------------------------------
// LLFloaterModelPreview()
//-----------------------------------------------------------------------------
LLFloaterModelPreview::LLFloaterModelPreview(const LLSD& key) :
LLFloaterModelUploadBase(key),
mUploadBtn(NULL),
mCalculateBtn(NULL),
mUploadLogText(NULL),
mTabContainer(NULL),
mAvatarTabIndex(0)
{
	sInstance = this;
	mLastMouseX = 0;
	mLastMouseY = 0;
	mStatusLock = new LLMutex();
	mModelPreview = NULL;

	mLODMode[LLModel::LOD_HIGH] = 0;
	for (U32 i = 0; i < LLModel::LOD_HIGH; i++)
	{
		mLODMode[i] = 1;
	}
}

//-----------------------------------------------------------------------------
// postBuild()
//-----------------------------------------------------------------------------
BOOL LLFloaterModelPreview::postBuild()
{
	if (!LLFloater::postBuild())
	{
		return FALSE;
	}

	childSetCommitCallback("cancel_btn", onCancel, this);
	childSetCommitCallback("crease_angle", onGenerateNormalsCommit, this);
	getChild<LLCheckBoxCtrl>("gen_normals")->setCommitCallback(boost::bind(&LLFloaterModelPreview::toggleGenarateNormals, this));

	childSetCommitCallback("lod_generate", onAutoFillCommit, this);

	for (S32 lod = 0; lod <= LLModel::LOD_HIGH; ++lod)
	{
		LLComboBox* lod_source_combo = getChild<LLComboBox>("lod_source_" + lod_name[lod]);
		lod_source_combo->setCommitCallback(boost::bind(&LLFloaterModelPreview::onLoDSourceCommit, this, lod));
		lod_source_combo->setCurrentByIndex(mLODMode[lod]);

		getChild<LLButton>("lod_browse_" + lod_name[lod])->setCommitCallback(boost::bind(&LLFloaterModelPreview::onBrowseLOD, this, lod));
		getChild<LLComboBox>("lod_mode_" + lod_name[lod])->setCommitCallback(boost::bind(&LLFloaterModelPreview::onLODParamCommit, this, lod, false));
		getChild<LLSpinCtrl>("lod_error_threshold_" + lod_name[lod])->setCommitCallback(boost::bind(&LLFloaterModelPreview::onLODParamCommit, this, lod, false));
		getChild<LLSpinCtrl>("lod_triangle_limit_" + lod_name[lod])->setCommitCallback(boost::bind(&LLFloaterModelPreview::onLODParamCommit, this, lod, true));
	}

	// Upload/avatar options, they need to refresh errors/notifications
	childSetCommitCallback("upload_skin", boost::bind(&LLFloaterModelPreview::onUploadOptionChecked, this, _1), NULL);
	childSetCommitCallback("upload_joints", boost::bind(&LLFloaterModelPreview::onUploadOptionChecked, this, _1), NULL);
	childSetCommitCallback("lock_scale_if_joint_position", boost::bind(&LLFloaterModelPreview::onUploadOptionChecked, this, _1), NULL);
	childSetCommitCallback("upload_textures", boost::bind(&LLFloaterModelPreview::onUploadOptionChecked, this, _1), NULL);

	childSetTextArg("status", "[STATUS]", getString("status_idle"));

	childSetAction("ok_btn", onUpload, this);
	childDisable("ok_btn");

	childSetAction("reset_btn", onReset, this);

	childSetCommitCallback("preview_lod_combo", onPreviewLODCommit, this);

	childSetCommitCallback("import_scale", onImportScaleCommit, this);
	childSetCommitCallback("pelvis_offset", onPelvisOffsetCommit, this);

	getChild<LLLineEditor>("description_form")->setKeystrokeCallback(boost::bind(&LLFloaterModelPreview::onDescriptionKeystroke, this, _1), NULL);

	getChild<LLCheckBoxCtrl>("show_edges")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onViewOptionChecked, this, _1));
	getChild<LLCheckBoxCtrl>("show_physics")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onViewOptionChecked, this, _1));
	getChild<LLCheckBoxCtrl>("show_textures")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onViewOptionChecked, this, _1));
	getChild<LLCheckBoxCtrl>("show_skin_weight")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onShowSkinWeightChecked, this, _1));
	getChild<LLCheckBoxCtrl>("show_joint_overrides")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onViewOptionChecked, this, _1));
	getChild<LLCheckBoxCtrl>("show_joint_positions")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onViewOptionChecked, this, _1));

	childDisable("upload_skin");
	childDisable("upload_joints");
	childDisable("lock_scale_if_joint_position");

	childSetVisible("skin_too_many_joints", false);
	childSetVisible("skin_unknown_joint", false);

    childSetVisible("warning_title", false);
    childSetVisible("warning_message", false);

	initDecompControls();

	LLView* preview_panel = getChild<LLView>("preview_panel");

	mPreviewRect = preview_panel->getRect();

	initModelPreview();

	//set callbacks for left click on line editor rows
	for (U32 i = 0; i <= LLModel::LOD_HIGH; i++)
	{
		LLTextBox* text = getChild<LLTextBox>(lod_label_name[i]);
		if (text)
		{
			text->setMouseDownCallback(boost::bind(&LLFloaterModelPreview::setPreviewLOD, this, i));
		}

		text = getChild<LLTextBox>(lod_triangles_name[i]);
		if (text)
		{
			text->setMouseDownCallback(boost::bind(&LLFloaterModelPreview::setPreviewLOD, this, i));
		}

		text = getChild<LLTextBox>(lod_vertices_name[i]);
		if (text)
		{
			text->setMouseDownCallback(boost::bind(&LLFloaterModelPreview::setPreviewLOD, this, i));
		}

		text = getChild<LLTextBox>(lod_status_name[i]);
		if (text)
		{
			text->setMouseDownCallback(boost::bind(&LLFloaterModelPreview::setPreviewLOD, this, i));
		}
	}
	std::string current_grid = LLGridManager::getInstance()->getGridId();
	std::transform(current_grid.begin(),current_grid.end(),current_grid.begin(),::tolower);
	std::string validate_url;
	if (current_grid == "agni")
	{
		validate_url = "http://secondlife.com/my/account/mesh.php";
	}
	else if (current_grid == "damballah")
	{
		// Staging grid has its own naming scheme.
		validate_url = "http://secondlife-staging.com/my/account/mesh.php";
	}
	else
	{
		validate_url = llformat("http://secondlife.%s.lindenlab.com/my/account/mesh.php",current_grid.c_str());
	}
	getChild<LLTextBox>("warning_message")->setTextArg("[VURL]", validate_url);

	mUploadBtn = getChild<LLButton>("ok_btn");
	mCalculateBtn = getChild<LLButton>("calculate_btn");
	mUploadLogText = getChild<LLViewerTextEditor>("log_text");
	mTabContainer = getChild<LLTabContainer>("import_tab");

    LLPanel *panel = mTabContainer->getPanelByName("rigging_panel");
    mAvatarTabIndex = mTabContainer->getIndexForPanel(panel);
    panel->getChild<LLScrollListCtrl>("joints_list")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onJointListSelection, this));

	if (LLConvexDecomposition::getInstance() != NULL)
	{
	mCalculateBtn->setClickedCallback(boost::bind(&LLFloaterModelPreview::onClickCalculateBtn, this));

	toggleCalculateButton(true);
	}
	else
	{
		mCalculateBtn->setEnabled(false);
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// reshape()
//-----------------------------------------------------------------------------

void LLFloaterModelPreview::reshape(S32 width, S32 height, BOOL called_from_parent)
{
    LLFloaterModelUploadBase::reshape(width, height, called_from_parent);

    LLView* preview_panel = getChild<LLView>("preview_panel");
    LLRect rect = preview_panel->getRect();

    if (rect != mPreviewRect)
    {
        mModelPreview->refresh();
        mPreviewRect = preview_panel->getRect();
    }
}

//-----------------------------------------------------------------------------
// LLFloaterModelPreview()
//-----------------------------------------------------------------------------
LLFloaterModelPreview::~LLFloaterModelPreview()
{
	sInstance = NULL;
	
	if ( mModelPreview )
	{
		delete mModelPreview;
	}

	delete mStatusLock;
	mStatusLock = NULL;
}

void LLFloaterModelPreview::initModelPreview()
{
	if (mModelPreview)
	{
		delete mModelPreview;
	}

	S32 tex_width = 512;
	S32 tex_height = 512;

	S32 max_width = llmin(PREVIEW_RENDER_SIZE, (S32)gPipeline.mScreenWidth);
	S32 max_height = llmin(PREVIEW_RENDER_SIZE, (S32)gPipeline.mScreenHeight);

	while ((tex_width << 1) < max_width)
	{
		tex_width <<= 1;
	}
	while ((tex_height << 1) < max_height)
	{
		tex_height <<= 1;
	}

	mModelPreview = new LLModelPreview(tex_width, tex_height, this);
    mModelPreview->setPreviewTarget(PREVIEW_CAMERA_DISTANCE);
	mModelPreview->setDetailsCallback(boost::bind(&LLFloaterModelPreview::setDetails, this, _1, _2, _3, _4, _5));
	mModelPreview->setModelUpdatedCallback(boost::bind(&LLFloaterModelPreview::modelUpdated, this, _1));
}

void LLFloaterModelPreview::onUploadOptionChecked(LLUICtrl* ctrl)
{
	if (mModelPreview)
	{
		auto name = ctrl->getName();
        bool value = ctrl->getValue().asBoolean();
        // update the option and notifications
        // (this is a bit convoluted, because of the current structure of mModelPreview)
        if (name == "upload_skin")
        {
            childSetValue("show_skin_weight", value);
            mModelPreview->mViewOption["show_skin_weight"] = value;
            if (!value)
            {
                mModelPreview->mViewOption["show_joint_overrides"] = false;
                mModelPreview->mViewOption["show_joint_positions"] = false;
                childSetValue("show_joint_overrides", false);
                childSetValue("show_joint_positions", false);
            }
        }
        else if (name == "upload_joints")
        {
            if (mModelPreview->mViewOption["show_skin_weight"])
            {
                childSetValue("show_joint_overrides", value);
                mModelPreview->mViewOption["show_joint_overrides"] = value;
            }
        }
        else if (name == "upload_textures")
        {
            childSetValue("show_textures", value);
            mModelPreview->mViewOption["show_textures"] = value;
        }
        else if (name == "lock_scale_if_joint_position")
        {
            mModelPreview->mViewOption["lock_scale_if_joint_position"] = value;
        }

        mModelPreview->refresh(); // a 'dirty' flag for render
        mModelPreview->resetPreviewTarget(); 
        mModelPreview->clearBuffers();
        mModelPreview->mDirty = true;
    }
    // set the button visible, it will be refreshed later
	toggleCalculateButton(true);
}

void LLFloaterModelPreview::onShowSkinWeightChecked(LLUICtrl* ctrl)
{
	if (mModelPreview)
	{
		mModelPreview->mCameraOffset.clearVec();
		onViewOptionChecked(ctrl);
	}
}

void LLFloaterModelPreview::onViewOptionChecked(LLUICtrl* ctrl)
{
	if (mModelPreview)
	{
		auto name = ctrl->getName();
		mModelPreview->mViewOption[name] = !mModelPreview->mViewOption[name];
		if (name == "show_physics")
		{
			auto enabled = mModelPreview->mViewOption[name];
			childSetEnabled("physics_explode", enabled);
			childSetVisible("physics_explode", enabled);
		}
		mModelPreview->refresh();
	}
}

bool LLFloaterModelPreview::isViewOptionChecked(const LLSD& userdata)
{
	if (mModelPreview)
	{
		return mModelPreview->mViewOption[userdata.asString()];
	}

	return false;
}

bool LLFloaterModelPreview::isViewOptionEnabled(const LLSD& userdata)
{
	return getChildView(userdata.asString())->getEnabled();
}

void LLFloaterModelPreview::setViewOptionEnabled(const std::string& option, bool enabled)
{
	childSetEnabled(option, enabled);
}

void LLFloaterModelPreview::enableViewOption(const std::string& option)
{
	setViewOptionEnabled(option, true);
}

void LLFloaterModelPreview::disableViewOption(const std::string& option)
{
	setViewOptionEnabled(option, false);
}

void LLFloaterModelPreview::loadHighLodModel()
{
	mModelPreview->mLookUpLodFiles = true;
	loadModel(3);
}

void LLFloaterModelPreview::loadModel(S32 lod)
{
	mModelPreview->mLoading = true;
	if (lod == LLModel::LOD_PHYSICS)
	{
		// loading physics from file
		mModelPreview->mPhysicsSearchLOD = lod;
	}

	(new LLMeshFilePicker(mModelPreview, lod))->getFile();
}

void LLFloaterModelPreview::loadModel(S32 lod, const std::string& file_name, bool force_disable_slm)
{
	mModelPreview->mLoading = true;

	mModelPreview->loadModel(file_name, lod, force_disable_slm);
}

void LLFloaterModelPreview::onClickCalculateBtn()
{
	clearLogTab();
	addStringToLog("Calculating model data.", false);
	mModelPreview->rebuildUploadData();

	bool upload_skinweights = childGetValue("upload_skin").asBoolean();
	bool upload_joint_positions = childGetValue("upload_joints").asBoolean();
    bool lock_scale_if_joint_position = childGetValue("lock_scale_if_joint_position").asBoolean();

    mUploadModelUrl.clear();
    mModelPhysicsFee.clear();

	gMeshRepo.uploadModel(mModelPreview->mUploadData, mModelPreview->mPreviewScale,
                          childGetValue("upload_textures").asBoolean(), 
                          upload_skinweights, upload_joint_positions, lock_scale_if_joint_position,
                          mUploadModelUrl, false,
						  getWholeModelFeeObserverHandle());

	toggleCalculateButton(false);
	mUploadBtn->setEnabled(false);
}

// Modified cell_params, make sure to clear values if you have to reuse cell_params outside of this function
void add_row_to_list(LLScrollListCtrl *listp,
                     LLScrollListCell::Params &cell_params,
                     const LLSD &item_value,
                     const std::string &name,
                     const LLSD &vx,
                     const LLSD &vy,
                     const LLSD &vz)
{
    LLScrollListItem::Params item_params;
    item_params.value = item_value;

    cell_params.column = "model_name";
    cell_params.value = name;

    item_params.columns.add(cell_params);

    cell_params.column = "axis_x";
    cell_params.value = vx;
    item_params.columns.add(cell_params);

    cell_params.column = "axis_y";
    cell_params.value = vy;
    item_params.columns.add(cell_params);

    cell_params.column = "axis_z";
    cell_params.value = vz;

    item_params.columns.add(cell_params);

    listp->addRow(item_params);
}

void populate_list_with_overrides(LLScrollListCtrl *listp, const LLJointOverrideData &data, bool include_overrides)
{
    if (data.mModelsNoOverrides.empty() && data.mPosOverrides.empty())
    {
        return;
    }

    static const std::string no_override_placeholder = "-";

    S32 count = 0;
    LLScrollListCell::Params cell_params;
    cell_params.font = LLFontGL::getFontSansSerif();
    // Start out right justifying numeric displays
    cell_params.font_halign = LLFontGL::HCENTER;

    std::map<std::string, LLVector3>::const_iterator map_iter = data.mPosOverrides.begin();
    std::map<std::string, LLVector3>::const_iterator map_end = data.mPosOverrides.end();
    while (map_iter != map_end)
    {
        if (include_overrides)
        {
            add_row_to_list(listp,
                cell_params,
                LLSD::Integer(count),
                map_iter->first,
                LLSD::Real(map_iter->second.mV[VX]),
                LLSD::Real(map_iter->second.mV[VY]),
                LLSD::Real(map_iter->second.mV[VZ]));
        }
        else
        {
            add_row_to_list(listp,
                cell_params,
                LLSD::Integer(count),
                map_iter->first,
                no_override_placeholder,
                no_override_placeholder,
                no_override_placeholder);
        }
        count++;
        map_iter++;
    }

    std::set<std::string>::const_iterator set_iter = data.mModelsNoOverrides.begin();
    std::set<std::string>::const_iterator set_end = data.mModelsNoOverrides.end();
    while (set_iter != set_end)
    {
        add_row_to_list(listp,
                        cell_params,
                        LLSD::Integer(count),
                        *set_iter,
                        no_override_placeholder,
                        no_override_placeholder,
                        no_override_placeholder);
        count++;
        set_iter++;
    }
}

void LLFloaterModelPreview::onJointListSelection()
{
    S32 display_lod = mModelPreview->mPreviewLOD;
    LLPanel *panel = mTabContainer->getPanelByName("rigging_panel");
    LLScrollListCtrl *joints_list = panel->getChild<LLScrollListCtrl>("joints_list");
    LLScrollListCtrl *joints_pos = panel->getChild<LLScrollListCtrl>("pos_overrides_list");
    LLScrollListCtrl *joints_scale = panel->getChild<LLScrollListCtrl>("scale_overrides_list");
    LLTextBox *joint_pos_descr = panel->getChild<LLTextBox>("pos_overrides_descr");

    joints_pos->deleteAllItems();
    joints_scale->deleteAllItems();

    LLScrollListItem *selected = joints_list->getFirstSelected();
    if (selected)
    {
        std::string label = selected->getValue().asString();
        LLJointOverrideData &data = mJointOverrides[display_lod][label];
        bool upload_joint_positions = childGetValue("upload_joints").asBoolean();
        populate_list_with_overrides(joints_pos, data, upload_joint_positions);

        joint_pos_descr->setTextArg("[JOINT]", label);
        mSelectedJointName = label;
    }
    else
    {
        // temporary value (shouldn't happen)
        std::string label = "mPelvis";
        joint_pos_descr->setTextArg("[JOINT]", label);
        mSelectedJointName.clear();
    }

    // Note: We can make a version of renderBones() to highlight selected joint
}

void LLFloaterModelPreview::onDescriptionKeystroke(LLUICtrl* ctrl)
{
	// Workaround for SL-4186, server doesn't allow name changes after 'calculate' stage
	LLLineEditor* input = static_cast<LLLineEditor*>(ctrl);
	if (input->isDirty()) // dirty will be reset after commit
	{
		toggleCalculateButton(true);
	}
}

//static
void LLFloaterModelPreview::onImportScaleCommit(LLUICtrl*,void* userdata)
{
	LLFloaterModelPreview *fp =(LLFloaterModelPreview *)userdata;

	if (!fp->mModelPreview)
	{
		return;
	}

	fp->mModelPreview->mDirty = true;

	fp->toggleCalculateButton(true);

	fp->mModelPreview->refresh();
}
//static
void LLFloaterModelPreview::onPelvisOffsetCommit( LLUICtrl*, void* userdata )
{
	LLFloaterModelPreview *fp =(LLFloaterModelPreview*)userdata;

	if (!fp->mModelPreview)
	{
		return;
	}

	fp->mModelPreview->mDirty = true;

	fp->toggleCalculateButton(true);

	fp->mModelPreview->refresh();
}

//static
void LLFloaterModelPreview::onPreviewLODCommit(LLUICtrl* ctrl, void* userdata)
{
	LLFloaterModelPreview *fp =(LLFloaterModelPreview *)userdata;

	if (!fp->mModelPreview)
	{
		return;
	}

	S32 which_mode = 0;

	LLComboBox* combo = (LLComboBox*) ctrl;

	which_mode = (NUM_LOD-1)-combo->getFirstSelectedIndex(); // combo box list of lods is in reverse order

	fp->mModelPreview->setPreviewLOD(which_mode);
}

//static
void LLFloaterModelPreview::onGenerateNormalsCommit(LLUICtrl* ctrl, void* userdata)
{
	LLFloaterModelPreview* fp = (LLFloaterModelPreview*) userdata;

	fp->mModelPreview->generateNormals();
}

void LLFloaterModelPreview::toggleGenarateNormals()
{
	bool enabled = childGetValue("gen_normals").asBoolean();
	mModelPreview->mViewOption["gen_normals"] = enabled;
	childSetEnabled("crease_angle", enabled);
	if(enabled) {
		mModelPreview->generateNormals();
	} else {
		mModelPreview->restoreNormals();
	}
}

//static
void LLFloaterModelPreview::onExplodeCommit(LLUICtrl* ctrl, void* userdata)
{
	LLFloaterModelPreview* fp = LLFloaterModelPreview::sInstance;

	fp->mModelPreview->refresh();
}

//static
void LLFloaterModelPreview::onAutoFillCommit(LLUICtrl* ctrl, void* userdata)
{
	LLFloaterModelPreview* fp = (LLFloaterModelPreview*) userdata;

    fp->mModelPreview->queryLODs();
}

void LLFloaterModelPreview::onLODParamCommit(S32 lod, bool enforce_tri_limit)
{
	mModelPreview->onLODParamCommit(lod, enforce_tri_limit);

	//refresh LoDs that reference this one
	for (S32 i = lod - 1; i >= 0; --i)
	{
		LLComboBox* lod_source_combo = getChild<LLComboBox>("lod_source_" + lod_name[i]);
		if (lod_source_combo->getCurrentIndex() == LLModelPreview::USE_LOD_ABOVE)
		{
			onLoDSourceCommit(i);
		}
		else
		{
			break;
		}
	}
}

void LLFloaterModelPreview::draw3dPreview()
{
	gGL.color3f(1.f, 1.f, 1.f);

	gGL.getTexUnit(0)->bind(mModelPreview);

	gGL.begin( LLRender::QUADS );
	{
		gGL.texCoord2f(0.f, 1.f);
		gGL.vertex2i(mPreviewRect.mLeft+1, mPreviewRect.mTop-1);
		gGL.texCoord2f(0.f, 0.f);
		gGL.vertex2i(mPreviewRect.mLeft+1, mPreviewRect.mBottom+1);
		gGL.texCoord2f(1.f, 0.f);
		gGL.vertex2i(mPreviewRect.mRight-1, mPreviewRect.mBottom+1);
		gGL.texCoord2f(1.f, 1.f);
		gGL.vertex2i(mPreviewRect.mRight-1, mPreviewRect.mTop-1);
	}
	gGL.end();

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
}

//-----------------------------------------------------------------------------
// draw()
//-----------------------------------------------------------------------------
void LLFloaterModelPreview::draw()
{
    LLFloater::draw();

    if (!mModelPreview)
    {
        return;
    }

	mModelPreview->update();

	if (!mModelPreview->mLoading)
	{
		if ( mModelPreview->getLoadState() == LLModelLoader::ERROR_MATERIALS )
		{
			childSetTextArg("status", "[STATUS]", getString("status_material_mismatch"));
		}
		else
		if ( mModelPreview->getLoadState() > LLModelLoader::ERROR_MODEL )
		{
			childSetTextArg("status", "[STATUS]", getString(LLModel::getStatusString(mModelPreview->getLoadState() - LLModelLoader::ERROR_MODEL)));
		}
		else
		if ( mModelPreview->getLoadState() == LLModelLoader::ERROR_PARSING )
		{
			childSetTextArg("status", "[STATUS]", getString("status_parse_error"));
			toggleCalculateButton(false);
		}
        else
        if (mModelPreview->getLoadState() == LLModelLoader::WARNING_BIND_SHAPE_ORIENTATION)
        {
			childSetTextArg("status", "[STATUS]", getString("status_bind_shape_orientation"));
        }
		else
		{
			childSetTextArg("status", "[STATUS]", getString("status_idle"));
		}
	}

	childSetTextArg("prim_cost", "[PRIM_COST]", llformat("%d", mModelPreview->mResourceCost));
	childSetTextArg("description_label", "[TEXTURES]", llformat("%d", mModelPreview->mTextureSet.size()));

    if (!isMinimized() && mModelPreview->lodsReady())
	{
		draw3dPreview();
	}
}

//-----------------------------------------------------------------------------
// handleMouseDown()
//-----------------------------------------------------------------------------
BOOL LLFloaterModelPreview::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (mPreviewRect.pointInRect(x, y))
	{
		bringToFront( x, y );
		gFocusMgr.setMouseCapture(this);
		gViewerWindow->hideCursor();
		mLastMouseX = x;
		mLastMouseY = y;
		return TRUE;
	}

	return LLFloater::handleMouseDown(x, y, mask);
}

//-----------------------------------------------------------------------------
// handleMouseUp()
//-----------------------------------------------------------------------------
BOOL LLFloaterModelPreview::handleMouseUp(S32 x, S32 y, MASK mask)
{
	gFocusMgr.setMouseCapture(FALSE);
	gViewerWindow->showCursor();
	return LLFloater::handleMouseUp(x, y, mask);
}

//-----------------------------------------------------------------------------
// handleHover()
//-----------------------------------------------------------------------------
BOOL LLFloaterModelPreview::handleHover	(S32 x, S32 y, MASK mask)
{
	MASK local_mask = mask & ~MASK_ALT;

	if (mModelPreview && hasMouseCapture())
	{
		if (local_mask == MASK_PAN)
		{
			// pan here
			mModelPreview->pan((F32)(x - mLastMouseX) * -0.005f, (F32)(y - mLastMouseY) * -0.005f);
		}
		else if (local_mask == MASK_ORBIT)
		{
			F32 yaw_radians = (F32)(x - mLastMouseX) * -0.01f;
			F32 pitch_radians = (F32)(y - mLastMouseY) * 0.02f;

			mModelPreview->rotate(yaw_radians, pitch_radians);
		}
		else
		{

			F32 yaw_radians = (F32)(x - mLastMouseX) * -0.01f;
			F32 zoom_amt = (F32)(y - mLastMouseY) * 0.02f;

			mModelPreview->rotate(yaw_radians, 0.f);
			mModelPreview->zoom(zoom_amt);
		}


		mModelPreview->refresh();

		LLUI::getInstance()->setMousePositionLocal(this, mLastMouseX, mLastMouseY);
	}

	if (!mPreviewRect.pointInRect(x, y) || !mModelPreview)
	{
		return LLFloater::handleHover(x, y, mask);
	}
	else if (local_mask == MASK_ORBIT)
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLCAMERA);
	}
	else if (local_mask == MASK_PAN)
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLPAN);
	}
	else
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLZOOMIN);
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// handleScrollWheel()
//-----------------------------------------------------------------------------
BOOL LLFloaterModelPreview::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	if (mPreviewRect.pointInRect(x, y) && mModelPreview)
	{
		mModelPreview->zoom((F32)clicks * -0.2f);
		mModelPreview->refresh();
	}
    else
    {
        LLFloaterModelUploadBase::handleScrollWheel(x, y, clicks);
    }
    return TRUE;
}

/*virtual*/
void LLFloaterModelPreview::onOpen(const LLSD& key)
{
	LLModelPreview::sIgnoreLoadedCallback = false;
	requestAgentUploadPermissions();
}

/*virtual*/
void LLFloaterModelPreview::onClose(bool app_quitting)
{
	LLModelPreview::sIgnoreLoadedCallback = true;
}

//static
void LLFloaterModelPreview::onPhysicsParamCommit(LLUICtrl* ctrl, void* data)
{
	if (LLConvexDecomposition::getInstance() == NULL)
	{
		LL_INFOS() << "convex decomposition tool is a stub on this platform. cannot get decomp." << LL_ENDL;
		return;
	}

	if (sInstance)
	{
		LLCDParam* param = (LLCDParam*) data;
		std::string name(param->mName);

		LLSD value = ctrl->getValue();

		if("Retain%" == name)
		{
			value = ctrl->getValue().asReal() / RETAIN_COEFFICIENT;
		}

		sInstance->mDecompParams[name] = value;

		if (name == "Simplify Method")
		{
			bool show_retain = false;
			bool show_detail = true;

			if (ctrl->getValue().asInteger() == 0)
			{
				 show_retain = true;
				 show_detail = false;
			}

			sInstance->childSetVisible("Retain%", show_retain);
			sInstance->childSetVisible("Retain%_label", show_retain);

			sInstance->childSetVisible("Detail Scale", show_detail);
			sInstance->childSetVisible("Detail Scale label", show_detail);
		}
	}
}

//static
void LLFloaterModelPreview::onPhysicsStageExecute(LLUICtrl* ctrl, void* data)
{
	LLCDStageData* stage_data = (LLCDStageData*) data;
	std::string stage = stage_data->mName;

	if (sInstance)
	{
		if (!sInstance->mCurRequest.empty())
		{
			LL_INFOS() << "Decomposition request still pending." << LL_ENDL;
			return;
		}

		if (sInstance->mModelPreview)
		{
			for (S32 i = 0; i < sInstance->mModelPreview->mModel[LLModel::LOD_PHYSICS].size(); ++i)
			{
				LLModel* mdl = sInstance->mModelPreview->mModel[LLModel::LOD_PHYSICS][i];
				DecompRequest* request = new DecompRequest(stage, mdl);
				sInstance->mCurRequest.insert(request);
				gMeshRepo.mDecompThread->submitRequest(request);
			}
		}

		if (stage == "Decompose")
		{
			sInstance->setStatusMessage(sInstance->getString("decomposing"));
			sInstance->childSetVisible("Decompose", false);
			sInstance->childSetVisible("decompose_cancel", true);
			sInstance->childDisable("Simplify");
		}
		else if (stage == "Simplify")
		{
			sInstance->setStatusMessage(sInstance->getString("simplifying"));
			sInstance->childSetVisible("Simplify", false);
			sInstance->childSetVisible("simplify_cancel", true);
			sInstance->childDisable("Decompose");
		}
	}
}

//static
void LLFloaterModelPreview::onPhysicsBrowse(LLUICtrl* ctrl, void* userdata)
{
	sInstance->loadModel(LLModel::LOD_PHYSICS);
}

//static
void LLFloaterModelPreview::onPhysicsUseLOD(LLUICtrl* ctrl, void* userdata)
{
	S32 num_lods = 4;
	S32 which_mode;

	LLCtrlSelectionInterface* iface = sInstance->childGetSelectionInterface("physics_lod_combo");
	if (iface)
	{
		which_mode = iface->getFirstSelectedIndex();
	}
	else
	{
		LL_WARNS() << "no iface" << LL_ENDL;
		return;
	}

	if (which_mode <= 0)
	{
		LL_WARNS() << "which_mode out of range, " << which_mode << LL_ENDL;
	}

	S32 file_mode = iface->getItemCount() - 1;
	if (which_mode < file_mode)
	{
		S32 which_lod = num_lods - which_mode;
		sInstance->mModelPreview->setPhysicsFromLOD(which_lod);
	}

	LLModelPreview *model_preview = sInstance->mModelPreview;
	if (model_preview)
	{
		model_preview->refresh();
		model_preview->updateStatusMessages();
	}
}

//static 
void LLFloaterModelPreview::onCancel(LLUICtrl* ctrl, void* data)
{
	if (sInstance)
	{
		sInstance->closeFloater(false);
	}
}

//static
void LLFloaterModelPreview::onPhysicsStageCancel(LLUICtrl* ctrl, void*data)
{
	if (sInstance)
	{
		for (std::set<LLPointer<DecompRequest> >::iterator iter = sInstance->mCurRequest.begin();
			iter != sInstance->mCurRequest.end(); ++iter)
		{
		    DecompRequest* req = *iter;
		    req->mContinue = 0;
		}

		sInstance->mCurRequest.clear();

		if (sInstance->mModelPreview)
		{
			sInstance->mModelPreview->updateStatusMessages();
		}
	}
}

void LLFloaterModelPreview::initDecompControls()
{
	LLSD key;

	childSetCommitCallback("simplify_cancel", onPhysicsStageCancel, NULL);
	childSetCommitCallback("decompose_cancel", onPhysicsStageCancel, NULL);

	childSetCommitCallback("physics_lod_combo", onPhysicsUseLOD, NULL);
	childSetCommitCallback("physics_browse", onPhysicsBrowse, NULL);

	static const LLCDStageData* stage = NULL;
	static S32 stage_count = 0;

	if (!stage && LLConvexDecomposition::getInstance() != NULL)
	{
		stage_count = LLConvexDecomposition::getInstance()->getStages(&stage);
	}

	static const LLCDParam* param = NULL;
	static S32 param_count = 0;
	if (!param && LLConvexDecomposition::getInstance() != NULL)
	{
		param_count = LLConvexDecomposition::getInstance()->getParameters(&param);
	}

	for (S32 j = stage_count-1; j >= 0; --j)
	{
		LLButton* button = getChild<LLButton>(stage[j].mName);
		if (button)
		{
			button->setCommitCallback(onPhysicsStageExecute, (void*) &stage[j]);
		}

		gMeshRepo.mDecompThread->mStageID[stage[j].mName] = j;
		// protected against stub by stage_count being 0 for stub above
		LLConvexDecomposition::getInstance()->registerCallback(j, LLPhysicsDecomp::llcdCallback);

		//LL_INFOS() << "Physics decomp stage " << stage[j].mName << " (" << j << ") parameters:" << LL_ENDL;
		//LL_INFOS() << "------------------------------------" << LL_ENDL;

		for (S32 i = 0; i < param_count; ++i)
		{
			if (param[i].mStage != j)
			{
				continue;
			}

			std::string name(param[i].mName ? param[i].mName : "");
			std::string description(param[i].mDescription ? param[i].mDescription : "");

			std::string type = "unknown";

			LL_INFOS() << name << " - " << description << LL_ENDL;

			if (param[i].mType == LLCDParam::LLCD_FLOAT)
			{
				mDecompParams[param[i].mName] = LLSD(param[i].mDefault.mFloat);
				//LL_INFOS() << "Type: float, Default: " << param[i].mDefault.mFloat << LL_ENDL;


				LLUICtrl* ctrl = getChild<LLUICtrl>(name);
				if (LLSliderCtrl* slider = dynamic_cast<LLSliderCtrl*>(ctrl))
				{
					slider->setMinValue(param[i].mDetails.mRange.mLow.mFloat);
					slider->setMaxValue(param[i].mDetails.mRange.mHigh.mFloat);
					slider->setIncrement(param[i].mDetails.mRange.mDelta.mFloat);
					slider->setValue(param[i].mDefault.mFloat);
					slider->setCommitCallback(onPhysicsParamCommit, (void*) &param[i]);
				}
				else if (LLSpinCtrl* spinner = dynamic_cast<LLSpinCtrl*>(ctrl))
				{
					bool is_retain_ctrl = "Retain%" == name;
					double coefficient = is_retain_ctrl ? RETAIN_COEFFICIENT : 1.f;

					spinner->setMinValue(param[i].mDetails.mRange.mLow.mFloat * coefficient);
					spinner->setMaxValue(param[i].mDetails.mRange.mHigh.mFloat * coefficient);
					spinner->setIncrement(param[i].mDetails.mRange.mDelta.mFloat * coefficient);
					spinner->setValue(param[i].mDefault.mFloat * coefficient);
					spinner->setCommitCallback(onPhysicsParamCommit, (void*) &param[i]);
				}
				else if (LLComboBox* combo_box = dynamic_cast<LLComboBox*>(ctrl))
				{
					float min = param[i].mDetails.mRange.mLow.mFloat;
					float max = param[i].mDetails.mRange.mHigh.mFloat;
					float delta = param[i].mDetails.mRange.mDelta.mFloat;

					bool is_smooth_cb = ("Cosine%" == name);
					if (is_smooth_cb)
					{
						createSmoothComboBox(combo_box, min, max);
					}
					else
					{
						for(float value = min; value <= max; value += delta)
						{
							std::string label = llformat("%.1f", value);
							combo_box->add(label, value, ADD_BOTTOM, true);
						}
					}
					combo_box->setValue(is_smooth_cb ? 0: param[i].mDefault.mFloat);
					combo_box->setCommitCallback(onPhysicsParamCommit, (void*) &param[i]);
				}
			}
			else if (param[i].mType == LLCDParam::LLCD_INTEGER)
			{
				mDecompParams[param[i].mName] = LLSD(param[i].mDefault.mIntOrEnumValue);
				//LL_INFOS() << "Type: integer, Default: " << param[i].mDefault.mIntOrEnumValue << LL_ENDL;


				LLUICtrl* ctrl = getChild<LLUICtrl>(name);
				if (LLSliderCtrl* slider = dynamic_cast<LLSliderCtrl*>(ctrl))
				{
					slider->setMinValue(param[i].mDetails.mRange.mLow.mIntOrEnumValue);
					slider->setMaxValue(param[i].mDetails.mRange.mHigh.mIntOrEnumValue);
					slider->setIncrement(param[i].mDetails.mRange.mDelta.mIntOrEnumValue);
					slider->setValue(param[i].mDefault.mIntOrEnumValue);
					slider->setCommitCallback(onPhysicsParamCommit, (void*) &param[i]);
				}
				else if (LLComboBox* combo_box = dynamic_cast<LLComboBox*>(ctrl))
				{
					for(int k = param[i].mDetails.mRange.mLow.mIntOrEnumValue; k<=param[i].mDetails.mRange.mHigh.mIntOrEnumValue; k+=param[i].mDetails.mRange.mDelta.mIntOrEnumValue)
					{
						std::string name = llformat("%.1d", k);
						combo_box->add(name, k, ADD_BOTTOM, true);
					}
					combo_box->setValue(param[i].mDefault.mIntOrEnumValue);
					combo_box->setCommitCallback(onPhysicsParamCommit, (void*) &param[i]);
				}
			}
			else if (param[i].mType == LLCDParam::LLCD_BOOLEAN)
			{
				mDecompParams[param[i].mName] = LLSD(param[i].mDefault.mBool);
				//LL_INFOS() << "Type: boolean, Default: " << (param[i].mDefault.mBool ? "True" : "False") << LL_ENDL;

				LLCheckBoxCtrl* check_box = getChild<LLCheckBoxCtrl>(name);
				if (check_box)
				{
					check_box->setValue(param[i].mDefault.mBool);
					check_box->setCommitCallback(onPhysicsParamCommit, (void*) &param[i]);
				}
			}
			else if (param[i].mType == LLCDParam::LLCD_ENUM)
			{
				mDecompParams[param[i].mName] = LLSD(param[i].mDefault.mIntOrEnumValue);
				//LL_INFOS() << "Type: enum, Default: " << param[i].mDefault.mIntOrEnumValue << LL_ENDL;

				{ //plug into combo box

					//LL_INFOS() << "Accepted values: " << LL_ENDL;
					LLComboBox* combo_box = getChild<LLComboBox>(name);
					for (S32 k = 0; k < param[i].mDetails.mEnumValues.mNumEnums; ++k)
					{
						//LL_INFOS() << param[i].mDetails.mEnumValues.mEnumsArray[k].mValue
						//	<< " - " << param[i].mDetails.mEnumValues.mEnumsArray[k].mName << LL_ENDL;

						std::string name(param[i].mDetails.mEnumValues.mEnumsArray[k].mName);
						std::string localized_name;
						bool is_localized = LLTrans::findString(localized_name, name);

						combo_box->add(is_localized ? localized_name : name,
							LLSD::Integer(param[i].mDetails.mEnumValues.mEnumsArray[k].mValue));
					}
					combo_box->setValue(param[i].mDefault.mIntOrEnumValue);
					combo_box->setCommitCallback(onPhysicsParamCommit, (void*) &param[i]);
				}

				//LL_INFOS() << "----" << LL_ENDL;
			}
			//LL_INFOS() << "-----------------------------" << LL_ENDL;
		}
	}
	mDefaultDecompParams = mDecompParams;
	childSetCommitCallback("physics_explode", LLFloaterModelPreview::onExplodeCommit, this);
}

void LLFloaterModelPreview::createSmoothComboBox(LLComboBox* combo_box, float min, float max)
{
	float delta = (max - min) / SMOOTH_VALUES_NUMBER;
	int ilabel = 0;

	combo_box->add("0 (none)", ADD_BOTTOM, true);

	for(float value = min + delta; value < max; value += delta)
	{
		std::string label = (++ilabel == SMOOTH_VALUES_NUMBER) ? "10 (max)" : llformat("%.1d", ilabel);
		combo_box->add(label, value, ADD_BOTTOM, true);
	}


}

//-----------------------------------------------------------------------------
// onMouseCaptureLost()
//-----------------------------------------------------------------------------
// static
void LLFloaterModelPreview::onMouseCaptureLostModelPreview(LLMouseHandler* handler)
{
	gViewerWindow->showCursor();
}

//-----------------------------------------------------------------------------
// addStringToLog()
//-----------------------------------------------------------------------------
//static
void LLFloaterModelPreview::addStringToLog(const std::string& message, const LLSD& args, bool flash, S32 lod)
{
    if (sInstance && sInstance->hasString(message))
    {
        std::string str;
        switch (lod)
        {
        case LLModel::LOD_IMPOSTOR: str = "LOD0 "; break;
        case LLModel::LOD_LOW:      str = "LOD1 "; break;
        case LLModel::LOD_MEDIUM:   str = "LOD2 "; break;
        case LLModel::LOD_PHYSICS:  str = "PHYS "; break;
        case LLModel::LOD_HIGH:     str = "LOD3 ";   break;
        default: break;
        }
        
        LLStringUtil::format_map_t args_msg;
        LLSD::map_const_iterator iter = args.beginMap();
        LLSD::map_const_iterator end = args.endMap();
        for (; iter != end; ++iter)
        {
            args_msg[iter->first] = iter->second.asString();
        }
        str += sInstance->getString(message, args_msg);
        sInstance->addStringToLogTab(str, flash);
    }
}

// static
void LLFloaterModelPreview::addStringToLog(const std::string& str, bool flash)
{
    if (sInstance)
    {
        sInstance->addStringToLogTab(str, flash);
    }
}

// static
void LLFloaterModelPreview::addStringToLog(const std::ostringstream& strm, bool flash)
{
    if (sInstance)
    {
        sInstance->addStringToLogTab(strm.str(), flash);
    }
}

void LLFloaterModelPreview::clearAvatarTab()
{
    LLPanel *panel = mTabContainer->getPanelByName("rigging_panel");
    LLScrollListCtrl *joints_list = panel->getChild<LLScrollListCtrl>("joints_list");
    joints_list->deleteAllItems();
    LLScrollListCtrl *joints_pos = panel->getChild<LLScrollListCtrl>("pos_overrides_list");
    joints_pos->deleteAllItems();    mSelectedJointName.clear();

    for (U32 i = 0; i < LLModel::NUM_LODS; ++i)
    {
        mJointOverrides[i].clear();
    }

    LLTextBox *joint_total_descr = panel->getChild<LLTextBox>("conflicts_description");
    joint_total_descr->setTextArg("[CONFLICTS]", llformat("%d", 0));
    joint_total_descr->setTextArg("[JOINTS_COUNT]", llformat("%d", 0));


    LLTextBox *joint_pos_descr = panel->getChild<LLTextBox>("pos_overrides_descr");
    joint_pos_descr->setTextArg("[JOINT]", std::string("mPelvis")); // Might be better to hide it
}

void LLFloaterModelPreview::updateAvatarTab(bool highlight_overrides)
{
    S32 display_lod = mModelPreview->mPreviewLOD;
    if (mModelPreview->mModel[display_lod].empty())
    {
        mSelectedJointName.clear();
        return;
    }

    // Joints will be listed as long as they are listed in mAlternateBindMatrix
    // even if they are for some reason identical to defaults.
    // Todo: Are overrides always identical for all lods? They normally are, but there might be situations where they aren't.
    if (mJointOverrides[display_lod].empty())
    {
        // populate map
        for (LLModelLoader::scene::iterator iter = mModelPreview->mScene[display_lod].begin(); iter != mModelPreview->mScene[display_lod].end(); ++iter)
        {
            for (LLModelLoader::model_instance_list::iterator model_iter = iter->second.begin(); model_iter != iter->second.end(); ++model_iter)
            {
                LLModelInstance& instance = *model_iter;
                LLModel* model = instance.mModel;
                const LLMeshSkinInfo *skin = &model->mSkinInfo;
                U32 joint_count = LLSkinningUtil::getMeshJointCount(skin);
                U32 bind_count = highlight_overrides ? skin->mAlternateBindMatrix.size() : 0; // simply do not include overrides if data is not needed
                if (bind_count > 0 && bind_count != joint_count)
                {
                    std::ostringstream out;
                    out << "Invalid joint overrides for model " << model->getName();
                    out << ". Amount of joints " << joint_count;
                    out << ", is different from amount of overrides " << bind_count;
                    LL_INFOS() << out.str() << LL_ENDL;
                    addStringToLog(out.str(), true);
                    // Disable overrides for this model
                    bind_count = 0;
                }
                if (bind_count > 0)
                {
                    for (U32 j = 0; j < joint_count; ++j)
                    {
                        const LLVector3& joint_pos = skin->mAlternateBindMatrix[j].getTranslation();
                        LLJointOverrideData &data = mJointOverrides[display_lod][skin->mJointNames[j]];

                        LLJoint* pJoint = LLModelPreview::lookupJointByName(skin->mJointNames[j], mModelPreview);
                        if (pJoint)
                        {
                            // see how voavatar uses aboveJointPosThreshold
                            if (pJoint->aboveJointPosThreshold(joint_pos))
                            {
                                // valid override
                                if (data.mPosOverrides.size() > 0
                                    && (data.mPosOverrides.begin()->second - joint_pos).lengthSquared() > (LL_JOINT_TRESHOLD_POS_OFFSET * LL_JOINT_TRESHOLD_POS_OFFSET))
                                {
                                    // File contains multiple meshes with conflicting joint offsets
                                    // preview may be incorrect, upload result might wary (depends onto
                                    // mesh_id that hasn't been generated yet).
                                    data.mHasConflicts = true;
                                }
                                data.mPosOverrides[model->getName()] = joint_pos;
                            }
                            else
                            {
                                // default value, it won't be accounted for by avatar
                                data.mModelsNoOverrides.insert(model->getName());
                            }
                        }
                    }
                }
                else
                {
                    for (U32 j = 0; j < joint_count; ++j)
                    {
                        LLJointOverrideData &data = mJointOverrides[display_lod][skin->mJointNames[j]];
                        data.mModelsNoOverrides.insert(model->getName());
                    }
                }
            }
        }
    }

    LLPanel *panel = mTabContainer->getPanelByName("rigging_panel");
    LLScrollListCtrl *joints_list = panel->getChild<LLScrollListCtrl>("joints_list");

    if (joints_list->isEmpty())
    {
        // Populate table

        std::map<std::string, std::string> joint_alias_map;
        mModelPreview->getJointAliases(joint_alias_map);

        S32 conflicts = 0;
        joint_override_data_map_t::iterator joint_iter = mJointOverrides[display_lod].begin();
        joint_override_data_map_t::iterator joint_end = mJointOverrides[display_lod].end();
        while (joint_iter != joint_end)
        {
            const std::string& listName = joint_iter->first;

            LLScrollListItem::Params item_params;
            item_params.value(listName);

            LLScrollListCell::Params cell_params;
            cell_params.font = LLFontGL::getFontSansSerif();
            cell_params.value = listName;
            if (joint_alias_map.find(listName) == joint_alias_map.end())
            {
                // Missing names
                cell_params.color = LLColor4::red;
            }
            if (joint_iter->second.mHasConflicts)
            {
                // Conflicts
                cell_params.color = LLColor4::orange;
                conflicts++;
            }
            if (highlight_overrides && joint_iter->second.mPosOverrides.size() > 0)
            {
                cell_params.font.style = "BOLD";
            }

            item_params.columns.add(cell_params);

            joints_list->addRow(item_params, ADD_BOTTOM);
            joint_iter++;
        }
        joints_list->selectFirstItem();
        LLScrollListItem *selected = joints_list->getFirstSelected();
        if (selected)
        {
            mSelectedJointName = selected->getValue().asString();
        }

        LLTextBox *joint_conf_descr = panel->getChild<LLTextBox>("conflicts_description");
        joint_conf_descr->setTextArg("[CONFLICTS]", llformat("%d", conflicts));
        joint_conf_descr->setTextArg("[JOINTS_COUNT]", llformat("%d", mJointOverrides[display_lod].size()));
    }
}

//-----------------------------------------------------------------------------
// addStringToLogTab()
//-----------------------------------------------------------------------------
void LLFloaterModelPreview::addStringToLogTab(const std::string& str, bool flash)
{
    if (str.empty())
    {
        return;
    }

    LLWString text = utf8str_to_wstring(str);
    S32 add_text_len = text.length() + 1; // newline
    S32 editor_max_len = mUploadLogText->getMaxTextLength();
    if (add_text_len > editor_max_len)
    {
        return;
    }

    // Make sure we have space for new string
    S32 editor_text_len = mUploadLogText->getLength();
    if (editor_max_len < (editor_text_len + add_text_len)
        && mUploadLogText->getLineCount() <= 0)
    {
        mUploadLogText->getTextBoundingRect();// forces a reflow() to fix line count
    }
    while (editor_max_len < (editor_text_len + add_text_len))
    {
        S32 shift = mUploadLogText->removeFirstLine();
        if (shift > 0)
        {
            // removed a line
            editor_text_len -= shift;
        }
        else
        {
            //nothing to remove?
            LL_WARNS() << "Failed to clear log lines" << LL_ENDL;
            break;
        }
    }

    mUploadLogText->appendText(str, true);

    if (flash)
    {
        LLPanel* panel = mTabContainer->getPanelByName("logs_panel");
        if (mTabContainer->getCurrentPanel() != panel)
        {
            mTabContainer->setTabPanelFlashing(panel, true);
        }
    }
}

void LLFloaterModelPreview::setDetails(F32 x, F32 y, F32 z, F32 streaming_cost, F32 physics_cost)
{
	assert_main_thread();
	childSetTextArg("import_dimensions", "[X]", llformat("%.3f", x));
	childSetTextArg("import_dimensions", "[Y]", llformat("%.3f", y));
	childSetTextArg("import_dimensions", "[Z]", llformat("%.3f", z));
}

void LLFloaterModelPreview::setPreviewLOD(S32 lod)
{
	if (mModelPreview)
	{
		mModelPreview->setPreviewLOD(lod);
	}
}

void LLFloaterModelPreview::onBrowseLOD(S32 lod)
{
	assert_main_thread();

	loadModel(lod);
}

//static
void LLFloaterModelPreview::onReset(void* user_data)
{
	assert_main_thread();


	LLFloaterModelPreview* fmp = (LLFloaterModelPreview*) user_data;
	fmp->childDisable("reset_btn");
	fmp->clearLogTab();
	fmp->clearAvatarTab();
	LLModelPreview* mp = fmp->mModelPreview;
	std::string filename = mp->mLODFile[LLModel::LOD_HIGH]; 

	fmp->resetDisplayOptions();
	fmp->resetUploadOptions();
	//reset model preview
	fmp->initModelPreview();

	mp = fmp->mModelPreview;
	mp->loadModel(filename,LLModel::LOD_HIGH,true);
}

//static
void LLFloaterModelPreview::onUpload(void* user_data)
{
	assert_main_thread();

	LLFloaterModelPreview* mp = (LLFloaterModelPreview*) user_data;
	mp->clearLogTab();

	mp->mUploadBtn->setEnabled(false);

	mp->mModelPreview->rebuildUploadData();

	bool upload_skinweights = mp->childGetValue("upload_skin").asBoolean();
	bool upload_joint_positions = mp->childGetValue("upload_joints").asBoolean();
    bool lock_scale_if_joint_position = mp->childGetValue("lock_scale_if_joint_position").asBoolean();

	if (gSavedSettings.getBOOL("MeshImportUseSLM"))
	{
        mp->mModelPreview->saveUploadData(upload_skinweights, upload_joint_positions, lock_scale_if_joint_position);
    }

	gMeshRepo.uploadModel(mp->mModelPreview->mUploadData, mp->mModelPreview->mPreviewScale,
						  mp->childGetValue("upload_textures").asBoolean(), 
                          upload_skinweights, upload_joint_positions, lock_scale_if_joint_position, 
                          mp->mUploadModelUrl,
						  true, LLHandle<LLWholeModelFeeObserver>(), mp->getWholeModelUploadObserverHandle());
}


void LLFloaterModelPreview::refresh()
{
	sInstance->toggleCalculateButton(true);
	sInstance->mModelPreview->mDirty = true;
}

LLFloaterModelPreview::DecompRequest::DecompRequest(const std::string& stage, LLModel* mdl)
{
	mStage = stage;
	mContinue = 1;
	mModel = mdl;
	mDecompID = &mdl->mDecompID;
	mParams = sInstance->mDecompParams;

	//copy out positions and indices
	assignData(mdl) ;	
}

void LLFloaterModelPreview::setCtrlLoadFromFile(S32 lod)
{
    if (lod == LLModel::LOD_PHYSICS)
    {
        LLComboBox* lod_combo = findChild<LLComboBox>("physics_lod_combo");
        if (lod_combo)
        {
            lod_combo->setCurrentByIndex(5);
        }
    }
    else
    {
        LLComboBox* lod_combo = findChild<LLComboBox>("lod_source_" + lod_name[lod]);
        if (lod_combo)
        {
            lod_combo->setCurrentByIndex(0);
        }
    }
}

void LLFloaterModelPreview::setStatusMessage(const std::string& msg)
{
	LLMutexLock lock(mStatusLock);
	mStatusMessage = msg;
}

void LLFloaterModelPreview::toggleCalculateButton()
{
	toggleCalculateButton(true);
}

void LLFloaterModelPreview::modelUpdated(bool calculate_visible)
{
    mModelPhysicsFee.clear();
    toggleCalculateButton(calculate_visible);
}

void LLFloaterModelPreview::toggleCalculateButton(bool visible)
{
	mCalculateBtn->setVisible(visible);

	bool uploadingSkin		     = childGetValue("upload_skin").asBoolean();
	bool uploadingJointPositions = childGetValue("upload_joints").asBoolean();
	if ( uploadingSkin )
	{
		//Disable the calculate button *if* the rig is invalid - which is determined during the critiquing process
		if ( uploadingJointPositions && !mModelPreview->isRigValidForJointPositionUpload() )
		{
			mCalculateBtn->setVisible( false );
		}
	}
	
	mUploadBtn->setVisible(!visible);
	mUploadBtn->setEnabled(isModelUploadAllowed());

	if (visible)
	{
		std::string tbd = getString("tbd");
		childSetTextArg("prim_weight", "[EQ]", tbd);
		childSetTextArg("download_weight", "[ST]", tbd);
		childSetTextArg("server_weight", "[SIM]", tbd);
		childSetTextArg("physics_weight", "[PH]", tbd);
		if (!mModelPhysicsFee.isMap() || mModelPhysicsFee.emptyMap())
		{
			childSetTextArg("upload_fee", "[FEE]", tbd);
		}
		std::string dashes = hasString("--") ? getString("--") : "--";
		childSetTextArg("price_breakdown", "[STREAMING]", dashes);
		childSetTextArg("price_breakdown", "[PHYSICS]", dashes);
		childSetTextArg("price_breakdown", "[INSTANCES]", dashes);
		childSetTextArg("price_breakdown", "[TEXTURES]", dashes);
		childSetTextArg("price_breakdown", "[MODEL]", dashes);
		childSetTextArg("physics_breakdown", "[PCH]", dashes);
		childSetTextArg("physics_breakdown", "[PM]", dashes);
		childSetTextArg("physics_breakdown", "[PHU]", dashes);
	}
}

void LLFloaterModelPreview::onLoDSourceCommit(S32 lod)
{
	mModelPreview->updateLodControls(lod);
	refresh();

	LLComboBox* lod_source_combo = getChild<LLComboBox>("lod_source_" + lod_name[lod]);
	if (lod_source_combo->getCurrentIndex() == LLModelPreview::GENERATE)
	{ //rebuild LoD to update triangle counts
		onLODParamCommit(lod, true);
	}
}

void LLFloaterModelPreview::resetDisplayOptions()
{
	std::map<std::string,bool>::iterator option_it = mModelPreview->mViewOption.begin();

	for(;option_it != mModelPreview->mViewOption.end(); ++option_it)
	{
		LLUICtrl* ctrl = getChild<LLUICtrl>(option_it->first);
		ctrl->setValue(false);
	}
}

void LLFloaterModelPreview::resetUploadOptions()
{
	childSetValue("import_scale", 1);
	childSetValue("pelvis_offset", 0);
	childSetValue("physics_explode", 0);
	childSetValue("physics_file", "");
	childSetVisible("Retain%", false);
	childSetVisible("Retain%_label", false);
	childSetVisible("Detail Scale", true);
	childSetVisible("Detail Scale label", true);

	getChild<LLComboBox>("lod_source_" + lod_name[NUM_LOD - 1])->setCurrentByIndex(LLModelPreview::LOD_FROM_FILE);
	for (S32 lod = 0; lod < NUM_LOD - 1; ++lod)
	{
		getChild<LLComboBox>("lod_source_" + lod_name[lod])->setCurrentByIndex(LLModelPreview::GENERATE);
		childSetValue("lod_file_" + lod_name[lod], "");
	}

	for(auto& p : mDefaultDecompParams)
	{
		std::string ctrl_name(p.first);
		LLUICtrl* ctrl = getChild<LLUICtrl>(ctrl_name);
		if (ctrl)
		{
			ctrl->setValue(p.second);
		}
	}
	getChild<LLComboBox>("physics_lod_combo")->setCurrentByIndex(0);
	getChild<LLComboBox>("Cosine%")->setCurrentByIndex(0);
}

void LLFloaterModelPreview::clearLogTab()
{
    mUploadLogText->clear();
    LLPanel* panel = mTabContainer->getPanelByName("logs_panel");
    mTabContainer->setTabPanelFlashing(panel, false);
}

void LLFloaterModelPreview::onModelPhysicsFeeReceived(const LLSD& result, std::string upload_url)
{
	mModelPhysicsFee = result;
	mModelPhysicsFee["url"] = upload_url;

	doOnIdleOneTime(boost::bind(&LLFloaterModelPreview::handleModelPhysicsFeeReceived,this));
}

void LLFloaterModelPreview::handleModelPhysicsFeeReceived()
{
	const LLSD& result = mModelPhysicsFee;
	mUploadModelUrl = result["url"].asString();

	childSetTextArg("prim_weight", "[EQ]", llformat("%0.3f", result["resource_cost"].asReal()));
	childSetTextArg("download_weight", "[ST]", llformat("%0.3f", result["model_streaming_cost"].asReal()));
	childSetTextArg("server_weight", "[SIM]", llformat("%0.3f", result["simulation_cost"].asReal()));
	childSetTextArg("physics_weight", "[PH]", llformat("%0.3f", result["physics_cost"].asReal()));
	childSetTextArg("upload_fee", "[FEE]", llformat("%d", result["upload_price"].asInteger()));
	childSetTextArg("price_breakdown", "[STREAMING]", llformat("%d", result["upload_price_breakdown"]["mesh_streaming"].asInteger()));
	childSetTextArg("price_breakdown", "[PHYSICS]", llformat("%d", result["upload_price_breakdown"]["mesh_physics"].asInteger()));
	childSetTextArg("price_breakdown", "[INSTANCES]", llformat("%d", result["upload_price_breakdown"]["mesh_instance"].asInteger()));
	childSetTextArg("price_breakdown", "[TEXTURES]", llformat("%d", result["upload_price_breakdown"]["texture"].asInteger()));
	childSetTextArg("price_breakdown", "[MODEL]", llformat("%d", result["upload_price_breakdown"]["model"].asInteger()));

	childSetTextArg("physics_breakdown", "[PCH]", llformat("%0.3f", result["model_physics_cost"]["hull"].asReal()));
	childSetTextArg("physics_breakdown", "[PM]", llformat("%0.3f", result["model_physics_cost"]["mesh"].asReal()));
	childSetTextArg("physics_breakdown", "[PHU]", llformat("%0.3f", result["model_physics_cost"]["decomposition"].asReal()));
	childSetTextArg("streaming_breakdown", "[STR_TOTAL]", llformat("%d", result["streaming_cost"].asInteger()));
	childSetTextArg("streaming_breakdown", "[STR_HIGH]", llformat("%d", result["streaming_params"]["high_lod"].asInteger()));
	childSetTextArg("streaming_breakdown", "[STR_MED]", llformat("%d", result["streaming_params"]["medium_lod"].asInteger()));
	childSetTextArg("streaming_breakdown", "[STR_LOW]", llformat("%d", result["streaming_params"]["low_lod"].asInteger()));
	childSetTextArg("streaming_breakdown", "[STR_LOWEST]", llformat("%d", result["streaming_params"]["lowest_lod"].asInteger()));

	childSetVisible("upload_fee", true);
	childSetVisible("price_breakdown", true);
	mUploadBtn->setEnabled(isModelUploadAllowed());
}

void LLFloaterModelPreview::setModelPhysicsFeeErrorStatus(S32 status, const std::string& reason, const LLSD& result)
{
	std::ostringstream out;
	out << "LLFloaterModelPreview::setModelPhysicsFeeErrorStatus(" << status;
	out << " : " << reason << ")";
	LL_WARNS() << out.str() << LL_ENDL;
	LLFloaterModelPreview::addStringToLog(out, false);
	doOnIdleOneTime(boost::bind(&LLFloaterModelPreview::toggleCalculateButton, this, true));

    if (result.has("upload_price"))
    {
        mModelPhysicsFee = result;
        childSetTextArg("upload_fee", "[FEE]", llformat("%d", result["upload_price"].asInteger()));
        childSetVisible("upload_fee", true);
    }
    else
    {
        mModelPhysicsFee.clear();
    }
}

/*virtual*/ 
void LLFloaterModelPreview::onModelUploadSuccess()
{
	assert_main_thread();
	closeFloater(false);
}

/*virtual*/ 
void LLFloaterModelPreview::onModelUploadFailure()
{
	assert_main_thread();
	toggleCalculateButton(true);
	mUploadBtn->setEnabled(true);
}

bool LLFloaterModelPreview::isModelUploadAllowed()
{
	bool allow_upload = mHasUploadPerm && !mUploadModelUrl.empty();
	if (mModelPreview)
	{
		allow_upload &= mModelPreview->mModelNoErrors;
	}
	return allow_upload;
}

S32 LLFloaterModelPreview::DecompRequest::statusCallback(const char* status, S32 p1, S32 p2)
{
	if (mContinue)
	{
		setStatusMessage(llformat("%s: %d/%d", status, p1, p2));
		if (LLFloaterModelPreview::sInstance)
		{
			LLFloaterModelPreview::sInstance->setStatusMessage(mStatusMessage);
		}
	}

	return mContinue;
}

void LLFloaterModelPreview::DecompRequest::completed()
{ //called from the main thread
	if (mContinue)
	{
		mModel->setConvexHullDecomposition(mHull);

		if (sInstance)
		{
			if (mContinue)
			{
				if (sInstance->mModelPreview)
				{
					sInstance->mModelPreview->mDirty = true;
					LLFloaterModelPreview::sInstance->mModelPreview->refresh();
				}
			}

			sInstance->mCurRequest.erase(this);
		}
	}
	else if (sInstance)
	{
		llassert(sInstance->mCurRequest.find(this) == sInstance->mCurRequest.end());
	}
}

void dump_llsd_to_file(const LLSD& content, std::string filename);

void LLFloaterModelPreview::onPermissionsReceived(const LLSD& result)
{
	dump_llsd_to_file(result,"perm_received.xml");
	std::string upload_status = result["mesh_upload_status"].asString();
	// BAP HACK: handle "" for case that  MeshUploadFlag cap is broken.
	mHasUploadPerm = (("" == upload_status) || ("valid" == upload_status));

    if (!mHasUploadPerm) 
    {
        LL_WARNS() << "Upload permission set to false because upload_status=\"" << upload_status << "\"" << LL_ENDL;
    }
    else if (mHasUploadPerm && mUploadModelUrl.empty())
    {
        LL_WARNS() << "Upload permission set to true but uploadModelUrl is empty!" << LL_ENDL;
    }

	// isModelUploadAllowed() includes mHasUploadPerm
	mUploadBtn->setEnabled(isModelUploadAllowed());
	getChild<LLTextBox>("warning_title")->setVisible(!mHasUploadPerm);
	getChild<LLTextBox>("warning_message")->setVisible(!mHasUploadPerm);
}

void LLFloaterModelPreview::setPermissonsErrorStatus(S32 status, const std::string& reason)
{
	LL_WARNS() << "LLFloaterModelPreview::setPermissonsErrorStatus(" << status << " : " << reason << ")" << LL_ENDL;

	LLNotificationsUtil::add("MeshUploadPermError");
}

bool LLFloaterModelPreview::isModelLoading()
{
	if(mModelPreview)
	{
		return mModelPreview->mLoading;
	}
	return false;
}

