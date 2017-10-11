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
#include "llviewerparcelmgr.h"
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
}

LLSceneMonitor::~LLSceneMonitor()
{
	mDiffState = VIEWER_QUITTING;
	reset();

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

	mMonitorRecording.reset();
	mSceneLoadRecording.reset();
	mRecordingTimer.reset();

	unfreezeScene();

	if(mQueryObject > 0)
	{
		LLOcclusionCullingGroup::releaseOcclusionQueryObjectName(mQueryObject);
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
		cur_target->resize(width, height);
	}

	// we're promising the target exists
	return *cur_target;
}

void LLSceneMonitor::freezeAvatar(LLCharacter* avatarp)
{
	if(mEnabled)
	{
		mAvatarPauseHandles.push_back(avatarp->requestPause());
	}
}

void LLSceneMonitor::freezeScene()
{
	if(!mEnabled)
	{
		return;
	}

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
		return;
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
	static U32 last_capture_frame = 0;
	static LLCachedControl<bool> monitor_enabled(gSavedSettings, "SceneLoadingMonitorEnabled");
	static LLCachedControl<F32>  scene_load_sample_time(gSavedSettings, "SceneLoadingMonitorSampleTime");
	static bool force_capture = true;

	bool enabled = LLGLSLShader::sNoFixedFunction && (monitor_enabled || mDebugViewerVisible);
	if(mEnabled != enabled)
	{
		if(mEnabled)
		{			
			mEnabled = enabled;
			unfreezeScene();
			reset();
			force_capture = true;
		}
		else
		{
			mEnabled = enabled;
			reset();
			freezeScene();
		}
	}

	if (mEnabled 
		&& (mMonitorRecording.getSum(*LLViewerCamera::getVelocityStat()) > 0.1f
			|| mMonitorRecording.getSum(*LLViewerCamera::getAngularVelocityStat()) > 0.05f))
	{
		reset();
		freezeScene();
		force_capture = true;
	}

	if(mEnabled
		&& (mRecordingTimer.getElapsedTimeF32() > scene_load_sample_time() 
			|| force_capture)
		&& last_capture_frame != gFrameCount)
	{
		force_capture = false;

		mSceneLoadRecording.resume();
		mMonitorRecording.resume();

		last_capture_frame = gFrameCount;

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

static LLTrace::BlockTimerStatHandle FTM_GENERATE_SCENE_LOAD_DITHER_TEXTURE("Generate Scene Load Dither Texture");
static LLTrace::BlockTimerStatHandle FTM_SCENE_LOAD_IMAGE_DIFF("Scene Load Image Diff");

static LLStaticHashedString sDitherScale("dither_scale");
static LLStaticHashedString sDitherScaleS("dither_scale_s");
static LLStaticHashedString sDitherScaleT("dither_scale_t");

void LLSceneMonitor::compare()
{
#ifdef LL_WINDOWS
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

	LL_RECORD_BLOCK_TIME(FTM_SCENE_LOAD_IMAGE_DIFF);
	mDiffState = EXECUTE_DIFF;

	S32 width = gViewerWindow->getWindowWidthRaw();
	S32 height = gViewerWindow->getWindowHeightRaw();
	if(!mDiff)
	{
		LL_RECORD_BLOCK_TIME(FTM_GENERATE_SCENE_LOAD_DITHER_TEXTURE);
		mDiff = new LLRenderTarget();
		mDiff->allocate(width, height, GL_RGBA, false, false, LLTexUnit::TT_TEXTURE, true);

		generateDitheringTexture(width, height);
	}
	else if(mDiff->getWidth() != width || mDiff->getHeight() != height)
	{
		LL_RECORD_BLOCK_TIME(FTM_GENERATE_SCENE_LOAD_DITHER_TEXTURE);
		mDiff->resize(width, height);
		generateDitheringTexture(width, height);
	}

	mDiff->bindTarget();
	mDiff->clear();
	
	gTwoTextureCompareProgram.bind();
	
	gTwoTextureCompareProgram.uniform1f(sDitherScale, mDitherScale);
	gTwoTextureCompareProgram.uniform1f(sDitherScaleS, mDitherScaleS);
	gTwoTextureCompareProgram.uniform1f(sDitherScaleT, mDitherScaleT);

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
#endif
}

static LLStaticHashedString sTolerance("tolerance");

//calculate Diff aggregate information in GPU, and enable gl occlusion query to capture it.
void LLSceneMonitor::calcDiffAggregate()
{
#ifdef LL_WINDOWS

	LL_RECORD_BLOCK_TIME(FTM_SCENE_LOAD_IMAGE_DIFF);

	if(mDiffState != EXECUTE_DIFF && !mDebugViewerVisible)
	{
		return;
	}	

	if(!mQueryObject)
	{
		mQueryObject = LLOcclusionCullingGroup::getNewOcclusionQueryObjectName();
	}

	LLGLDepthTest depth(true, false, GL_ALWAYS);
	if(!mDebugViewerVisible)
	{
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	}

	LLGLSLShader* cur_shader = NULL;
	
	cur_shader = LLGLSLShader::sCurBoundShaderPtr;
	gOneTextureFilterProgram.bind();
	gOneTextureFilterProgram.uniform1f(sTolerance, mDiffTolerance);

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
#endif
}

static LLTrace::EventStatHandle<> sFramePixelDiff("FramePixelDifference");
void LLSceneMonitor::fetchQueryResult()
{
	LL_RECORD_BLOCK_TIME(FTM_SCENE_LOAD_IMAGE_DIFF);

	// also throttle timing here, to avoid going below sample time due to phasing with frame capture
	static LLCachedControl<F32>  scene_load_sample_time_control(gSavedSettings, "SceneLoadingMonitorSampleTime");
	F32Seconds scene_load_sample_time = (F32Seconds)scene_load_sample_time_control();

	if(mDiffState == WAIT_ON_RESULT 
		&& !LLAppViewer::instance()->quitRequested())
	{
		mDiffState = WAITING_FOR_NEXT_DIFF;

		GLuint available = 0;
		glGetQueryObjectuivARB(mQueryObject, GL_QUERY_RESULT_AVAILABLE_ARB, &available);
		if(available)
		{
			GLuint count = 0;
			glGetQueryObjectuivARB(mQueryObject, GL_QUERY_RESULT_ARB, &count);
	
			mDiffResult = sqrtf(count * 0.5f / (mDiff->getWidth() * mDiff->getHeight() * mDiffPixelRatio * mDiffPixelRatio)); //0.5 -> (front face + back face)

			LL_DEBUGS("SceneMonitor") << "Frame difference: " << mDiffResult << LL_ENDL;
			record(sFramePixelDiff, mDiffResult);

			static LLCachedControl<F32> diff_threshold(gSavedSettings,"SceneLoadingMonitorPixelDiffThreshold");
			F32Seconds elapsed_time = mRecordingTimer.getElapsedTimeF32();

			if (elapsed_time > scene_load_sample_time)
			{
				if(mDiffResult > diff_threshold())
				{
					mSceneLoadRecording.extend();
					llassert_always(mSceneLoadRecording.getResults().getLastRecording().getDuration() > scene_load_sample_time);
				}
				else
				{
					mSceneLoadRecording.nextPeriod();
				}
				mRecordingTimer.reset();
			}
		}
	}
}

//dump results to a file _scene_xmonitor_results.csv
void LLSceneMonitor::dumpToFile(std::string file_name)
{	using namespace LLTrace;

	if (!hasResults()) return;

	LL_INFOS("SceneMonitor") << "Saving scene load stats to " << file_name << LL_ENDL; 

	llofstream os(file_name.c_str());

	os << std::setprecision(10);

	PeriodicRecording& scene_load_recording = mSceneLoadRecording.getResults();
	const U32 frame_count = scene_load_recording.getNumRecordedPeriods();

	F64Seconds frame_time;

	os << "Stat";
	for (S32 frame = 1; frame <= frame_count; frame++)
	{
		frame_time += scene_load_recording.getPrevRecording(frame_count - frame).getDuration();
		os << ", " << frame_time.value();
	}
	os << '\n';

	os << "Sample period(s)";
	for (S32 frame = 1; frame <= frame_count; frame++)
	{
		frame_time = scene_load_recording.getPrevRecording(frame_count - frame).getDuration();
		os << ", " << frame_time.value();
	}
	os << '\n';


	typedef StatType<CountAccumulator> trace_count;
	for (trace_count::instance_iter it = trace_count::beginInstances(), end_it = trace_count::endInstances();
		it != end_it;
		++it)
	{
		std::ostringstream row;
		row << std::setprecision(10);

		row << it->getName();

		const char* unit_label = it->getUnitLabel();
		if(unit_label[0])
		{
			row << "(" << unit_label << ")";
		}

		S32 samples = 0;

		for (S32 frame = 1; frame <= frame_count; frame++)
		{
			Recording& recording = scene_load_recording.getPrevRecording(frame_count - frame);
			samples += recording.getSampleCount(*it);
			row << ", " << recording.getSum(*it);
		}

		row << '\n';

		if (samples > 0)
		{
			os << row.str();
		}
	}

	typedef StatType<EventAccumulator> trace_event;

	for (trace_event::instance_iter it = trace_event::beginInstances(), end_it = trace_event::endInstances();
		it != end_it;
		++it)
	{
		std::ostringstream row;
		row << std::setprecision(10);
		row << it->getName();

		const char* unit_label = it->getUnitLabel();
		if(unit_label[0])
		{
			row << "(" << unit_label << ")";
		}

		S32 samples = 0;

		for (S32 frame = 1; frame <= frame_count; frame++)
		{
			Recording& recording = scene_load_recording.getPrevRecording(frame_count - frame);
			samples += recording.getSampleCount(*it);
			F64 mean = recording.getMean(*it);
			if (llisnan(mean))
			{
				row << ", n/a";
			}
			else
			{
				row << ", " << mean;
			}
		}

		row << '\n';

		if (samples > 0)
		{
			os << row.str();
		}
	}

	typedef StatType<SampleAccumulator> trace_sample;

	for (trace_sample::instance_iter it = trace_sample::beginInstances(), end_it = trace_sample::endInstances();
		it != end_it;
		++it)
	{
		std::ostringstream row;
		row << std::setprecision(10);
		row << it->getName();

		const char* unit_label = it->getUnitLabel();
		if(unit_label[0])
		{
			row << "(" << unit_label << ")";
		}

		S32 samples = 0;

		for (S32 frame = 1; frame <= frame_count; frame++)
		{
			Recording& recording = scene_load_recording.getPrevRecording(frame_count - frame);
			samples += recording.getSampleCount(*it);
			F64 mean = recording.getMean(*it);
			if (llisnan(mean))
			{
				row << ", n/a";
			}
			else
			{
				row << ", " << mean;
			}
		}

		row << '\n'; 

		if (samples > 0)
		{
			os << row.str();
		}
	}

	typedef StatType<MemAccumulator> trace_mem;
	for (trace_mem::instance_iter it = trace_mem::beginInstances(), end_it = trace_mem::endInstances();
		it != end_it;
		++it)
	{
		os << it->getName() << "(KiB)";

		for (S32 frame = 1; frame <= frame_count; frame++)
		{
			os << ", " << scene_load_recording.getPrevRecording(frame_count - frame).getMax(*it).valueInUnits<LLUnits::Kilobytes>();
		}

		os << '\n';
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

	sTeleportFinishConnection = LLViewerParcelMgr::getInstance()->setTeleportFinishedCallback(boost::bind(&LLSceneMonitorView::onTeleportFinished, this));
}

LLSceneMonitorView::~LLSceneMonitorView()
{
	sTeleportFinishConnection.disconnect();
}

void LLSceneMonitorView::onClose(bool app_quitting)
{
	setVisible(false);	
}

void LLSceneMonitorView::onClickCloseBtn(bool app_quitting)
{
	setVisible(false);
}

void LLSceneMonitorView::onTeleportFinished()
{
	if(isInVisibleChain())
	{
		LLSceneMonitor::getInstance()->reset();
	}
}

void LLSceneMonitorView::onVisibilityChange(BOOL visible)
{
	if (!LLGLSLShader::sNoFixedFunction && visible)
	{
		visible = false;
		// keep Scene monitor and its view in sycn
		setVisible(false);
		LL_WARNS("SceneMonitor") << "Incompatible graphical settings, Scene Monitor can't be turned on" << LL_ENDL; 
	}
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

	num_str = llformat("Scene Loading time: %.3f seconds", (F32)LLSceneMonitor::getInstance()->getRecording()->getResults().getDuration().value());
	LLFontGL::getFontMonospace()->renderUTF8(num_str, 0, 5, getRect().getHeight() - line_height * lines, color, LLFontGL::LEFT, LLFontGL::TOP);
	lines++;

	LLView::draw();
}

