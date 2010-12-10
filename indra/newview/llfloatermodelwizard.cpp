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

#include "lldrawable.h"
#include "llfloater.h"
#include "llfloatermodelwizard.h"
#include "llfloatermodelpreview.h"
#include "llfloaterreg.h"



LLFloaterModelWizard::LLFloaterModelWizard(const LLSD& key)
	: LLFloater(key)
{
}

void LLFloaterModelWizard::loadModel()
{
	 mModelPreview->mLoading = TRUE;
	
	(new LLMeshFilePicker(mModelPreview, 3))->getFile();
}


BOOL LLFloaterModelWizard::postBuild()
{
	LLView* preview_panel = getChild<LLView>("preview_panel");

	childSetValue("import_scale", (F32) 0.67335826);

	getChild<LLUICtrl>("browse")->setCommitCallback(boost::bind(&LLFloaterModelWizard::loadModel, this));

	mPreviewRect = preview_panel->getRect();
	
	mModelPreview = new LLModelPreview(512, 512, this);
	mModelPreview->setPreviewTarget(16.f);
	mModelPreview->setAspect((F32) mPreviewRect.getWidth()/mPreviewRect.getHeight());

	center();

	return TRUE;
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
		

		LLView* preview_panel = getChild<LLView>("preview_panel");

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
			gGL.vertex2i(mPreviewRect.mLeft, item_rect.mTop);
			gGL.texCoord2f(0.f, 0.f);
			gGL.vertex2i(mPreviewRect.mLeft, item_rect.mBottom);
			gGL.texCoord2f(1.f, 0.f);
			gGL.vertex2i(mPreviewRect.mRight, item_rect.mBottom);
			gGL.texCoord2f(1.f, 1.f);
			gGL.vertex2i(mPreviewRect.mRight, item_rect.mTop);
		}
		gGL.end();
		
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	}
}
