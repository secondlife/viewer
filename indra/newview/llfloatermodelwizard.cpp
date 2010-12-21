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
#include "llcombobox.h"
#include "llfloater.h"
#include "llfloatermodelwizard.h"
#include "llfloatermodelpreview.h"
#include "llfloaterreg.h"
#include "llslider.h"


static	const std::string stateNames[]={
	"choose_file",
	"optimize",
	"physics",
	"review",
	"upload"};

LLFloaterModelWizard::LLFloaterModelWizard(const LLSD& key)
	: LLFloater(key)
{
}

void LLFloaterModelWizard::setState(int state)
{
	mState = state;
	setButtons(state);

	for(size_t t=0; t<LL_ARRAY_SIZE(stateNames); ++t)
	{
		LLView *view = getChild<LLView>(stateNames[t]+"_panel");
		if (view) 
		{
			view->setVisible(state == (int) t ? TRUE : FALSE);
		}
	}

	if (state == OPTIMIZE)
	{
		mModelPreview->genLODs(-1);
	}
}

void LLFloaterModelWizard::setButtons(int state)
{
	for(size_t i=0; i<LL_ARRAY_SIZE(stateNames); ++i)
	{
		LLButton *button = getChild<LLButton>(stateNames[i]+"_btn");

		if (i < state)
		{
			button->setEnabled(TRUE);
			button->setToggleState(FALSE);
		}
		else if (i == state)
		{
			button->setEnabled(TRUE);
			button->setToggleState(TRUE);
		}
		else
		{
			button->setEnabled(FALSE);
		}
	}
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

BOOL LLFloaterModelWizard::postBuild()
{
	LLView* preview_panel = getChild<LLView>("preview_panel");

	childSetValue("import_scale", (F32) 0.67335826);

	getChild<LLUICtrl>("browse")->setCommitCallback(boost::bind(&LLFloaterModelWizard::loadModel, this));
	getChild<LLUICtrl>("cancel")->setCommitCallback(boost::bind(&LLFloaterModelWizard::onClickCancel, this));
	getChild<LLUICtrl>("back")->setCommitCallback(boost::bind(&LLFloaterModelWizard::onClickBack, this));
	getChild<LLUICtrl>("next")->setCommitCallback(boost::bind(&LLFloaterModelWizard::onClickNext, this));
	childSetCommitCallback("preview_lod_combo", onPreviewLODCommit, this);
	getChild<LLUICtrl>("accuracy_slider")->setCommitCallback(boost::bind(&LLFloaterModelWizard::onAccuracyPerformance, this, _2));

	childSetAction("ok_btn", onUpload, this);

	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
	
	enable_registrar.add("Next.OnEnable", boost::bind(&LLFloaterModelWizard::onEnableNext, this));
	enable_registrar.add("Back.OnEnable", boost::bind(&LLFloaterModelWizard::onEnableBack, this));

	
	mPreviewRect = preview_panel->getRect();
	
	mModelPreview = new LLModelPreview(512, 512, this);
	mModelPreview->setPreviewTarget(16.f);

	center();

	setState(CHOOSE_FILE);

	childSetTextArg("import_dimensions", "[X]", LLStringUtil::null);
	childSetTextArg("import_dimensions", "[Y]", LLStringUtil::null);
	childSetTextArg("import_dimensions", "[Z]", LLStringUtil::null);

	return TRUE;
}

void LLFloaterModelWizard::onUpload(void* user_data)
{
	LLFloaterModelWizard* mp = (LLFloaterModelWizard*) user_data;
	
	mp->mModelPreview->rebuildUploadData();
	
	gMeshRepo.uploadModel(mp->mModelPreview->mUploadData, mp->mModelPreview->mPreviewScale, 
						  mp->childGetValue("upload_textures").asBoolean(), mp->childGetValue("upload_skin"), mp->childGetValue("upload_joints"));
	
	mp->closeFloater(false);
}


void LLFloaterModelWizard::onAccuracyPerformance(const LLSD& data)
{
	int val = (int) data.asInteger();

	mModelPreview->genLODs(-1, NUM_LOD-val);

	mModelPreview->refresh();
}

void LLFloaterModelWizard::onPreviewLODCommit(LLUICtrl* ctrl, void* userdata)
{
	LLFloaterModelWizard *fp =(LLFloaterModelWizard *)userdata;
	
	if (!fp->mModelPreview)
	{
		return;
	}
	
	S32 which_mode = 0;
	
	LLComboBox* combo = (LLComboBox*) ctrl;
	
	which_mode = (NUM_LOD-1)-combo->getFirstSelectedIndex(); // combo box list of lods is in reverse order

	fp->mModelPreview->setPreviewLOD(which_mode);
}

void LLFloaterModelWizard::draw()
{
	LLFloater::draw();
	LLRect r = getRect();
	
	mModelPreview->update();

	if (mModelPreview)
	{
		gGL.color3f(1.f, 1.f, 1.f);
		
		gGL.getTexUnit(0)->bind(mModelPreview);
		
		LLView *view = getChild<LLView>(stateNames[mState]+"_panel");
		LLView* preview_panel = view->getChild<LLView>("preview_panel");

		LLRect rect = preview_panel->getRect();
		if (rect != mPreviewRect)
		{
			mModelPreview->refresh();
			mPreviewRect = preview_panel->getRect();
		}
		
		LLRect item_rect;
		preview_panel->localRectToOtherView(preview_panel->getLocalRect(), &item_rect, this);
		
		gGL.begin( LLRender::QUADS );
		{
			gGL.texCoord2f(0.f, 1.f);
			gGL.vertex2i(item_rect.mLeft, item_rect.mTop);
			gGL.texCoord2f(0.f, 0.f);
			gGL.vertex2i(item_rect.mLeft, item_rect.mBottom);
			gGL.texCoord2f(1.f, 0.f);
			gGL.vertex2i(item_rect.mRight, item_rect.mBottom);
			gGL.texCoord2f(1.f, 1.f);
			gGL.vertex2i(item_rect.mRight, item_rect.mTop);
		}
		gGL.end();
		
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	}
}
