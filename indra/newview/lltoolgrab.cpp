/** 
 * @file lltoolgrab.cpp
 * @brief LLToolGrab class implementation
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lltoolgrab.h"
 
// library headers
#include "indra_constants.h"		// for agent control flags
#include "llviewercontrol.h"
#include "llquaternion.h"
#include "llbox.h"
#include "message.h"
#include "llview.h"
#include "llfontgl.h"
#include "llui.h"

// newview headers
#include "llagent.h"
//#include "llfloateravatarinfo.h"
#include "lldrawable.h"
#include "llfloatertools.h"
#include "llhudeffect.h"
#include "llhudmanager.h"
#include "llregionposition.h"
#include "llselectmgr.h"
#include "llstatusbar.h"
#include "lltoolmgr.h"
#include "lltoolpie.h"
#include "llviewercamera.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h" 
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llworld.h"
#include "viewer.h"

const S32 SLOP_DIST_SQ = 4;

// Override modifier key behavior with these buttons
BOOL gGrabBtnVertical = FALSE;
BOOL gGrabBtnSpin = FALSE;
LLTool* gGrabTransientTool = NULL;
LLToolGrab *gToolGrab = NULL;
extern BOOL gDebugClicks;

//
// Methods
//
LLToolGrab::LLToolGrab( LLToolComposite* composite )
:	LLTool( "Grab", composite ),
	mMode( GRAB_INACTIVE ),
	mVerticalDragging( FALSE ),
	mHitLand(FALSE),
	mHitObjectID(),
	mGrabObject( NULL ),
	mMouseDownX( -1 ),
	mMouseDownY( -1 ),
	mHasMoved( FALSE ),
	mSpinGrabbing( FALSE ),
	mSpinRotation(),
	mHideBuildHighlight(FALSE)
{ }

LLToolGrab::~LLToolGrab()
{ }


// virtual
void LLToolGrab::handleSelect()
{
	gFloaterTools->setStatusText("Drag to move objects, Ctrl to lift, Ctrl-Shift to spin");
	gGrabBtnVertical = FALSE;
	gGrabBtnSpin = FALSE;
}

void LLToolGrab::handleDeselect()
{
	if( hasMouseCapture() )
	{
		setMouseCapture( FALSE );
	}

	gFloaterTools->setStatusText("");
}

BOOL LLToolGrab::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (gDebugClicks)
	{
		llinfos << "LLToolGrab handleDoubleClick (becoming mouseDown)" << llendl;
	}

	return FALSE;
}

BOOL LLToolGrab::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (gDebugClicks)
	{
		llinfos << "LLToolGrab handleMouseDown" << llendl;
	}

	mHitLand = FALSE;

	// call the base class to propogate info to sim
	LLTool::handleMouseDown(x, y, mask);
	
	if (!gAgent.leftButtonGrabbed())
	{
		// can grab transparent objects (how touch event propagates, scripters rely on this)
		gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback, TRUE);
	}
	return TRUE;
}

void LLToolGrab::pickCallback(S32 x, S32 y, MASK mask)
{
	LLViewerObject	*objectp = gObjectList.findObject( gLastHitObjectID );

	BOOL extend_select = (mask & MASK_SHIFT);

	if (!extend_select && !gSelectMgr->getSelection()->isEmpty())
	{
		gSelectMgr->deselectAll();
		gToolGrab->mDeselectedThisClick = TRUE;
	}
	else
	{
		gToolGrab->mDeselectedThisClick = FALSE;
	}

	// if not over object, do nothing
	if (!objectp)
	{
		gToolGrab->setMouseCapture(TRUE);
		gToolGrab->mMode = GRAB_NOOBJECT;
		gToolGrab->mHitObjectID.setNull();
	}
	else
	{
		gToolGrab->handleObjectHit(objectp, x, y, mask);
	}
}

BOOL LLToolGrab::handleObjectHit(LLViewerObject *objectp, S32 x, S32 y, MASK mask)
{
	mMouseDownX = x;
	mMouseDownY = y;
	mMouseMask = mask;

	if (gDebugClicks)
	{
		llinfos << "LLToolGrab handleObjectHit " << mMouseDownX << "," << mMouseDownY << llendl;
	}

	if (objectp->isAvatar())
	{
		if (gGrabTransientTool)
		{
			gBasicToolset->selectTool( gGrabTransientTool );
			gGrabTransientTool = NULL;
		}
		return TRUE;
	}

	setMouseCapture( TRUE );

	mHitObjectID = objectp->getID();

	// Grabs always start from the root
	// objectp = (LLViewerObject *)objectp->getRoot();

	LLViewerObject* parent = objectp->getRootEdit();
	BOOL script_touch = (objectp && objectp->flagHandleTouch()) || (parent && parent->flagHandleTouch());

	// Clicks on scripted or physical objects are temporary grabs, so
	// not "Build mode"
	mHideBuildHighlight = script_touch || objectp->usePhysics();

	if (!objectp->usePhysics())
	{
		// In mouselook, we shouldn't be able to grab non-physical, 
		// non-touchable objects.  If it has a touch handler, we
		// do grab it (so llDetectedGrab works), but movement is
		// blocked on the server side. JC
		if (gAgent.cameraMouselook() && !script_touch)
		{
			mMode = GRAB_LOCKED;
		}
		else
		{
			mMode = GRAB_NONPHYSICAL;
		}
		gViewerWindow->hideCursor();
		gViewerWindow->moveCursorToCenter();
		// Don't bail out here, go on and grab so buttons can get
		// their "touched" event.
	}
	else if( !objectp->permMove() )
	{
		// if mouse is over a physical object without move permission, show feedback if user tries to move it.
		mMode = GRAB_LOCKED;

		// Don't bail out here, go on and grab so buttons can get
		// their "touched" event.
	}
	else
	{
		// if mouse is over a physical object with move permission, 
		// select it and enter "grab" mode (hiding cursor, etc.)

		mMode = GRAB_ACTIVE_CENTER;

		gViewerWindow->hideCursor();
		gViewerWindow->moveCursorToCenter();
	}

	// Always send "touched" message

	mAccumDeltaX = 0;
	mAccumDeltaY = 0;
	mHasMoved = FALSE;
	mOutsideSlop = FALSE;

	mGrabObject = objectp;

	mGrabOffset.clearVec();

	mVerticalDragging = (mask == MASK_VERTICAL) || gGrabBtnVertical;

	startGrab(x, y);

	if ((mask == MASK_SPIN) || gGrabBtnSpin)
	{
		startSpin();
	}

	gSelectMgr->updateSelectionCenter();		// update selection beam

	// update point at
	LLViewerObject *edit_object = gObjectList.findObject(mHitObjectID);
	if (edit_object)
	{
		LLVector3 local_edit_point = gAgent.getPosAgentFromGlobal(gLastHitNonFloraPosGlobal);
		local_edit_point -= edit_object->getPositionAgent();
		local_edit_point = local_edit_point * ~edit_object->getRenderRotation();
		gAgent.setPointAt(POINTAT_TARGET_GRAB, edit_object, local_edit_point );
		gAgent.setLookAt(LOOKAT_TARGET_SELECT, edit_object, local_edit_point );
	}

	// on transient grabs (clicks on world objects), kill the grab immediately
	if (!gViewerWindow->getLeftMouseDown() 
		&& gGrabTransientTool 
		&& (mMode == GRAB_NONPHYSICAL || mMode == GRAB_LOCKED))
	{
		gBasicToolset->selectTool( gGrabTransientTool );
		gGrabTransientTool = NULL;
	}

	return TRUE;
}


void LLToolGrab::startSpin()
{
	mSpinGrabbing = TRUE;

	// Was saveSelectedObjectTransform()
	LLViewerObject *root = (LLViewerObject *)mGrabObject->getRoot();
	mSpinRotation = root->getRotation();

	LLMessageSystem *msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ObjectSpinStart);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addUUIDFast(_PREHASH_ObjectID, mGrabObject->getID() );
	msg->sendMessage( mGrabObject->getRegion()->getHost() );
}


void LLToolGrab::stopSpin()
{
	mSpinGrabbing = FALSE;

	LLMessageSystem *msg = gMessageSystem;
	switch(mMode)
	{
	case GRAB_ACTIVE_CENTER:
	case GRAB_NONPHYSICAL:
	case GRAB_LOCKED:
		msg->newMessageFast(_PREHASH_ObjectSpinStop);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addUUIDFast(_PREHASH_ObjectID, mGrabObject->getID() );
		msg->sendMessage( mGrabObject->getRegion()->getHost() );
		break;

	case GRAB_NOOBJECT:
	case GRAB_INACTIVE:
	default:
		// do nothing
		break;
	}
}


void LLToolGrab::startGrab(S32 x, S32 y)
{
	// Compute grab_offset in the OBJECT's root's coordinate frame
	// (sometimes root == object)
	LLViewerObject *root = (LLViewerObject *)mGrabObject->getRoot();

	// drag from center
	LLVector3d grab_start_global = root->getPositionGlobal();

	// Where the grab starts, relative to the center of the root object of the set.
	// JC - This code looks wonky, but I believe it does the right thing.
	// Otherwise, when you grab a linked object set, it "pops" on the start
	// of the drag.
	LLVector3d grab_offsetd = root->getPositionGlobal() - mGrabObject->getPositionGlobal();

	LLVector3 grab_offset;
	grab_offset.setVec(grab_offsetd);

	LLQuaternion rotation = root->getRotation();
	rotation.conjQuat();
	grab_offset = grab_offset * rotation;

	// This planar drag starts at the grab point
	mDragStartPointGlobal = grab_start_global;
	mDragStartFromCamera = grab_start_global - gAgent.getCameraPositionGlobal();

	LLMessageSystem	*msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ObjectGrab);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addU32Fast(_PREHASH_LocalID, mGrabObject->mLocalID);
	msg->addVector3Fast(_PREHASH_GrabOffset, grab_offset );
	msg->sendMessage( mGrabObject->getRegion()->getHost());

	mGrabOffsetFromCenterInitial = grab_offset;
	mGrabHiddenOffsetFromCamera = mDragStartFromCamera;

	mGrabTimer.reset();
}


BOOL LLToolGrab::handleHover(S32 x, S32 y, MASK mask)
{
	mLastMouseX = x;
	mLastMouseY = y;

	if (!gViewerWindow->getLeftMouseDown())
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLGRAB);
		setMouseCapture(FALSE);
		return TRUE;
	}

	// Do the right hover based on mode
	switch( mMode )
	{
	case GRAB_ACTIVE_CENTER:
	case GRAB_NONPHYSICAL:
		handleHoverActive( x, y, mask );	// cursor hidden
		break;

	case GRAB_INACTIVE:
		handleHoverInactive( x, y, mask );  // cursor set here
		break;

	case GRAB_NOOBJECT:
	case GRAB_LOCKED:
		handleHoverFailed( x, y, mask );
		break;

	}

	return TRUE;
}



		
// Dragging.
void LLToolGrab::handleHoverActive(S32 x, S32 y, MASK mask)
{
	llassert( hasMouseCapture() );
	llassert( mGrabObject );
	if (mGrabObject->isDead())
	{
		// Bail out of drag because object has been killed
		setMouseCapture(FALSE);
		return;
	}

	//--------------------------------------------------
	// Toggle spinning
	//--------------------------------------------------
	if (mSpinGrabbing && !(mask == MASK_SPIN) && !gGrabBtnSpin)
	{
		// user released ALT key, stop spinning
		stopSpin();
	}
	else if (!mSpinGrabbing && (mask == MASK_SPIN) )
	{
		// user pressed ALT key, start spinning
		startSpin();
	}

	//--------------------------------------------------
	// Toggle vertical dragging
	//--------------------------------------------------
	if (mVerticalDragging && !(mask == MASK_VERTICAL) && !gGrabBtnVertical)
	{
		// ...switch to horizontal dragging
		mVerticalDragging = FALSE;

		mDragStartPointGlobal = gViewerWindow->clickPointInWorldGlobal(x, y, mGrabObject);
		mDragStartFromCamera = mDragStartPointGlobal - gAgent.getCameraPositionGlobal();
	}
	else if (!mVerticalDragging && (mask == MASK_VERTICAL) )
	{
		// ...switch to vertical dragging
		mVerticalDragging = TRUE;

		mDragStartPointGlobal = gViewerWindow->clickPointInWorldGlobal(x, y, mGrabObject);
		mDragStartFromCamera = mDragStartPointGlobal - gAgent.getCameraPositionGlobal();
	}

	const F32 RADIANS_PER_PIXEL_X = 0.01f;
	const F32 RADIANS_PER_PIXEL_Y = 0.01f;

	const F32 SENSITIVITY_X = 0.0075f;
	const F32 SENSITIVITY_Y = 0.0075f;

	S32 dx = x - (gViewerWindow->getWindowWidth() / 2);
	S32 dy = y - (gViewerWindow->getWindowHeight() / 2);

	if (dx != 0 || dy != 0)
	{
		mAccumDeltaX += dx;
		mAccumDeltaY += dy;
		S32 dist_sq = mAccumDeltaX * mAccumDeltaX + mAccumDeltaY * mAccumDeltaY;
		if (dist_sq > SLOP_DIST_SQ)
		{
			mOutsideSlop = TRUE;
		}

		// mouse has moved outside center
		mHasMoved = TRUE;
		
		if (mSpinGrabbing)
		{
			//------------------------------------------------------
			// Handle spinning
			//------------------------------------------------------

			// x motion maps to rotation around vertical axis
			LLVector3 up(0.f, 0.f, 1.f);
			LLQuaternion rotation_around_vertical( dx*RADIANS_PER_PIXEL_X, up );

			// y motion maps to rotation around left axis
			const LLVector3 &agent_left = gCamera->getLeftAxis();
			LLQuaternion rotation_around_left( dy*RADIANS_PER_PIXEL_Y, agent_left );

			// compose with current rotation
			mSpinRotation = mSpinRotation * rotation_around_vertical;
			mSpinRotation = mSpinRotation * rotation_around_left;

			// TODO: Throttle these
			LLMessageSystem *msg = gMessageSystem;
			msg->newMessageFast(_PREHASH_ObjectSpinUpdate);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_ObjectData);
			msg->addUUIDFast(_PREHASH_ObjectID, mGrabObject->getID() );
			msg->addQuatFast(_PREHASH_Rotation, mSpinRotation );
			msg->sendMessage( mGrabObject->getRegion()->getHost() );
		}
		else
		{
			//------------------------------------------------------
			// Handle grabbing
			//------------------------------------------------------

			LLVector3d x_part;
			x_part.setVec(gCamera->getLeftAxis());
			x_part.mdV[VZ] = 0.0;
			x_part.normVec();

			LLVector3d y_part;
			if( mVerticalDragging )
			{
				y_part.setVec(gCamera->getUpAxis());
				// y_part.setVec(0.f, 0.f, 1.f);
			}
			else
			{
				// drag toward camera
				y_part = x_part % LLVector3d::z_axis;
				y_part.mdV[VZ] = 0.0;
				y_part.normVec();
			}

			mGrabHiddenOffsetFromCamera = mGrabHiddenOffsetFromCamera 
				+ (x_part * (-dx * SENSITIVITY_X)) 
				+ (y_part * ( dy * SENSITIVITY_Y));


			// Send the message to the viewer.
			F32 dt = mGrabTimer.getElapsedTimeAndResetF32();
			U32 dt_milliseconds = (U32) (1000.f * dt);

			// need to return offset from mGrabStartPoint
			LLVector3d grab_point_global;

			grab_point_global = gAgent.getCameraPositionGlobal() + mGrabHiddenOffsetFromCamera;

			/* Snap to grid disabled for grab tool - very confusing
			// Handle snapping to grid, but only when the tool is formally selected.
			BOOL snap_on = gSavedSettings.getBOOL("SnapEnabled");
			if (snap_on && !gGrabTransientTool)
			{
				F64	snap_size = gSavedSettings.getF32("GridResolution");
				U8 snap_dimensions = (mVerticalDragging ? 3 : 2);

				for (U8 i = 0; i < snap_dimensions; i++)
				{
					grab_point_global.mdV[i] += snap_size / 2;
					grab_point_global.mdV[i] -= fmod(grab_point_global.mdV[i], snap_size);
				}
			}
			*/

			// Don't let object centers go underground.
			F32 land_height = gWorldPointer->resolveLandHeightGlobal(grab_point_global);

			if (grab_point_global.mdV[VZ] < land_height)
			{
				grab_point_global.mdV[VZ] = land_height;
			}

			// For safety, cap heights where objects can be dragged
			if (grab_point_global.mdV[VZ] > MAX_OBJECT_Z)
			{
				grab_point_global.mdV[VZ] = MAX_OBJECT_Z;
			}

			grab_point_global = gWorldp->clipToVisibleRegions(mDragStartPointGlobal, grab_point_global);
			// propagate constrained grab point back to grab offset
			mGrabHiddenOffsetFromCamera = grab_point_global - gAgent.getCameraPositionGlobal();

			// Handle auto-rotation at screen edge.
			LLVector3 grab_pos_agent = gAgent.getPosAgentFromGlobal( grab_point_global );

			LLCoordGL grab_center_gl( gViewerWindow->getWindowWidth() / 2, gViewerWindow->getWindowHeight() / 2);
			gCamera->projectPosAgentToScreen(grab_pos_agent, grab_center_gl);

			const S32 ROTATE_H_MARGIN = gViewerWindow->getWindowWidth() / 20;
			const F32 ROTATE_ANGLE_PER_SECOND = 30.f * DEG_TO_RAD;
			const F32 rotate_angle = ROTATE_ANGLE_PER_SECOND / gFPSClamped;
			// ...build mode moves camera about focus point
			if (grab_center_gl.mX < ROTATE_H_MARGIN)
			{
				if (gAgent.getFocusOnAvatar())
				{
					gAgent.yaw(rotate_angle);
				}
				else
				{
					gAgent.cameraOrbitAround(rotate_angle);
				}
			}
			else if (grab_center_gl.mX > gViewerWindow->getWindowWidth() - ROTATE_H_MARGIN)
			{
				if (gAgent.getFocusOnAvatar())
				{
					gAgent.yaw(-rotate_angle);
				}
				else
				{
					gAgent.cameraOrbitAround(-rotate_angle);
				}
			}

			// Don't move above top of screen or below bottom
			if ((grab_center_gl.mY < gViewerWindow->getWindowHeight() - 6)
				&& (grab_center_gl.mY > 24))
			{
				// Transmit update to simulator
				LLVector3 grab_pos_region = mGrabObject->getRegion()->getPosRegionFromGlobal( grab_point_global );

				LLMessageSystem *msg = gMessageSystem;
				msg->newMessageFast(_PREHASH_ObjectGrabUpdate);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_ObjectData);
				msg->addUUIDFast(_PREHASH_ObjectID, mGrabObject->getID() );
				msg->addVector3Fast(_PREHASH_GrabOffsetInitial, mGrabOffsetFromCenterInitial );
				msg->addVector3Fast(_PREHASH_GrabPosition, grab_pos_region );
				msg->addU32Fast(_PREHASH_TimeSinceLast, dt_milliseconds );
				msg->sendMessage( mGrabObject->getRegion()->getHost() );
			}
		}

		gViewerWindow->moveCursorToCenter();

		gSelectMgr->updateSelectionCenter();

	}

	// once we've initiated a drag, lock the camera down
	if (mHasMoved)
	{
		if (!gAgent.cameraMouselook() && 
			!mGrabObject->isHUDAttachment() && 
			mGrabObject->getRoot() == gAgent.getAvatarObject()->getRoot())
		{
			// force focus to point in space where we were looking previously
			gAgent.setFocusGlobal(gAgent.calcFocusPositionTargetGlobal(), LLUUID::null);
			gAgent.setFocusOnAvatar(FALSE, ANIMATE);
		}
		else
		{
			gAgent.clearFocusObject();
		}
	}

	// HACK to avoid assert: error checking system makes sure that the cursor is set during every handleHover.  This is actually a no-op since the cursor is hidden.
	gViewerWindow->setCursor(UI_CURSOR_ARROW);  

	lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolGrab (active) [cursor hidden]" << llendl;		
}
 

// Not dragging.  Just showing affordances
void LLToolGrab::handleHoverInactive(S32 x, S32 y, MASK mask)
{
	const F32 ROTATE_ANGLE_PER_SECOND = 40.f * DEG_TO_RAD;
	const F32 rotate_angle = ROTATE_ANGLE_PER_SECOND / gFPSClamped;

	// Look for cursor against the edge of the screen
	// Only works in fullscreen
	if (gSavedSettings.getBOOL("FullScreen"))
	{
		if (gAgent.cameraThirdPerson() )
		{
			if (x == 0)
			{
				gAgent.yaw(rotate_angle);
				//gAgent.setControlFlags(AGENT_CONTROL_YAW_POS);
			}
			else if (x == (gViewerWindow->getWindowWidth() - 1) )
			{
				gAgent.yaw(-rotate_angle);
				//gAgent.setControlFlags(AGENT_CONTROL_YAW_NEG);
			}
		}
	}

	// JC - TODO - change cursor based on gGrabBtnVertical, gGrabBtnSpin
	lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolGrab (inactive-not over editable object)" << llendl;		
	gViewerWindow->setCursor(UI_CURSOR_TOOLGRAB);
}

// User is trying to do something that's not allowed.
void LLToolGrab::handleHoverFailed(S32 x, S32 y, MASK mask)
{
	if( GRAB_NOOBJECT == mMode )
	{
		gViewerWindow->setCursor(UI_CURSOR_NO);
		lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolGrab (not on object)" << llendl;		
	}
	else
	{
		S32 dist_sq = (x-mMouseDownX) * (x-mMouseDownX) + (y-mMouseDownY) * (y-mMouseDownY);
		if( mOutsideSlop || dist_sq > SLOP_DIST_SQ )
		{
			mOutsideSlop = TRUE;

			switch( mMode )
			{
			case GRAB_LOCKED:
				gViewerWindow->setCursor(UI_CURSOR_GRABLOCKED);
				lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolGrab (grab failed, no move permission)" << llendl;		
				break;

//  Non physical now handled by handleHoverActive - CRO				
//			case GRAB_NONPHYSICAL:
//				gViewerWindow->setCursor(UI_CURSOR_ARROW);
//				lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolGrab (grab failed, nonphysical)" << llendl;		
//				break;
			default:
				llassert(0);
			}
		}
		else
		{
			gViewerWindow->setCursor(UI_CURSOR_ARROW);
			lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolGrab (grab failed but within slop)" << llendl;		
		}
	}
}




BOOL LLToolGrab::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// call the base class to propogate info to sim
	LLTool::handleMouseUp(x, y, mask);

	if( hasMouseCapture() )
	{
		setMouseCapture( FALSE );
	}
	mMode = GRAB_INACTIVE;

	// HACK: Make some grabs temporary
	if (gGrabTransientTool)
	{
		gBasicToolset->selectTool( gGrabTransientTool );
		gGrabTransientTool = NULL;
	}

	//gAgent.setObjectTracking(gSavedSettings.getBOOL("TrackFocusObject"));

	return TRUE;
} 

void LLToolGrab::stopEditing()
{
	if( hasMouseCapture() )
	{
		setMouseCapture( FALSE );
	}
}

void LLToolGrab::onMouseCaptureLost()
{
	// First, fix cursor placement
	if( !gAgent.cameraMouselook() 
		&& (GRAB_ACTIVE_CENTER == mMode || GRAB_NONPHYSICAL == mMode))
	{
		llassert( mGrabObject ); 

		if (mGrabObject->isHUDAttachment())
		{
			// ...move cursor "naturally", as if it had moved when hidden
			S32 x = mMouseDownX + mAccumDeltaX;
			S32 y = mMouseDownY + mAccumDeltaY;
			LLUI::setCursorPositionScreen(x, y);
		}
		else if (mHasMoved)
		{
			// ...move cursor back to the center of the object
			LLVector3 grab_point_agent = mGrabObject->getRenderPosition();

			LLCoordGL gl_point;
			gCamera->projectPosAgentToScreen(grab_point_agent, gl_point);

			LLUI::setCursorPositionScreen(gl_point.mX, gl_point.mY);
		}
		else
		{
			// ...move cursor back to click position
			LLUI::setCursorPositionScreen(mMouseDownX, mMouseDownY);
		}

		gViewerWindow->showCursor();
	}

	stopGrab();
	stopSpin();
	mMode = GRAB_INACTIVE;

	mHideBuildHighlight = FALSE;

	mGrabObject = NULL;

	gSelectMgr->updateSelectionCenter();
	gAgent.setPointAt(POINTAT_TARGET_CLEAR);
	gAgent.setLookAt(LOOKAT_TARGET_CLEAR);

	dialog_refresh_all();
}


void LLToolGrab::stopGrab()
{
	// Next, send messages to simulator
	LLMessageSystem *msg = gMessageSystem;
	switch(mMode)
	{
	case GRAB_ACTIVE_CENTER:
	case GRAB_NONPHYSICAL:
	case GRAB_LOCKED:
		msg->newMessageFast(_PREHASH_ObjectDeGrab);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addU32Fast(_PREHASH_LocalID, mGrabObject->mLocalID);
		msg->sendMessage(mGrabObject->getRegion()->getHost());

		mVerticalDragging = FALSE;
		mGrabOffset.clearVec();
		break;

	case GRAB_NOOBJECT:
	case GRAB_INACTIVE:
	default:
		// do nothing
		break;
	}

	mHideBuildHighlight = FALSE;
}


void LLToolGrab::draw()
{ }

void LLToolGrab::render()
{ }

BOOL LLToolGrab::isEditing()
{
	// Can't just compare to null directly due to "smart" pointer.
	LLViewerObject *obj = mGrabObject;
	return (obj != NULL);
}

LLViewerObject* LLToolGrab::getEditingObject()
{
	return mGrabObject;
}


LLVector3d LLToolGrab::getEditingPointGlobal()
{
	return getGrabPointGlobal();
}

LLVector3d LLToolGrab::getGrabPointGlobal()
{
	switch(mMode)
	{
	case GRAB_ACTIVE_CENTER:
	case GRAB_NONPHYSICAL:
	case GRAB_LOCKED:
		return gAgent.getCameraPositionGlobal() + mGrabHiddenOffsetFromCamera;

	case GRAB_NOOBJECT:
	case GRAB_INACTIVE:
	default:
		return gAgent.getPositionGlobal();
	}
}
