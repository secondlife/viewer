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
#include "llspatialpartition.h"
#include "llagent.h"
#include "pipeline.h"

LLSceneMonitorView* gSceneMonitorView = NULL;

//
//The procedures of monitoring when the scene finishes loading visually, 
//i.e., no pixel differences among frames, are:
//1, freeze all dynamic objects and avatars;
//2, (?) disable all sky and water;
//3, capture frames periodically, by calling "capture()";
//4, compute pixel differences between two latest captured frames, by calling "compare()", results are stored at mDiff;
//5, compute the number of pixels in mDiff above some tolerance threshold in GPU, by calling "queryDiff() -> calcDiffAggregate()";
//6, use gl occlusion query to fetch the result from GPU, by calling "fetchQueryResult()";
//END.
//

LLSceneMonitor::LLSceneMonitor() : 
	mEnabled(FALSE), 
	mDiff(NULL),
	mDiffResult(0.f),
	mDiffTolerance(0.1f),
	mCurTarget(NULL), 
	mNeedsUpdateDiff(FALSE),
	mHasNewDiff(FALSE),
	mHasNewQueryResult(FALSE),
	mDebugViewerVisible(FALSE),
	mQueryObject(0),
	mSamplingTime(1.0f),
	mDiffPixelRatio(0.5f)
{
	mFrames[0] = NULL;
	mFrames[1] = NULL;

	mRecording = new LLTrace::ExtendableRecording();
	mRecording->start();
}

LLSceneMonitor::~LLSceneMonitor()
{
	destroyClass();
}

void LLSceneMonitor::destroyClass()
{
	reset();

	delete mRecording;
	mRecording = NULL;
	mDitheringTexture = NULL;
}

void LLSceneMonitor::reset()
{
	delete mFrames[0];
	delete mFrames[1];
	delete mDiff;

	mFrames[0] = NULL;
	mFrames[1] = NULL;
	mDiff = NULL;
	mCurTarget = NULL;

	unfreezeScene();

	if(mQueryObject > 0)
	{
		release_occlusion_query_object_name(mQueryObject);
		mQueryObject = 0;
	}
}

void LLSceneMonitor::generateDitheringTexture(S32 width, S32 height)
{
#if 1
	//4 * 4 matrix
	mDitherMatrixWidth = 4;	
	S32 dither_matrix[4][4] = 
	{
		{1, 9, 3, 11}, 
		{13, 5, 15, 7}, 
		{4, 12, 2, 10}, 
		{16, 8, 14, 6}
	};
	
	mDitherScale = 255.f / 17;
#else
	//8 * 8 matrix
	mDitherMatrixWidth = 16;	
	S32 dither_matrix[16][16] = 
	{
		{1, 49, 13, 61, 4, 52, 16, 64, 1, 49, 13, 61, 4, 52, 16, 64}, 
		{33, 17, 45, 29, 36, 20, 48, 32, 33, 17, 45, 29, 36, 20, 48, 32}, 
		{9, 57, 5, 53, 12, 60, 8, 56, 9, 57, 5, 53, 12, 60, 8, 56}, 
		{41, 25, 37, 21, 44, 28, 40, 24, 41, 25, 37, 21, 44, 28, 40, 24},
		{3, 51, 15, 63, 2, 50, 14, 62, 3, 51, 15, 63, 2, 50, 14, 62},
		{35, 19, 47, 31, 34, 18, 46, 30, 35, 19, 47, 31, 34, 18, 46, 30},
		{11, 59, 7, 55, 10, 58, 6, 54, 11, 59, 7, 55, 10, 58, 6, 54},
		{43, 27, 39, 23, 42, 26, 38, 22, 43, 27, 39, 23, 42, 26, 38, 22},
		{1, 49, 13, 61, 4, 52, 16, 64, 1, 49, 13, 61, 4, 52, 16, 64}, 
		{33, 17, 45, 29, 36, 20, 48, 32, 33, 17, 45, 29, 36, 20, 48, 32}, 
		{9, 57, 5, 53, 12, 60, 8, 56, 9, 57, 5, 53, 12, 60, 8, 56}, 
		{41, 25, 37, 21, 44, 28, 40, 24, 41, 25, 37, 21, 44, 28, 40, 24},
		{3, 51, 15, 63, 2, 50, 14, 62, 3, 51, 15, 63, 2, 50, 14, 62},
		{35, 19, 47, 31, 34, 18, 46, 30, 35, 19, 47, 31, 34, 18, 46, 30},
		{11, 59, 7, 55, 10, 58, 6, 54, 11, 59, 7, 55, 10, 58, 6, 54},
		{43, 27, 39, 23, 42, 26, 38, 22, 43, 27, 39, 23, 42, 26, 38, 22}
	};

	mDitherScale = 255.f / 65;
#endif

	LLPointer<LLImageRaw> image_raw = new LLImageRaw(mDitherMatrixWidth, mDitherMatrixWidth, 3);
	U8* data = image_raw->getData();
	for (S32 i = 0; i < mDitherMatrixWidth; i++)
	{
		for (S32 j = 0; j < mDitherMatrixWidth; j++)
		{
			U8 val = dither_matrix[i][j];
			*data++ = val;
			*data++ = val;
			*data++ = val;
		}
	}

	mDitheringTexture = LLViewerTextureManager::getLocalTexture(image_raw.get(), FALSE) ;
	mDitheringTexture->setAddressMode(LLTexUnit::TAM_WRAP);
	mDitheringTexture->setFilteringOption(LLTexUnit::TFO_POINT);
	
	mDitherScaleS = (F32)width / mDitherMatrixWidth;
	mDitherScaleT = (F32)height / mDitherMatrixWidth;
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

	if(gAgent.isPositionChanged())
	{
		mRecording->reset();
	}

	if(timer.getElapsedTimeF32() < mSamplingTime)
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

	gPipeline.clearRenderTypeMask(LLPipeline::RENDER_TYPE_SKY, LLPipeline::RENDER_TYPE_WL_SKY, 
		LLPipeline::RENDER_TYPE_WATER, LLPipeline::RENDER_TYPE_CLOUDS, LLPipeline::END_RENDER_TYPES);
}

void LLSceneMonitor::unfreezeScene()
{
	//thaw all avatars
	mAvatarPauseHandles.clear();

	// thaw everything else
	gSavedSettings.setBOOL("FreezeTime", FALSE);

	gPipeline.setRenderTypeMask(LLPipeline::RENDER_TYPE_SKY, LLPipeline::RENDER_TYPE_WL_SKY, 
		LLPipeline::RENDER_TYPE_WATER, LLPipeline::RENDER_TYPE_CLOUDS, LLPipeline::END_RENDER_TYPES);
}

void LLSceneMonitor::capture()
{
	static U32 last_capture_time = 0;

	if(last_capture_time == gFrameCount)
	{
		return;
	}
	last_capture_time = gFrameCount;

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

	mCurTarget = NULL;
	mNeedsUpdateDiff = TRUE;
}

bool LLSceneMonitor::needsUpdate() const
{
	return mNeedsUpdateDiff;
}

void LLSceneMonitor::compare()
{
	if(!mNeedsUpdateDiff)
	{
		return;
	}
	mNeedsUpdateDiff = FALSE;

	if(!mFrames[0] || !mFrames[1])
	{
		return;
	}
	if(mFrames[0]->getWidth() != mFrames[1]->getWidth() || mFrames[0]->getHeight() != mFrames[1]->getHeight())
	{
		return; //size does not match
	}

	S32 width = gViewerWindow->getWindowWidthRaw();
	S32 height = gViewerWindow->getWindowHeightRaw();
	if(!mDiff)
	{
		mDiff = new LLRenderTarget();
		mDiff->allocate(width, height, GL_RGBA, false, false, LLTexUnit::TT_TEXTURE, true);

		generateDitheringTexture(width, height);
	}
	else if(mDiff->getWidth() != width || mDiff->getHeight() != height)
	{
		mDiff->resize(width, height, GL_RGBA);
		generateDitheringTexture(width, height);
	}

	mDiff->bindTarget();
	mDiff->clear();
	
	gTwoTextureCompareProgram.bind();
	
	gTwoTextureCompareProgram.uniform1f("dither_scale", mDitherScale);
	gTwoTextureCompareProgram.uniform1f("dither_scale_s", mDitherScaleS);
	gTwoTextureCompareProgram.uniform1f("dither_scale_t", mDitherScaleT);

	gGL.getTexUnit(0)->activate();
	gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
	gGL.getTexUnit(0)->bind(mFrames[0]);
	gGL.getTexUnit(0)->activate();

	gGL.getTexUnit(1)->activate();
	gGL.getTexUnit(1)->enable(LLTexUnit::TT_TEXTURE);
	gGL.getTexUnit(1)->bind(mFrames[1]);
	gGL.getTexUnit(1)->activate();	
	
	gGL.getTexUnit(2)->activate();
	gGL.getTexUnit(2)->enable(LLTexUnit::TT_TEXTURE);
	gGL.getTexUnit(2)->bind(mDitheringTexture);
	gGL.getTexUnit(2)->activate();	

	gl_rect_2d_simple_tex(width, height);
	
	mDiff->flush();	

	gTwoTextureCompareProgram.unbind();

	gGL.getTexUnit(0)->disable();
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.getTexUnit(1)->disable();
	gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.getTexUnit(2)->disable();
	gGL.getTexUnit(2)->unbind(LLTexUnit::TT_TEXTURE);

	mHasNewDiff = TRUE;
	
	//send out the query request.
	queryDiff();
}

void LLSceneMonitor::queryDiff()
{
	if(mDebugViewerVisible)
	{
		return;
	}

	calcDiffAggregate();
}

//calculate Diff aggregate information in GPU, and enable gl occlusion query to capture it.
void LLSceneMonitor::calcDiffAggregate()
{
	if(!mHasNewDiff && !mDebugViewerVisible)
	{
		return;
	}	

	if(!mQueryObject)
	{
		mQueryObject = get_new_occlusion_query_object_name();
	}

	LLGLDepthTest depth(true, false, GL_ALWAYS);
	if(!mDebugViewerVisible)
	{
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	}

	LLGLSLShader* cur_shader = NULL;
	
	cur_shader = LLGLSLShader::sCurBoundShaderPtr;
	gOneTextureFilterProgram.bind();
	gOneTextureFilterProgram.uniform1f("tolerance", mDiffTolerance);

	if(mHasNewDiff)
	{
		glBeginQueryARB(GL_SAMPLES_PASSED_ARB, mQueryObject);
	}

	gl_draw_scaled_target(0, 0, S32(mDiff->getWidth() * mDiffPixelRatio), S32(mDiff->getHeight() * mDiffPixelRatio), mDiff);

	if(mHasNewDiff)
	{
		glEndQueryARB(GL_SAMPLES_PASSED_ARB);
		mHasNewDiff = FALSE;	
		mHasNewQueryResult = TRUE;
	}
		
	gOneTextureFilterProgram.unbind();
	
	if(cur_shader != NULL)
	{
		cur_shader->bind();
	}
	
	if(!mDebugViewerVisible)
	{
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}	
}

static LLTrace::MeasurementStatHandle<> sFramePixelDiff("FramePixelDifference");
void LLSceneMonitor::fetchQueryResult()
{
	if(!mHasNewQueryResult)
	{
		return;
	}
	mHasNewQueryResult = FALSE;

	GLuint available = 0;
	glGetQueryObjectuivARB(mQueryObject, GL_QUERY_RESULT_AVAILABLE_ARB, &available);
	if(!available)
	{
		return;
	}

	GLuint count = 0;
	glGetQueryObjectuivARB(mQueryObject, GL_QUERY_RESULT_ARB, &count);
	
	mDiffResult = count * 0.5f / (mDiff->getWidth() * mDiff->getHeight() * mDiffPixelRatio * mDiffPixelRatio); //0.5 -> (front face + back face)

	if(mDiffResult > 0.01f)
	{
		addMonitorResult();
	}
}

void LLSceneMonitor::addMonitorResult()
{
	mRecording->extend();
	sample(sFramePixelDiff, mDiffResult);

	ll_monitor_result_t result;
	result.mTimeStamp = LLImageGL::sLastFrameTime;
	result.mDiff = mDiffResult;
	mMonitorResults.push_back(result);
}

//dump results to a file _scene_monitor_results.csv
void LLSceneMonitor::dumpToFile(std::string file_name)
{
	if(mMonitorResults.empty())
	{
		return; //nothing to dump
	}

	std::ofstream os(file_name.c_str());

	//total scene loading time
	os << llformat("Scene Loading time: %.4f seconds\n", (F32)getRecording()->getAcceptedRecording().getDuration().value());

	S32 num_results = mMonitorResults.size();
	for(S32 i = 0; i < num_results; i++)
	{
		os << llformat("%.4f %.4f\n", mMonitorResults[i].mTimeStamp, mMonitorResults[i].mDiff);
	}

	os.flush();
	os.close();

	mMonitorResults.clear();
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
	visible = visible && LLGLSLShader::sNoFixedFunction;
	LLSceneMonitor::getInstance()->setDebugViewerVisible(visible);

	LLView::setVisible(visible);
}

void LLSceneMonitorView::draw()
{
	const LLRenderTarget* target = LLSceneMonitor::getInstance()->getDiffTarget();
	if(!target)
	{
		return;
	}

	F32 ratio = LLSceneMonitor::getInstance()->getDiffPixelRatio();
	S32 height = (S32)(target->getHeight() * ratio);
	S32 width = (S32)(target->getWidth() * ratio);
	//S32 height = (S32) (gViewerWindow->getWindowRectScaled().getHeight()*0.5f);
	//S32 width = (S32) (gViewerWindow->getWindowRectScaled().getWidth() * 0.5f);
	
	LLRect new_rect;
	new_rect.setLeftTopAndSize(getRect().mLeft, getRect().mTop, width, height);
	setRect(new_rect);

	//draw background
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gl_rect_2d(0, getRect().getHeight(), getRect().getWidth(), 0, LLColor4(0.f, 0.f, 0.f, 0.25f));
	
	LLSceneMonitor::getInstance()->calcDiffAggregate();

	//show some texts
	LLColor4 color = LLColor4::white;
	S32 line_height = LLFontGL::getFontMonospace()->getLineHeight();

	S32 lines = 0;
	std::string num_str = llformat("Frame difference: %.6f", LLSceneMonitor::getInstance()->getDiffResult());
	LLFontGL::getFontMonospace()->renderUTF8(num_str, 0, 5, getRect().getHeight() - line_height * lines, color, LLFontGL::LEFT, LLFontGL::TOP);
	lines++;

	num_str = llformat("Pixel tolerance: (R+G+B) < %.4f", LLSceneMonitor::getInstance()->getDiffTolerance());
	LLFontGL::getFontMonospace()->renderUTF8(num_str, 0, 5, getRect().getHeight() - line_height * lines, color, LLFontGL::LEFT, LLFontGL::TOP);
	lines++;

	num_str = llformat("Sampling time: %.3f seconds", LLSceneMonitor::getInstance()->getSamplingTime());
	LLFontGL::getFontMonospace()->renderUTF8(num_str, 0, 5, getRect().getHeight() - line_height * lines, color, LLFontGL::LEFT, LLFontGL::TOP);
	lines++;

	num_str = llformat("Scene Loading time: %.3f seconds", (F32)LLSceneMonitor::getInstance()->getRecording()->getAcceptedRecording().getDuration().value());
	LLFontGL::getFontMonospace()->renderUTF8(num_str, 0, 5, getRect().getHeight() - line_height * lines, color, LLFontGL::LEFT, LLFontGL::TOP);
	lines++;

	LLView::draw();
}

