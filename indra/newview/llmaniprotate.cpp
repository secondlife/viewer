/** 
 * @file llmaniprotate.cpp
 * @brief LLManipRotate class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llmaniprotate.h"

// library includes
#include "llmath.h"
#include "llgl.h"
#include "llrender.h"
#include "v4color.h"
#include "llprimitive.h"
#include "llview.h"
#include "llfontgl.h"

// viewer includes
#include "llagent.h"
#include "llagentcamera.h"
#include "llbox.h"
#include "llbutton.h"
#include "llviewercontrol.h"
#include "llcriticaldamp.h"
#include "lltooltip.h"
#include "llfloatertools.h"
#include "llselectmgr.h"
#include "llstatusbar.h"
#include "llui.h"
#include "llvoavatar.h"
#include "llviewercamera.h"
#include "llviewerobject.h"
#include "llviewerobject.h"
#include "llviewershadermgr.h"
#include "llviewerwindow.h"
#include "llworld.h"
#include "pipeline.h"
#include "lldrawable.h"
#include "llglheaders.h"
#include "lltrans.h"
#include "llvoavatarself.h"

const F32 RADIUS_PIXELS = 100.f;		// size in screen space
const F32 SQ_RADIUS = RADIUS_PIXELS * RADIUS_PIXELS;
const F32 WIDTH_PIXELS = 8;
const S32 CIRCLE_STEPS = 100;
const F32 DELTA = F_TWO_PI / CIRCLE_STEPS;
const F32 SIN_DELTA = sin( DELTA );
const F32 COS_DELTA = cos( DELTA );
const F32 MAX_MANIP_SELECT_DISTANCE = 100.f;
const F32 SNAP_ANGLE_INCREMENT = 5.625f;
const F32 SNAP_ANGLE_DETENTE = SNAP_ANGLE_INCREMENT;
const F32 SNAP_GUIDE_RADIUS_1 = 2.8f;
const F32 SNAP_GUIDE_RADIUS_2 = 2.4f;
const F32 SNAP_GUIDE_RADIUS_3 = 2.2f;
const F32 SNAP_GUIDE_RADIUS_4 = 2.1f;
const F32 SNAP_GUIDE_RADIUS_5 = 2.05f;
const F32 SNAP_GUIDE_INNER_RADIUS = 2.f;
const F32 AXIS_ONTO_CAM_TOLERANCE = cos( 80.f * DEG_TO_RAD );
const F32 SELECTED_MANIPULATOR_SCALE = 1.05f;
const F32 MANIPULATOR_SCALE_HALF_LIFE = 0.07f;

extern void handle_reset_rotation(void*);  // in LLViewerWindow

LLManipRotate::LLManipRotate( LLToolComposite* composite )
: 	LLManip( std::string("Rotate"), composite ),
	mRotationCenter(),
	mCenterScreen(),
	mRotation(),
	mMouseDown(),
	mMouseCur(),
	mRadiusMeters(0.f),
	mCenterToCam(),
	mCenterToCamNorm(),
	mCenterToCamMag(0.f),
	mCenterToProfilePlane(),
	mCenterToProfilePlaneMag(0.f),
	mSendUpdateOnMouseUp( FALSE ),
	mSmoothRotate( FALSE ),
	mCamEdgeOn(FALSE),
	mManipulatorScales(1.f, 1.f, 1.f, 1.f)
{ }

void LLManipRotate::handleSelect()
{
	// *FIX: put this in mouseDown?
	LLSelectMgr::getInstance()->saveSelectedObjectTransform(SELECT_ACTION_TYPE_PICK);
	gFloaterTools->setStatusText("rotate");
	LLManip::handleSelect();
}

void LLManipRotate::render()
{
	LLGLSUIDefault gls_ui;
	gGL.getTexUnit(0)->bind(LLViewerFetchedTexture::sWhiteImagep);
	LLGLDepthTest gls_depth(GL_TRUE);
	LLGLEnable gl_blend(GL_BLEND);
	LLGLEnable gls_alpha_test(GL_ALPHA_TEST);
	
	// You can rotate if you can move
	LLViewerObject* first_object = mObjectSelection->getFirstMoveableObject(TRUE);
	if( !first_object )
	{
		return;
	}

	if( !updateVisiblity() )
	{
		return;
	}

	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
	{
		F32 zoom = gAgentCamera.mHUDCurZoom;
		gGL.scalef(zoom, zoom, zoom);
	}


	LLVector3 center = gAgent.getPosAgentFromGlobal( mRotationCenter );

	LLColor4 highlight_outside( 1.f, 1.f, 0.f, 1.f );
	LLColor4 highlight_inside( 0.7f, 0.7f, 0.f, 0.5f );
	F32 width_meters = WIDTH_PIXELS * mRadiusMeters / RADIUS_PIXELS;

	gGL.pushMatrix();
	{
		
		// are we in the middle of a constrained drag?
		if (mManipPart >= LL_ROT_X && mManipPart <= LL_ROT_Z)
		{
			renderSnapGuides();
		}
		else
		{
			if (LLGLSLShader::sNoFixedFunction)
			{
				gDebugProgram.bind();
			}

			LLGLEnable cull_face(GL_CULL_FACE);
			LLGLDepthTest gls_depth(GL_FALSE);
			gGL.pushMatrix();
			{
				// Draw "sphere" (intersection of sphere with tangent cone that has apex at camera)
				gGL.translatef( mCenterToProfilePlane.mV[VX], mCenterToProfilePlane.mV[VY], mCenterToProfilePlane.mV[VZ] );
				gGL.translatef( center.mV[VX], center.mV[VY], center.mV[VZ] );

				// Inverse change of basis vectors
				LLVector3 forward = mCenterToCamNorm;
				LLVector3 left = gAgent.getUpAxis() % forward;
				left.normVec();
				LLVector3 up = forward % left;

				LLVector4 a(-forward);
				a.mV[3] = 0;
				LLVector4 b(up);
				b.mV[3] = 0;
				LLVector4 c(left);
				c.mV[3] = 0;
				LLMatrix4 mat;
				mat.initRows(a, b, c, LLVector4(0.f, 0.f, 0.f, 1.f));

				gGL.multMatrix( &mat.mMatrix[0][0] );

				gGL.rotatef( -90, 0.f, 1.f, 0.f);
				LLColor4 color;
				if (mManipPart == LL_ROT_ROLL || mHighlightedPart == LL_ROT_ROLL)
				{
					color.setVec(0.8f, 0.8f, 0.8f, 0.8f);
					gGL.scalef(mManipulatorScales.mV[VW], mManipulatorScales.mV[VW], mManipulatorScales.mV[VW]);
				}
				else
				{
					color.setVec( 0.7f, 0.7f, 0.7f, 0.6f );
				}
				gGL.diffuseColor4fv(color.mV);
				gl_washer_2d(mRadiusMeters + width_meters, mRadiusMeters, CIRCLE_STEPS, color, color);


				if (mManipPart == LL_NO_PART)
				{
					gGL.color4f( 0.7f, 0.7f, 0.7f, 0.3f );
					gGL.diffuseColor4f(0.7f, 0.7f, 0.7f, 0.3f);
					gl_circle_2d( 0, 0,  mRadiusMeters, CIRCLE_STEPS, TRUE );
				}
				
				gGL.flush();
			}
			gGL.popMatrix();

			if (LLGLSLShader::sNoFixedFunction)
			{
				gUIProgram.bind();
			}
		}
		
		gGL.translatef( center.mV[VX], center.mV[VY], center.mV[VZ] );

		LLQuaternion rot;
		F32 angle_radians, x, y, z;

		LLVector3 grid_origin;
		LLVector3 grid_scale;
		LLQuaternion grid_rotation;

		LLSelectMgr::getInstance()->getGrid(grid_origin, grid_rotation, grid_scale);

		grid_rotation.getAngleAxis(&angle_radians, &x, &y, &z);
		gGL.rotatef(angle_radians * RAD_TO_DEG, x, y, z);


		if (LLGLSLShader::sNoFixedFunction)
		{
			gDebugProgram.bind();
		}

		if (mManipPart == LL_ROT_Z)
		{
			mManipulatorScales = lerp(mManipulatorScales, LLVector4(1.f, 1.f, SELECTED_MANIPULATOR_SCALE, 1.f), LLCriticalDamp::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE));
			gGL.pushMatrix();
			{
				// selected part
				gGL.scalef(mManipulatorScales.mV[VZ], mManipulatorScales.mV[VZ], mManipulatorScales.mV[VZ]);
				renderActiveRing( mRadiusMeters, width_meters, LLColor4( 0.f, 0.f, 1.f, 1.f) , LLColor4( 0.f, 0.f, 1.f, 0.3f ));
			}
			gGL.popMatrix();
		}
		else if (mManipPart == LL_ROT_Y)
		{
			mManipulatorScales = lerp(mManipulatorScales, LLVector4(1.f, SELECTED_MANIPULATOR_SCALE, 1.f, 1.f), LLCriticalDamp::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE));
			gGL.pushMatrix();
			{
				gGL.rotatef( 90.f, 1.f, 0.f, 0.f );
				gGL.scalef(mManipulatorScales.mV[VY], mManipulatorScales.mV[VY], mManipulatorScales.mV[VY]);
				renderActiveRing( mRadiusMeters, width_meters, LLColor4( 0.f, 1.f, 0.f, 1.f), LLColor4( 0.f, 1.f, 0.f, 0.3f));
			}
			gGL.popMatrix();
		}
		else if (mManipPart == LL_ROT_X)
		{
			mManipulatorScales = lerp(mManipulatorScales, LLVector4(SELECTED_MANIPULATOR_SCALE, 1.f, 1.f, 1.f), LLCriticalDamp::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE));
			gGL.pushMatrix();
			{
				gGL.rotatef( 90.f, 0.f, 1.f, 0.f );
				gGL.scalef(mManipulatorScales.mV[VX], mManipulatorScales.mV[VX], mManipulatorScales.mV[VX]);
				renderActiveRing( mRadiusMeters, width_meters, LLColor4( 1.f, 0.f, 0.f, 1.f), LLColor4( 1.f, 0.f, 0.f, 0.3f));
			}
			gGL.popMatrix();
		}
		else if (mManipPart == LL_ROT_ROLL)
		{
			mManipulatorScales = lerp(mManipulatorScales, LLVector4(1.f, 1.f, 1.f, SELECTED_MANIPULATOR_SCALE), LLCriticalDamp::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE));
		}
		else if (mManipPart == LL_NO_PART)
		{
			if (mHighlightedPart == LL_NO_PART)
			{
				mManipulatorScales = lerp(mManipulatorScales, LLVector4(1.f, 1.f, 1.f, 1.f), LLCriticalDamp::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE));
			}

			LLGLEnable cull_face(GL_CULL_FACE);
			LLGLEnable clip_plane0(GL_CLIP_PLANE0);
			LLGLDepthTest gls_depth(GL_FALSE);

			// First pass: centers. Second pass: sides.
			for( S32 i=0; i<2; i++ )
			{
				
				gGL.pushMatrix();
				{
					if (mHighlightedPart == LL_ROT_Z)
					{
						mManipulatorScales = lerp(mManipulatorScales, LLVector4(1.f, 1.f, SELECTED_MANIPULATOR_SCALE, 1.f), LLCriticalDamp::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE));
						gGL.scalef(mManipulatorScales.mV[VZ], mManipulatorScales.mV[VZ], mManipulatorScales.mV[VZ]);
						// hovering over part
						gl_ring( mRadiusMeters, width_meters, LLColor4( 0.f, 0.f, 1.f, 1.f ), LLColor4( 0.f, 0.f, 1.f, 0.5f ), CIRCLE_STEPS, i);
					}
					else
					{
						// default
						gl_ring( mRadiusMeters, width_meters, LLColor4( 0.f, 0.f, 0.8f, 0.8f ), LLColor4( 0.f, 0.f, 0.8f, 0.4f ), CIRCLE_STEPS, i);
					}
				}
				gGL.popMatrix();
				
				gGL.pushMatrix();
				{
					gGL.rotatef( 90.f, 1.f, 0.f, 0.f );
					if (mHighlightedPart == LL_ROT_Y)
					{
						mManipulatorScales = lerp(mManipulatorScales, LLVector4(1.f, SELECTED_MANIPULATOR_SCALE, 1.f, 1.f), LLCriticalDamp::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE));
						gGL.scalef(mManipulatorScales.mV[VY], mManipulatorScales.mV[VY], mManipulatorScales.mV[VY]);
						// hovering over part
						gl_ring( mRadiusMeters, width_meters, LLColor4( 0.f, 1.f, 0.f, 1.f ), LLColor4( 0.f, 1.f, 0.f, 0.5f ), CIRCLE_STEPS, i);
					}
					else
					{
						// default
						gl_ring( mRadiusMeters, width_meters, LLColor4( 0.f, 0.8f, 0.f, 0.8f ), LLColor4( 0.f, 0.8f, 0.f, 0.4f ), CIRCLE_STEPS, i);
					}						
				}
				gGL.popMatrix();

				gGL.pushMatrix();
				{
					gGL.rotatef( 90.f, 0.f, 1.f, 0.f );
					if (mHighlightedPart == LL_ROT_X)
					{
						mManipulatorScales = lerp(mManipulatorScales, LLVector4(SELECTED_MANIPULATOR_SCALE, 1.f, 1.f, 1.f), LLCriticalDamp::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE));
						gGL.scalef(mManipulatorScales.mV[VX], mManipulatorScales.mV[VX], mManipulatorScales.mV[VX]);
	
						// hovering over part
						gl_ring( mRadiusMeters, width_meters, LLColor4( 1.f, 0.f, 0.f, 1.f ), LLColor4( 1.f, 0.f, 0.f, 0.5f ), CIRCLE_STEPS, i);
					}
					else
					{
						// default
						gl_ring( mRadiusMeters, width_meters, LLColor4( 0.8f, 0.f, 0.f, 0.8f ), LLColor4( 0.8f, 0.f, 0.f, 0.4f ), CIRCLE_STEPS, i);
					}
				}
				gGL.popMatrix();

				if (mHighlightedPart == LL_ROT_ROLL)
				{
					mManipulatorScales = lerp(mManipulatorScales, LLVector4(1.f, 1.f, 1.f, SELECTED_MANIPULATOR_SCALE), LLCriticalDamp::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE));
				}
				
			}
			
		}

		if (LLGLSLShader::sNoFixedFunction)
		{
			gUIProgram.bind();
		}
		
	}
	gGL.popMatrix();
	gGL.popMatrix();
	

	LLVector3 euler_angles;
	LLQuaternion object_rot = first_object->getRotationEdit();
	object_rot.getEulerAngles(&(euler_angles.mV[VX]), &(euler_angles.mV[VY]), &(euler_angles.mV[VZ]));
	euler_angles *= RAD_TO_DEG;
	euler_angles.mV[VX] = llround(fmodf(euler_angles.mV[VX] + 360.f, 360.f), 0.05f);
	euler_angles.mV[VY] = llround(fmodf(euler_angles.mV[VY] + 360.f, 360.f), 0.05f);
	euler_angles.mV[VZ] = llround(fmodf(euler_angles.mV[VZ] + 360.f, 360.f), 0.05f);

	renderXYZ(euler_angles);
}

BOOL LLManipRotate::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	LLViewerObject* first_object = mObjectSelection->getFirstMoveableObject(TRUE);
	if( first_object )
	{
		if( mHighlightedPart != LL_NO_PART )
		{
			handled = handleMouseDownOnPart( x, y, mask );
		}
	}

	return handled;
}

// Assumes that one of the parts of the manipulator was hit.
BOOL LLManipRotate::handleMouseDownOnPart( S32 x, S32 y, MASK mask )
{
	BOOL can_rotate = canAffectSelection();
	if (!can_rotate)
	{
		return FALSE;
	}

	highlightManipulators(x, y);
	S32 hit_part = mHighlightedPart;
	// we just started a drag, so save initial object positions
	LLSelectMgr::getInstance()->saveSelectedObjectTransform(SELECT_ACTION_TYPE_ROTATE);

	// save selection center
	mRotationCenter = gAgent.getPosGlobalFromAgent( getPivotPoint() ); //LLSelectMgr::getInstance()->getSelectionCenterGlobal();

	mManipPart = (EManipPart)hit_part;
	LLVector3 center = gAgent.getPosAgentFromGlobal( mRotationCenter );

	if( mManipPart == LL_ROT_GENERAL)
	{
		mMouseDown = intersectMouseWithSphere( x, y, center, mRadiusMeters);
	}
	else
	{
		// Project onto the plane of the ring
		LLVector3 axis = getConstraintAxis();

		F32 axis_onto_cam = llabs( axis * mCenterToCamNorm );
		const F32 AXIS_ONTO_CAM_TOL = cos( 85.f * DEG_TO_RAD );
		if( axis_onto_cam < AXIS_ONTO_CAM_TOL )
		{
			LLVector3 up_from_axis = mCenterToCamNorm % axis;
			up_from_axis.normVec();
			LLVector3 cur_intersection;
			getMousePointOnPlaneAgent(cur_intersection, x, y, center, mCenterToCam);
			cur_intersection -= center;
			mMouseDown = projected_vec(cur_intersection, up_from_axis);
			F32 mouse_depth = SNAP_GUIDE_INNER_RADIUS * mRadiusMeters;
			F32 mouse_dist_sqrd = mMouseDown.magVecSquared();
			if (mouse_dist_sqrd > 0.0001f)
			{
				mouse_depth = sqrtf((SNAP_GUIDE_INNER_RADIUS * mRadiusMeters) * (SNAP_GUIDE_INNER_RADIUS * mRadiusMeters) - 
									mouse_dist_sqrd);
			}
			LLVector3 projected_center_to_cam = mCenterToCamNorm - projected_vec(mCenterToCamNorm, axis);
			mMouseDown += mouse_depth * projected_center_to_cam;

		}
		else
		{
			mMouseDown = findNearestPointOnRing( x, y, center, axis ) - center;
			mMouseDown.normVec();
		}
	}

	mMouseCur = mMouseDown;

	// Route future Mouse messages here preemptively.  (Release on mouse up.)
	setMouseCapture( TRUE );
	LLSelectMgr::getInstance()->enableSilhouette(FALSE);
	return TRUE;
}


LLVector3 LLManipRotate::findNearestPointOnRing( S32 x, S32 y, const LLVector3& center, const LLVector3& axis )
{
	// Project the delta onto the ring and rescale it by the radius so that it's _on_ the ring.
	LLVector3 proj_onto_ring;
	getMousePointOnPlaneAgent(proj_onto_ring, x, y, center, axis);
	proj_onto_ring -= center;
	proj_onto_ring.normVec();

	return center + proj_onto_ring * mRadiusMeters;
}

BOOL LLManipRotate::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// first, perform normal processing in case this was a quick-click
	handleHover(x, y, mask);

	if( hasMouseCapture() )
	{
		for (LLObjectSelection::iterator iter = mObjectSelection->begin();
		 iter != mObjectSelection->end(); iter++)
		{
			LLSelectNode* selectNode = *iter;
			LLViewerObject* object = selectNode->getObject();
			LLViewerObject* root_object = (object == NULL) ? NULL : object->getRootEdit();

			// have permission to move and object is root of selection or individually selected
			if (object->permMove() && !object->isPermanentEnforced() &&
				((root_object == NULL) || !root_object->isPermanentEnforced()) &&
				(object->isRootEdit() || selectNode->mIndividualSelection))
			{
				object->mUnselectedChildrenPositions.clear() ;
			}
		}

		mManipPart = LL_NO_PART;

		// Might have missed last update due to timing.
		LLSelectMgr::getInstance()->sendMultipleUpdate( UPD_ROTATION | UPD_POSITION );
		LLSelectMgr::getInstance()->enableSilhouette(TRUE);
		//gAgent.setObjectTracking(gSavedSettings.getBOOL("TrackFocusObject"));

		LLSelectMgr::getInstance()->updateSelectionCenter();
		LLSelectMgr::getInstance()->saveSelectedObjectTransform(SELECT_ACTION_TYPE_PICK);
	}

	return LLManip::handleMouseUp(x, y, mask);
}


BOOL LLManipRotate::handleHover(S32 x, S32 y, MASK mask)
{
	if( hasMouseCapture() )
	{
		if( mObjectSelection->isEmpty() )
		{
			// Somehow the object got deselected while we were dragging it.
			setMouseCapture( FALSE );
		}
		else
		{
			drag(x, y);
		}

		lldebugst(LLERR_USER_INPUT) << "hover handled by LLManipRotate (active)" << llendl;		
	}
	else
	{
		highlightManipulators(x, y);
		lldebugst(LLERR_USER_INPUT) << "hover handled by LLManipRotate (inactive)" << llendl;		
	}

	gViewerWindow->setCursor(UI_CURSOR_TOOLROTATE);
	return TRUE;
}


LLVector3 LLManipRotate::projectToSphere( F32 x, F32 y, BOOL* on_sphere ) 
{
	F32 z = 0.f;
	F32 dist_squared = x*x + y*y;

	*on_sphere = dist_squared <= SQ_RADIUS;
    if( *on_sphere )
	{    
        z = sqrt(SQ_RADIUS - dist_squared);
    }
	return LLVector3( x, y, z );
}

// Freeform rotation
void LLManipRotate::drag( S32 x, S32 y )
{
	if( !updateVisiblity() )
	{
		return;
	}

	if( mManipPart == LL_ROT_GENERAL )
	{
		mRotation = dragUnconstrained(x, y);
	}
	else
	{
		mRotation = dragConstrained(x, y);
	}

	BOOL damped = mSmoothRotate;
	mSmoothRotate = FALSE;

	for (LLObjectSelection::iterator iter = mObjectSelection->begin();
		 iter != mObjectSelection->end(); iter++)
	{
		LLSelectNode* selectNode = *iter;
		LLViewerObject* object = selectNode->getObject();
		LLViewerObject* root_object = (object == NULL) ? NULL : object->getRootEdit();

		// have permission to move and object is root of selection or individually selected
		if (object->permMove() && !object->isPermanentEnforced() &&
			((root_object == NULL) || !root_object->isPermanentEnforced()) &&
			(object->isRootEdit() || selectNode->mIndividualSelection))
		{
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

			LLQuaternion new_rot = selectNode->mSavedRotation * mRotation;
			std::vector<LLVector3>& child_positions = object->mUnselectedChildrenPositions ;
			std::vector<LLQuaternion> child_rotations;
			if (object->isRootEdit() && selectNode->mIndividualSelection)
			{
				object->saveUnselectedChildrenRotation(child_rotations) ;
				object->saveUnselectedChildrenPosition(child_positions) ;			
			}

			if (object->getParent() && object->mDrawable.notNull())
			{
				LLQuaternion invParentRotation = object->mDrawable->mXform.getParent()->getWorldRotation();
				invParentRotation.transQuat();

				object->setRotation(new_rot * invParentRotation, damped);
				rebuild(object);
			}
			else
			{
				object->setRotation(new_rot, damped);
				rebuild(object);
			}

			// for individually selected roots, we need to counterrotate all the children
			if (object->isRootEdit() && selectNode->mIndividualSelection)
			{
				//RN: must do non-damped updates on these objects so relative rotation appears constant
				// instead of having two competing slerps making the child objects appear to "wobble"
				object->resetChildrenRotationAndPosition(child_rotations, child_positions) ;
			}
		}
	}

	// update positions
	for (LLObjectSelection::iterator iter = mObjectSelection->begin();
		 iter != mObjectSelection->end(); iter++)
	{
		LLSelectNode* selectNode = *iter;
		LLViewerObject* object = selectNode->getObject();
		LLViewerObject* root_object = (object == NULL) ? NULL : object->getRootEdit();

		// to avoid cumulative position changes we calculate the objects new position using its saved position
		if (object && object->permMove() && !object->isPermanentEnforced() &&
			((root_object == NULL) || !root_object->isPermanentEnforced()))
		{
			LLVector3 center   = gAgent.getPosAgentFromGlobal( mRotationCenter );

			LLVector3 old_position;
			LLVector3 new_position;

			if (object->isAttachment() && object->mDrawable.notNull())
			{ 
				// need to work in drawable space to handle selected items from multiple attachments 
				// (which have no shared frame of reference other than their render positions)
				LLXform* parent_xform = object->mDrawable->getXform()->getParent();
				new_position = (selectNode->mSavedPositionLocal * parent_xform->getWorldRotation()) + parent_xform->getWorldPosition();
				old_position = (object->getPosition() * parent_xform->getWorldRotation()) + parent_xform->getWorldPosition();//object->getRenderPosition();
			}
			else
			{
				new_position = gAgent.getPosAgentFromGlobal( selectNode->mSavedPositionGlobal );
				old_position = object->getPositionAgent();
			}

			new_position = (new_position - center) * mRotation;		// new relative rotated position
			new_position += center;
			
			if (object->isRootEdit() && !object->isAttachment())
			{
				LLVector3d new_pos_global = gAgent.getPosGlobalFromAgent(new_position);
				new_pos_global = LLWorld::getInstance()->clipToVisibleRegions(selectNode->mSavedPositionGlobal, new_pos_global);
				new_position = gAgent.getPosAgentFromGlobal(new_pos_global);
			}

			// for individually selected child objects
			if (!object->isRootEdit() && selectNode->mIndividualSelection)
			{
				LLViewerObject* parentp = (LLViewerObject*)object->getParent();
				if (!parentp->isSelected())
				{
					if (object->isAttachment() && object->mDrawable.notNull())
					{
						// find position relative to render position of parent
						object->setPosition((new_position - parentp->getRenderPosition()) * ~parentp->getRenderRotation());
						rebuild(object);
					}
					else
					{
						object->setPositionParent((new_position - parentp->getPositionAgent()) * ~parentp->getRotationRegion());
						rebuild(object);
					}
				}
			}
			else if (object->isRootEdit())
			{
				if (object->isAttachment() && object->mDrawable.notNull())
				{
					LLXform* parent_xform = object->mDrawable->getXform()->getParent();
					object->setPosition((new_position - parent_xform->getWorldPosition()) * ~parent_xform->getWorldRotation());
					rebuild(object);
				}
				else
				{
					object->setPositionAgent(new_position);
					rebuild(object);
				}
			}

			// for individually selected roots, we need to counter-translate all unselected children
			if (object->isRootEdit() && selectNode->mIndividualSelection)
			{
				// only offset by parent's translation as we've already countered parent's rotation
				rebuild(object);
				object->resetChildrenPosition(old_position - new_position) ;				
			}
		}
	}

	// store changes to override updates
	for (LLObjectSelection::iterator iter = LLSelectMgr::getInstance()->getSelection()->begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->end(); iter++)
	{
		LLSelectNode* selectNode = *iter;
		LLViewerObject*cur = selectNode->getObject();
		LLViewerObject *root_object = (cur == NULL) ? NULL : cur->getRootEdit();
		if( cur->permModify() && cur->permMove() && !cur->isPermanentEnforced() &&
			((root_object == NULL) || !root_object->isPermanentEnforced()) &&
			!cur->isAvatar())
		{
			selectNode->mLastRotation = cur->getRotation();
			selectNode->mLastPositionLocal = cur->getPosition();
		}
	}	

	LLSelectMgr::getInstance()->updateSelectionCenter();

	// RN: just clear focus so camera doesn't follow spurious object updates
	gAgentCamera.clearFocusObject();
	dialog_refresh_all();
}

void LLManipRotate::renderActiveRing( F32 radius, F32 width, const LLColor4& front_color, const LLColor4& back_color)
{
	LLGLEnable cull_face(GL_CULL_FACE);
	{
		gl_ring(radius, width, back_color, back_color * 0.5f, CIRCLE_STEPS, FALSE);
		gl_ring(radius, width, back_color, back_color * 0.5f, CIRCLE_STEPS, TRUE);
	}
	{
		LLGLDepthTest gls_depth(GL_FALSE);
		gl_ring(radius, width, front_color, front_color * 0.5f, CIRCLE_STEPS, FALSE);
		gl_ring(radius, width, front_color, front_color * 0.5f, CIRCLE_STEPS, TRUE);
	}
}

void LLManipRotate::renderSnapGuides()
{
	LLVector3 grid_origin;
	LLVector3 grid_scale;
	LLQuaternion grid_rotation;
	LLVector3 constraint_axis = getConstraintAxis();

	LLSelectMgr::getInstance()->getGrid(grid_origin, grid_rotation, grid_scale);

	if (!gSavedSettings.getBOOL("SnapEnabled"))
	{
		return;
	}

	LLVector3 center = gAgent.getPosAgentFromGlobal( mRotationCenter );
	LLVector3 cam_at_axis;
	if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
	{
		cam_at_axis.setVec(1.f, 0.f, 0.f);
	}
	else
	{
		cam_at_axis = center - gAgentCamera.getCameraPositionAgent();
		cam_at_axis.normVec();
	}

	LLVector3 world_snap_axis;
	LLVector3 test_axis = constraint_axis;

	BOOL constrain_to_ref_object = FALSE;
	if (mObjectSelection->getSelectType() == SELECT_TYPE_ATTACHMENT && isAgentAvatarValid())
	{
		test_axis = test_axis * ~grid_rotation;
	}
	else if (LLSelectMgr::getInstance()->getGridMode() == GRID_MODE_REF_OBJECT)
	{
		test_axis = test_axis * ~grid_rotation;
		constrain_to_ref_object = TRUE;
	}

	test_axis.abs();

	// find closest global/reference axis to local constraint axis;
	if (test_axis.mV[VX] > test_axis.mV[VY] && test_axis.mV[VX] > test_axis.mV[VZ])
	{
		world_snap_axis = LLVector3::y_axis;
	}
	else if (test_axis.mV[VY] > test_axis.mV[VZ])
	{
		world_snap_axis = LLVector3::z_axis;
	}
	else
	{
		world_snap_axis = LLVector3::x_axis;
	}

	LLVector3 projected_snap_axis = world_snap_axis;
	if (mObjectSelection->getSelectType() == SELECT_TYPE_ATTACHMENT && isAgentAvatarValid())
	{
		projected_snap_axis = projected_snap_axis * grid_rotation;
	}
	else if (constrain_to_ref_object)
	{
		projected_snap_axis = projected_snap_axis * grid_rotation;
	}

	// project world snap axis onto constraint plane
	projected_snap_axis -= projected_vec(projected_snap_axis, constraint_axis);
	projected_snap_axis.normVec();

	S32 num_rings = mCamEdgeOn ? 2 : 1;
	for (S32 ring_num = 0; ring_num < num_rings; ring_num++)
	{
		LLVector3 center = gAgent.getPosAgentFromGlobal( mRotationCenter );

		if (mCamEdgeOn)
		{
			// draw two opposing rings
			if (ring_num == 0)
			{
				center += constraint_axis * mRadiusMeters * 0.5f;
			}
			else
			{
				center -= constraint_axis * mRadiusMeters * 0.5f;
			}
		}

		LLGLDepthTest gls_depth(GL_FALSE);
		for (S32 pass = 0; pass < 3; pass++)
		{
			// render snap guide ring
			gGL.pushMatrix();
			
			LLQuaternion snap_guide_rot;
			F32 angle_radians, x, y, z;
			snap_guide_rot.shortestArc(LLVector3::z_axis, getConstraintAxis());
			snap_guide_rot.getAngleAxis(&angle_radians, &x, &y, &z);
			gGL.translatef(center.mV[VX], center.mV[VY], center.mV[VZ]);
			gGL.rotatef(angle_radians * RAD_TO_DEG, x, y, z);

			LLColor4 line_color = setupSnapGuideRenderPass(pass);

			gGL.color4fv(line_color.mV);

			if (mCamEdgeOn)
			{
				// render an arc
				LLVector3 edge_normal = cam_at_axis % constraint_axis;
				edge_normal.normVec();
				LLVector3 x_axis_snap = LLVector3::x_axis * snap_guide_rot;
				LLVector3 y_axis_snap = LLVector3::y_axis * snap_guide_rot;

				F32 end_angle = atan2(y_axis_snap * edge_normal, x_axis_snap * edge_normal);
				//F32 start_angle = angle_between((-1.f * LLVector3::x_axis) * snap_guide_rot, edge_normal);
				F32 start_angle = end_angle - F_PI;
				gl_arc_2d(0.f, 0.f, mRadiusMeters * SNAP_GUIDE_INNER_RADIUS, CIRCLE_STEPS, FALSE, start_angle, end_angle);
			}
			else
			{
				gl_circle_2d(0.f, 0.f, mRadiusMeters * SNAP_GUIDE_INNER_RADIUS, CIRCLE_STEPS, FALSE);
			}
			gGL.popMatrix();

			for (S32 i = 0; i < 64; i++)
			{
				BOOL render_text = TRUE;
				F32 deg = 5.625f * (F32)i;
				LLVector3 inner_point;
				LLVector3 outer_point;
				LLVector3 text_point;
				LLQuaternion rot(deg * DEG_TO_RAD, constraint_axis);
				gGL.begin(LLRender::LINES);
				{
					inner_point = (projected_snap_axis * mRadiusMeters * SNAP_GUIDE_INNER_RADIUS * rot) + center;
					F32 tick_length = 0.f;
					if (i % 16 == 0)
					{
						tick_length = mRadiusMeters * (SNAP_GUIDE_RADIUS_1 - SNAP_GUIDE_INNER_RADIUS);
					}
					else if (i % 8 == 0)
					{
						tick_length = mRadiusMeters * (SNAP_GUIDE_RADIUS_2 - SNAP_GUIDE_INNER_RADIUS);
					}
					else if (i % 4 == 0)
					{
						tick_length = mRadiusMeters * (SNAP_GUIDE_RADIUS_3 - SNAP_GUIDE_INNER_RADIUS);
					}
					else if (i % 2 == 0)
					{
						tick_length = mRadiusMeters * (SNAP_GUIDE_RADIUS_4 - SNAP_GUIDE_INNER_RADIUS);
					}
					else
					{
						tick_length = mRadiusMeters * (SNAP_GUIDE_RADIUS_5 - SNAP_GUIDE_INNER_RADIUS);
					}
					
					if (mCamEdgeOn)
					{
						// don't draw ticks that are on back side of circle
						F32 dot = cam_at_axis * (projected_snap_axis * rot);
						if (dot > 0.f)
						{
							outer_point = inner_point;
							render_text = FALSE;
						}
						else
						{
							if (ring_num == 0)
							{
								outer_point = inner_point + (constraint_axis * tick_length) * rot;
							}
							else
							{
								outer_point = inner_point - (constraint_axis * tick_length) * rot;
							}
						}
					}
					else
					{
						outer_point = inner_point + (projected_snap_axis * tick_length) * rot;
					}

					text_point = outer_point + (projected_snap_axis * mRadiusMeters * 0.1f) * rot;

					gGL.vertex3fv(inner_point.mV);
					gGL.vertex3fv(outer_point.mV);
				}
				gGL.end();

				//RN: text rendering does own shadow pass, so only render once
				if (pass == 1 && render_text && i % 16 == 0)
				{
					if (world_snap_axis.mV[VX])
					{
						if (i == 0)
						{
							renderTickText(text_point, mObjectSelection->isAttachment() ? LLTrans::getString("Direction_Forward") : LLTrans::getString("Direction_East"), LLColor4::white);
						}
						else if (i == 16)
						{
							if (constraint_axis.mV[VZ] > 0.f)
							{
								renderTickText(text_point, mObjectSelection->isAttachment() ? LLTrans::getString("Direction_Left") : LLTrans::getString("Direction_North"), LLColor4::white);
							}
							else
							{
								renderTickText(text_point, mObjectSelection->isAttachment() ? LLTrans::getString("Direction_Right") : LLTrans::getString("Direction_South"), LLColor4::white);
							}
						}
						else if (i == 32)
						{
							renderTickText(text_point, mObjectSelection->isAttachment() ? LLTrans::getString("Direction_Back") : LLTrans::getString("Direction_West"), LLColor4::white);
						}
						else
						{
							if (constraint_axis.mV[VZ] > 0.f)
							{
								renderTickText(text_point, mObjectSelection->isAttachment() ? LLTrans::getString("Direction_Right") : LLTrans::getString("Direction_South"), LLColor4::white);
							}
							else
							{
								renderTickText(text_point, mObjectSelection->isAttachment() ? LLTrans::getString("Direction_Left") : LLTrans::getString("Direction_North"), LLColor4::white);
							}
						}
					}
					else if (world_snap_axis.mV[VY])
					{
						if (i == 0)
						{
							renderTickText(text_point, mObjectSelection->isAttachment() ? LLTrans::getString("Direction_Left") : LLTrans::getString("Direction_North"), LLColor4::white);
						}
						else if (i == 16)
						{
							if (constraint_axis.mV[VX] > 0.f)
							{
								renderTickText(text_point, LLTrans::getString("Direction_Up"), LLColor4::white);
							}
							else
							{
								renderTickText(text_point, LLTrans::getString("Direction_Down"), LLColor4::white);
							}
						}
						else if (i == 32)
						{
							renderTickText(text_point, mObjectSelection->isAttachment() ? LLTrans::getString("Direction_Right") : LLTrans::getString("Direction_South"), LLColor4::white);
						}
						else
						{
							if (constraint_axis.mV[VX] > 0.f)
							{
								renderTickText(text_point, LLTrans::getString("Direction_Down"), LLColor4::white);
							}
							else
							{
								renderTickText(text_point, LLTrans::getString("Direction_Up"), LLColor4::white);
							}
						}
					}
					else if (world_snap_axis.mV[VZ])
					{
						if (i == 0)
						{
							renderTickText(text_point, LLTrans::getString("Direction_Up"), LLColor4::white);
						}
						else if (i == 16)
						{
							if (constraint_axis.mV[VY] > 0.f)
							{
								renderTickText(text_point, mObjectSelection->isAttachment() ? LLTrans::getString("Direction_Forward") : LLTrans::getString("Direction_East"), LLColor4::white);
							}
							else
							{
								renderTickText(text_point, mObjectSelection->isAttachment() ? LLTrans::getString("Direction_Back") : LLTrans::getString("Direction_West"), LLColor4::white);
							}
						}
						else if (i == 32)
						{
							renderTickText(text_point, LLTrans::getString("Direction_Down"), LLColor4::white);
						}
						else
						{
							if (constraint_axis.mV[VY] > 0.f)
							{
								renderTickText(text_point, mObjectSelection->isAttachment() ? LLTrans::getString("Direction_Back") : LLTrans::getString("Direction_West"), LLColor4::white);
							}
							else
							{
								renderTickText(text_point, mObjectSelection->isAttachment() ? LLTrans::getString("Direction_Forward") : LLTrans::getString("Direction_East"), LLColor4::white);
							}
						}
					}
				}
				gGL.color4fv(line_color.mV);
			}

			// now render projected object axis
			if (mInSnapRegime)
			{
				LLVector3 object_axis;
				getObjectAxisClosestToMouse(object_axis);

				// project onto constraint plane
				LLSelectNode* first_node = mObjectSelection->getFirstMoveableNode(TRUE);
				object_axis = object_axis * first_node->getObject()->getRenderRotation();
				object_axis = object_axis - (object_axis * getConstraintAxis()) * getConstraintAxis();
				object_axis.normVec();
				object_axis = object_axis * SNAP_GUIDE_INNER_RADIUS * mRadiusMeters + center;
				LLVector3 line_start = center;

				gGL.begin(LLRender::LINES);
				{
					gGL.vertex3fv(line_start.mV);
					gGL.vertex3fv(object_axis.mV);
				}
				gGL.end();

				// draw snap guide arrow
				gGL.begin(LLRender::TRIANGLES);
				{
					LLVector3 arrow_dir;
					LLVector3 arrow_span = (object_axis - line_start) % getConstraintAxis();
					arrow_span.normVec();

					arrow_dir = mCamEdgeOn ? getConstraintAxis() : object_axis - line_start;
					arrow_dir.normVec();
					if (ring_num == 1)
					{
						arrow_dir *= -1.f;
					}
					gGL.vertex3fv((object_axis + arrow_dir * mRadiusMeters * 0.1f).mV);
					gGL.vertex3fv((object_axis + arrow_span * mRadiusMeters * 0.1f).mV);
					gGL.vertex3fv((object_axis - arrow_span * mRadiusMeters * 0.1f).mV);
				}
				gGL.end();

				{
					LLGLDepthTest gls_depth(GL_TRUE);
					gGL.begin(LLRender::LINES);
					{
						gGL.vertex3fv(line_start.mV);
						gGL.vertex3fv(object_axis.mV);
					}
					gGL.end();

					// draw snap guide arrow
					gGL.begin(LLRender::TRIANGLES);
					{
						LLVector3 arrow_dir;
						LLVector3 arrow_span = (object_axis - line_start) % getConstraintAxis();
						arrow_span.normVec();

						arrow_dir = mCamEdgeOn ? getConstraintAxis() : object_axis - line_start;
						arrow_dir.normVec();
						if (ring_num == 1)
						{
							arrow_dir *= -1.f;
						}

						gGL.vertex3fv((object_axis + arrow_dir * mRadiusMeters * 0.1f).mV);
						gGL.vertex3fv((object_axis + arrow_span * mRadiusMeters * 0.1f).mV);
						gGL.vertex3fv((object_axis - arrow_span * mRadiusMeters * 0.1f).mV);
					}
					gGL.end();
				}
			}
		}
	}
}

// Returns TRUE if center of sphere is visible.  Also sets a bunch of member variables that are used later (e.g. mCenterToCam)
BOOL LLManipRotate::updateVisiblity()
{
	// Don't want to recalculate the center of the selection during a drag.
	// Due to packet delays, sometimes half the objects in the selection have their
	// new position and half have their old one.  This creates subtle errors in the
	// computed center position for that frame.  Unfortunately, these errors
	// accumulate.  The result is objects seem to "fly apart" during rotations.
	// JC - 03.26.2002
	if (!hasMouseCapture())
	{
		mRotationCenter = gAgent.getPosGlobalFromAgent( getPivotPoint() );//LLSelectMgr::getInstance()->getSelectionCenterGlobal();
	}

	BOOL visible = FALSE;

	LLVector3 center = gAgent.getPosAgentFromGlobal( mRotationCenter );
	if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
	{
		mCenterToCam = LLVector3(-1.f / gAgentCamera.mHUDCurZoom, 0.f, 0.f);
		mCenterToCamNorm = mCenterToCam;
		mCenterToCamMag = mCenterToCamNorm.normVec();

		mRadiusMeters = RADIUS_PIXELS / (F32) LLViewerCamera::getInstance()->getViewHeightInPixels();
		mRadiusMeters /= gAgentCamera.mHUDCurZoom;

		mCenterToProfilePlaneMag = mRadiusMeters * mRadiusMeters / mCenterToCamMag;
		mCenterToProfilePlane = -mCenterToProfilePlaneMag * mCenterToCamNorm;

		// x axis range is (-aspect * 0.5f, +aspect * 0.5)
		// y axis range is (-0.5, 0.5)
		// so use getWorldViewHeightRaw as scale factor when converting to pixel coordinates
		mCenterScreen.set((S32)((0.5f - center.mV[VY]) / gAgentCamera.mHUDCurZoom * gViewerWindow->getWorldViewHeightScaled()),
							(S32)((center.mV[VZ] + 0.5f) / gAgentCamera.mHUDCurZoom * gViewerWindow->getWorldViewHeightScaled()));
		visible = TRUE;
	}
	else
	{
		visible = LLViewerCamera::getInstance()->projectPosAgentToScreen(center, mCenterScreen );
		if( visible )
		{
			mCenterToCam = gAgentCamera.getCameraPositionAgent() - center;
			mCenterToCamNorm = mCenterToCam;
			mCenterToCamMag = mCenterToCamNorm.normVec();
			LLVector3 cameraAtAxis = LLViewerCamera::getInstance()->getAtAxis();
			cameraAtAxis.normVec();

			F32 z_dist = -1.f * (mCenterToCam * cameraAtAxis);

			// Don't drag manip if object too far away
			if (gSavedSettings.getBOOL("LimitSelectDistance"))
			{
				F32 max_select_distance = gSavedSettings.getF32("MaxSelectDistance");
				if (dist_vec_squared(gAgent.getPositionAgent(), center) > (max_select_distance * max_select_distance))
				{
					visible = FALSE;
				}
			}
			
			if (mCenterToCamMag > 0.001f)
			{
				F32 fraction_of_fov = RADIUS_PIXELS / (F32) LLViewerCamera::getInstance()->getViewHeightInPixels();
				F32 apparent_angle = fraction_of_fov * LLViewerCamera::getInstance()->getView();  // radians
				mRadiusMeters = z_dist * tan(apparent_angle);

				mCenterToProfilePlaneMag = mRadiusMeters * mRadiusMeters / mCenterToCamMag;
				mCenterToProfilePlane = -mCenterToProfilePlaneMag * mCenterToCamNorm;
			}
			else
			{
				visible = FALSE;
			}
		}
	}

	mCamEdgeOn = FALSE;
	F32 axis_onto_cam = mManipPart >= LL_ROT_X ? llabs( getConstraintAxis() * mCenterToCamNorm ) : 0.f;
	if( axis_onto_cam < AXIS_ONTO_CAM_TOLERANCE )
	{
		mCamEdgeOn = TRUE;
	}

	return visible;
}

LLQuaternion LLManipRotate::dragUnconstrained( S32 x, S32 y )
{
	LLVector3 cam = gAgentCamera.getCameraPositionAgent();
	LLVector3 center =  gAgent.getPosAgentFromGlobal( mRotationCenter );

	mMouseCur = intersectMouseWithSphere( x, y, center, mRadiusMeters);

	F32 delta_x = (F32)(mCenterScreen.mX - x);
	F32 delta_y = (F32)(mCenterScreen.mY - y);

	F32 dist_from_sphere_center = sqrt(delta_x * delta_x + delta_y * delta_y);

	LLVector3 axis = mMouseDown % mMouseCur;
	axis.normVec();
	F32 angle = acos(mMouseDown * mMouseCur);
	LLQuaternion sphere_rot( angle, axis );

	if (is_approx_zero(1.f - mMouseDown * mMouseCur))
	{
		return LLQuaternion::DEFAULT;
	}
	else if (dist_from_sphere_center < RADIUS_PIXELS)
	{
		return sphere_rot;
	}
	else
	{
		LLVector3 intersection;
		getMousePointOnPlaneAgent( intersection, x, y, center + mCenterToProfilePlane, mCenterToCamNorm );

		// amount dragging in sphere from center to periphery would rotate object
		F32 in_sphere_angle = F_PI_BY_TWO;
		F32 dist_to_tangent_point = mRadiusMeters;
		if( !is_approx_zero( mCenterToProfilePlaneMag ) )
		{
			dist_to_tangent_point = sqrt( mRadiusMeters * mRadiusMeters - mCenterToProfilePlaneMag * mCenterToProfilePlaneMag );
			in_sphere_angle = atan2( dist_to_tangent_point, mCenterToProfilePlaneMag );
		}

		LLVector3 profile_center_to_intersection = intersection - (center + mCenterToProfilePlane);
		F32 dist_to_intersection = profile_center_to_intersection.normVec();
		F32 angle = (-1.f + dist_to_intersection / dist_to_tangent_point) * in_sphere_angle;

		LLVector3 axis;
		if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
		{
			axis = LLVector3(-1.f, 0.f, 0.f) % profile_center_to_intersection;
		}
		else
		{
			axis = (cam - center) % profile_center_to_intersection;
			axis.normVec();
		}
		return sphere_rot * LLQuaternion( angle, axis );
	}
}

LLVector3 LLManipRotate::getConstraintAxis()
{
	LLVector3 axis;
	if( LL_ROT_ROLL == mManipPart )
	{
		axis = mCenterToCamNorm;
	}
	else
	{
		S32 axis_dir = mManipPart - LL_ROT_X;
		if ((axis_dir >= 0) && (axis_dir < 3))
		{
			axis.mV[axis_dir] = 1.f;
		}
		else
		{
#ifndef LL_RELEASE_FOR_DOWNLOAD
			llerrs << "Got bogus hit part in LLManipRotate::getConstraintAxis():" << mManipPart << llendl;
#else
			llwarns << "Got bogus hit part in LLManipRotate::getConstraintAxis():" << mManipPart << llendl;
#endif
			axis.mV[0] = 1.f;
		}

		LLVector3 grid_origin;
		LLVector3 grid_scale;
		LLQuaternion grid_rotation;

		LLSelectMgr::getInstance()->getGrid(grid_origin, grid_rotation, grid_scale);

		LLSelectNode* first_node = mObjectSelection->getFirstMoveableNode(TRUE);
		if (first_node)
		{
			// *FIX: get agent local attachment grid working
			// Put rotation into frame of first selected root object
			axis = axis * grid_rotation;
		}
	}

	return axis;
}

LLQuaternion LLManipRotate::dragConstrained( S32 x, S32 y )
{
	LLSelectNode* first_object_node = mObjectSelection->getFirstMoveableNode(TRUE);
	LLVector3 constraint_axis = getConstraintAxis();
	LLVector3 center = gAgent.getPosAgentFromGlobal( mRotationCenter );

	F32 angle = 0.f;

	// build snap axes
	LLVector3 grid_origin;
	LLVector3 grid_scale;
	LLQuaternion grid_rotation;

	LLSelectMgr::getInstance()->getGrid(grid_origin, grid_rotation, grid_scale);

	LLVector3 axis1;
	LLVector3 axis2;

	LLVector3 test_axis = constraint_axis;
	if (mObjectSelection->getSelectType() == SELECT_TYPE_ATTACHMENT && isAgentAvatarValid())
	{
		test_axis = test_axis * ~grid_rotation;
	}
	else if (LLSelectMgr::getInstance()->getGridMode() == GRID_MODE_REF_OBJECT)
	{
		test_axis = test_axis * ~grid_rotation;
	}
	test_axis.abs();

	// find closest global axis to constraint axis;
	if (test_axis.mV[VX] > test_axis.mV[VY] && test_axis.mV[VX] > test_axis.mV[VZ])
	{
		axis1 = LLVector3::y_axis;
	}
	else if (test_axis.mV[VY] > test_axis.mV[VZ])
	{
		axis1 = LLVector3::z_axis;
	}
	else
	{
		axis1 = LLVector3::x_axis;
	}

	if (mObjectSelection->getSelectType() == SELECT_TYPE_ATTACHMENT && isAgentAvatarValid())
	{
		axis1 = axis1 * grid_rotation;
	}
	else if (LLSelectMgr::getInstance()->getGridMode() == GRID_MODE_REF_OBJECT)
	{
		axis1 = axis1 * grid_rotation;
	}

	//project axis onto constraint plane
	axis1 -= (axis1 * constraint_axis) * constraint_axis;
	axis1.normVec();

	// calculate third and final axis
	axis2 = constraint_axis % axis1;

	//F32 axis_onto_cam = llabs( constraint_axis * mCenterToCamNorm );
	if( mCamEdgeOn )
	{
		// We're looking at the ring edge-on.
		LLVector3 snap_plane_center = (center + (constraint_axis * mRadiusMeters * 0.5f));
		LLVector3 cam_to_snap_plane;
		if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
		{
			cam_to_snap_plane.setVec(1.f, 0.f, 0.f);
		}
		else
		{
			cam_to_snap_plane = snap_plane_center - gAgentCamera.getCameraPositionAgent();
			cam_to_snap_plane.normVec();
		}

		LLVector3 projected_mouse;
		BOOL hit = getMousePointOnPlaneAgent(projected_mouse, x, y, snap_plane_center, constraint_axis);
		projected_mouse -= snap_plane_center;

		S32 snap_plane = 0;

		F32 dot = cam_to_snap_plane * constraint_axis;
		if (llabs(dot) < 0.01f)
		{
			// looking at ring edge on, project onto view plane and check if mouse is past ring
			getMousePointOnPlaneAgent(projected_mouse, x, y, snap_plane_center, cam_to_snap_plane);
			projected_mouse -= snap_plane_center;
			dot = projected_mouse * constraint_axis;
			if (projected_mouse * constraint_axis > 0)
			{
				snap_plane = 1;
			}
			projected_mouse -= dot * constraint_axis;
		}
		else if (dot > 0.f)
		{
			// look for mouse position outside and in front of snap circle
			if (hit && projected_mouse.magVec() > SNAP_GUIDE_INNER_RADIUS * mRadiusMeters && projected_mouse * cam_to_snap_plane < 0.f)
			{
				snap_plane = 1;
			}
		}
		else
		{
			// look for mouse position inside or in back of snap circle
			if (projected_mouse.magVec() < SNAP_GUIDE_INNER_RADIUS * mRadiusMeters || projected_mouse * cam_to_snap_plane > 0.f || !hit)
			{
				snap_plane = 1;
			}
		}

		if (snap_plane == 0)
		{
			// try other plane
			snap_plane_center = (center - (constraint_axis * mRadiusMeters * 0.5f));
			if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
			{
				cam_to_snap_plane.setVec(1.f, 0.f, 0.f);
			}
			else
			{
				cam_to_snap_plane = snap_plane_center - gAgentCamera.getCameraPositionAgent();
				cam_to_snap_plane.normVec();
			}

			hit = getMousePointOnPlaneAgent(projected_mouse, x, y, snap_plane_center, constraint_axis);
			projected_mouse -= snap_plane_center;

			dot = cam_to_snap_plane * constraint_axis;
			if (llabs(dot) < 0.01f)
			{
				// looking at ring edge on, project onto view plane and check if mouse is past ring
				getMousePointOnPlaneAgent(projected_mouse, x, y, snap_plane_center, cam_to_snap_plane);
				projected_mouse -= snap_plane_center;
				dot = projected_mouse * constraint_axis;
				if (projected_mouse * constraint_axis < 0)
				{
					snap_plane = 2;
				}
				projected_mouse -= dot * constraint_axis;
			}
			else if (dot < 0.f)
			{
				// look for mouse position outside and in front of snap circle
				if (hit && projected_mouse.magVec() > SNAP_GUIDE_INNER_RADIUS * mRadiusMeters && projected_mouse * cam_to_snap_plane < 0.f)
				{
					snap_plane = 2;
				}
			}
			else
			{
				// look for mouse position inside or in back of snap circle
				if (projected_mouse.magVec() < SNAP_GUIDE_INNER_RADIUS * mRadiusMeters || projected_mouse * cam_to_snap_plane > 0.f || !hit)
				{
					snap_plane = 2;
				}
			}
		}

		if (snap_plane > 0)
		{
			LLVector3 cam_at_axis;
			if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
			{
				cam_at_axis.setVec(1.f, 0.f, 0.f);
			}
			else
			{
				cam_at_axis = snap_plane_center - gAgentCamera.getCameraPositionAgent();
				cam_at_axis.normVec();
			}

			// first, project mouse onto screen plane at point tangent to rotation radius. 
			getMousePointOnPlaneAgent(projected_mouse, x, y, snap_plane_center, cam_at_axis);
			// project that point onto rotation plane
			projected_mouse -= snap_plane_center;
			projected_mouse -= projected_vec(projected_mouse, constraint_axis);

			F32 mouse_lateral_dist = llmin(SNAP_GUIDE_INNER_RADIUS * mRadiusMeters, projected_mouse.magVec());
			F32 mouse_depth = SNAP_GUIDE_INNER_RADIUS * mRadiusMeters;
			if (llabs(mouse_lateral_dist) > 0.01f)
			{
				mouse_depth = sqrtf((SNAP_GUIDE_INNER_RADIUS * mRadiusMeters) * (SNAP_GUIDE_INNER_RADIUS * mRadiusMeters) - 
									(mouse_lateral_dist * mouse_lateral_dist));
			}
			LLVector3 projected_camera_at = cam_at_axis - projected_vec(cam_at_axis, constraint_axis);
			projected_mouse -= mouse_depth * projected_camera_at;

			if (!mInSnapRegime)
			{
				mSmoothRotate = TRUE;
			}
			mInSnapRegime = TRUE;
			// 0 to 360 deg
			F32 mouse_angle = fmodf(atan2(projected_mouse * axis1, projected_mouse * axis2) * RAD_TO_DEG + 360.f, 360.f);
			
			F32 relative_mouse_angle = fmodf(mouse_angle + (SNAP_ANGLE_DETENTE / 2), SNAP_ANGLE_INCREMENT);
			//fmodf(llround(mouse_angle * RAD_TO_DEG, 7.5f) + 360.f, 360.f);

			LLVector3 object_axis;
			getObjectAxisClosestToMouse(object_axis);
			object_axis = object_axis * first_object_node->mSavedRotation;

			// project onto constraint plane
			object_axis = object_axis - (object_axis * getConstraintAxis()) * getConstraintAxis();
			object_axis.normVec();

			if (relative_mouse_angle < SNAP_ANGLE_DETENTE)
			{
				F32 quantized_mouse_angle = mouse_angle - (relative_mouse_angle - (SNAP_ANGLE_DETENTE * 0.5f));
				angle = (quantized_mouse_angle * DEG_TO_RAD) - atan2(object_axis * axis1, object_axis * axis2);
			}
			else
			{
				angle = (mouse_angle * DEG_TO_RAD) - atan2(object_axis * axis1, object_axis * axis2);
			}
			return LLQuaternion( -angle, constraint_axis );
		}
		else
		{
			if (mInSnapRegime)
			{
				mSmoothRotate = TRUE;
			}
			mInSnapRegime = FALSE;

			LLVector3 up_from_axis = mCenterToCamNorm % constraint_axis;
			up_from_axis.normVec();
			LLVector3 cur_intersection;
			getMousePointOnPlaneAgent(cur_intersection, x, y, center, mCenterToCam);
			cur_intersection -= center;
			mMouseCur = projected_vec(cur_intersection, up_from_axis);
			F32 mouse_depth = SNAP_GUIDE_INNER_RADIUS * mRadiusMeters;
			F32 mouse_dist_sqrd = mMouseCur.magVecSquared();
			if (mouse_dist_sqrd > 0.0001f)
			{
				mouse_depth = sqrtf((SNAP_GUIDE_INNER_RADIUS * mRadiusMeters) * (SNAP_GUIDE_INNER_RADIUS * mRadiusMeters) - 
									mouse_dist_sqrd);
			}
			LLVector3 projected_center_to_cam = mCenterToCamNorm - projected_vec(mCenterToCamNorm, constraint_axis);
			mMouseCur += mouse_depth * projected_center_to_cam;

			F32 dist = (cur_intersection * up_from_axis) - (mMouseDown * up_from_axis);
			angle = dist / (SNAP_GUIDE_INNER_RADIUS * mRadiusMeters) * -F_PI_BY_TWO;
		}
	}
	else
	{
		LLVector3 projected_mouse;
		getMousePointOnPlaneAgent(projected_mouse, x, y, center, constraint_axis);
		projected_mouse -= center;
		mMouseCur = projected_mouse;
		mMouseCur.normVec();

		if (!first_object_node)
		{
			return LLQuaternion::DEFAULT;
		}

		if (gSavedSettings.getBOOL("SnapEnabled") && projected_mouse.magVec() > SNAP_GUIDE_INNER_RADIUS * mRadiusMeters)
		{
			if (!mInSnapRegime)
			{
				mSmoothRotate = TRUE;
			}
			mInSnapRegime = TRUE;
			// 0 to 360 deg
			F32 mouse_angle = fmodf(atan2(projected_mouse * axis1, projected_mouse * axis2) * RAD_TO_DEG + 360.f, 360.f);
			
			F32 relative_mouse_angle = fmodf(mouse_angle + (SNAP_ANGLE_DETENTE / 2), SNAP_ANGLE_INCREMENT);
			//fmodf(llround(mouse_angle * RAD_TO_DEG, 7.5f) + 360.f, 360.f);

			LLVector3 object_axis;
			getObjectAxisClosestToMouse(object_axis);
			object_axis = object_axis * first_object_node->mSavedRotation;

			// project onto constraint plane
			object_axis = object_axis - (object_axis * getConstraintAxis()) * getConstraintAxis();
			object_axis.normVec();

			if (relative_mouse_angle < SNAP_ANGLE_DETENTE)
			{
				F32 quantized_mouse_angle = mouse_angle - (relative_mouse_angle - (SNAP_ANGLE_DETENTE * 0.5f));
				angle = (quantized_mouse_angle * DEG_TO_RAD) - atan2(object_axis * axis1, object_axis * axis2);
			}
			else
			{
				angle = (mouse_angle * DEG_TO_RAD) - atan2(object_axis * axis1, object_axis * axis2);
			}
			return LLQuaternion( -angle, constraint_axis );
		}
		else
		{
			if (mInSnapRegime)
			{
				mSmoothRotate = TRUE;
			}
			mInSnapRegime = FALSE;
		}

		angle = acos(mMouseCur * mMouseDown);

		F32 dir = (mMouseDown % mMouseCur) * constraint_axis;  // cross product
		if( dir < 0.f )
		{
			angle *= -1.f;
		}
	}

	F32 rot_step = gSavedSettings.getF32("RotationStep");
	F32 step_size = DEG_TO_RAD * rot_step;
	angle -= fmod(angle, step_size);
		
	return LLQuaternion( angle, constraint_axis );
}



LLVector3 LLManipRotate::intersectMouseWithSphere( S32 x, S32 y, const LLVector3& sphere_center, F32 sphere_radius)
{
	LLVector3 ray_pt;
	LLVector3 ray_dir;
	mouseToRay( x, y, &ray_pt, &ray_dir);
	return intersectRayWithSphere( ray_pt, ray_dir, sphere_center, sphere_radius );
}

LLVector3 LLManipRotate::intersectRayWithSphere( const LLVector3& ray_pt, const LLVector3& ray_dir, const LLVector3& sphere_center, F32 sphere_radius)
{
	LLVector3 ray_pt_to_center = sphere_center - ray_pt;
	F32 center_distance = ray_pt_to_center.normVec();

	F32 dot = ray_dir * ray_pt_to_center;

	if (dot == 0.f)
	{
		return LLVector3::zero;
	}

	// point which ray hits plane centered on sphere origin, facing ray origin
	LLVector3 intersection_sphere_plane = ray_pt + (ray_dir * center_distance / dot); 
	// vector from sphere origin to the point, normalized to sphere radius
	LLVector3 sphere_center_to_intersection = (intersection_sphere_plane - sphere_center) / sphere_radius;

	F32 dist_squared = sphere_center_to_intersection.magVecSquared();
	LLVector3 result;

	if (dist_squared > 1.f)
	{
		result = sphere_center_to_intersection;
		result.normVec();
	}
	else
	{
		result = sphere_center_to_intersection - ray_dir * sqrt(1.f - dist_squared);
	}

	return result;
}

// Utility function.  Should probably be moved to another class.
//static
void LLManipRotate::mouseToRay( S32 x, S32 y, LLVector3* ray_pt, LLVector3* ray_dir )
{
	if (LLSelectMgr::getInstance()->getSelection()->getSelectType() == SELECT_TYPE_HUD)
	{
		F32 mouse_x = (((F32)x / gViewerWindow->getWorldViewRectScaled().getWidth()) - 0.5f) / gAgentCamera.mHUDCurZoom;
		F32 mouse_y = ((((F32)y) / gViewerWindow->getWorldViewRectScaled().getHeight()) - 0.5f) / gAgentCamera.mHUDCurZoom;

		*ray_pt = LLVector3(-1.f, -mouse_x, mouse_y);
		*ray_dir = LLVector3(1.f, 0.f, 0.f);
	}
	else
	{
		*ray_pt = gAgentCamera.getCameraPositionAgent();
		LLViewerCamera::getInstance()->projectScreenToPosAgent(x, y, ray_dir);
		*ray_dir -= *ray_pt;
		ray_dir->normVec();
	}
}

void LLManipRotate::highlightManipulators( S32 x, S32 y )
{
	mHighlightedPart = LL_NO_PART;

	//LLBBox bbox = LLSelectMgr::getInstance()->getBBoxOfSelection();
	LLViewerObject *first_object = mObjectSelection->getFirstMoveableObject(TRUE);
	
	if (!first_object)
	{
		return;
	}
	
	LLQuaternion object_rot = first_object->getRenderRotation();
	LLVector3 rotation_center = gAgent.getPosAgentFromGlobal(mRotationCenter);
	LLVector3 mouse_dir_x;
	LLVector3 mouse_dir_y;
	LLVector3 mouse_dir_z;
	LLVector3 intersection_roll;

	LLVector3 grid_origin;
	LLVector3 grid_scale;
	LLQuaternion grid_rotation;

	LLSelectMgr::getInstance()->getGrid(grid_origin, grid_rotation, grid_scale);

	LLVector3 rot_x_axis = LLVector3::x_axis * grid_rotation;
	LLVector3 rot_y_axis = LLVector3::y_axis * grid_rotation;
	LLVector3 rot_z_axis = LLVector3::z_axis * grid_rotation;

	F32 proj_rot_x_axis = llabs(rot_x_axis * mCenterToCamNorm);
	F32 proj_rot_y_axis = llabs(rot_y_axis * mCenterToCamNorm);
	F32 proj_rot_z_axis = llabs(rot_z_axis * mCenterToCamNorm);

	F32 min_select_distance = 0.f;
	F32 cur_select_distance = 0.f;

	// test x
	getMousePointOnPlaneAgent(mouse_dir_x, x, y, rotation_center, rot_x_axis);
	mouse_dir_x -= rotation_center;
	// push intersection point out when working at obtuse angle to make ring easier to hit
	mouse_dir_x *= 1.f + (1.f - llabs(rot_x_axis * mCenterToCamNorm)) * 0.1f;

	// test y
	getMousePointOnPlaneAgent(mouse_dir_y, x, y, rotation_center, rot_y_axis);
	mouse_dir_y -= rotation_center;
	mouse_dir_y *= 1.f + (1.f - llabs(rot_y_axis * mCenterToCamNorm)) * 0.1f;

	// test z
	getMousePointOnPlaneAgent(mouse_dir_z, x, y, rotation_center, rot_z_axis);
	mouse_dir_z -= rotation_center;
	mouse_dir_z *= 1.f + (1.f - llabs(rot_z_axis * mCenterToCamNorm)) * 0.1f;

	// test roll
	getMousePointOnPlaneAgent(intersection_roll, x, y, rotation_center, mCenterToCamNorm);
	intersection_roll -= rotation_center;

	F32 dist_x = mouse_dir_x.normVec();
	F32 dist_y = mouse_dir_y.normVec();
	F32 dist_z = mouse_dir_z.normVec();

	F32 distance_threshold = (MAX_MANIP_SELECT_DISTANCE * mRadiusMeters) / gViewerWindow->getWorldViewHeightScaled();

	if (llabs(dist_x - mRadiusMeters) * llmax(0.05f, proj_rot_x_axis) < distance_threshold)
	{
		// selected x
		cur_select_distance = dist_x * mouse_dir_x * mCenterToCamNorm;
		if (cur_select_distance >= -0.05f && (min_select_distance == 0.f || cur_select_distance > min_select_distance))
		{
			min_select_distance = cur_select_distance;
			mHighlightedPart = LL_ROT_X;
		}
	}
	if (llabs(dist_y - mRadiusMeters) * llmax(0.05f, proj_rot_y_axis) < distance_threshold)
	{
		// selected y
		cur_select_distance = dist_y * mouse_dir_y * mCenterToCamNorm;
		if (cur_select_distance >= -0.05f && (min_select_distance == 0.f || cur_select_distance > min_select_distance))
		{
			min_select_distance = cur_select_distance;
			mHighlightedPart = LL_ROT_Y;
		}
	}
	if (llabs(dist_z - mRadiusMeters) * llmax(0.05f, proj_rot_z_axis) < distance_threshold)
	{
		// selected z
		cur_select_distance = dist_z * mouse_dir_z * mCenterToCamNorm;
		if (cur_select_distance >= -0.05f && (min_select_distance == 0.f || cur_select_distance > min_select_distance))
		{
			min_select_distance = cur_select_distance;
			mHighlightedPart = LL_ROT_Z;
		}
	}

	// test for edge-on intersections
	if (proj_rot_x_axis < 0.05f)
	{
		if ((proj_rot_y_axis > 0.05f && (dist_y * llabs(mouse_dir_y * rot_x_axis) < distance_threshold) && dist_y < mRadiusMeters) ||
			(proj_rot_z_axis > 0.05f && (dist_z * llabs(mouse_dir_z * rot_x_axis) < distance_threshold) && dist_z < mRadiusMeters))
		{
			mHighlightedPart = LL_ROT_X;
		}
	}

	if (proj_rot_y_axis < 0.05f)
	{
		if ((proj_rot_x_axis > 0.05f && (dist_x * llabs(mouse_dir_x * rot_y_axis) < distance_threshold) && dist_x < mRadiusMeters) ||
			(proj_rot_z_axis > 0.05f && (dist_z * llabs(mouse_dir_z * rot_y_axis) < distance_threshold) && dist_z < mRadiusMeters))
		{
			mHighlightedPart = LL_ROT_Y;
		}
	}

	if (proj_rot_z_axis < 0.05f)
	{
		if ((proj_rot_x_axis > 0.05f && (dist_x * llabs(mouse_dir_x * rot_z_axis) < distance_threshold) && dist_x < mRadiusMeters) ||
			(proj_rot_y_axis > 0.05f && (dist_y * llabs(mouse_dir_y * rot_z_axis) < distance_threshold) && dist_y < mRadiusMeters))
		{
			mHighlightedPart = LL_ROT_Z;
		}
	}

	// test for roll
	if (mHighlightedPart == LL_NO_PART)
	{
		F32 roll_distance = intersection_roll.magVec();
		F32 width_meters = WIDTH_PIXELS * mRadiusMeters / RADIUS_PIXELS;

		// use larger distance threshold for roll as it is checked only if something else wasn't highlighted
		if (llabs(roll_distance - (mRadiusMeters + (width_meters * 2.f))) < distance_threshold * 2.f)
		{
			mHighlightedPart = LL_ROT_ROLL;
		}
		else if (roll_distance < mRadiusMeters)
		{
			mHighlightedPart = LL_ROT_GENERAL;
		}
	}
}

S32 LLManipRotate::getObjectAxisClosestToMouse(LLVector3& object_axis)
{
	LLSelectNode* first_object_node = mObjectSelection->getFirstMoveableNode(TRUE);

	if (!first_object_node)
	{
		object_axis.clearVec();
		return -1;
	}

	LLQuaternion obj_rotation = first_object_node->mSavedRotation;
	LLVector3 mouse_down_object = mMouseDown * ~obj_rotation;
	LLVector3 mouse_down_abs = mouse_down_object;
	mouse_down_abs.abs();

	S32 axis_index = 0;
	if (mouse_down_abs.mV[VX] > mouse_down_abs.mV[VY] && mouse_down_abs.mV[VX] > mouse_down_abs.mV[VZ])
	{
		if (mouse_down_object.mV[VX] > 0.f)
		{
			object_axis = LLVector3::x_axis;
		}
		else
		{
			object_axis = LLVector3::x_axis_neg;
		}
		axis_index = VX;
	}
	else if (mouse_down_abs.mV[VY] > mouse_down_abs.mV[VZ])
	{
		if (mouse_down_object.mV[VY] > 0.f)
		{
			object_axis = LLVector3::y_axis;
		}
		else
		{
			object_axis = LLVector3::y_axis_neg;
		}
		axis_index = VY;
	}
	else
	{
		if (mouse_down_object.mV[VZ] > 0.f)
		{
			object_axis = LLVector3::z_axis;
		}
		else
		{
			object_axis = LLVector3::z_axis_neg;
		}
		axis_index = VZ;
	}

	return axis_index;
}

//virtual
BOOL LLManipRotate::canAffectSelection()
{
	BOOL can_rotate = mObjectSelection->getObjectCount() != 0;
	if (can_rotate)
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
		can_rotate = mObjectSelection->applyToObjects(&func);
	}
	return can_rotate;
}

