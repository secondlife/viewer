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
#include "llviewerpartsim.h"

LLSceneMonitorView* gSceneMonitorView = NULL;

//
//The procedures of monitoring when the scene finishes loading visually, 
//i.e., no pixel differences among frames, are:
//1, freeze all dynamic objects and avatars;
//2, (?) disable all sky and water;
//3, capture frames periodically, by calling "capture()";
//4, compute pixel differences between two latest captured frames, by calling "compare()", results are stored at mDiff;
//5, compute the number of pixels in mDiff above some tolerance threshold in GPU, by calling "calcDiffAggregate()";
//6, use gl occlusion query to fetch the result from GPU, by calling "fetchQueryResult()";
//END.
//

LLSceneMonitor::LLSceneMonitor() : 
	mEnabled(false), 
	mDiff(NULL),
	mDiffResult(0.f),
	mDiffTolerance(0.1f),
	mDiffState(WAITING_FOR_NEXT_DIFF),
	mDebugViewerVisible(false),
	mQueryObject(0),
	mDiffPixelRatio(0.5f)
{
	mFrames[0] = NULL;
	mFrames[1] = NULL;

	mRecording = new LLTrace::ExtendablePeriodicRecording();
}

LLSceneMonitor::~LLSceneMonitor()
{
	mDiffState = VIEWER_QUITTING;
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

	mRecording->reset();

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

void LLSceneMonitor::setDebugViewerVisible(bool visible) 
{
	mDebugViewerVisible = visible;
}

LLRenderTarget& LLSceneMonitor::getCaptureTarget()
{
	LLRenderTarget* cur_target = NULL;

	S32 width = gViewerWindow->getWorldViewWidthRaw();
	S32 height = gViewerWindow->getWorldViewHeightRaw();
	
	if(!mFrames[0])
	{
		mFrames[0] = new LLRenderTarget();
		mFrames[0]->allocate(width, height, GL_RGB, false, false, LLTexUnit::TT_TEXTURE, true);
		gGL.getTexUnit(0)->bind(mFrames[0]);
		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		cur_target = mFrames[0];
	}
	else if(!mFrames[1])
	{
		mFrames[1] = new LLRenderTarget();
		mFrames[1]->allocate(width, height, GL_RGB, false, false, LLTexUnit::TT_TEXTURE, true);
		gGL.getTexUnit(0)->bind(mFrames[1]);
		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		cur_target = mFrames[1];
	}
	else //swap
	{
		cur_target = mFrames[0];
		mFrames[0] = mFrames[1];
		mFrames[1] = cur_target;
	}
	
	if(cur_target->getWidth() != width || cur_target->getHeight() != height) //size changed
	{
		cur_target->resize(width, height, GL_RGB);
	}

	// we're promising the target exists
	return *cur_target;
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

	//disable sky, water and clouds
	gPipeline.clearRenderTypeMask(LLPipeline::RENDER_TYPE_SKY, LLPipeline::RENDER_TYPE_WL_SKY, 
		LLPipeline::RENDER_TYPE_WATER, LLPipeline::RENDER_TYPE_CLOUDS, LLPipeline::END_RENDER_TYPES);

	//disable particle system
	LLViewerPartSim::getInstance()->enable(false);
}

void LLSceneMonitor::unfreezeScene()
{
	//thaw all avatars
	mAvatarPauseHandles.clear();

	if(mDiffState == VIEWER_QUITTING)
	{
		return; //we are quitting viewer.
	}

	// thaw everything else
	gSavedSettings.setBOOL("FreezeTime", FALSE);

	//enable sky, water and clouds
	gPipeline.setRenderTypeMask(LLPipeline::RENDER_TYPE_SKY, LLPipeline::RENDER_TYPE_WL_SKY, 
		LLPipeline::RENDER_TYPE_WATER, LLPipeline::RENDER_TYPE_CLOUDS, LLPipeline::END_RENDER_TYPES);

	//enable particle system
	LLViewerPartSim::getInstance()->enable(true);
}

void LLSceneMonitor::capture()
{
	static U32 last_capture_time = 0;
	static LLCachedControl<bool> monitor_enabled(gSavedSettings, "SceneLoadingMonitorEnabled");
	static LLCachedControl<F32>  scene_load_sample_time(gSavedSettings, "SceneLoadingMonitorSampleTime");
	static LLFrameTimer timer;	

	LLTrace::Recording& last_frame_recording = LLTrace::get_frame_recording().getLastRecording();
	if (last_frame_recording.getSum(*LLViewerCamera::getVelocityStat()) > 0.001f
		|| last_frame_recording.getSum(*LLViewerCamera::getAngularVelocityStat()) > 0.01f)
	{
		reset();
	}

	bool enabled = monitor_enabled || mDebugViewerVisible;
	if(mEnabled != enabled)
	{
		if(mEnabled)
		{
			unfreezeScene();
		}
		else
		{
			reset();
			freezeScene();
		}

		mEnabled = enabled;
	}

	if(timer.getElapsedTimeF32() > scene_load_sample_time()
		&& mEnabled
		&& LLGLSLShader::sNoFixedFunction
		&& last_capture_time != gFrameCount)
	{
		mRecording->resume();

		timer.reset();

		last_capture_time = gFrameCount;

		LLRenderTarget& cur_target = getCaptureTarget();

		U32 old_FBO = LLRenderTarget::sCurFBO;

		gGL.getTexUnit(0)->bind(&cur_target);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); //point to the main frame buffer.
		
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, cur_target.getWidth(), cur_target.getHeight()); //copy the content
	
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);		
		glBindFramebuffer(GL_FRAMEBUFFER, old_FBO);

		mDiffState = NEED_DIFF;
	}
}

bool LLSceneMonitor::needsUpdate() const
{
	return mDiffState == NEED_DIFF;
}

static LLFastTimer::DeclareTimer FTM_GENERATE_SCENE_LOAD_DITHER_TEXTURE("Generate Scene Load Dither Texture");
static LLFastTimer::DeclareTimer FTM_SCENE_LOAD_IMAGE_DIFF("Scene Load Image Diff");

void LLSceneMonitor::compare()
{
	if(mDiffState != NEED_DIFF)
	{
		return;
	}

	if(!mFrames[0] || !mFrames[1])
	{
		return;
	}
	if(mFrames[0]->getWidth() != mFrames[1]->getWidth() || mFrames[0]->getHeight() != mFrames[1]->getHeight())
	{	//size does not match
		return; 
	}

	LLFastTimer _(FTM_SCENE_LOAD_IMAGE_DIFF);
	mDiffState = EXECUTE_DIFF;

	S32 width = gViewerWindow->getWindowWidthRaw();
	S32 height = gViewerWindow->getWindowHeightRaw();
	if(!mDiff)
	{
		LLFastTimer _(FTM_GENERATE_SCENE_LOAD_DITHER_TEXTURE);
		mDiff = new LLRenderTarget();
		mDiff->allocate(width, height, GL_RGBA, false, false, LLTexUnit::TT_TEXTURE, true);

		generateDitheringTexture(width, height);
	}
	else if(mDiff->getWidth() != width || mDiff->getHeight() != height)
	{
		LLFastTimer _(FTM_GENERATE_SCENE_LOAD_DITHER_TEXTURE);
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

	if (!mDebugViewerVisible)
	{
		calcDiffAggregate();
	}
}

//calculate Diff aggregate information in GPU, and enable gl occlusion query to capture it.
void LLSceneMonitor::calcDiffAggregate()
{
	LLFastTimer _(FTM_SCENE_LOAD_IMAGE_DIFF);

	if(mDiffState != EXECUTE_DIFF && !mDebugViewerVisible)
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

	if(mDiffState == EXECUTE_DIFF)
	{
		glBeginQueryARB(GL_SAMPLES_PASSED_ARB, mQueryObject);
	}

	gl_draw_scaled_target(0, 0, S32(mDiff->getWidth() * mDiffPixelRatio), S32(mDiff->getHeight() * mDiffPixelRatio), mDiff);

	if(mDiffState == EXECUTE_DIFF)
	{
		glEndQueryARB(GL_SAMPLES_PASSED_ARB);
		mDiffState = WAIT_ON_RESULT;
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
	LLFastTimer _(FTM_SCENE_LOAD_IMAGE_DIFF);

	if(mDiffState == WAIT_ON_RESULT)
	{
		mDiffState = WAITING_FOR_NEXT_DIFF;

		GLuint available = 0;
		glGetQueryObjectuivARB(mQueryObject, GL_QUERY_RESULT_AVAILABLE_ARB, &available);
		if(available)
		{
			GLuint count = 0;
			glGetQueryObjectuivARB(mQueryObject, GL_QUERY_RESULT_ARB, &count);
	
			mDiffResult = count * 0.5f / (mDiff->getWidth() * mDiff->getHeight() * mDiffPixelRatio * mDiffPixelRatio); //0.5 -> (front face + back face)

			LL_DEBUGS("SceneMonitor") << "Frame difference: " << std::setprecision(4) << mDiffResult << LL_ENDL;
			sample(sFramePixelDiff, mDiffResult);

			static LLCachedControl<F32> diff_threshold(gSavedSettings,"SceneLoadingPixelDiffThreshold");
			if(mDiffResult > diff_threshold())
			{
				mRecording->extend();
			}

			mRecording->getPotentialRecording().nextPeriod();
		}
	}
}

void LLSceneMonitor::addMonitorResult()
{
	ll_monitor_result_t result;
	result.mTimeStamp = LLImageGL::sLastFrameTime;
	result.mDiff = mDiffResult;
	mMonitorResults.push_back(result);
}

//dump results to a file _scene_xmonitor_results.csv
void LLSceneMonitor::dumpToFile(std::string file_name)
{
	LL_INFOS("SceneMonitor") << "Saving scene load stats to " << file_name << LL_ENDL; 

	std::ofstream os(file_name.c_str());

	//total scene loading time
	os << std::setprecision(4);

	LLTrace::PeriodicRecording& scene_load_recording = mRecording->getAcceptedRecording();
	U32 frame_count = scene_load_recording.getNumPeriods();

	LLUnit<LLUnits::Seconds, F64> frame_time;

	os << "Stat";
	for (S32 frame = 0; frame < frame_count; frame++)
	{
		frame_time += scene_load_recording.getPrevRecording(frame_count - frame).getDuration();
		os << ", " << frame_time.value();
	}
	os << std::endl;

	for (LLTrace::CountStatHandle<F64>::instance_iter it = LLTrace::CountStatHandle<F64>::beginInstances(), end_it = LLTrace::CountStatHandle<F64>::endInstances();
		it != end_it;
		++it)
	{
		std::ostringstream row;
		row << it->getName();

		S32 samples = 0;

		for (S32 i = frame_count - 1; i >= 0; --i)
		{
			samples += scene_load_recording.getPrevRecording(i).getSampleCount(*it);
			row << ", " << scene_load_recording.getPrevRecording(i).getSum(*it);
		}

		row << std::endl;

		if (samples > 0)
		{
			os << row.str();
		}
	}

	for (LLTrace::CountStatHandle<S64>::instance_iter it = LLTrace::CountStatHandle<S64>::beginInstances(), end_it = LLTrace::CountStatHandle<S64>::endInstances();
		it != end_it;
		++it)
	{
		std::ostringstream row;
		row << it->getName();

		S32 samples = 0;

		for (S32 i = frame_count - 1; i >= 0; --i)
		{
			samples += scene_load_recording.getPrevRecording(i).getSampleCount(*it);
			row << ", " << scene_load_recording.getPrevRecording(i).getSum(*it);
		}

		row << std::endl;

		if (samples > 0)
		{
			os << row.str();
		}
	}

	for (LLTrace::MeasurementStatHandle<F64>::instance_iter it = LLTrace::MeasurementStatHandle<F64>::beginInstances(), end_it = LLTrace::MeasurementStatHandle<F64>::endInstances();
		it != end_it;
		++it)
	{
		std::ostringstream row;
		row << it->getName();

		S32 samples = 0;

		for (S32 i = frame_count - 1; i >= 0; --i)
		{
			samples += scene_load_recording.getPrevRecording(i).getSampleCount(*it);
			row << ", " << scene_load_recording.getPrevRecording(i).getMean(*it);
		}

		row << std::endl;

		if (samples > 0)
		{
			os << row.str();
		}
	}

	for (LLTrace::MeasurementStatHandle<S64>::instance_iter it = LLTrace::MeasurementStatHandle<S64>::beginInstances(), end_it = LLTrace::MeasurementStatHandle<S64>::endInstances();
		it != end_it;
		++it)
	{
		std::ostringstream row;
		row << it->getName();

		S32 samples = 0;

		for (S32 i = frame_count - 1; i >= 0; --i)
		{
			samples += scene_load_recording.getPrevRecording(i).getSampleCount(*it);
			row << ", " << scene_load_recording.getPrevRecording(i).getMean(*it);
		}

		row << std::endl;

		if (samples > 0)
		{
			os << row.str();
		}
	}

	os.flush();
	os.close();

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

void LLSceneMonitorView::onVisibilityChange(BOOL visible)
{
	visible = visible && LLGLSLShader::sNoFixedFunction;
	LLSceneMonitor::getInstance()->setDebugViewerVisible(visible);
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

	num_str = llformat("Sampling time: %.3f seconds", gSavedSettings.getF32("SceneLoadingMonitorSampleTime"));
	LLFontGL::getFontMonospace()->renderUTF8(num_str, 0, 5, getRect().getHeight() - line_height * lines, color, LLFontGL::LEFT, LLFontGL::TOP);
	lines++;

	num_str = llformat("Scene Loading time: %.3f seconds", (F32)LLSceneMonitor::getInstance()->getRecording()->getAcceptedRecording().getDuration().value());
	LLFontGL::getFontMonospace()->renderUTF8(num_str, 0, 5, getRect().getHeight() - line_height * lines, color, LLFontGL::LEFT, LLFontGL::TOP);
	lines++;

	LLView::draw();
}

