/** 
 * @file llscenemonitor.cpp
 * @brief monitor the scene loading process.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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
#include "llrendertarget.h"
#include "llscenemonitor.h"
#include "llviewerwindow.h"
#include "llviewerdisplay.h"
#include "llviewercontrol.h"
#include "llviewershadermgr.h"
#include "llui.h"
#include "llstartup.h"
#include "llappviewer.h"
#include "llwindow.h"
#include "llpointer.h"

LLSceneMonitorView* gSceneMonitorView = NULL;

LLSceneMonitor::LLSceneMonitor() : 
	mEnabled(FALSE), 
	mDiff(NULL), 
	mCurTarget(NULL), 
	mNeedsUpdateDiff(FALSE), 
	mDebugViewerVisible(FALSE)
{
	mFrames[0] = NULL;
	mFrames[1] = NULL;
}

LLSceneMonitor::~LLSceneMonitor()
{
	destroyClass();
}

void LLSceneMonitor::destroyClass()
{
	reset();
}

void LLSceneMonitor::reset()
{
	delete mFrames[0];
	delete mFrames[1];
	delete mDiff;

	mFrames[0] = NULL;
	mFrames[1] = NULL;
	mDiff = NULL;
}

void LLSceneMonitor::setDebugViewerVisible(BOOL visible) 
{
	mDebugViewerVisible = visible;
}

bool LLSceneMonitor::preCapture()
{
	static LLCachedControl<bool> monitor_enabled(gSavedSettings,"SceneLoadingMonitorEnabled");
	static LLFrameTimer timer;	

	mCurTarget = NULL;
	if (!LLGLSLShader::sNoFixedFunction)
	{
		return false;
	}

	BOOL enabled = (BOOL)monitor_enabled || mDebugViewerVisible;
	if(mEnabled != enabled)
	{
		if(mEnabled)
		{
			reset();
			unfreezeScene();
		}
		else
		{
			freezeScene();
		}

		mEnabled = enabled;
	}

	if(!mEnabled)
	{
		return false;
	}

	if (LLStartUp::getStartupState() < STATE_STARTED)
	{
		return false;
	}

	if(LLAppViewer::instance()->logoutRequestSent())
	{
		return false;
	}

	if(gWindowResized || gHeadlessClient || gTeleportDisplay || gRestoreGL || gDisconnected)
	{
		return false;
	}

	if (   !gViewerWindow->getActive()
		|| !gViewerWindow->getWindow()->getVisible() 
		|| gViewerWindow->getWindow()->getMinimized() )
	{
		return false;
	}

	if(timer.getElapsedTimeF32() < 1.0f)
	{
		return false;
	}
	timer.reset();
	
	S32 width = gViewerWindow->getWorldViewWidthRaw();
	S32 height = gViewerWindow->getWorldViewHeightRaw();
	
	if(!mFrames[0])
	{
		mFrames[0] = new LLRenderTarget();
		mFrames[0]->allocate(width, height, GL_RGB, false, false, LLTexUnit::TT_TEXTURE, true);
		gGL.getTexUnit(0)->bind(mFrames[0]);
		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		mCurTarget = mFrames[0];
	}
	else if(!mFrames[1])
	{
		mFrames[1] = new LLRenderTarget();
		mFrames[1]->allocate(width, height, GL_RGB, false, false, LLTexUnit::TT_TEXTURE, true);
		gGL.getTexUnit(0)->bind(mFrames[1]);
		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		mCurTarget = mFrames[1];
	}
	else //swap
	{
		mCurTarget = mFrames[0];
		mFrames[0] = mFrames[1];
		mFrames[1] = mCurTarget;
	}
	
	if(mCurTarget->getWidth() != width || mCurTarget->getHeight() != height) //size changed
	{
		mCurTarget->resize(width, height, GL_RGB);
	}

	return true;
}

void LLSceneMonitor::postCapture()
{
	mCurTarget = NULL;
	mNeedsUpdateDiff = TRUE;
}

void LLSceneMonitor::freezeAvatar(LLCharacter* avatarp)
{
	mAvatarPauseHandles.push_back(avatarp->requestPause());
}

void LLSceneMonitor::freezeScene()
{
	//freeze all avatars
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		iter != LLCharacter::sInstances.end(); ++iter)
	{
		freezeAvatar((LLCharacter*)(*iter));
	}

	// freeze everything else
	gSavedSettings.setBOOL("FreezeTime", TRUE);
}

void LLSceneMonitor::unfreezeScene()
{
	//thaw all avatars
	mAvatarPauseHandles.clear();

	// thaw everything else
	gSavedSettings.setBOOL("FreezeTime", FALSE);
}

LLRenderTarget* LLSceneMonitor::getDiffTarget() const 
{
	return mDiff;
}

void LLSceneMonitor::capture()
{
	static U32 count = 0;
	if(count == gFrameCount)
	{
		return;
	}
	count = gFrameCount;

	preCapture();

	if(!mCurTarget)
	{
		return;
	}
	
	U32 old_FBO = LLRenderTarget::sCurFBO;

	gGL.getTexUnit(0)->bind(mCurTarget);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); //point to the main frame buffer.
		
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, mCurTarget->getWidth(), mCurTarget->getHeight()); //copy the content
	
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);		
	glBindFramebuffer(GL_FRAMEBUFFER, old_FBO);

	postCapture();
}

void LLSceneMonitor::compare()
{
	if(!mNeedsUpdateDiff)
	{
		return;
	}

	if(!mFrames[0] || !mFrames[1])
	{
		return;
	}
	if(mFrames[0]->getWidth() != mFrames[1]->getWidth() || mFrames[0]->getHeight() != mFrames[1]->getHeight())
	{
		return; //size does not match
	}

	if (!LLGLSLShader::sNoFixedFunction)
	{
		return;
	}

	S32 width = gViewerWindow->getWindowWidthRaw();
	S32 height = gViewerWindow->getWindowHeightRaw();
	if(!mDiff)
	{
		mDiff = new LLRenderTarget();
		mDiff->allocate(width, height, GL_RGBA, false, false, LLTexUnit::TT_TEXTURE, true);
	}
	else if(mDiff->getWidth() != width || mDiff->getHeight() != height)
	{
		mDiff->resize(width, height, GL_RGBA);
	}
	mDiff->bindTarget();
	mDiff->clear();

	gTwoTextureCompareProgram.bind();
	
	gGL.getTexUnit(0)->activate();
	gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
	gGL.getTexUnit(0)->bind(mFrames[0]);
	gGL.getTexUnit(0)->activate();

	gGL.getTexUnit(1)->activate();
	gGL.getTexUnit(1)->enable(LLTexUnit::TT_TEXTURE);
	gGL.getTexUnit(1)->bind(mFrames[1]);
	gGL.getTexUnit(1)->activate();	
			
	gl_rect_2d_simple_tex(width, height);
	
	mDiff->flush();

	gTwoTextureCompareProgram.unbind();
	
	gGL.getTexUnit(0)->disable();
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.getTexUnit(1)->disable();
	gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);

	mNeedsUpdateDiff = FALSE;
}

//-------------------------------------------------------------------------------------------------------------
//definition of class LLSceneMonitorView
//-------------------------------------------------------------------------------------------------------------
LLSceneMonitorView::LLSceneMonitorView(const LLRect& rect)
	:	LLFloater(LLSD())
{
	setRect(rect);
	setVisible(FALSE);
	
	setCanMinimize(false);
	setCanClose(true);
}

void LLSceneMonitorView::onClickCloseBtn()
{
	setVisible(false);	
}

void LLSceneMonitorView::setVisible(BOOL visible)
{
	LLSceneMonitor::getInstance()->setDebugViewerVisible(visible);

	LLView::setVisible(visible);
}

void LLSceneMonitorView::draw()
{
	if (!LLGLSLShader::sNoFixedFunction)
	{
		return;
	}
	LLRenderTarget* target = LLSceneMonitor::getInstance()->getDiffTarget();
	if(!target)
	{
		return;
	}

	S32 height = (S32) (gViewerWindow->getWindowRectScaled().getHeight()*0.5f);
	S32 width = (S32) (gViewerWindow->getWindowRectScaled().getWidth() * 0.5f);
	
	LLRect new_rect;
	new_rect.setLeftTopAndSize(getRect().mLeft, getRect().mTop, width, height);
	setRect(new_rect);

	//gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	//gl_rect_2d(0, getRect().getHeight(), getRect().getWidth(), 0, LLColor4(0.f, 0.f, 0.f, 0.25f));
	
	gl_draw_scaled_target(0, 0, getRect().getWidth(), getRect().getHeight(), target);

	LLView::draw();
}

