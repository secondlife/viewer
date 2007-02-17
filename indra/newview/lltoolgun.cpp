/** 
 * @file lltoolgun.cpp
 * @brief LLToolGun class implementation
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lltoolgun.h"

#include "llviewerwindow.h"
#include "llagent.h"
#include "llviewercontrol.h"
#include "llsky.h"
#include "viewer.h"
#include "llresmgr.h"
#include "llfontgl.h"
#include "llui.h"
#include "llviewerimagelist.h"
#include "llviewercamera.h"
#include "llhudmanager.h"
#include "lltoolmgr.h"
#include "lltoolgrab.h"

LLToolGun::LLToolGun( LLToolComposite* composite )
	:
	LLTool( "gun", composite )
{
	mCrosshairImg = gImageList.getImage( LLUUID( gSavedSettings.getString("UIImgCrosshairsUUID") ), MIPMAP_FALSE, TRUE );
}

void LLToolGun::handleSelect()
{
	gViewerWindow->hideCursor();
	gViewerWindow->moveCursorToCenter();
	gViewerWindow->mWindow->setMouseClipping(TRUE);
}

void LLToolGun::handleDeselect()
{
	gViewerWindow->moveCursorToCenter();
	gViewerWindow->showCursor();
	gViewerWindow->mWindow->setMouseClipping(FALSE);
}

BOOL LLToolGun::handleMouseDown(S32 x, S32 y, MASK mask)
{
	gGrabTransientTool = this;
	gToolMgr->getCurrentToolset()->selectTool( gToolGrab );

	return gToolGrab->handleMouseDown(x, y, mask);
}

BOOL LLToolGun::handleHover(S32 x, S32 y, MASK mask) 
{
	if( gAgent.cameraMouselook() )
	{
		#if 1 //LL_WINDOWS || LL_DARWIN
			const F32 NOMINAL_MOUSE_SENSITIVITY = 0.0025f;
		#else
			const F32 NOMINAL_MOUSE_SENSITIVITY = 0.025f;
		#endif

		
		F32 mouse_sensitivity = clamp_rescale(gMouseSensitivity, 0.f, 15.f, 0.5f, 2.75f) * NOMINAL_MOUSE_SENSITIVITY;

		// ...move the view with the mouse

		// get mouse movement delta
		S32 dx = -gViewerWindow->getCurrentMouseDX();
		S32 dy = -gViewerWindow->getCurrentMouseDY();
		
		if (dx != 0 || dy != 0)
		{
			// ...actually moved off center
			if (gInvertMouse)
			{
				gAgent.pitch(mouse_sensitivity * -dy);
			}
			else
			{
				gAgent.pitch(mouse_sensitivity * dy);
			}
			LLVector3 skyward = gAgent.getReferenceUpVector();
			gAgent.rotate(mouse_sensitivity * dx, skyward.mV[VX], skyward.mV[VY], skyward.mV[VZ]);

			if (gSavedSettings.getBOOL("MouseSun"))
			{
				gSky.setSunDirection(gCamera->getAtAxis(), LLVector3(0.f, 0.f, 0.f));
				gSky.setOverrideSun(TRUE);
				gSavedSettings.setVector3("SkySunDefaultPosition", gCamera->getAtAxis());
			}

			gViewerWindow->moveCursorToCenter();
			gViewerWindow->hideCursor();
		}

		lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolGun (mouselook)" << llendl;
	}
	else
	{
		lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolGun (not mouselook)" << llendl;
	}

	// HACK to avoid assert: error checking system makes sure that the cursor is set during every handleHover.  This is actually a no-op since the cursor is hidden.
	gViewerWindow->setCursor(UI_CURSOR_ARROW);  

	return TRUE;
}

void LLToolGun::draw()
{
	if( gSavedSettings.getBOOL("ShowCrosshairs") )
	{
		gl_draw_image(
			( gViewerWindow->getWindowWidth() - mCrosshairImg->getWidth() ) / 2,
			( gViewerWindow->getWindowHeight() - mCrosshairImg->getHeight() ) / 2,
			mCrosshairImg );
	}
}
