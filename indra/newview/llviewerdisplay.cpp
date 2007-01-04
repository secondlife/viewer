/** 
 * @file llviewerdisplay.cpp
 * @brief LLViewerDisplay class implementation
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llviewercontrol.h"
#include "llcoord.h"
#include "llcriticaldamp.h"
#include "lldir.h"
#include "lldynamictexture.h"
#include "lldrawpoolalpha.h"
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

	LLDynamicTexture::updateAllInstances();

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
	for (S32 j = 7; j >=0; j--)
	{
		glActiveTextureARB(GL_TEXTURE0_ARB+j);
		glClientActiveTextureARB(GL_TEXTURE0_ARB+j);
		j == 0 ? glEnable(GL_TEXTURE_2D) : glDisable(GL_TEXTURE_2D);
	}
	
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
			if (!gViewerWindow->renderingFastFrame())
			{
				gFrameStats.start(LLFrameStats::REBUILD);
				gPipeline.rebuildPools();
			}
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
	if (gStartupState < STATE_STARTED)
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
			gAgent.setTeleportMessage("Requesting Teleport...");
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
			gAgent.setTeleportMessage("Arriving...");
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

		case LLAgent::TELEPORT_CANCELLING:
			gViewerWindow->setProgressCancelButtonVisible(FALSE, "Cancel");
			gViewerWindow->setProgressPercent(  100.f );
			gViewerWindow->setProgressString("Canceling...");
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

	if (rebuild)
	{
		if (gViewerWindow->renderingFastFrame())
		{
			gFrameStats.start(LLFrameStats::STATE_SORT);
			gFrameStats.start(LLFrameStats::REBUILD);
		}
	}

	/////////////////////////////
	//
	// Update the camera
	//
	//

	gCamera->setZoomParameters(zoom_factor, subfield);

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
	
	//LLGLState::verify();

	/////////////////////////////////////
	//
	// Render
	//
	// Actually push all of our triangles to the screen.
	//
	if (!gDisconnected)
	{
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
		
		gFrameStats.start(LLFrameStats::UPDATE_CULL);
		gPipeline.updateCull();
		stop_glerror();
		
		if (rebuild && !gViewerWindow->renderingFastFrame())
		{
			LLFastTimer t(LLFastTimer::FTM_REBUILD);

			///////////////////////////////////
			//
			// StateSort
			//
			// Responsible for taking visible objects, and adding them to the appropriate draw orders.
			// In the case of alpha objects, z-sorts them first.
			// Also creates special lists for outlines and selected face rendering.
			//
			gFrameStats.start(LLFrameStats::STATE_SORT);
			gPipeline.stateSort();
			stop_glerror();
			
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

	if (gViewerWindow->renderingFastFrame())
	{
		gFrameStats.start(LLFrameStats::RENDER_SYNC);
		gFrameStats.start(LLFrameStats::RENDER_GEOM);
	}
	else if (!(gLogoutRequestSent && gHaveSavedSnapshot) 
			&& !gRestoreGL
			&& !gDisconnected)
	{
		gPipeline.renderGeom();
		stop_glerror();
	}

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


void render_ui_and_swap()
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkStates();
#endif

	LLGLSDefault gls_default;
	{
		LLGLSUIDefault gls_ui;
		gPipeline.disableLights();
		
		if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
		{
			LLFastTimer t(LLFastTimer::FTM_RENDER_UI);
			if (!gViewerWindow->renderingFastFrame() && !gDisconnected)
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

		// now do the swap buffer
		if (gDisplaySwapBuffers)
		{
			LLFastTimer t(LLFastTimer::FTM_SWAP);
			gViewerWindow->mWindow->swapBuffers();
		}
	}

	gViewerWindow->finishFirstFastFrame();
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

	glDisableClientState(GL_VERTEX_ARRAY);
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
		glTranslatef((F32)half_width, (F32)half_height, 0.f);
		glScalef(gAgent.getAvatarObject()->mHUDCurZoom, gAgent.getAvatarObject()->mHUDCurZoom, gAgent.getAvatarObject()->mHUDCurZoom);
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

		char temp_str[MAX_PATH];
		strcpy(temp_str, gDirUtilp->getLindenUserDir().c_str());
		strcat(temp_str, gDirUtilp->getDirDelimiter().c_str());

		strcat(temp_str, SCREEN_LAST_FILENAME);

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
