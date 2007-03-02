/** 
 * @file lltoolfocus.cpp
 * @brief A tool to set the build focus point.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

// File includes
#include "lltoolfocus.h" 

// Library includes
#include "v3math.h"
#include "llfontgl.h"
#include "llui.h"

// Viewer includes
#include "llagent.h"
#include "llbutton.h"
#include "llviewercontrol.h"
#include "lldrawable.h"
#include "llhoverview.h"
#include "llhudmanager.h"
#include "llfloatertools.h"
#include "llselectmgr.h"
#include "llstatusbar.h"
#include "lltoolmgr.h"
#include "lltoolselect.h"
#include "llviewborder.h"
#include "llviewercamera.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "viewer.h"
#include "llvoavatar.h"
#include "llmorphview.h"

// Globals
LLToolCamera	*gToolCamera = NULL;
BOOL gCameraBtnOrbit = FALSE;
BOOL gCameraBtnPan = FALSE;
const S32 SLOP_RANGE = 4;
const F32 FOCUS_OFFSET_FACTOR = 1.f;

//
// Camera - shared functionality
//

LLToolCamera::LLToolCamera()
:	LLTool("Camera"),
	mAccumX(0),
	mAccumY(0),
	mMouseDownX(0),
	mMouseDownY(0),
	mOutsideSlopX(FALSE),
	mOutsideSlopY(FALSE),
	mValidClickPoint(FALSE),
	mMouseSteering(FALSE),
	mMouseUpX(0),
	mMouseUpY(0),
	mMouseUpMask(MASK_NONE)
{ }


LLToolCamera::~LLToolCamera()
{ }

// virtual
void LLToolCamera::handleSelect()
{
	if (gFloaterTools)
	{
		gFloaterTools->setStatusText("Click and drag to change view");
	}
}

// virtual
void LLToolCamera::handleDeselect()
{
	if (gFloaterTools)
	{
		gFloaterTools->setStatusText("");
	}
//	gAgent.setLookingAtAvatar(FALSE);
}

BOOL LLToolCamera::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// Ensure a mouseup
	setMouseCapture(TRUE);

	// call the base class to propogate info to sim
	LLTool::handleMouseDown(x, y, mask);

	mAccumX = 0;
	mAccumY = 0;

	mOutsideSlopX = FALSE;
	mOutsideSlopY = FALSE;

	mValidClickPoint = FALSE;

	// If mouse capture gets ripped away, claim we moused up
	// at the point we moused down. JC
	mMouseUpX = x;
	mMouseUpY = y;
	mMouseUpMask = mask;

	gViewerWindow->hideCursor();

	gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback);
	// don't steal focus from UI
	return FALSE;
}

void LLToolCamera::pickCallback(S32 x, S32 y, MASK mask)
{
	if (!gToolCamera->hasMouseCapture())
	{
		return;
	}

	gToolCamera->mMouseDownX = x;
	gToolCamera->mMouseDownY = y;

	gViewerWindow->moveCursorToCenter();

	// Potentially recenter if click outside rectangle
	LLViewerObject* hit_obj = gViewerWindow->lastObjectHit();

	// Check for hit the sky, or some other invalid point
	if (!hit_obj && gLastHitPosGlobal.isExactlyZero())
	{
		gToolCamera->mValidClickPoint = FALSE;
		return;
	}

	// check for hud attachments
	if (hit_obj && hit_obj->isHUDAttachment())
	{
		LLObjectSelectionHandle selection = gSelectMgr->getSelection();
		if (!selection->getObjectCount() || selection->getSelectType() != SELECT_TYPE_HUD)
		{
			gToolCamera->mValidClickPoint = FALSE;
			return;
		}
	}

	if( CAMERA_MODE_CUSTOMIZE_AVATAR == gAgent.getCameraMode() )
	{
		BOOL good_customize_avatar_hit = FALSE;
		if( hit_obj )
		{
			LLVOAvatar* avatar = gAgent.getAvatarObject();
			if( hit_obj == avatar) 
			{
				// It's you
				good_customize_avatar_hit = TRUE;
			}
			else
			if( hit_obj->isAttachment() && hit_obj->permYouOwner() )
			{
				// It's an attachment that you're wearing
				good_customize_avatar_hit = TRUE;
			}
		}

		if( !good_customize_avatar_hit )
		{
			gToolCamera->mValidClickPoint = FALSE;
			return;
		}

		if( gMorphView )
		{
			gMorphView->setCameraDrivenByKeys( FALSE );
		}
	}
	//RN: check to see if this is mouse-driving as opposed to ALT-zoom or Focus tool
	else if (mask & MASK_ALT || 
			(gToolMgr->getCurrentTool()->getName() == "Camera")) 
	{
		LLViewerObject* hit_obj = gViewerWindow->lastObjectHit();
		if (hit_obj)
		{
			// ...clicked on a world object, so focus at its position
			// Use "gLastHitPosGlobal" because it's correct for avatar heads,
			// not pelvis.
			if (!hit_obj->isHUDAttachment())
			{
				gAgent.setFocusOnAvatar(FALSE, ANIMATE);
				gAgent.setFocusGlobal( gLastHitObjectOffset + gLastHitPosGlobal, gLastHitObjectID);
			}
		}
		else if (!gLastHitPosGlobal.isExactlyZero())
		{
			// Hit the ground
			gAgent.setFocusOnAvatar(FALSE, ANIMATE);
			gAgent.setFocusGlobal( gLastHitPosGlobal, gLastHitObjectID);
		}

		if (!(mask & MASK_ALT) &&
			gAgent.cameraThirdPerson() &&
			gViewerWindow->getLeftMouseDown() && 
			!gSavedSettings.getBOOL("FreezeTime") &&
			(hit_obj == gAgent.getAvatarObject() || 
				(hit_obj && hit_obj->isAttachment() && LLVOAvatar::findAvatarFromAttachment(hit_obj)->mIsSelf)))
		{
			gToolCamera->mMouseSteering = TRUE;
		}

	}

	gToolCamera->mValidClickPoint = TRUE;

	if( CAMERA_MODE_CUSTOMIZE_AVATAR == gAgent.getCameraMode() )
	{
		gAgent.setFocusOnAvatar(FALSE, FALSE);
		
		LLVector3d cam_pos = gAgent.getCameraPositionGlobal();
		cam_pos -= LLVector3d(gCamera->getLeftAxis() * gAgent.calcCustomizeAvatarUIOffset( cam_pos ));

		gAgent.setCameraPosAndFocusGlobal( cam_pos, gLastHitObjectOffset + gLastHitPosGlobal, gLastHitObjectID);
	}
}


// "Let go" of the mouse, for example on mouse up or when
// we lose mouse capture.  This ensures that cursor becomes visible
// if a modal dialog pops up during Alt-Zoom. JC
void LLToolCamera::releaseMouse()
{
	// Need to tell the sim that the mouse button is up, since this
	// tool is no longer working and cursor is visible (despite actual
	// mouse button status).
	LLTool::handleMouseUp(mMouseUpX, mMouseUpY, mMouseUpMask);

	gViewerWindow->showCursor();

	gToolMgr->clearTransientTool();

	mMouseSteering = FALSE;
	mValidClickPoint = FALSE;
	mOutsideSlopX = FALSE;
	mOutsideSlopY = FALSE;
}


BOOL LLToolCamera::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// Claim that we're mousing up somewhere
	mMouseUpX = x;
	mMouseUpY = y;
	mMouseUpMask = mask;

	if (hasMouseCapture())
	{
		if (mValidClickPoint)
		{
			if( CAMERA_MODE_CUSTOMIZE_AVATAR == gAgent.getCameraMode() )
			{
				LLCoordGL mouse_pos;
				LLVector3 focus_pos = gAgent.getPosAgentFromGlobal(gAgent.getFocusGlobal());
				gCamera->projectPosAgentToScreen(focus_pos, mouse_pos);

				LLUI::setCursorPositionScreen(mouse_pos.mX, mouse_pos.mY);
			}
			else if (mMouseSteering)
			{
				LLUI::setCursorPositionScreen(mMouseDownX, mMouseDownY);
			}
			else
			{
				gViewerWindow->moveCursorToCenter();
			}
		}
		else
		{
			// not a valid zoomable object
			LLUI::setCursorPositionScreen(mMouseDownX, mMouseDownY);
		}

		// calls releaseMouse() internally
		setMouseCapture(FALSE);
	}
	else
	{
		releaseMouse();
	}

	return TRUE;
}


BOOL LLToolCamera::handleHover(S32 x, S32 y, MASK mask)
{
	S32 dx = gViewerWindow->getCurrentMouseDX();
	S32 dy = gViewerWindow->getCurrentMouseDY();
	
	BOOL moved_outside_slop = FALSE;
	
	if (hasMouseCapture() && mValidClickPoint)
	{
		mAccumX += llabs(dx);
		mAccumY += llabs(dy);

		if (mAccumX >= SLOP_RANGE)
		{
			if (!mOutsideSlopX)
			{
				moved_outside_slop = TRUE;
			}
			mOutsideSlopX = TRUE;
		}

		if (mAccumY >= SLOP_RANGE)
		{
			if (!mOutsideSlopY)
			{
				moved_outside_slop = TRUE;
			}
			mOutsideSlopY = TRUE;
		}
	}

	if (mOutsideSlopX || mOutsideSlopY)
	{
		if (!mValidClickPoint)
		{
			lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolFocus [invalid point]" << llendl;
			gViewerWindow->setCursor(UI_CURSOR_NO);
			gViewerWindow->showCursor();
			return TRUE;
		}

		if (gCameraBtnOrbit ||
			mask == MASK_ORBIT || 
			mask == (MASK_ALT | MASK_ORBIT))
		{
			// Orbit tool
			if (hasMouseCapture())
			{
				const F32 RADIANS_PER_PIXEL = 360.f * DEG_TO_RAD / gViewerWindow->getWindowWidth();

				if (dx != 0)
				{
					gAgent.cameraOrbitAround( -dx * RADIANS_PER_PIXEL );
				}

				if (dy != 0)
				{
					gAgent.cameraOrbitOver( -dy * RADIANS_PER_PIXEL );
				}

				gViewerWindow->moveCursorToCenter();
			}
			lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolFocus [active]" << llendl;
		}
		else if (	gCameraBtnPan ||
					mask == MASK_PAN ||
					mask == (MASK_PAN | MASK_ALT) )
		{
			// Pan tool
			if (hasMouseCapture())
			{
				LLVector3d camera_to_focus = gAgent.getCameraPositionGlobal();
				camera_to_focus -= gAgent.getFocusGlobal();
				F32 dist = (F32) camera_to_focus.normVec();

				// Fudge factor for pan
				F32 meters_per_pixel = 3.f * dist / gViewerWindow->getWindowWidth();

				if (dx != 0)
				{
					gAgent.cameraPanLeft( dx * meters_per_pixel );
				}

				if (dy != 0)
				{
					gAgent.cameraPanUp( -dy * meters_per_pixel );
				}

				gViewerWindow->moveCursorToCenter();
			}
			lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolPan" << llendl;
		}
		else
		{
			// Zoom tool
			if (hasMouseCapture())
			{

				const F32 RADIANS_PER_PIXEL = 360.f * DEG_TO_RAD / gViewerWindow->getWindowWidth();

				if (dx != 0)
				{
					gAgent.cameraOrbitAround( -dx * RADIANS_PER_PIXEL );
				}

				const F32 IN_FACTOR = 0.99f;

				if (dy != 0 && mOutsideSlopY )
				{
					if (mMouseSteering)
					{
						gAgent.cameraOrbitOver( -dy * RADIANS_PER_PIXEL );
					}
					else
					{
						gAgent.cameraZoomIn( pow( IN_FACTOR, dy ) );
					}
				}

				gViewerWindow->moveCursorToCenter();
			}

			lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolZoom" << llendl;		
		}
	}

	if (gCameraBtnOrbit ||
		mask == MASK_ORBIT || 
		mask == (MASK_ALT | MASK_ORBIT))
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLCAMERA);
	}
	else if (	gCameraBtnPan ||
				mask == MASK_PAN ||
				mask == (MASK_PAN | MASK_ALT) )
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLPAN);
	}
	else
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLZOOMIN);
	}
	
	return TRUE;
}


void LLToolCamera::onMouseCaptureLost()
{
	releaseMouse();
}
