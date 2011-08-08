/** 
 * @file llfloatermodelwizard.cpp
 * @author Leyla Farazha
 * @brief Implementation of the LLFloaterModelWizard class.
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

#include "llbutton.h"
#include "lldrawable.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfloater.h"
#include "llfloatermodelwizard.h"
#include "llfloatermodelpreview.h"
#include "llfloaterreg.h"
#include "llsliderctrl.h"
#include "lltoolmgr.h"
#include "llviewerwindow.h"

LLFloaterModelWizard* LLFloaterModelWizard::sInstance = NULL;

static	const std::string stateNames[]={
	"choose_file",
	"optimize",
	"physics",
	"review",
	"upload"};

static void swap_controls(LLUICtrl* first_ctrl, LLUICtrl* second_ctrl, bool first_ctr_visible);

LLFloaterModelWizard::LLFloaterModelWizard(const LLSD& key)
	: LLFloaterModelUploadBase(key)
	 ,mRecalculateGeometryBtn(NULL)
	 ,mRecalculatePhysicsBtn(NULL)
	 ,mRecalculatingPhysicsBtn(NULL)
	 ,mCalculateWeightsBtn(NULL)
	 ,mCalculatingWeightsBtn(NULL)
	 ,mChooseFilePreviewPanel(NULL)
	 ,mOptimizePreviewPanel(NULL)
	 ,mPhysicsPreviewPanel(NULL)
{
	mLastEnabledState = CHOOSE_FILE;
	sInstance = this;

	mCommitCallbackRegistrar.add("Wizard.Choose", boost::bind(&LLFloaterModelWizard::setState, this, CHOOSE_FILE));
	mCommitCallbackRegistrar.add("Wizard.Optimize", boost::bind(&LLFloaterModelWizard::setState, this, OPTIMIZE));
	mCommitCallbackRegistrar.add("Wizard.Physics", boost::bind(&LLFloaterModelWizard::setState, this, PHYSICS));
	mCommitCallbackRegistrar.add("Wizard.Review", boost::bind(&LLFloaterModelWizard::setState, this, REVIEW));
	mCommitCallbackRegistrar.add("Wizard.Upload", boost::bind(&LLFloaterModelWizard::setState, this, UPLOAD));
}
LLFloaterModelWizard::~LLFloaterModelWizard()
{
	sInstance = NULL;
}
void LLFloaterModelWizard::setState(int state)
{

	mState = state;

	for(size_t t=0; t<LL_ARRAY_SIZE(stateNames); ++t)
	{
		LLView *view = getChildView(stateNames[t]+"_panel");
		if (view) 
		{
			view->setVisible(state == (int) t ? TRUE : FALSE);
		}
	}

	LLView* current_preview_panel = NULL;

	if (state == CHOOSE_FILE)
	{
		mModelPreview->mViewOption["show_physics"] = false;

		current_preview_panel = mChooseFilePreviewPanel;

		getChildView("close")->setVisible(false);
		getChildView("back")->setVisible(true);
		getChildView("back")->setEnabled(false);
		getChildView("next")->setVisible(true);
		getChildView("upload")->setVisible(false);
		getChildView("cancel")->setVisible(true);
		mCalculateWeightsBtn->setVisible(false);
		mCalculatingWeightsBtn->setVisible(false);
	}

	if (state == OPTIMIZE)
	{
		if (mLastEnabledState < state)
		{			
			mModelPreview->genLODs(-1);
		}

		mModelPreview->mViewOption["show_physics"] = false;

		current_preview_panel = mOptimizePreviewPanel;

		getChildView("back")->setVisible(true);
		getChildView("back")->setEnabled(true);
		getChildView("close")->setVisible(false);
		getChildView("next")->setVisible(true);
		getChildView("upload")->setVisible(false);
		getChildView("cancel")->setVisible(true);
		mCalculateWeightsBtn->setVisible(false);
		mCalculatingWeightsBtn->setVisible(false);
	}

	if (state == PHYSICS)
	{
		if (mLastEnabledState < state)
		{
			mModelPreview->setPhysicsFromLOD(1);

			// Trigger the recalculate physics when first entering
			// the Physics step.
			onClickRecalculatePhysics();
		}

		mModelPreview->mViewOption["show_physics"] = true;

		current_preview_panel = mPhysicsPreviewPanel;

		getChildView("next")->setVisible(false);
		getChildView("upload")->setVisible(false);
		getChildView("close")->setVisible(false);
		getChildView("back")->setVisible(true);
		getChildView("back")->setEnabled(true);
		getChildView("cancel")->setVisible(true);
		mCalculateWeightsBtn->setVisible(true);
		mCalculatingWeightsBtn->setVisible(false);
	}

	if (state == REVIEW)
	{
		
		mModelPreview->mViewOption["show_physics"] = false;

		getChildView("close")->setVisible(false);
		getChildView("next")->setVisible(false);
		getChildView("back")->setVisible(true);
		getChildView("back")->setEnabled(true);
		getChildView("upload")->setVisible(true);
		getChildView("cancel")->setVisible(true);
		mCalculateWeightsBtn->setVisible(false);
		mCalculatingWeightsBtn->setVisible(false);
	}

	if (state == UPLOAD)
	{
		getChildView("close")->setVisible(true);
		getChildView("next")->setVisible(false);
		getChildView("back")->setVisible(false);
		getChildView("upload")->setVisible(false);
		getChildView("cancel")->setVisible(false);
		mCalculateWeightsBtn->setVisible(false);
		mCalculatingWeightsBtn->setVisible(false);
	}

	if (current_preview_panel)
	{
		LLRect rect;
		current_preview_panel->localRectToOtherView(current_preview_panel->getLocalRect(), &rect, this);

		// Reduce the preview rect by 1 px to fit the borders
		rect.stretch(-1);

		if (rect != mPreviewRect)
		{
			mPreviewRect = rect;
			mModelPreview->refresh();
		}
	}
	updateButtons();
}



void LLFloaterModelWizard::updateButtons()
{
	if (mLastEnabledState < mState)
	{
		mLastEnabledState = mState;
	}

	for(size_t i=0; i<LL_ARRAY_SIZE(stateNames); ++i)
	{
		LLButton *button = getChild<LLButton>(stateNames[i]+"_btn");

		if (i == mState)
		{
			button->setEnabled(TRUE);
			button->setToggleState(TRUE);
		}
		else if (i <= mLastEnabledState)
		{
			button->setEnabled(TRUE);
			button->setToggleState(FALSE);
		}
		else
		{
			button->setEnabled(FALSE);
		}
	}
}

void LLFloaterModelWizard::onClickSwitchToAdvanced()
{
	LLFloaterModelPreview* floater_preview = LLFloaterReg::getTypedInstance<LLFloaterModelPreview>("upload_model");
	if (!floater_preview)
	{
		llwarns << "FLoater model preview not found." << llendl;
		return;
	}

	// Open floater model preview
	floater_preview->openFloater();

	// Close the wizard
	closeFloater();

	std::string filename = getChild<LLUICtrl>("lod_file")->getValue().asString();
	if (!filename.empty())
	{
		// Re-load the model to the floater model preview if it has been loaded
		// into the wizard.
		floater_preview->loadModel(3, filename);
	}
}

void LLFloaterModelWizard::onClickRecalculateGeometry()
{
	S32 val = getChild<LLUICtrl>("accuracy_slider")->getValue().asInteger();

	mModelPreview->genLODs(-1, NUM_LOD - val);

	mModelPreview->refresh();
}

void LLFloaterModelWizard::onClickRecalculatePhysics()
{
	// Hide the "Recalculate physics" button and show the "Recalculating..."
	// button instead.
	swap_controls(mRecalculatePhysicsBtn, mRecalculatingPhysicsBtn, false);

	executePhysicsStage("Decompose");
}

void LLFloaterModelWizard::onClickCalculateUploadFee()
{
	swap_controls(mCalculateWeightsBtn, mCalculatingWeightsBtn, false);

	mModelPreview->rebuildUploadData();

	mUploadModelUrl.clear();

	gMeshRepo.uploadModel(mModelPreview->mUploadData, mModelPreview->mPreviewScale,
			true, false, false, mUploadModelUrl, false, getWholeModelFeeObserverHandle());
}

void LLFloaterModelWizard::loadModel()
{
	 mModelPreview->mLoading = TRUE;
	
	(new LLMeshFilePicker(mModelPreview, 3))->getFile();
}

void LLFloaterModelWizard::onClickCancel()
{
	closeFloater();
}

void LLFloaterModelWizard::onClickBack()
{
	setState(llmax((int) CHOOSE_FILE, mState-1));
}

void LLFloaterModelWizard::onClickNext()
{
	setState(llmin((int) UPLOAD, mState+1));
}

bool LLFloaterModelWizard::onEnableNext()
{
	return true;
}

bool LLFloaterModelWizard::onEnableBack()
{
	return true;
}


//-----------------------------------------------------------------------------
// handleMouseDown()
//-----------------------------------------------------------------------------
BOOL LLFloaterModelWizard::handleMouseDown(S32 x, S32 y, MASK mask)
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
BOOL LLFloaterModelWizard::handleMouseUp(S32 x, S32 y, MASK mask)
{
	gFocusMgr.setMouseCapture(FALSE);
	gViewerWindow->showCursor();
	return LLFloater::handleMouseUp(x, y, mask);
}

//-----------------------------------------------------------------------------
// handleHover()
//-----------------------------------------------------------------------------
BOOL LLFloaterModelWizard::handleHover	(S32 x, S32 y, MASK mask)
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
BOOL LLFloaterModelWizard::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	if (mPreviewRect.pointInRect(x, y) && mModelPreview)
	{
		mModelPreview->zoom((F32)clicks * -0.2f);
		mModelPreview->refresh();
	}
	
	return TRUE;
}


void LLFloaterModelWizard::initDecompControls()
{
	LLSD key;

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
		gMeshRepo.mDecompThread->mStageID[stage[j].mName] = j;
		// protected against stub by stage_count being 0 for stub above
		LLConvexDecomposition::getInstance()->registerCallback(j, LLPhysicsDecomp::llcdCallback);

		for (S32 i = 0; i < param_count; ++i)
		{
			if (param[i].mStage != j)
			{
				continue;
			}

			std::string name(param[i].mName ? param[i].mName : "");
			std::string description(param[i].mDescription ? param[i].mDescription : "");

			if (param[i].mType == LLCDParam::LLCD_FLOAT)
			{
				mDecompParams[param[i].mName] = LLSD(param[i].mDefault.mFloat);
			}
			else if (param[i].mType == LLCDParam::LLCD_INTEGER)
			{
				mDecompParams[param[i].mName] = LLSD(param[i].mDefault.mIntOrEnumValue);
			}
			else if (param[i].mType == LLCDParam::LLCD_BOOLEAN)
			{
				mDecompParams[param[i].mName] = LLSD(param[i].mDefault.mBool);
			}
			else if (param[i].mType == LLCDParam::LLCD_ENUM)
			{
				mDecompParams[param[i].mName] = LLSD(param[i].mDefault.mIntOrEnumValue);
			}
		}
	}

	mDecompParams["Simplify Method"] = 0; // set it to retain %
}

/*virtual*/
void LLFloaterModelWizard::onPermissionsReceived(const LLSD& result)
{
	std::string upload_status = result["mesh_upload_status"].asString();
	// BAP HACK: handle "" for case that  MeshUploadFlag cap is broken.
	mHasUploadPerm = (("" == upload_status) || ("valid" == upload_status));

	getChildView("warning_label")->setVisible(!mHasUploadPerm);
	getChildView("warning_text")->setVisible(!mHasUploadPerm);
}

/*virtual*/
void LLFloaterModelWizard::setPermissonsErrorStatus(U32 status, const std::string& reason)
{
	llwarns << "LLFloaterModelWizard::setPermissonsErrorStatus(" << status << " : " << reason << ")" << llendl;
}

/*virtual*/
void LLFloaterModelWizard::onModelPhysicsFeeReceived(const LLSD& result, std::string upload_url)
{
	swap_controls(mCalculateWeightsBtn, mCalculatingWeightsBtn, true);

	// Enable the "Upload" buton if we have calculated the upload fee
	// and have the permission to upload.
	getChildView("upload")->setEnabled(mHasUploadPerm);

	mUploadModelUrl = upload_url;

	S32 fee = result["upload_price"].asInteger();
	childSetTextArg("review_fee", "[FEE]", llformat("%d", fee));
	childSetTextArg("charged_fee", "[FEE]", llformat("%d", fee));

	setState(REVIEW);
}

/*virtual*/
void LLFloaterModelWizard::setModelPhysicsFeeErrorStatus(U32 status, const std::string& reason)
{
	swap_controls(mCalculateWeightsBtn, mCalculatingWeightsBtn, true);

	// Disable the "Review" step if it has been previously enabled.
	modelChangedCallback();

	llwarns << "LLFloaterModelWizard::setModelPhysicsFeeErrorStatus(" << status << " : " << reason << ")" << llendl;

	setState(PHYSICS);
}

/*virtual*/ 
void LLFloaterModelWizard::onModelUploadSuccess() 
{
	// success!
	setState(UPLOAD);
}

/*virtual*/
void LLFloaterModelWizard::onModelUploadFailure()
{
	// Failure. Make the user recalculate fees
	setState(PHYSICS);
	// Disable the "Review" step if it has been previously enabled.
	if (mLastEnabledState > PHYSICS)
	{
		 mLastEnabledState = PHYSICS;
	}

	updateButtons();
}

//static
void LLFloaterModelWizard::executePhysicsStage(std::string stage_name)
{
	if (sInstance)
	{
		// Invert the slider value so that "performance" end is giving the least detailed physics,
		// and the "accuracy" end is giving the most detailed physics
		F64 physics_accuracy = 1 - sInstance->getChild<LLSliderCtrl>("physics_slider")->getValue().asReal();

		sInstance->mDecompParams["Retain%"] = physics_accuracy;

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
				DecompRequest* request = new DecompRequest(stage_name, mdl);
				if(request->isValid())
				{
					sInstance->mCurRequest.insert(request);
					gMeshRepo.mDecompThread->submitRequest(request);
				}				
			}
		}
	}
}

LLFloaterModelWizard::DecompRequest::DecompRequest(const std::string& stage, LLModel* mdl)
{
	mStage = stage;
	mContinue = 1;
	mModel = mdl;
	mDecompID = &mdl->mDecompID;
	mParams = sInstance->mDecompParams;

	//copy out positions and indices
	assignData(mdl) ;	
}


S32 LLFloaterModelWizard::DecompRequest::statusCallback(const char* status, S32 p1, S32 p2)
{
	setStatusMessage(llformat("%s: %d/%d", status, p1, p2));

	return mContinue;
}

void LLFloaterModelWizard::DecompRequest::completed()
{ //called from the main thread
	mModel->setConvexHullDecomposition(mHull);

	if (sInstance)
	{
		if (sInstance->mModelPreview)
		{
			sInstance->mModelPreview->mDirty = true;
			LLFloaterModelWizard::sInstance->mModelPreview->refresh();
		}

		sInstance->mCurRequest.erase(this);
	}

	if (mStage == "Decompose")
	{
		executePhysicsStage("Simplify");
	}
	else
	{
		// Decomp request is complete so we can enable the "Recalculate physics" button again.
		swap_controls(sInstance->mRecalculatePhysicsBtn, sInstance->mRecalculatingPhysicsBtn, true);
	}
}


BOOL LLFloaterModelWizard::postBuild()
{
	childSetValue("import_scale", (F32) 0.67335826);

	getChild<LLUICtrl>("browse")->setCommitCallback(boost::bind(&LLFloaterModelWizard::loadModel, this));
	//getChild<LLUICtrl>("lod_file")->setCommitCallback(boost::bind(&LLFloaterModelWizard::loadModel, this));
	getChild<LLUICtrl>("cancel")->setCommitCallback(boost::bind(&LLFloaterModelWizard::onClickCancel, this));
	getChild<LLUICtrl>("close")->setCommitCallback(boost::bind(&LLFloaterModelWizard::onClickCancel, this));
	getChild<LLUICtrl>("back")->setCommitCallback(boost::bind(&LLFloaterModelWizard::onClickBack, this));
	getChild<LLUICtrl>("next")->setCommitCallback(boost::bind(&LLFloaterModelWizard::onClickNext, this));
	getChild<LLUICtrl>("preview_lod_combo")->setCommitCallback(boost::bind(&LLFloaterModelWizard::onPreviewLODCommit, this, _1));
	getChild<LLUICtrl>("preview_lod_combo2")->setCommitCallback(boost::bind(&LLFloaterModelWizard::onPreviewLODCommit, this, _1));
	getChild<LLUICtrl>("upload")->setCommitCallback(boost::bind(&LLFloaterModelWizard::onUpload, this));
	getChild<LLUICtrl>("switch_to_advanced")->setCommitCallback(boost::bind(&LLFloaterModelWizard::onClickSwitchToAdvanced, this));

	mRecalculateGeometryBtn = getChild<LLButton>("recalculate_geometry_btn");
	mRecalculateGeometryBtn->setCommitCallback(boost::bind(&LLFloaterModelWizard::onClickRecalculateGeometry, this));

	mRecalculatePhysicsBtn = getChild<LLButton>("recalculate_physics_btn");
	mRecalculatePhysicsBtn->setCommitCallback(boost::bind(&LLFloaterModelWizard::onClickRecalculatePhysics, this));

	mRecalculatingPhysicsBtn = getChild<LLButton>("recalculating_physics_btn");

	mCalculateWeightsBtn = getChild<LLButton>("calculate");
	mCalculateWeightsBtn->setCommitCallback(boost::bind(&LLFloaterModelWizard::onClickCalculateUploadFee, this));

	mCalculatingWeightsBtn = getChild<LLButton>("calculating");

	mChooseFilePreviewPanel = getChild<LLView>("choose_file_preview_panel");
	mOptimizePreviewPanel = getChild<LLView>("optimize_preview_panel");
	mPhysicsPreviewPanel = getChild<LLView>("physics_preview_panel");

	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
	
	enable_registrar.add("Next.OnEnable", boost::bind(&LLFloaterModelWizard::onEnableNext, this));
	enable_registrar.add("Back.OnEnable", boost::bind(&LLFloaterModelWizard::onEnableBack, this));
	
	mModelPreview = new LLModelPreview(512, 512, this);
	mModelPreview->setPreviewTarget(16.f);
	mModelPreview->setDetailsCallback(boost::bind(&LLFloaterModelWizard::setDetails, this, _1, _2, _3, _4, _5));
	mModelPreview->setModelLoadedCallback(boost::bind(&LLFloaterModelWizard::modelLoadedCallback, this));
	mModelPreview->setModelUpdatedCallback(boost::bind(&LLFloaterModelWizard::modelChangedCallback, this));
	mModelPreview->mViewOption["show_textures"] = true;

	center();

	setState(CHOOSE_FILE);

	childSetTextArg("import_dimensions", "[X]", LLStringUtil::null);
	childSetTextArg("import_dimensions", "[Y]", LLStringUtil::null);
	childSetTextArg("import_dimensions", "[Z]", LLStringUtil::null);

	initDecompControls();

	requestAgentUploadPermissions();

	return TRUE;
}


void LLFloaterModelWizard::setDetails(F32 x, F32 y, F32 z, F32 streaming_cost, F32 physics_cost)
{
	// iterate through all the panels, setting the dimensions
	for(size_t t=0; t<LL_ARRAY_SIZE(stateNames); ++t)
	{
		LLPanel *panel = getChild<LLPanel>(stateNames[t]+"_panel");
		if (panel) 
		{
			panel->childSetText("dimension_x", llformat("%.1f", x));
			panel->childSetText("dimension_y", llformat("%.1f", y));
			panel->childSetText("dimension_z", llformat("%.1f", z));
		}
	}

	childSetTextArg("review_prim_equiv", "[EQUIV]", llformat("%d", mModelPreview->mResourceCost));
}

void LLFloaterModelWizard::modelLoadedCallback()
{
	mLastEnabledState = CHOOSE_FILE;
	updateButtons();
}

void LLFloaterModelWizard::modelChangedCallback()
{
	// Don't allow to proceed to the "Review" step if the model has changed
	// but the new upload fee hasn't been calculated yet.
	if (mLastEnabledState > PHYSICS)
	{
		 mLastEnabledState = PHYSICS;
	}

	getChildView("upload")->setEnabled(false);

	updateButtons();
}

void LLFloaterModelWizard::onUpload()
{	
	mModelPreview->rebuildUploadData();
	
	gMeshRepo.uploadModel(mModelPreview->mUploadData, mModelPreview->mPreviewScale, 
						  true, false, false, mUploadModelUrl, true,
						  LLHandle<LLWholeModelFeeObserver>(), getWholeModelUploadObserverHandle());
}

void LLFloaterModelWizard::onPreviewLODCommit(LLUICtrl* ctrl)
{
	if (!mModelPreview)
	{
		return;
	}
	
	S32 which_mode = 0;
	
	LLComboBox* combo = (LLComboBox*) ctrl;
	
	which_mode = (NUM_LOD-1)-combo->getFirstSelectedIndex(); // combo box list of lods is in reverse order

	mModelPreview->setPreviewLOD(which_mode);
}

void LLFloaterModelWizard::refresh()
{
	if (mState == CHOOSE_FILE)
	{
		bool model_loaded = false;

		if (mModelPreview && mModelPreview->getLoadState() == LLModelLoader::DONE)
		{
			model_loaded = true;
		}
		
		getChildView("next")->setEnabled(model_loaded);
	}
}

void LLFloaterModelWizard::draw()
{
	refresh();

	LLFloater::draw();

	if (mModelPreview && mState < REVIEW)
	{
		mModelPreview->update();

		gGL.color3f(1.f, 1.f, 1.f);
		
		gGL.getTexUnit(0)->bind(mModelPreview);
		
		gGL.begin( LLRender::QUADS );
		{
			gGL.texCoord2f(0.f, 1.f);
			gGL.vertex2i(mPreviewRect.mLeft, mPreviewRect.mTop);
			gGL.texCoord2f(0.f, 0.f);
			gGL.vertex2i(mPreviewRect.mLeft, mPreviewRect.mBottom);
			gGL.texCoord2f(1.f, 0.f);
			gGL.vertex2i(mPreviewRect.mRight, mPreviewRect.mBottom);
			gGL.texCoord2f(1.f, 1.f);
			gGL.vertex2i(mPreviewRect.mRight, mPreviewRect.mTop);
		}
		gGL.end();
		
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	}
}

// static
void swap_controls(LLUICtrl* first_ctrl, LLUICtrl* second_ctrl, bool first_ctr_visible)
{
	first_ctrl->setVisible(first_ctr_visible);
	second_ctrl->setVisible(!first_ctr_visible);
}
