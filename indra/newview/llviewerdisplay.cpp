/** 
 * @file llviewerdisplay.cpp
 * @brief LLViewerDisplay class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llviewerdisplay.h"

#include "llgl.h"
#include "llglimmediate.h"
#include "llglheaders.h"
#include "llagent.h"
#include "llviewercontrol.h"
#include "llcoord.h"
#include "llcriticaldamp.h"
#include "lldir.h"
#include "lldynamictexture.h"
#include "lldrawpoolalpha.h"
#include "llfeaturemanager.h"
#include "llframestats.h"
#include "llhudmanager.h"
#include "llimagebmp.h"
#include "llimagegl.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "llstartup.h"
#include "lltoolfocus.h"
#include "lltoolmgr.h"
#include "lltooldraganddrop.h"
#include "lltoolpie.h"
#include "lltracker.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llvograss.h"
#include "llworld.h"
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llappviewer.h"
#include "llstartup.h"
#include "llfasttimer.h"
#include "llfloatertools.h"
#include "llviewerimagelist.h"
#include "llfocusmgr.h"
#include "llcubemap.h"
#include "llviewerregion.h"
#include "lldrawpoolwater.h"
#include "llwlparammanager.h"
#include "llwaterparammanager.h"
#include "llpostprocess.h"

extern LLPointer<LLImageGL> gStartImageGL;
extern BOOL gDisplaySwapBuffers;

LLPointer<LLImageGL> gDisconnectedImagep = NULL;

// used to toggle renderer back on after teleport
const F32 TELEPORT_RENDER_DELAY = 20.f; // Max time a teleport is allowed to take before we raise the curtain
const F32 TELEPORT_ARRIVAL_DELAY = 2.f; // Time to preload the world before raising the curtain after we've actually already arrived.
BOOL		 gTeleportDisplay = FALSE;
LLFrameTimer gTeleportDisplayTimer;
LLFrameTimer gTeleportArrivalTimer;
const F32		RESTORE_GL_TIME = 5.f;	// Wait this long while reloading textures before we raise the curtain

BOOL gForceRenderLandFence = FALSE;
BOOL gDisplaySwapBuffers = FALSE;
BOOL gResizeScreenTexture = FALSE;
BOOL gSnapshot = FALSE;

// Rendering stuff
void pre_show_depth_buffer();
void post_show_depth_buffer();
void render_ui_and_swap();
void render_ui_and_swap_if_needed();
void render_hud_attachments();
void render_ui_3d();
void render_ui_2d();
void render_disconnected_background();
void render_hud_elements();
void process_keystrokes_async();

void display_startup()
{
	if (   !gViewerWindow->getActive()
		|| !gViewerWindow->mWindow->getVisible() 
		|| gViewerWindow->mWindow->getMinimized()
		|| gNoRender )
	{
		return; 
	}

	LLGLSDefault gls_default;

	// Required for HTML update in login screen
	static S32 frame_count = 0;
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
#endif

	if (frame_count++ > 1) // make sure we have rendered a frame first
	{
		LLDynamicTexture::updateAllInstances();
	}

#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
#endif

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	LLGLSUIDefault gls_ui;
	gPipeline.disableLights();

	gViewerWindow->setup2DRender();
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	gGL.start();
	gViewerWindow->draw();
	gGL.stop();

#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
#endif

	gViewerWindow->mWindow->swapBuffers();
	glClear(GL_DEPTH_BUFFER_BIT);
}


void display_update_camera()
{
	// TODO: cut draw distance down if customizing avatar?
	// TODO: cut draw distance on per-parcel basis?

	// Cut draw distance in half when customizing avatar,
	// but on the viewer only.
	F32 final_far = gAgent.mDrawDistance;
	if (CAMERA_MODE_CUSTOMIZE_AVATAR == gAgent.getCameraMode())
	{
		final_far *= 0.5f;
	}
	gCamera->setFar(final_far);
	gViewerWindow->setup3DRender();
	
	// update all the sky/atmospheric/water settings
	LLWLParamManager::instance()->update(gCamera);
	LLWaterParamManager::instance()->update(gCamera);

	// Update land visibility too
	if (gWorldp)
	{
		gWorldp->setLandFarClip(final_far);
	}
}


// Paint the display!
void display(BOOL rebuild, F32 zoom_factor, int subfield, BOOL for_snapshot)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER);

	if (LLPipeline::sRenderFrameTest)
	{
		send_agent_pause();
	}

	gSnapshot = for_snapshot;

	LLGLSDefault gls_default;
	LLGLDepthTest gls_depth(GL_TRUE, GL_TRUE, GL_LEQUAL);

	// No clue where this is getting unset, but safe enough to reset it here.
	//this causes frame stalls, try real hard not to uncomment this line - DaveP
	//LLGLState::resetTextureStates();
	
	
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
#endif
	
	gPipeline.disableLights();
	
	// Don't draw if the window is hidden or minimized.
	// In fact, must explicitly check the minimized state before drawing.
	// Attempting to draw into a minimized window causes a GL error. JC
	if (   !gViewerWindow->getActive()
		|| !gViewerWindow->mWindow->getVisible() 
		|| gViewerWindow->mWindow->getMinimized() )
	{
		// Clean up memory the pools may have allocated
		if (rebuild)
		{
			gFrameStats.start(LLFrameStats::REBUILD);
			gPipeline.rebuildPools();
		}
		return; 
	}

	gViewerWindow->checkSettings();
	
	gViewerWindow->performPick();
	

#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
#endif
	
	//////////////////////////////////////////////////////////
	//
	// Logic for forcing window updates if we're in drone mode.
	//

	if (gNoRender) 
	{
#if LL_WINDOWS
		static F32 last_update_time = 0.f;
		if ((gFrameTimeSeconds - last_update_time) > 1.f)
		{
			InvalidateRect((HWND)gViewerWindow->getPlatformWindow(), NULL, FALSE);
			last_update_time = gFrameTimeSeconds;
		}
#elif LL_DARWIN
		// MBW -- Do something clever here.
#endif
		// Not actually rendering, don't bother.
		return;
	}


	//
	// Bail out if we're in the startup state and don't want to try to
	// render the world.
	//
	if (LLStartUp::getStartupState() < STATE_STARTED)
	{
		display_startup();
		return;
	}

	//LLGLState::verify(FALSE);

	/////////////////////////////////////////////////
	//
	// Update GL Texture statistics (used for discard logic?)
	//

	gFrameStats.start(LLFrameStats::UPDATE_TEX_STATS);
	stop_glerror();

	LLImageGL::updateStats(gFrameTimeSeconds);
	
	LLVOAvatar::sRenderName = gSavedSettings.getS32("RenderName");
	LLVOAvatar::sRenderGroupTitles = gSavedSettings.getBOOL("RenderGroupTitleAll");
	gPipeline.mBackfaceCull = TRUE;
	gFrameCount++;
	if (gFocusMgr.getAppHasFocus())
	{
		gForegroundFrameCount++;
	}

	//////////////////////////////////////////////////////////
	//
	// Display start screen if we're teleporting, and skip render
	//

	if (gTeleportDisplay)
	{
		const F32 TELEPORT_ARRIVAL_DELAY = 2.f; // Time to preload the world before raising the curtain after we've actually already arrived.

		S32 attach_count = 0;
		if (gAgent.getAvatarObject())
		{
			attach_count = gAgent.getAvatarObject()->getAttachmentCount();
		}
		F32 teleport_save_time = TELEPORT_EXPIRY + TELEPORT_EXPIRY_PER_ATTACHMENT * attach_count;
		F32 teleport_elapsed = gTeleportDisplayTimer.getElapsedTimeF32();
		F32 teleport_percent = teleport_elapsed * (100.f / teleport_save_time);
		if( (gAgent.getTeleportState() != LLAgent::TELEPORT_START) && (teleport_percent > 100.f) )
		{
			// Give up.  Don't keep the UI locked forever.
			gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
			gAgent.setTeleportMessage("");
		}

		const LLString& message = gAgent.getTeleportMessage();
		switch( gAgent.getTeleportState() )
		{
		case LLAgent::TELEPORT_START:
			// Transition to REQUESTED.  Viewer has sent some kind
			// of TeleportRequest to the source simulator
			gTeleportDisplayTimer.reset();
			gViewerWindow->setShowProgress(TRUE);
			gViewerWindow->setProgressPercent(0);
			gAgent.setTeleportState( LLAgent::TELEPORT_REQUESTED );
			gAgent.setTeleportMessage(
				LLAgent::sTeleportProgressMessages["requesting"]);
			break;

		case LLAgent::TELEPORT_REQUESTED:
			// Waiting for source simulator to respond
			gViewerWindow->setProgressPercent( llmin(teleport_percent, 37.5f) );
			gViewerWindow->setProgressString(message);
			break;

		case LLAgent::TELEPORT_MOVING:
			// Viewer has received destination location from source simulator
			gViewerWindow->setProgressPercent( llmin(teleport_percent, 75.f) );
			gViewerWindow->setProgressString(message);
			break;

		case LLAgent::TELEPORT_START_ARRIVAL:
			// Transition to ARRIVING.  Viewer has received avatar update, etc., from destination simulator
			gTeleportArrivalTimer.reset();
			gViewerWindow->setProgressCancelButtonVisible(FALSE, "Cancel");
			gViewerWindow->setProgressPercent(75.f);
			gAgent.setTeleportState( LLAgent::TELEPORT_ARRIVING );
			gAgent.setTeleportMessage(
				LLAgent::sTeleportProgressMessages["arriving"]);
			gImageList.mForceResetTextureStats = TRUE;
			break;

		case LLAgent::TELEPORT_ARRIVING:
			// Make the user wait while content "pre-caches"
			{
				F32 arrival_fraction = (gTeleportArrivalTimer.getElapsedTimeF32() / TELEPORT_ARRIVAL_DELAY);
				if( arrival_fraction > 1.f )
				{
					arrival_fraction = 1.f;
					gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
				}
				gViewerWindow->setProgressCancelButtonVisible(FALSE, "Cancel");
				gViewerWindow->setProgressPercent(  arrival_fraction * 25.f + 75.f);
				gViewerWindow->setProgressString(message);
			}
			break;

		case LLAgent::TELEPORT_NONE:
			// No teleport in progress
			gViewerWindow->setShowProgress(FALSE);
			gTeleportDisplay = FALSE;
			break;
		}
	}
    else if(LLAppViewer::instance()->logoutRequestSent())
	{
		F32 percent_done = gLogoutTimer.getElapsedTimeF32() * 100.f / gLogoutMaxTime;
		if (percent_done > 100.f)
		{
			percent_done = 100.f;
		}

		if( LLApp::isExiting() )
		{
			percent_done = 100.f;
		}
		
		gViewerWindow->setProgressPercent( percent_done );
	}
	else
	if (gRestoreGL)
	{
		F32 percent_done = gRestoreGLTimer.getElapsedTimeF32() * 100.f / RESTORE_GL_TIME;
		if( percent_done > 100.f )
		{
			gViewerWindow->setShowProgress(FALSE);
			gRestoreGL = FALSE;
		}
		else
		{

			if( LLApp::isExiting() )
			{
				percent_done = 100.f;
			}
			
			gViewerWindow->setProgressPercent( percent_done );
		}
	}

	//////////////////////////
	//
	// Prepare for the next frame
	//

	/////////////////////////////
	//
	// Update the camera
	//
	//

	gCamera->setZoomParameters(zoom_factor, subfield);
	gCamera->setNear(MIN_NEAR_PLANE);

	//////////////////////////
	//
	// clear the next buffer
	// (must follow dynamic texture writing since that uses the frame buffer)
	//

	if (gDisconnected)
	{
		render_ui_and_swap_if_needed();
		gDisplaySwapBuffers = TRUE;
		
		render_disconnected_background();
	}
	
	//////////////////////////
	//
	// Set rendering options
	//
	//
	stop_glerror();
	if (gSavedSettings.getBOOL("ShowDepthBuffer"))
	{
		pre_show_depth_buffer();
	}

	stop_glerror();

	///////////////////////////////////////
	//
	// Slam lighting parameters back to our defaults.
	// Note that these are not the same as GL defaults...

	stop_glerror();
	F32 one[4] =	{1.f, 1.f, 1.f, 1.f};
	glLightModelfv (GL_LIGHT_MODEL_AMBIENT,one);
	stop_glerror();
		
	/////////////////////////////////////
	//
	// Render
	//
	// Actually push all of our triangles to the screen.
	//

	// do render-to-texture stuff here
	if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_DYNAMIC_TEXTURES))
	{
		LLFastTimer t(LLFastTimer::FTM_UPDATE_TEXTURES);
		if (LLDynamicTexture::updateAllInstances())
		{
			glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
	}

	gViewerWindow->setupViewport();
	
	if (!gDisconnected)
	{
		if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
		{ //don't draw hud objects in this frame
			gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
		}
		
		//upkeep gl name pools
		LLGLNamePool::upkeepPools();
		
		stop_glerror();
		display_update_camera();
		stop_glerror();
				
		// *TODO: merge these two methods
		gHUDManager->updateEffects();
		LLHUDObject::updateAll();
		stop_glerror();
		
		gFrameStats.start(LLFrameStats::UPDATE_GEOM);
		const F32 max_geom_update_time = 0.005f*10.f*gFrameIntervalSeconds; // 50 ms/second update time
		gPipeline.updateGeom(max_geom_update_time);
		stop_glerror();
		
		gFrameStats.start(LLFrameStats::UPDATE_CULL);
		S32 water_clip = 0;
		if (LLShaderMgr::getVertexShaderLevel(LLShaderMgr::SHADER_ENVIRONMENT) > 1)
		{
			if (gCamera->cameraUnderWater())
			{
				water_clip = -1;
			}
			else
			{
				water_clip = 1;
			}
		}

		//Increment drawable frame counter
		LLDrawable::incrementVisible();

		LLPipeline::sUseOcclusion = 
				(!gUseWireframe
				&& gFeatureManagerp->isFeatureAvailable("UseOcclusion") 
				&& gSavedSettings.getBOOL("UseOcclusion") 
				&& gGLManager.mHasOcclusionQuery) ? 2 : 0;
		LLPipeline::sFastAlpha = gSavedSettings.getBOOL("RenderFastAlpha");
		LLPipeline::sUseFarClip = gSavedSettings.getBOOL("RenderUseFarClip");
		LLVOAvatar::sMaxVisible = gSavedSettings.getS32("RenderAvatarMaxVisible");
		
		S32 occlusion = LLPipeline::sUseOcclusion;
		if (!gDisplaySwapBuffers)
		{ //depth buffer is invalid, don't overwrite occlusion state
			LLPipeline::sUseOcclusion = llmin(occlusion, 1);
		}

		static LLCullResult result;
		gPipeline.updateCull(*gCamera, result, water_clip);
		stop_glerror();

		BOOL to_texture = !for_snapshot &&
						gPipeline.canUseVertexShaders() &&
						LLPipeline::sRenderGlow &&
						gGLManager.mHasFramebufferObject;

		// now do the swap buffer (just before rendering to framebuffer)
		{ //swap and flush state from previous frame
			{
 				LLFastTimer ftm(LLFastTimer::FTM_CLIENT_COPY);
				LLVertexBuffer::clientCopy(0.016);
			}

			if (gResizeScreenTexture)
			{
				gResizeScreenTexture = FALSE;
				gPipeline.resizeScreenTexture();
			}

			glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
			glClearColor(0,0,0,0);
			
			if (!for_snapshot)
			{
				render_ui_and_swap_if_needed();
				gDisplaySwapBuffers = TRUE;
				
				glh::matrix4f proj = glh_get_current_projection();
				glh::matrix4f mod = glh_get_current_modelview();
				glViewport(0,0,128,256);
				LLVOAvatar::updateImpostors();
				glh_set_current_projection(proj);
				glh_set_current_modelview(mod);
				glMatrixMode(GL_PROJECTION);
				glLoadMatrixf(proj.m);
				glMatrixMode(GL_MODELVIEW);
				glLoadMatrixf(mod.m);
				gViewerWindow->setupViewport();
			}
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}

		if (!for_snapshot)
		{
			gPipeline.processImagery(*gCamera);
			gPipeline.generateWaterReflection(*gCamera);
		}

		///////////////////////////////////
		//
		// StateSort
		//
		// Responsible for taking visible objects, and adding them to the appropriate draw orders.
		// In the case of alpha objects, z-sorts them first.
		// Also creates special lists for outlines and selected face rendering.
		//
		{
			gFrameStats.start(LLFrameStats::STATE_SORT);
			gPipeline.stateSort(*gCamera, result);
			stop_glerror();
				
			if (rebuild)
			{
				//////////////////////////////////////
				//
				// rebuildPools
				//
				//
				gFrameStats.start(LLFrameStats::REBUILD);
				gPipeline.rebuildPools();
				stop_glerror();
			}
		}

		LLPipeline::sUseOcclusion = occlusion;

		{
			LLFastTimer t(LLFastTimer::FTM_UPDATE_SKY);	
			gSky.updateSky();
		}

		if(gUseWireframe)
		{
			glClearColor(0.5f, 0.5f, 0.5f, 0.f);
			glClear(GL_COLOR_BUFFER_BIT);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		//// render frontmost floater opaque for occlusion culling purposes
		//LLFloater* frontmost_floaterp = gFloaterView->getFrontmost();
		//// assumes frontmost floater with focus is opaque
		//if (frontmost_floaterp && gFocusMgr.childHasKeyboardFocus(frontmost_floaterp))
		//{
		//	glMatrixMode(GL_MODELVIEW);
		//	glPushMatrix();
		//	{
		//		LLGLSNoTexture gls_no_texture;

		//		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
		//		glLoadIdentity();

		//		LLRect floater_rect = frontmost_floaterp->getScreenRect();
		//		// deflate by one pixel so rounding errors don't occlude outside of floater extents
		//		floater_rect.stretch(-1);
		//		LLRectf floater_3d_rect((F32)floater_rect.mLeft / (F32)gViewerWindow->getWindowWidth(), 
		//								(F32)floater_rect.mTop / (F32)gViewerWindow->getWindowHeight(),
		//								(F32)floater_rect.mRight / (F32)gViewerWindow->getWindowWidth(),
		//								(F32)floater_rect.mBottom / (F32)gViewerWindow->getWindowHeight());
		//		floater_3d_rect.translate(-0.5f, -0.5f);
		//		glTranslatef(0.f, 0.f, -gCamera->getNear());
		//		glScalef(gCamera->getNear() * gCamera->getAspect() / sinf(gCamera->getView()), gCamera->getNear() / sinf(gCamera->getView()), 1.f);
		//		gGL.color4fv(LLColor4::white.mV);
		//		gGL.begin(GL_QUADS);
		//		{
		//			gGL.vertex3f(floater_3d_rect.mLeft, floater_3d_rect.mBottom, 0.f);
		//			gGL.vertex3f(floater_3d_rect.mLeft, floater_3d_rect.mTop, 0.f);
		//			gGL.vertex3f(floater_3d_rect.mRight, floater_3d_rect.mTop, 0.f);
		//			gGL.vertex3f(floater_3d_rect.mRight, floater_3d_rect.mBottom, 0.f);
		//		}
		//		gGL.end();
		//		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		//	}
		//	glPopMatrix();
		//}

		if (to_texture)
		{
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			gPipeline.mScreen.bindTarget();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
		}

		if (!(LLAppViewer::instance()->logoutRequestSent() && LLAppViewer::instance()->hasSavedFinalSnapshot())
				&& !gRestoreGL)
		{
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
			LLPipeline::sUnderWaterRender = gCamera->cameraUnderWater() ? TRUE : FALSE;
			gPipeline.renderGeom(*gCamera, TRUE);
			LLPipeline::sUnderWaterRender = FALSE;
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

			//store this frame's modelview matrix for use
			//when rendering next frame's occlusion queries
			for (U32 i = 0; i < 16; i++)
			{
				gGLLastModelView[i] = gGLModelView[i];
			}
			stop_glerror();
		}
	
		render_hud_attachments();
		
		if (to_texture)
		{
			gPipeline.mScreen.flush();
		}

		/// We copy the frame buffer straight into a texture here,
		/// and then display it again with compositor effects.
		/// Using render to texture would be faster/better, but I don't have a 
		/// grasp of their full display stack just yet.
		// gPostProcess->apply(gViewerWindow->getWindowDisplayWidth(), gViewerWindow->getWindowDisplayHeight());
	}
	gFrameStats.start(LLFrameStats::RENDER_UI);

	if (gHandleKeysAsync)
	{
		process_keystrokes_async();
		stop_glerror();
	}

	gFrameStats.start(LLFrameStats::MISC_END);
	stop_glerror();

	if (LLPipeline::sRenderFrameTest)
	{
		send_agent_resume();
		LLPipeline::sRenderFrameTest = FALSE;
	}
}

void render_hud_attachments()
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
		
	glh::matrix4f current_proj = glh_get_current_projection();
	glh::matrix4f current_mod = glh_get_current_modelview();

	if (LLPipeline::sShowHUDAttachments && !gDisconnected && setup_hud_matrices(FALSE))
	{
		LLCamera hud_cam = *gCamera;
		LLVector3 origin = hud_cam.getOrigin();
		hud_cam.setOrigin(-1.f,0,0);
		hud_cam.setAxes(LLVector3(1,0,0), LLVector3(0,1,0), LLVector3(0,0,1));
		LLViewerCamera::updateFrustumPlanes(hud_cam, TRUE);
		
		//only render hud objects
		U32 mask = gPipeline.getRenderTypeMask();
		gPipeline.setRenderTypeMask(0);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);

		BOOL has_ui = gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI);
		if (has_ui)
		{
			gPipeline.toggleRenderDebugFeature((void*) LLPipeline::RENDER_DEBUG_FEATURE_UI);
		}

		S32 use_occlusion = LLPipeline::sUseOcclusion;
		LLPipeline::sUseOcclusion = 0;
		LLPipeline::sDisableShaders = TRUE;

		//cull, sort, and render hud objects
		static LLCullResult result;
		gPipeline.updateCull(hud_cam, result);

		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_BUMP);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_SIMPLE);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_VOLUME);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_GLOW);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_ALPHA);
		
		gPipeline.stateSort(hud_cam, result);

		gPipeline.renderGeom(hud_cam);

		//restore type mask
		gPipeline.setRenderTypeMask(mask);
		if (has_ui)
		{
			gPipeline.toggleRenderDebugFeature((void*) LLPipeline::RENDER_DEBUG_FEATURE_UI);
		}
		LLPipeline::sUseOcclusion = use_occlusion;
		LLPipeline::sDisableShaders = FALSE;
	}
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
	glh_set_current_projection(current_proj);
	glh_set_current_modelview(current_mod);
}

BOOL setup_hud_matrices(BOOL for_select)
{
	LLVOAvatar* my_avatarp = gAgent.getAvatarObject();
	if (my_avatarp && my_avatarp->hasHUDAttachment())
	{
		if (!for_select)
		{
			// clamp target zoom level to reasonable values
			my_avatarp->mHUDTargetZoom = llclamp(my_avatarp->mHUDTargetZoom, 0.1f, 1.f);
			// smoothly interpolate current zoom level
			my_avatarp->mHUDCurZoom = lerp(my_avatarp->mHUDCurZoom, my_avatarp->mHUDTargetZoom, LLCriticalDamp::getInterpolant(0.03f));
		}

		F32 zoom_level = my_avatarp->mHUDCurZoom;
		// clear z buffer and set up transform for hud
		if (!for_select)
		{
			//glClear(GL_DEPTH_BUFFER_BIT);
		}
		LLBBox hud_bbox = my_avatarp->getHUDBBox();

		
		// set up transform to encompass bounding box of HUD
		glMatrixMode(GL_PROJECTION);
		F32 hud_depth = llmax(1.f, hud_bbox.getExtentLocal().mV[VX] * 1.1f);
		if (for_select)
		{
			//RN: reset viewport to window extents so ortho screen is calculated with proper reference frame
			gViewerWindow->setupViewport();
		}
		glh::matrix4f proj = gl_ortho(-0.5f * gCamera->getAspect(), 0.5f * gCamera->getAspect(), -0.5f, 0.5f, 0.f, hud_depth);
		proj.element(2,2) = -0.01f;
		
		// apply camera zoom transform (for high res screenshots)
		F32 zoom_factor = gCamera->getZoomFactor();
		S16 sub_region = gCamera->getZoomSubRegion();
		if (zoom_factor > 1.f)
		{
			float offset = zoom_factor - 1.f;
			int pos_y = sub_region / llceil(zoom_factor);
			int pos_x = sub_region - (pos_y*llceil(zoom_factor));
			glh::matrix4f mat;
			mat.set_scale(glh::vec3f(zoom_factor, zoom_factor, 1.f));
			mat.set_translate(glh::vec3f(gCamera->getAspect() * 0.5f * (offset - (F32)pos_x * 2.f), 0.5f * (offset - (F32)pos_y * 2.f), 0.f));
			proj *= mat;
		}

		glLoadMatrixf(proj.m);
		glh_set_current_projection(proj);

		glMatrixMode(GL_MODELVIEW);
		glh::matrix4f model((GLfloat*) OGL_TO_CFR_ROTATION);
		
		glh::matrix4f mat;
		mat.set_translate(glh::vec3f(-hud_bbox.getCenterLocal().mV[VX] + (hud_depth * 0.5f), 0.f, 0.f));
		mat.set_scale(glh::vec3f(zoom_level, zoom_level, zoom_level));

		model *= mat;
		glLoadMatrixf(model.m);
		glh_set_current_modelview(model);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


void render_ui_and_swap()
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkStates();
#endif
	
	{
		BOOL to_texture = gPipeline.canUseVertexShaders() &&
							LLPipeline::sRenderGlow &&
							gGLManager.mHasFramebufferObject;

		if (to_texture)
		{
			gPipeline.renderBloom(gSnapshot);
		}
	}

	LLGLSDefault gls_default;
	LLGLSUIDefault gls_ui;
	{
		gPipeline.disableLights();
	}

	{
		LLVertexBuffer::startRender();
		gGL.start();
		if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
		{
			LLFastTimer t(LLFastTimer::FTM_RENDER_UI);

			if (!gDisconnected)
			{
				render_ui_3d();
#ifndef LL_RELEASE_FOR_DOWNLOAD
				LLGLState::checkStates();
#endif
			}

			render_ui_2d();
#ifndef LL_RELEASE_FOR_DOWNLOAD
			LLGLState::checkStates();
#endif
		}
		gGL.stop();

		{
			gViewerWindow->setup2DRender();
			gViewerWindow->updateDebugText();
			gViewerWindow->drawDebugText();
		}

		LLVertexBuffer::stopRender();
	}
}

void render_ui_and_swap_if_needed()
{
	if (gDisplaySwapBuffers)
	{
		render_ui_and_swap();
		
		{
			LLFastTimer t(LLFastTimer::FTM_SWAP);
			gViewerWindow->mWindow->swapBuffers();
		}
	}
}

void renderCoordinateAxes()
{
	LLGLSNoTexture gls_no_texture;
	gGL.begin(GL_LINES);
		gGL.color3f(1.0f, 0.0f, 0.0f);   // i direction = X-Axis = red
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(2.0f, 0.0f, 0.0f);
		gGL.vertex3f(3.0f, 0.0f, 0.0f);
		gGL.vertex3f(5.0f, 0.0f, 0.0f);
		gGL.vertex3f(6.0f, 0.0f, 0.0f);
		gGL.vertex3f(8.0f, 0.0f, 0.0f);
		// Make an X
		gGL.vertex3f(11.0f, 1.0f, 1.0f);
		gGL.vertex3f(11.0f, -1.0f, -1.0f);
		gGL.vertex3f(11.0f, 1.0f, -1.0f);
		gGL.vertex3f(11.0f, -1.0f, 1.0f);

		gGL.color3f(0.0f, 1.0f, 0.0f);   // j direction = Y-Axis = green
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(0.0f, 2.0f, 0.0f);
		gGL.vertex3f(0.0f, 3.0f, 0.0f);
		gGL.vertex3f(0.0f, 5.0f, 0.0f);
		gGL.vertex3f(0.0f, 6.0f, 0.0f);
		gGL.vertex3f(0.0f, 8.0f, 0.0f);
		// Make a Y
		gGL.vertex3f(1.0f, 11.0f, 1.0f);
		gGL.vertex3f(0.0f, 11.0f, 0.0f);
		gGL.vertex3f(-1.0f, 11.0f, 1.0f);
		gGL.vertex3f(0.0f, 11.0f, 0.0f);
		gGL.vertex3f(0.0f, 11.0f, 0.0f);
		gGL.vertex3f(0.0f, 11.0f, -1.0f);

		gGL.color3f(0.0f, 0.0f, 1.0f);   // Z-Axis = blue
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(0.0f, 0.0f, 2.0f);
		gGL.vertex3f(0.0f, 0.0f, 3.0f);
		gGL.vertex3f(0.0f, 0.0f, 5.0f);
		gGL.vertex3f(0.0f, 0.0f, 6.0f);
		gGL.vertex3f(0.0f, 0.0f, 8.0f);
		// Make a Z
		gGL.vertex3f(-1.0f, 1.0f, 11.0f);
		gGL.vertex3f(1.0f, 1.0f, 11.0f);
		gGL.vertex3f(1.0f, 1.0f, 11.0f);
		gGL.vertex3f(-1.0f, -1.0f, 11.0f);
		gGL.vertex3f(-1.0f, -1.0f, 11.0f);
		gGL.vertex3f(1.0f, -1.0f, 11.0f);
	gGL.end();
}


void draw_axes() 
{
	LLGLSUIDefault gls_ui;
	LLGLSNoTexture gls_no_texture;
	// A vertical white line at origin
	LLVector3 v = gAgent.getPositionAgent();
	gGL.begin(GL_LINES);
		gGL.color3f(1.0f, 1.0f, 1.0f); 
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(0.0f, 0.0f, 40.0f);
	gGL.end();
	// Some coordinate axes
	glPushMatrix();
		glTranslatef( v.mV[VX], v.mV[VY], v.mV[VZ] );
		renderCoordinateAxes();
	glPopMatrix();
}

void render_ui_3d()
{
	LLGLSPipeline gls_pipeline;

	//////////////////////////////////////
	//
	// Render 3D UI elements
	// NOTE: zbuffer is cleared before we get here by LLDrawPoolHUD,
	//		 so 3d elements requiring Z buffer are moved to LLDrawPoolHUD
	//

	// Render selections
	//glDisableClientState(GL_COLOR_ARRAY);
	//glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	//glDisableClientState(GL_NORMAL_ARRAY);

	/////////////////////////////////////////////////////////////
	//
	// Render 2.5D elements (2D elements in the world)
	// Stuff without z writes
	//

	// Debugging stuff goes before the UI.

	if (gSavedSettings.getBOOL("ShowDepthBuffer"))
	{
		post_show_depth_buffer();
	}

	// Coordinate axes
	if (gSavedSettings.getBOOL("ShowAxes"))
	{
		draw_axes();
	}

	stop_glerror();
		
	gViewerWindow->renderSelections(FALSE, FALSE, TRUE); // Non HUD call in render_hud_elements
	stop_glerror();
}

void render_ui_2d()
{
	LLGLSUIDefault gls_ui;

	/////////////////////////////////////////////////////////////
	//
	// Render 2D UI elements that overlay the world (no z compare)

	//  Disable wireframe mode below here, as this is HUD/menus
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//  Menu overlays, HUD, etc
	gViewerWindow->setup2DRender();

	F32 zoom_factor = gCamera->getZoomFactor();
	S16 sub_region = gCamera->getZoomSubRegion();

	if (zoom_factor > 1.f)
	{
		//decompose subregion number to x and y values
		int pos_y = sub_region / llceil(zoom_factor);
		int pos_x = sub_region - (pos_y*llceil(zoom_factor));
		// offset for this tile
		LLFontGL::sCurOrigin.mX -= llround((F32)gViewerWindow->getWindowWidth() * (F32)pos_x / zoom_factor);
		LLFontGL::sCurOrigin.mY -= llround((F32)gViewerWindow->getWindowHeight() * (F32)pos_y / zoom_factor);
	}

	stop_glerror();
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// render outline for HUD
	if (gAgent.getAvatarObject() && gAgent.getAvatarObject()->mHUDCurZoom < 0.98f)
	{
		glPushMatrix();
		S32 half_width = (gViewerWindow->getWindowWidth() / 2);
		S32 half_height = (gViewerWindow->getWindowHeight() / 2);
		glScalef(LLUI::sGLScaleFactor.mV[0], LLUI::sGLScaleFactor.mV[1], 1.f);
		glTranslatef((F32)half_width, (F32)half_height, 0.f);
		F32 zoom = gAgent.getAvatarObject()->mHUDCurZoom;
		glScalef(zoom,zoom,1.f);
		gGL.color4fv(LLColor4::white.mV);
		gl_rect_2d(-half_width, half_height, half_width, -half_height, FALSE);
		glPopMatrix();
		stop_glerror();
	}
	gViewerWindow->draw();
	if (gDebugSelect)
	{
		gViewerWindow->drawPickBuffer();
	}

	// reset current origin for font rendering, in case of tiling render
	LLFontGL::sCurOrigin.set(0, 0);
}


void render_disconnected_background()
{
	gGL.start();
	if (!gDisconnectedImagep && gDisconnected)
	{
		llinfos << "Loading last bitmap..." << llendl;

		char temp_str[MAX_PATH];		/* Flawfinder: ignore */
		strncpy(temp_str, gDirUtilp->getLindenUserDir().c_str(), MAX_PATH -1);		/* Flawfinder: ignore */
		temp_str[MAX_PATH -1] = '\0';
		strncat(temp_str, gDirUtilp->getDirDelimiter().c_str(), MAX_PATH - strlen(temp_str) -1);		/* Flawfinder: ignore */

		strcat(temp_str, SCREEN_LAST_FILENAME);		/* Flawfinder: ignore */

		LLPointer<LLImageBMP> image_bmp = new LLImageBMP;
		if( !image_bmp->load(temp_str) )
		{
			//llinfos << "Bitmap load failed" << llendl;
			return;
		}

		gDisconnectedImagep = new LLImageGL( FALSE );
		LLPointer<LLImageRaw> raw = new LLImageRaw;
		if (!image_bmp->decode(raw))
		{
			llinfos << "Bitmap decode failed" << llendl;
			gDisconnectedImagep = NULL;
			return;
		}

		U8 *rawp = raw->getData();
		S32 npixels = (S32)image_bmp->getWidth()*(S32)image_bmp->getHeight();
		for (S32 i = 0; i < npixels; i++)
		{
			S32 sum = 0;
			sum = *rawp + *(rawp+1) + *(rawp+2);
			sum /= 3;
			*rawp = ((S32)sum*6 + *rawp)/7;
			rawp++;
			*rawp = ((S32)sum*6 + *rawp)/7;
			rawp++;
			*rawp = ((S32)sum*6 + *rawp)/7;
			rawp++;
		}

		
		raw->expandToPowerOfTwo();
		gDisconnectedImagep->createGLTexture(0, raw);
		gStartImageGL = gDisconnectedImagep;
		LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
	}

	// Make sure the progress view always fills the entire window.
	S32 width = gViewerWindow->getWindowWidth();
	S32 height = gViewerWindow->getWindowHeight();

	if (gDisconnectedImagep)
	{
		LLGLSUIDefault gls_ui;
		gViewerWindow->setup2DRender();
		glPushMatrix();
		{
			// scale ui to reflect UIScaleFactor
			// this can't be done in setup2DRender because it requires a
			// pushMatrix/popMatrix pair
			const LLVector2& display_scale = gViewerWindow->getDisplayScale();
			glScalef(display_scale.mV[VX], display_scale.mV[VY], 1.f);

			LLViewerImage::bindTexture(gDisconnectedImagep);
			gGL.color4f(1.f, 1.f, 1.f, 1.f);
			gl_rect_2d_simple_tex(width, height);
			LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
		}
		glPopMatrix();
	}
	gGL.stop();
}

void display_cleanup()
{
	gDisconnectedImagep = NULL;
}

void process_keystrokes_async()
{
#if LL_WINDOWS
	MSG			msg;
	// look through all input messages, leaving them in the event queue
	while( PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE | PM_NOYIELD))
	{
		// on first mouse message, break out
		if (msg.message >= WM_MOUSEFIRST && 
			msg.message <= WM_MOUSELAST ||
			msg.message == WM_QUIT)
		{
			break;
		}

		// this is a message we want to handle now, so remove it from the event queue
		PeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE | PM_NOYIELD);
		//		if (msg.message == WM_KEYDOWN)
		//		{
		//			llinfos << "Process async key down " << (U32)msg.wParam << llendl;
		//		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Scan keyboard for movement keys.  Command keys and typing
	// are handled by windows callbacks.  Don't do this until we're
	// done initializing.  JC
	if (gViewerWindow->mWindow->getVisible() 
		&& gViewerWindow->getActive()
		&& !gViewerWindow->mWindow->getMinimized()
		&& LLStartUp::getStartupState() == STATE_STARTED
		&& !gViewerWindow->getShowProgress()
		&& !gFocusMgr.focusLocked())
	{
		gKeyboard->scanKeyboard();
	}
#endif
}
