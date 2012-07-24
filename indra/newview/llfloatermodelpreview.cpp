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

#if LL_MSVC
#pragma warning (disable : 4263)
#pragma warning (disable : 4264)
#endif
#include "dae.h"
//#include "dom.h"
#include "dom/domAsset.h"
#include "dom/domBind_material.h"
#include "dom/domCOLLADA.h"
#include "dom/domConstants.h"
#include "dom/domController.h"
#include "dom/domEffect.h"
#include "dom/domGeometry.h"
#include "dom/domInstance_geometry.h"
#include "dom/domInstance_material.h"
#include "dom/domInstance_node.h"
#include "dom/domInstance_effect.h"
#include "dom/domMaterial.h"
#include "dom/domMatrix.h"
#include "dom/domNode.h"
#include "dom/domProfile_COMMON.h"
#include "dom/domRotate.h"
#include "dom/domScale.h"
#include "dom/domTranslate.h"
#include "dom/domVisual_scene.h"
#if LL_MSVC
#pragma warning (default : 4263)
#pragma warning (default : 4264)
#endif

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
#include "lldrawpoolavatar.h"
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


const S32 SLM_SUPPORTED_VERSION = 3;

//static
S32 LLFloaterModelPreview::sUploadAmount = 10;
LLFloaterModelPreview* LLFloaterModelPreview::sInstance = NULL;
std::list<LLModelLoader*> LLModelLoader::sActiveLoaderList;

const S32 PREVIEW_BORDER_WIDTH = 2;
const S32 PREVIEW_RESIZE_HANDLE_SIZE = S32(RESIZE_HANDLE_WIDTH * OO_SQRT2) + PREVIEW_BORDER_WIDTH;
const S32 PREVIEW_HPAD = PREVIEW_RESIZE_HANDLE_SIZE;
const S32 PREF_BUTTON_HEIGHT = 16 + 7 + 16;
const S32 PREVIEW_TEXTURE_HEIGHT = 300;

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

std::string colladaVersion[VERSIONTYPE_COUNT+1] = 
{
	"1.4.0",
	"1.4.1",
	"Unsupported"
};


#define LL_DEGENERACY_TOLERANCE  1e-7f

inline F32 dot3fpu(const LLVector4a& a, const LLVector4a& b)
{
    volatile F32 p0 = a[0] * b[0];
    volatile F32 p1 = a[1] * b[1];
    volatile F32 p2 = a[2] * b[2];
    return p0 + p1 + p2;
}

bool ll_is_degenerate(const LLVector4a& a, const LLVector4a& b, const LLVector4a& c, F32 tolerance = LL_DEGENERACY_TOLERANCE)
{
        // small area check
        {
                LLVector4a edge1; edge1.setSub( a, b );
                LLVector4a edge2; edge2.setSub( a, c );
                //////////////////////////////////////////////////////////////////////////
                /// Linden Modified
                //////////////////////////////////////////////////////////////////////////

                // If no one edge is more than 10x longer than any other edge, we weaken
                // the tolerance by a factor of 1e-4f.

                LLVector4a edge3; edge3.setSub( c, b );
				const F32 len1sq = edge1.dot3(edge1).getF32();
                const F32 len2sq = edge2.dot3(edge2).getF32();
                const F32 len3sq = edge3.dot3(edge3).getF32();
                bool abOK = (len1sq <= 100.f * len2sq) && (len1sq <= 100.f * len3sq);
                bool acOK = (len2sq <= 100.f * len1sq) && (len1sq <= 100.f * len3sq);
                bool cbOK = (len3sq <= 100.f * len1sq) && (len1sq <= 100.f * len2sq);
                if ( abOK && acOK && cbOK )
                {
                        tolerance *= 1e-4f;
                }

                //////////////////////////////////////////////////////////////////////////
                /// End Modified
                //////////////////////////////////////////////////////////////////////////

                LLVector4a cross; cross.setCross3( edge1, edge2 );

                LLVector4a edge1b; edge1b.setSub( b, a );
                LLVector4a edge2b; edge2b.setSub( b, c );
                LLVector4a crossb; crossb.setCross3( edge1b, edge2b );

                if ( ( cross.dot3(cross).getF32() < tolerance ) || ( crossb.dot3(crossb).getF32() < tolerance ))
                {
                        return true;
                }
        }

        // point triangle distance check
        {
                LLVector4a Q; Q.setSub(a, b);
                LLVector4a R; R.setSub(c, b);

                const F32 QQ = dot3fpu(Q, Q);
                const F32 RR = dot3fpu(R, R);
                const F32 QR = dot3fpu(R, Q);

                volatile F32 QQRR = QQ * RR;
                volatile F32 QRQR = QR * QR;
                F32 Det = (QQRR - QRQR);

                if( Det == 0.0f )
                {
                        return true;
                }
        }

        return false;
}

bool validate_face(const LLVolumeFace& face)
{
	for (U32 i = 0; i < face.mNumIndices; ++i)
	{
		if (face.mIndices[i] >= face.mNumVertices)
		{
			llwarns << "Face has invalid index." << llendl;
			return false;
		}
	}

	if (face.mNumIndices % 3 != 0 || face.mNumIndices == 0)
	{
		llwarns << "Face has invalid number of indices." << llendl;
		return false;
	}

	/*const LLVector4a scale(0.5f);

	for (U32 i = 0; i < face.mNumIndices; i+=3)
	{
		U16 idx1 = face.mIndices[i];
		U16 idx2 = face.mIndices[i+1];
		U16 idx3 = face.mIndices[i+2];

		LLVector4a v1; v1.setMul(face.mPositions[idx1], scale);
		LLVector4a v2; v2.setMul(face.mPositions[idx2], scale);
		LLVector4a v3; v3.setMul(face.mPositions[idx3], scale);

		if (ll_is_degenerate(v1,v2,v3))
		{
			llwarns << "Degenerate face found!" << llendl;
			return false;
		}
	}*/

	return true;
}

bool validate_model(const LLModel* mdl)
{
	if (mdl->getNumVolumeFaces() == 0)
	{
		llwarns << "Model has no faces!" << llendl;
		return false;
	}

	for (S32 i = 0; i < mdl->getNumVolumeFaces(); ++i)
	{
		if (mdl->getVolumeFace(i).mNumVertices == 0)
		{
			llwarns << "Face has no vertices." << llendl;
			return false;
		}

		if (mdl->getVolumeFace(i).mNumIndices == 0)
		{
			llwarns << "Face has no indices." << llendl;
			return false;
		}

		if (!validate_face(mdl->getVolumeFace(i)))
		{
			return false;
		}
	}

	return true;
}

BOOL stop_gloderror()
{
	GLuint error = glodGetError();

	if (error != GLOD_NO_ERROR)
	{
		llwarns << "GLOD error detected, cannot generate LOD: " << std::hex << error << llendl;
		return TRUE;
	}

	return FALSE;
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
	childSetCommitCallback("upload_textures", boost::bind(&LLFloaterModelPreview::toggleCalculateButton, this), NULL);

	childSetTextArg("status", "[STATUS]", getString("status_idle"));

	childSetAction("ok_btn", onUpload, this);
	childDisable("ok_btn");

	childSetAction("reset_btn", onReset, this);

	childSetCommitCallback("preview_lod_combo", onPreviewLODCommit, this);

	childSetCommitCallback("upload_skin", onUploadSkinCommit, this);
	childSetCommitCallback("upload_joints", onUploadJointsCommit, this);

	childSetCommitCallback("import_scale", onImportScaleCommit, this);
	childSetCommitCallback("pelvis_offset", onPelvisOffsetCommit, this);

	getChild<LLCheckBoxCtrl>("show_edges")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onViewOptionChecked, this, _1));
	getChild<LLCheckBoxCtrl>("show_physics")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onViewOptionChecked, this, _1));
	getChild<LLCheckBoxCtrl>("show_textures")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onViewOptionChecked, this, _1));
	getChild<LLCheckBoxCtrl>("show_skin_weight")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onViewOptionChecked, this, _1));
	getChild<LLCheckBoxCtrl>("show_joint_positions")->setCommitCallback(boost::bind(&LLFloaterModelPreview::onViewOptionChecked, this, _1));

	childDisable("upload_skin");
	childDisable("upload_joints");

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
			text->setMouseDownCallback(boost::bind(&LLModelPreview::setPreviewLOD, mModelPreview, i));
		}

		text = getChild<LLTextBox>(lod_triangles_name[i]);
		if (text)
		{
			text->setMouseDownCallback(boost::bind(&LLModelPreview::setPreviewLOD, mModelPreview, i));
		}

		text = getChild<LLTextBox>(lod_vertices_name[i]);
		if (text)
		{
			text->setMouseDownCallback(boost::bind(&LLModelPreview::setPreviewLOD, mModelPreview, i));
		}

		text = getChild<LLTextBox>(lod_status_name[i]);
		if (text)
		{
			text->setMouseDownCallback(boost::bind(&LLModelPreview::setPreviewLOD, mModelPreview, i));
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

	mCalculateBtn->setClickedCallback(boost::bind(&LLFloaterModelPreview::onClickCalculateBtn, this));

	toggleCalculateButton(true);

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
	return childIsEnabled(userdata.asString());
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

	mUploadModelUrl.clear();

	gMeshRepo.uploadModel(mModelPreview->mUploadData, mModelPreview->mPreviewScale,
			childGetValue("upload_textures").asBoolean(), upload_skinweights, upload_joint_positions, mUploadModelUrl, false,
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

	fp->mModelPreview->genLODs();
}

void LLFloaterModelPreview::onLODParamCommit(S32 lod, bool enforce_tri_limit)
{
	mModelPreview->onLODParamCommit(lod, enforce_tri_limit);
}


//-----------------------------------------------------------------------------
// draw()
//-----------------------------------------------------------------------------
void LLFloaterModelPreview::draw()
{
	LLFloater::draw();
	LLRect r = getRect();

	mModelPreview->update();

	if (!mModelPreview->mLoading)
	{
		if ( mModelPreview->getLoadState() == LLModelLoader::ERROR_MATERIALS )
		{
			childSetTextArg("status", "[STATUS]", getString("status_material_mismatch"));
		}
		else
		if ( mModelPreview->getLoadState() > LLModelLoader::ERROR_PARSING )
		{		
			childSetTextArg("status", "[STATUS]", getString(LLModel::getStatusString(mModelPreview->getLoadState() - LLModelLoader::ERROR_PARSING)));
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

	if (mModelPreview)
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
	requestAgentUploadPermissions();
}

//static
void LLFloaterModelPreview::onPhysicsParamCommit(LLUICtrl* ctrl, void* data)
{
	if (LLConvexDecomposition::getInstance() == NULL)
	{
		llinfos << "convex decomposition tool is a stub on this platform. cannot get decomp." << llendl;
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
			llinfos << "Decomposition request still pending." << llendl;
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
		llwarns << "no iface" << llendl;
		return;
	}

	if (which_mode <= 0)
	{
		llwarns << "which_mode out of range, " << which_mode << llendl;
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

		//llinfos << "Physics decomp stage " << stage[j].mName << " (" << j << ") parameters:" << llendl;
		//llinfos << "------------------------------------" << llendl;

		for (S32 i = 0; i < param_count; ++i)
		{
			if (param[i].mStage != j)
			{
				continue;
			}

			std::string name(param[i].mName ? param[i].mName : "");
			std::string description(param[i].mDescription ? param[i].mDescription : "");

			std::string type = "unknown";

			llinfos << name << " - " << description << llendl;

			if (param[i].mType == LLCDParam::LLCD_FLOAT)
			{
				mDecompParams[param[i].mName] = LLSD(param[i].mDefault.mFloat);
				//llinfos << "Type: float, Default: " << param[i].mDefault.mFloat << llendl;


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
				//llinfos << "Type: integer, Default: " << param[i].mDefault.mIntOrEnumValue << llendl;


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
				//llinfos << "Type: boolean, Default: " << (param[i].mDefault.mBool ? "True" : "False") << llendl;

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
				//llinfos << "Type: enum, Default: " << param[i].mDefault.mIntOrEnumValue << llendl;

				{ //plug into combo box

					//llinfos << "Accepted values: " << llendl;
					LLComboBox* combo_box = getChild<LLComboBox>(name);
					for (S32 k = 0; k < param[i].mDetails.mEnumValues.mNumEnums; ++k)
					{
						//llinfos << param[i].mDetails.mEnumValues.mEnumsArray[k].mValue
						//	<< " - " << param[i].mDetails.mEnumValues.mEnumsArray[k].mName << llendl;

						std::string name(param[i].mDetails.mEnumValues.mEnumsArray[k].mName);
						std::string localized_name;
						bool is_localized = LLTrans::findString(localized_name, name);

						combo_box->add(is_localized ? localized_name : name,
							LLSD::Integer(param[i].mDetails.mEnumValues.mEnumsArray[k].mValue));
					}
					combo_box->setValue(param[i].mDefault.mIntOrEnumValue);
					combo_box->setCommitCallback(onPhysicsParamCommit, (void*) &param[i]);
				}

				//llinfos << "----" << llendl;
			}
			//llinfos << "-----------------------------" << llendl;
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
// LLModelLoader
//-----------------------------------------------------------------------------
LLModelLoader::LLModelLoader( std::string filename, S32 lod, LLModelPreview* preview, JointTransformMap& jointMap, 
							  std::deque<std::string>& jointsFromNodes )
: mJointList( jointMap )
, mJointsFromNode( jointsFromNodes )
, LLThread("Model Loader"), mFilename(filename), mLod(lod), mPreview(preview), mFirstTransform(TRUE), mNumOfFetchingTextures(0)
{
	mJointMap["mPelvis"] = "mPelvis";
	mJointMap["mTorso"] = "mTorso";
	mJointMap["mChest"] = "mChest";
	mJointMap["mNeck"] = "mNeck";
	mJointMap["mHead"] = "mHead";
	mJointMap["mSkull"] = "mSkull";
	mJointMap["mEyeRight"] = "mEyeRight";
	mJointMap["mEyeLeft"] = "mEyeLeft";
	mJointMap["mCollarLeft"] = "mCollarLeft";
	mJointMap["mShoulderLeft"] = "mShoulderLeft";
	mJointMap["mElbowLeft"] = "mElbowLeft";
	mJointMap["mWristLeft"] = "mWristLeft";
	mJointMap["mCollarRight"] = "mCollarRight";
	mJointMap["mShoulderRight"] = "mShoulderRight";
	mJointMap["mElbowRight"] = "mElbowRight";
	mJointMap["mWristRight"] = "mWristRight";
	mJointMap["mHipRight"] = "mHipRight";
	mJointMap["mKneeRight"] = "mKneeRight";
	mJointMap["mAnkleRight"] = "mAnkleRight";
	mJointMap["mFootRight"] = "mFootRight";
	mJointMap["mToeRight"] = "mToeRight";
	mJointMap["mHipLeft"] = "mHipLeft";
	mJointMap["mKneeLeft"] = "mKneeLeft";
	mJointMap["mAnkleLeft"] = "mAnkleLeft";
	mJointMap["mFootLeft"] = "mFootLeft";
	mJointMap["mToeLeft"] = "mToeLeft";

	mJointMap["avatar_mPelvis"] = "mPelvis";
	mJointMap["avatar_mTorso"] = "mTorso";
	mJointMap["avatar_mChest"] = "mChest";
	mJointMap["avatar_mNeck"] = "mNeck";
	mJointMap["avatar_mHead"] = "mHead";
	mJointMap["avatar_mSkull"] = "mSkull";
	mJointMap["avatar_mEyeRight"] = "mEyeRight";
	mJointMap["avatar_mEyeLeft"] = "mEyeLeft";
	mJointMap["avatar_mCollarLeft"] = "mCollarLeft";
	mJointMap["avatar_mShoulderLeft"] = "mShoulderLeft";
	mJointMap["avatar_mElbowLeft"] = "mElbowLeft";
	mJointMap["avatar_mWristLeft"] = "mWristLeft";
	mJointMap["avatar_mCollarRight"] = "mCollarRight";
	mJointMap["avatar_mShoulderRight"] = "mShoulderRight";
	mJointMap["avatar_mElbowRight"] = "mElbowRight";
	mJointMap["avatar_mWristRight"] = "mWristRight";
	mJointMap["avatar_mHipRight"] = "mHipRight";
	mJointMap["avatar_mKneeRight"] = "mKneeRight";
	mJointMap["avatar_mAnkleRight"] = "mAnkleRight";
	mJointMap["avatar_mFootRight"] = "mFootRight";
	mJointMap["avatar_mToeRight"] = "mToeRight";
	mJointMap["avatar_mHipLeft"] = "mHipLeft";
	mJointMap["avatar_mKneeLeft"] = "mKneeLeft";
	mJointMap["avatar_mAnkleLeft"] = "mAnkleLeft";
	mJointMap["avatar_mFootLeft"] = "mFootLeft";
	mJointMap["avatar_mToeLeft"] = "mToeLeft";


	mJointMap["hip"] = "mPelvis";
	mJointMap["abdomen"] = "mTorso";
	mJointMap["chest"] = "mChest";
	mJointMap["neck"] = "mNeck";
	mJointMap["head"] = "mHead";
	mJointMap["figureHair"] = "mSkull";
	mJointMap["lCollar"] = "mCollarLeft";
	mJointMap["lShldr"] = "mShoulderLeft";
	mJointMap["lForeArm"] = "mElbowLeft";
	mJointMap["lHand"] = "mWristLeft";
	mJointMap["rCollar"] = "mCollarRight";
	mJointMap["rShldr"] = "mShoulderRight";
	mJointMap["rForeArm"] = "mElbowRight";
	mJointMap["rHand"] = "mWristRight";
	mJointMap["rThigh"] = "mHipRight";
	mJointMap["rShin"] = "mKneeRight";
	mJointMap["rFoot"] = "mFootRight";
	mJointMap["lThigh"] = "mHipLeft";
	mJointMap["lShin"] = "mKneeLeft";
	mJointMap["lFoot"] = "mFootLeft";

	if (mPreview)
	{
		//only try to load from slm if viewer is configured to do so and this is the 
		//initial model load (not an LoD or physics shape)
		mTrySLM = gSavedSettings.getBOOL("MeshImportUseSLM") && mPreview->mUploadData.empty();
		mPreview->setLoadState(STARTING);
	}
	else
	{
		mTrySLM = false;
	}

	assert_main_thread();
	sActiveLoaderList.push_back(this) ;
}

LLModelLoader::~LLModelLoader()
{
	assert_main_thread();
	sActiveLoaderList.remove(this);
}

void stretch_extents(LLModel* model, LLMatrix4a& mat, LLVector4a& min, LLVector4a& max, BOOL& first_transform)
{
	LLVector4a box[] =
	{
		LLVector4a(-1, 1,-1),
		LLVector4a(-1, 1, 1),
		LLVector4a(-1,-1,-1),
		LLVector4a(-1,-1, 1),
		LLVector4a( 1, 1,-1),
		LLVector4a( 1, 1, 1),
		LLVector4a( 1,-1,-1),
		LLVector4a( 1,-1, 1),
	};

	for (S32 j = 0; j < model->getNumVolumeFaces(); ++j)
	{
		const LLVolumeFace& face = model->getVolumeFace(j);

		LLVector4a center;
		center.setAdd(face.mExtents[0], face.mExtents[1]);
		center.mul(0.5f);
		LLVector4a size;
		size.setSub(face.mExtents[1],face.mExtents[0]);
		size.mul(0.5f);

		for (U32 i = 0; i < 8; i++)
		{
			LLVector4a t;
			t.setMul(size, box[i]);
			t.add(center);

			LLVector4a v;

			mat.affineTransform(t, v);

			if (first_transform)
			{
				first_transform = FALSE;
				min = max = v;
			}
			else
			{
				update_min_max(min, max, v);
			}
		}
	}
}

void stretch_extents(LLModel* model, LLMatrix4& mat, LLVector3& min, LLVector3& max, BOOL& first_transform)
{
	LLVector4a mina, maxa;
	LLMatrix4a mata;

	mata.loadu(mat);
	mina.load3(min.mV);
	maxa.load3(max.mV);

	stretch_extents(model, mata, mina, maxa, first_transform);

	min.set(mina.getF32ptr());
	max.set(maxa.getF32ptr());
}

void LLModelLoader::run()
{
	doLoadModel();
	doOnIdleOneTime(boost::bind(&LLModelLoader::loadModelCallback,this));
}

bool LLModelLoader::doLoadModel()
{
	//first, look for a .slm file of the same name that was modified later
	//than the .dae

	if (mTrySLM)
	{
		std::string filename = mFilename;
			
		std::string::size_type i = filename.rfind(".");
		if (i != std::string::npos)
		{
			filename.replace(i, filename.size()-1, ".slm");
			llstat slm_status;
			if (LLFile::stat(filename, &slm_status) == 0)
			{ //slm file exists
				llstat dae_status;
				if (LLFile::stat(mFilename, &dae_status) != 0 ||
					dae_status.st_mtime < slm_status.st_mtime)
				{
					if (loadFromSLM(filename))
					{ //slm successfully loaded, if this fails, fall through and
						//try loading from dae

						mLod = -1; //successfully loading from an slm implicitly sets all 
									//LoDs
						return true;
					}
				}
			}	
		}
	}

	//no suitable slm exists, load from the .dae file
	DAE dae;
	domCOLLADA* dom = dae.open(mFilename);
	
	if (!dom)
	{
		llinfos<<" Error with dae - traditionally indicates a corrupt file."<<llendl;
		setLoadState( ERROR_PARSING );
		return false;
	}
	//Dom version
	daeString domVersion = dae.getDomVersion();
	std::string sldom(domVersion);
	llinfos<<"Collada Importer Version: "<<sldom<<llendl;
	//Dae version
	domVersionType docVersion = dom->getVersion();
	//0=1.4
	//1=1.4.1
	//2=Currently unsupported, however may work
	if (docVersion > 1 ) 
	{ 
		docVersion = VERSIONTYPE_COUNT;
	}
	llinfos<<"Dae version "<<colladaVersion[docVersion]<<llendl;
	
	
	daeDatabase* db = dae.getDatabase();
	
	daeInt count = db->getElementCount(NULL, COLLADA_TYPE_MESH);
	
	daeDocument* doc = dae.getDoc(mFilename);
	if (!doc)
	{
		llwarns << "can't find internal doc" << llendl;
		return false;
	}
	
	daeElement* root = doc->getDomRoot();
	if (!root)
	{
		llwarns << "document has no root" << llendl;
		return false;
	}
	
	//Verify some basic properties of the dae
	//1. Basic validity check on controller 
	U32 controllerCount = (int) db->getElementCount( NULL, "controller" );
	bool result = false;
	for ( int i=0; i<controllerCount; ++i )
	{
		domController* pController = NULL;
		db->getElement( (daeElement**) &pController, i , NULL, "controller" );
		result = mPreview->verifyController( pController );
		if (!result)
		{
			setLoadState( ERROR_PARSING );
			return true;
		}
	}


	//get unit scale
	mTransform.setIdentity();
	
	domAsset::domUnit* unit = daeSafeCast<domAsset::domUnit>(root->getDescendant(daeElement::matchType(domAsset::domUnit::ID())));
	
	if (unit)
	{
		F32 meter = unit->getMeter();
		mTransform.mMatrix[0][0] = meter;
		mTransform.mMatrix[1][1] = meter;
		mTransform.mMatrix[2][2] = meter;
	}
	
	//get up axis rotation
	LLMatrix4 rotation;
	
	domUpAxisType up = UPAXISTYPE_Y_UP;  // default is Y_UP
	domAsset::domUp_axis* up_axis =
	daeSafeCast<domAsset::domUp_axis>(root->getDescendant(daeElement::matchType(domAsset::domUp_axis::ID())));
	
	if (up_axis)
	{
		up = up_axis->getValue();
	}
	
	if (up == UPAXISTYPE_X_UP)
	{
		rotation.initRotation(0.0f, 90.0f * DEG_TO_RAD, 0.0f);
	}
	else if (up == UPAXISTYPE_Y_UP)
	{
		rotation.initRotation(90.0f * DEG_TO_RAD, 0.0f, 0.0f);
	}
	
	rotation *= mTransform;
	mTransform = rotation;
	
	
	for (daeInt idx = 0; idx < count; ++idx)
	{ //build map of domEntities to LLModel
		domMesh* mesh = NULL;
		db->getElement((daeElement**) &mesh, idx, NULL, COLLADA_TYPE_MESH);
		
		if (mesh)
		{
			LLPointer<LLModel> model = LLModel::loadModelFromDomMesh(mesh);
			
			if(model->getStatus() != LLModel::NO_ERRORS)
			{
				setLoadState(ERROR_PARSING + model->getStatus()) ;
				return false; //abort
			}

			if (model.notNull() && validate_model(model))
			{
				mModelList.push_back(model);
				mModel[mesh] = model;
			}
		}
	}
	
	count = db->getElementCount(NULL, COLLADA_TYPE_SKIN);
	for (daeInt idx = 0; idx < count; ++idx)
	{ //add skinned meshes as instances
		domSkin* skin = NULL;
		db->getElement((daeElement**) &skin, idx, NULL, COLLADA_TYPE_SKIN);
		
		if (skin)
		{
			domGeometry* geom = daeSafeCast<domGeometry>(skin->getSource().getElement());
			
			if (geom)
			{
				domMesh* mesh = geom->getMesh();
				if (mesh)
				{
					LLModel* model = mModel[mesh];
					if (model)
					{
						LLVector3 mesh_scale_vector;
						LLVector3 mesh_translation_vector;
						model->getNormalizedScaleTranslation(mesh_scale_vector, mesh_translation_vector);
						
						LLMatrix4 normalized_transformation;
						normalized_transformation.setTranslation(mesh_translation_vector);
						
						LLMatrix4 mesh_scale;
						mesh_scale.initScale(mesh_scale_vector);
						mesh_scale *= normalized_transformation;
						normalized_transformation = mesh_scale;
						
						glh::matrix4f inv_mat((F32*) normalized_transformation.mMatrix);
						inv_mat = inv_mat.inverse();
						LLMatrix4 inverse_normalized_transformation(inv_mat.m);
						
						domSkin::domBind_shape_matrix* bind_mat = skin->getBind_shape_matrix();
						
						if (bind_mat)
						{ //get bind shape matrix
							domFloat4x4& dom_value = bind_mat->getValue();
							
							LLMeshSkinInfo& skin_info = model->mSkinInfo;

							for (int i = 0; i < 4; i++)
							{
								for(int j = 0; j < 4; j++)
								{
									skin_info.mBindShapeMatrix.mMatrix[i][j] = dom_value[i + j*4];
								}
							}
							
							LLMatrix4 trans = normalized_transformation;
							trans *= skin_info.mBindShapeMatrix;
							skin_info.mBindShapeMatrix = trans;							
						}
										
											
						//Some collada setup for accessing the skeleton
						daeElement* pElement = 0;
						dae.getDatabase()->getElement( &pElement, 0, 0, "skeleton" );
						
						//Try to get at the skeletal instance controller
						domInstance_controller::domSkeleton* pSkeleton = daeSafeCast<domInstance_controller::domSkeleton>( pElement );
						bool missingSkeletonOrScene = false;
						
						//If no skeleton, do a breadth-first search to get at specific joints
						bool rootNode = false;
						bool skeletonWithNoRootNode = false;
						
						//Need to test for a skeleton that does not have a root node
						//This occurs when your instance controller does not have an associated scene 
						if ( pSkeleton )
						{
							daeElement* pSkeletonRootNode = pSkeleton->getValue().getElement();
							if ( pSkeletonRootNode )
							{
								rootNode = true;
							}
							else 
							{
								skeletonWithNoRootNode = true;
							}

						}
						if ( !pSkeleton || !rootNode )
						{
							daeElement* pScene = root->getDescendant("visual_scene");
							if ( !pScene )
							{
								llwarns<<"No visual scene - unable to parse bone offsets "<<llendl;
								missingSkeletonOrScene = true;
							}
							else
							{
								//Get the children at this level
								daeTArray< daeSmartRef<daeElement> > children = pScene->getChildren();
								S32 childCount = children.getCount();
								
								//Process any children that are joints
								//Not all children are joints, some code be ambient lights, cameras, geometry etc..
								for (S32 i = 0; i < childCount; ++i)
								{
									domNode* pNode = daeSafeCast<domNode>(children[i]);
									if ( isNodeAJoint( pNode ) )
									{
										processJointNode( pNode, mJointList );
									}
								}
							}
						}
						else
							//Has Skeleton
						{
							//Get the root node of the skeleton
							daeElement* pSkeletonRootNode = pSkeleton->getValue().getElement();
							if ( pSkeletonRootNode )
							{
								//Once we have the root node - start acccessing it's joint components
								const int jointCnt = mJointMap.size();
								std::map<std::string, std::string> :: const_iterator jointIt = mJointMap.begin();
								
								//Loop over all the possible joints within the .dae - using the allowed joint list in the ctor.
								for ( int i=0; i<jointCnt; ++i, ++jointIt )
								{
									//Build a joint for the resolver to work with
									char str[64]={0};
									sprintf(str,"./%s",(*jointIt).first.c_str() );
									//llwarns<<"Joint "<< str <<llendl;
									
									//Setup the resolver
                                    daeSIDResolver resolver( pSkeletonRootNode, str );
									
                                    //Look for the joint
                                    domNode* pJoint = daeSafeCast<domNode>( resolver.getElement() );
                                    if ( pJoint )
                                    {
										//Pull out the translate id and store it in the jointTranslations map
										daeSIDResolver jointResolverA( pJoint, "./translate" );
										domTranslate* pTranslateA = daeSafeCast<domTranslate>( jointResolverA.getElement() );
										daeSIDResolver jointResolverB( pJoint, "./location" );
										domTranslate* pTranslateB = daeSafeCast<domTranslate>( jointResolverB.getElement() );
										
										LLMatrix4 workingTransform;
										
										//Translation via SID
										if ( pTranslateA )
										{
											extractTranslation( pTranslateA, workingTransform );
										}
										else
										if ( pTranslateB )
										{
											extractTranslation( pTranslateB, workingTransform );
										}
										else
										{
											//Translation via child from element
											daeElement* pTranslateElement = getChildFromElement( pJoint, "translate" );
											if ( pTranslateElement && pTranslateElement->typeID() != domTranslate::ID() )
											{
												llwarns<< "The found element is not a translate node" <<llendl;
												missingSkeletonOrScene = true;
											}
											else
											if ( pTranslateElement )
											{
												extractTranslationViaElement( pTranslateElement, workingTransform );
											}
											else
											{
												extractTranslationViaSID( pJoint, workingTransform );
											}

										}
										
										//Store the joint transform w/respect to it's name.
										mJointList[(*jointIt).second.c_str()] = workingTransform;
                                    }
								}
								
								//If anything failed in regards to extracting the skeleton, joints or translation id,
								//mention it
								if ( missingSkeletonOrScene  )
								{
									llwarns<< "Partial jointmap found in asset - did you mean to just have a partial map?" << llendl;
								}
							}//got skeleton?
						}
						
						
						domSkin::domJoints* joints = skin->getJoints();
						
						domInputLocal_Array& joint_input = joints->getInput_array();
						
						for (size_t i = 0; i < joint_input.getCount(); ++i)
						{
							domInputLocal* input = joint_input.get(i);
							xsNMTOKEN semantic = input->getSemantic();
							
							if (strcmp(semantic, COMMON_PROFILE_INPUT_JOINT) == 0)
							{ //found joint source, fill model->mJointMap and model->mSkinInfo.mJointNames
								daeElement* elem = input->getSource().getElement();
								
								domSource* source = daeSafeCast<domSource>(elem);
								if (source)
								{
									
									
									domName_array* names_source = source->getName_array();
									
									if (names_source)
									{
										domListOfNames &names = names_source->getValue();
										
										for (size_t j = 0; j < names.getCount(); ++j)
										{
											std::string name(names.get(j));
											if (mJointMap.find(name) != mJointMap.end())
											{
												name = mJointMap[name];
											}
											model->mSkinInfo.mJointNames.push_back(name);
											model->mSkinInfo.mJointMap[name] = j;
										}
									}
									else
									{
										domIDREF_array* names_source = source->getIDREF_array();
										if (names_source)
										{
											xsIDREFS& names = names_source->getValue();
											
											for (size_t j = 0; j < names.getCount(); ++j)
											{
												std::string name(names.get(j).getID());
												if (mJointMap.find(name) != mJointMap.end())
												{
													name = mJointMap[name];
												}
												model->mSkinInfo.mJointNames.push_back(name);
												model->mSkinInfo.mJointMap[name] = j;
											}
										}
									}
								}
							}
							else if (strcmp(semantic, COMMON_PROFILE_INPUT_INV_BIND_MATRIX) == 0)
							{ //found inv_bind_matrix array, fill model->mInvBindMatrix
								domSource* source = daeSafeCast<domSource>(input->getSource().getElement());
								if (source)
								{
									domFloat_array* t = source->getFloat_array();
									if (t)
									{
										domListOfFloats& transform = t->getValue();
										S32 count = transform.getCount()/16;
										
										for (S32 k = 0; k < count; ++k)
										{
											LLMatrix4 mat;
											
											for (int i = 0; i < 4; i++)
											{
												for(int j = 0; j < 4; j++)
												{
													mat.mMatrix[i][j] = transform[k*16 + i + j*4];
												}
											}
											
											model->mSkinInfo.mInvBindMatrix.push_back(mat);											
										}
									}
								}
							}
						}
						
						//Now that we've parsed the joint array, let's determine if we have a full rig
						//(which means we have all the joint sthat are required for an avatar versus
						//a skinned asset attached to a node in a file that contains an entire skeleton,
						//but does not use the skeleton).						
						buildJointToNodeMappingFromScene( root );
						mPreview->critiqueRigForUploadApplicability( model->mSkinInfo.mJointNames );
										
						if ( !missingSkeletonOrScene )
						{
							//Set the joint translations on the avatar - if it's a full mapping
							//The joints are reset in the dtor
							if ( mPreview->getRigWithSceneParity() )
							{	
								std::map<std::string, std::string> :: const_iterator masterJointIt = mJointMap.begin();
								std::map<std::string, std::string> :: const_iterator masterJointItEnd = mJointMap.end();
								for (;masterJointIt!=masterJointItEnd;++masterJointIt )
								{
									std::string lookingForJoint = (*masterJointIt).first.c_str();
									
									if ( mJointList.find( lookingForJoint ) != mJointList.end() )
									{
										//llinfos<<"joint "<<lookingForJoint.c_str()<<llendl;
										LLMatrix4 jointTransform = mJointList[lookingForJoint];
										LLJoint* pJoint = mPreview->getPreviewAvatar()->getJoint( lookingForJoint );
										if ( pJoint )
										{   
											pJoint->storeCurrentXform( jointTransform.getTranslation() );												
										}
										else
										{
											//Most likely an error in the asset.
											llwarns<<"Tried to apply joint position from .dae, but it did not exist in the avatar rig." << llendl;
										}
									}
								}
							}
						} //missingSkeletonOrScene
						
						
						//We need to construct the alternate bind matrix (which contains the new joint positions)
						//in the same order as they were stored in the joint buffer. The joints associated
						//with the skeleton are not stored in the same order as they are in the exported joint buffer.
						//This remaps the skeletal joints to be in the same order as the joints stored in the model.
						std::vector<std::string> :: const_iterator jointIt  = model->mSkinInfo.mJointNames.begin();
						const int jointCnt = model->mSkinInfo.mJointNames.size();
						for ( int i=0; i<jointCnt; ++i, ++jointIt )
						{
							std::string lookingForJoint = (*jointIt).c_str();
							//Look for the joint xform that we extracted from the skeleton, using the jointIt as the key
							//and store it in the alternate bind matrix
							if ( mJointList.find( lookingForJoint ) != mJointList.end() )
							{
								LLMatrix4 jointTransform = mJointList[lookingForJoint];
								LLMatrix4 newInverse = model->mSkinInfo.mInvBindMatrix[i];
								newInverse.setTranslation( mJointList[lookingForJoint].getTranslation() );
								model->mSkinInfo.mAlternateBindMatrix.push_back( newInverse );
							}
							else
							{
								llwarns<<"Possibly misnamed/missing joint [" <<lookingForJoint.c_str()<<" ] "<<llendl;
							}
						}
						
						//grab raw position array
						
						domVertices* verts = mesh->getVertices();
						if (verts)
						{
							domInputLocal_Array& inputs = verts->getInput_array();
							for (size_t i = 0; i < inputs.getCount() && model->mPosition.empty(); ++i)
							{
								if (strcmp(inputs[i]->getSemantic(), COMMON_PROFILE_INPUT_POSITION) == 0)
								{
									domSource* pos_source = daeSafeCast<domSource>(inputs[i]->getSource().getElement());
									if (pos_source)
									{
										domFloat_array* pos_array = pos_source->getFloat_array();
										if (pos_array)
										{
											domListOfFloats& pos = pos_array->getValue();
											
											for (size_t j = 0; j < pos.getCount(); j += 3)
											{
												if (pos.getCount() <= j+2)
												{
													llerrs << "Invalid position array size." << llendl;
												}
												
												LLVector3 v(pos[j], pos[j+1], pos[j+2]);
												
												//transform from COLLADA space to volume space
												v = v * inverse_normalized_transformation;
												
												model->mPosition.push_back(v);
											}
										}
									}
								}
							}
						}
						
						//grab skin weights array
						domSkin::domVertex_weights* weights = skin->getVertex_weights();
						if (weights)
						{
							domInputLocalOffset_Array& inputs = weights->getInput_array();
							domFloat_array* vertex_weights = NULL;
							for (size_t i = 0; i < inputs.getCount(); ++i)
							{
								if (strcmp(inputs[i]->getSemantic(), COMMON_PROFILE_INPUT_WEIGHT) == 0)
								{
									domSource* weight_source = daeSafeCast<domSource>(inputs[i]->getSource().getElement());
									if (weight_source)
									{
										vertex_weights = weight_source->getFloat_array();
									}
								}
							}
							
							if (vertex_weights)
							{
								domListOfFloats& w = vertex_weights->getValue();
								domListOfUInts& vcount = weights->getVcount()->getValue();
								domListOfInts& v = weights->getV()->getValue();
								
								U32 c_idx = 0;
								for (size_t vc_idx = 0; vc_idx < vcount.getCount(); ++vc_idx)
								{ //for each vertex
									daeUInt count = vcount[vc_idx];
									
									//create list of weights that influence this vertex
									LLModel::weight_list weight_list;
									
									for (daeUInt i = 0; i < count; ++i)
									{ //for each weight
										daeInt joint_idx = v[c_idx++];
										daeInt weight_idx = v[c_idx++];
										
										if (joint_idx == -1)
										{
											//ignore bindings to bind_shape_matrix
											continue;
										}
										
										F32 weight_value = w[weight_idx];
										
										weight_list.push_back(LLModel::JointWeight(joint_idx, weight_value));
									}
									
									//sort by joint weight
									std::sort(weight_list.begin(), weight_list.end(), LLModel::CompareWeightGreater());
									
									std::vector<LLModel::JointWeight> wght;
									
									F32 total = 0.f;
									
									for (U32 i = 0; i < llmin((U32) 4, (U32) weight_list.size()); ++i)
									{ //take up to 4 most significant weights
										if (weight_list[i].mWeight > 0.f)
										{
											wght.push_back( weight_list[i] );
											total += weight_list[i].mWeight;
										}
									}
									
									F32 scale = 1.f/total;
									if (scale != 1.f)
									{ //normalize weights
										for (U32 i = 0; i < wght.size(); ++i)
										{
											wght[i].mWeight *= scale;
										}
									}
									
									model->mSkinWeights[model->mPosition[vc_idx]] = wght;
								}
								
								//add instance to scene for this model
								
								LLMatrix4 transformation = mTransform;
								// adjust the transformation to compensate for mesh normalization
								
								LLMatrix4 mesh_translation;
								mesh_translation.setTranslation(mesh_translation_vector);
								mesh_translation *= transformation;
								transformation = mesh_translation;
								
								LLMatrix4 mesh_scale;
								mesh_scale.initScale(mesh_scale_vector);
								mesh_scale *= transformation;
								transformation = mesh_scale;
								
								std::map<std::string, LLImportMaterial> materials;
								for (U32 i = 0; i < model->mMaterialList.size(); ++i)
								{
									materials[model->mMaterialList[i]] = LLImportMaterial();
								}
								mScene[transformation].push_back(LLModelInstance(model, model->mLabel, transformation, materials));
								stretch_extents(model, transformation, mExtents[0], mExtents[1], mFirstTransform);
							}
						}
					}
				}
			}
		}
	}
	
	daeElement* scene = root->getDescendant("visual_scene");
	
	if (!scene)
	{
		llwarns << "document has no visual_scene" << llendl;
		setLoadState( ERROR_PARSING );
		return true;
	}
	
	setLoadState( DONE );

	bool badElement = false;
	
	processElement( scene, badElement );
	
	if ( badElement )
	{
		setLoadState( ERROR_PARSING );
	}
	
	return true;
}

void LLModelLoader::setLoadState(U32 state)
{
	if (mPreview)
	{
		mPreview->setLoadState(state);
	}
}

bool LLModelLoader::loadFromSLM(const std::string& filename)
{ 
	//only need to populate mScene with data from slm
	llstat stat;

	if (LLFile::stat(filename, &stat))
	{ //file does not exist
		return false;
	}

	S32 file_size = (S32) stat.st_size;
	
	llifstream ifstream(filename, std::ifstream::in | std::ifstream::binary);
	LLSD data;
	LLSDSerialize::fromBinary(data, ifstream, file_size);
	ifstream.close();

	//build model list for each LoD
	model_list model[LLModel::NUM_LODS];

	if (data["version"].asInteger() != SLM_SUPPORTED_VERSION)
	{  //unsupported version
		return false;
	}

	LLSD& mesh = data["mesh"];

	LLVolumeParams volume_params;
	volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);

	for (S32 lod = 0; lod < LLModel::NUM_LODS; ++lod)
	{
		for (U32 i = 0; i < mesh.size(); ++i)
		{
			std::stringstream str(mesh[i].asString());
			LLPointer<LLModel> loaded_model = new LLModel(volume_params, (F32) lod);
			if (loaded_model->loadModel(str))
			{
				loaded_model->mLocalID = i;
				model[lod].push_back(loaded_model);

				if (lod == LLModel::LOD_HIGH && !loaded_model->mSkinInfo.mJointNames.empty())
				{ 
					//check to see if rig is valid					
					mPreview->critiqueRigForUploadApplicability( loaded_model->mSkinInfo.mJointNames );					
				}
			}
		}
	}	

	if (model[LLModel::LOD_HIGH].empty())
	{ //failed to load high lod
		return false;
	}

	// Set name.
	std::string name = data["name"];
	if (!name.empty())
	{
		model[LLModel::LOD_HIGH][0]->mLabel = name;
	}
	

	//load instance list
	model_instance_list instance_list;

	LLSD& instance = data["instance"];

	for (U32 i = 0; i < instance.size(); ++i)
	{
		//deserialize instance list
		instance_list.push_back(LLModelInstance(instance[i]));

		//match up model instance pointers
		S32 idx = instance_list[i].mLocalMeshID;

		for (U32 lod = 0; lod < LLModel::NUM_LODS; ++lod)
		{
			if (!model[lod].empty())
			{
				instance_list[i].mLOD[lod] = model[lod][idx];
			}
		}

		instance_list[i].mModel = model[LLModel::LOD_HIGH][idx];
	}


	//convert instance_list to mScene
	mFirstTransform = TRUE;
	for (U32 i = 0; i < instance_list.size(); ++i)
	{
		LLModelInstance& cur_instance = instance_list[i];
		mScene[cur_instance.mTransform].push_back(cur_instance);
		stretch_extents(cur_instance.mModel, cur_instance.mTransform, mExtents[0], mExtents[1], mFirstTransform);
	}
	
	setLoadState( DONE );

	return true;
}

//static
bool LLModelLoader::isAlive(LLModelLoader* loader)
{
	if(!loader)
	{
		return false ;
	}

	std::list<LLModelLoader*>::iterator iter = sActiveLoaderList.begin() ;
	for(; iter != sActiveLoaderList.end() && (*iter) != loader; ++iter) ;
	
	return *iter == loader ;
}

void LLModelLoader::loadModelCallback()
{
	assert_main_thread();

	if (mPreview)
	{
		mPreview->loadModelCallback(mLod);	
	}

	while (!isStopped())
	{ //wait until this thread is stopped before deleting self
		apr_sleep(100);
	}

	//doubel check if "this" is valid before deleting it, in case it is aborted during running.
	if(!isAlive(this))
	{
		return ;
	}

	//cleanup model loader
	if (mPreview)
	{
		mPreview->mModelLoader = NULL;
	}

	delete this;
}
//-----------------------------------------------------------------------------
// buildJointToNodeMappingFromScene()
//-----------------------------------------------------------------------------
void LLModelLoader::buildJointToNodeMappingFromScene( daeElement* pRoot )
{
	daeElement* pScene = pRoot->getDescendant("visual_scene");
	if ( pScene )
	{
		daeTArray< daeSmartRef<daeElement> > children = pScene->getChildren();
		S32 childCount = children.getCount();
		for (S32 i = 0; i < childCount; ++i)
		{
			domNode* pNode = daeSafeCast<domNode>(children[i]);
			processJointToNodeMapping( pNode );			
		}
	}
}
//-----------------------------------------------------------------------------
// processJointToNodeMapping()
//-----------------------------------------------------------------------------
void LLModelLoader::processJointToNodeMapping( domNode* pNode )
{
	if ( isNodeAJoint( pNode ) )
	{
		//1.Store the parent
		std::string nodeName = pNode->getName();
		if ( !nodeName.empty() )
		{
			mJointsFromNode.push_front( pNode->getName() );
		}
		//2. Handle the kiddo's
		processChildJoints( pNode );
	}
	else
	{
		//Determine if the're any children wrt to this failed node.
		//This occurs when an armature is exported and ends up being what essentially amounts to
		//as the root for the visual_scene
		if ( pNode ) 
		{
			processChildJoints( pNode );
		}
		else 
		{
			llinfos<<"Node is NULL"<<llendl;
		}

	}
}
//-----------------------------------------------------------------------------
// processChildJoint()
//-----------------------------------------------------------------------------
void LLModelLoader::processChildJoints( domNode* pParentNode )
{	
	daeTArray< daeSmartRef<daeElement> > childOfChild = pParentNode->getChildren();
	S32 childOfChildCount = childOfChild.getCount();
	for (S32 i = 0; i < childOfChildCount; ++i)
	{
		domNode* pChildNode = daeSafeCast<domNode>( childOfChild[i] );
		if ( pChildNode )
		{
			processJointToNodeMapping( pChildNode );
		}
	}
}

//-----------------------------------------------------------------------------
// critiqueRigForUploadApplicability()
//-----------------------------------------------------------------------------
void LLModelPreview::critiqueRigForUploadApplicability( const std::vector<std::string> &jointListFromAsset )
{
	critiqueJointToNodeMappingFromScene();
	
	//Determines the following use cases for a rig:
	//1. It is suitable for upload with skin weights & joint positions, or
	//2. It is suitable for upload as standard av with just skin weights
	
	bool isJointPositionUploadOK = isRigSuitableForJointPositionUpload( jointListFromAsset );
	bool isRigLegacyOK			 = isRigLegacy( jointListFromAsset );

	//It's OK that both could end up being true, both default to false
	if ( isJointPositionUploadOK )
	{
		setRigValidForJointPositionUpload( true );
	}

	if ( isRigLegacyOK) 
	{	
		setLegacyRigValid( true );
	}

}
//-----------------------------------------------------------------------------
// critiqueJointToNodeMappingFromScene()
//-----------------------------------------------------------------------------
void LLModelPreview::critiqueJointToNodeMappingFromScene( void  )
{
	//Do the actual nodes back the joint listing from the dae?
	//if yes then this is a fully rigged asset, otherwise it's just a partial rig
	
	std::deque<std::string>::iterator jointsFromNodeIt = mJointsFromNode.begin();
	std::deque<std::string>::iterator jointsFromNodeEndIt = mJointsFromNode.end();
	bool result = true;

	if ( !mJointsFromNode.empty() )
	{
		for ( ;jointsFromNodeIt!=jointsFromNodeEndIt;++jointsFromNodeIt )
		{
			std::string name = *jointsFromNodeIt;
			if ( mJointTransformMap.find( name ) != mJointTransformMap.end() )
			{
				continue;
			}
			else
			{
				llinfos<<"critiqueJointToNodeMappingFromScene is missing a: "<<name<<llendl;
				result = false;				
			}
		}
	}
	else
	{
		result = false;
	}

	//Determines the following use cases for a rig:
	//1. Full av rig  w/1-1 mapping from the scene and joint array
	//2. Partial rig but w/o parity between the scene and joint array
	if ( result )
	{		
		setRigWithSceneParity( true );
	}	
}
//-----------------------------------------------------------------------------
// isRigLegacy()
//-----------------------------------------------------------------------------
bool LLModelPreview::isRigLegacy( const std::vector<std::string> &jointListFromAsset )
{
	//No joints in asset
	if ( jointListFromAsset.size() == 0 )
	{
		return false;
	}

	bool result = false;

	std::deque<std::string> :: const_iterator masterJointIt = mMasterLegacyJointList.begin();	
	std::deque<std::string> :: const_iterator masterJointEndIt = mMasterLegacyJointList.end();
	
	std::vector<std::string> :: const_iterator modelJointIt = jointListFromAsset.begin();	
	std::vector<std::string> :: const_iterator modelJointItEnd = jointListFromAsset.end();
	
	for ( ;masterJointIt!=masterJointEndIt;++masterJointIt )
	{
		result = false;
		modelJointIt = jointListFromAsset.begin();

		for ( ;modelJointIt!=modelJointItEnd; ++modelJointIt )
		{
			if ( *masterJointIt == *modelJointIt )
			{
				result = true;
				break;
			}			
		}		
		if ( !result )
		{
			llinfos<<" Asset did not contain the joint (if you're u/l a fully rigged asset w/joint positions - it is required)." << *masterJointIt<< llendl;
			break;
		}
	}	
	return result;
}
//-----------------------------------------------------------------------------
// isRigSuitableForJointPositionUpload()
//-----------------------------------------------------------------------------
bool LLModelPreview::isRigSuitableForJointPositionUpload( const std::vector<std::string> &jointListFromAsset )
{
	bool result = false;

	std::deque<std::string> :: const_iterator masterJointIt = mMasterJointList.begin();	
	std::deque<std::string> :: const_iterator masterJointEndIt = mMasterJointList.end();
	
	std::vector<std::string> :: const_iterator modelJointIt = jointListFromAsset.begin();	
	std::vector<std::string> :: const_iterator modelJointItEnd = jointListFromAsset.end();
	
	for ( ;masterJointIt!=masterJointEndIt;++masterJointIt )
	{
		result = false;
		modelJointIt = jointListFromAsset.begin();

		for ( ;modelJointIt!=modelJointItEnd; ++modelJointIt )
		{
			if ( *masterJointIt == *modelJointIt )
			{
				result = true;
				break;
			}			
		}		
		if ( !result )
		{
			llinfos<<" Asset did not contain the joint (if you're u/l a fully rigged asset w/joint positions - it is required)." << *masterJointIt<< llendl;
			break;
		}
	}	
	return result;
}


//called in the main thread
void LLModelLoader::loadTextures()
{
	BOOL is_paused = isPaused() ;
	pause() ; //pause the loader 

	for(scene::iterator iter = mScene.begin(); iter != mScene.end(); ++iter)
	{
		for(U32 i = 0 ; i < iter->second.size(); i++)
		{
			for(std::map<std::string, LLImportMaterial>::iterator j = iter->second[i].mMaterial.begin();
				j != iter->second[i].mMaterial.end(); ++j)
			{
				LLImportMaterial& material = j->second;

				if(!material.mDiffuseMapFilename.empty())
				{
					material.mDiffuseMap = 
						LLViewerTextureManager::getFetchedTextureFromUrl("file://" + material.mDiffuseMapFilename, TRUE, LLViewerTexture::BOOST_PREVIEW);
					material.mDiffuseMap->setLoadedCallback(LLModelPreview::textureLoadedCallback, 0, TRUE, FALSE, mPreview, NULL, FALSE);
					material.mDiffuseMap->forceToSaveRawImage(0, F32_MAX);
					mNumOfFetchingTextures++ ;
				}
			}
		}
	}

	if(!is_paused)
	{
		unpause() ;
	}
}

//-----------------------------------------------------------------------------
// isNodeAJoint()
//-----------------------------------------------------------------------------
bool LLModelLoader::isNodeAJoint( domNode* pNode )
{
	if ( !pNode )
	{
		llinfos<<"Created node is NULL"<<llendl;
		return false;
	}
	
	if ( pNode->getName() == NULL )
	{
		llinfos<<"Parsed node has no name "<<llendl;
		//Attempt to write the node id, if possible (aids in debugging the visual scene)
		if ( pNode->getId() )
		{
			llinfos<<"Parsed node ID: "<<pNode->getId()<<llendl;
		}
		return false;
	}

	if ( mJointMap.find( pNode->getName() )  != mJointMap.end() )
	{
		return true;
	}

	return false;
}
//-----------------------------------------------------------------------------
// verifyCount
//-----------------------------------------------------------------------------
bool LLModelPreview::verifyCount( int expected, int result )
{
	if ( expected != result )
	{
		llinfos<< "Error: (expected/got)"<<expected<<"/"<<result<<"verts"<<llendl;
		return false;
	}
	return true;
}
//-----------------------------------------------------------------------------
// verifyController
//-----------------------------------------------------------------------------
bool LLModelPreview::verifyController( domController* pController )
{	

	bool result = true;

	domSkin* pSkin = pController->getSkin();

	if ( pSkin )
	{
		xsAnyURI & uri = pSkin->getSource();
		domElement* pElement = uri.getElement();

		if ( !pElement )
		{
			llinfos<<"Can't resolve skin source"<<llendl;
			return false;
		}

		daeString type_str = pElement->getTypeName();
		if ( stricmp(type_str, "geometry") == 0 )
		{	
			//Skin is reference directly by geometry and get the vertex count from skin
			domSkin::domVertex_weights* pVertexWeights = pSkin->getVertex_weights();
			U32 vertexWeightsCount = pVertexWeights->getCount();
			domGeometry* pGeometry = (domGeometry*) (domElement*) uri.getElement();
			domMesh* pMesh = pGeometry->getMesh();				
			
			if ( pMesh )
			{
				//Get vertex count from geometry
				domVertices* pVertices = pMesh->getVertices();
				if ( !pVertices )
				{ 
					llinfos<<"No vertices!"<<llendl;
					return false;
				}

				if ( pVertices )
				{
					xsAnyURI src = pVertices->getInput_array()[0]->getSource();
					domSource* pSource = (domSource*) (domElement*) src.getElement();
					U32 verticesCount = pSource->getTechnique_common()->getAccessor()->getCount();
					result = verifyCount( verticesCount, vertexWeightsCount );
					if ( !result )
					{
						return result;
					}
				}
			}	

			U32 vcountCount = (U32) pVertexWeights->getVcount()->getValue().getCount();
			result = verifyCount( vcountCount, vertexWeightsCount );	
			if ( !result )
			{
				return result;
			}

			domInputLocalOffset_Array& inputs = pVertexWeights->getInput_array();
			U32 sum = 0;
			for (size_t i=0; i<vcountCount; i++)
			{
				sum += pVertexWeights->getVcount()->getValue()[i];
			}
			result = verifyCount( sum * inputs.getCount(), (domInt) pVertexWeights->getV()->getValue().getCount() );
		}
	}
	
	return result;
}

//-----------------------------------------------------------------------------
// extractTranslation()
//-----------------------------------------------------------------------------
void LLModelLoader::extractTranslation( domTranslate* pTranslate, LLMatrix4& transform )
{
	domFloat3 jointTrans = pTranslate->getValue();
	LLVector3 singleJointTranslation( jointTrans[0], jointTrans[1], jointTrans[2] );
	transform.setTranslation( singleJointTranslation );
}
//-----------------------------------------------------------------------------
// extractTranslationViaElement()
//-----------------------------------------------------------------------------
void LLModelLoader::extractTranslationViaElement( daeElement* pTranslateElement, LLMatrix4& transform )
{
	if ( pTranslateElement )
	{
		domTranslate* pTranslateChild = dynamic_cast<domTranslate*>( pTranslateElement );
		domFloat3 translateChild = pTranslateChild->getValue();
		LLVector3 singleJointTranslation( translateChild[0], translateChild[1], translateChild[2] );
		transform.setTranslation( singleJointTranslation );
	}	
}
//-----------------------------------------------------------------------------
// extractTranslationViaSID()
//-----------------------------------------------------------------------------
void LLModelLoader::extractTranslationViaSID( daeElement* pElement, LLMatrix4& transform )
{
	if ( pElement )
	{	
		daeSIDResolver resolver( pElement, "./transform" );
		domMatrix* pMatrix = daeSafeCast<domMatrix>( resolver.getElement() );
		//We are only extracting out the translational component atm
		LLMatrix4 workingTransform;
		if ( pMatrix )
		{
			domFloat4x4 domArray = pMatrix->getValue();									
			for ( int i = 0; i < 4; i++ )
			{
				for( int j = 0; j < 4; j++ )
				{
					workingTransform.mMatrix[i][j] = domArray[i + j*4];
				}
			}
			LLVector3 trans = workingTransform.getTranslation();
			transform.setTranslation( trans );	
		}
	}
	else
	{
		llwarns<<"Element is nonexistent - empty/unsupported node."<<llendl;
	}
}
//-----------------------------------------------------------------------------
// processJointNode()
//-----------------------------------------------------------------------------
void LLModelLoader::processJointNode( domNode* pNode, JointTransformMap& jointTransforms )
{
	if (pNode->getName() == NULL)
	{
		llwarns << "nameless node, can't process" << llendl;
		return;
	}

	//llwarns<<"ProcessJointNode# Node:" <<pNode->getName()<<llendl;

	//1. handle the incoming node - extract out translation via SID or element

	LLMatrix4 workingTransform;

	//Pull out the translate id and store it in the jointTranslations map
	daeSIDResolver jointResolverA( pNode, "./translate" );
	domTranslate* pTranslateA = daeSafeCast<domTranslate>( jointResolverA.getElement() );
	daeSIDResolver jointResolverB( pNode, "./location" );
	domTranslate* pTranslateB = daeSafeCast<domTranslate>( jointResolverB.getElement() );

	//Translation via SID was successful
	if ( pTranslateA )
	{
		extractTranslation( pTranslateA, workingTransform );
	}
	else
	if ( pTranslateB )
	{
		extractTranslation( pTranslateB, workingTransform );
	}
	else
	{
		//Translation via child from element
		daeElement* pTranslateElement = getChildFromElement( pNode, "translate" );
		if ( !pTranslateElement || pTranslateElement->typeID() != domTranslate::ID() )
		{
			//llwarns<< "The found element is not a translate node" <<llendl;
			daeSIDResolver jointResolver( pNode, "./matrix" );
			domMatrix* pMatrix = daeSafeCast<domMatrix>( jointResolver.getElement() );
			if ( pMatrix )
			{
				//llinfos<<"A matrix SID was however found!"<<llendl;
				domFloat4x4 domArray = pMatrix->getValue();									
				for ( int i = 0; i < 4; i++ )
				{
					for( int j = 0; j < 4; j++ )
					{
						workingTransform.mMatrix[i][j] = domArray[i + j*4];
					}
				}
			}
			else
			{
				llwarns<< "The found element is not translate or matrix node - most likely a corrupt export!" <<llendl;
			}
		}
		else
		{
			extractTranslationViaElement( pTranslateElement, workingTransform );
		}
	}

	//Store the working transform relative to the nodes name.
	jointTransforms[ pNode->getName() ] = workingTransform;

	//2. handle the nodes children

	//Gather and handle the incoming nodes children
	daeTArray< daeSmartRef<daeElement> > childOfChild = pNode->getChildren();
	S32 childOfChildCount = childOfChild.getCount();

	for (S32 i = 0; i < childOfChildCount; ++i)
	{
		domNode* pChildNode = daeSafeCast<domNode>( childOfChild[i] );
		if ( pChildNode )
		{
			processJointNode( pChildNode, jointTransforms );
		}
	}
}
//-----------------------------------------------------------------------------
// getChildFromElement()
//-----------------------------------------------------------------------------
daeElement* LLModelLoader::getChildFromElement( daeElement* pElement, std::string const & name )
{
    daeElement* pChildOfElement = pElement->getChild( name.c_str() );
	if ( pChildOfElement )
	{
		return pChildOfElement;
	}
	llwarns<< "Could not find a child [" << name << "] for the element: \"" << pElement->getAttribute("id") << "\"" << llendl;
    return NULL;
}

void LLModelLoader::processElement( daeElement* element, bool& badElement )
{
	LLMatrix4 saved_transform = mTransform;

	domTranslate* translate = daeSafeCast<domTranslate>(element);
	if (translate)
	{
		domFloat3 dom_value = translate->getValue();

		LLMatrix4 translation;
		translation.setTranslation(LLVector3(dom_value[0], dom_value[1], dom_value[2]));

		translation *= mTransform;
		mTransform = translation;
	}

	domRotate* rotate = daeSafeCast<domRotate>(element);
	if (rotate)
	{
		domFloat4 dom_value = rotate->getValue();

		LLMatrix4 rotation;
		rotation.initRotTrans(dom_value[3] * DEG_TO_RAD, LLVector3(dom_value[0], dom_value[1], dom_value[2]), LLVector3(0, 0, 0));

		rotation *= mTransform;
		mTransform = rotation;
	}

	domScale* scale = daeSafeCast<domScale>(element);
	if (scale)
	{
		domFloat3 dom_value = scale->getValue();


		LLVector3 scale_vector = LLVector3(dom_value[0], dom_value[1], dom_value[2]);
		scale_vector.abs(); // Set all values positive, since we don't currently support mirrored meshes
		LLMatrix4 scaling;
		scaling.initScale(scale_vector);

		scaling *= mTransform;
		mTransform = scaling;
	}

	domMatrix* matrix = daeSafeCast<domMatrix>(element);
	if (matrix)
	{
		domFloat4x4 dom_value = matrix->getValue();

		LLMatrix4 matrix_transform;

		for (int i = 0; i < 4; i++)
		{
			for(int j = 0; j < 4; j++)
			{
				matrix_transform.mMatrix[i][j] = dom_value[i + j*4];
			}
		}

		matrix_transform *= mTransform;
		mTransform = matrix_transform;
	}

	domInstance_geometry* instance_geo = daeSafeCast<domInstance_geometry>(element);
	if (instance_geo)
	{
		domGeometry* geo = daeSafeCast<domGeometry>(instance_geo->getUrl().getElement());
		if (geo)
		{
			domMesh* mesh = daeSafeCast<domMesh>(geo->getDescendant(daeElement::matchType(domMesh::ID())));
			if (mesh)
			{
				LLModel* model = mModel[mesh];
				if (model)
				{
					LLMatrix4 transformation = mTransform;

					if (mTransform.determinant() < 0)
					{ //negative scales are not supported
						llinfos << "Negative scale detected, unsupported transform.  domInstance_geometry: " << LLModel::getElementLabel(instance_geo) << llendl;
						badElement = true;
					}
					
					std::map<std::string, LLImportMaterial> materials = getMaterials(model, instance_geo);

					// adjust the transformation to compensate for mesh normalization
					LLVector3 mesh_scale_vector;
					LLVector3 mesh_translation_vector;
					model->getNormalizedScaleTranslation(mesh_scale_vector, mesh_translation_vector);

					LLMatrix4 mesh_translation;
					mesh_translation.setTranslation(mesh_translation_vector);
					mesh_translation *= transformation;
					transformation = mesh_translation;

					LLMatrix4 mesh_scale;
					mesh_scale.initScale(mesh_scale_vector);
					mesh_scale *= transformation;
					transformation = mesh_scale;

					std::string label = getElementLabel(instance_geo);
					mScene[transformation].push_back(LLModelInstance(model, label, transformation, materials));

					stretch_extents(model, transformation, mExtents[0], mExtents[1], mFirstTransform);
				}
			}
		}
		else 
		{
			llinfos<<"Unable to resolve geometry URL."<<llendl;
			badElement = true;			
		}

	}

	domInstance_node* instance_node = daeSafeCast<domInstance_node>(element);
	if (instance_node)
	{
		daeElement* instance = instance_node->getUrl().getElement();
		if (instance)
		{
			processElement(instance,badElement);
		}
	}

	//process children
	daeTArray< daeSmartRef<daeElement> > children = element->getChildren();
	int childCount = children.getCount();
	for (S32 i = 0; i < childCount; i++)
	{
		processElement(children[i],badElement);
	}

	domNode* node = daeSafeCast<domNode>(element);
	if (node)
	{ //this element was a node, restore transform before processiing siblings
		mTransform = saved_transform;
	}
}

std::map<std::string, LLImportMaterial> LLModelLoader::getMaterials(LLModel* model, domInstance_geometry* instance_geo)
{
	std::map<std::string, LLImportMaterial> materials;
	for (int i = 0; i < model->mMaterialList.size(); i++)
	{
		LLImportMaterial import_material;

		domInstance_material* instance_mat = NULL;

		domBind_material::domTechnique_common* technique =
		daeSafeCast<domBind_material::domTechnique_common>(instance_geo->getDescendant(daeElement::matchType(domBind_material::domTechnique_common::ID())));

		if (technique)
		{
			daeTArray< daeSmartRef<domInstance_material> > inst_materials = technique->getChildrenByType<domInstance_material>();
			for (int j = 0; j < inst_materials.getCount(); j++)
			{
				std::string symbol(inst_materials[j]->getSymbol());

				if (symbol == model->mMaterialList[i]) // found the binding
				{
					instance_mat = inst_materials[j];
				}
			}
		}

		if (instance_mat)
		{
			domMaterial* material = daeSafeCast<domMaterial>(instance_mat->getTarget().getElement());
			if (material)
			{
				domInstance_effect* instance_effect =
				daeSafeCast<domInstance_effect>(material->getDescendant(daeElement::matchType(domInstance_effect::ID())));
				if (instance_effect)
				{
					domEffect* effect = daeSafeCast<domEffect>(instance_effect->getUrl().getElement());
					if (effect)
					{
						domProfile_COMMON* profile =
						daeSafeCast<domProfile_COMMON>(effect->getDescendant(daeElement::matchType(domProfile_COMMON::ID())));
						if (profile)
						{
							import_material = profileToMaterial(profile);
						}
					}
				}
			}
		}

		import_material.mBinding = model->mMaterialList[i];
		materials[model->mMaterialList[i]] = import_material;
	}

	return materials;
}

LLImportMaterial LLModelLoader::profileToMaterial(domProfile_COMMON* material)
{
	LLImportMaterial mat;
	mat.mFullbright = FALSE;

	daeElement* diffuse = material->getDescendant("diffuse");
	if (diffuse)
	{
		domCommon_color_or_texture_type_complexType::domTexture* texture =
		daeSafeCast<domCommon_color_or_texture_type_complexType::domTexture>(diffuse->getDescendant("texture"));
		if (texture)
		{
			domCommon_newparam_type_Array newparams = material->getNewparam_array();
			for (S32 i = 0; i < newparams.getCount(); i++)
			{
				domFx_surface_common* surface = newparams[i]->getSurface();
				if (surface)
				{
					domFx_surface_init_common* init = surface->getFx_surface_init_common();
					if (init)
					{
						domFx_surface_init_from_common_Array init_from = init->getInit_from_array();

						if (init_from.getCount() > i)
						{
							domImage* image = daeSafeCast<domImage>(init_from[i]->getValue().getElement());
							if (image)
							{
								// we only support init_from now - embedded data will come later
								domImage::domInit_from* init = image->getInit_from();
								if (init)
								{									
									mat.mDiffuseMapFilename = cdom::uriToNativePath(init->getValue().str());
									mat.mDiffuseMapLabel = getElementLabel(material);
								}
							}
						}
					}
				}
			}
		}

		domCommon_color_or_texture_type_complexType::domColor* color =
		daeSafeCast<domCommon_color_or_texture_type_complexType::domColor>(diffuse->getDescendant("color"));
		if (color)
		{
			domFx_color_common domfx_color = color->getValue();
			LLColor4 value = LLColor4(domfx_color[0], domfx_color[1], domfx_color[2], domfx_color[3]);
			mat.mDiffuseColor = value;
		}
	}

	daeElement* emission = material->getDescendant("emission");
	if (emission)
	{
		LLColor4 emission_color = getDaeColor(emission);
		if (((emission_color[0] + emission_color[1] + emission_color[2]) / 3.0) > 0.25)
		{
			mat.mFullbright = TRUE;
		}
	}

	return mat;
}

// try to get a decent label for this element
std::string LLModelLoader::getElementLabel(daeElement *element)
{
	// if we have a name attribute, use it
	std::string name = element->getAttribute("name");
	if (name.length())
	{
		return name;
	}

	// if we have an ID attribute, use it
	if (element->getID())
	{
		return std::string(element->getID());
	}

	// if we have a parent, use it
	daeElement* parent = element->getParent();
	if (parent)
	{
		// if parent has a name, use it
		std::string name = parent->getAttribute("name");
		if (name.length())
		{
			return name;
		}

		// if parent has an ID, use it
		if (parent->getID())
		{
			return std::string(parent->getID());
		}
	}

	// try to use our type
	daeString element_name = element->getElementName();
	if (element_name)
	{
		return std::string(element_name);
	}

	// if all else fails, use "object"
	return std::string("object");
}

LLColor4 LLModelLoader::getDaeColor(daeElement* element)
{
	LLColor4 value;
	domCommon_color_or_texture_type_complexType::domColor* color =
	daeSafeCast<domCommon_color_or_texture_type_complexType::domColor>(element->getDescendant("color"));
	if (color)
	{
		domFx_color_common domfx_color = color->getValue();
		value = LLColor4(domfx_color[0], domfx_color[1], domfx_color[2], domfx_color[3]);
	}

	return value;
}

//-----------------------------------------------------------------------------
// LLModelPreview
//-----------------------------------------------------------------------------

LLModelPreview::LLModelPreview(S32 width, S32 height, LLFloater* fmp)
: LLViewerDynamicTexture(width, height, 3, ORDER_MIDDLE, FALSE), LLMutex(NULL)
, mPelvisZOffset( 0.0f )
, mLegacyRigValid( false )
, mRigValidJointUpload( false )
, mResetJoints( false )
, mRigParityWithScene( false )
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

	//move into joint mapper class
	//1. joints for joint offset verification
	mMasterJointList.push_front("mPelvis");
	mMasterJointList.push_front("mTorso");
	mMasterJointList.push_front("mChest");
	mMasterJointList.push_front("mNeck");
	mMasterJointList.push_front("mHead");
	mMasterJointList.push_front("mCollarLeft");
	mMasterJointList.push_front("mShoulderLeft");
	mMasterJointList.push_front("mElbowLeft");
	mMasterJointList.push_front("mWristLeft");
	mMasterJointList.push_front("mCollarRight");
	mMasterJointList.push_front("mShoulderRight");
	mMasterJointList.push_front("mElbowRight");
	mMasterJointList.push_front("mWristRight");
	mMasterJointList.push_front("mHipRight");
	mMasterJointList.push_front("mKneeRight");
	mMasterJointList.push_front("mFootRight");
	mMasterJointList.push_front("mHipLeft");
	mMasterJointList.push_front("mKneeLeft");
	mMasterJointList.push_front("mFootLeft");
	//2. legacy joint list - used to verify rigs that will not be using joint offsets
	mMasterLegacyJointList.push_front("mPelvis");
	mMasterLegacyJointList.push_front("mTorso");
	mMasterLegacyJointList.push_front("mChest");
	mMasterLegacyJointList.push_front("mNeck");
	mMasterLegacyJointList.push_front("mHead");
	mMasterLegacyJointList.push_front("mHipRight");
	mMasterLegacyJointList.push_front("mKneeRight");
	mMasterLegacyJointList.push_front("mFootRight");
	mMasterLegacyJointList.push_front("mHipLeft");
	mMasterLegacyJointList.push_front("mKneeLeft");
	mMasterLegacyJointList.push_front("mFootLeft");

	createPreviewAvatar();
}

LLModelPreview::~LLModelPreview()
{
	if (mModelLoader)
	{
		mModelLoader->mPreview = NULL;
		mModelLoader = NULL;
	}
	//*HACK : *TODO : turn this back on when we understand why this crashes
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
		getPreviewAvatar()->setPelvisOffset( mPelvisZOffset );
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
					   TRUE);
			
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

	//reorder materials to match mBaseModel
	for (U32 i = 0; i < LLModel::NUM_LODS-1; i++)
	{
		if (mBaseModel.size() == mModel[i].size())
		{
			for (U32 j = 0; j < mBaseModel.size(); ++j)
			{
				
				int refFaceCnt = 0;
				int modelFaceCnt = 0;
				
				if ( !mModel[i][j]->matchMaterialOrder(mBaseModel[j], refFaceCnt, modelFaceCnt ) )
				{
					setLoadState( LLModelLoader::ERROR_MATERIALS );
					mFMP->childDisable( "calculate_btn" );
				}
			}
		}
	}

	for (LLModelLoader::scene::iterator iter = mBaseScene.begin(); iter != mBaseScene.end(); ++iter)
	{ //for each transform in scene
		LLMatrix4 mat = iter->first;

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

		for (LLModelLoader::model_instance_list::iterator model_iter = iter->second.begin(); model_iter != iter->second.end(); ++model_iter)
		{ //for each instance with said transform applied
			LLModelInstance instance = *model_iter;

			LLModel* base_model = instance.mModel;
			
			if (base_model)
			{
				base_model->mRequestedLabel = requested_name;
				base_model->mMetric = metric;
			}

			S32 idx = 0;
			for (idx = 0; idx < mBaseModel.size(); ++idx)
			{  //find reference instance for this model
				if (mBaseModel[idx] == base_model)
				{
					break;
				}
			}

			if(idx < mBaseModel.size())
			{
				for (U32 i = 0; i < LLModel::NUM_LODS; i++)
				{ //fill LOD slots based on reference model index
					if (mModel[i].size() > idx)
					{
						instance.mLOD[i] = mModel[i][idx];
					}
					else
					{
						instance.mLOD[i] = NULL;
					}
				}
			}
			instance.mTransform = mat;
			mUploadData.push_back(instance);
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

void LLModelPreview::saveUploadData(bool save_skinweights, bool save_joint_positions)
{
	if (!mLODFile[LLModel::LOD_HIGH].empty())
	{
		std::string filename = mLODFile[LLModel::LOD_HIGH];
		
		std::string::size_type i = filename.rfind(".");
		if (i != std::string::npos)
		{
			filename.replace(i, filename.size()-1, ".slm");
			saveUploadData(filename, save_skinweights, save_joint_positions);
		}
	}
}

void LLModelPreview::saveUploadData(const std::string& filename, bool save_skinweights, bool save_joint_positions)
{
	if (!gSavedSettings.getBOOL("MeshImportUseSLM"))
	{
		return;
	}

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
				save_skinweights, save_joint_positions, FALSE, TRUE);

			
			data["mesh"][instance.mModel->mLocalID] = str.str();
		}

		data["instance"][i] = instance.asLLSD();
	}

	llofstream out(filename, std::ios_base::out | std::ios_base::binary);
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

void LLModelPreview::loadModel(std::string filename, S32 lod, bool force_disable_slm)
{
	assert_main_thread();

	LLMutexLock lock(this);

	if (lod < LLModel::LOD_IMPOSTOR || lod > LLModel::NUM_LODS - 1)
	{
		llwarns << "Invalid level of detail: " << lod << llendl;
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
			mLoading = false;
		}
		return;
	}

	if (mModelLoader)
	{
		llwarns << "Incompleted model load operation pending." << llendl;
		return;
	}
	
	mLODFile[lod] = filename;

	if (lod == LLModel::LOD_HIGH)
	{
		clearGLODGroup();
	}

	mModelLoader = new LLModelLoader(filename, lod, this, mJointTransformMap, mJointsFromNode );

	if (force_disable_slm)
	{
		mModelLoader->mTrySLM = false;
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
		mFMP->childSetText("lod_file_" + lod_name[lod], mLODFile[lod]);
	}
	else if (lod == LLModel::LOD_PHYSICS)
	{
		mFMP->childSetText("physics_file", mLODFile[lod]);
	}

	mFMP->openFloater();
}

void LLModelPreview::setPhysicsFromLOD(S32 lod)
{
	assert_main_thread();

	if (lod >= 0 && lod <= 3)
	{
		mModel[LLModel::LOD_PHYSICS] = mModel[lod];
		mScene[LLModel::LOD_PHYSICS] = mScene[lod];
		mLODFile[LLModel::LOD_PHYSICS].clear();
		mFMP->childSetText("physics_file", mLODFile[LLModel::LOD_PHYSICS]);
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

	for (U32 i = 0; i <= LLModel::LOD_HIGH; i++)
	{ //clear out any entries that aren't compatible with this model
		if (i != lod)
		{
			if (mModel[i].size() != mModel[lod].size())
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

void LLModelPreview::loadModelCallback(S32 lod)
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
		return ;
	}

	mModelLoader->loadTextures() ;

	if (lod == -1)
	{ //populate all LoDs from model loader scene
		mBaseModel.clear();
		mBaseScene.clear();

		bool skin_weights = false;
		bool joint_positions = false;

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
		}

		//copy high lod to base scene for LoD generation
		mBaseScene = mScene[LLModel::LOD_HIGH];
		mBaseModel = mModel[LLModel::LOD_HIGH];

		mDirty = true;
		resetPreviewTarget();
	}
	else
	{ //only replace given LoD
		mModel[lod] = mModelLoader->mModelList;
		mScene[lod] = mModelLoader->mScene;
		mVertexBuffer[lod].clear();

		setPreviewLOD(lod);

		if (lod == LLModel::LOD_HIGH)
		{ //save a copy of the highest LOD for automatic LOD manipulation
			if (mBaseModel.empty())
			{ //first time we've loaded a model, auto-gen LoD
				mGenLOD = true;
			}

			mBaseModel = mModel[lod];
			clearGLODGroup();

			mBaseScene = mScene[lod];
			mVertexBuffer[5].clear();
		}

		clearIncompatible(lod);

		mDirty = true;

		if (lod == LLModel::LOD_HIGH)
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
			if (mFMP->getChild<LLUICtrl>("description_form")->getValue().asString().empty())
			{
				const std::string& model_name = mBaseModel[0]->getName();
				mFMP->getChild<LLUICtrl>("description_form")->setValue(model_name);
			}
		}
	}
	refresh();

	mModelLoadedSignal();
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
		for (LLModelLoader::model_list::iterator iter = mBaseModel.begin(); iter != mBaseModel.end(); ++iter)
		{
			(*iter)->generateNormals(angle_cutoff);
		}

		mVertexBuffer[5].clear();
	}

	for (LLModelLoader::model_list::iterator iter = mModel[which_lod].begin(); iter != mModel[which_lod].end(); ++iter)
	{
		(*iter)->generateNormals(angle_cutoff);
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
		llwarns << "Invalid level of detail: " << which_lod << llendl;
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

	for (LLModelLoader::model_list::iterator iter = mBaseModel.begin(); iter != mBaseModel.end(); ++iter)
	{
		LLModel* mdl = *iter;
		for (S32 i = 0; i < mdl->getNumVolumeFaces(); ++i)
		{
			triangle_count += mdl->getVolumeFace(i).mNumIndices/3;
		}
	}

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

		mRequestedTriangleCount[lod] = triangle_count;
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
					llerrs << "Invalid face generated during LOD generation." << llendl;
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
				llerrs << "Invalid model generated when creating LODs" << llendl;
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

	/*if (which_lod == -1 && mScene[LLModel::LOD_PHYSICS].empty())
	 { //build physics scene
	 mScene[LLModel::LOD_PHYSICS] = mScene[LLModel::LOD_LOW];
	 mModel[LLModel::LOD_PHYSICS] = mModel[LLModel::LOD_LOW];

	 for (U32 i = 1; i < mModel[LLModel::LOD_PHYSICS].size(); ++i)
	 {
	 mPhysicsQ.push(mModel[LLModel::LOD_PHYSICS][i]);
	 }
	 }*/
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

	for (S32 lod = 0; lod < LLModel::NUM_LODS; ++lod)
	{
		//initialize total for this lod to 0
		total_tris[lod] = total_verts[lod] = total_submeshes[lod] = 0;

		for (U32 i = 0; i < mModel[lod].size(); ++i)
		{ //for each model in the lod
			S32 cur_tris = 0;
			S32 cur_verts = 0;
			S32 cur_submeshes = mModel[lod][i]->getNumVolumeFaces();

			for (S32 j = 0; j < cur_submeshes; ++j)
			{ //for each submesh (face), add triangles and vertices to current total
				const LLVolumeFace& face = mModel[lod][i]->getVolumeFace(j);
				cur_tris += face.mNumIndices/3;
				cur_verts += face.mNumVertices;
			}

			//add this model to the lod total
			total_tris[lod] += cur_tris;
			total_verts[lod] += cur_verts;
			total_submeshes[lod] += cur_submeshes;

			//store this model's counts to asset data
			tris[lod].push_back(cur_tris);
			verts[lod].push_back(cur_verts);
			submeshes[lod].push_back(cur_submeshes);
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
			if (mModel[lod][i]->mPhysics.mHull.empty())
			{ //no decomp exists
				S32 cur_submeshes = mModel[lod][i]->getNumVolumeFaces();
				for (S32 j = 0; j < cur_submeshes && !has_degenerate; ++j)
				{ //for each submesh (face), add triangles and vertices to current total
					const LLVolumeFace& face = mModel[lod][i]->getVolumeFace(j);
					for (S32 k = 0; k < face.mNumIndices && !has_degenerate; )
					{
						LLVector4a v1; v1.setMul(face.mPositions[face.mIndices[k++]], scale);
						LLVector4a v2; v2.setMul(face.mPositions[face.mIndices[k++]], scale);
						LLVector4a v3; v3.setMul(face.mPositions[face.mIndices[k++]], scale);

						if (ll_is_degenerate(v1,v2,v3))
						{
							has_degenerate = true;
						}
					}
				}
			}
		}
	}
	
	mFMP->childSetTextArg("submeshes_info", "[SUBMESHES]", llformat("%d", total_submeshes[LLModel::LOD_HIGH]));

	std::string mesh_status_na = mFMP->getString("mesh_status_na");

	S32 upload_status[LLModel::LOD_HIGH+1];

	bool upload_ok = true;

	for (S32 lod = 0; lod <= LLModel::LOD_HIGH; ++lod)
	{
		upload_status[lod] = 0;

		std::string message = "mesh_status_good";

		if (total_tris[lod] > 0)
		{
			mFMP->childSetText(lod_triangles_name[lod], llformat("%d", total_tris[lod]));
			mFMP->childSetText(lod_vertices_name[lod], llformat("%d", total_verts[lod]));
		}
		else
		{
			if (lod == LLModel::LOD_HIGH)
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

			mFMP->childSetText(lod_triangles_name[lod], mesh_status_na);
			mFMP->childSetText(lod_vertices_name[lod], mesh_status_na);
		}

		const U32 lod_high = LLModel::LOD_HIGH;

		if (lod != lod_high)
		{
			if (total_submeshes[lod] && total_submeshes[lod] != total_submeshes[lod_high])
			{ //number of submeshes is different
				message = "mesh_status_submesh_mismatch";
				upload_status[lod] = 2;
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
					upload_status[lod] = 2;
				}
			}
		}

		LLIconCtrl* icon = mFMP->getChild<LLIconCtrl>(lod_icon_name[lod]);
		LLUIImagePtr img = LLUI::getUIImage(lod_status_image[upload_status[lod]]);
		icon->setVisible(true);
		icon->setImage(img);

		if (upload_status[lod] >= 2)
		{
			upload_ok = false;
		}

		if (lod == mPreviewLOD)
		{
			mFMP->childSetText("lod_status_message_text", mFMP->getString(message));
			icon = mFMP->getChild<LLIconCtrl>("lod_status_message_icon");
			icon->setImage(img);
		}

		updateLodControls(lod);
	}


	//make sure no hulls have more than 256 points in them
	for (U32 i = 0; upload_ok && i < mModel[LLModel::LOD_PHYSICS].size(); ++i)
	{
		LLModel* mdl = mModel[LLModel::LOD_PHYSICS][i];

		for (U32 j = 0; upload_ok && j < mdl->mPhysics.mHull.size(); ++j)
		{
			if (mdl->mPhysics.mHull[j].size() > 256)
			{
				upload_ok = false;
			}
		}
	}

	bool errorStateFromLoader = getLoadState() >= LLModelLoader::ERROR_PARSING ? true : false;

	bool skinAndRigOk = true;
	bool uploadingSkin		     = mFMP->childGetValue("upload_skin").asBoolean();
	bool uploadingJointPositions = mFMP->childGetValue("upload_joints").asBoolean();

	if ( uploadingSkin )
	{
		if ( uploadingJointPositions && !isRigValidForJointPositionUpload() )
		{
			skinAndRigOk = false;
		}	
	}
	
	if(upload_ok && mModelLoader)
	{
		if(!mModelLoader->areTexturesReady() && mFMP->childGetValue("upload_textures").asBoolean())
		{
			upload_ok = false ;
		}
	}

	if (!upload_ok || errorStateFromLoader || !skinAndRigOk || has_degenerate)
	{
		mFMP->childDisable("ok_btn");
	}
	
	//add up physics triangles etc
	S32 start = 0;
	S32 end = mModel[LLModel::LOD_PHYSICS].size();

	S32 phys_tris = 0;
	S32 phys_hulls = 0;
	S32 phys_points = 0;

	for (S32 i = start; i < end; ++i)
	{ //add up hulls and points and triangles for selected mesh(es)
		LLModel* model = mModel[LLModel::LOD_PHYSICS][i];
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
		llwarns << "Invalid level of detail: " << lod << llendl;
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
	if (lod_mode == 0) // LoD from file
	{
		fmp->mLODMode[lod] = 0;
		for (U32 i = 0; i < num_file_controls; ++i)
		{
			mFMP->childShow(file_controls[i] + lod_name[lod]);
		}

		for (U32 i = 0; i < num_lod_controls; ++i)
		{
			mFMP->childHide(lod_controls[i] + lod_name[lod]);
		}
	}
	else if (lod_mode == 2) // use LoD above
	{
		fmp->mLODMode[lod] = 2;
		for (U32 i = 0; i < num_file_controls; ++i)
		{
			mFMP->childHide(file_controls[i] + lod_name[lod]);
		}

		for (U32 i = 0; i < num_lod_controls; ++i)
		{
			mFMP->childHide(lod_controls[i] + lod_name[lod]);
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
			mFMP->childHide(file_controls[i] + lod_name[lod]);
		}

		for (U32 i = 0; i < num_lod_controls; ++i)
		{
			mFMP->childShow(lod_controls[i] + lod_name[lod]);
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

		for (S32 i = 0; i < mdl->getNumVolumeFaces(); ++i)
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
				LLVector4a::memcpyNonAliased16((F32*) tc_strider.get(), (F32*) vf.mTexCoords, num_vertices*2*sizeof(F32));
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

					LLVector4 w(0,0,0,0);
					
					for (U32 i = 0; i < weight_list.size(); ++i)
					{
						F32 wght = llmin(weight_list[i].mWeight, 0.999999f);
						F32 joint = (F32) weight_list[i].mJointIdx;
						w.mV[i] = joint + wght;
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
	if (mDirty)
	{
		mDirty = false;
		mResourceCost = calcResourceCost();
		refresh();
		updateStatusMessages();
	}

	if (mGenLOD)
	{
		mGenLOD = false;
		genLODs();
		refresh();
		updateStatusMessages();
	}

}
//-----------------------------------------------------------------------------
// getTranslationForJointOffset()
//-----------------------------------------------------------------------------
LLVector3 LLModelPreview::getTranslationForJointOffset( std::string joint )
{
	LLMatrix4 jointTransform;
	if ( mJointTransformMap.find( joint ) != mJointTransformMap.end() )
	{
		jointTransform = mJointTransformMap[joint];
		return jointTransform.getTranslation();
	}
	return LLVector3(0.0f,0.0f,0.0f);								
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
		llinfos<<"Failed to create preview avatar for upload model window"<<llendl;
	}
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

	bool resetJoints = false;
	if ( upload_joints != mLastJointUpdate )
	{
		if ( mLastJointUpdate )
		{
			resetJoints = true;
		}

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

	if (has_skin_weights)
	{ //model has skin weights, enable view options for skin weights and joint positions
		if (fmp && isLegacyRigValid() )
		{
			fmp->enableViewOption("show_skin_weight");
			fmp->setViewOptionEnabled("show_joint_positions", skin_weight);	
			mFMP->childEnable("upload_skin");
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
		}
		skin_weight = false;
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

	if (use_shaders)
	{
		gObjectPreviewProgram.bind();
	}

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
		}

		//make sure material lists all match
		for (U32 i = 0; i < LLModel::NUM_LODS-1; i++)
		{
			if (mBaseModel.size() == mModel[i].size())
			{
				for (U32 j = 0; j < mBaseModel.size(); ++j)
				{
					int refFaceCnt = 0;
					int modelFaceCnt = 0;
										
					if ( !mModel[i][j]->matchMaterialOrder(mBaseModel[j], refFaceCnt, modelFaceCnt ) )
					{
						mFMP->childDisable( "calculate_btn" );
					}
				}
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

				for (U32 i = 0; i < mVertexBuffer[mPreviewLOD][model].size(); ++i)
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

							if (material.mDiffuseMap.notNull())
							{
								if (material.mDiffuseMap->getDiscardLevel() > -1)
								{
									gGL.getTexUnit(0)->bind(material.mDiffuseMap, true);
									mTextureSet.insert(material.mDiffuseMap.get());
								}
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

										glColor4ubv(hull_colors[i].mV);
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
							for (U32 i = 0; i < mVertexBuffer[LLModel::LOD_PHYSICS][model].size(); ++i)
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

			if (joint_positions)
			{
				LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
				if (shader)
				{
					gDebugProgram.bind();
				}
				getPreviewAvatar()->renderCollisionVolumes();
				if (shader)
				{
					shader->bind();
				}
			}

			for (LLModelLoader::scene::iterator iter = mScene[mPreviewLOD].begin(); iter != mScene[mPreviewLOD].end(); ++iter)
			{
				for (LLModelLoader::model_instance_list::iterator model_iter = iter->second.begin(); model_iter != iter->second.end(); ++model_iter)
				{
					LLModelInstance& instance = *model_iter;
					LLModel* model = instance.mModel;

					if (!model->mSkinWeights.empty())
					{
						for (U32 i = 0; i < mVertexBuffer[mPreviewLOD][model].size(); ++i)
						{
							LLVertexBuffer* buffer = mVertexBuffer[mPreviewLOD][model][i];

							const LLVolumeFace& face = model->getVolumeFace(i);

							LLStrider<LLVector3> position;
							buffer->getVertexStrider(position);

							LLStrider<LLVector4> weight;
							buffer->getWeight4Strider(weight);

							//quick 'n dirty software vertex skinning

							//build matrix palette
							
							LLMatrix4 mat[64];
							for (U32 j = 0; j < model->mSkinInfo.mJointNames.size(); ++j)
							{
								LLJoint* joint = getPreviewAvatar()->getJoint(model->mSkinInfo.mJointNames[j]);
								if (joint)
								{
									mat[j] = model->mSkinInfo.mInvBindMatrix[j];
									mat[j] *= joint->getWorldMatrix();
								}
							}

							for (U32 j = 0; j < buffer->getNumVerts(); ++j)
							{
								LLMatrix4 final_mat;
								final_mat.mMatrix[0][0] = final_mat.mMatrix[1][1] = final_mat.mMatrix[2][2] = final_mat.mMatrix[3][3] = 0.f;

								LLVector4 wght;
								S32 idx[4];

								F32 scale = 0.f;
								for (U32 k = 0; k < 4; k++)
								{
									F32 w = weight[j].mV[k];

									idx[k] = (S32) floorf(w);
									wght.mV[k] = w - floorf(w);
									scale += wght.mV[k];
								}

								wght *= 1.f/scale;

								for (U32 k = 0; k < 4; k++)
								{
									F32* src = (F32*) mat[idx[k]].mMatrix;
									F32* dst = (F32*) final_mat.mMatrix;

									F32 w = wght.mV[k];

									for (U32 l = 0; l < 16; l++)
									{
										dst[l] += src[l]*w;
									}
								}

								//VECTORIZE THIS
								LLVector3 v(face.mPositions[j].getF32ptr());

								v = v * model->mSkinInfo.mBindShapeMatrix;
								v = v * final_mat;

								position[j] = v;
							}

							const std::string& binding = instance.mModel->mMaterialList[i];
							const LLImportMaterial& material = instance.mMaterial[binding];

							buffer->setBuffer(type_mask & buffer->getTypeMask());
							gGL.diffuseColor4fv(material.mDiffuseColor.mV);
							gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
							if (material.mDiffuseMap.notNull())
							{
								if (material.mDiffuseMap->getDiscardLevel() > -1)
								{
									gGL.getTexUnit(0)->bind(material.mDiffuseMap, true);
									mTextureSet.insert(material.mDiffuseMap.get());
								}
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
		mFMP->childSetText("lod_file_" + lod_name[mPreviewLOD], mLODFile[mPreviewLOD]);

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
	std::string filename = mp->mLODFile[3]; 

	fmp->resetDisplayOptions();
	//reset model preview
	fmp->initModelPreview();

	mp = fmp->mModelPreview;
	mp->loadModel(filename,3,true);
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

	mp->mModelPreview->saveUploadData(upload_skinweights, upload_joint_positions);

	gMeshRepo.uploadModel(mp->mModelPreview->mUploadData, mp->mModelPreview->mPreviewScale,
						  mp->childGetValue("upload_textures").asBoolean(), upload_skinweights, upload_joint_positions, mp->mUploadModelUrl,
						  true, LLHandle<LLWholeModelFeeObserver>(), mp->getWholeModelUploadObserverHandle());
}


void LLFloaterModelPreview::refresh()
{
	sInstance->toggleCalculateButton(true);
	sInstance->mModelPreview->mDirty = true;
}

//static
void LLModelPreview::textureLoadedCallback( BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* src_aux, S32 discard_level, BOOL final, void* userdata )
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
	mUploadBtn->setEnabled(mHasUploadPerm && !mUploadModelUrl.empty());

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
	mUploadBtn->setEnabled(mHasUploadPerm && !mUploadModelUrl.empty());
}

void LLFloaterModelPreview::setModelPhysicsFeeErrorStatus(U32 status, const std::string& reason)
{
	llwarns << "LLFloaterModelPreview::setModelPhysicsFeeErrorStatus(" << status << " : " << reason << ")" << llendl;
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

	//mUploadBtn->setEnabled(mHasUploadPerm);
	mUploadBtn->setEnabled(mHasUploadPerm && !mUploadModelUrl.empty());
	getChild<LLTextBox>("warning_title")->setVisible(!mHasUploadPerm);
	getChild<LLTextBox>("warning_message")->setVisible(!mHasUploadPerm);
}

void LLFloaterModelPreview::setPermissonsErrorStatus(U32 status, const std::string& reason)
{
	llwarns << "LLFloaterModelPreview::setPermissonsErrorStatus(" << status << " : " << reason << ")" << llendl;

	LLNotificationsUtil::add("MeshUploadPermError");
}
