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

#include "llagent.h"
#include "llviewercontrol.h"
#include "llcoord.h"
#include "llcriticaldamp.h"
#include "lldir.h"
#include "lldynamictexture.h"
#include "lldrawpoolalpha.h"
#include "llfeaturemanager.h"
#include "llframestats.h"
#include "llgl.h"
#include "llglheaders.h"
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
#include "viewer.h"
#include "llstartup.h"
#include "llfasttimer.h"
#include "llfloatertools.h"
#include "llviewerimagelist.h"
#include "llfocusmgr.h"
#include "llcubemap.h"
#include "llviewerregion.h"
#include "lldrawpoolwater.h"

extern U32 gFrameCount;
extern LLPointer<LLImageGL> gStartImageGL;
extern LLPointer<LLImageGL> gDisconnectedImagep;
extern BOOL gLogoutRequestSent;
extern LLTimer gLogoutTimer;
extern BOOL gHaveSavedSnapshot;
extern BOOL gDisplaySwapBuffers;

// used to toggle renderer back on after teleport
const F32 TELEPORT_RENDER_DELAY = 20.f; // Max time a teleport is allowed to take before we raise the curtain
const F32 TELEPORT_ARRIVAL_DELAY = 2.f; // Time to preload the world before raising the curtain after we've actually already arrived.
BOOL		 gTeleportDisplay = FALSE;
LLFrameTimer gTeleportDisplayTimer;
LLFrameTimer gTeleportArrivalTimer;
const F32		RESTORE_GL_TIME = 5.f;	// Wait this long while reloading textures before we raise the curtain

BOOL gForceRenderLandFence = FALSE;
BOOL gDisplaySwapBuffers = FALSE;

// Rendering stuff
void pre_show_depth_buffer();
void post_show_depth_buffer();
void render_ui_and_swap();
void render_ui_3d();
void render_ui_2d();
void render_disconnected_background();

void process_keystrokes_async(); // in viewer.cpp

void display_startup()
{
	if (   !gViewerWindow->getActive()
		|| !gViewerWindow->mWindow->getVisible() 
		|| gViewerWindow->mWindow->getMinimized()
		|| gNoRender )
	{
		return; 
	}

	// Required for HTML update in login screen
	static S32 frame_count = 0;
	if (frame_count++ > 1) // make sure we have rendered a frame first
	{
		LLDynamicTexture::updateAllInstances();
	}
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	LLGLSDefault gls_default;
	LLGLSUIDefault gls_ui;
	gPipeline.disableLights();

	gViewerWindow->setup2DRender();
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	gViewerWindow->draw();
	gViewerWindow->mWindow->swapBuffers();
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

	// Update land visibility too
	if (gWorldp)
	{
		gWorldp->setLandFarClip(final_far);
	}
}


// Paint the display!
void display(BOOL rebuild, F32 zoom_factor, int subfield)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER);

	LLGLSDefault gls_default;
	LLGLDepthTest gls_depth(GL_TRUE, GL_TRUE, GL_LEQUAL);

	// No clue where this is getting unset, but safe enough to reset it here.
	LLGLState::resetTextureStates();
	
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
	else if(gLogoutRequestSent)
	{
		F32 percent_done = gLogoutTimer.getElapsedTimeF32() * 100.f / gLogoutMaxTime;
		if (percent_done > 100.f)
		{
			percent_done = 100.f;
		}

		if( gQuit )
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

			if( gQuit )
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

	// Hmm...  Should this be moved elsewhere? - djs 09/09/02
	// do render-to-texture stuff here
	if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_DYNAMIC_TEXTURES))
	{
// 		LLFastTimer t(LLFastTimer::FTM_UPDATE_TEXTURES);
		if (LLDynamicTexture::updateAllInstances())
		{
			glClear(GL_COLOR_BUFFER_BIT);
		}
	}


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
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		render_disconnected_background();
	}
	else if (!gViewerWindow->isPickPending())
	{
		glClear( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
		//DEBUG TEMPORARY
		glClear(GL_COLOR_BUFFER_BIT);
	}
	gViewerWindow->setupViewport();


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

	if(gUseWireframe)//gSavedSettings.getBOOL("UseWireframe"))
	{
		glClearColor(0.5f, 0.5f, 0.5f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		LLPipeline::sUseOcclusion = FALSE;
	}
	else
	{
		LLPipeline::sUseOcclusion = gSavedSettings.getBOOL("UseOcclusion") && gGLManager.mHasOcclusionQuery && gFeatureManagerp->isFeatureAvailable("UseOcclusion");
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
	
	//Increment drawable frame counter
	LLDrawable::incrementVisible();

	/////////////////////////////////////
	//
	// Render
	//
	// Actually push all of our triangles to the screen.
	//
	if (!gDisconnected)
	{
		if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
		{ //don't draw hud objects in this frame
			gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
		}
		
		LLFastTimer t(LLFastTimer::FTM_WORLD_UPDATE);
		stop_glerror();
		display_update_camera();
		stop_glerror();
		
		// *TODO: merge these two methods
		gHUDManager->updateEffects();
		LLHUDObject::updateAll();
		stop_glerror();
		
		gFrameStats.start(LLFrameStats::UPDATE_GEOM);
		const F32 max_geom_update_time = 0.005f; // 5 ms update time
		gPipeline.updateGeom(max_geom_update_time);
		stop_glerror();
		
		LLSpatialPartition* part = gPipeline.getSpatialPartition(LLPipeline::PARTITION_VOLUME);
		part->processImagery(gCamera);

		display_update_camera();

		gFrameStats.start(LLFrameStats::UPDATE_CULL);
		gPipeline.updateCull(*gCamera);
		stop_glerror();
		
		///////////////////////////////////
		//
		// StateSort
		//
		// Responsible for taking visible objects, and adding them to the appropriate draw orders.
		// In the case of alpha objects, z-sorts them first.
		// Also creates special lists for outlines and selected face rendering.
		//
		{
			LLFastTimer t(LLFastTimer::FTM_REBUILD);
			
			gFrameStats.start(LLFrameStats::STATE_SORT);
			gPipeline.stateSort(*gCamera);
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
	//		glColor4fv(LLColor4::white.mV);
	//		glBegin(GL_QUADS);
	//		{
	//			glVertex3f(floater_3d_rect.mLeft, floater_3d_rect.mBottom, 0.f);
	//			glVertex3f(floater_3d_rect.mLeft, floater_3d_rect.mTop, 0.f);
	//			glVertex3f(floater_3d_rect.mRight, floater_3d_rect.mTop, 0.f);
	//			glVertex3f(floater_3d_rect.mRight, floater_3d_rect.mBottom, 0.f);
	//		}
	//		glEnd();
	//		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	//	}
	//	glPopMatrix();
	//}

	if (!(gLogoutRequestSent && gHaveSavedSnapshot) 
			&& !gRestoreGL
			&& !gDisconnected)
	{
		gPipeline.renderGeom(*gCamera);
		stop_glerror();
	}

	//render hud attachments
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	if (LLPipeline::sShowHUDAttachments && !gDisconnected && setup_hud_matrices(FALSE))
	{
		LLCamera hud_cam = *gCamera;
		glClear(GL_DEPTH_BUFFER_BIT);
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

		BOOL use_occlusion = gSavedSettings.getBOOL("UseOcclusion");
		gSavedSettings.setBOOL("UseOcclusion", FALSE);

		//cull, sort, and render hud objects
		gPipeline.updateCull(hud_cam);

		gPipeline.toggleRenderType(LLDrawPool::POOL_ALPHA_POST_WATER);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_BUMP);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_SIMPLE);
		gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_VOLUME);
		
		{
			LLFastTimer ftm(LLFastTimer::FTM_REBUILD);
			gPipeline.stateSort(hud_cam);
		}
				
		gPipeline.renderGeom(hud_cam);

		//restore type mask
		gPipeline.setRenderTypeMask(mask);
		if (has_ui)
		{
			gPipeline.toggleRenderDebugFeature((void*) LLPipeline::RENDER_DEBUG_FEATURE_UI);
		}
		gSavedSettings.setBOOL("UseOcclusion", use_occlusion);
	}
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	gFrameStats.start(LLFrameStats::RENDER_UI);

	if (gHandleKeysAsync)
	{
		process_keystrokes_async();
		stop_glerror();
	}

	
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkStates();
#endif
	render_ui_and_swap();
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkStates();
#endif

	gFrameStats.start(LLFrameStats::MISC_END);
	stop_glerror();

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
			glClear(GL_DEPTH_BUFFER_BIT);
		}
		LLBBox hud_bbox = my_avatarp->getHUDBBox();

		
		// set up transform to encompass bounding box of HUD
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		F32 hud_depth = llmax(1.f, hud_bbox.getExtentLocal().mV[VX] * 1.1f);
		if (for_select)
		{
			//RN: reset viewport to window extents so ortho screen is calculated with proper reference frame
			gViewerWindow->setupViewport();
		}
		glOrtho(-0.5f * gCamera->getAspect(), 0.5f * gCamera->getAspect(), -0.5f, 0.5f, 0.f, hud_depth);

		// apply camera zoom transform (for high res screenshots)
		F32 zoom_factor = gCamera->getZoomFactor();
		S16 sub_region = gCamera->getZoomSubRegion();
		if (zoom_factor > 1.f)
		{
			float offset = zoom_factor - 1.f;
			int pos_y = sub_region / llceil(zoom_factor);
			int pos_x = sub_region - (pos_y*llceil(zoom_factor));
			glTranslatef(gCamera->getAspect() * 0.5f * (offset - (F32)pos_x * 2.f), 0.5f * (offset - (F32)pos_y * 2.f), 0.f);
			glScalef(zoom_factor, zoom_factor, 1.f);
		}

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glLoadMatrixf(OGL_TO_CFR_ROTATION);		// Load Cory's favorite reference frame
		glTranslatef(-hud_bbox.getCenterLocal().mV[VX] + (hud_depth * 0.5f), 0.f, 0.f);
		glScalef(zoom_level, zoom_level, zoom_level);
		
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

	LLGLSDefault gls_default;
	{
		LLGLSUIDefault gls_ui;
		gPipeline.disableLights();
		LLVertexBuffer::startRender();
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
		LLVertexBuffer::stopRender();
		glFlush();

		// now do the swap buffer
		if (gDisplaySwapBuffers)
		{
			LLFastTimer t(LLFastTimer::FTM_SWAP);
			gViewerWindow->mWindow->swapBuffers();
		}

		{
 			LLFastTimer ftm(LLFastTimer::FTM_CLIENT_COPY);
			LLVertexBuffer::clientCopy(0.016);
		}

	}
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
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

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
		glColor4fv(LLColor4::white.mV);
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

void renderCoordinateAxes()
{
	LLGLSNoTexture gls_no_texture;
	glBegin(GL_LINES);
		glColor3f(1.0f, 0.0f, 0.0f);   // i direction = X-Axis = red
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(2.0f, 0.0f, 0.0f);
		glVertex3f(3.0f, 0.0f, 0.0f);
		glVertex3f(5.0f, 0.0f, 0.0f);
		glVertex3f(6.0f, 0.0f, 0.0f);
		glVertex3f(8.0f, 0.0f, 0.0f);
		// Make an X
		glVertex3f(11.0f, 1.0f, 1.0f);
		glVertex3f(11.0f, -1.0f, -1.0f);
		glVertex3f(11.0f, 1.0f, -1.0f);
		glVertex3f(11.0f, -1.0f, 1.0f);

		glColor3f(0.0f, 1.0f, 0.0f);   // j direction = Y-Axis = green
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, 2.0f, 0.0f);
		glVertex3f(0.0f, 3.0f, 0.0f);
		glVertex3f(0.0f, 5.0f, 0.0f);
		glVertex3f(0.0f, 6.0f, 0.0f);
		glVertex3f(0.0f, 8.0f, 0.0f);
		// Make a Y
		glVertex3f(1.0f, 11.0f, 1.0f);
		glVertex3f(0.0f, 11.0f, 0.0f);
		glVertex3f(-1.0f, 11.0f, 1.0f);
		glVertex3f(0.0f, 11.0f, 0.0f);
		glVertex3f(0.0f, 11.0f, 0.0f);
		glVertex3f(0.0f, 11.0f, -1.0f);

		glColor3f(0.0f, 0.0f, 1.0f);   // Z-Axis = blue
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, 0.0f, 2.0f);
		glVertex3f(0.0f, 0.0f, 3.0f);
		glVertex3f(0.0f, 0.0f, 5.0f);
		glVertex3f(0.0f, 0.0f, 6.0f);
		glVertex3f(0.0f, 0.0f, 8.0f);
		// Make a Z
		glVertex3f(-1.0f, 1.0f, 11.0f);
		glVertex3f(1.0f, 1.0f, 11.0f);
		glVertex3f(1.0f, 1.0f, 11.0f);
		glVertex3f(-1.0f, -1.0f, 11.0f);
		glVertex3f(-1.0f, -1.0f, 11.0f);
		glVertex3f(1.0f, -1.0f, 11.0f);
	glEnd();
}

void draw_axes() 
{
	LLGLSUIDefault gls_ui;
	LLGLSNoTexture gls_no_texture;
	// A vertical white line at origin
	LLVector3 v = gAgent.getPositionAgent();
	glBegin(GL_LINES);
		glColor3f(1.0f, 1.0f, 1.0f); 
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, 0.0f, 40.0f);
	glEnd();
	// Some coordinate axes
	glPushMatrix();
		glTranslatef( v.mV[VX], v.mV[VY], v.mV[VZ] );
		renderCoordinateAxes();
	glPopMatrix();
}


void render_disconnected_background()
{
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
			glColor4f(1.f, 1.f, 1.f, 1.f);
			gl_rect_2d_simple_tex(width, height);
			LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
		}
		glPopMatrix();
	}
}
