/** 
 * @file lltoolgun.cpp
 * @brief LLToolGun class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "lltoolgun.h"

#include "llviewerwindow.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llviewercontrol.h"
#include "llsky.h"
#include "llappviewer.h"
#include "llresmgr.h"
#include "llfontgl.h"
#include "llui.h"
#include "llviewertexturelist.h"
#include "llviewercamera.h"
#include "llhudmanager.h"
#include "lltoolmgr.h"
#include "lltoolgrab.h"

// Linden library includes
#include "llwindow.h"			// setMouseClipping()

LLToolGun::LLToolGun( LLToolComposite* composite )
:	LLTool( std::string("gun"), composite ),
		mIsSelected(FALSE)
{
}

void LLToolGun::handleSelect()
{
	gViewerWindow->hideCursor();
	gViewerWindow->moveCursorToCenter();
	gViewerWindow->getWindow()->setMouseClipping(TRUE);
	mIsSelected = TRUE;
}

void LLToolGun::handleDeselect()
{
	gViewerWindow->moveCursorToCenter();
	gViewerWindow->showCursor();
	gViewerWindow->getWindow()->setMouseClipping(FALSE);
	mIsSelected = FALSE;
}

BOOL LLToolGun::handleMouseDown(S32 x, S32 y, MASK mask)
{
	gGrabTransientTool = this;
	LLToolMgr::getInstance()->getCurrentToolset()->selectTool( LLToolGrab::getInstance() );

	return LLToolGrab::getInstance()->handleMouseDown(x, y, mask);
}

BOOL LLToolGun::handleHover(S32 x, S32 y, MASK mask) 
{
	if( gAgentCamera.cameraMouselook() && mIsSelected )
	{
		const F32 NOMINAL_MOUSE_SENSITIVITY = 0.0025f;

		F32 mouse_sensitivity = gSavedSettings.getF32("MouseSensitivity");
		mouse_sensitivity = clamp_rescale(mouse_sensitivity, 0.f, 15.f, 0.5f, 2.75f) * NOMINAL_MOUSE_SENSITIVITY;

		// ...move the view with the mouse

		// get mouse movement delta
		S32 dx = -gViewerWindow->getCurrentMouseDX();
		S32 dy = -gViewerWindow->getCurrentMouseDY();
		
		if (dx != 0 || dy != 0)
		{
			// ...actually moved off center
			if (gSavedSettings.getBOOL("InvertMouse"))
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
				gSky.setSunDirection(LLViewerCamera::getInstance()->getAtAxis(), LLVector3(0.f, 0.f, 0.f));
				gSky.setOverrideSun(TRUE);
				gSavedSettings.setVector3("SkySunDefaultPosition", LLViewerCamera::getInstance()->getAtAxis());
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
		LLUIImagePtr crosshair = LLUI::getUIImage("crosshairs.tga");
		crosshair->draw(
			( gViewerWindow->getWorldViewRectScaled().getWidth() - crosshair->getWidth() ) / 2,
			( gViewerWindow->getWorldViewRectScaled().getHeight() - crosshair->getHeight() ) / 2);
	}
}
