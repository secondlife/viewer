/** 
 * @file lltoolgrab.cpp
 * @brief LLToolGrab class implementation
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
#include "llagentcamera.h"
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
#include "llviewerinput.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h" 
#include "llviewerregion.h"
#include "llvoavatarself.h"
#include "llworld.h"
#include "llmenugl.h"

const S32 SLOP_DIST_SQ = 4;

// Override modifier key behavior with these buttons
bool gGrabBtnVertical = false;
bool gGrabBtnSpin = false;
LLTool* gGrabTransientTool = NULL;
extern bool gDebugClicks;

//
// Methods
//
LLToolGrabBase::LLToolGrabBase( LLToolComposite* composite )
:	LLTool( std::string("Grab"), composite ),
	mMode( GRAB_INACTIVE ),
	mVerticalDragging( false ),
	mHitLand(false),
	mLastMouseX(0),
	mLastMouseY(0),
	mAccumDeltaX(0),
	mAccumDeltaY(0),	
	mHasMoved( false ),
	mOutsideSlop(false),
	mDeselectedThisClick(false),
	mLastFace(0),
	mSpinGrabbing( false ),
	mSpinRotation(),
	mClickedInMouselook( false ),
	mHideBuildHighlight(false)
{ }

LLToolGrabBase::~LLToolGrabBase()
{ }


// virtual
void LLToolGrabBase::handleSelect()
{
	if(gFloaterTools)
	{
		// viewer can crash during startup if we don't check.
		gFloaterTools->setStatusText("grab");
		// in case we start from tools floater, we count any selection as valid
		mValidSelection = gFloaterTools->getVisible();
	}
	gGrabBtnVertical = false;
	gGrabBtnSpin = false;
}

void LLToolGrabBase::handleDeselect()
{
	if( hasMouseCapture() )
	{
		setMouseCapture( false );
	}

	// Make sure that temporary(invalid) selection won't pass anywhere except pie tool.
	MASK override_mask = gKeyboard ? gKeyboard->currentMask(true) : 0;
	if (!mValidSelection && (override_mask != MASK_NONE || (gFloaterTools && gFloaterTools->getVisible())))
	{
		LLMenuGL::sMenuContainer->hideMenus();
		LLSelectMgr::getInstance()->validateSelection();
	}

}

bool LLToolGrabBase::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (gDebugClicks)
	{
		LL_INFOS() << "LLToolGrab handleDoubleClick (becoming mouseDown)" << LL_ENDL;
	}

	return false;
}

bool LLToolGrabBase::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (gDebugClicks)
	{
		LL_INFOS() << "LLToolGrab handleMouseDown" << LL_ENDL;
	}

	LLTool::handleMouseDown(x, y, mask);

	// leftButtonGrabbed() checks if controls are reserved by scripts, but does not take masks into account
	if (!gAgent.leftButtonGrabbed() || ((mask & DEFAULT_GRAB_MASK) != 0 && !gAgentCamera.cameraMouselook()))
	{
		// can grab transparent objects (how touch event propagates, scripters rely on this)
		gViewerWindow->pickAsync(x, y, mask, pickCallback, /*bool pick_transparent*/ true);
	}
	mClickedInMouselook = gAgentCamera.cameraMouselook();

    if (mClickedInMouselook && gViewerInput.isLMouseHandlingDefault(MODE_FIRST_PERSON))
    {
        // LLToolCompGun::handleMouseDown handles the event if ML controls are grabed,
        // but LLToolGrabBase is often the end point for mouselook clicks if ML controls
        // are not grabbed and LLToolGrabBase::handleMouseDown consumes the event,
        // so send clicks from here.
        // We are sending specifically CONTROL_LBUTTON_DOWN instead of _ML_ version.
        gAgent.setControlFlags(AGENT_CONTROL_LBUTTON_DOWN);

        // Todo: LLToolGrabBase probably shouldn't consume the event if there is nothing
        // to grab in Mouselook, it intercepts handling in scanMouse
    }
	return true;
}

void LLToolGrabBase::pickCallback(const LLPickInfo& pick_info)
{
	LLToolGrab::getInstance()->mGrabPick = pick_info;
	LLViewerObject	*objectp = pick_info.getObject();

	bool extend_select = (pick_info.mKeyMask & MASK_SHIFT);

	if (!extend_select && !LLSelectMgr::getInstance()->getSelection()->isEmpty())
	{
		LLSelectMgr::getInstance()->deselectAll();
		LLToolGrab::getInstance()->mDeselectedThisClick = true;
	}
	else
	{
		LLToolGrab::getInstance()->mDeselectedThisClick = false;
	}

	// if not over object, do nothing
	if (!objectp)
	{
		LLToolGrab::getInstance()->setMouseCapture(true);
		LLToolGrab::getInstance()->mMode = GRAB_NOOBJECT;
		LLToolGrab::getInstance()->mGrabPick.mObjectID.setNull();
	}
	else
	{
		LLToolGrab::getInstance()->handleObjectHit(LLToolGrab::getInstance()->mGrabPick);
	}
}

bool LLToolGrabBase::handleObjectHit(const LLPickInfo& info)
{
	mGrabPick = info;
	LLViewerObject* objectp = mGrabPick.getObject();

	if (gDebugClicks)
	{
		LL_INFOS() << "LLToolGrab handleObjectHit " << info.mMousePt.mX << "," << info.mMousePt.mY << LL_ENDL;
	}

	if (NULL == objectp) // unexpected
	{
		LL_WARNS() << "objectp was NULL; returning false" << LL_ENDL;
		return false;
	}

	if (objectp->isAvatar())
	{
		if (gGrabTransientTool)
		{
			gBasicToolset->selectTool( gGrabTransientTool );
			gGrabTransientTool = NULL;
		}
		return true;
	}

	setMouseCapture( true );

	// Grabs always start from the root
	// objectp = (LLViewerObject *)objectp->getRoot();

	LLViewerObject* parent = objectp->getRootEdit();
	bool script_touch = (objectp->flagHandleTouch()) || (parent && parent->flagHandleTouch());

	// Clicks on scripted or physical objects are temporary grabs, so
	// not "Build mode"
	mHideBuildHighlight = script_touch || objectp->flagUsePhysics();

	if (!objectp->flagUsePhysics())
	{
		if (script_touch)
		{
			mMode = GRAB_NONPHYSICAL;  // if it has a script, use the non-physical grab
		}
		else
		{
			// In mouselook, we shouldn't be able to grab non-physical, 
			// non-touchable objects.  If it has a touch handler, we
			// do grab it (so llDetectedGrab works), but movement is
			// blocked on the server side. JC
			if (gAgentCamera.cameraMouselook())
			{
				mMode = GRAB_LOCKED;
				gViewerWindow->hideCursor();
				gViewerWindow->moveCursorToCenter();
			}
			else if (objectp->permMove() && !objectp->isPermanentEnforced())
			{
				mMode = GRAB_ACTIVE_CENTER;
				gViewerWindow->hideCursor();
				gViewerWindow->moveCursorToCenter();
			}
			else
			{
				mMode = GRAB_LOCKED;
			}

			
		}
	}
	else if( objectp->flagCharacter() || !objectp->permMove() || objectp->isPermanentEnforced())
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

	mLastMouseX = gViewerWindow->getCurrentMouseX();
	mLastMouseY = gViewerWindow->getCurrentMouseY();
	mAccumDeltaX = 0;
	mAccumDeltaY = 0;
	mHasMoved = false;
	mOutsideSlop = false;

	mVerticalDragging = (info.mKeyMask == MASK_VERTICAL) || gGrabBtnVertical;

	startGrab();

	if ((info.mKeyMask == MASK_SPIN) || gGrabBtnSpin)
	{
		startSpin();
	}

	LLSelectMgr::getInstance()->updateSelectionCenter();		// update selection beam

	// update point at
	LLViewerObject *edit_object = info.getObject();
	if (edit_object && info.mPickType != LLPickInfo::PICK_FLORA)
	{
		LLVector3 local_edit_point = gAgent.getPosAgentFromGlobal(info.mPosGlobal);
		local_edit_point -= edit_object->getPositionAgent();
		local_edit_point = local_edit_point * ~edit_object->getRenderRotation();
		gAgentCamera.setPointAt(POINTAT_TARGET_GRAB, edit_object, local_edit_point );
		gAgentCamera.setLookAt(LOOKAT_TARGET_SELECT, edit_object, local_edit_point );
	}

	// on transient grabs (clicks on world objects), kill the grab immediately
	if (!gViewerWindow->getLeftMouseDown() 
		&& gGrabTransientTool 
		&& (mMode == GRAB_NONPHYSICAL || mMode == GRAB_LOCKED))
	{
		gBasicToolset->selectTool( gGrabTransientTool );
		gGrabTransientTool = NULL;
	}

	return true;
}


void LLToolGrabBase::startSpin()
{
	LLViewerObject* objectp = mGrabPick.getObject();
	if (!objectp)
	{
		return;
	}
	mSpinGrabbing = true;

	// Was saveSelectedObjectTransform()
	LLViewerObject *root = (LLViewerObject *)objectp->getRoot();
	mSpinRotation = root->getRotation();

	LLMessageSystem *msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ObjectSpinStart);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addUUIDFast(_PREHASH_ObjectID, mGrabPick.mObjectID );
	msg->sendMessage( objectp->getRegion()->getHost() );
}


void LLToolGrabBase::stopSpin()
{
	mSpinGrabbing = false;

	LLViewerObject* objectp = mGrabPick.getObject();
	if (!objectp)
	{
		return;
	}

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
		msg->addUUIDFast(_PREHASH_ObjectID, objectp->getID() );
		msg->sendMessage( objectp->getRegion()->getHost() );
		break;

	case GRAB_NOOBJECT:
	case GRAB_INACTIVE:
	default:
		// do nothing
		break;
	}
}


void LLToolGrabBase::startGrab()
{
	// Compute grab_offset in the OBJECT's root's coordinate frame
	// (sometimes root == object)
	LLViewerObject* objectp = mGrabPick.getObject();
	if (!objectp)
	{
		return;
	}

	LLViewerObject *root = (LLViewerObject *)objectp->getRoot();

	// drag from center
	LLVector3d grab_start_global = root->getPositionGlobal();

	// Where the grab starts, relative to the center of the root object of the set.
	// JC - This code looks wonky, but I believe it does the right thing.
	// Otherwise, when you grab a linked object set, it "pops" on the start
	// of the drag.
	LLVector3d grab_offsetd = root->getPositionGlobal() - objectp->getPositionGlobal();

	LLVector3 grab_offset;
	grab_offset.setVec(grab_offsetd);

	LLQuaternion rotation = root->getRotation();
	rotation.conjQuat();
	grab_offset = grab_offset * rotation;

	// This planar drag starts at the grab point
	mDragStartPointGlobal = grab_start_global;
	mDragStartFromCamera = grab_start_global - gAgentCamera.getCameraPositionGlobal();

	send_ObjectGrab_message(objectp, mGrabPick, grab_offset);

	mGrabOffsetFromCenterInitial = grab_offset;
	mGrabHiddenOffsetFromCamera = mDragStartFromCamera;

	mGrabTimer.reset();

	mLastUVCoords = mGrabPick.mUVCoords;
	mLastSTCoords = mGrabPick.mSTCoords;
	mLastFace = mGrabPick.mObjectFace;
	mLastIntersection = mGrabPick.mIntersection;
	mLastNormal = mGrabPick.mNormal;
	mLastBinormal = mGrabPick.mBinormal;
	mLastGrabPos = LLVector3(-1.f, -1.f, -1.f);
}


bool LLToolGrabBase::handleHover(S32 x, S32 y, MASK mask)
{
	if (!gViewerWindow->getLeftMouseDown())
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLGRAB);
		setMouseCapture(false);
		return true;
	}

	// Do the right hover based on mode
	switch( mMode )
	{
	case GRAB_ACTIVE_CENTER:
		handleHoverActive( x, y, mask );	// cursor hidden
		break;
		
	case GRAB_NONPHYSICAL:
		handleHoverNonPhysical(x, y, mask);
		break;

	case GRAB_INACTIVE:
		handleHoverInactive( x, y, mask );  // cursor set here
		break;

	case GRAB_NOOBJECT:
	case GRAB_LOCKED:
		handleHoverFailed( x, y, mask );
		break;

	}

	mLastMouseX = x;
	mLastMouseY = y;
	
	return true;
}

const F32 GRAB_SENSITIVITY_X = 0.0075f;
const F32 GRAB_SENSITIVITY_Y = 0.0075f;



		
// Dragging.
void LLToolGrabBase::handleHoverActive(S32 x, S32 y, MASK mask)
{
	LLViewerObject* objectp = mGrabPick.getObject();
	if (!objectp || !hasMouseCapture() ) return;
	if (objectp->isDead())
	{
		// Bail out of drag because object has been killed
		setMouseCapture(false);
		return;
	}

	//--------------------------------------------------
	// Determine target mode
	//--------------------------------------------------
	bool vertical_dragging = false;
	bool spin_grabbing = false;
	if ((mask == MASK_VERTICAL)
		|| (gGrabBtnVertical && (mask != MASK_SPIN)))
	{
		vertical_dragging = true;
	}
	else if ((mask == MASK_SPIN)
			|| (gGrabBtnSpin && (mask != MASK_VERTICAL)))
	{
		spin_grabbing = true;
	}

	//--------------------------------------------------
	// Toggle spinning
	//--------------------------------------------------
	if (mSpinGrabbing && !spin_grabbing)
	{
		// user released or switched mask key(s), stop spinning
		stopSpin();
	}
	else if (!mSpinGrabbing && spin_grabbing)
	{
		// user pressed mask key(s), start spinning
		startSpin();
	}
	mSpinGrabbing = spin_grabbing;

	//--------------------------------------------------
	// Toggle vertical dragging
	//--------------------------------------------------
	if (mVerticalDragging && !vertical_dragging)
	{
		// ...switch to horizontal dragging
		mDragStartPointGlobal = gViewerWindow->clickPointInWorldGlobal(x, y, objectp);
		mDragStartFromCamera = mDragStartPointGlobal - gAgentCamera.getCameraPositionGlobal();
	}
	else if (!mVerticalDragging && vertical_dragging)
	{
		// ...switch to vertical dragging
		mDragStartPointGlobal = gViewerWindow->clickPointInWorldGlobal(x, y, objectp);
		mDragStartFromCamera = mDragStartPointGlobal - gAgentCamera.getCameraPositionGlobal();
	}
	mVerticalDragging = vertical_dragging;

	const F32 RADIANS_PER_PIXEL_X = 0.01f;
	const F32 RADIANS_PER_PIXEL_Y = 0.01f;

    S32 dx = gViewerWindow->getCurrentMouseDX();
    S32 dy = gViewerWindow->getCurrentMouseDY();

	if (dx != 0 || dy != 0)
	{
		mAccumDeltaX += dx;
		mAccumDeltaY += dy;
		S32 dist_sq = mAccumDeltaX * mAccumDeltaX + mAccumDeltaY * mAccumDeltaY;
		if (dist_sq > SLOP_DIST_SQ)
		{
			mOutsideSlop = true;
		}

		// mouse has moved outside center
		mHasMoved = true;
		
		if (mSpinGrabbing)
		{
			//------------------------------------------------------
			// Handle spinning
			//------------------------------------------------------

			// x motion maps to rotation around vertical axis
			LLVector3 up(0.f, 0.f, 1.f);
			LLQuaternion rotation_around_vertical( dx*RADIANS_PER_PIXEL_X, up );

			// y motion maps to rotation around left axis
			const LLVector3 &agent_left = LLViewerCamera::getInstance()->getLeftAxis();
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
			msg->addUUIDFast(_PREHASH_ObjectID, objectp->getID() );
			msg->addQuatFast(_PREHASH_Rotation, mSpinRotation );
			msg->sendMessage( objectp->getRegion()->getHost() );
		}
		else
		{
			//------------------------------------------------------
			// Handle grabbing
			//------------------------------------------------------

			LLVector3d x_part;
			x_part.setVec(LLViewerCamera::getInstance()->getLeftAxis());
			x_part.mdV[VZ] = 0.0;
			x_part.normVec();

			LLVector3d y_part;
			if( mVerticalDragging )
			{
				y_part.setVec(LLViewerCamera::getInstance()->getUpAxis());
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
				+ (x_part * (-dx * GRAB_SENSITIVITY_X)) 
				+ (y_part * ( dy * GRAB_SENSITIVITY_Y));


			// Send the message to the viewer.
			F32 dt = mGrabTimer.getElapsedTimeAndResetF32();
			U32 dt_milliseconds = (U32) (1000.f * dt);

			// need to return offset from mGrabStartPoint
			LLVector3d grab_point_global;

			grab_point_global = gAgentCamera.getCameraPositionGlobal() + mGrabHiddenOffsetFromCamera;

			/* Snap to grid disabled for grab tool - very confusing
			// Handle snapping to grid, but only when the tool is formally selected.
			bool snap_on = gSavedSettings.getBOOL("SnapEnabled");
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
			F32 land_height = LLWorld::getInstance()->resolveLandHeightGlobal(grab_point_global);

			if (grab_point_global.mdV[VZ] < land_height)
			{
				grab_point_global.mdV[VZ] = land_height;
			}

			// For safety, cap heights where objects can be dragged
			if (grab_point_global.mdV[VZ] > MAX_OBJECT_Z)
			{
				grab_point_global.mdV[VZ] = MAX_OBJECT_Z;
			}

			grab_point_global = LLWorld::getInstance()->clipToVisibleRegions(mDragStartPointGlobal, grab_point_global);
			// propagate constrained grab point back to grab offset
			mGrabHiddenOffsetFromCamera = grab_point_global - gAgentCamera.getCameraPositionGlobal();

			// Handle auto-rotation at screen edge.
			LLVector3 grab_pos_agent = gAgent.getPosAgentFromGlobal( grab_point_global );

			LLCoordGL grab_center_gl( gViewerWindow->getWorldViewWidthScaled() / 2, gViewerWindow->getWorldViewHeightScaled() / 2);
			LLViewerCamera::getInstance()->projectPosAgentToScreen(grab_pos_agent, grab_center_gl);

			const S32 ROTATE_H_MARGIN = gViewerWindow->getWorldViewWidthScaled() / 20;
			const F32 ROTATE_ANGLE_PER_SECOND = 30.f * DEG_TO_RAD;
			const F32 rotate_angle = ROTATE_ANGLE_PER_SECOND / gFPSClamped;
			// ...build mode moves camera about focus point
			if (grab_center_gl.mX < ROTATE_H_MARGIN)
			{
				if (gAgentCamera.getFocusOnAvatar())
				{
					gAgent.yaw(rotate_angle);
				}
				else
				{
					gAgentCamera.cameraOrbitAround(rotate_angle);
				}
			}
			else if (grab_center_gl.mX > gViewerWindow->getWorldViewWidthScaled() - ROTATE_H_MARGIN)
			{
				if (gAgentCamera.getFocusOnAvatar())
				{
					gAgent.yaw(-rotate_angle);
				}
				else
				{
					gAgentCamera.cameraOrbitAround(-rotate_angle);
				}
			}

			// Don't move above top of screen or below bottom
			if ((grab_center_gl.mY < gViewerWindow->getWorldViewHeightScaled() - 6)
				&& (grab_center_gl.mY > 24))
			{
				// Transmit update to simulator
				LLVector3 grab_pos_region = objectp->getRegion()->getPosRegionFromGlobal( grab_point_global );

				LLMessageSystem *msg = gMessageSystem;
				msg->newMessageFast(_PREHASH_ObjectGrabUpdate);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_ObjectData);
				msg->addUUIDFast(_PREHASH_ObjectID, objectp->getID() );
				msg->addVector3Fast(_PREHASH_GrabOffsetInitial, mGrabOffsetFromCenterInitial );
				msg->addVector3Fast(_PREHASH_GrabPosition, grab_pos_region );
				msg->addU32Fast(_PREHASH_TimeSinceLast, dt_milliseconds );
				msg->nextBlock("SurfaceInfo");
				msg->addVector3("UVCoord", LLVector3(mGrabPick.mUVCoords));
				msg->addVector3("STCoord", LLVector3(mGrabPick.mSTCoords));
				msg->addS32Fast(_PREHASH_FaceIndex, mGrabPick.mObjectFace);
				msg->addVector3("Position", mGrabPick.mIntersection);
				msg->addVector3("Normal", mGrabPick.mNormal);
				msg->addVector3("Binormal", mGrabPick.mBinormal);

				msg->sendMessage( objectp->getRegion()->getHost() );
			}
		}

		gViewerWindow->moveCursorToCenter();

		LLSelectMgr::getInstance()->updateSelectionCenter();

	}

	// once we've initiated a drag, lock the camera down
	if (mHasMoved)
	{
		if (!gAgentCamera.cameraMouselook() && 
			!objectp->isHUDAttachment() && 
			objectp->getRoot() == gAgentAvatarp->getRoot())
		{
			// we are essentially editing object position
			if (!gSavedSettings.getBOOL("EditCameraMovement"))
			{
				// force focus to point in space where we were looking previously
				// Example of use: follow cam scripts shouldn't affect you when movng objects arouns
				gAgentCamera.setFocusGlobal(gAgentCamera.calcFocusPositionTargetGlobal(), LLUUID::null);
				gAgentCamera.setFocusOnAvatar(false, ANIMATE);
			}
		}
		else
		{
			gAgentCamera.clearFocusObject();
		}
	}

	// HACK to avoid assert: error checking system makes sure that the cursor is set during every handleHover.  This is actually a no-op since the cursor is hidden.
	gViewerWindow->setCursor(UI_CURSOR_ARROW);  

	LL_DEBUGS("UserInput") << "hover handled by LLToolGrab (active) [cursor hidden]" << LL_ENDL;		
}
 

void LLToolGrabBase::handleHoverNonPhysical(S32 x, S32 y, MASK mask)
{
	LLViewerObject* objectp = mGrabPick.getObject();
	if (!objectp || !hasMouseCapture() ) return;
	if (objectp->isDead())
	{
		// Bail out of drag because object has been killed
		setMouseCapture(false);
		return;
	}

	LLPickInfo pick = mGrabPick;
	pick.mMousePt = LLCoordGL(x, y);
	pick.getSurfaceInfo();

	// compute elapsed time
	F32 dt = mGrabTimer.getElapsedTimeAndResetF32();
	U32 dt_milliseconds = (U32) (1000.f * dt);

	// i'm not a big fan of the following code - it's been culled from the physical grab case.
	// ideally these two would be nicely integrated - but the code in that method is a serious
	// mess of spaghetti.  so here we go:

	LLVector3 grab_pos_region(0,0,0);
	
	const bool SUPPORT_LLDETECTED_GRAB = true;
	if (SUPPORT_LLDETECTED_GRAB)
	{
		//--------------------------------------------------
		// Toggle vertical dragging
		//--------------------------------------------------
		if (!(mask == MASK_VERTICAL) && !gGrabBtnVertical)
		{
			mVerticalDragging = false;
		}
	
		else if ((gGrabBtnVertical && (mask != MASK_SPIN)) 
				|| (mask == MASK_VERTICAL))
		{
			mVerticalDragging = true;
		}
	
		S32 dx = x - mLastMouseX;
		S32 dy = y - mLastMouseY;

		if (dx != 0 || dy != 0)
		{
			mAccumDeltaX += dx;
			mAccumDeltaY += dy;
		
			S32 dist_sq = mAccumDeltaX * mAccumDeltaX + mAccumDeltaY * mAccumDeltaY;
			if (dist_sq > SLOP_DIST_SQ)
			{
				mOutsideSlop = true;
			}

			// mouse has moved 
			mHasMoved = true;

			//------------------------------------------------------
			// Handle grabbing
			//------------------------------------------------------

			LLVector3d x_part;
			x_part.setVec(LLViewerCamera::getInstance()->getLeftAxis());
			x_part.mdV[VZ] = 0.0;
			x_part.normVec();

			LLVector3d y_part;
			if( mVerticalDragging )
			{
				y_part.setVec(LLViewerCamera::getInstance()->getUpAxis());
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
				+ (x_part * (-dx * GRAB_SENSITIVITY_X)) 
				+ (y_part * ( dy * GRAB_SENSITIVITY_Y));

		}
		
		// need to return offset from mGrabStartPoint
		LLVector3d grab_point_global = gAgentCamera.getCameraPositionGlobal() + mGrabHiddenOffsetFromCamera;
		grab_pos_region = objectp->getRegion()->getPosRegionFromGlobal( grab_point_global );
	}


	// only send message if something has changed since last message
	
	bool changed_since_last_update = false;

	// test if touch data needs to be updated
	if ((pick.mObjectFace != mLastFace) ||
		(pick.mUVCoords != mLastUVCoords) ||
		(pick.mSTCoords != mLastSTCoords) ||
		(pick.mIntersection != mLastIntersection) ||
		(pick.mNormal != mLastNormal) ||
		(pick.mBinormal != mLastBinormal) ||
		(grab_pos_region != mLastGrabPos))
	{
		changed_since_last_update = true;
	}

	if (changed_since_last_update)
	{
		LLMessageSystem *msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ObjectGrabUpdate);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addUUIDFast(_PREHASH_ObjectID, objectp->getID() );
		msg->addVector3Fast(_PREHASH_GrabOffsetInitial, mGrabOffsetFromCenterInitial );
		msg->addVector3Fast(_PREHASH_GrabPosition, grab_pos_region );
		msg->addU32Fast(_PREHASH_TimeSinceLast, dt_milliseconds );
		msg->nextBlock("SurfaceInfo");
		msg->addVector3("UVCoord", LLVector3(pick.mUVCoords));
		msg->addVector3("STCoord", LLVector3(pick.mSTCoords));
		msg->addS32Fast(_PREHASH_FaceIndex, pick.mObjectFace);
		msg->addVector3("Position", pick.mIntersection);
		msg->addVector3("Normal", pick.mNormal);
		msg->addVector3("Binormal", pick.mBinormal);

		msg->sendMessage( objectp->getRegion()->getHost() );

		mLastUVCoords = pick.mUVCoords;
		mLastSTCoords = pick.mSTCoords;
		mLastFace = pick.mObjectFace;
		mLastIntersection = pick.mIntersection;
		mLastNormal= pick.mNormal;
		mLastBinormal= pick.mBinormal;
		mLastGrabPos = grab_pos_region;
	}
	
	// update point-at / look-at
	if (pick.mObjectFace != -1) // if the intersection was on the surface of the obejct
	{
		LLVector3 local_edit_point = pick.mIntersection;
		local_edit_point -= objectp->getPositionAgent();
		local_edit_point = local_edit_point * ~objectp->getRenderRotation();
		gAgentCamera.setPointAt(POINTAT_TARGET_GRAB, objectp, local_edit_point );
		gAgentCamera.setLookAt(LOOKAT_TARGET_SELECT, objectp, local_edit_point );
	}
	
	
	
	gViewerWindow->setCursor(UI_CURSOR_HAND);  
}
 

// Not dragging.  Just showing affordances
void LLToolGrabBase::handleHoverInactive(S32 x, S32 y, MASK mask)
{
	// JC - TODO - change cursor based on gGrabBtnVertical, gGrabBtnSpin
	LL_DEBUGS("UserInput") << "hover handled by LLToolGrab (inactive-not over editable object)" << LL_ENDL;		
	gViewerWindow->setCursor(UI_CURSOR_TOOLGRAB);
}

// User is trying to do something that's not allowed.
void LLToolGrabBase::handleHoverFailed(S32 x, S32 y, MASK mask)
{
	if( GRAB_NOOBJECT == mMode )
	{
		gViewerWindow->setCursor(UI_CURSOR_NO);
		LL_DEBUGS("UserInput") << "hover handled by LLToolGrab (not on object)" << LL_ENDL;		
	}
	else
	{
		S32 dist_sq = (x-mGrabPick.mMousePt.mX) * (x-mGrabPick.mMousePt.mX) + (y-mGrabPick.mMousePt.mY) * (y-mGrabPick.mMousePt.mY);
		if( mOutsideSlop || dist_sq > SLOP_DIST_SQ )
		{
			mOutsideSlop = true;

			switch( mMode )
			{
			case GRAB_LOCKED:
				gViewerWindow->setCursor(UI_CURSOR_GRABLOCKED);
				LL_DEBUGS("UserInput") << "hover handled by LLToolGrab (grab failed, no move permission)" << LL_ENDL;		
				break;

//  Non physical now handled by handleHoverActive - CRO				
//			case GRAB_NONPHYSICAL:
//				gViewerWindow->setCursor(UI_CURSOR_ARROW);
//				LL_DEBUGS("UserInput") << "hover handled by LLToolGrab (grab failed, nonphysical)" << LL_ENDL;		
//				break;
			default:
				llassert(0);
			}
		}
		else
		{
			gViewerWindow->setCursor(UI_CURSOR_ARROW);
			LL_DEBUGS("UserInput") << "hover handled by LLToolGrab (grab failed but within slop)" << LL_ENDL;		
		}
	}
}




bool LLToolGrabBase::handleMouseUp(S32 x, S32 y, MASK mask)
{
	LLTool::handleMouseUp(x, y, mask);

    if (gAgentCamera.cameraMouselook() && gViewerInput.isLMouseHandlingDefault(MODE_FIRST_PERSON))
    {
        // LLToolCompGun::handleMouseUp handles the event if ML controls are grabed,
        // but LLToolGrabBase is often the end point for mouselook clicks if ML controls
        // are not grabbed and LToolGrabBase::handleMouseUp consumes the event,
        // so send clicks from here.
        // We are sending specifically CONTROL_LBUTTON_UP instead of _ML_ version.
        gAgent.setControlFlags(AGENT_CONTROL_LBUTTON_UP);
    }

	if( hasMouseCapture() )
	{
		setMouseCapture( false );
	}

	mMode = GRAB_INACTIVE;

	if(mClickedInMouselook && !gAgentCamera.cameraMouselook())
	{
		mClickedInMouselook = false;
	}
	else
	{
		// HACK: Make some grabs temporary
		if (gGrabTransientTool)
		{
			gBasicToolset->selectTool( gGrabTransientTool );
			gGrabTransientTool = NULL;
		}
	}

	//gAgent.setObjectTracking(gSavedSettings.getBOOL("TrackFocusObject"));

	return true;
} 

void LLToolGrabBase::stopEditing()
{
	if( hasMouseCapture() )
	{
		setMouseCapture( false );
	}
}

void LLToolGrabBase::onMouseCaptureLost()
{
	LLViewerObject* objectp = mGrabPick.getObject();
	if (!objectp)
	{
		gViewerWindow->showCursor();
		return;
	}
	// First, fix cursor placement
	if( !gAgentCamera.cameraMouselook() 
		&& (GRAB_ACTIVE_CENTER == mMode))
	{
		if (objectp->isHUDAttachment())
		{
			// ...move cursor "naturally", as if it had moved when hidden
			S32 x = mGrabPick.mMousePt.mX + mAccumDeltaX;
			S32 y = mGrabPick.mMousePt.mY + mAccumDeltaY;
			LLUI::getInstance()->setMousePositionScreen(x, y);
		}
		else if (mHasMoved)
		{
			// ...move cursor back to the center of the object
			LLVector3 grab_point_agent = objectp->getRenderPosition();

			LLCoordGL gl_point;
			if (LLViewerCamera::getInstance()->projectPosAgentToScreen(grab_point_agent, gl_point))
			{
				LLUI::getInstance()->setMousePositionScreen(gl_point.mX, gl_point.mY);
			}
		}
		else
		{
			// ...move cursor back to click position
			LLUI::getInstance()->setMousePositionScreen(mGrabPick.mMousePt.mX, mGrabPick.mMousePt.mY);
		}

		gViewerWindow->showCursor();
	}

	stopGrab();
	if (mSpinGrabbing)
	stopSpin();
	
	mMode = GRAB_INACTIVE;

	mHideBuildHighlight = false;

	mGrabPick.mObjectID.setNull();

	LLSelectMgr::getInstance()->updateSelectionCenter();
	gAgentCamera.setPointAt(POINTAT_TARGET_CLEAR);
	gAgentCamera.setLookAt(LOOKAT_TARGET_CLEAR);

	dialog_refresh_all();
}


void LLToolGrabBase::stopGrab()
{
	LLViewerObject* objectp = mGrabPick.getObject();
	if (!objectp)
	{
		return;
	}

	LLPickInfo pick = mGrabPick;

	if (mMode == GRAB_NONPHYSICAL)
	{
		// for non-physical (touch) grabs,
		// gather surface info for this degrab (mouse-up)
		S32 x = gViewerWindow->getCurrentMouseX();
		S32 y = gViewerWindow->getCurrentMouseY();
		pick.mMousePt = LLCoordGL(x, y);
		pick.getSurfaceInfo();
	}

	// Next, send messages to simulator
	switch(mMode)
	{
	case GRAB_ACTIVE_CENTER:
	case GRAB_NONPHYSICAL:
	case GRAB_LOCKED:
		send_ObjectDeGrab_message(objectp, pick);
		mVerticalDragging = false;
		break;

	case GRAB_NOOBJECT:
	case GRAB_INACTIVE:
	default:
		// do nothing
		break;
	}

	mHideBuildHighlight = false;
}


void LLToolGrabBase::draw()
{ }

void LLToolGrabBase::render()
{ }

bool LLToolGrabBase::isEditing()
{
	return (mGrabPick.getObject().notNull());
}

LLViewerObject* LLToolGrabBase::getEditingObject()
{
	return mGrabPick.getObject();
}


LLVector3d LLToolGrabBase::getEditingPointGlobal()
{
	return getGrabPointGlobal();
}

LLVector3d LLToolGrabBase::getGrabPointGlobal()
{
	switch(mMode)
	{
	case GRAB_ACTIVE_CENTER:
	case GRAB_NONPHYSICAL:
	case GRAB_LOCKED:
		return gAgentCamera.getCameraPositionGlobal() + mGrabHiddenOffsetFromCamera;

	case GRAB_NOOBJECT:
	case GRAB_INACTIVE:
	default:
		return gAgent.getPositionGlobal();
	}
}


void send_ObjectGrab_message(LLViewerObject* object, const LLPickInfo & pick, const LLVector3 &grab_offset)
{
	if (!object) return;

	LLMessageSystem	*msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_ObjectGrab);
	msg->nextBlockFast( _PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast( _PREHASH_ObjectData);
	msg->addU32Fast(    _PREHASH_LocalID, object->mLocalID);
	msg->addVector3Fast(_PREHASH_GrabOffset, grab_offset);
	msg->nextBlock("SurfaceInfo");
	msg->addVector3("UVCoord", LLVector3(pick.mUVCoords));
	msg->addVector3("STCoord", LLVector3(pick.mSTCoords));
	msg->addS32Fast(_PREHASH_FaceIndex, pick.mObjectFace);
	msg->addVector3("Position", pick.mIntersection);
	msg->addVector3("Normal", pick.mNormal);
	msg->addVector3("Binormal", pick.mBinormal);
	msg->sendMessage( object->getRegion()->getHost());

	/*  Diagnostic code
	LL_INFOS() << "mUVCoords: " << pick.mUVCoords
			<< ", mSTCoords: " << pick.mSTCoords
			<< ", mObjectFace: " << pick.mObjectFace
			<< ", mIntersection: " << pick.mIntersection
			<< ", mNormal: " << pick.mNormal
			<< ", mBinormal: " << pick.mBinormal
			<< LL_ENDL;

	LL_INFOS() << "Avatar pos: " << gAgent.getPositionAgent() << LL_ENDL;
	LL_INFOS() << "Object pos: " << object->getPosition() << LL_ENDL;
	*/
}


void send_ObjectDeGrab_message(LLViewerObject* object, const LLPickInfo & pick)
{
	if (!object) return;

	LLMessageSystem	*msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_ObjectDeGrab);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addU32Fast(_PREHASH_LocalID, object->mLocalID);
	msg->nextBlock("SurfaceInfo");
	msg->addVector3("UVCoord", LLVector3(pick.mUVCoords));
	msg->addVector3("STCoord", LLVector3(pick.mSTCoords));
	msg->addS32Fast(_PREHASH_FaceIndex, pick.mObjectFace);
	msg->addVector3("Position", pick.mIntersection);
	msg->addVector3("Normal", pick.mNormal);
	msg->addVector3("Binormal", pick.mBinormal);
	msg->sendMessage(object->getRegion()->getHost());
}



