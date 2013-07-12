/** 
 * @file llmaniptranslate.cpp
 * @brief LLManipTranslate class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

/**
 * Positioning tool
 */

#include "llviewerprecompiledheaders.h"

#include "llmaniptranslate.h"

#include "llgl.h"
#include "llrender.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llbbox.h"
#include "llbox.h"
#include "llviewercontrol.h"
#include "llcriticaldamp.h"
#include "llcylinder.h"
#include "lldrawable.h"
#include "llfloatertools.h"
#include "llfontgl.h"
#include "llglheaders.h"
#include "llhudrender.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llrendersphere.h"
#include "llstatusbar.h"
#include "lltoolmgr.h"
#include "llviewercamera.h"
#include "llviewerjoint.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llworld.h"
#include "llui.h"
#include "pipeline.h"
#include "llviewershadermgr.h"

const S32 NUM_AXES = 3;
const S32 MOUSE_DRAG_SLOP = 2;       // pixels
const F32 HANDLE_HIDE_ANGLE = 0.15f; // radians
const F32 SELECTED_ARROW_SCALE = 1.3f;
const F32 MANIPULATOR_HOTSPOT_START = 0.2f;
const F32 MANIPULATOR_HOTSPOT_END = 1.2f;
const F32 SNAP_GUIDE_SCREEN_SIZE = 0.7f;
const F32 MIN_PLANE_MANIP_DOT_PRODUCT = 0.25f;
const F32 PLANE_TICK_SIZE = 0.4f;
const F32 MANIPULATOR_SCALE_HALF_LIFE = 0.07f;
const F32 SNAP_ARROW_SCALE = 0.7f;

static LLPointer<LLViewerTexture> sGridTex = NULL ;

const LLManip::EManipPart MANIPULATOR_IDS[9] = 
{
	LLManip::LL_X_ARROW,
	LLManip::LL_Y_ARROW,
	LLManip::LL_Z_ARROW,
	LLManip::LL_X_ARROW,
	LLManip::LL_Y_ARROW,
	LLManip::LL_Z_ARROW,
	LLManip::LL_YZ_PLANE,
	LLManip::LL_XZ_PLANE,
	LLManip::LL_XY_PLANE
};

const U32 ARROW_TO_AXIS[4] = 
{
	VX,
	VX,
	VY,
	VZ
};

// Sort manipulator handles by their screen-space projection
struct ClosestToCamera
{
	bool operator()(const LLManipTranslate::ManipulatorHandle& a,
					const LLManipTranslate::ManipulatorHandle& b) const
	{
		return a.mEndPosition.mV[VZ] < b.mEndPosition.mV[VZ];
	}
};

LLManipTranslate::LLManipTranslate( LLToolComposite* composite )
:	LLManip( std::string("Move"), composite ),
	mLastHoverMouseX(-1),
	mLastHoverMouseY(-1),
	mSendUpdateOnMouseUp(FALSE),
	mMouseOutsideSlop(FALSE),
	mCopyMadeThisDrag(FALSE),
	mMouseDownX(-1),
	mMouseDownY(-1),
	mAxisArrowLength(50),
	mConeSize(0),
	mArrowLengthMeters(0.f),
	mGridSizeMeters(1.f),
	mPlaneManipOffsetMeters(0.f),
	mUpdateTimer(),
	mSnapOffsetMeters(0.f),
	mSubdivisions(10.f),
	mInSnapRegime(FALSE),
	mSnapped(FALSE),
	mArrowScales(1.f, 1.f, 1.f),
	mPlaneScales(1.f, 1.f, 1.f),
	mPlaneManipPositions(1.f, 1.f, 1.f, 1.f)
{ 
	if (sGridTex.isNull())
	{ 
		restoreGL();
	}
}

//static
U32 LLManipTranslate::getGridTexName()
{
	if(sGridTex.isNull())
	{
		restoreGL() ;
	}

	return sGridTex.isNull() ? 0 : sGridTex->getTexName() ;
}

//static
void LLManipTranslate::destroyGL()
{
	if (sGridTex)
	{
		sGridTex = NULL ;
	}
}

//static
void LLManipTranslate::restoreGL()
{
	//generate grid texture
	U32 rez = 512;
	U32 mip = 0;

	destroyGL() ;
	sGridTex = LLViewerTextureManager::getLocalTexture() ;
	if(!sGridTex->createGLTexture())
	{
		sGridTex = NULL ;
		return ;
	}

	GLuint* d = new GLuint[rez*rez];	

	gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, sGridTex->getTexName(), true);
	gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_TRILINEAR);

	while (rez >= 1)
	{
		for (U32 i = 0; i < rez*rez; i++)
		{
			d[i] = 0x00FFFFFF;
		}
		
		U32 subcol = 0xFFFFFFFF;
		if (rez >= 4)
		{	//large grain grid
			for (U32 i = 0; i < rez; i++)
			{
				if (rez <= 16)
				{
					if (rez == 16)
					{
						subcol = 0xA0FFFFFF;
					}
					else if (rez == 8)
					{
						subcol = 0x80FFFFFF;
					}
					else
					{
						subcol = 0x40FFFFFF;
					}
				}
				else
				{
					subcol = 0xFFFFFFFF;	
				}
				d[i			*rez+ 0		 ] = subcol;
				d[0			*rez+ i		 ] = subcol;				
				if (rez >= 32)
				{
					d[i			*rez+ (rez-1)] = subcol;
					d[(rez-1)	*rez+ i		 ] = subcol;
				}

				if (rez >= 64)
				{
					subcol = 0xFFFFFFFF;
					
					if (i > 0 && i < (rez-1))
					{
						d[i			*rez+ 1		 ] = subcol;
						d[i			*rez+ (rez-2)] = subcol;
						d[1			*rez+ i		 ] = subcol;
						d[(rez-2)	*rez+ i		 ] = subcol;
					}
				}
			}
		}

		subcol = 0x50A0A0A0;
		if (rez >= 128)
		{ //small grain grid
			for (U32 i = 8; i < rez; i+=8)
			{
				for (U32 j = 2; j < rez-2; j++)
				{
					d[i	*rez+ j] = subcol;
					d[j	*rez+ i] = subcol;			
				}
			}
		}
		if (rez >= 64)
		{ //medium grain grid
			if (rez == 64)
			{
				subcol = 0x50A0A0A0;
			}
			else
			{
				subcol = 0xA0D0D0D0;
			}

			for (U32 i = 32; i < rez; i+=32)
			{
				U32 pi = i-1;
				for (U32 j = 2; j < rez-2; j++)
				{
					d[i		*rez+ j] = subcol;
					d[j		*rez+ i] = subcol;

					if (rez > 128)
					{
						d[pi	*rez+ j] = subcol;
						d[j		*rez+ pi] = subcol;			
					}
				}
			}
		}
#ifdef LL_WINDOWS
		LLImageGL::setManualImage(GL_TEXTURE_2D, mip, GL_RGBA, rez, rez, GL_RGBA, GL_UNSIGNED_BYTE, d);
#else
		LLImageGL::setManualImage(GL_TEXTURE_2D, mip, GL_RGBA, rez, rez, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, d);
#endif
		rez = rez >> 1;
		mip++;
	}
	delete [] d;
}


LLManipTranslate::~LLManipTranslate()
{
}


void LLManipTranslate::handleSelect()
{
	LLSelectMgr::getInstance()->saveSelectedObjectTransform(SELECT_ACTION_TYPE_PICK);
	gFloaterTools->setStatusText("move");
	LLManip::handleSelect();
}

BOOL LLManipTranslate::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// didn't click in any UI object, so must have clicked in the world
	if( (mHighlightedPart == LL_X_ARROW ||
		 mHighlightedPart == LL_Y_ARROW ||
		 mHighlightedPart == LL_Z_ARROW ||
		 mHighlightedPart == LL_YZ_PLANE ||
		 mHighlightedPart == LL_XZ_PLANE ||
		 mHighlightedPart == LL_XY_PLANE ) )
	{
		handled = handleMouseDownOnPart( x, y, mask );
	}

	return handled;
}

// Assumes that one of the arrows on an object was hit.
BOOL LLManipTranslate::handleMouseDownOnPart( S32 x, S32 y, MASK mask )
{
	BOOL can_move = canAffectSelection();
	if (!can_move)
	{
		return FALSE;
	}

	highlightManipulators(x, y);
	S32 hit_part = mHighlightedPart;

	if( (hit_part != LL_X_ARROW) && 
		(hit_part != LL_Y_ARROW) &&
		(hit_part != LL_Z_ARROW) &&
		(hit_part != LL_YZ_PLANE) &&
		(hit_part != LL_XZ_PLANE) &&
		(hit_part != LL_XY_PLANE) )
	{
		return TRUE;
	}

	mHelpTextTimer.reset();
	sNumTimesHelpTextShown++;

	LLSelectMgr::getInstance()->getGrid(mGridOrigin, mGridRotation, mGridScale);

	LLSelectMgr::getInstance()->enableSilhouette(FALSE);

	// we just started a drag, so save initial object positions
	LLSelectMgr::getInstance()->saveSelectedObjectTransform(SELECT_ACTION_TYPE_MOVE);

	mManipPart = (EManipPart)hit_part;
	mMouseDownX = x;
	mMouseDownY = y;
	mMouseOutsideSlop = FALSE;

	LLVector3		axis;

	LLSelectNode *selectNode = mObjectSelection->getFirstMoveableNode(TRUE);

	if (!selectNode)
	{
		// didn't find the object in our selection...oh well
		llwarns << "Trying to translate an unselected object" << llendl;
		return TRUE;
	}

	LLViewerObject *selected_object = selectNode->getObject();
	if (!selected_object)
	{
		// somehow we lost the object!
		llwarns << "Translate manip lost the object, no selected object" << llendl;
		gViewerWindow->setCursor(UI_CURSOR_TOOLTRANSLATE);
		return TRUE;
	}

	// Compute unit vectors for arrow hit and a plane through that vector
	BOOL axis_exists = getManipAxis(selected_object, mManipPart, axis);
	getManipNormal(selected_object, mManipPart, mManipNormal);

	//LLVector3 select_center_agent = gAgent.getPosAgentFromGlobal(LLSelectMgr::getInstance()->getSelectionCenterGlobal());
	// TomY: The above should (?) be identical to the below
	LLVector3 select_center_agent = getPivotPoint();
	mSubdivisions = llclamp(getSubdivisionLevel(select_center_agent, axis_exists ? axis : LLVector3::z_axis, getMinGridScale()), sGridMinSubdivisionLevel, sGridMaxSubdivisionLevel);

	// if we clicked on a planar manipulator, recenter mouse cursor
	if (mManipPart >= LL_YZ_PLANE && mManipPart <= LL_XY_PLANE)
	{
		LLCoordGL mouse_pos;
		if (!LLViewerCamera::getInstance()->projectPosAgentToScreen(select_center_agent, mouse_pos))
		{
			// mouse_pos may be nonsense
			llwarns << "Failed to project object center to screen" << llendl;
		}
		else if (gSavedSettings.getBOOL("SnapToMouseCursor"))
		{
			LLUI::setMousePositionScreen(mouse_pos.mX, mouse_pos.mY);
			x = mouse_pos.mX;
			y = mouse_pos.mY;
		}
	}

	LLSelectMgr::getInstance()->updateSelectionCenter();
	LLVector3d object_start_global = gAgent.getPosGlobalFromAgent(getPivotPoint());
	getMousePointOnPlaneGlobal(mDragCursorStartGlobal, x, y, object_start_global, mManipNormal);
	mDragSelectionStartGlobal = object_start_global;
	mCopyMadeThisDrag = FALSE;

	// Route future Mouse messages here preemptively.  (Release on mouse up.)
	setMouseCapture( TRUE );

	return TRUE;
}

BOOL LLManipTranslate::handleHover(S32 x, S32 y, MASK mask)
{
	// Translation tool only works if mouse button is down.
	// Bail out if mouse not down.
	if( !hasMouseCapture() )
	{
		lldebugst(LLERR_USER_INPUT) << "hover handled by LLManipTranslate (inactive)" << llendl;		
		// Always show cursor
		// gViewerWindow->setCursor(UI_CURSOR_ARROW);
		gViewerWindow->setCursor(UI_CURSOR_TOOLTRANSLATE);

		highlightManipulators(x, y);
		return TRUE;
	}
	
	// Handle auto-rotation if necessary.
	LLRect world_rect = gViewerWindow->getWorldViewRectScaled();
	const F32 ROTATE_ANGLE_PER_SECOND = 30.f * DEG_TO_RAD;
	const S32 ROTATE_H_MARGIN = world_rect.getWidth() / 20;
	const F32 rotate_angle = ROTATE_ANGLE_PER_SECOND / gFPSClamped;
	BOOL rotated = FALSE;

	// ...build mode moves camera about focus point
	if (mObjectSelection->getSelectType() != SELECT_TYPE_HUD)
	{
		if (x < ROTATE_H_MARGIN)
		{
			gAgentCamera.cameraOrbitAround(rotate_angle);
			rotated = TRUE;
		}
		else if (x > world_rect.getWidth() - ROTATE_H_MARGIN)
		{
			gAgentCamera.cameraOrbitAround(-rotate_angle);
			rotated = TRUE;
		}
	}

	// Suppress processing if mouse hasn't actually moved.
	// This may cause problems if the camera moves outside of the
	// rotation above.
	if( x == mLastHoverMouseX && y == mLastHoverMouseY && !rotated)
	{
		lldebugst(LLERR_USER_INPUT) << "hover handled by LLManipTranslate (mouse unmoved)" << llendl;
		gViewerWindow->setCursor(UI_CURSOR_TOOLTRANSLATE);
		return TRUE;
	}
	mLastHoverMouseX = x;
	mLastHoverMouseY = y;

	// Suppress if mouse hasn't moved past the initial slop region
	// Reset once we start moving
	if( !mMouseOutsideSlop )
	{
		if (abs(mMouseDownX - x) < MOUSE_DRAG_SLOP && abs(mMouseDownY - y) < MOUSE_DRAG_SLOP )
		{
			lldebugst(LLERR_USER_INPUT) << "hover handled by LLManipTranslate (mouse inside slop)" << llendl;
			gViewerWindow->setCursor(UI_CURSOR_TOOLTRANSLATE);
			return TRUE;
		}
		else
		{
			// ...just went outside the slop region
			mMouseOutsideSlop = TRUE;
			// If holding down shift, leave behind a copy.
			if (mask == MASK_COPY)
			{
				// ...we're trying to make a copy
				LLSelectMgr::getInstance()->selectDuplicate(LLVector3::zero, FALSE);
				mCopyMadeThisDrag = TRUE;

				// When we make the copy, we don't want to do any other processing.
				// If so, the object will also be moved, and the copy will be offset.
				lldebugst(LLERR_USER_INPUT) << "hover handled by LLManipTranslate (made copy)" << llendl;
				gViewerWindow->setCursor(UI_CURSOR_TOOLTRANSLATE);
			}
		}
	}

	// Throttle updates to 10 per second.

	LLVector3		axis_f;
	LLVector3d		axis_d;

	// pick the first object to constrain to grid w/ common origin
	// this is so we don't screw up groups
	LLSelectNode* selectNode = mObjectSelection->getFirstMoveableNode(TRUE);
	if (!selectNode)
	{
		// somehow we lost the object!
		llwarns << "Translate manip lost the object, no selectNode" << llendl;
		gViewerWindow->setCursor(UI_CURSOR_TOOLTRANSLATE);
		return TRUE;
	}

	LLViewerObject* object = selectNode->getObject();
	if (!object)
	{
		// somehow we lost the object!
		llwarns << "Translate manip lost the object, no object in selectNode" << llendl;
		gViewerWindow->setCursor(UI_CURSOR_TOOLTRANSLATE);
		return TRUE;
	}

	// Compute unit vectors for arrow hit and a plane through that vector
	BOOL axis_exists = getManipAxis(object, mManipPart, axis_f);		// TODO: move this

	axis_d.setVec(axis_f);

	LLSelectMgr::getInstance()->updateSelectionCenter();
	LLVector3d current_pos_global = gAgent.getPosGlobalFromAgent(getPivotPoint());

	mSubdivisions = llclamp(getSubdivisionLevel(getPivotPoint(), axis_f, getMinGridScale()), sGridMinSubdivisionLevel, sGridMaxSubdivisionLevel);

	// Project the cursor onto that plane
	LLVector3d relative_move;
	getMousePointOnPlaneGlobal(relative_move, x, y, current_pos_global, mManipNormal);\
	relative_move -= mDragCursorStartGlobal;

	// You can't move more than some distance from your original mousedown point.
	if (gSavedSettings.getBOOL("LimitDragDistance"))
	{
		F32 max_drag_distance = gSavedSettings.getF32("MaxDragDistance");

		if (relative_move.magVecSquared() > max_drag_distance * max_drag_distance)
		{
			lldebugst(LLERR_USER_INPUT) << "hover handled by LLManipTranslate (too far)" << llendl;
			gViewerWindow->setCursor(UI_CURSOR_NOLOCKED);
			return TRUE;
		}
	}

	F64 axis_magnitude = relative_move * axis_d;					// dot product
	LLVector3d cursor_point_snap_line;
	
	F64 off_axis_magnitude;

	getMousePointOnPlaneGlobal(cursor_point_snap_line, x, y, current_pos_global, mSnapOffsetAxis % axis_f);
	off_axis_magnitude = axis_exists ? llabs((cursor_point_snap_line - current_pos_global) * LLVector3d(mSnapOffsetAxis)) : 0.f;

	if (gSavedSettings.getBOOL("SnapEnabled"))
	{
		if (off_axis_magnitude > mSnapOffsetMeters)
		{
			mInSnapRegime = TRUE;
			LLVector3 mouse_down_offset(mDragCursorStartGlobal - mDragSelectionStartGlobal);
			LLVector3 cursor_snap_agent = gAgent.getPosAgentFromGlobal(cursor_point_snap_line);
			if (!gSavedSettings.getBOOL("SnapToMouseCursor"))
			{
				cursor_snap_agent -= mouse_down_offset;
			}

			F32 cursor_grid_dist = (cursor_snap_agent - mGridOrigin) * axis_f;
			
			F32 snap_dist = getMinGridScale() / (2.f * mSubdivisions);
			F32 relative_snap_dist = fmodf(llabs(cursor_grid_dist) + snap_dist, getMinGridScale() / mSubdivisions);
			if (relative_snap_dist < snap_dist * 2.f)
			{
				if (cursor_grid_dist > 0.f)
				{
					cursor_grid_dist -= relative_snap_dist - snap_dist;
				}
				else
				{
					cursor_grid_dist += relative_snap_dist - snap_dist;
				}
			}

			F32 object_start_on_axis = (gAgent.getPosAgentFromGlobal(mDragSelectionStartGlobal) - mGridOrigin) * axis_f;
			axis_magnitude = cursor_grid_dist - object_start_on_axis;
		}
		else if (mManipPart >= LL_YZ_PLANE && mManipPart <= LL_XY_PLANE)
		{
			// subtract offset from object center
			LLVector3d cursor_point_global;
			getMousePointOnPlaneGlobal( cursor_point_global, x, y, current_pos_global, mManipNormal );
			cursor_point_global -= (mDragCursorStartGlobal - mDragSelectionStartGlobal);

			// snap to planar grid
			LLVector3 cursor_point_agent = gAgent.getPosAgentFromGlobal(cursor_point_global);
			LLVector3 camera_plane_projection = LLViewerCamera::getInstance()->getAtAxis();
			camera_plane_projection -= projected_vec(camera_plane_projection, mManipNormal);
			camera_plane_projection.normVec();
			LLVector3 camera_projected_dir = camera_plane_projection;
			camera_plane_projection.rotVec(~mGridRotation);
			camera_plane_projection.scaleVec(mGridScale);
			camera_plane_projection.abs();
			F32 max_grid_scale;
			if (camera_plane_projection.mV[VX] > camera_plane_projection.mV[VY] &&
				camera_plane_projection.mV[VX] > camera_plane_projection.mV[VZ])
			{
				max_grid_scale = mGridScale.mV[VX];
			}
			else if (camera_plane_projection.mV[VY] > camera_plane_projection.mV[VZ])
			{
				max_grid_scale = mGridScale.mV[VY];
			}
			else
			{
				max_grid_scale = mGridScale.mV[VZ];
			}

			F32 num_subdivisions = llclamp(getSubdivisionLevel(getPivotPoint(), camera_projected_dir, max_grid_scale), sGridMinSubdivisionLevel, sGridMaxSubdivisionLevel);

			F32 grid_scale_a;
			F32 grid_scale_b;
			LLVector3 cursor_point_grid = (cursor_point_agent - mGridOrigin) * ~mGridRotation;

			switch (mManipPart)
			{
			case LL_YZ_PLANE:
				grid_scale_a = mGridScale.mV[VY] / num_subdivisions;
				grid_scale_b = mGridScale.mV[VZ] / num_subdivisions;
				cursor_point_grid.mV[VY] -= fmod(cursor_point_grid.mV[VY] + grid_scale_a * 0.5f, grid_scale_a) - grid_scale_a * 0.5f;
				cursor_point_grid.mV[VZ] -= fmod(cursor_point_grid.mV[VZ] + grid_scale_b * 0.5f, grid_scale_b) - grid_scale_b * 0.5f;
				break;
			case LL_XZ_PLANE:
				grid_scale_a = mGridScale.mV[VX] / num_subdivisions;
				grid_scale_b = mGridScale.mV[VZ] / num_subdivisions;
				cursor_point_grid.mV[VX] -= fmod(cursor_point_grid.mV[VX] + grid_scale_a * 0.5f, grid_scale_a) - grid_scale_a * 0.5f;
				cursor_point_grid.mV[VZ] -= fmod(cursor_point_grid.mV[VZ] + grid_scale_b * 0.5f, grid_scale_b) - grid_scale_b * 0.5f;
				break;
			case LL_XY_PLANE:
				grid_scale_a = mGridScale.mV[VX] / num_subdivisions;
				grid_scale_b = mGridScale.mV[VY] / num_subdivisions;
				cursor_point_grid.mV[VX] -= fmod(cursor_point_grid.mV[VX] + grid_scale_a * 0.5f, grid_scale_a) - grid_scale_a * 0.5f;
				cursor_point_grid.mV[VY] -= fmod(cursor_point_grid.mV[VY] + grid_scale_b * 0.5f, grid_scale_b) - grid_scale_b * 0.5f;
				break;
			default:
				break;
			}
			cursor_point_agent = (cursor_point_grid * mGridRotation) + mGridOrigin;
			relative_move.setVec(cursor_point_agent - gAgent.getPosAgentFromGlobal(mDragSelectionStartGlobal));
			mInSnapRegime = TRUE;
		}
		else
		{
			mInSnapRegime = FALSE;
		}
	}
	else
	{
		mInSnapRegime = FALSE;
	}

	// Clamp to arrow direction
	// *FIX: does this apply anymore?
	if (!axis_exists)
	{
		axis_magnitude = relative_move.normVec();
		axis_d.setVec(relative_move);
		axis_d.normVec();
		axis_f.setVec(axis_d);
	}

	LLVector3d clamped_relative_move = axis_magnitude * axis_d;	// scalar multiply
	LLVector3 clamped_relative_move_f = (F32)axis_magnitude * axis_f; // scalar multiply
	
	for (LLObjectSelection::iterator iter = mObjectSelection->begin();
		 iter != mObjectSelection->end(); iter++)
	{
		LLSelectNode* selectNode = *iter;
		LLViewerObject* object = selectNode->getObject();
		
		// Only apply motion to root objects and objects selected
		// as "individual".
		if (!object->isRootEdit() && !selectNode->mIndividualSelection)
		{
			continue;
		}

		if (!object->isRootEdit())
		{
			// child objects should not update if parent is selected
			LLViewerObject* editable_root = (LLViewerObject*)object->getParent();
			if (editable_root->isSelected())
			{
				// we will be moved properly by our parent, so skip
				continue;
			}
		}

		LLViewerObject* root_object = (object == NULL) ? NULL : object->getRootEdit();
		if (object->permMove() && !object->isPermanentEnforced() &&
			((root_object == NULL) || !root_object->isPermanentEnforced()))
		{
			// handle attachments in local space
			if (object->isAttachment() && object->mDrawable.notNull())
			{
				// calculate local version of relative move
				LLQuaternion objWorldRotation = object->mDrawable->mXform.getParent()->getWorldRotation();
				objWorldRotation.transQuat();

				LLVector3 old_position_local = object->getPosition();
				LLVector3 new_position_local = selectNode->mSavedPositionLocal + (clamped_relative_move_f * objWorldRotation);

				//RN: I forget, but we need to do this because of snapping which doesn't often result
				// in position changes even when the mouse moves
				object->setPosition(new_position_local);
				rebuild(object);
				gAgentAvatarp->clampAttachmentPositions();
				new_position_local = object->getPosition();

				if (selectNode->mIndividualSelection)
				{
					// counter-translate child objects if we are moving the root as an individual
					object->resetChildrenPosition(old_position_local - new_position_local, TRUE) ;					
				}
			}
			else
			{
				// compute new position to send to simulators, but don't set it yet.
				// We need the old position to know which simulator to send the move message to.
				LLVector3d new_position_global = selectNode->mSavedPositionGlobal + clamped_relative_move;

				// Don't let object centers go too far underground
				F64 min_height = LLWorld::getInstance()->getMinAllowedZ(object, object->getPositionGlobal());
				if (new_position_global.mdV[VZ] < min_height)
				{
					new_position_global.mdV[VZ] = min_height;
				}

				// For safety, cap heights where objects can be dragged
				if (new_position_global.mdV[VZ] > MAX_OBJECT_Z)
				{
					new_position_global.mdV[VZ] = MAX_OBJECT_Z;
				}

				// Grass is always drawn on the ground, so clamp its position to the ground
				if (object->getPCode() == LL_PCODE_LEGACY_GRASS)
				{
					new_position_global.mdV[VZ] = LLWorld::getInstance()->resolveLandHeightGlobal(new_position_global) + 1.f;
				}
				
				if (object->isRootEdit())
				{
					new_position_global = LLWorld::getInstance()->clipToVisibleRegions(object->getPositionGlobal(), new_position_global);
				}

				// PR: Only update if changed
				LLVector3 old_position_agent = object->getPositionAgent();
				LLVector3 new_position_agent = gAgent.getPosAgentFromGlobal(new_position_global);
				if (object->isRootEdit())
				{
					// finally, move parent object after children have calculated new offsets
					object->setPositionAgent(new_position_agent);
					rebuild(object);
				}
				else
				{
					LLViewerObject* root_object = object->getRootEdit();
					new_position_agent -= root_object->getPositionAgent();
					new_position_agent = new_position_agent * ~root_object->getRotation();
					object->setPositionParent(new_position_agent, FALSE);
					rebuild(object);
				}

				if (selectNode->mIndividualSelection)
				{
					// counter-translate child objects if we are moving the root as an individual
					object->resetChildrenPosition(old_position_agent - new_position_agent, TRUE) ;					
				}
			}
			selectNode->mLastPositionLocal  = object->getPosition();
		}
	}

	LLSelectMgr::getInstance()->updateSelectionCenter();
	gAgentCamera.clearFocusObject();
	dialog_refresh_all();		// ??? is this necessary?

	lldebugst(LLERR_USER_INPUT) << "hover handled by LLManipTranslate (active)" << llendl;
	gViewerWindow->setCursor(UI_CURSOR_TOOLTRANSLATE);
	return TRUE;
}

void LLManipTranslate::highlightManipulators(S32 x, S32 y)
{
	mHighlightedPart = LL_NO_PART;

	if (!mObjectSelection->getObjectCount())
	{
		return;
	}
	
	//LLBBox bbox = LLSelectMgr::getInstance()->getBBoxOfSelection();
	LLMatrix4 projMatrix = LLViewerCamera::getInstance()->getProjection();
	LLMatrix4 modelView = LLViewerCamera::getInstance()->getModelview();

	LLVector3 object_position = getPivotPoint();
	
	LLVector3 grid_origin;
	LLVector3 grid_scale;
	LLQuaternion grid_rotation;

	LLSelectMgr::getInstance()->getGrid(grid_origin, grid_rotation, grid_scale);

	LLVector3 relative_camera_dir;

	LLMatrix4 transform;

	if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
	{
		relative_camera_dir = LLVector3(1.f, 0.f, 0.f) * ~grid_rotation;
		LLVector4 translation(object_position);
		transform.initRotTrans(grid_rotation, translation);
		LLMatrix4 cfr(OGL_TO_CFR_ROTATION);
		transform *= cfr;
		LLMatrix4 window_scale;
		F32 zoom_level = 2.f * gAgentCamera.mHUDCurZoom;
		window_scale.initAll(LLVector3(zoom_level / LLViewerCamera::getInstance()->getAspect(), zoom_level, 0.f),
			LLQuaternion::DEFAULT,
			LLVector3::zero);
		transform *= window_scale;
	}
	else
	{
		relative_camera_dir = (object_position - LLViewerCamera::getInstance()->getOrigin()) * ~grid_rotation;
		relative_camera_dir.normVec();

		transform.initRotTrans(grid_rotation, LLVector4(object_position));
		transform *= modelView;
		transform *= projMatrix;
	}
		
	S32 numManips = 0;

	// edges
	mManipulatorVertices[numManips++] = LLVector4(mArrowLengthMeters * MANIPULATOR_HOTSPOT_START, 0.f, 0.f, 1.f);
	mManipulatorVertices[numManips++] = LLVector4(mArrowLengthMeters * MANIPULATOR_HOTSPOT_END, 0.f, 0.f, 1.f);

	mManipulatorVertices[numManips++] = LLVector4(0.f, mArrowLengthMeters * MANIPULATOR_HOTSPOT_START, 0.f, 1.f);
	mManipulatorVertices[numManips++] = LLVector4(0.f, mArrowLengthMeters * MANIPULATOR_HOTSPOT_END, 0.f, 1.f);

	mManipulatorVertices[numManips++] = LLVector4(0.f, 0.f, mArrowLengthMeters * MANIPULATOR_HOTSPOT_START, 1.f);
	mManipulatorVertices[numManips++] = LLVector4(0.f, 0.f, mArrowLengthMeters * MANIPULATOR_HOTSPOT_END, 1.f);

	mManipulatorVertices[numManips++] = LLVector4(mArrowLengthMeters * -MANIPULATOR_HOTSPOT_START, 0.f, 0.f, 1.f);
	mManipulatorVertices[numManips++] = LLVector4(mArrowLengthMeters * -MANIPULATOR_HOTSPOT_END, 0.f, 0.f, 1.f);

	mManipulatorVertices[numManips++] = LLVector4(0.f, mArrowLengthMeters * -MANIPULATOR_HOTSPOT_START, 0.f, 1.f);
	mManipulatorVertices[numManips++] = LLVector4(0.f, mArrowLengthMeters * -MANIPULATOR_HOTSPOT_END, 0.f, 1.f);

	mManipulatorVertices[numManips++] = LLVector4(0.f, 0.f, mArrowLengthMeters * -MANIPULATOR_HOTSPOT_START, 1.f);
	mManipulatorVertices[numManips++] = LLVector4(0.f, 0.f, mArrowLengthMeters * -MANIPULATOR_HOTSPOT_END, 1.f);

	S32 num_arrow_manips = numManips;

	// planar manipulators
	BOOL planar_manip_yz_visible = FALSE;
	BOOL planar_manip_xz_visible = FALSE;
	BOOL planar_manip_xy_visible = FALSE;

	mManipulatorVertices[numManips] = LLVector4(0.f, mPlaneManipOffsetMeters * (1.f - PLANE_TICK_SIZE * 0.5f), mPlaneManipOffsetMeters * (1.f - PLANE_TICK_SIZE * 0.5f), 1.f);
	mManipulatorVertices[numManips++].scaleVec(mPlaneManipPositions);
	mManipulatorVertices[numManips] = LLVector4(0.f, mPlaneManipOffsetMeters * (1.f + PLANE_TICK_SIZE * 0.5f), mPlaneManipOffsetMeters * (1.f + PLANE_TICK_SIZE * 0.5f), 1.f);
	mManipulatorVertices[numManips++].scaleVec(mPlaneManipPositions);
	if (llabs(relative_camera_dir.mV[VX]) > MIN_PLANE_MANIP_DOT_PRODUCT)
	{
		planar_manip_yz_visible = TRUE;
	}

	mManipulatorVertices[numManips] = LLVector4(mPlaneManipOffsetMeters * (1.f - PLANE_TICK_SIZE * 0.5f), 0.f, mPlaneManipOffsetMeters * (1.f - PLANE_TICK_SIZE * 0.5f), 1.f);
	mManipulatorVertices[numManips++].scaleVec(mPlaneManipPositions);
	mManipulatorVertices[numManips] = LLVector4(mPlaneManipOffsetMeters * (1.f + PLANE_TICK_SIZE * 0.5f), 0.f, mPlaneManipOffsetMeters * (1.f + PLANE_TICK_SIZE * 0.5f), 1.f);
	mManipulatorVertices[numManips++].scaleVec(mPlaneManipPositions);
	if (llabs(relative_camera_dir.mV[VY]) > MIN_PLANE_MANIP_DOT_PRODUCT)
	{
		planar_manip_xz_visible = TRUE;
	}

	mManipulatorVertices[numManips] = LLVector4(mPlaneManipOffsetMeters * (1.f - PLANE_TICK_SIZE * 0.5f), mPlaneManipOffsetMeters * (1.f - PLANE_TICK_SIZE * 0.5f), 0.f, 1.f);
	mManipulatorVertices[numManips++].scaleVec(mPlaneManipPositions);
	mManipulatorVertices[numManips] = LLVector4(mPlaneManipOffsetMeters * (1.f + PLANE_TICK_SIZE * 0.5f), mPlaneManipOffsetMeters * (1.f + PLANE_TICK_SIZE * 0.5f), 0.f, 1.f);
	mManipulatorVertices[numManips++].scaleVec(mPlaneManipPositions);
	if (llabs(relative_camera_dir.mV[VZ]) > MIN_PLANE_MANIP_DOT_PRODUCT)
	{
		planar_manip_xy_visible = TRUE;
	}

	// Project up to 9 manipulators to screen space 2*X, 2*Y, 2*Z, 3*planes
	std::vector<ManipulatorHandle> projected_manipulators;
	projected_manipulators.reserve(9);
	
	for (S32 i = 0; i < num_arrow_manips; i+= 2)
	{
		LLVector4 projected_start = mManipulatorVertices[i] * transform;
		projected_start = projected_start * (1.f / projected_start.mV[VW]);

		LLVector4 projected_end = mManipulatorVertices[i + 1] * transform;
		projected_end = projected_end * (1.f / projected_end.mV[VW]);

		ManipulatorHandle projected_manip(
				LLVector3(projected_start.mV[VX], projected_start.mV[VY], projected_start.mV[VZ]), 
				LLVector3(projected_end.mV[VX], projected_end.mV[VY], projected_end.mV[VZ]), 
				MANIPULATOR_IDS[i / 2],
				10.f); // 10 pixel hotspot for arrows
		projected_manipulators.push_back(projected_manip);
	}

	if (planar_manip_yz_visible)
	{
		S32 i = num_arrow_manips;
		LLVector4 projected_start = mManipulatorVertices[i] * transform;
		projected_start = projected_start * (1.f / projected_start.mV[VW]);

		LLVector4 projected_end = mManipulatorVertices[i + 1] * transform;
		projected_end = projected_end * (1.f / projected_end.mV[VW]);

		ManipulatorHandle projected_manip(
				LLVector3(projected_start.mV[VX], projected_start.mV[VY], projected_start.mV[VZ]), 
				LLVector3(projected_end.mV[VX], projected_end.mV[VY], projected_end.mV[VZ]), 
				MANIPULATOR_IDS[i / 2],
				20.f); // 20 pixels for planar manipulators
		projected_manipulators.push_back(projected_manip);
	}

	if (planar_manip_xz_visible)
	{
		S32 i = num_arrow_manips + 2;
		LLVector4 projected_start = mManipulatorVertices[i] * transform;
		projected_start = projected_start * (1.f / projected_start.mV[VW]);

		LLVector4 projected_end = mManipulatorVertices[i + 1] * transform;
		projected_end = projected_end * (1.f / projected_end.mV[VW]);

		ManipulatorHandle projected_manip(
				LLVector3(projected_start.mV[VX], projected_start.mV[VY], projected_start.mV[VZ]), 
				LLVector3(projected_end.mV[VX], projected_end.mV[VY], projected_end.mV[VZ]), 
				MANIPULATOR_IDS[i / 2],
				20.f); // 20 pixels for planar manipulators
		projected_manipulators.push_back(projected_manip);
	}

	if (planar_manip_xy_visible)
	{
		S32 i = num_arrow_manips + 4;
		LLVector4 projected_start = mManipulatorVertices[i] * transform;
		projected_start = projected_start * (1.f / projected_start.mV[VW]);

		LLVector4 projected_end = mManipulatorVertices[i + 1] * transform;
		projected_end = projected_end * (1.f / projected_end.mV[VW]);

		ManipulatorHandle projected_manip(
				LLVector3(projected_start.mV[VX], projected_start.mV[VY], projected_start.mV[VZ]), 
				LLVector3(projected_end.mV[VX], projected_end.mV[VY], projected_end.mV[VZ]), 
				MANIPULATOR_IDS[i / 2],
				20.f); // 20 pixels for planar manipulators
		projected_manipulators.push_back(projected_manip);
	}

	LLVector2 manip_start_2d;
	LLVector2 manip_end_2d;
	LLVector2 manip_dir;
	LLRect world_view_rect = gViewerWindow->getWorldViewRectScaled();
	F32 half_width = (F32)world_view_rect.getWidth() / 2.f;
	F32 half_height = (F32)world_view_rect.getHeight() / 2.f;
	LLVector2 mousePos((F32)x - half_width, (F32)y - half_height);
	LLVector2 mouse_delta;

	// Keep order consistent with insertion via stable_sort
	std::stable_sort( projected_manipulators.begin(),
		projected_manipulators.end(),
		ClosestToCamera() );

	std::vector<ManipulatorHandle>::iterator it = projected_manipulators.begin();
	for ( ; it != projected_manipulators.end(); ++it)
	{
		ManipulatorHandle& manipulator = *it;
		{
			manip_start_2d.setVec(manipulator.mStartPosition.mV[VX] * half_width, manipulator.mStartPosition.mV[VY] * half_height);
			manip_end_2d.setVec(manipulator.mEndPosition.mV[VX] * half_width, manipulator.mEndPosition.mV[VY] * half_height);
			manip_dir = manip_end_2d - manip_start_2d;

			mouse_delta = mousePos - manip_start_2d;

			F32 manip_length = manip_dir.normVec();

			F32 mouse_pos_manip = mouse_delta * manip_dir;
			F32 mouse_dist_manip_squared = mouse_delta.magVecSquared() - (mouse_pos_manip * mouse_pos_manip);

			if (mouse_pos_manip > 0.f &&
				mouse_pos_manip < manip_length &&
				mouse_dist_manip_squared < manipulator.mHotSpotRadius * manipulator.mHotSpotRadius)
			{
				mHighlightedPart = manipulator.mManipID;
				break;
			}
		}
	}
}

F32 LLManipTranslate::getMinGridScale()
{
	F32 scale;
	switch (mManipPart)
	{
	case LL_NO_PART:
	default:
		scale = 1.f;
		break;
	case LL_X_ARROW:
		scale = mGridScale.mV[VX];
		break;
	case LL_Y_ARROW:
		scale = mGridScale.mV[VY];
		break;
	case LL_Z_ARROW:
		scale = mGridScale.mV[VZ];
		break;
	case LL_YZ_PLANE:
		scale = llmin(mGridScale.mV[VY], mGridScale.mV[VZ]);
		break;
	case LL_XZ_PLANE:
		scale = llmin(mGridScale.mV[VX], mGridScale.mV[VZ]);
		break;
	case LL_XY_PLANE:
		scale = llmin(mGridScale.mV[VX], mGridScale.mV[VY]);
		break;
	}

	return scale;
}


BOOL LLManipTranslate::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// first, perform normal processing in case this was a quick-click
	handleHover(x, y, mask);

	if(hasMouseCapture())
	{
		// make sure arrow colors go back to normal
		mManipPart = LL_NO_PART;
		LLSelectMgr::getInstance()->enableSilhouette(TRUE);

		// Might have missed last update due to UPDATE_DELAY timing.
		LLSelectMgr::getInstance()->sendMultipleUpdate( UPD_POSITION );
		
		mInSnapRegime = FALSE;
		LLSelectMgr::getInstance()->saveSelectedObjectTransform(SELECT_ACTION_TYPE_PICK);
		//gAgent.setObjectTracking(gSavedSettings.getBOOL("TrackFocusObject"));
	}

	return LLManip::handleMouseUp(x, y, mask);
}


void LLManipTranslate::render()
{
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
	{
		F32 zoom = gAgentCamera.mHUDCurZoom;
		gGL.scalef(zoom, zoom, zoom);
	}
	{
		LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
		renderGuidelines();
	}
	{
		renderTranslationHandles();
		renderSnapGuides();
	}
	gGL.popMatrix();

	renderText();
}

void LLManipTranslate::renderSnapGuides()
{
	if (!gSavedSettings.getBOOL("SnapEnabled"))
	{
		return;
	}

	F32 max_subdivisions = sGridMaxSubdivisionLevel;//(F32)gSavedSettings.getS32("GridSubdivision");
	F32 line_alpha = gSavedSettings.getF32("GridOpacity");

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	LLGLDepthTest gls_depth(GL_TRUE);
	LLGLDisable gls_cull(GL_CULL_FACE);
	LLVector3 translate_axis;

	if (mManipPart == LL_NO_PART)
	{
		return;
	}

	LLSelectNode *first_node = mObjectSelection->getFirstMoveableNode(TRUE);
	if (!first_node)
	{
		return;
	}
	
	updateGridSettings();

	F32 smallest_grid_unit_scale = getMinGridScale() / max_subdivisions;
	LLVector3 grid_origin;
	LLVector3 grid_scale;
	LLQuaternion grid_rotation;

	LLSelectMgr::getInstance()->getGrid(grid_origin, grid_rotation, grid_scale);
	LLVector3 saved_selection_center = getSavedPivotPoint(); //LLSelectMgr::getInstance()->getSavedBBoxOfSelection().getCenterAgent();
	LLVector3 selection_center = getPivotPoint();

	LLViewerObject *first_object = first_node->getObject();

	//pick appropriate projection plane for snap rulers according to relative camera position
	if (mManipPart >= LL_X_ARROW && mManipPart <= LL_Z_ARROW)
	{
		LLVector3 normal;
		LLColor4 inner_color;
		LLManip::EManipPart temp_manip = mManipPart;
		switch (mManipPart)
		{
		case LL_X_ARROW:
			normal.setVec(1,0,0);
			inner_color.setVec(0,1,1,line_alpha);
			mManipPart = LL_YZ_PLANE;
			break;
		case LL_Y_ARROW:
			normal.setVec(0,1,0);
			inner_color.setVec(1,0,1,line_alpha);
			mManipPart = LL_XZ_PLANE;
			break;
		case LL_Z_ARROW:
			normal.setVec(0,0,1);
			inner_color.setVec(1,1,0,line_alpha);
			mManipPart = LL_XY_PLANE;
			break;
		default:
			break;
		}

		highlightIntersection(normal, selection_center, grid_rotation, inner_color);
		mManipPart = temp_manip;
		getManipAxis(first_object, mManipPart, translate_axis);

		LLVector3 at_axis_abs;
		if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
		{
			at_axis_abs = LLVector3::x_axis * ~grid_rotation;
		}
		else
		{
			at_axis_abs = saved_selection_center - LLViewerCamera::getInstance()->getOrigin();
			at_axis_abs.normVec();

			at_axis_abs = at_axis_abs * ~grid_rotation;
		}
		at_axis_abs.abs();

		if (at_axis_abs.mV[VX] > at_axis_abs.mV[VY] && at_axis_abs.mV[VX] > at_axis_abs.mV[VZ])
		{
			if (mManipPart == LL_Y_ARROW)
			{
				mSnapOffsetAxis = LLVector3::z_axis;
			}
			else if (mManipPart == LL_Z_ARROW)
			{
				mSnapOffsetAxis = LLVector3::y_axis;
			}
			else if (at_axis_abs.mV[VY] > at_axis_abs.mV[VZ])
			{
				mSnapOffsetAxis = LLVector3::z_axis;
			}
			else
			{
				mSnapOffsetAxis = LLVector3::y_axis;
			}
		}
		else if (at_axis_abs.mV[VY] > at_axis_abs.mV[VZ])
		{
			if (mManipPart == LL_X_ARROW)
			{
				mSnapOffsetAxis = LLVector3::z_axis;
			}
			else if (mManipPart == LL_Z_ARROW)
			{
				mSnapOffsetAxis = LLVector3::x_axis;
			}
			else if (at_axis_abs.mV[VX] > at_axis_abs.mV[VZ])
			{
				mSnapOffsetAxis = LLVector3::z_axis;
			}
			else
			{
				mSnapOffsetAxis = LLVector3::x_axis;
			}
		}
		else
		{
			if (mManipPart == LL_X_ARROW)
			{
				mSnapOffsetAxis = LLVector3::y_axis;
			}
			else if (mManipPart == LL_Y_ARROW)
			{
				mSnapOffsetAxis = LLVector3::x_axis;
			}
			else if (at_axis_abs.mV[VX] > at_axis_abs.mV[VY])
			{
				mSnapOffsetAxis = LLVector3::y_axis;
			}
			else
			{
				mSnapOffsetAxis = LLVector3::x_axis;
			}
		}

		mSnapOffsetAxis = mSnapOffsetAxis * grid_rotation;

		F32 guide_size_meters;

		if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
		{
			guide_size_meters = 1.f / gAgentCamera.mHUDCurZoom;
			mSnapOffsetMeters = mArrowLengthMeters * 1.5f;
		}
		else
		{
			LLVector3 cam_to_selection = getPivotPoint() - LLViewerCamera::getInstance()->getOrigin();
			F32 current_range = cam_to_selection.normVec();
			guide_size_meters = SNAP_GUIDE_SCREEN_SIZE * gViewerWindow->getWorldViewHeightRaw() * current_range / LLViewerCamera::getInstance()->getPixelMeterRatio();
	
			F32 fraction_of_fov = mAxisArrowLength / (F32) LLViewerCamera::getInstance()->getViewHeightInPixels();
			F32 apparent_angle = fraction_of_fov * LLViewerCamera::getInstance()->getView();  // radians
			F32 offset_at_camera = tan(apparent_angle) * 1.5f;
			F32 range = dist_vec(gAgent.getPosAgentFromGlobal(first_node->mSavedPositionGlobal), LLViewerCamera::getInstance()->getOrigin());
			mSnapOffsetMeters = range * offset_at_camera;
		}

		LLVector3 tick_start;
		LLVector3 tick_end;

		// how far away from grid origin is the selection along the axis of translation?
		F32 dist_grid_axis = (selection_center - mGridOrigin) * translate_axis;
		// find distance to nearest smallest grid unit
		F32 offset_nearest_grid_unit = fmodf(dist_grid_axis, smallest_grid_unit_scale);
		// how many smallest grid units are we away from largest grid scale?
		S32 sub_div_offset = llround(fmod(dist_grid_axis - offset_nearest_grid_unit, getMinGridScale() / sGridMinSubdivisionLevel) / smallest_grid_unit_scale);
		S32 num_ticks_per_side = llmax(1, llfloor(0.5f * guide_size_meters / smallest_grid_unit_scale));

		LLGLDepthTest gls_depth(GL_FALSE);

		for (S32 pass = 0; pass < 3; pass++)
		{
			LLColor4 line_color = setupSnapGuideRenderPass(pass);

			gGL.begin(LLRender::LINES);
			{
				LLVector3 line_start = selection_center + (mSnapOffsetMeters * mSnapOffsetAxis) + (translate_axis * (guide_size_meters * 0.5f + offset_nearest_grid_unit));
				LLVector3 line_end = selection_center + (mSnapOffsetMeters * mSnapOffsetAxis) - (translate_axis * (guide_size_meters * 0.5f + offset_nearest_grid_unit));
				LLVector3 line_mid = (line_start + line_end) * 0.5f;

				gGL.color4f(line_color.mV[VX], line_color.mV[VY], line_color.mV[VZ], line_color.mV[VW] * 0.2f);
				gGL.vertex3fv(line_start.mV);
				gGL.color4f(line_color.mV[VX], line_color.mV[VY], line_color.mV[VZ], line_color.mV[VW]);
				gGL.vertex3fv(line_mid.mV);
				gGL.vertex3fv(line_mid.mV);
				gGL.color4f(line_color.mV[VX], line_color.mV[VY], line_color.mV[VZ], line_color.mV[VW] * 0.2f);
				gGL.vertex3fv(line_end.mV);

				line_start.setVec(selection_center + (mSnapOffsetAxis * -mSnapOffsetMeters) + (translate_axis * guide_size_meters * 0.5f));
				line_end.setVec(selection_center + (mSnapOffsetAxis * -mSnapOffsetMeters) - (translate_axis * guide_size_meters * 0.5f));
				line_mid = (line_start + line_end) * 0.5f;

				gGL.color4f(line_color.mV[VX], line_color.mV[VY], line_color.mV[VZ], line_color.mV[VW] * 0.2f);
				gGL.vertex3fv(line_start.mV);
				gGL.color4f(line_color.mV[VX], line_color.mV[VY], line_color.mV[VZ], line_color.mV[VW]);
				gGL.vertex3fv(line_mid.mV);
				gGL.vertex3fv(line_mid.mV);
				gGL.color4f(line_color.mV[VX], line_color.mV[VY], line_color.mV[VZ], line_color.mV[VW] * 0.2f);
				gGL.vertex3fv(line_end.mV);

				for (S32 i = -num_ticks_per_side; i <= num_ticks_per_side; i++)
				{
					tick_start = selection_center + (translate_axis * (smallest_grid_unit_scale * (F32)i - offset_nearest_grid_unit));

					F32 cur_subdivisions = llclamp(getSubdivisionLevel(tick_start, translate_axis, getMinGridScale()), sGridMinSubdivisionLevel, sGridMaxSubdivisionLevel);

					if (fmodf((F32)(i + sub_div_offset), (max_subdivisions / cur_subdivisions)) != 0.f)
					{
						continue;
					}

					// add in off-axis offset
					tick_start += (mSnapOffsetAxis * mSnapOffsetMeters);

					F32 tick_scale = 1.f;
					for (F32 division_level = max_subdivisions; division_level >= sGridMinSubdivisionLevel; division_level /= 2.f)
					{
						if (fmodf((F32)(i + sub_div_offset), division_level) == 0.f)
						{
							break;
						}
						tick_scale *= 0.7f;
					}

// 					S32 num_ticks_to_fade = is_sub_tick ? num_ticks_per_side / 2 : num_ticks_per_side;
// 					F32 alpha = line_alpha * (1.f - (0.8f *  ((F32)llabs(i) / (F32)num_ticks_to_fade)));

					tick_end = tick_start + (mSnapOffsetAxis * mSnapOffsetMeters * tick_scale);

					gGL.color4f(line_color.mV[VX], line_color.mV[VY], line_color.mV[VZ], line_color.mV[VW]);
					gGL.vertex3fv(tick_start.mV);
					gGL.vertex3fv(tick_end.mV);

					tick_start = selection_center + (mSnapOffsetAxis * -mSnapOffsetMeters) +
						(translate_axis * (getMinGridScale() / (F32)(max_subdivisions) * (F32)i - offset_nearest_grid_unit));
					tick_end = tick_start - (mSnapOffsetAxis * mSnapOffsetMeters * tick_scale);

					gGL.vertex3fv(tick_start.mV);
					gGL.vertex3fv(tick_end.mV);
				}
			}
			gGL.end();

			if (mInSnapRegime)
			{
				LLVector3 line_start = selection_center - mSnapOffsetAxis * mSnapOffsetMeters;
				LLVector3 line_end = selection_center + mSnapOffsetAxis * mSnapOffsetMeters;

				gGL.begin(LLRender::LINES);
				{
					gGL.color4f(line_color.mV[VX], line_color.mV[VY], line_color.mV[VZ], line_color.mV[VW]);

					gGL.vertex3fv(line_start.mV);
					gGL.vertex3fv(line_end.mV);
				}
				gGL.end();

				// draw snap guide arrow
				gGL.begin(LLRender::TRIANGLES);
				{
					gGL.color4f(line_color.mV[VX], line_color.mV[VY], line_color.mV[VZ], line_color.mV[VW]);

					LLVector3 arrow_dir;
					LLVector3 arrow_span = translate_axis;

					arrow_dir = -mSnapOffsetAxis;
					gGL.vertex3fv((line_start + arrow_dir * mConeSize * SNAP_ARROW_SCALE).mV);
					gGL.vertex3fv((line_start + arrow_span * mConeSize * SNAP_ARROW_SCALE).mV);
					gGL.vertex3fv((line_start - arrow_span * mConeSize * SNAP_ARROW_SCALE).mV);

					arrow_dir = mSnapOffsetAxis;
					gGL.vertex3fv((line_end + arrow_dir * mConeSize * SNAP_ARROW_SCALE).mV);
					gGL.vertex3fv((line_end + arrow_span * mConeSize * SNAP_ARROW_SCALE).mV);
					gGL.vertex3fv((line_end - arrow_span * mConeSize * SNAP_ARROW_SCALE).mV);
				}
				gGL.end();
			}
		}

		sub_div_offset = llround(fmod(dist_grid_axis - offset_nearest_grid_unit, getMinGridScale() * 32.f) / smallest_grid_unit_scale);

		LLVector2 screen_translate_axis(llabs(translate_axis * LLViewerCamera::getInstance()->getLeftAxis()), llabs(translate_axis * LLViewerCamera::getInstance()->getUpAxis()));
		screen_translate_axis.normVec();

		S32 tick_label_spacing = llround(screen_translate_axis * sTickLabelSpacing);
        
		// render tickmark values
		for (S32 i = -num_ticks_per_side; i <= num_ticks_per_side; i++)
		{
			LLVector3 tick_pos = selection_center + (translate_axis * ((smallest_grid_unit_scale * (F32)i) - offset_nearest_grid_unit));
			F32 alpha = line_alpha * (1.f - (0.5f *  ((F32)llabs(i) / (F32)num_ticks_per_side)));

			F32 tick_scale = 1.f;
			for (F32 division_level = max_subdivisions; division_level >= sGridMinSubdivisionLevel; division_level /= 2.f)
			{
				if (fmodf((F32)(i + sub_div_offset), division_level) == 0.f)
				{
					break;
				}
				tick_scale *= 0.7f;
			}

			if (fmodf((F32)(i + sub_div_offset), (max_subdivisions / llmin(sGridMaxSubdivisionLevel, getSubdivisionLevel(tick_pos, translate_axis, getMinGridScale(), tick_label_spacing)))) == 0.f)
			{
				F32 snap_offset_meters;

				if (mSnapOffsetAxis * LLViewerCamera::getInstance()->getUpAxis() > 0.f)
				{
					snap_offset_meters = mSnapOffsetMeters;			
				}
				else
				{
					snap_offset_meters = -mSnapOffsetMeters;
				}
				LLVector3 text_origin = selection_center + 
						(translate_axis * ((smallest_grid_unit_scale * (F32)i) - offset_nearest_grid_unit)) + 
							(mSnapOffsetAxis * snap_offset_meters * (1.f + tick_scale));
				
				LLVector3 tick_offset = (tick_pos - mGridOrigin) * ~mGridRotation;
				F32 offset_val = 0.5f * tick_offset.mV[ARROW_TO_AXIS[mManipPart]] / getMinGridScale();
				EGridMode grid_mode = LLSelectMgr::getInstance()->getGridMode();
				F32 text_highlight = 0.8f;
				if(i - llround(offset_nearest_grid_unit / smallest_grid_unit_scale) == 0 && mInSnapRegime)
				{
					text_highlight = 1.f;
				}
				
				if (grid_mode == GRID_MODE_WORLD)
				{
					// rescale units to meters from multiple of grid scale
					offset_val *= 2.f * grid_scale[ARROW_TO_AXIS[mManipPart]];
					renderTickValue(text_origin, offset_val, std::string("m"), LLColor4(text_highlight, text_highlight, text_highlight, alpha));
				}
				else
				{
					renderTickValue(text_origin, offset_val, std::string("x"), LLColor4(text_highlight, text_highlight, text_highlight, alpha));
				}
			}
		}
		if (mObjectSelection->getSelectType() != SELECT_TYPE_HUD)
		{
			// render helpful text
			if (mHelpTextTimer.getElapsedTimeF32() < sHelpTextVisibleTime + sHelpTextFadeTime && sNumTimesHelpTextShown < sMaxTimesShowHelpText)
			{
				F32 snap_offset_meters_up;
				if (mSnapOffsetAxis * LLViewerCamera::getInstance()->getUpAxis() > 0.f)
				{
					snap_offset_meters_up = mSnapOffsetMeters;			
				}
				else
				{
					snap_offset_meters_up = -mSnapOffsetMeters;
				}

				LLVector3 selection_center_start = getSavedPivotPoint();//LLSelectMgr::getInstance()->getSavedBBoxOfSelection().getCenterAgent();

				LLVector3 help_text_pos = selection_center_start + (snap_offset_meters_up * 3.f * mSnapOffsetAxis);
				const LLFontGL* big_fontp = LLFontGL::getFontSansSerif();

				std::string help_text = "Move mouse cursor over ruler";
				LLColor4 help_text_color = LLColor4::white;
				help_text_color.mV[VALPHA] = clamp_rescale(mHelpTextTimer.getElapsedTimeF32(), sHelpTextVisibleTime, sHelpTextVisibleTime + sHelpTextFadeTime, line_alpha, 0.f);
				hud_render_utf8text(help_text, help_text_pos, *big_fontp, LLFontGL::NORMAL, LLFontGL::NO_SHADOW, -0.5f * big_fontp->getWidthF32(help_text), 3.f, help_text_color, mObjectSelection->getSelectType() == SELECT_TYPE_HUD);
				help_text = "to snap to grid";
				help_text_pos -= LLViewerCamera::getInstance()->getUpAxis() * mSnapOffsetMeters * 0.2f;
				hud_render_utf8text(help_text, help_text_pos, *big_fontp, LLFontGL::NORMAL, LLFontGL::NO_SHADOW, -0.5f * big_fontp->getWidthF32(help_text), 3.f, help_text_color, mObjectSelection->getSelectType() == SELECT_TYPE_HUD);
			}
		}
	}
	else
	{
		// render gridlines for planar snapping

		F32 u = 0, v = 0;
        LLColor4 inner_color;
		LLVector3 normal;
		LLVector3 grid_center = selection_center - grid_origin;
		F32 usc = 1;
		F32 vsc = 1;
		
		grid_center *= ~grid_rotation;

		switch (mManipPart)
		{
		case LL_YZ_PLANE:
			u = grid_center.mV[VY];
			v = grid_center.mV[VZ];
			usc = grid_scale.mV[VY];
			vsc = grid_scale.mV[VZ];
			inner_color.setVec(0,1,1,line_alpha);
			normal.setVec(1,0,0);
			break;
		case LL_XZ_PLANE:
			u = grid_center.mV[VX];
			v = grid_center.mV[VZ];
			usc = grid_scale.mV[VX];
			vsc = grid_scale.mV[VZ];
			inner_color.setVec(1,0,1,line_alpha);
			normal.setVec(0,1,0);
			break;
		case LL_XY_PLANE:
			u = grid_center.mV[VX];
			v = grid_center.mV[VY];
			usc = grid_scale.mV[VX];
			vsc = grid_scale.mV[VY];
			inner_color.setVec(1,1,0,line_alpha);
			normal.setVec(0,0,1);
			break;
		default:
			break;
		}

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		highlightIntersection(normal, selection_center, grid_rotation, inner_color);

		gGL.pushMatrix();

		F32 x,y,z,angle_radians;
		grid_rotation.getAngleAxis(&angle_radians, &x, &y, &z);
		gGL.translatef(selection_center.mV[VX], selection_center.mV[VY], selection_center.mV[VZ]);
		gGL.rotatef(angle_radians * RAD_TO_DEG, x, y, z);
		
		F32 sz = mGridSizeMeters;
		F32 tiles = sz;

		gGL.matrixMode(LLRender::MM_TEXTURE);
		gGL.pushMatrix();
		usc = 1.0f/usc;
		vsc = 1.0f/vsc;
		
		while (usc > vsc*4.0f)
		{
			usc *= 0.5f;
		}
		while (vsc > usc * 4.0f)
		{
			vsc *= 0.5f;
		}

		gGL.scalef(usc, vsc, 1.0f);
		gGL.translatef(u, v, 0);
		
		float a = line_alpha;

		{
			//draw grid behind objects
			LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

			{
				LLGLDisable stencil(GL_STENCIL_TEST);
				{
					LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, GL_GREATER);
					gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, getGridTexName());
					gGL.flush();
					gGL.blendFunc(LLRender::BF_ZERO, LLRender::BF_ONE_MINUS_SOURCE_ALPHA);
					renderGrid(u,v,tiles,0.9f, 0.9f, 0.9f,a*0.15f);
					gGL.flush();
					gGL.setSceneBlendType(LLRender::BT_ALPHA);
				}
				
				{
					LLGLDisable alpha_test(GL_ALPHA_TEST);
					//draw black overlay
					gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
					renderGrid(u,v,tiles,0.0f, 0.0f, 0.0f,a*0.16f);

					//draw grid top
					gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, getGridTexName());
					renderGrid(u,v,tiles,1,1,1,a);

					gGL.popMatrix();
					gGL.matrixMode(LLRender::MM_MODELVIEW);
					gGL.popMatrix();
				}

				{
					LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
					renderGuidelines();
				}

				{
					LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, GL_GREATER);
					LLGLEnable stipple(GL_LINE_STIPPLE);
					gGL.flush();

					if (!LLGLSLShader::sNoFixedFunction)
					{
						glLineStipple(1, 0x3333);
					}
		
					switch (mManipPart)
					{
					  case LL_YZ_PLANE:
						renderGuidelines(FALSE, TRUE, TRUE);
						break;
					  case LL_XZ_PLANE:
						renderGuidelines(TRUE, FALSE, TRUE);
						break;
					  case LL_XY_PLANE:
						renderGuidelines(TRUE, TRUE, FALSE);
						break;
					  default:
						break;
					}
					gGL.flush();
				}
			}
		}
	}
}

void LLManipTranslate::renderGrid(F32 x, F32 y, F32 size, F32 r, F32 g, F32 b, F32 a)
{
	F32 d = size*0.5f;

	for (F32 xx = -size-d; xx < size+d; xx += d)
	{
		gGL.begin(LLRender::TRIANGLE_STRIP);
		for (F32 yy = -size-d; yy < size+d; yy += d)
		{
			float dx, dy, da;

			dx = xx; dy = yy;
			da = sqrtf(llmax(0.0f, 1.0f-sqrtf(dx*dx+dy*dy)/size))*a;
			gGL.texCoord2f(dx, dy);
			renderGridVert(dx,dy,r,g,b,da);

			dx = xx+d; dy = yy;
			da = sqrtf(llmax(0.0f, 1.0f-sqrtf(dx*dx+dy*dy)/size))*a;
			gGL.texCoord2f(dx, dy);
			renderGridVert(dx,dy,r,g,b,da);
			
			dx = xx; dy = yy+d;
			da = sqrtf(llmax(0.0f, 1.0f-sqrtf(dx*dx+dy*dy)/size))*a;
			gGL.texCoord2f(dx, dy);
			renderGridVert(dx,dy,r,g,b,da);

			dx = xx+d; dy = yy+d;
			da = sqrtf(llmax(0.0f, 1.0f-sqrtf(dx*dx+dy*dy)/size))*a;
			gGL.texCoord2f(dx, dy);
			renderGridVert(dx,dy,r,g,b,da);
		}
		gGL.end();
	}

	
}

void LLManipTranslate::highlightIntersection(LLVector3 normal, 
											 LLVector3 selection_center, 
											 LLQuaternion grid_rotation, 
											 LLColor4 inner_color)
{
	if (!gSavedSettings.getBOOL("GridCrossSections") || !LLGLSLShader::sNoFixedFunction)
	{
		return;
	}
	
	
	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;

	
	U32 types[] = { LLRenderPass::PASS_SIMPLE, LLRenderPass::PASS_ALPHA, LLRenderPass::PASS_FULLBRIGHT, LLRenderPass::PASS_SHINY };
	U32 num_types = LL_ARRAY_SIZE(types);

	GLuint stencil_mask = 0xFFFFFFFF;
	//stencil in volumes

	gGL.flush();

	if (shader)
	{
		gClipProgram.bind();
	}
		
	{
		glStencilMask(stencil_mask);
		glClearStencil(1);
		glClear(GL_STENCIL_BUFFER_BIT);
		LLGLEnable cull_face(GL_CULL_FACE);
		LLGLEnable stencil(GL_STENCIL_TEST);
		LLGLDepthTest depth (GL_TRUE, GL_FALSE, GL_ALWAYS);
		glStencilFunc(GL_ALWAYS, 0, stencil_mask);
		gGL.setColorMask(false, false);
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		gGL.diffuseColor4f(1,1,1,1);

		//setup clip plane
		normal = normal * grid_rotation;
		if (normal * (LLViewerCamera::getInstance()->getOrigin()-selection_center) < 0)
		{
			normal = -normal;
		}
		F32 d = -(selection_center * normal);
		glh::vec4f plane(normal.mV[0], normal.mV[1], normal.mV[2], d );

		gGL.getModelviewMatrix().inverse().mult_vec_matrix(plane);

		gClipProgram.uniform4fv("clip_plane", 1, plane.v);
		
		BOOL particles = gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_PARTICLES);
		BOOL clouds = gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_CLOUDS);
		
		if (particles)
		{
			LLPipeline::toggleRenderType(LLPipeline::RENDER_TYPE_PARTICLES);
		}
		if (clouds)
		{
			LLPipeline::toggleRenderType(LLPipeline::RENDER_TYPE_CLOUDS);
		}
		
		//stencil in volumes
		glStencilOp(GL_INCR, GL_INCR, GL_INCR);
		glCullFace(GL_FRONT);
		for (U32 i = 0; i < num_types; i++)
		{
			gPipeline.renderObjects(types[i], LLVertexBuffer::MAP_VERTEX, FALSE);
		}

		glStencilOp(GL_DECR, GL_DECR, GL_DECR);
		glCullFace(GL_BACK);
		for (U32 i = 0; i < num_types; i++)
		{
			gPipeline.renderObjects(types[i], LLVertexBuffer::MAP_VERTEX, FALSE);
		}
		
		if (particles)
		{
			LLPipeline::toggleRenderType(LLPipeline::RENDER_TYPE_PARTICLES);
		}
		if (clouds)
		{
			LLPipeline::toggleRenderType(LLPipeline::RENDER_TYPE_CLOUDS);
		}

		gGL.setColorMask(true, false);
	}
	gGL.color4f(1,1,1,1);

	gGL.pushMatrix();

	F32 x,y,z,angle_radians;
	grid_rotation.getAngleAxis(&angle_radians, &x, &y, &z);
	gGL.translatef(selection_center.mV[VX], selection_center.mV[VY], selection_center.mV[VZ]);
	gGL.rotatef(angle_radians * RAD_TO_DEG, x, y, z);
	
	F32 sz = mGridSizeMeters;
	F32 tiles = sz;

	if (shader)
	{
		shader->bind();
	}

	if (shader)
	{
		shader->bind();
	}

	//draw volume/plane intersections
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		LLGLDepthTest depth(GL_FALSE);
		LLGLEnable stencil(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glStencilFunc(GL_EQUAL, 0, stencil_mask);
		renderGrid(0,0,tiles,inner_color.mV[0], inner_color.mV[1], inner_color.mV[2], 0.25f);
	}

	glStencilFunc(GL_ALWAYS, 255, 0xFFFFFFFF);
	glStencilMask(0xFFFFFFFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	gGL.popMatrix();
}

void LLManipTranslate::renderText()
{
	if (mObjectSelection->getRootObjectCount() && !mObjectSelection->isAttachment())
	{
		LLVector3 pos = getPivotPoint();
		renderXYZ(pos);
	}
	else
	{
		const BOOL children_ok = TRUE;
		LLViewerObject* objectp = mObjectSelection->getFirstRootObject(children_ok);
		if (objectp)
		{
			renderXYZ(objectp->getPositionEdit());
		}
	}
}

void LLManipTranslate::renderTranslationHandles()
{
	LLVector3 grid_origin;
	LLVector3 grid_scale;
	LLQuaternion grid_rotation;
	LLGLDepthTest gls_depth(GL_FALSE);
	
	LLSelectMgr::getInstance()->getGrid(grid_origin, grid_rotation, grid_scale);
	LLVector3 at_axis;
	if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
	{
		at_axis = LLVector3::x_axis * ~grid_rotation;
	}
	else
	{
		at_axis = LLViewerCamera::getInstance()->getAtAxis() * ~grid_rotation;
	}

	if (at_axis.mV[VX] > 0.f)
	{
		mPlaneManipPositions.mV[VX] = 1.f;
	}
	else
	{
		mPlaneManipPositions.mV[VX] = -1.f;
	}

	if (at_axis.mV[VY] > 0.f)
	{
		mPlaneManipPositions.mV[VY] = 1.f;
	}
	else
	{
		mPlaneManipPositions.mV[VY] = -1.f;
	}

	if (at_axis.mV[VZ] > 0.f)
	{
		mPlaneManipPositions.mV[VZ] = 1.f;
	}
	else
	{
		mPlaneManipPositions.mV[VZ] = -1.f;
	}

	LLViewerObject *first_object = mObjectSelection->getFirstMoveableObject(TRUE);
	if (!first_object) return;

	LLVector3 selection_center = getPivotPoint();

	// Drag handles 	
	if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
	{
		mArrowLengthMeters = mAxisArrowLength / gViewerWindow->getWorldViewHeightRaw();
		mArrowLengthMeters /= gAgentCamera.mHUDCurZoom;
	}
	else
	{
		LLVector3 camera_pos_agent = gAgentCamera.getCameraPositionAgent();
		F32 range = dist_vec(camera_pos_agent, selection_center);
		F32 range_from_agent = dist_vec(gAgent.getPositionAgent(), selection_center);
		
		// Don't draw handles if you're too far away
		if (gSavedSettings.getBOOL("LimitSelectDistance"))
		{
			if (range_from_agent > gSavedSettings.getF32("MaxSelectDistance"))
			{
				return;
			}
		}

		if (range > 0.001f)
		{
			// range != zero
			F32 fraction_of_fov = mAxisArrowLength / (F32) LLViewerCamera::getInstance()->getViewHeightInPixels();
			F32 apparent_angle = fraction_of_fov * LLViewerCamera::getInstance()->getView();  // radians
			mArrowLengthMeters = range * tan(apparent_angle);
		}
		else
		{
			// range == zero
			mArrowLengthMeters = 1.0f;
		}
	}

	mPlaneManipOffsetMeters = mArrowLengthMeters * 1.8f;
	mGridSizeMeters = gSavedSettings.getF32("GridDrawSize");
	mConeSize = mArrowLengthMeters / 4.f;

	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	{
		gGL.translatef(selection_center.mV[VX], selection_center.mV[VY], selection_center.mV[VZ]);

		F32 angle_radians, x, y, z;
		grid_rotation.getAngleAxis(&angle_radians, &x, &y, &z);

		gGL.rotatef(angle_radians * RAD_TO_DEG, x, y, z);

		LLQuaternion invRotation = grid_rotation;
		invRotation.conjQuat();

		LLVector3 relative_camera_dir;
		
		if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
		{
			relative_camera_dir = LLVector3::x_axis * invRotation;
		}
		else
		{
			relative_camera_dir = (selection_center - LLViewerCamera::getInstance()->getOrigin()) * invRotation;
		}
		relative_camera_dir.normVec();

		{
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			LLGLDisable cull_face(GL_CULL_FACE);

			LLColor4 color1;
			LLColor4 color2;

			// update manipulator sizes
			for (S32 index = 0; index < 3; index++)
			{
				if (index == mManipPart - LL_X_ARROW || index == mHighlightedPart - LL_X_ARROW)
				{
					mArrowScales.mV[index] = lerp(mArrowScales.mV[index], SELECTED_ARROW_SCALE, LLCriticalDamp::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE ));
					mPlaneScales.mV[index] = lerp(mPlaneScales.mV[index], 1.f, LLCriticalDamp::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE ));
				}
				else if (index == mManipPart - LL_YZ_PLANE || index == mHighlightedPart - LL_YZ_PLANE)
				{
					mArrowScales.mV[index] = lerp(mArrowScales.mV[index], 1.f, LLCriticalDamp::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE ));
					mPlaneScales.mV[index] = lerp(mPlaneScales.mV[index], SELECTED_ARROW_SCALE, LLCriticalDamp::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE ));
				}
				else
				{
					mArrowScales.mV[index] = lerp(mArrowScales.mV[index], 1.f, LLCriticalDamp::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE ));
					mPlaneScales.mV[index] = lerp(mPlaneScales.mV[index], 1.f, LLCriticalDamp::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE ));
				}
			}

			if ((mManipPart == LL_NO_PART || mManipPart == LL_YZ_PLANE) && llabs(relative_camera_dir.mV[VX]) > MIN_PLANE_MANIP_DOT_PRODUCT)
			{
				// render YZ plane manipulator
				gGL.pushMatrix();
				gGL.scalef(mPlaneManipPositions.mV[VX], mPlaneManipPositions.mV[VY], mPlaneManipPositions.mV[VZ]);
				gGL.translatef(0.f, mPlaneManipOffsetMeters, mPlaneManipOffsetMeters);
				gGL.scalef(mPlaneScales.mV[VX], mPlaneScales.mV[VX], mPlaneScales.mV[VX]);
				if (mHighlightedPart == LL_YZ_PLANE)
				{
					color1.setVec(0.f, 1.f, 0.f, 1.f);
					color2.setVec(0.f, 0.f, 1.f, 1.f);
				}
				else
				{
					color1.setVec(0.f, 1.f, 0.f, 0.6f);
					color2.setVec(0.f, 0.f, 1.f, 0.6f);
				}
				gGL.begin(LLRender::TRIANGLES);
				{
					gGL.color4fv(color1.mV);
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f));
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.75f));
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f));

					gGL.color4fv(color2.mV);
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f));
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.75f), mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f));
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f));
				}
				gGL.end();

				LLUI::setLineWidth(3.0f);
				gGL.begin(LLRender::LINES);
				{
					gGL.color4f(0.f, 0.f, 0.f, 0.3f);
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f,  mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.25f,  mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.25f,  mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.1f,   mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.1f);
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.25f,  mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.1f,   mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.4f);

					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.25f);
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.25f);
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.1f,  mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.1f);
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.25f);
					gGL.vertex3f(0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.4f,  mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.1f);
				}
				gGL.end();
				LLUI::setLineWidth(1.0f);
				gGL.popMatrix();
			}

			if ((mManipPart == LL_NO_PART || mManipPart == LL_XZ_PLANE) && llabs(relative_camera_dir.mV[VY]) > MIN_PLANE_MANIP_DOT_PRODUCT)
			{
				// render XZ plane manipulator
				gGL.pushMatrix();
				gGL.scalef(mPlaneManipPositions.mV[VX], mPlaneManipPositions.mV[VY], mPlaneManipPositions.mV[VZ]);
				gGL.translatef(mPlaneManipOffsetMeters, 0.f, mPlaneManipOffsetMeters);
				gGL.scalef(mPlaneScales.mV[VY], mPlaneScales.mV[VY], mPlaneScales.mV[VY]);
				if (mHighlightedPart == LL_XZ_PLANE)
				{
					color1.setVec(0.f, 0.f, 1.f, 1.f);
					color2.setVec(1.f, 0.f, 0.f, 1.f);
				}
				else
				{
					color1.setVec(0.f, 0.f, 1.f, 0.6f);
					color2.setVec(1.f, 0.f, 0.f, 0.6f);
				}

				gGL.begin(LLRender::TRIANGLES);
				{
					gGL.color4fv(color1.mV);
					gGL.vertex3f(mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f), 0.f, mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f));
					gGL.vertex3f(mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.75f), 0.f, mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f));
					gGL.vertex3f(mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f), 0.f, mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f));

					gGL.color4fv(color2.mV);
					gGL.vertex3f(mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f), 0.f, mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f));
					gGL.vertex3f(mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f),	0.f, mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.75f));
					gGL.vertex3f(mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f),	0.f, mPlaneManipOffsetMeters * (PLANE_TICK_SIZE * 0.25f));
				}
				gGL.end();

				LLUI::setLineWidth(3.0f);
				gGL.begin(LLRender::LINES);
				{
					gGL.color4f(0.f, 0.f, 0.f, 0.3f);
					gGL.vertex3f(mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f,  0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
					gGL.vertex3f(mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.25f,  0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
					gGL.vertex3f(mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.25f,  0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
					gGL.vertex3f(mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.1f,   0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.1f);
					gGL.vertex3f(mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.25f,  0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
					gGL.vertex3f(mPlaneManipOffsetMeters * PLANE_TICK_SIZE  * 0.1f,   0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.4f);
																				
					gGL.vertex3f(mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f,  0.f, mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f);
					gGL.vertex3f(mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f,  0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.25f);
					gGL.vertex3f(mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f,  0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.25f);
					gGL.vertex3f(mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.1f,   0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.1f);
					gGL.vertex3f(mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.25f,  0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.25f);
					gGL.vertex3f(mPlaneManipOffsetMeters * -PLANE_TICK_SIZE * 0.4f,   0.f, mPlaneManipOffsetMeters * PLANE_TICK_SIZE * 0.1f);
				}
				gGL.end();
				LLUI::setLineWidth(1.0f);

				gGL.popMatrix();
			}

			if ((mManipPart == LL_NO_PART || mManipPart == LL_XY_PLANE) && llabs(relative_camera_dir.mV[VZ]) > MIN_PLANE_MANIP_DOT_PRODUCT)
			{
				// render XY plane manipulator
				gGL.pushMatrix();
				gGL.scalef(mPlaneManipPositions.mV[VX], mPlaneManipPositions.mV[VY], mPlaneManipPositions.mV[VZ]);
				
/*				 			  Y
				 			  ^
				 			  v1
				 			  |  \ 
				 			  |<- v0
				 			  |  /| \ 
				 			  v2__v__v3 > X
*/
					LLVector3 v0,v1,v2,v3;
#if 0
					// This should theoretically work but looks off; could be tuned later -SJB
					gGL.translatef(-mPlaneManipOffsetMeters, -mPlaneManipOffsetMeters, 0.f);
					v0 = LLVector3(mPlaneManipOffsetMeters * ( PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * ( PLANE_TICK_SIZE * 0.25f), 0.f);
					v1 = LLVector3(mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * ( PLANE_TICK_SIZE * 0.75f), 0.f);
					v2 = LLVector3(mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f), 0.f);
					v3 = LLVector3(mPlaneManipOffsetMeters * ( PLANE_TICK_SIZE * 0.75f), mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f), 0.f);
#else
					gGL.translatef(mPlaneManipOffsetMeters, mPlaneManipOffsetMeters, 0.f);
					v0 = LLVector3(mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.25f), 0.f);
					v1 = LLVector3(mPlaneManipOffsetMeters * ( PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.75f), 0.f);
					v2 = LLVector3(mPlaneManipOffsetMeters * ( PLANE_TICK_SIZE * 0.25f), mPlaneManipOffsetMeters * ( PLANE_TICK_SIZE * 0.25f), 0.f);
					v3 = LLVector3(mPlaneManipOffsetMeters * (-PLANE_TICK_SIZE * 0.75f), mPlaneManipOffsetMeters * ( PLANE_TICK_SIZE * 0.25f), 0.f);
#endif
					gGL.scalef(mPlaneScales.mV[VZ], mPlaneScales.mV[VZ], mPlaneScales.mV[VZ]);
					if (mHighlightedPart == LL_XY_PLANE)
					{
						color1.setVec(1.f, 0.f, 0.f, 1.f);
						color2.setVec(0.f, 1.f, 0.f, 1.f);
					}
					else
					{
						color1.setVec(0.8f, 0.f, 0.f, 0.6f);
						color2.setVec(0.f, 0.8f, 0.f, 0.6f);
					}
				
					gGL.begin(LLRender::TRIANGLES);
					{
						gGL.color4fv(color1.mV);
						gGL.vertex3fv(v0.mV);
						gGL.vertex3fv(v1.mV);
						gGL.vertex3fv(v2.mV);

						gGL.color4fv(color2.mV);
						gGL.vertex3fv(v2.mV);
						gGL.vertex3fv(v3.mV);
						gGL.vertex3fv(v0.mV);
					}
					gGL.end();

					LLUI::setLineWidth(3.0f);
					gGL.begin(LLRender::LINES);
					{
						gGL.color4f(0.f, 0.f, 0.f, 0.3f);
						LLVector3 v12 = (v1 + v2) * .5f;
						gGL.vertex3fv(v0.mV);
						gGL.vertex3fv(v12.mV);
						gGL.vertex3fv(v12.mV);
						gGL.vertex3fv((v12 + (v0-v12)*.3f + (v2-v12)*.3f).mV);
						gGL.vertex3fv(v12.mV);
						gGL.vertex3fv((v12 + (v0-v12)*.3f + (v1-v12)*.3f).mV);

						LLVector3 v23 = (v2 + v3) * .5f;
						gGL.vertex3fv(v0.mV);
						gGL.vertex3fv(v23.mV);
						gGL.vertex3fv(v23.mV);
						gGL.vertex3fv((v23 + (v0-v23)*.3f + (v3-v23)*.3f).mV);
						gGL.vertex3fv(v23.mV);
						gGL.vertex3fv((v23 + (v0-v23)*.3f + (v2-v23)*.3f).mV);
					}
					gGL.end();
					LLUI::setLineWidth(1.0f);

				gGL.popMatrix();
			}
		}
		{
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

			// Since we draw handles with depth testing off, we need to draw them in the 
			// proper depth order.

			// Copied from LLDrawable::updateGeometry
			LLVector3 pos_agent     = first_object->getPositionAgent();
			LLVector3 camera_agent	= gAgentCamera.getCameraPositionAgent();
			LLVector3 headPos		= pos_agent - camera_agent;

			LLVector3 orientWRTHead    = headPos * invRotation;

			// Find nearest vertex
			U32 nearest = (orientWRTHead.mV[0] < 0.0f ? 1 : 0) + 
				(orientWRTHead.mV[1] < 0.0f ? 2 : 0) + 
				(orientWRTHead.mV[2] < 0.0f ? 4 : 0);

			// opposite faces on Linden cubes:
			// 0 & 5
			// 1 & 3
			// 2 & 4

			// Table of order to draw faces, based on nearest vertex
			static U32 face_list[8][NUM_AXES*2] = { 
				{ 2,0,1, 4,5,3 }, // v6  F201 F453
				{ 2,0,3, 4,5,1 }, // v7  F203 F451
				{ 4,0,1, 2,5,3 }, // v5  F401 F253
				{ 4,0,3, 2,5,1 }, // v4  F403 F251
				{ 2,5,1, 4,0,3 }, // v2  F251 F403
				{ 2,5,3, 4,0,1 }, // v3  F253 F401
				{ 4,5,1, 2,0,3 }, // v1  F451 F203
				{ 4,5,3, 2,0,1 }, // v0  F453 F201
			};
			static const EManipPart which_arrow[6] = {
				LL_Z_ARROW,
				LL_X_ARROW,
				LL_Y_ARROW,
				LL_X_ARROW,
				LL_Y_ARROW,
				LL_Z_ARROW};

			// draw arrows for deeper faces first, closer faces last
			LLVector3 camera_axis;
			if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
			{
				camera_axis = LLVector3::x_axis;
			}
			else
			{
				camera_axis.setVec(gAgentCamera.getCameraPositionAgent() - first_object->getPositionAgent());
			}

			for (U32 i = 0; i < NUM_AXES*2; i++)
			{				
				U32 face = face_list[nearest][i];

				LLVector3 arrow_axis;
				getManipAxis(first_object, which_arrow[face], arrow_axis);

				renderArrow(which_arrow[face],
							mManipPart,
							(face >= 3) ? -mConeSize : mConeSize,
							(face >= 3) ? -mArrowLengthMeters : mArrowLengthMeters,
							mConeSize,
							FALSE);
			}
		}
	}
	gGL.popMatrix();
}


void LLManipTranslate::renderArrow(S32 which_arrow, S32 selected_arrow, F32 box_size, F32 arrow_size, F32 handle_size, BOOL reverse_direction)
{
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	LLGLEnable gls_blend(GL_BLEND);
	LLGLEnable gls_color_material(GL_COLOR_MATERIAL);

	for (S32 pass = 1; pass <= 2; pass++)
	{	
		LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, pass == 1 ? GL_LEQUAL : GL_GREATER);
		gGL.pushMatrix();
			
		S32 index = 0;
	
		index = ARROW_TO_AXIS[which_arrow];
		
		// assign a color for this arrow
		LLColor4 color;  // black
		if (which_arrow == selected_arrow || which_arrow == mHighlightedPart)
		{
			color.mV[index] = (pass == 1) ? 1.f : 0.5f;
		}
		else if (selected_arrow != LL_NO_PART)
		{
			color.mV[VALPHA] = 0.f;
		}
		else 
		{
			color.mV[index] = pass == 1 ? .8f : .35f ;			// red, green, or blue
			color.mV[VALPHA] = 0.6f;
		}
		gGL.color4fv( color.mV );

		LLVector3 vec;

		{
			LLUI::setLineWidth(2.0f);
			gGL.begin(LLRender::LINES);
				vec.mV[index] = box_size;
				gGL.vertex3f(vec.mV[0], vec.mV[1], vec.mV[2]);

				vec.mV[index] = arrow_size;
				gGL.vertex3f(vec.mV[0], vec.mV[1], vec.mV[2]);
			gGL.end();
			LLUI::setLineWidth(1.0f);
		}
		
		gGL.translatef(vec.mV[0], vec.mV[1], vec.mV[2]);
		gGL.scalef(handle_size, handle_size, handle_size);

		F32 rot = 0.0f;
		LLVector3 axis;

		switch(which_arrow)
		{
		case LL_X_ARROW:
			rot = reverse_direction ? -90.0f : 90.0f;
			axis.mV[1] = 1.0f;
			break;
		case LL_Y_ARROW:
			rot = reverse_direction ? 90.0f : -90.0f;
			axis.mV[0] = 1.0f;
			break;
		case LL_Z_ARROW:
			rot = reverse_direction ? 180.0f : 0.0f;
			axis.mV[0] = 1.0f;
			break;
		default:
			llerrs << "renderArrow called with bad arrow " << which_arrow << llendl;
			break;
		}

		gGL.diffuseColor4fv(color.mV);
		gGL.rotatef(rot, axis.mV[0], axis.mV[1], axis.mV[2]);
		gGL.scalef(mArrowScales.mV[index], mArrowScales.mV[index], mArrowScales.mV[index] * 1.5f);

		gCone.render();

		gGL.popMatrix();
	}
}

void LLManipTranslate::renderGridVert(F32 x_trans, F32 y_trans, F32 r, F32 g, F32 b, F32 alpha)
{
	gGL.color4f(r, g, b, alpha);
	switch (mManipPart)
	{
	case LL_YZ_PLANE:
		gGL.vertex3f(0, x_trans, y_trans);
		break;
	case LL_XZ_PLANE:
		gGL.vertex3f(x_trans, 0, y_trans);
		break;
	case LL_XY_PLANE:
		gGL.vertex3f(x_trans, y_trans, 0);
		break;
	default:
		gGL.vertex3f(0,0,0);
		break;
	}

}

// virtual
BOOL LLManipTranslate::canAffectSelection()
{
	BOOL can_move = mObjectSelection->getObjectCount() != 0;
	if (can_move)
	{
		struct f : public LLSelectedObjectFunctor
		{
			virtual bool apply(LLViewerObject* objectp)
			{
				LLViewerObject *root_object = (objectp == NULL) ? NULL : objectp->getRootEdit();
				return objectp->permMove() && !objectp->isPermanentEnforced() &&
					((root_object == NULL) || !root_object->isPermanentEnforced()) &&
					(objectp->permModify() || !gSavedSettings.getBOOL("EditLinkedParts"));
			}
		} func;
		can_move = mObjectSelection->applyToObjects(&func);
	}
	return can_move;
}
