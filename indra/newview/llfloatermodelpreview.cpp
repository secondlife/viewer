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
#include "lldaeloader.h"

#include "llfloatermodelpreview.h"

#include "llfilepicker.h"
#include "llimagebmp.h"
#include "llimagetga.h"
#include "llimagejpeg.h"
#include "llimagepng.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "lldatapacker.h"
#include "lldrawable.h"
#include "llrender.h"
#include "llface.h"
#include "lleconomy.h"
#include "llfocusmgr.h"
#include "llfloaterperms.h"
#include "lliconctrl.h"
#include "llmatrix4a.h"
#include "llmenubutton.h"
#include "llmeshrepository.h"
#include "llnotificationsutil.h"
#include "llsdutil_math.h"
#include "llskinningutil.h"
#include "lltextbox.h"
#include "lltoolmgr.h"
#include "llui.h"
#include "llvector4a.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "pipeline.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"
#include "llviewermenufile.h"
#include "llviewerregion.h"
#include "llviewertexturelist.h"
#include "llstring.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llradiogroup.h"
#include "llsdserialize.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "lltoggleablemenu.h"
#include "lltrans.h"
#include "llvfile.h"
#include "llvfs.h"
#include "llcallbacklist.h"
#include "llviewerobjectlist.h"
#include "llanimationstates.h"
#include "llviewernetwork.h"
#include "llviewershadermgr.h"

#include "glod/glod.h"
#include <boost/algorithm/string.hpp>

//static
S32 LLFloaterModelPreview::sUploadAmount = 10;
LLFloaterModelPreview* LLFloaterModelPreview::sInstance = NULL;

bool LLModelPreview::sIgnoreLoadedCallback = false;

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

void drawBoxOutline(const LLVector3& pos, const LLVector3& size);


std::string lod_name[NUM_LOD+1] =
{
	"lowest",
	"low",
	"medium",
	"high",
	"I went off the end of the lod_name array.  Me so smart."
};

std::string lod_triangles_name[NUM_LOD+1] =
{
	"lowest_triangles",
	"low_triangles",
	"medium_triangles",
	"high_triangles",
	"I went off the end of the lod_triangles_name array.  Me so smart."
};

std::string lod_vertices_name[NUM_LOD+1] =
{
	"lowest_vertices",
	"low_vertices",
	"medium_vertices",
	"high_vertices",
	"I went off the end of the lod_vertices_name array.  Me so smart."
};

std::string lod_status_name[NUM_LOD+1] =
{
	"lowest_status",
	"low_status",
	"medium_status",
	"high_status",
	"I went off the end of the lod_status_name array.  Me so smart."
};

std::string lod_icon_name[NUM_LOD+1] =
{
	"status_icon_lowest",
	"status_icon_low",
	"status_icon_medium",
	"status_icon_high",
	"I went off the end of the lod_status_name array.  Me so smart."
};

std::string lod_status_image[NUM_LOD+1] =
{
	"ModelImport_Status_Good",
	"ModelImport_Status_Warning",
	"ModelImport_Status_Error",
	"I went off the end of the lod_status_image array.  Me so smart."
};

std::string lod_label_name[NUM_LOD+1] =
{
	"lowest_label",
	"low_label",
	"medium_label",
	"high_label",
	"I went off the end of the lod_label_name array.  Me so smart."
};

BOOL stop_gloderror()
{
	GLuint error = glodGetError();

	if (error != GLOD_NO_ERROR)
	{
		LL_WARNS() << "GLOD error detected, cannot generate LOD: " << std::hex << error << LL_ENDL;
		return TRUE;
	}

	return FALSE;
}

LLViewerFetchedTexture* bindMaterialDiffuseTexture(const LLImportMaterial& material)
{
	LLViewerFetchedTexture *texture = LLViewerTextureManager::getFetchedTexture(material.getDiffuseMap(), FTT_DEFAULT, TRUE, LLGLTexture::BOOST_PREVIEW);

	if (texture)
	{
		if (texture->getDiscardLevel() > -1)
		{
			gGL.getTexUnit(0)->bind(texture, true);
			return texture;
		}
	}

	return NULL;
}

std::string stripSuffix(std::string name)
{
	if ((name.find("_LOD") != -1) || (name.find("_PHYS") != -1))
	{
		return name.substr(0, name.rfind('_'));
	}
	return name;
}

LLMeshFilePicker::LLMeshFilePicker(LLModelPreview* mp, S32 lod)
: LLFilePickerThread(LLFilePicker::FFLOAD_COLLADA)
	{
		mMP = mp;
		mLOD = lod;
	}

void LLMeshFilePicker::notify(const std::string& filename)
{
	mMP->loadModel(mFile, mLOD);
}

void FindModel(LLModelLoader::scene& scene, const std::string& name_to_match, LLModel*& baseModelOut, LLMatrix4& matOut)
{
    LLModelLoader::scene::iterator base_iter = scene.begin();
    bool found = false;
    while (!found && (base_iter != scene.end()))
    {
        matOut = base_iter->first;

        LLModelLoader::model_instance_list::iterator base_instance_iter = base_iter->second.begin();
        while (!found && (base_instance_iter != base_iter->second.end()))
        {
		    LLModelInstance& base_instance = *base_instance_iter++;					    		    
            LLModel* base_model = base_instance.mModel;
         
            if (base_model && (base_model->mLabel == name_to_match))
            {
                baseModelOut = base_model;
                return;
            }
        }
        base_iter++;
    }
}

//-----------------------------------------------------------------------------
// LLFloaterModelPreview()
//-----------------------------------------------------------------------------
LLFloaterModelPreview::LLFloaterModelPreview(const LLSD& key) :
LLFloaterModelUploadBase(key),
mUploadBtn(NULL),
mCalculateBtn(NULL)
{
	sInstance = this;
	mLastMouseX = 0;
	mLastMouseY = 0;
	mStatusLock = new LLMutex(NULL);
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

	childSetCommitCallback("upload_skin", boost::bind(&LLFloaterModelPreview::toggleCalculateButton, this), NULL);
	childSetCommitCallback("upload_joints", boost::bind(&LLFloaterModelPreview::toggleCalculateButton, this), NULL);
	childSetCommitCallback("lock_scale_if_joint_position", boost::bind(&LLFloaterModelPreview::toggleCalculateButton, this), NULL);
	childSetCommitCallback("upload_textures", boost::bind(&LLFloaterModelPreview::toggleCalculateButton, this), NULL);

	childSetTextArg("status", "[STATUS]", getString("status_idle"));

	childSetAction("ok_btn", onUpload, this);
	childDisable("ok_btn");

	childSetAction("reset_btn", onReset, this);

	childSetCommitCallback("preview_lod_combo", onPreviewLODCommit, this);

	childSetCommitCallback("upload_skin", onUploadSkinCommit, this);
	childSetCommitCallback("upload_joints", onUploadJointsCommit, this);
	childSetCommitCallback("lock_scale_if_joint_position", onUploadJointsCommit, this);

	childSetCommitCallback("import_scale", onImportScaleCommit, this);
	childSetCommitCallback("pelvis_offset", onPelvisOffsetCommit, this);

	getChild<LLCheckBoxCtrl>("show_edges")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onViewOptionChecked, this, _1));
	getChild<LLCheckBoxCtrl>("show_physics")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onViewOptionChecked, this, _1));
	getChild<LLCheckBoxCtrl>("show_textures")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onViewOptionChecked, this, _1));
	getChild<LLCheckBoxCtrl>("show_skin_weight")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onViewOptionChecked, this, _1));
	getChild<LLCheckBoxCtrl>("show_joint_positions")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onViewOptionChecked, this, _1));

	childDisable("upload_skin");
	childDisable("upload_joints");
	childDisable("lock_scale_if_joint_position");

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

	mModelPreview = new LLModelPreview(512, 512, this );
	mModelPreview->setPreviewTarget(16.f);
	mModelPreview->setDetailsCallback(boost::bind(&LLFloaterModelPreview::setDetails, this, _1, _2, _3, _4, _5));
	mModelPreview->setModelUpdatedCallback(boost::bind(&LLFloaterModelPreview::toggleCalculateButton, this, _1));
}

void LLFloaterModelPreview::onViewOptionChecked(LLUICtrl* ctrl)
{
	if (mModelPreview)
	{
		mModelPreview->mViewOption[ctrl->getName()] = !mModelPreview->mViewOption[ctrl->getName()];
		
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
	mModelPreview->rebuildUploadData();

	bool upload_skinweights = childGetValue("upload_skin").asBoolean();
	bool upload_joint_positions = childGetValue("upload_joints").asBoolean();
    bool lock_scale_if_joint_position = childGetValue("lock_scale_if_joint_position").asBoolean();

    if (upload_joint_positions)
    {
        // Diagnostic message showing list of joints for which joint offsets are defined.
        // FIXME - given time, would be much better to put this in the UI, in updateStatusMessages().
		mModelPreview->getPreviewAvatar()->showAttachmentOverrides();
    }

	mUploadModelUrl.clear();

	gMeshRepo.uploadModel(mModelPreview->mUploadData, mModelPreview->mPreviewScale,
                          childGetValue("upload_textures").asBoolean(), 
                          upload_skinweights, upload_joint_positions, lock_scale_if_joint_position,
                          mUploadModelUrl, false,
						  getWholeModelFeeObserverHandle());

	toggleCalculateButton(false);
	mUploadBtn->setEnabled(false);
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
void LLFloaterModelPreview::onUploadJointsCommit(LLUICtrl*,void* userdata)
{
	LLFloaterModelPreview *fp =(LLFloaterModelPreview *)userdata;

	if (!fp->mModelPreview)
	{
		return;
	}

	fp->mModelPreview->refresh();
}

//static
void LLFloaterModelPreview::onUploadSkinCommit(LLUICtrl*,void* userdata)
{
	LLFloaterModelPreview *fp =(LLFloaterModelPreview *)userdata;

	if (!fp->mModelPreview)
	{
		return;
	}
	fp->mModelPreview->refresh();
	fp->mModelPreview->resetPreviewTarget();
	fp->mModelPreview->clearBuffers();
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
		{
			childSetTextArg("status", "[STATUS]", getString("status_idle"));
		}
	}

	childSetTextArg("prim_cost", "[PRIM_COST]", llformat("%d", mModelPreview->mResourceCost));
	childSetTextArg("description_label", "[TEXTURES]", llformat("%d", mModelPreview->mTextureSet.size()));

    if (mModelPreview->lodsReady())
	{
		gGL.color3f(1.f, 1.f, 1.f);

		gGL.getTexUnit(0)->bind(mModelPreview);


		LLView* preview_panel = getChild<LLView>("preview_panel");

		LLRect rect = preview_panel->getRect();
		if (rect != mPreviewRect)
		{
			mModelPreview->refresh();
			mPreviewRect = preview_panel->getRect();
		}

		gGL.begin( LLRender::QUADS );
		{
			gGL.texCoord2f(0.f, 1.f);
			gGL.vertex2i(mPreviewRect.mLeft, mPreviewRect.mTop-1);
			gGL.texCoord2f(0.f, 0.f);
			gGL.vertex2i(mPreviewRect.mLeft, mPreviewRect.mBottom);
			gGL.texCoord2f(1.f, 0.f);
			gGL.vertex2i(mPreviewRect.mRight-1, mPreviewRect.mBottom);
			gGL.texCoord2f(1.f, 1.f);
			gGL.vertex2i(mPreviewRect.mRight-1, mPreviewRect.mTop-1);
		}
		gGL.end();

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
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

		LLUI::setMousePositionLocal(this, mLastMouseX, mLastMouseY);
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

					if ("Cosine%" == name)
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
						combo_box->setValue(param[i].mDefault.mFloat);

					}

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
// LLModelPreview
//-----------------------------------------------------------------------------

LLModelPreview::LLModelPreview(S32 width, S32 height, LLFloater* fmp)
: LLViewerDynamicTexture(width, height, 3, ORDER_MIDDLE, FALSE), LLMutex(NULL)
, mLodsQuery()
, mLodsWithParsingError()
, mPelvisZOffset( 0.0f )
, mLegacyRigValid( false )
, mRigValidJointUpload( false )
, mPhysicsSearchLOD( LLModel::LOD_PHYSICS )
, mResetJoints( false )
, mModelNoErrors( true )
, mLastJointUpdate( false )
{
	mNeedsUpdate = TRUE;
	mCameraDistance = 0.f;
	mCameraYaw = 0.f;
	mCameraPitch = 0.f;
	mCameraZoom = 1.f;
	mTextureName = 0;
	mPreviewLOD = 0;
	mModelLoader = NULL;
	mMaxTriangleLimit = 0;
	mDirty = false;
	mGenLOD = false;
	mLoading = false;
	mLoadState = LLModelLoader::STARTING;
	mGroup = 0;
	mLODFrozen = false;
	mBuildShareTolerance = 0.f;
	mBuildQueueMode = GLOD_QUEUE_GREEDY;
	mBuildBorderMode = GLOD_BORDER_UNLOCK;
	mBuildOperator = GLOD_OPERATOR_EDGE_COLLAPSE;

	for (U32 i = 0; i < LLModel::NUM_LODS; ++i)
	{
		mRequestedTriangleCount[i] = 0;
		mRequestedCreaseAngle[i] = -1.f;
		mRequestedLoDMode[i] = 0;
		mRequestedErrorThreshold[i] = 0.f;
		mRequestedBuildOperator[i] = 0;
		mRequestedQueueMode[i] = 0;
		mRequestedBorderMode[i] = 0;
		mRequestedShareTolerance[i] = 0.f;
	}

	mViewOption["show_textures"] = false;

	mFMP = fmp;

	mHasPivot = false;
	mModelPivot = LLVector3( 0.0f, 0.0f, 0.0f );
	
	glodInit();

	createPreviewAvatar();
}

LLModelPreview::~LLModelPreview()
{
	// glod apparently has internal mem alignment issues that are angering
	// the heap-check code in windows, these should be hunted down in that
	// TP code, if possible
	//
	// kernel32.dll!HeapFree()  + 0x14 bytes	
	// msvcr100.dll!free(void * pBlock)  Line 51	C
	// glod.dll!glodGetGroupParameteriv()  + 0x119 bytes	
	// glod.dll!glodShutdown()  + 0x77 bytes	
	//
	//glodShutdown();
}

U32 LLModelPreview::calcResourceCost()
{
	assert_main_thread();

	rebuildUploadData();

	//Upload skin is selected BUT check to see if the joints coming in from the asset were malformed.
	if ( mFMP && mFMP->childGetValue("upload_skin").asBoolean() )
	{
		bool uploadingJointPositions = mFMP->childGetValue("upload_joints").asBoolean();
		if ( uploadingJointPositions && !isRigValidForJointPositionUpload() )
		{
			mFMP->childDisable("ok_btn");		
		}		
	}
	
	std::set<LLModel*> accounted;
	U32 num_points = 0;
	U32 num_hulls = 0;

	F32 debug_scale = mFMP ? mFMP->childGetValue("import_scale").asReal() : 1.f;
	mPelvisZOffset = mFMP ? mFMP->childGetValue("pelvis_offset").asReal() : 3.0f;
	
	if ( mFMP && mFMP->childGetValue("upload_joints").asBoolean() )
	{
		// FIXME if preview avatar ever gets reused, this fake mesh ID stuff will fail.
		// see also call to addAttachmentPosOverride.
		LLUUID fake_mesh_id;
		fake_mesh_id.generate();
		getPreviewAvatar()->addPelvisFixup( mPelvisZOffset, fake_mesh_id );
	}

	F32 streaming_cost = 0.f;
	F32 physics_cost = 0.f;
	for (U32 i = 0; i < mUploadData.size(); ++i)
	{
		LLModelInstance& instance = mUploadData[i];
		
		if (accounted.find(instance.mModel) == accounted.end())
		{
			accounted.insert(instance.mModel);

			LLModel::Decomposition& decomp =
			instance.mLOD[LLModel::LOD_PHYSICS] ?
			instance.mLOD[LLModel::LOD_PHYSICS]->mPhysics :
			instance.mModel->mPhysics;
			
			//update instance skin info for each lods pelvisZoffset 
			for ( int j=0; j<LLModel::NUM_LODS; ++j )
			{	
				if ( instance.mLOD[j] )
				{
					instance.mLOD[j]->mSkinInfo.mPelvisOffset = mPelvisZOffset;
				}
			}

			std::stringstream ostr;
			LLSD ret = LLModel::writeModel(ostr,
					   instance.mLOD[4],
					   instance.mLOD[3],
					   instance.mLOD[2],
					   instance.mLOD[1],
					   instance.mLOD[0],
					   decomp,
					   mFMP->childGetValue("upload_skin").asBoolean(),
					   mFMP->childGetValue("upload_joints").asBoolean(),
					   mFMP->childGetValue("lock_scale_if_joint_position").asBoolean(),
					   TRUE,
					   FALSE,
					   instance.mModel->mSubmodelID);
			
			num_hulls += decomp.mHull.size();
			for (U32 i = 0; i < decomp.mHull.size(); ++i)
			{
				num_points += decomp.mHull[i].size();
			}

			//calculate streaming cost
			LLMatrix4 transformation = instance.mTransform;

			LLVector3 position = LLVector3(0, 0, 0) * transformation;

			LLVector3 x_transformed = LLVector3(1, 0, 0) * transformation - position;
			LLVector3 y_transformed = LLVector3(0, 1, 0) * transformation - position;
			LLVector3 z_transformed = LLVector3(0, 0, 1) * transformation - position;
			F32 x_length = x_transformed.normalize();
			F32 y_length = y_transformed.normalize();
			F32 z_length = z_transformed.normalize();
			LLVector3 scale = LLVector3(x_length, y_length, z_length);

			F32 radius = scale.length()*0.5f*debug_scale;

			streaming_cost += LLMeshRepository::getStreamingCost(ret, radius);
		}
	}

	F32 scale = mFMP ? mFMP->childGetValue("import_scale").asReal()*2.f : 2.f;

	mDetailsSignal(mPreviewScale[0]*scale, mPreviewScale[1]*scale, mPreviewScale[2]*scale, streaming_cost, physics_cost);

	updateStatusMessages();

	return (U32) streaming_cost;
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


void LLModelPreview::rebuildUploadData()
{
	assert_main_thread();

	mUploadData.clear();
	mTextureSet.clear();

	//fill uploaddata instance vectors from scene data

	std::string requested_name = mFMP->getChild<LLUICtrl>("description_form")->getValue().asString();

	std::string metric = mFMP->getChild<LLUICtrl>("model_category_combo")->getValue().asString();

	LLSpinCtrl* scale_spinner = mFMP->getChild<LLSpinCtrl>("import_scale");

	F32 scale = scale_spinner->getValue().asReal();

	LLMatrix4 scale_mat;
	scale_mat.initScale(LLVector3(scale, scale, scale));

	F32 max_scale = 0.f;

	BOOL importerDebug = gSavedSettings.getBOOL("ImporterDebug");
	BOOL legacyMatching = gSavedSettings.getBOOL("ImporterLegacyMatching");

	for (LLModelLoader::scene::iterator iter = mBaseScene.begin(); iter != mBaseScene.end(); ++iter)
	{ //for each transform in scene
		LLMatrix4 mat		= iter->first;

		// compute position
		LLVector3 position = LLVector3(0, 0, 0) * mat;

		// compute scale
		LLVector3 x_transformed = LLVector3(1, 0, 0) * mat - position;
		LLVector3 y_transformed = LLVector3(0, 1, 0) * mat - position;
		LLVector3 z_transformed = LLVector3(0, 0, 1) * mat - position;
		F32 x_length = x_transformed.normalize();
		F32 y_length = y_transformed.normalize();
		F32 z_length = z_transformed.normalize();

		max_scale = llmax(llmax(llmax(max_scale, x_length), y_length), z_length);

		mat *= scale_mat;

		for (LLModelLoader::model_instance_list::iterator model_iter = iter->second.begin(); model_iter != iter->second.end();)
		{ //for each instance with said transform applied 
			LLModelInstance instance = *model_iter++;

			LLModel* base_model = instance.mModel;
			
			if (base_model && !requested_name.empty())
			{
				base_model->mRequestedLabel = requested_name;
				base_model->mMetric = metric;
			}

			for (int i = LLModel::NUM_LODS - 1; i >= LLModel::LOD_IMPOSTOR; i--)
			{
				LLModel* lod_model = NULL;
				if (!legacyMatching)
				{
					// Fill LOD slots by finding matching meshes by label with name extensions
					// in the appropriate scene for each LOD. This fixes all kinds of issues
					// where the indexed method below fails in spectacular fashion.
					// If you don't take the time to name your LOD and PHYS meshes
					// with the name of their corresponding mesh in the HIGH LOD,
					// then the indexed method will be attempted below.

					LLMatrix4 transform;

					std::string name_to_match = instance.mLabel;
					llassert(!name_to_match.empty());

					int extensionLOD;
					if (i != LLModel::LOD_PHYSICS || mModel[LLModel::LOD_PHYSICS].empty())
					{
						extensionLOD = i;
					}
					else
					{
						//Physics can be inherited from other LODs or loaded, so we need to adjust what extension we are searching for
						extensionLOD = mPhysicsSearchLOD;
					}

					std::string toAdd;
					switch (extensionLOD)
					{
					case LLModel::LOD_IMPOSTOR: toAdd = "_LOD0"; break;
					case LLModel::LOD_LOW:      toAdd = "_LOD1"; break;
					case LLModel::LOD_MEDIUM:   toAdd = "_LOD2"; break;
					case LLModel::LOD_PHYSICS:  toAdd = "_PHYS"; break;
					case LLModel::LOD_HIGH:                      break;
					}

					if (name_to_match.find(toAdd) == -1)
					{
						name_to_match += toAdd;
					}

					FindModel(mScene[i], name_to_match, lod_model, transform);

					if (!lod_model && i != LLModel::LOD_PHYSICS)
					{
						if (importerDebug)
						{
							LL_INFOS() << "Search of" << name_to_match << " in LOD" << i << " list failed. Searching for alternative among LOD lists." << LL_ENDL;
						}

						int searchLOD = (i > LLModel::LOD_HIGH) ? LLModel::LOD_HIGH : i;
						while ((searchLOD <= LLModel::LOD_HIGH) && !lod_model)
						{
							std::string name_to_match = instance.mLabel;
							llassert(!name_to_match.empty());

							std::string toAdd;
							switch (searchLOD)
							{
							case LLModel::LOD_IMPOSTOR: toAdd = "_LOD0"; break;
							case LLModel::LOD_LOW:      toAdd = "_LOD1"; break;
							case LLModel::LOD_MEDIUM:   toAdd = "_LOD2"; break;
							case LLModel::LOD_PHYSICS:  toAdd = "_PHYS"; break;
							case LLModel::LOD_HIGH:                      break;
							}

							if (name_to_match.find(toAdd) == -1)
							{
								name_to_match += toAdd;
							}

							// See if we can find an appropriately named model in LOD 'searchLOD'
							//
							FindModel(mScene[searchLOD], name_to_match, lod_model, transform);
							searchLOD++;
						}
					}
				}
				else
				{
					// Use old method of index-based association
					U32 idx = 0;
					for (idx = 0; idx < mBaseModel.size(); ++idx)
					{
						// find reference instance for this model
						if (mBaseModel[idx] == base_model)
						{
							if (importerDebug)
							{
								LL_INFOS() << "Attempting to use model index " << idx << " for LOD " << i << " of " << instance.mLabel << LL_ENDL;
							}
							break;
						}
					}

					// If the model list for the current LOD includes that index...
					//
					if (mModel[i].size() > idx)
					{
						// Assign that index from the model list for our LOD as the LOD model for this instance
						//
						lod_model = mModel[i][idx];
						if (importerDebug)
						{
							LL_INFOS() << "Indexed match of model index " << idx << " at LOD " << i << " to model named " << lod_model->mLabel << LL_ENDL;
						}
					}
					else if (importerDebug)
					{
						LL_INFOS() << "List of models does not include index " << idx << LL_ENDL;
					}
				}

				if (lod_model)
				{
					if (importerDebug)
					{
						if (i == LLModel::LOD_PHYSICS)
						{
							LL_INFOS() << "Assigning collision for " << instance.mLabel << " to match " << lod_model->mLabel << LL_ENDL;
						}
						else
						{
							LL_INFOS() << "Assigning LOD" << i << " for " << instance.mLabel << " to found match " << lod_model->mLabel << LL_ENDL;
						}
					}
					instance.mLOD[i] = lod_model;
				}
				else
				{
					if (i < LLModel::LOD_HIGH && !lodsReady())
					{
						// assign a placeholder from previous LOD until lod generation is complete.
						// Note: we might need to assign it regardless of conditions like named search does, to prevent crashes.
						instance.mLOD[i] = instance.mLOD[i + 1];
					}
					if (importerDebug)
					{
						LL_INFOS() << "List of models does not include " << instance.mLabel << LL_ENDL;
					}
				}
			}

			LLModel* high_lod_model = instance.mLOD[LLModel::LOD_HIGH];
			if (!high_lod_model)
			{
				setLoadState( LLModelLoader::ERROR_MATERIALS );
				mFMP->childDisable( "calculate_btn" );
			}
			else
			{
				for (U32 i = 0; i < LLModel::NUM_LODS-1; i++)
				{				
					int refFaceCnt = 0;
					int modelFaceCnt = 0;
					llassert(instance.mLOD[i]);
					if (instance.mLOD[i] && !instance.mLOD[i]->matchMaterialOrder(high_lod_model, refFaceCnt, modelFaceCnt ) )
					{
						setLoadState( LLModelLoader::ERROR_MATERIALS );
						mFMP->childDisable( "calculate_btn" );
					}
				}
			}
			instance.mTransform = mat;
			mUploadData.push_back(instance);
		}
	}

	for (U32 lod = 0; lod < LLModel::NUM_LODS-1; lod++)
	{
		// Search for models that are not included into upload data
		// If we found any, that means something we loaded is not a sub-model.
		for (U32 model_ind = 0; model_ind < mModel[lod].size(); ++model_ind)
		{
			bool found_model = false;
			for (LLMeshUploadThread::instance_list::iterator iter = mUploadData.begin(); iter != mUploadData.end(); ++iter)
			{
				LLModelInstance& instance = *iter;
				if (instance.mLOD[lod] == mModel[lod][model_ind])
				{
					found_model = true;
					break;
				}
			}
			if (!found_model && mModel[lod][model_ind] && !mModel[lod][model_ind]->mSubmodelID)
			{
				if (importerDebug)
				{
					LL_INFOS() << "Model " << mModel[lod][model_ind]->mLabel << " was not used - mismatching lod models." <<  LL_ENDL;
				}
				setLoadState( LLModelLoader::ERROR_MATERIALS );
				mFMP->childDisable( "calculate_btn" );
			}
		}
	}

	F32 max_import_scale = (DEFAULT_MAX_PRIM_SCALE-0.1f)/max_scale;

	F32 max_axis = llmax(mPreviewScale.mV[0], mPreviewScale.mV[1]);
	max_axis = llmax(max_axis, mPreviewScale.mV[2]);
	max_axis *= 2.f;

	//clamp scale so that total imported model bounding box is smaller than 240m on a side
	max_import_scale = llmin(max_import_scale, 240.f/max_axis);

	scale_spinner->setMaxValue(max_import_scale);

	if (max_import_scale < scale)
	{
		scale_spinner->setValue(max_import_scale);
	}

}

void LLModelPreview::saveUploadData(bool save_skinweights, bool save_joint_positions, bool lock_scale_if_joint_position)
{
	if (!mLODFile[LLModel::LOD_HIGH].empty())
	{
		std::string filename = mLODFile[LLModel::LOD_HIGH];
        std::string slm_filename;

        if (LLModelLoader::getSLMFilename(filename, slm_filename))
        {
			saveUploadData(slm_filename, save_skinweights, save_joint_positions, lock_scale_if_joint_position);
		}
	}
}

void LLModelPreview::saveUploadData(const std::string& filename, 
                                    bool save_skinweights, bool save_joint_positions, bool lock_scale_if_joint_position)
{

	std::set<LLPointer<LLModel> > meshes;
	std::map<LLModel*, std::string> mesh_binary;

	LLModel::hull empty_hull;

	LLSD data;

	data["version"] = SLM_SUPPORTED_VERSION;
	if (!mBaseModel.empty())
	{
		data["name"] = mBaseModel[0]->getName();
	}

	S32 mesh_id = 0;

	//build list of unique models and initialize local id
	for (U32 i = 0; i < mUploadData.size(); ++i)
	{
		LLModelInstance& instance = mUploadData[i];
		
		if (meshes.find(instance.mModel) == meshes.end())
		{
			instance.mModel->mLocalID = mesh_id++;
			meshes.insert(instance.mModel);

			std::stringstream str;
			LLModel::Decomposition& decomp =
				instance.mLOD[LLModel::LOD_PHYSICS].notNull() ? 
				instance.mLOD[LLModel::LOD_PHYSICS]->mPhysics : 
				instance.mModel->mPhysics;

			LLModel::writeModel(str, 
				instance.mLOD[LLModel::LOD_PHYSICS], 
				instance.mLOD[LLModel::LOD_HIGH], 
				instance.mLOD[LLModel::LOD_MEDIUM], 
				instance.mLOD[LLModel::LOD_LOW], 
				instance.mLOD[LLModel::LOD_IMPOSTOR], 
				decomp, 
				save_skinweights, 
                save_joint_positions,
                lock_scale_if_joint_position,
                FALSE, TRUE, instance.mModel->mSubmodelID);
			
			data["mesh"][instance.mModel->mLocalID] = str.str();
		}

		data["instance"][i] = instance.asLLSD();
	}

	llofstream out(filename.c_str(), std::ios_base::out | std::ios_base::binary);
	LLSDSerialize::toBinary(data, out);
	out.flush();
	out.close();
}

void LLModelPreview::clearModel(S32 lod)
{
	if (lod < 0 || lod > LLModel::LOD_PHYSICS)
	{
		return;
	}

	mVertexBuffer[lod].clear();
	mModel[lod].clear();
	mScene[lod].clear();
}

void LLModelPreview::getJointAliases( JointMap& joint_map)
{
    // Get all standard skeleton joints from the preview avatar.
    LLVOAvatar *av = getPreviewAvatar();
    
    //Joint names and aliases come from avatar_skeleton.xml
    
    joint_map = av->getJointAliases();
    for (S32 i = 0; i < av->mNumCollisionVolumes; i++)
    {
        joint_map[av->mCollisionVolumes[i].getName()] = av->mCollisionVolumes[i].getName();
    }
}

void LLModelPreview::loadModel(std::string filename, S32 lod, bool force_disable_slm)
{
	assert_main_thread();

	LLMutexLock lock(this);

	if (lod < LLModel::LOD_IMPOSTOR || lod > LLModel::NUM_LODS - 1)
	{
		LL_WARNS() << "Invalid level of detail: " << lod << LL_ENDL;
		assert(lod >= LLModel::LOD_IMPOSTOR && lod < LLModel::NUM_LODS);
		return;
	}

	// This triggers if you bring up the file picker and then hit CANCEL.
	// Just use the previous model (if any) and ignore that you brought up
	// the file picker.

	if (filename.empty())
	{
		if (mBaseModel.empty())
		{
			// this is the initial file picking. Close the whole floater
			// if we don't have a base model to show for high LOD.
			mFMP->closeFloater(false);
		}
		mLoading = false;
		return;
	}

	if (mModelLoader)
	{
		LL_WARNS() << "Incompleted model load operation pending." << LL_ENDL;
		return;
	}
	
	mLODFile[lod] = filename;

	if (lod == LLModel::LOD_HIGH)
	{
		clearGLODGroup();
	}

    std::map<std::string, std::string> joint_alias_map;
    getJointAliases(joint_alias_map);
    
	mModelLoader = new LLDAELoader(
		filename,
		lod, 
		&LLModelPreview::loadedCallback,
		&LLModelPreview::lookupJointByName,
		&LLModelPreview::loadTextures,
		&LLModelPreview::stateChangedCallback,
		this,
		mJointTransformMap,
		mJointsFromNode,
        joint_alias_map,
		LLSkinningUtil::getMaxJointCount(),
		gSavedSettings.getU32("ImporterModelLimit"),
		gSavedSettings.getBOOL("ImporterPreprocessDAE"));

	if (force_disable_slm)
	{
		mModelLoader->mTrySLM = false;
	}
	else
	{
        // For MAINT-6647, we have set force_disable_slm to true,
        // which means this code path will never be taken. Trying to
        // re-use SLM files has never worked properly; in particular,
        // it tends to force the UI into strange checkbox options
        // which cannot be altered.
        
		//only try to load from slm if viewer is configured to do so and this is the 
		//initial model load (not an LoD or physics shape)
		mModelLoader->mTrySLM = gSavedSettings.getBOOL("MeshImportUseSLM") && mUploadData.empty();
	}
	mModelLoader->start();

	mFMP->childSetTextArg("status", "[STATUS]", mFMP->getString("status_reading_file"));

	setPreviewLOD(lod);

	if ( getLoadState() >= LLModelLoader::ERROR_PARSING )
	{
		mFMP->childDisable("ok_btn");
		mFMP->childDisable( "calculate_btn" );
	}
	
	if (lod == mPreviewLOD)
	{
		mFMP->childSetValue("lod_file_" + lod_name[lod], mLODFile[lod]);
	}
	else if (lod == LLModel::LOD_PHYSICS)
	{
		mFMP->childSetValue("physics_file", mLODFile[lod]);
	}

	mFMP->openFloater();
}

void LLModelPreview::setPhysicsFromLOD(S32 lod)
{
	assert_main_thread();

	if (lod >= 0 && lod <= 3)
	{
		mPhysicsSearchLOD = lod;
		mModel[LLModel::LOD_PHYSICS] = mModel[lod];
		mScene[LLModel::LOD_PHYSICS] = mScene[lod];
		mLODFile[LLModel::LOD_PHYSICS].clear();
		mFMP->childSetValue("physics_file", mLODFile[LLModel::LOD_PHYSICS]);
		mVertexBuffer[LLModel::LOD_PHYSICS].clear();
		rebuildUploadData();
		refresh();
		updateStatusMessages();
	}
}

void LLModelPreview::clearIncompatible(S32 lod)
{
	//Don't discard models if specified model is the physic rep
	if ( lod == LLModel::LOD_PHYSICS )
	{
		return;
	}

	// at this point we don't care about sub-models,
	// different amount of sub-models means face count mismatch, not incompatibility
	U32 lod_size = countRootModels(mModel[lod]);
	for (U32 i = 0; i <= LLModel::LOD_HIGH; i++)
	{ //clear out any entries that aren't compatible with this model
		if (i != lod)
		{
			if (countRootModels(mModel[i]) != lod_size)
			{
				mModel[i].clear();
				mScene[i].clear();
				mVertexBuffer[i].clear();

				if (i == LLModel::LOD_HIGH)
				{
					mBaseModel = mModel[lod];
					clearGLODGroup();
					mBaseScene = mScene[lod];
					mVertexBuffer[5].clear();
				}
			}
		}
	}
}

void LLModelPreview::clearGLODGroup()
{
	if (mGroup)
	{
		for (std::map<LLPointer<LLModel>, U32>::iterator iter = mObject.begin(); iter != mObject.end(); ++iter)
		{
			glodDeleteObject(iter->second);
			stop_gloderror();
		}
		mObject.clear();

		glodDeleteGroup(mGroup);
		stop_gloderror();
		mGroup = 0;
	}
}

void LLModelPreview::loadModelCallback(S32 loaded_lod)
{
	assert_main_thread();

	LLMutexLock lock(this);
	if (!mModelLoader)
	{
		mLoading = false ;
		return;
	}
	if(getLoadState() >= LLModelLoader::ERROR_PARSING)
	{
		mLoading = false ;
		mModelLoader = NULL;
		mLodsWithParsingError.push_back(loaded_lod);
		return ;
	}

	mLodsWithParsingError.erase(std::remove(mLodsWithParsingError.begin(), mLodsWithParsingError.end(), loaded_lod), mLodsWithParsingError.end());
	if(mLodsWithParsingError.empty())
	{
		mFMP->childEnable( "calculate_btn" );
	}

	// Copy determinations about rig so UI will reflect them
	//
	setRigValidForJointPositionUpload(mModelLoader->isRigValidForJointPositionUpload());
	setLegacyRigValid(mModelLoader->isLegacyRigValid());

	mModelLoader->loadTextures() ;

	if (loaded_lod == -1)
	{ //populate all LoDs from model loader scene
		mBaseModel.clear();
		mBaseScene.clear();

		bool skin_weights = false;
		bool joint_positions = false;
		bool lock_scale_if_joint_position = false;

		for (S32 lod = 0; lod < LLModel::NUM_LODS; ++lod)
		{ //for each LoD

			//clear scene and model info
			mScene[lod].clear();
			mModel[lod].clear();
			mVertexBuffer[lod].clear();
			
			if (mModelLoader->mScene.begin()->second[0].mLOD[lod].notNull())
			{ //if this LoD exists in the loaded scene

				//copy scene to current LoD
				mScene[lod] = mModelLoader->mScene;
			
				//touch up copied scene to look like current LoD
				for (LLModelLoader::scene::iterator iter = mScene[lod].begin(); iter != mScene[lod].end(); ++iter)
				{
					LLModelLoader::model_instance_list& list = iter->second;

					for (LLModelLoader::model_instance_list::iterator list_iter = list.begin(); list_iter != list.end(); ++list_iter)
					{	
						//override displayed model with current LoD
						list_iter->mModel = list_iter->mLOD[lod];

						if (!list_iter->mModel)
						{
							continue;
						}

						//add current model to current LoD's model list (LLModel::mLocalID makes a good vector index)
						S32 idx = list_iter->mModel->mLocalID;

						if (mModel[lod].size() <= idx)
						{ //stretch model list to fit model at given index
							mModel[lod].resize(idx+1);
						}

						mModel[lod][idx] = list_iter->mModel;
						if (!list_iter->mModel->mSkinWeights.empty())
						{
							skin_weights = true;

							if (!list_iter->mModel->mSkinInfo.mAlternateBindMatrix.empty())
							{
								joint_positions = true;
							}
							if (list_iter->mModel->mSkinInfo.mLockScaleIfJointPosition)
							{
								lock_scale_if_joint_position = true;
							}
						}
					}
				}
			}
		}

		if (mFMP)
		{
			LLFloaterModelPreview* fmp = (LLFloaterModelPreview*) mFMP;

			if (skin_weights)
			{ //enable uploading/previewing of skin weights if present in .slm file
				fmp->enableViewOption("show_skin_weight");
				mViewOption["show_skin_weight"] = true;
				fmp->childSetValue("upload_skin", true);
			}

			if (joint_positions)
			{ 
				fmp->enableViewOption("show_joint_positions");
				mViewOption["show_joint_positions"] = true;
				fmp->childSetValue("upload_joints", true);
			}

			if (lock_scale_if_joint_position)
			{
				fmp->enableViewOption("lock_scale_if_joint_position");
				mViewOption["lock_scale_if_joint_position"] = true;
				fmp->childSetValue("lock_scale_if_joint_position", true);
			}
		}

		//copy high lod to base scene for LoD generation
		mBaseScene = mScene[LLModel::LOD_HIGH];
		mBaseModel = mModel[LLModel::LOD_HIGH];

		mDirty = true;
		resetPreviewTarget();
	}
	else
	{ //only replace given LoD
		mModel[loaded_lod] = mModelLoader->mModelList;
		mScene[loaded_lod] = mModelLoader->mScene;
		mVertexBuffer[loaded_lod].clear();

		setPreviewLOD(loaded_lod);

		if (loaded_lod == LLModel::LOD_HIGH)
		{ //save a copy of the highest LOD for automatic LOD manipulation
			if (mBaseModel.empty())
			{ //first time we've loaded a model, auto-gen LoD
				mGenLOD = true;
			}

			mBaseModel = mModel[loaded_lod];
			clearGLODGroup();

			mBaseScene = mScene[loaded_lod];
			mVertexBuffer[5].clear();
		}
		else
		{
			BOOL importerDebug = gSavedSettings.getBOOL("ImporterDebug");
			BOOL legacyMatching = gSavedSettings.getBOOL("ImporterLegacyMatching");
			if (!legacyMatching)
			{
				if (!mBaseModel.empty())
				{ 
					BOOL name_based = FALSE;
					BOOL has_submodels = FALSE;
					for (U32 idx = 0; idx < mBaseModel.size(); ++idx)
					{
						if (mBaseModel[idx]->mSubmodelID)
						{ // don't do index-based renaming when the base model has submodels
							has_submodels = TRUE;
							if (importerDebug)
							{
								LL_INFOS() << "High LOD has submodels" << LL_ENDL;
							}
							break;
						}
					}

					for (U32 idx = 0; idx < mModel[loaded_lod].size(); ++idx)
					{
						std::string loaded_name = stripSuffix(mModel[loaded_lod][idx]->mLabel);

						LLModel* found_model = NULL;
						LLMatrix4 transform;
						FindModel(mBaseScene, loaded_name, found_model, transform);
						if (found_model)
						{ // don't rename correctly named models (even if they are placed in a wrong order)
							name_based = TRUE;
						}

						if (mModel[loaded_lod][idx]->mSubmodelID)
						{ // don't rename the models when loaded LOD model has submodels
							has_submodels = TRUE;
						}
					}

					if (importerDebug)
					{
						LL_INFOS() << "Loaded LOD " << loaded_lod << ": correct names" << (name_based ? "" : "NOT ") << "found; submodels " << (has_submodels ? "" : "NOT ") << "found" << LL_ENDL;
					}

					if (!name_based && !has_submodels)
					{ // replace the name of the model loaded for any non-HIGH LOD to match the others (MAINT-5601)
					  // this actually works like "ImporterLegacyMatching" for this particular LOD
						for (U32 idx = 0; idx < mModel[loaded_lod].size() && idx < mBaseModel.size(); ++idx)
						{ 
							std::string name = mBaseModel[idx]->mLabel;
							std::string loaded_name = stripSuffix(mModel[loaded_lod][idx]->mLabel);

							if (loaded_name != name)
							{
								switch (loaded_lod)
								{
								case LLModel::LOD_IMPOSTOR: name += "_LOD0"; break;
								case LLModel::LOD_LOW:      name += "_LOD1"; break;
								case LLModel::LOD_MEDIUM:   name += "_LOD2"; break;
								case LLModel::LOD_PHYSICS:  name += "_PHYS"; break;
								case LLModel::LOD_HIGH:                      break;
								}

								if (importerDebug)
								{
									LL_WARNS() << "Loded model name " << mModel[loaded_lod][idx]->mLabel << " for LOD " << loaded_lod << " doesn't match the base model. Renaming to " << name << LL_ENDL;
								}

								mModel[loaded_lod][idx]->mLabel = name;
							}
						}
					}
				}
			}
		}

		clearIncompatible(loaded_lod);

		mDirty = true;

		if (loaded_lod == LLModel::LOD_HIGH)
		{
			resetPreviewTarget();
		}
	}

	mLoading = false;
	if (mFMP)
	{
		mFMP->getChild<LLCheckBoxCtrl>("confirm_checkbox")->set(FALSE);
		if (!mBaseModel.empty())
		{
			const std::string& model_name = mBaseModel[0]->getName();
			LLLineEditor* description_form = mFMP->getChild<LLLineEditor>("description_form");
			if (description_form->getText().empty())
			{
				description_form->setText(model_name);
			}
		}
	}
	refresh();

	mModelLoadedSignal();

	mModelLoader = NULL;
}

void LLModelPreview::resetPreviewTarget()
{
	if ( mModelLoader )
	{
		mPreviewTarget = (mModelLoader->mExtents[0] + mModelLoader->mExtents[1]) * 0.5f;
		mPreviewScale = (mModelLoader->mExtents[1] - mModelLoader->mExtents[0]) * 0.5f;
	}

	setPreviewTarget(mPreviewScale.magVec()*10.f);
}

void LLModelPreview::generateNormals()
{
	assert_main_thread();

	S32 which_lod = mPreviewLOD;

	if (which_lod > 4 || which_lod < 0 ||
		mModel[which_lod].empty())
	{
		return;
	}

	F32 angle_cutoff = mFMP->childGetValue("crease_angle").asReal();

	mRequestedCreaseAngle[which_lod] = angle_cutoff;

	angle_cutoff *= DEG_TO_RAD;

	if (which_lod == 3 && !mBaseModel.empty())
	{
		if(mBaseModelFacesCopy.empty())
		{
			mBaseModelFacesCopy.reserve(mBaseModel.size());
			for (LLModelLoader::model_list::iterator it = mBaseModel.begin(), itE = mBaseModel.end(); it != itE; ++it)
			{
				v_LLVolumeFace_t faces;
				(*it)->copyFacesTo(faces);
				mBaseModelFacesCopy.push_back(faces);
			}
		}

		for (LLModelLoader::model_list::iterator it = mBaseModel.begin(), itE = mBaseModel.end(); it != itE; ++it)
		{
			(*it)->generateNormals(angle_cutoff);
		}

		mVertexBuffer[5].clear();
	}

	bool perform_copy = mModelFacesCopy[which_lod].empty();
	if(perform_copy) {
		mModelFacesCopy[which_lod].reserve(mModel[which_lod].size());
	}

	for (LLModelLoader::model_list::iterator it = mModel[which_lod].begin(), itE = mModel[which_lod].end(); it != itE; ++it)
	{
		if(perform_copy)
		{
			v_LLVolumeFace_t faces;
			(*it)->copyFacesTo(faces);
			mModelFacesCopy[which_lod].push_back(faces);
		}

		(*it)->generateNormals(angle_cutoff);
	}

	mVertexBuffer[which_lod].clear();
	refresh();
	updateStatusMessages();
}

void LLModelPreview::restoreNormals()
{
	S32 which_lod = mPreviewLOD;

	if (which_lod > 4 || which_lod < 0 ||
		mModel[which_lod].empty())
	{
		return;
	}

	if(!mBaseModelFacesCopy.empty())
	{
		llassert(mBaseModelFacesCopy.size() == mBaseModel.size());

		vv_LLVolumeFace_t::const_iterator itF = mBaseModelFacesCopy.begin();
		for (LLModelLoader::model_list::iterator it = mBaseModel.begin(), itE = mBaseModel.end(); it != itE; ++it, ++itF)
		{
			(*it)->copyFacesFrom((*itF));
		}

		mBaseModelFacesCopy.clear();
	}
	
	if(!mModelFacesCopy[which_lod].empty())
	{
		vv_LLVolumeFace_t::const_iterator itF = mModelFacesCopy[which_lod].begin();
		for (LLModelLoader::model_list::iterator it = mModel[which_lod].begin(), itE = mModel[which_lod].end(); it != itE; ++it, ++itF)
		{
			(*it)->copyFacesFrom((*itF));
		}

		mModelFacesCopy[which_lod].clear();
	}
	
	mVertexBuffer[which_lod].clear();
	refresh();
	updateStatusMessages();
}

void LLModelPreview::genLODs(S32 which_lod, U32 decimation, bool enforce_tri_limit)
{
	// Allow LoD from -1 to LLModel::LOD_PHYSICS
	if (which_lod < -1 || which_lod > LLModel::NUM_LODS - 1)
	{
		LL_WARNS() << "Invalid level of detail: " << which_lod << LL_ENDL;
		assert(which_lod >= -1 && which_lod < LLModel::NUM_LODS);
		return;
	}

	if (mBaseModel.empty())
	{
		return;
	}

	LLVertexBuffer::unbind();

	bool no_ff = LLGLSLShader::sNoFixedFunction;
	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
	LLGLSLShader::sNoFixedFunction = false;

	if (shader)
	{
		shader->unbind();
	}
	
	stop_gloderror();
	static U32 cur_name = 1;

	S32 limit = -1;

	U32 triangle_count = 0;

	U32 instanced_triangle_count = 0;

	//get the triangle count for the whole scene
	for (LLModelLoader::scene::iterator iter = mBaseScene.begin(), endIter = mBaseScene.end(); iter != endIter; ++iter)
	{
		for (LLModelLoader::model_instance_list::iterator instance = iter->second.begin(), end_instance = iter->second.end(); instance != end_instance; ++instance)
		{
			LLModel* mdl = instance->mModel;
			if (mdl)
			{
				instanced_triangle_count += mdl->getNumTriangles();
			}
		}
	}

	//get the triangle count for the non-instanced set of models
	for (U32 i = 0; i < mBaseModel.size(); ++i)
	{
		triangle_count += mBaseModel[i]->getNumTriangles();
	}
	
	//get ratio of uninstanced triangles to instanced triangles
	F32 triangle_ratio = (F32) triangle_count / (F32) instanced_triangle_count;

	U32 base_triangle_count = triangle_count;

	U32 type_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0;

	U32 lod_mode = 0;

	F32 lod_error_threshold = 0;

	// The LoD should be in range from Lowest to High
	if (which_lod > -1 && which_lod < NUM_LOD)
	{
		LLCtrlSelectionInterface* iface = mFMP->childGetSelectionInterface("lod_mode_" + lod_name[which_lod]);
		if (iface)
		{
			lod_mode = iface->getFirstSelectedIndex();
		}

		lod_error_threshold = mFMP->childGetValue("lod_error_threshold_" + lod_name[which_lod]).asReal();
	}

	if (which_lod != -1)
	{
		mRequestedLoDMode[which_lod] = lod_mode;
	}

	if (lod_mode == 0)
	{
		lod_mode = GLOD_TRIANGLE_BUDGET;

		// The LoD should be in range from Lowest to High
		if (which_lod > -1 && which_lod < NUM_LOD)
		{
			limit = mFMP->childGetValue("lod_triangle_limit_" + lod_name[which_lod]).asInteger();
			//convert from "scene wide" to "non-instanced" triangle limit
			limit = (S32) ( (F32) limit*triangle_ratio );
		}
	}
	else
	{
		lod_mode = GLOD_ERROR_THRESHOLD;
	}

	bool object_dirty = false;

	if (mGroup == 0)
	{
		object_dirty = true;
		mGroup = cur_name++;
		glodNewGroup(mGroup);
	}

	if (object_dirty)
	{
		for (LLModelLoader::model_list::iterator iter = mBaseModel.begin(); iter != mBaseModel.end(); ++iter)
		{ //build GLOD objects for each model in base model list
			LLModel* mdl = *iter;

			if (mObject[mdl] != 0)
			{
				glodDeleteObject(mObject[mdl]);
			}

			mObject[mdl] = cur_name++;

			glodNewObject(mObject[mdl], mGroup, GLOD_DISCRETE);
			stop_gloderror();

			if (iter == mBaseModel.begin() && !mdl->mSkinWeights.empty())
			{ //regenerate vertex buffer for skinned models to prevent animation feedback during LOD generation
				mVertexBuffer[5].clear();
			}

			if (mVertexBuffer[5].empty())
			{
				genBuffers(5, false);
			}

			U32 tri_count = 0;
			for (U32 i = 0; i < mVertexBuffer[5][mdl].size(); ++i)
			{
				LLVertexBuffer* buff = mVertexBuffer[5][mdl][i];
				buff->setBuffer(type_mask & buff->getTypeMask());
				
				U32 num_indices = mVertexBuffer[5][mdl][i]->getNumIndices();
				if (num_indices > 2)
				{
					glodInsertElements(mObject[mdl], i, GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, (U8*) mVertexBuffer[5][mdl][i]->getIndicesPointer(), 0, 0.f);
				}
				tri_count += num_indices/3;
				stop_gloderror();
			}

			glodBuildObject(mObject[mdl]);
			stop_gloderror();
		}
	}


	S32 start = LLModel::LOD_HIGH;
	S32 end = 0;

	if (which_lod != -1)
	{
		start = end = which_lod;
	}

	mMaxTriangleLimit = base_triangle_count;

	for (S32 lod = start; lod >= end; --lod)
	{
		if (which_lod == -1)
		{
			if (lod < start)
			{
				triangle_count /= decimation;
			}
		}
		else
		{
			if (enforce_tri_limit)
			{
				triangle_count = limit;
			}
			else
			{
				for (S32 j=LLModel::LOD_HIGH; j>which_lod; --j)
				{
					triangle_count /= decimation;
				}
			}
		}

		mModel[lod].clear();
		mModel[lod].resize(mBaseModel.size());
		mVertexBuffer[lod].clear();

		U32 actual_tris = 0;
		U32 actual_verts = 0;
		U32 submeshes = 0;

		mRequestedTriangleCount[lod] = (S32) ( (F32) triangle_count / triangle_ratio );
		mRequestedErrorThreshold[lod] = lod_error_threshold;

		glodGroupParameteri(mGroup, GLOD_ADAPT_MODE, lod_mode);
		stop_gloderror();

		glodGroupParameteri(mGroup, GLOD_ERROR_MODE, GLOD_OBJECT_SPACE_ERROR);
		stop_gloderror();

		glodGroupParameterf(mGroup, GLOD_OBJECT_SPACE_ERROR_THRESHOLD, lod_error_threshold);
		stop_gloderror();

		if (lod_mode != GLOD_TRIANGLE_BUDGET)
		{ 			
			glodGroupParameteri(mGroup, GLOD_MAX_TRIANGLES, 0);
		}
		else
		{
			//SH-632: always add 1 to desired amount to avoid decimating below desired amount
			glodGroupParameteri(mGroup, GLOD_MAX_TRIANGLES, triangle_count+1);
		}
			
		stop_gloderror();
		glodAdaptGroup(mGroup);
		stop_gloderror();		

		for (U32 mdl_idx = 0; mdl_idx < mBaseModel.size(); ++mdl_idx)
		{
			LLModel* base = mBaseModel[mdl_idx];

			GLint patch_count = 0;
			glodGetObjectParameteriv(mObject[base], GLOD_NUM_PATCHES, &patch_count);
			stop_gloderror();

			LLVolumeParams volume_params;
			volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);
			mModel[lod][mdl_idx] = new LLModel(volume_params, 0.f);

            std::string name = base->mLabel;

            switch (lod)
            {
                case LLModel::LOD_IMPOSTOR: name += "_LOD0"; break;
                case LLModel::LOD_LOW:      name += "_LOD1"; break;
		        case LLModel::LOD_MEDIUM:   name += "_LOD2"; break;
                case LLModel::LOD_PHYSICS:  name += "_PHYS"; break;
                case LLModel::LOD_HIGH:                      break;
            }

            mModel[lod][mdl_idx]->mLabel = name;
			mModel[lod][mdl_idx]->mSubmodelID = base->mSubmodelID;
            
			GLint* sizes = new GLint[patch_count*2];
			glodGetObjectParameteriv(mObject[base], GLOD_PATCH_SIZES, sizes);
			stop_gloderror();

			GLint* names = new GLint[patch_count];
			glodGetObjectParameteriv(mObject[base], GLOD_PATCH_NAMES, names);
			stop_gloderror();

			mModel[lod][mdl_idx]->setNumVolumeFaces(patch_count);

			LLModel* target_model = mModel[lod][mdl_idx];

			for (GLint i = 0; i < patch_count; ++i)
			{
				type_mask = mVertexBuffer[5][base][i]->getTypeMask();

				LLPointer<LLVertexBuffer> buff = new LLVertexBuffer(type_mask, 0);

				if (sizes[i*2+1] > 0 && sizes[i*2] > 0)
				{
					buff->allocateBuffer(sizes[i*2+1], sizes[i*2], true);
					buff->setBuffer(type_mask);
					glodFillElements(mObject[base], names[i], GL_UNSIGNED_SHORT, (U8*) buff->getIndicesPointer());
					stop_gloderror();
				}
				else
				{ //this face was eliminated, create a dummy triangle (one vertex, 3 indices, all 0)
					buff->allocateBuffer(1, 3, true);
					memset((U8*) buff->getMappedData(), 0, buff->getSize());
					memset((U8*) buff->getIndicesPointer(), 0, buff->getIndicesSize());
				}

				buff->validateRange(0, buff->getNumVerts()-1, buff->getNumIndices(), 0);

				LLStrider<LLVector3> pos;
				LLStrider<LLVector3> norm;
				LLStrider<LLVector2> tc;
				LLStrider<U16> index;

				buff->getVertexStrider(pos);
				if (type_mask & LLVertexBuffer::MAP_NORMAL)
				{
					buff->getNormalStrider(norm);
				}
				if (type_mask & LLVertexBuffer::MAP_TEXCOORD0)
				{
					buff->getTexCoord0Strider(tc);
				}

				buff->getIndexStrider(index);

				target_model->setVolumeFaceData(names[i], pos, norm, tc, index, buff->getNumVerts(), buff->getNumIndices());
				actual_tris += buff->getNumIndices()/3;
				actual_verts += buff->getNumVerts();
				++submeshes;

				if (!validate_face(target_model->getVolumeFace(names[i])))
				{
					LL_ERRS() << "Invalid face generated during LOD generation." << LL_ENDL;
				}
			}

			//blind copy skin weights and just take closest skin weight to point on
			//decimated mesh for now (auto-generating LODs with skin weights is still a bit
			//of an open problem).
			target_model->mPosition = base->mPosition;
			target_model->mSkinWeights = base->mSkinWeights;
			target_model->mSkinInfo = base->mSkinInfo;
			//copy material list
			target_model->mMaterialList = base->mMaterialList;

			if (!validate_model(target_model))
			{
				LL_ERRS() << "Invalid model generated when creating LODs" << LL_ENDL;
			}

			delete [] sizes;
			delete [] names;
		}

		//rebuild scene based on mBaseScene
		mScene[lod].clear();
		mScene[lod] = mBaseScene;

		for (U32 i = 0; i < mBaseModel.size(); ++i)
		{
			LLModel* mdl = mBaseModel[i];
			LLModel* target = mModel[lod][i];
			if (target)
			{
				for (LLModelLoader::scene::iterator iter = mScene[lod].begin(); iter != mScene[lod].end(); ++iter)
				{
					for (U32 j = 0; j < iter->second.size(); ++j)
					{
						if (iter->second[j].mModel == mdl)
						{
							iter->second[j].mModel = target;
						}
					}
				}
			}
		}
	}

	mResourceCost = calcResourceCost();

	LLVertexBuffer::unbind();
	LLGLSLShader::sNoFixedFunction = no_ff;
	if (shader)
	{
		shader->bind();
	}
}

void LLModelPreview::updateStatusMessages()
{
	assert_main_thread();

	//triangle/vertex/submesh count for each mesh asset for each lod
	std::vector<S32> tris[LLModel::NUM_LODS];
	std::vector<S32> verts[LLModel::NUM_LODS];
	std::vector<S32> submeshes[LLModel::NUM_LODS];

	//total triangle/vertex/submesh count for each lod
	S32 total_tris[LLModel::NUM_LODS];
	S32 total_verts[LLModel::NUM_LODS];
	S32 total_submeshes[LLModel::NUM_LODS];

    for (U32 i = 0; i < LLModel::NUM_LODS-1; i++)
    {
        total_tris[i] = 0;
	    total_verts[i] = 0;
	    total_submeshes[i] = 0;
    }

    for (LLMeshUploadThread::instance_list::iterator iter = mUploadData.begin(); iter != mUploadData.end(); ++iter)
	{
		LLModelInstance& instance = *iter;

        LLModel* model_high_lod = instance.mLOD[LLModel::LOD_HIGH];
        if (!model_high_lod)
		{
			setLoadState( LLModelLoader::ERROR_MATERIALS );
			mFMP->childDisable( "calculate_btn" );
			continue;
		}

        for (U32 i = 0; i < LLModel::NUM_LODS-1; i++)
		{
            LLModel* lod_model = instance.mLOD[i];
            if (!lod_model)
            {
                setLoadState( LLModelLoader::ERROR_MATERIALS );
                mFMP->childDisable( "calculate_btn" );
            }
            else
			{
					//for each model in the lod
				S32 cur_tris = 0;
				S32 cur_verts = 0;
				S32 cur_submeshes = lod_model->getNumVolumeFaces();

				for (S32 j = 0; j < cur_submeshes; ++j)
				{ //for each submesh (face), add triangles and vertices to current total
					const LLVolumeFace& face = lod_model->getVolumeFace(j);
					cur_tris += face.mNumIndices/3;
					cur_verts += face.mNumVertices;
				}

                std::string instance_name = instance.mLabel;

                BOOL importerDebug = gSavedSettings.getBOOL("ImporterDebug");
                if (importerDebug)
                {
                    // Useful for debugging generalized complaints below about total submeshes which don't have enough
                    // context to address exactly what needs to be fixed to move towards compliance with the rules.
                    //
                    LL_INFOS() << "Instance " << lod_model->mLabel << " LOD " << i << " Verts: "   << cur_verts     << LL_ENDL;
                    LL_INFOS() << "Instance " << lod_model->mLabel << " LOD " << i << " Tris:  "   << cur_tris      << LL_ENDL;
                    LL_INFOS() << "Instance " << lod_model->mLabel << " LOD " << i << " Faces: "   << cur_submeshes << LL_ENDL;

                    LLModel::material_list::iterator mat_iter = lod_model->mMaterialList.begin();
                    while (mat_iter != lod_model->mMaterialList.end())
                    {
                        LL_INFOS() << "Instance " << lod_model->mLabel << " LOD " << i << " Material " << *(mat_iter) << LL_ENDL;
                        mat_iter++;
                    }
                }

                //add this model to the lod total
				total_tris[i] += cur_tris;
				total_verts[i] += cur_verts;
				total_submeshes[i] += cur_submeshes;

				//store this model's counts to asset data
				tris[i].push_back(cur_tris);
				verts[i].push_back(cur_verts);
				submeshes[i].push_back(cur_submeshes);
			}
		}
    }

	if (mMaxTriangleLimit == 0)
	{
		mMaxTriangleLimit = total_tris[LLModel::LOD_HIGH];
	}

	bool has_degenerate = false;

	{//check for degenerate triangles in physics mesh
		U32 lod = LLModel::LOD_PHYSICS;
		const LLVector4a scale(0.5f);
		for (U32 i = 0; i < mModel[lod].size() && !has_degenerate; ++i)
		{ //for each model in the lod
			if (mModel[lod][i] && mModel[lod][i]->mPhysics.mHull.empty())
			{ //no decomp exists
				S32 cur_submeshes = mModel[lod][i]->getNumVolumeFaces();
				for (S32 j = 0; j < cur_submeshes && !has_degenerate; ++j)
				{ //for each submesh (face), add triangles and vertices to current total
					LLVolumeFace& face = mModel[lod][i]->getVolumeFace(j);
					for (S32 k = 0; (k < face.mNumIndices) && !has_degenerate; )
					{
						U16 index_a = face.mIndices[k+0];
						U16 index_b = face.mIndices[k+1];
						U16 index_c = face.mIndices[k+2];

						LLVector4a v1; v1.setMul(face.mPositions[index_a], scale);
						LLVector4a v2; v2.setMul(face.mPositions[index_b], scale);
						LLVector4a v3; v3.setMul(face.mPositions[index_c], scale);

						if (ll_is_degenerate(v1,v2,v3))
						{
							has_degenerate = true;
						}
						else
						{
							k += 3;
						}
					}
				}
			}
		}
	}

	mFMP->childSetTextArg("submeshes_info", "[SUBMESHES]", llformat("%d", total_submeshes[LLModel::LOD_HIGH]));

	std::string mesh_status_na = mFMP->getString("mesh_status_na");

	S32 upload_status[LLModel::LOD_HIGH+1];

	mModelNoErrors = true;

	const U32 lod_high = LLModel::LOD_HIGH;
	U32 high_submodel_count = mModel[lod_high].size() - countRootModels(mModel[lod_high]);

	for (S32 lod = 0; lod <= lod_high; ++lod)
	{
		upload_status[lod] = 0;

		std::string message = "mesh_status_good";

		if (total_tris[lod] > 0)
		{
			mFMP->childSetValue(lod_triangles_name[lod], llformat("%d", total_tris[lod]));
			mFMP->childSetValue(lod_vertices_name[lod], llformat("%d", total_verts[lod]));
		}
		else
		{
			if (lod == lod_high)
			{
				upload_status[lod] = 2;
				message = "mesh_status_missing_lod";
			}
			else
			{
				for (S32 i = lod-1; i >= 0; --i)
				{
					if (total_tris[i] > 0)
					{
						upload_status[lod] = 2;
						message = "mesh_status_missing_lod";
					}
				}
			}

			mFMP->childSetValue(lod_triangles_name[lod], mesh_status_na);
			mFMP->childSetValue(lod_vertices_name[lod], mesh_status_na);
		}

		if (lod != lod_high)
		{
			if (total_submeshes[lod] && total_submeshes[lod] != total_submeshes[lod_high])
			{ //number of submeshes is different
				message = "mesh_status_submesh_mismatch";
				upload_status[lod] = 2;
			}
			else if (mModel[lod].size() - countRootModels(mModel[lod]) != high_submodel_count)
			{//number of submodels is different, not all faces are matched correctly.
				message = "mesh_status_submesh_mismatch";
				upload_status[lod] = 2;
				// Note: Submodels in instance were loaded from higher LOD and as result face count
				// returns same value and total_submeshes[lod] is identical to high_lod one.
			}
			else if (!tris[lod].empty() && tris[lod].size() != tris[lod_high].size())
			{ //number of meshes is different
				message = "mesh_status_mesh_mismatch";
				upload_status[lod] = 2;
			}
			else if (!verts[lod].empty())
			{
				S32 sum_verts_higher_lod = 0;
				S32 sum_verts_this_lod = 0;
				for (U32 i = 0; i < verts[lod].size(); ++i)
				{
					sum_verts_higher_lod += ((i < verts[lod+1].size()) ? verts[lod+1][i] : 0);
					sum_verts_this_lod += verts[lod][i];
				}

				if ((sum_verts_higher_lod > 0) &&
					(sum_verts_this_lod > sum_verts_higher_lod))
				{
					//too many vertices in this lod
					message = "mesh_status_too_many_vertices";
					upload_status[lod] = 1;
				}
			}
		}

		LLIconCtrl* icon = mFMP->getChild<LLIconCtrl>(lod_icon_name[lod]);
		LLUIImagePtr img = LLUI::getUIImage(lod_status_image[upload_status[lod]]);
		icon->setVisible(true);
		icon->setImage(img);

		if (upload_status[lod] >= 2)
		{
			mModelNoErrors = false;
		}

		if (lod == mPreviewLOD)
		{
			mFMP->childSetValue("lod_status_message_text", mFMP->getString(message));
			icon = mFMP->getChild<LLIconCtrl>("lod_status_message_icon");
			icon->setImage(img);
		}

		updateLodControls(lod);
	}


	//warn if hulls have more than 256 points in them
	BOOL physExceededVertexLimit = FALSE;
	for (U32 i = 0; mModelNoErrors && i < mModel[LLModel::LOD_PHYSICS].size(); ++i)
	{
		LLModel* mdl = mModel[LLModel::LOD_PHYSICS][i];

		if (mdl)
		{
			for (U32 j = 0; j < mdl->mPhysics.mHull.size(); ++j)
			{
				if (mdl->mPhysics.mHull[j].size() > 256)
				{
					physExceededVertexLimit = TRUE;
					LL_INFOS() << "Physical model " << mdl->mLabel << " exceeds vertex per hull limitations." << LL_ENDL;
					break;
				}
			}
		}
	}
	mFMP->childSetVisible("physics_status_message_text", physExceededVertexLimit);
	LLIconCtrl* physStatusIcon = mFMP->getChild<LLIconCtrl>("physics_status_message_icon");
	physStatusIcon->setVisible(physExceededVertexLimit);
	if (physExceededVertexLimit)
	{
		mFMP->childSetValue("physics_status_message_text", mFMP->getString("phys_status_vertex_limit_exceeded"));
		LLUIImagePtr img = LLUI::getUIImage("ModelImport_Status_Warning");
		physStatusIcon->setImage(img);
	}

	if (getLoadState() >= LLModelLoader::ERROR_PARSING)
	{
		mModelNoErrors = false;
		LL_INFOS() << "Loader returned errors, model can't be uploaded" << LL_ENDL;
	}

	bool uploadingSkin		     = mFMP->childGetValue("upload_skin").asBoolean();
	bool uploadingJointPositions = mFMP->childGetValue("upload_joints").asBoolean();

	if ( uploadingSkin )
	{
		if ( uploadingJointPositions && !isRigValidForJointPositionUpload() )
		{
			mModelNoErrors = false;
			LL_INFOS() << "Invalid rig, there might be issues with uploading Joint positions" << LL_ENDL;
		}
	}

	if(mModelNoErrors && mModelLoader)
	{
		if(!mModelLoader->areTexturesReady() && mFMP->childGetValue("upload_textures").asBoolean())
		{
			// Some textures are still loading, prevent upload until they are done
			mModelNoErrors = false;
		}
	}

	// Todo: investigate use of has_degenerate and include into mModelNoErrors upload blocking mechanics
	// current use of has_degenerate won't block upload permanently - later checks will restore the button
	if (!mModelNoErrors || has_degenerate)
	{
		mFMP->childDisable("ok_btn");
	}
	
	//add up physics triangles etc
	S32 phys_tris = 0;
	S32 phys_hulls = 0;
	S32 phys_points = 0;

	//get the triangle count for the whole scene
	for (LLModelLoader::scene::iterator iter = mScene[LLModel::LOD_PHYSICS].begin(), endIter = mScene[LLModel::LOD_PHYSICS].end(); iter != endIter; ++iter)
	{
		for (LLModelLoader::model_instance_list::iterator instance = iter->second.begin(), end_instance = iter->second.end(); instance != end_instance; ++instance)
		{
			LLModel* model = instance->mModel;
			if (model)
			{
				S32 cur_submeshes = model->getNumVolumeFaces();

				LLModel::convex_hull_decomposition& decomp = model->mPhysics.mHull;

				if (!decomp.empty())
				{
					phys_hulls += decomp.size();
					for (U32 i = 0; i < decomp.size(); ++i)
					{
						phys_points += decomp[i].size();
					}
				}
				else
				{ //choose physics shape OR decomposition, can't use both
					for (S32 j = 0; j < cur_submeshes; ++j)
					{ //for each submesh (face), add triangles and vertices to current total
						const LLVolumeFace& face = model->getVolumeFace(j);
						phys_tris += face.mNumIndices/3;
					}
				}
			}
		}
	}

	if (phys_tris > 0)
	{
		mFMP->childSetTextArg("physics_triangles", "[TRIANGLES]", llformat("%d", phys_tris));
	}
	else
	{
		mFMP->childSetTextArg("physics_triangles", "[TRIANGLES]", mesh_status_na);
	}

	if (phys_hulls > 0)
	{
		mFMP->childSetTextArg("physics_hulls", "[HULLS]", llformat("%d", phys_hulls));
		mFMP->childSetTextArg("physics_points", "[POINTS]", llformat("%d", phys_points));
	}
	else
	{
		mFMP->childSetTextArg("physics_hulls", "[HULLS]", mesh_status_na);
		mFMP->childSetTextArg("physics_points", "[POINTS]", mesh_status_na);
	}

	LLFloaterModelPreview* fmp = LLFloaterModelPreview::sInstance;
	if (fmp)
	{
		if (phys_tris > 0 || phys_hulls > 0)
		{
			if (!fmp->isViewOptionEnabled("show_physics"))
			{
				fmp->enableViewOption("show_physics");
				mViewOption["show_physics"] = true;
				fmp->childSetValue("show_physics", true);
			}
		}
		else
		{
			fmp->disableViewOption("show_physics");
			mViewOption["show_physics"] = false;
			fmp->childSetValue("show_physics", false);

		}

		//bool use_hull = fmp->childGetValue("physics_use_hull").asBoolean();

		//fmp->childSetEnabled("physics_optimize", !use_hull);

		bool enable = (phys_tris > 0 || phys_hulls > 0) && fmp->mCurRequest.empty();
		//enable = enable && !use_hull && fmp->childGetValue("physics_optimize").asBoolean();

		//enable/disable "analysis" UI
		LLPanel* panel = fmp->getChild<LLPanel>("physics analysis");
		LLView* child = panel->getFirstChild();
		while (child)
		{
			child->setEnabled(enable);
			child = panel->findNextSibling(child);
		}

		enable = phys_hulls > 0 && fmp->mCurRequest.empty();
		//enable/disable "simplification" UI
		panel = fmp->getChild<LLPanel>("physics simplification");
		child = panel->getFirstChild();
		while (child)
		{
			child->setEnabled(enable);
			child = panel->findNextSibling(child);
		}

		if (fmp->mCurRequest.empty())
		{
			fmp->childSetVisible("Simplify", true);
			fmp->childSetVisible("simplify_cancel", false);
			fmp->childSetVisible("Decompose", true);
			fmp->childSetVisible("decompose_cancel", false);

			if (phys_hulls > 0)
			{
				fmp->childEnable("Simplify");
			}
		
			if (phys_tris || phys_hulls > 0)
			{
				fmp->childEnable("Decompose");
			}
		}
		else
		{
			fmp->childEnable("simplify_cancel");
			fmp->childEnable("decompose_cancel");
		}
	}

	
	LLCtrlSelectionInterface* iface = fmp->childGetSelectionInterface("physics_lod_combo");
	S32 which_mode = 0; 
	S32 file_mode = 1;
	if (iface)
	{
		which_mode = iface->getFirstSelectedIndex();
		file_mode = iface->getItemCount() - 1;
	}

	if (which_mode == file_mode)
	{
		mFMP->childEnable("physics_file");
		mFMP->childEnable("physics_browse");
	}
	else
	{
		mFMP->childDisable("physics_file");
		mFMP->childDisable("physics_browse");
	}

	LLSpinCtrl* crease = mFMP->getChild<LLSpinCtrl>("crease_angle");
	
	if (mRequestedCreaseAngle[mPreviewLOD] == -1.f)
	{
		mFMP->childSetColor("crease_label", LLColor4::grey);
		crease->forceSetValue(75.f);
	}
	else
	{
		mFMP->childSetColor("crease_label", LLColor4::white);
		crease->forceSetValue(mRequestedCreaseAngle[mPreviewLOD]);
	}

	mModelUpdatedSignal(true);

}

void LLModelPreview::updateLodControls(S32 lod)
{
	if (lod < LLModel::LOD_IMPOSTOR || lod > LLModel::LOD_HIGH)
	{
		LL_WARNS() << "Invalid level of detail: " << lod << LL_ENDL;
		assert(lod >= LLModel::LOD_IMPOSTOR && lod <= LLModel::LOD_HIGH);
		return;
	}

	const char* lod_controls[] =
	{
		"lod_mode_",
		"lod_triangle_limit_",
		"lod_error_threshold_"
	};
	const U32 num_lod_controls = sizeof(lod_controls)/sizeof(char*);

	const char* file_controls[] =
	{
		"lod_browse_",
		"lod_file_",
	};
	const U32 num_file_controls = sizeof(file_controls)/sizeof(char*);

	LLFloaterModelPreview* fmp = LLFloaterModelPreview::sInstance;
	if (!fmp) return;

	LLComboBox* lod_combo = mFMP->findChild<LLComboBox>("lod_source_" + lod_name[lod]);
	if (!lod_combo) return;

	S32 lod_mode = lod_combo->getCurrentIndex();
	if (lod_mode == LOD_FROM_FILE) // LoD from file
	{
		fmp->mLODMode[lod] = 0;
		for (U32 i = 0; i < num_file_controls; ++i)
		{
			mFMP->childSetVisible(file_controls[i] + lod_name[lod], true);
		}

		for (U32 i = 0; i < num_lod_controls; ++i)
		{
			mFMP->childSetVisible(lod_controls[i] + lod_name[lod], false);
		}
	}
	else if (lod_mode == USE_LOD_ABOVE) // use LoD above
	{
		fmp->mLODMode[lod] = 2;
		for (U32 i = 0; i < num_file_controls; ++i)
		{
			mFMP->childSetVisible(file_controls[i] + lod_name[lod], false);
		}

		for (U32 i = 0; i < num_lod_controls; ++i)
		{
			mFMP->childSetVisible(lod_controls[i] + lod_name[lod], false);
		}

		if (lod < LLModel::LOD_HIGH)
		{
			mModel[lod] = mModel[lod + 1];
			mScene[lod] = mScene[lod + 1];
			mVertexBuffer[lod].clear();

			// Also update lower LoD
			if (lod > LLModel::LOD_IMPOSTOR)
			{
				updateLodControls(lod - 1);
			}
		}
	}
	else // auto generate, the default case for all LoDs except High
	{
		fmp->mLODMode[lod] = 1;

		//don't actually regenerate lod when refreshing UI
		mLODFrozen = true;

		for (U32 i = 0; i < num_file_controls; ++i)
		{
			mFMP->getChildView(file_controls[i] + lod_name[lod])->setVisible(false);
		}

		for (U32 i = 0; i < num_lod_controls; ++i)
		{
			mFMP->getChildView(lod_controls[i] + lod_name[lod])->setVisible(true);
		}


		LLSpinCtrl* threshold = mFMP->getChild<LLSpinCtrl>("lod_error_threshold_" + lod_name[lod]);
		LLSpinCtrl* limit = mFMP->getChild<LLSpinCtrl>("lod_triangle_limit_" + lod_name[lod]);

		limit->setMaxValue(mMaxTriangleLimit);
		limit->forceSetValue(mRequestedTriangleCount[lod]);

		threshold->forceSetValue(mRequestedErrorThreshold[lod]);

		mFMP->getChild<LLComboBox>("lod_mode_" + lod_name[lod])->selectNthItem(mRequestedLoDMode[lod]);

		if (mRequestedLoDMode[lod] == 0)
		{
			limit->setVisible(true);
			threshold->setVisible(false);

			limit->setMaxValue(mMaxTriangleLimit);
			limit->setIncrement(mMaxTriangleLimit/32);
		}
		else
		{
			limit->setVisible(false);
			threshold->setVisible(true);
		}

		mLODFrozen = false;
	}
}

void LLModelPreview::setPreviewTarget(F32 distance)
{
	mCameraDistance = distance;
	mCameraZoom = 1.f;
	mCameraPitch = 0.f;
	mCameraYaw = 0.f;
	mCameraOffset.clearVec();
}

void LLModelPreview::clearBuffers()
{
	for (U32 i = 0; i < 6; i++)
	{
		mVertexBuffer[i].clear();
	}
}

void LLModelPreview::genBuffers(S32 lod, bool include_skin_weights)
{
	U32 tri_count = 0;
	U32 vertex_count = 0;
	U32 mesh_count = 0;

	
	LLModelLoader::model_list* model = NULL;

	if (lod < 0 || lod > 4)
	{
		model = &mBaseModel;
		lod = 5;
	}
	else
	{
		model = &(mModel[lod]);
	}

	if (!mVertexBuffer[lod].empty())
	{
		mVertexBuffer[lod].clear();
	}

	mVertexBuffer[lod].clear();

	LLModelLoader::model_list::iterator base_iter = mBaseModel.begin();

	for (LLModelLoader::model_list::iterator iter = model->begin(); iter != model->end(); ++iter)
	{
		LLModel* mdl = *iter;
		if (!mdl)
		{
			continue;
		}

		LLModel* base_mdl = *base_iter;
		base_iter++;

		S32 num_faces = mdl->getNumVolumeFaces();
		for (S32 i = 0; i < num_faces; ++i)
		{
			const LLVolumeFace &vf = mdl->getVolumeFace(i);
			U32 num_vertices = vf.mNumVertices;
			U32 num_indices = vf.mNumIndices;

			if (!num_vertices || ! num_indices)
			{
				continue;
			}

			LLVertexBuffer* vb = NULL;

			bool skinned = include_skin_weights && !mdl->mSkinWeights.empty();

			U32 mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0 ;

			if (skinned)
			{
				mask |= LLVertexBuffer::MAP_WEIGHT4;
			}

			vb = new LLVertexBuffer(mask, 0);

			vb->allocateBuffer(num_vertices, num_indices, TRUE);

			LLStrider<LLVector3> vertex_strider;
			LLStrider<LLVector3> normal_strider;
			LLStrider<LLVector2> tc_strider;
			LLStrider<U16> index_strider;
			LLStrider<LLVector4> weights_strider;

			vb->getVertexStrider(vertex_strider);
			vb->getIndexStrider(index_strider);

			if (skinned)
			{
				vb->getWeight4Strider(weights_strider);
			}

			LLVector4a::memcpyNonAliased16((F32*) vertex_strider.get(), (F32*) vf.mPositions, num_vertices*4*sizeof(F32));
			
			if (vf.mTexCoords)
			{
				vb->getTexCoord0Strider(tc_strider);
				S32 tex_size = (num_vertices*2*sizeof(F32)+0xF) & ~0xF;
				LLVector4a::memcpyNonAliased16((F32*) tc_strider.get(), (F32*) vf.mTexCoords, tex_size);
			}
			
			if (vf.mNormals)
			{
				vb->getNormalStrider(normal_strider);
				LLVector4a::memcpyNonAliased16((F32*) normal_strider.get(), (F32*) vf.mNormals, num_vertices*4*sizeof(F32));
			}

			if (skinned)
			{
				for (U32 i = 0; i < num_vertices; i++)
				{
					//find closest weight to vf.mVertices[i].mPosition
					LLVector3 pos(vf.mPositions[i].getF32ptr());

					const LLModel::weight_list& weight_list = base_mdl->getJointInfluences(pos);
                    llassert(weight_list.size()>0 && weight_list.size() <= 4); // LLModel::loadModel() should guarantee this

					LLVector4 w(0,0,0,0);
					
					for (U32 i = 0; i < weight_list.size(); ++i)
					{
						F32 wght = llclamp(weight_list[i].mWeight, 0.001f, 0.999f);
						F32 joint = (F32) weight_list[i].mJointIdx;
						w.mV[i] = joint + wght;
                        llassert(w.mV[i]-(S32)w.mV[i]>0.0f); // because weights are non-zero, and range of wt values
                                                             //should not cause floating point precision issues.
					}

					*(weights_strider++) = w;
				}
			}

			// build indices
			for (U32 i = 0; i < num_indices; i++)
			{
				*(index_strider++) = vf.mIndices[i];
			}

			mVertexBuffer[lod][mdl].push_back(vb);

			vertex_count += num_vertices;
			tri_count += num_indices/3;
			++mesh_count;

		}
	}
}

void LLModelPreview::update()
{
    if (mGenLOD)
    {
        bool subscribe_for_generation = mLodsQuery.empty();
        mGenLOD = false;
        mDirty = true;
        mLodsQuery.clear();

        for (S32 lod = LLModel::LOD_HIGH; lod >= 0; --lod)
        {
            // adding all lods into query for generation
            mLodsQuery.push_back(lod);
        }

        if (subscribe_for_generation)
        {
            doOnIdleRepeating(lodQueryCallback);
        }
    }

    if (mDirty && mLodsQuery.empty())
	{
		mDirty = false;
		mResourceCost = calcResourceCost();
		refresh();
		updateStatusMessages();
	}
}

//-----------------------------------------------------------------------------
// createPreviewAvatar
//-----------------------------------------------------------------------------
void LLModelPreview::createPreviewAvatar( void )
{
	mPreviewAvatar = (LLVOAvatar*)gObjectList.createObjectViewer( LL_PCODE_LEGACY_AVATAR, gAgent.getRegion() );
	if ( mPreviewAvatar )
	{
		mPreviewAvatar->createDrawable( &gPipeline );
		mPreviewAvatar->mIsDummy = TRUE;
		mPreviewAvatar->mSpecialRenderMode = 1;
		mPreviewAvatar->setPositionAgent( LLVector3::zero );
		mPreviewAvatar->slamPosition();
		mPreviewAvatar->updateJointLODs();
		mPreviewAvatar->updateGeometry( mPreviewAvatar->mDrawable );
		mPreviewAvatar->startMotion( ANIM_AGENT_STAND );
		mPreviewAvatar->hideSkirt();
	}
	else
	{
		LL_INFOS() << "Failed to create preview avatar for upload model window" << LL_ENDL;
	}
}

//static
U32 LLModelPreview::countRootModels(LLModelLoader::model_list models)
{
	U32 root_models = 0;
	model_list::iterator model_iter = models.begin();
	while (model_iter != models.end())
	{
		LLModel* mdl = *model_iter;
		if (mdl && mdl->mSubmodelID == 0)
		{
			root_models++;
		}
		model_iter++;
	}
	return root_models;
}

void LLModelPreview::loadedCallback(
	LLModelLoader::scene& scene,
	LLModelLoader::model_list& model_list,
	S32 lod,
	void* opaque)
{
	LLModelPreview* pPreview = static_cast< LLModelPreview* >(opaque);
	if (pPreview && !LLModelPreview::sIgnoreLoadedCallback)
	{
		pPreview->loadModelCallback(lod);
	}	
}

void LLModelPreview::stateChangedCallback(U32 state,void* opaque)
{
	LLModelPreview* pPreview = static_cast< LLModelPreview* >(opaque);
	if (pPreview)
	{
	 pPreview->setLoadState(state);
	}
}

LLJoint* LLModelPreview::lookupJointByName(const std::string& str, void* opaque)
{
	LLModelPreview* pPreview = static_cast< LLModelPreview* >(opaque);
	if (pPreview)
	{
		return pPreview->getPreviewAvatar()->getJoint(str);
	}
	return NULL;
}

U32 LLModelPreview::loadTextures(LLImportMaterial& material,void* opaque)
{
	(void)opaque;

	if (material.mDiffuseMapFilename.size())
	{
		material.mOpaqueData = new LLPointer< LLViewerFetchedTexture >;
		LLPointer< LLViewerFetchedTexture >& tex = (*reinterpret_cast< LLPointer< LLViewerFetchedTexture > * >(material.mOpaqueData));

		tex = LLViewerTextureManager::getFetchedTextureFromUrl("file://" + material.mDiffuseMapFilename, FTT_LOCAL_FILE, TRUE, LLGLTexture::BOOST_PREVIEW);
		tex->setLoadedCallback(LLModelPreview::textureLoadedCallback, 0, TRUE, FALSE, opaque, NULL, FALSE);
		tex->forceToSaveRawImage(0, F32_MAX);
		material.setDiffuseMap(tex->getID()); // record tex ID
		return 1;
	}

	material.mOpaqueData = NULL;
	return 0;	
}

void LLModelPreview::addEmptyFace( LLModel* pTarget )
{
	U32 type_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0;
	
	LLPointer<LLVertexBuffer> buff = new LLVertexBuffer(type_mask, 0);
	
	buff->allocateBuffer(1, 3, true);
	memset( (U8*) buff->getMappedData(), 0, buff->getSize() );
	memset( (U8*) buff->getIndicesPointer(), 0, buff->getIndicesSize() );
		
	buff->validateRange( 0, buff->getNumVerts()-1, buff->getNumIndices(), 0 );
		
	LLStrider<LLVector3> pos;
	LLStrider<LLVector3> norm;
	LLStrider<LLVector2> tc;
	LLStrider<U16> index;
		
	buff->getVertexStrider(pos);
		
	if ( type_mask & LLVertexBuffer::MAP_NORMAL )
	{
		buff->getNormalStrider(norm);
	}
	if ( type_mask & LLVertexBuffer::MAP_TEXCOORD0 )
	{
		buff->getTexCoord0Strider(tc);
	}
		
	buff->getIndexStrider(index);
		
	//resize face array
	int faceCnt = pTarget->getNumVolumeFaces();
	pTarget->setNumVolumeFaces( faceCnt+1 );	
	pTarget->setVolumeFaceData( faceCnt+1, pos, norm, tc, index, buff->getNumVerts(), buff->getNumIndices() );
	
}	

//-----------------------------------------------------------------------------
// render()
//-----------------------------------------------------------------------------
BOOL LLModelPreview::render()
{
	assert_main_thread();

	LLMutexLock lock(this);
	mNeedsUpdate = FALSE;

	bool use_shaders = LLGLSLShader::sNoFixedFunction;

	bool edges = mViewOption["show_edges"];
	bool joint_positions = mViewOption["show_joint_positions"];
	bool skin_weight = mViewOption["show_skin_weight"];
	bool textures = mViewOption["show_textures"];
	bool physics = mViewOption["show_physics"];

	S32 width = getWidth();
	S32 height = getHeight();

	LLGLSUIDefault def;
	LLGLDisable no_blend(GL_BLEND);
	LLGLEnable cull(GL_CULL_FACE);
	LLGLDepthTest depth(GL_TRUE);
	LLGLDisable fog(GL_FOG);

	{
		if (use_shaders)
		{
			gUIProgram.bind();
		}
		//clear background to blue
		gGL.matrixMode(LLRender::MM_PROJECTION);
		gGL.pushMatrix();
		gGL.loadIdentity();
		gGL.ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);

		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gGL.pushMatrix();
		gGL.loadIdentity();

		gGL.color4f(0.169f, 0.169f, 0.169f, 1.f);

		gl_rect_2d_simple( width, height );

		gGL.matrixMode(LLRender::MM_PROJECTION);
		gGL.popMatrix();

		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gGL.popMatrix();
		if (use_shaders)
		{
			gUIProgram.unbind();
		}
	}

	LLFloaterModelPreview* fmp = LLFloaterModelPreview::sInstance;
	
	bool has_skin_weights = false;
	bool upload_skin = mFMP->childGetValue("upload_skin").asBoolean();	
	bool upload_joints = mFMP->childGetValue("upload_joints").asBoolean();

	if ( upload_joints != mLastJointUpdate )
	{
		mLastJointUpdate = upload_joints;
	}

	for (LLModelLoader::scene::iterator iter = mScene[mPreviewLOD].begin(); iter != mScene[mPreviewLOD].end(); ++iter)
	{
		for (LLModelLoader::model_instance_list::iterator model_iter = iter->second.begin(); model_iter != iter->second.end(); ++model_iter)
		{
			LLModelInstance& instance = *model_iter;
			LLModel* model = instance.mModel;
			model->mPelvisOffset = mPelvisZOffset;
			if (!model->mSkinWeights.empty())
			{
				has_skin_weights = true;
			}
		}
	}

	if (has_skin_weights && lodsReady())
	{ //model has skin weights, enable view options for skin weights and joint positions
		if (fmp && isLegacyRigValid() )
		{
			fmp->enableViewOption("show_skin_weight");
			fmp->setViewOptionEnabled("show_joint_positions", skin_weight);	
			mFMP->childEnable("upload_skin");
			mFMP->childSetValue("show_skin_weight", skin_weight);
		}
	}
	else
	{
		mFMP->childDisable("upload_skin");
		if (fmp)
		{
			mViewOption["show_skin_weight"] = false;
			fmp->disableViewOption("show_skin_weight");
			fmp->disableViewOption("show_joint_positions");

			skin_weight = false;
			mFMP->childSetValue("show_skin_weight", false);
			fmp->setViewOptionEnabled("show_skin_weight", skin_weight);
		}
	}

	if (upload_skin && !has_skin_weights)
	{ //can't upload skin weights if model has no skin weights
		mFMP->childSetValue("upload_skin", false);
		upload_skin = false;
	}

	if (!upload_skin && upload_joints)
	{ //can't upload joints if not uploading skin weights
		mFMP->childSetValue("upload_joints", false);
		upload_joints = false;		
	}	

    if (upload_skin && upload_joints)
    {
        mFMP->childEnable("lock_scale_if_joint_position");
    }
    else
    {
        mFMP->childDisable("lock_scale_if_joint_position");
        mFMP->childSetValue("lock_scale_if_joint_position", false);
    }
    
	//Only enable joint offsets if it passed the earlier critiquing
	if ( isRigValidForJointPositionUpload() )  
	{
		mFMP->childSetEnabled("upload_joints", upload_skin);
	}

	F32 explode = mFMP->childGetValue("physics_explode").asReal();

	glClear(GL_DEPTH_BUFFER_BIT);

	LLRect preview_rect;

	preview_rect = mFMP->getChildView("preview_panel")->getRect();

	F32 aspect = (F32) preview_rect.getWidth()/preview_rect.getHeight();

	LLViewerCamera::getInstance()->setAspect(aspect);

	LLViewerCamera::getInstance()->setView(LLViewerCamera::getInstance()->getDefaultFOV() / mCameraZoom);

	LLVector3 offset = mCameraOffset;
	LLVector3 target_pos = mPreviewTarget+offset;

	F32 z_near = 0.001f;
	F32 z_far = mCameraDistance*10.0f+mPreviewScale.magVec()+mCameraOffset.magVec();

	if (skin_weight)
	{
		target_pos = getPreviewAvatar()->getPositionAgent();
		z_near = 0.01f;
		z_far = 1024.f;
		mCameraDistance = 16.f;

		//render avatar previews every frame
		refresh();
	}

	if (use_shaders)
	{
		gObjectPreviewProgram.bind();
	}

	gGL.loadIdentity();
	gPipeline.enableLightsPreview();

	LLQuaternion camera_rot = LLQuaternion(mCameraPitch, LLVector3::y_axis) *
	LLQuaternion(mCameraYaw, LLVector3::z_axis);

	LLQuaternion av_rot = camera_rot;
	LLViewerCamera::getInstance()->setOriginAndLookAt(
													  target_pos + ((LLVector3(mCameraDistance, 0.f, 0.f) + offset) * av_rot),		// camera
													  LLVector3::z_axis,																	// up
													  target_pos);											// point of interest


	z_near = llclamp(z_far * 0.001f, 0.001f, 0.1f);

	LLViewerCamera::getInstance()->setPerspective(FALSE, mOrigin.mX, mOrigin.mY, width, height, FALSE, z_near, z_far);

	stop_glerror();

	gGL.pushMatrix();
	const F32 BRIGHTNESS = 0.9f;
	gGL.color3f(BRIGHTNESS, BRIGHTNESS, BRIGHTNESS);

	const U32 type_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0;

	LLGLEnable normalize(GL_NORMALIZE);

	if (!mBaseModel.empty() && mVertexBuffer[5].empty())
	{
		genBuffers(-1, skin_weight);
		//genBuffers(3);
		//genLODs();
	}

	if (!mModel[mPreviewLOD].empty())
	{
		mFMP->childEnable("reset_btn");

		bool regen = mVertexBuffer[mPreviewLOD].empty();
		if (!regen)
		{
			const std::vector<LLPointer<LLVertexBuffer> >& vb_vec = mVertexBuffer[mPreviewLOD].begin()->second;
			if (!vb_vec.empty())
			{
				const LLVertexBuffer* buff = vb_vec[0];
				regen = buff->hasDataType(LLVertexBuffer::TYPE_WEIGHT4) != skin_weight;
			}
			else
			{
				LL_INFOS() << "Vertex Buffer[" << mPreviewLOD << "]" << " is EMPTY!!!" << LL_ENDL;
				regen = TRUE;
			}
		}

		if (regen)
		{
			genBuffers(mPreviewLOD, skin_weight);
		}

		if (!skin_weight)
		{
			for (LLMeshUploadThread::instance_list::iterator iter = mUploadData.begin(); iter != mUploadData.end(); ++iter)
			{
				LLModelInstance& instance = *iter;

				LLModel* model = instance.mLOD[mPreviewLOD];

					if (!model)
					{
						continue;
					}

					gGL.pushMatrix();
					LLMatrix4 mat = instance.mTransform;

					gGL.multMatrix((GLfloat*) mat.mMatrix);


					U32 num_models = mVertexBuffer[mPreviewLOD][model].size();
					for (U32 i = 0; i < num_models; ++i)
					{
						LLVertexBuffer* buffer = mVertexBuffer[mPreviewLOD][model][i];
				
						buffer->setBuffer(type_mask & buffer->getTypeMask());

						if (textures)
						{
							int materialCnt = instance.mModel->mMaterialList.size();
							if ( i < materialCnt )
							{
								const std::string& binding = instance.mModel->mMaterialList[i];						
								const LLImportMaterial& material = instance.mMaterial[binding];

								gGL.diffuseColor4fv(material.mDiffuseColor.mV);

								// Find the tex for this material, bind it, and add it to our set
								//
								LLViewerFetchedTexture* tex = bindMaterialDiffuseTexture(material);
								if (tex)
								{
									mTextureSet.insert(tex);
								}
							}
						}
						else
						{
							gGL.diffuseColor4f(1,1,1,1);
						}

						buffer->drawRange(LLRender::TRIANGLES, 0, buffer->getNumVerts()-1, buffer->getNumIndices(), 0);
						gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
						gGL.diffuseColor3f(0.4f, 0.4f, 0.4f);

						if (edges)
						{
							glLineWidth(3.f);
							glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
							buffer->drawRange(LLRender::TRIANGLES, 0, buffer->getNumVerts()-1, buffer->getNumIndices(), 0);
							glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
							glLineWidth(1.f);
						}
					}
					gGL.popMatrix();
				}

			if (physics)
			{
				glClear(GL_DEPTH_BUFFER_BIT);
				
				for (U32 i = 0; i < 2; i++)
				{
					if (i == 0)
					{ //depth only pass
						gGL.setColorMask(false, false);
					}
					else
					{
						gGL.setColorMask(true, true);
					}

					//enable alpha blending on second pass but not first pass
					LLGLState blend(GL_BLEND, i); 
					
					gGL.blendFunc(LLRender::BF_SOURCE_ALPHA, LLRender::BF_ONE_MINUS_SOURCE_ALPHA);

					for (LLMeshUploadThread::instance_list::iterator iter = mUploadData.begin(); iter != mUploadData.end(); ++iter)
					{
						LLModelInstance& instance = *iter;

						LLModel* model = instance.mLOD[LLModel::LOD_PHYSICS];

							if (!model)
							{
								continue;
							}

							gGL.pushMatrix();
							LLMatrix4 mat = instance.mTransform;

						gGL.multMatrix((GLfloat*) mat.mMatrix);


							bool render_mesh = true;

							LLPhysicsDecomp* decomp = gMeshRepo.mDecompThread;
							if (decomp)
							{
								LLMutexLock(decomp->mMutex);

								LLModel::Decomposition& physics = model->mPhysics;

								if (!physics.mHull.empty())
								{
									render_mesh = false;

									if (physics.mMesh.empty())
									{ //build vertex buffer for physics mesh
										gMeshRepo.buildPhysicsMesh(physics);
									}
						
									if (!physics.mMesh.empty())
									{ //render hull instead of mesh
										for (U32 i = 0; i < physics.mMesh.size(); ++i)
										{
											if (explode > 0.f)
											{
												gGL.pushMatrix();

												LLVector3 offset = model->mHullCenter[i]-model->mCenterOfHullCenters;
												offset *= explode;

												gGL.translatef(offset.mV[0], offset.mV[1], offset.mV[2]);
											}

											static std::vector<LLColor4U> hull_colors;

											if (i+1 >= hull_colors.size())
											{
												hull_colors.push_back(LLColor4U(rand()%128+127, rand()%128+127, rand()%128+127, 128));
											}

											gGL.diffuseColor4ubv(hull_colors[i].mV);
											LLVertexBuffer::drawArrays(LLRender::TRIANGLES, physics.mMesh[i].mPositions, physics.mMesh[i].mNormals);

											if (explode > 0.f)
											{
												gGL.popMatrix();
											}
										}
									}
								}
							}
						
							if (render_mesh)
							{
								if (mVertexBuffer[LLModel::LOD_PHYSICS].empty())
								{
									genBuffers(LLModel::LOD_PHYSICS, false);
								}

								U32 num_models = mVertexBuffer[LLModel::LOD_PHYSICS][model].size();
								for (U32 i = 0; i < num_models; ++i)
								{
									LLVertexBuffer* buffer = mVertexBuffer[LLModel::LOD_PHYSICS][model][i];

									gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
									gGL.diffuseColor4f(0.4f, 0.4f, 0.0f, 0.4f);

									buffer->setBuffer(type_mask & buffer->getTypeMask());
									buffer->drawRange(LLRender::TRIANGLES, 0, buffer->getNumVerts()-1, buffer->getNumIndices(), 0);

									gGL.diffuseColor3f(1.f, 1.f, 0.f);

									glLineWidth(2.f);
									glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
									buffer->drawRange(LLRender::TRIANGLES, 0, buffer->getNumVerts()-1, buffer->getNumIndices(), 0);

									glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
									glLineWidth(1.f);
								}
							}

							gGL.popMatrix();
						}

					glLineWidth(3.f);
					glPointSize(8.f);
					gPipeline.enableLightsFullbright(LLColor4::white);
					//show degenerate triangles
					LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_ALWAYS);
					LLGLDisable cull(GL_CULL_FACE);
					gGL.diffuseColor4f(1.f,0.f,0.f,1.f);
					const LLVector4a scale(0.5f);

					for (LLMeshUploadThread::instance_list::iterator iter = mUploadData.begin(); iter != mUploadData.end(); ++iter)
					{
						LLModelInstance& instance = *iter;

						LLModel* model = instance.mLOD[LLModel::LOD_PHYSICS];

						if (!model)
						{
							continue;
						}

						gGL.pushMatrix();
						LLMatrix4 mat = instance.mTransform;

						gGL.multMatrix((GLfloat*) mat.mMatrix);


						LLPhysicsDecomp* decomp = gMeshRepo.mDecompThread;
						if (decomp)
						{
							LLMutexLock(decomp->mMutex);

							LLModel::Decomposition& physics = model->mPhysics;

							if (physics.mHull.empty())
							{
								if (mVertexBuffer[LLModel::LOD_PHYSICS].empty())
								{
									genBuffers(LLModel::LOD_PHYSICS, false);
								}
							
								for (U32 i = 0; i < mVertexBuffer[LLModel::LOD_PHYSICS][model].size(); ++i)
								{
									LLVertexBuffer* buffer = mVertexBuffer[LLModel::LOD_PHYSICS][model][i];

									buffer->setBuffer(type_mask & buffer->getTypeMask());

									LLStrider<LLVector3> pos_strider; 
									buffer->getVertexStrider(pos_strider, 0);
									LLVector4a* pos = (LLVector4a*) pos_strider.get();
							
									LLStrider<U16> idx;
									buffer->getIndexStrider(idx, 0);

									for (U32 i = 0; i < buffer->getNumIndices(); i += 3)
									{
										LLVector4a v1; v1.setMul(pos[*idx++], scale);
										LLVector4a v2; v2.setMul(pos[*idx++], scale);
										LLVector4a v3; v3.setMul(pos[*idx++], scale);

										if (ll_is_degenerate(v1,v2,v3))
										{
											buffer->draw(LLRender::LINE_LOOP, 3, i);
											buffer->draw(LLRender::POINTS, 3, i);
										}
									}
								}
							}
						}

						gGL.popMatrix();
					}
					glLineWidth(1.f);
					glPointSize(1.f);
					gPipeline.enableLightsPreview();
					gGL.setSceneBlendType(LLRender::BT_ALPHA);
				}
			}
		}
		else
		{
			target_pos = getPreviewAvatar()->getPositionAgent();

			LLViewerCamera::getInstance()->setOriginAndLookAt(
															  target_pos + ((LLVector3(mCameraDistance, 0.f, 0.f) + offset) * av_rot),		// camera
															  LLVector3::z_axis,																	// up
															  target_pos);											// point of interest

			for (LLModelLoader::scene::iterator iter = mScene[mPreviewLOD].begin(); iter != mScene[mPreviewLOD].end(); ++iter)
			{
				for (LLModelLoader::model_instance_list::iterator model_iter = iter->second.begin(); model_iter != iter->second.end(); ++model_iter)
				{
					LLModelInstance& instance = *model_iter;
					LLModel* model = instance.mModel;

					if (!model->mSkinWeights.empty())
					{
						for (U32 i = 0, e = mVertexBuffer[mPreviewLOD][model].size(); i < e; ++i)
						{
							LLVertexBuffer* buffer = mVertexBuffer[mPreviewLOD][model][i];

							const LLVolumeFace& face = model->getVolumeFace(i);

							LLStrider<LLVector3> position;
							buffer->getVertexStrider(position);

							LLStrider<LLVector4> weight;
							buffer->getWeight4Strider(weight);

							//quick 'n dirty software vertex skinning

							//build matrix palette

							LLMatrix4a mat[LL_MAX_JOINTS_PER_MESH_OBJECT];
                            const LLMeshSkinInfo *skin = &model->mSkinInfo;
							U32 count = LLSkinningUtil::getMeshJointCount(skin);
                            LLSkinningUtil::initSkinningMatrixPalette((LLMatrix4*)mat, count,
                                                                        skin, getPreviewAvatar());
                            LLMatrix4a bind_shape_matrix;
                            bind_shape_matrix.loadu(skin->mBindShapeMatrix);
                            U32 max_joints = LLSkinningUtil::getMaxJointCount();
							for (U32 j = 0; j < buffer->getNumVerts(); ++j)
							{
                                LLMatrix4a final_mat;
                                F32 *wptr = weight[j].mV;
                                LLSkinningUtil::getPerVertexSkinMatrix(wptr, mat, true, final_mat, max_joints);

								//VECTORIZE THIS
                                LLVector4a& v = face.mPositions[j];

                                LLVector4a t;
                                LLVector4a dst;
                                bind_shape_matrix.affineTransform(v, t);
                                final_mat.affineTransform(t, dst);

								position[j][0] = dst[0];
								position[j][1] = dst[1];
								position[j][2] = dst[2];
							}

							llassert(model->mMaterialList.size() > i); 
							const std::string& binding = instance.mModel->mMaterialList[i];
							const LLImportMaterial& material = instance.mMaterial[binding];

							buffer->setBuffer(type_mask & buffer->getTypeMask());
							gGL.diffuseColor4fv(material.mDiffuseColor.mV);
							gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

							// Find the tex for this material, bind it, and add it to our set
							//
							LLViewerFetchedTexture* tex = bindMaterialDiffuseTexture(material);
							if (tex)
							{
								mTextureSet.insert(tex);
							}
						
							buffer->draw(LLRender::TRIANGLES, buffer->getNumIndices(), 0);
							gGL.diffuseColor3f(0.4f, 0.4f, 0.4f);

							if (edges)
							{
								glLineWidth(3.f);
								glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
								buffer->draw(LLRender::TRIANGLES, buffer->getNumIndices(), 0);
								glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
								glLineWidth(1.f);
							}
						}
					}
				}
			}

			if (joint_positions)
			{
				LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
				if (shader)
				{
					gDebugProgram.bind();
				}
				getPreviewAvatar()->renderCollisionVolumes();
				getPreviewAvatar()->renderBones();
				if (shader)
				{
					shader->bind();
				}
			}

		}
	}

	if (use_shaders)
	{
		gObjectPreviewProgram.unbind();
	}

	gGL.popMatrix();

	return TRUE;
}

//-----------------------------------------------------------------------------
// refresh()
//-----------------------------------------------------------------------------
void LLModelPreview::refresh()
{
	mNeedsUpdate = TRUE;
}

//-----------------------------------------------------------------------------
// rotate()
//-----------------------------------------------------------------------------
void LLModelPreview::rotate(F32 yaw_radians, F32 pitch_radians)
{
	mCameraYaw = mCameraYaw + yaw_radians;

	mCameraPitch = llclamp(mCameraPitch + pitch_radians, F_PI_BY_TWO * -0.8f, F_PI_BY_TWO * 0.8f);
}

//-----------------------------------------------------------------------------
// zoom()
//-----------------------------------------------------------------------------
void LLModelPreview::zoom(F32 zoom_amt)
{
	F32 new_zoom = mCameraZoom+zoom_amt;

	mCameraZoom	= llclamp(new_zoom, 1.f, 10.f);
}

void LLModelPreview::pan(F32 right, F32 up)
{
	mCameraOffset.mV[VY] = llclamp(mCameraOffset.mV[VY] + right * mCameraDistance / mCameraZoom, -1.f, 1.f);
	mCameraOffset.mV[VZ] = llclamp(mCameraOffset.mV[VZ] + up * mCameraDistance / mCameraZoom, -1.f, 1.f);
}

void LLModelPreview::setPreviewLOD(S32 lod)
{
	lod = llclamp(lod, 0, (S32) LLModel::LOD_HIGH);

	if (lod != mPreviewLOD)
	{
		mPreviewLOD = lod;

		LLComboBox* combo_box = mFMP->getChild<LLComboBox>("preview_lod_combo");
		combo_box->setCurrentByIndex((NUM_LOD-1)-mPreviewLOD); // combo box list of lods is in reverse order
		mFMP->childSetValue("lod_file_" + lod_name[mPreviewLOD], mLODFile[mPreviewLOD]);

		LLComboBox* combo_box2 = mFMP->getChild<LLComboBox>("preview_lod_combo2");
		combo_box2->setCurrentByIndex((NUM_LOD-1)-mPreviewLOD); // combo box list of lods is in reverse order
		
		LLComboBox* combo_box3 = mFMP->getChild<LLComboBox>("preview_lod_combo3");
		combo_box3->setCurrentByIndex((NUM_LOD-1)-mPreviewLOD); // combo box list of lods is in reverse order

		LLColor4 highlight_color = LLUIColorTable::instance().getColor("MeshImportTableHighlightColor");
		LLColor4 normal_color = LLUIColorTable::instance().getColor("MeshImportTableNormalColor");

		for (S32 i = 0; i <= LLModel::LOD_HIGH; ++i)
		{
			const LLColor4& color = (i == lod) ? highlight_color : normal_color;

			mFMP->childSetColor(lod_status_name[i], color);
			mFMP->childSetColor(lod_label_name[i], color);
			mFMP->childSetColor(lod_triangles_name[i], color);
			mFMP->childSetColor(lod_vertices_name[i], color);
		}
	}
	refresh();
	updateStatusMessages();
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
	LLModelPreview* mp = fmp->mModelPreview;
	std::string filename = mp->mLODFile[LLModel::LOD_HIGH]; 

	fmp->resetDisplayOptions();
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

//static
void LLModelPreview::textureLoadedCallback(
    BOOL success,
    LLViewerFetchedTexture *src_vi,
    LLImageRaw* src,
    LLImageRaw* src_aux,
    S32 discard_level,
    BOOL final,
    void* userdata )
{
	LLModelPreview* preview = (LLModelPreview*) userdata;
	preview->refresh();

	if(final && preview->mModelLoader)
	{
		if(preview->mModelLoader->mNumOfFetchingTextures > 0)
		{
			preview->mModelLoader->mNumOfFetchingTextures-- ;
		}
	}
}

// static
bool LLModelPreview::lodQueryCallback()
{
    // not the best solution, but model preview belongs to floater
    // so it is an easy way to check that preview still exists.
    LLFloaterModelPreview* fmp = LLFloaterModelPreview::sInstance;
    if (fmp && fmp->mModelPreview)
    {
        LLModelPreview* preview = fmp->mModelPreview;
        if (preview->mLodsQuery.size() > 0)
        {
            S32 lod = preview->mLodsQuery.back();
            preview->mLodsQuery.pop_back();
            preview->genLODs(lod);

            // return false to continue cycle
            return false;
        }
    }
    // nothing to process
    return true;
}

void LLModelPreview::onLODParamCommit(S32 lod, bool enforce_tri_limit)
{
	if (!mLODFrozen)
	{
		genLODs(lod, 3, enforce_tri_limit);
		refresh();
	}
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

void LLFloaterModelPreview::setStatusMessage(const std::string& msg)
{
	LLMutexLock lock(mStatusLock);
	mStatusMessage = msg;
}

void LLFloaterModelPreview::toggleCalculateButton()
{
	toggleCalculateButton(true);
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
		childSetTextArg("upload_fee", "[FEE]", tbd);
		childSetTextArg("price_breakdown", "[STREAMING]", tbd);
		childSetTextArg("price_breakdown", "[PHYSICS]", tbd);
		childSetTextArg("price_breakdown", "[INSTANCES]", tbd);
		childSetTextArg("price_breakdown", "[TEXTURES]", tbd);
		childSetTextArg("price_breakdown", "[MODEL]", tbd);
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
	childSetVisible("upload_fee", true);
	childSetVisible("price_breakdown", true);
	mUploadBtn->setEnabled(isModelUploadAllowed());
}

void LLFloaterModelPreview::setModelPhysicsFeeErrorStatus(S32 status, const std::string& reason)
{
	LL_WARNS() << "LLFloaterModelPreview::setModelPhysicsFeeErrorStatus(" << status << " : " << reason << ")" << LL_ENDL;
	doOnIdleOneTime(boost::bind(&LLFloaterModelPreview::toggleCalculateButton, this, true));
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

