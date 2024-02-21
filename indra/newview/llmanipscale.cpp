/** 
 * @file llmanipscale.cpp
 * @brief LLManipScale class implementation
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

#include "llmanipscale.h"

// library includes
#include "llmath.h"
#include "v3math.h"
#include "llquaternion.h"
#include "llgl.h"
#include "llrender.h"
#include "v4color.h"
#include "llprimitive.h"

// viewer includes
#include "llagent.h"
#include "llagentcamera.h"
#include "llbbox.h"
#include "llbox.h"
#include "llviewercontrol.h"
#include "llcriticaldamp.h"
#include "lldrawable.h"
#include "llfloatertools.h"
#include "llglheaders.h"
#include "llselectmgr.h"
#include "llstatusbar.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llhudrender.h"
#include "llworld.h"
#include "v2math.h"
#include "llvoavatar.h"
#include "llmeshrepository.h"
#include "lltrans.h"

const F32 MAX_MANIP_SELECT_DISTANCE_SQUARED = 11.f * 11.f;
const F32 SNAP_GUIDE_SCREEN_OFFSET = 0.05f;
const F32 SNAP_GUIDE_SCREEN_LENGTH = 0.7f;
const F32 SELECTED_MANIPULATOR_SCALE = 1.2f;
const F32 MANIPULATOR_SCALE_HALF_LIFE = 0.07f;

const LLManip::EManipPart MANIPULATOR_IDS[LLManipScale::NUM_MANIPULATORS] = 
{
	LLManip::LL_CORNER_NNN,
	LLManip::LL_CORNER_NNP,
	LLManip::LL_CORNER_NPN,
	LLManip::LL_CORNER_NPP,
	LLManip::LL_CORNER_PNN,
	LLManip::LL_CORNER_PNP,
	LLManip::LL_CORNER_PPN,
	LLManip::LL_CORNER_PPP,
	LLManip::LL_FACE_POSZ,
	LLManip::LL_FACE_POSX,
	LLManip::LL_FACE_POSY,
	LLManip::LL_FACE_NEGX,
	LLManip::LL_FACE_NEGY,
	LLManip::LL_FACE_NEGZ
};

F32 get_default_max_prim_scale(bool is_flora)
{
	// a bit of a hack, but if it's foilage, we don't want to use the
	// new larger scale which would result in giant trees and grass
	if (gMeshRepo.meshRezEnabled() &&
		!is_flora)
	{
		return DEFAULT_MAX_PRIM_SCALE;
	}
	else
	{
		return DEFAULT_MAX_PRIM_SCALE_NO_MESH;
	}
}

// static
void LLManipScale::setUniform(bool b)
{
	gSavedSettings.setBOOL("ScaleUniform", b);
}

// static
void LLManipScale::setShowAxes(bool b)
{
	gSavedSettings.setBOOL("ScaleShowAxes", b);
}

// static
void LLManipScale::setStretchTextures(bool b)
{
	gSavedSettings.setBOOL("ScaleStretchTextures", b);
}

// static
bool LLManipScale::getUniform()
{
	return gSavedSettings.getBOOL("ScaleUniform");
}

// static
bool LLManipScale::getShowAxes()
{
	return gSavedSettings.getBOOL("ScaleShowAxes");
}

// static
bool LLManipScale::getStretchTextures()
{
	return gSavedSettings.getBOOL("ScaleStretchTextures");
}

inline void LLManipScale::conditionalHighlight( U32 part, const LLColor4* highlight, const LLColor4* normal )
{
	LLColor4 default_highlight( 1.f, 1.f, 1.f, 1.f );
	LLColor4 default_normal( 0.7f, 0.7f, 0.7f, 0.6f );
	LLColor4 invisible(0.f, 0.f, 0.f, 0.f);

	for (S32 i = 0; i < NUM_MANIPULATORS; i++)
	{
		if((U32)MANIPULATOR_IDS[i] == part)
		{
			mScaledBoxHandleSize = mManipulatorScales[i] * mBoxHandleSize[i];
			break;
		}
	}

	if (mManipPart != (S32)LL_NO_PART && mManipPart != (S32)part)
	{
		gGL.color4fv( invisible.mV );
	}
	else if( mHighlightedPart == (S32)part )
	{
		gGL.color4fv( highlight ? highlight->mV : default_highlight.mV );
	}
	else
	{
		gGL.color4fv( normal ? normal->mV : default_normal.mV  );
	}
}

void LLManipScale::handleSelect()
{
	LLBBox bbox = LLSelectMgr::getInstance()->getBBoxOfSelection();
	updateSnapGuides(bbox);
	LLSelectMgr::getInstance()->saveSelectedObjectTransform(SELECT_ACTION_TYPE_PICK);
    if (gFloaterTools)
    {
        gFloaterTools->setStatusText("scale");
    }
	LLManip::handleSelect();
}

LLManipScale::LLManipScale( LLToolComposite* composite )
	:
	LLManip( std::string("Scale"), composite ),
	mScaledBoxHandleSize( 1.f ),
	mLastMouseX( -1 ),
	mLastMouseY( -1 ),
	mSendUpdateOnMouseUp( false ),
	mLastUpdateFlags( 0 ),
	mScaleSnapUnit1(1.f),
	mScaleSnapUnit2(1.f),
	mSnapRegimeOffset(0.f),
	mTickPixelSpacing1(0.f),
	mTickPixelSpacing2(0.f),
	mSnapGuideLength(0.f),
	mSnapRegime(SNAP_REGIME_NONE),
	mScaleSnappedValue(0.f)
{
	for (S32 i = 0; i < NUM_MANIPULATORS; i++)
	{
		mManipulatorScales[i] = 1.f;
		mBoxHandleSize[i]     = 1.f;
	}
}

LLManipScale::~LLManipScale()
{
	for_each(mProjectedManipulators.begin(), mProjectedManipulators.end(), DeletePointer());
}

void LLManipScale::render()
{
	LLGLSUIDefault gls_ui;
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	LLGLDepthTest gls_depth(GL_TRUE);
	LLGLEnable gl_blend(GL_BLEND);
	LLBBox bbox = LLSelectMgr::getInstance()->getBBoxOfSelection();

	if( canAffectSelection() )
	{
		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gGL.pushMatrix();
		if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
		{
			F32 zoom = gAgentCamera.mHUDCurZoom;
			gGL.scalef(zoom, zoom, zoom);
		}

		////////////////////////////////////////////////////////////////////////
		// Calculate size of drag handles

		const F32 BOX_HANDLE_BASE_SIZE		= 50.0f;   // box size in pixels = BOX_HANDLE_BASE_SIZE * BOX_HANDLE_BASE_FACTOR
		const F32 BOX_HANDLE_BASE_FACTOR	= 0.2f;

		//Assume that UI scale factor is equivalent for X and Y axis
		F32 ui_scale_factor = LLUI::getScaleFactor().mV[VX];

		if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
		{
			for (S32 i = 0; i < NUM_MANIPULATORS; i++)
			{
				mBoxHandleSize[i] = BOX_HANDLE_BASE_SIZE * BOX_HANDLE_BASE_FACTOR / (F32) LLViewerCamera::getInstance()->getViewHeightInPixels();
				mBoxHandleSize[i] /= gAgentCamera.mHUDCurZoom;
				mBoxHandleSize[i] *= ui_scale_factor;
			}
		}
		else
		{
			for (S32 i = 0; i < NUM_MANIPULATORS; i++)
			{
				LLVector3 manipulator_pos = bbox.localToAgent(unitVectorToLocalBBoxExtent(partToUnitVector(MANIPULATOR_IDS[i]), bbox));
				F32 range_squared = dist_vec_squared(gAgentCamera.getCameraPositionAgent(), manipulator_pos);
				F32 range_from_agent_squared = dist_vec_squared(gAgent.getPositionAgent(), manipulator_pos);

				// Don't draw manip if object too far away
				if (gSavedSettings.getBOOL("LimitSelectDistance"))
				{
					F32 max_select_distance = gSavedSettings.getF32("MaxSelectDistance");
					if (range_from_agent_squared > max_select_distance * max_select_distance)
					{
						return;
					}
				}

				if (range_squared > 0.001f * 0.001f)
				{
					// range != zero
					F32 fraction_of_fov = BOX_HANDLE_BASE_SIZE / (F32) LLViewerCamera::getInstance()->getViewHeightInPixels();
					F32 apparent_angle = fraction_of_fov * LLViewerCamera::getInstance()->getView();  // radians
					mBoxHandleSize[i] = (F32) sqrtf(range_squared) * tan(apparent_angle) * BOX_HANDLE_BASE_FACTOR;
				}
				else
				{
					// range == zero
					mBoxHandleSize[i] = BOX_HANDLE_BASE_FACTOR;
				}
				mBoxHandleSize[i] *= ui_scale_factor;
			}
		}

		////////////////////////////////////////////////////////////////////////
		// Draw bounding box

		LLVector3 pos_agent = bbox.getPositionAgent();
		LLQuaternion rot = bbox.getRotation();

		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gGL.pushMatrix();
		{
			gGL.translatef(pos_agent.mV[VX], pos_agent.mV[VY], pos_agent.mV[VZ]);

			F32 angle_radians, x, y, z;
			rot.getAngleAxis(&angle_radians, &x, &y, &z);
			gGL.rotatef(angle_radians * RAD_TO_DEG, x, y, z);


			{
				LLGLEnable poly_offset(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset( -2.f, -2.f);

				renderCorners( bbox );
				renderFaces( bbox );

				if (mManipPart != LL_NO_PART)
				{
					renderGuidelinesPart( bbox );
				}

				glPolygonOffset( 0.f, 0.f);
			}
		}
		gGL.popMatrix();

		if (mManipPart != LL_NO_PART)
		{
			renderSnapGuides(bbox);
		}
		gGL.popMatrix();

		renderXYZ(bbox.getExtentLocal());
	}
}

bool LLManipScale::handleMouseDown(S32 x, S32 y, MASK mask)
{
	bool	handled = false;

	if(mHighlightedPart != LL_NO_PART)
	{
		handled = handleMouseDownOnPart( x, y, mask );
	}

	return handled;
}

// Assumes that one of the arrows on an object was hit.
bool LLManipScale::handleMouseDownOnPart( S32 x, S32 y, MASK mask )
{
	bool can_scale = canAffectSelection();
	if (!can_scale)
	{
		return false;
	}

	highlightManipulators(x, y);
	S32 hit_part = mHighlightedPart;

	LLSelectMgr::getInstance()->enableSilhouette(false);
	mManipPart = (EManipPart)hit_part;

	LLBBox bbox = LLSelectMgr::getInstance()->getBBoxOfSelection();
	LLVector3 box_center_agent = bbox.getCenterAgent();
	LLVector3 box_corner_agent = bbox.localToAgent( unitVectorToLocalBBoxExtent( partToUnitVector( mManipPart ), bbox ) );

	updateSnapGuides(bbox);

	mFirstClickX = x;
	mFirstClickY = y;
	mIsFirstClick = true;

	mDragStartPointGlobal = gAgent.getPosGlobalFromAgent(box_corner_agent);
	mDragStartCenterGlobal = gAgent.getPosGlobalFromAgent(box_center_agent);
	LLVector3 far_corner_agent = bbox.localToAgent( unitVectorToLocalBBoxExtent( -1.f * partToUnitVector( mManipPart ), bbox ) );
	mDragFarHitGlobal = gAgent.getPosGlobalFromAgent(far_corner_agent);
	mDragPointGlobal = mDragStartPointGlobal;

	// we just started a drag, so save initial object positions, orientations, and scales
	LLSelectMgr::getInstance()->saveSelectedObjectTransform(SELECT_ACTION_TYPE_SCALE);
	// Route future Mouse messages here preemptively.  (Release on mouse up.)
	setMouseCapture( true );

	mHelpTextTimer.reset();
	sNumTimesHelpTextShown++;
	return true;
}


bool LLManipScale::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// first, perform normal processing in case this was a quick-click
	handleHover(x, y, mask);

	if( hasMouseCapture() )
	{
		if( (LL_FACE_MIN <= (S32)mManipPart)
			&& ((S32)mManipPart <= LL_FACE_MAX) )
		{
			sendUpdates(true,true,false);
		}
		else
		if( (LL_CORNER_MIN <= (S32)mManipPart)
			&& ((S32)mManipPart <= LL_CORNER_MAX) )
		{
			sendUpdates(true,true,true);
		}

		//send texture update
		LLSelectMgr::getInstance()->adjustTexturesByScale(true, getStretchTextures());

		LLSelectMgr::getInstance()->enableSilhouette(true);
		mManipPart = LL_NO_PART;

		// Might have missed last update due to UPDATE_DELAY timing
		LLSelectMgr::getInstance()->sendMultipleUpdate( mLastUpdateFlags );

		//gAgent.setObjectTracking(gSavedSettings.getBOOL("TrackFocusObject"));
		LLSelectMgr::getInstance()->saveSelectedObjectTransform(SELECT_ACTION_TYPE_PICK);
	}
	return LLManip::handleMouseUp(x, y, mask);
}


bool LLManipScale::handleHover(S32 x, S32 y, MASK mask)
{
	if( hasMouseCapture() )
	{
		if( mObjectSelection->isEmpty() )
		{
			// Somehow the object got deselected while we were dragging it.
			setMouseCapture( false );
		}
		else
		{
			if((mFirstClickX != x) || (mFirstClickY != y))
			{
				mIsFirstClick = false;
			}

			if(!mIsFirstClick)
			{
				drag( x, y );
			}
		}
		LL_DEBUGS("UserInput") << "hover handled by LLManipScale (active)" << LL_ENDL;		
	}
	else
	{
		mSnapRegime = SNAP_REGIME_NONE;
		// not dragging...
		highlightManipulators(x, y);
	}

	// Patch up textures, if possible.
	LLSelectMgr::getInstance()->adjustTexturesByScale(false, getStretchTextures());

	gViewerWindow->setCursor(UI_CURSOR_TOOLSCALE);
	return true;
}

void LLManipScale::highlightManipulators(S32 x, S32 y)
{
	mHighlightedPart = LL_NO_PART;

	// If we have something selected, try to hit its manipulator handles.
	// Don't do this with nothing selected, as it kills the framerate.
	LLBBox bbox = LLSelectMgr::getInstance()->getBBoxOfSelection();

	if( canAffectSelection() )
	{
		LLMatrix4 transform;
		if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
		{
			LLVector4 translation(bbox.getPositionAgent());
			transform.initRotTrans(bbox.getRotation(), translation);
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
			LLMatrix4 projMatrix = LLViewerCamera::getInstance()->getProjection();
			LLMatrix4 modelView = LLViewerCamera::getInstance()->getModelview();
			transform.initAll(LLVector3(1.f, 1.f, 1.f), bbox.getRotation(), bbox.getPositionAgent());

			transform *= modelView;
			transform *= projMatrix;
		}

		LLVector3 min = bbox.getMinLocal();
		LLVector3 max = bbox.getMaxLocal();
		LLVector3 ctr = bbox.getCenterLocal();

		S32 numManips = 0;
		// corners
		mManipulatorVertices[numManips++] = LLVector4(min.mV[VX], min.mV[VY], min.mV[VZ], 1.f);
		mManipulatorVertices[numManips++] = LLVector4(min.mV[VX], min.mV[VY], max.mV[VZ], 1.f);
		mManipulatorVertices[numManips++] = LLVector4(min.mV[VX], max.mV[VY], min.mV[VZ], 1.f);
		mManipulatorVertices[numManips++] = LLVector4(min.mV[VX], max.mV[VY], max.mV[VZ], 1.f);
		mManipulatorVertices[numManips++] = LLVector4(max.mV[VX], min.mV[VY], min.mV[VZ], 1.f);
		mManipulatorVertices[numManips++] = LLVector4(max.mV[VX], min.mV[VY], max.mV[VZ], 1.f);
		mManipulatorVertices[numManips++] = LLVector4(max.mV[VX], max.mV[VY], min.mV[VZ], 1.f);
		mManipulatorVertices[numManips++] = LLVector4(max.mV[VX], max.mV[VY], max.mV[VZ], 1.f);

		// 1-D highlights are applicable iff one object is selected
		if( mObjectSelection->getObjectCount() == 1 )
		{
			// face centers
			mManipulatorVertices[numManips++] = LLVector4(ctr.mV[VX], ctr.mV[VY], max.mV[VZ], 1.f);
			mManipulatorVertices[numManips++] = LLVector4(max.mV[VX], ctr.mV[VY], ctr.mV[VZ], 1.f);
			mManipulatorVertices[numManips++] = LLVector4(ctr.mV[VX], max.mV[VY], ctr.mV[VZ], 1.f);
			mManipulatorVertices[numManips++] = LLVector4(min.mV[VX], ctr.mV[VY], ctr.mV[VZ], 1.f);
			mManipulatorVertices[numManips++] = LLVector4(ctr.mV[VX], min.mV[VY], ctr.mV[VZ], 1.f);
			mManipulatorVertices[numManips++] = LLVector4(ctr.mV[VX], ctr.mV[VY], min.mV[VZ], 1.f);
		}

		for_each(mProjectedManipulators.begin(), mProjectedManipulators.end(), DeletePointer());
		mProjectedManipulators.clear();

		for (S32 i = 0; i < numManips; i++)
		{
			LLVector4 projectedVertex = mManipulatorVertices[i] * transform;
			projectedVertex = projectedVertex * (1.f / projectedVertex.mV[VW]);

			ManipulatorHandle* projManipulator = new ManipulatorHandle(LLVector3(projectedVertex.mV[VX], projectedVertex.mV[VY],
				projectedVertex.mV[VZ]), MANIPULATOR_IDS[i], (i < 7) ? SCALE_MANIP_CORNER : SCALE_MANIP_FACE);
			mProjectedManipulators.insert(projManipulator);
		}

		LLRect world_view_rect = gViewerWindow->getWorldViewRectScaled();
		F32 half_width = (F32)world_view_rect.getWidth() / 2.f;
		F32 half_height = (F32)world_view_rect.getHeight() / 2.f;
		LLVector2 manip2d;
		LLVector2 mousePos((F32)x - half_width, (F32)y - half_height);
		LLVector2 delta;

		mHighlightedPart = LL_NO_PART;

		for (manipulator_list_t::iterator iter = mProjectedManipulators.begin();
			 iter != mProjectedManipulators.end(); ++iter)
		{
			ManipulatorHandle* manipulator = *iter;
			{
				manip2d.set(manipulator->mPosition.mV[VX] * half_width, manipulator->mPosition.mV[VY] * half_height);

				delta = manip2d - mousePos;
				if (delta.lengthSquared() < MAX_MANIP_SELECT_DISTANCE_SQUARED)
				{
					mHighlightedPart = manipulator->mManipID;

					//LL_INFOS() << "Tried: " << mHighlightedPart << LL_ENDL;
					break;
				}
			}
		}
	}

	for (S32 i = 0; i < NUM_MANIPULATORS; i++)
	{
		if (mHighlightedPart == MANIPULATOR_IDS[i])
		{
			mManipulatorScales[i] = lerp(mManipulatorScales[i], SELECTED_MANIPULATOR_SCALE, LLSmoothInterpolation::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE));
		}
		else
		{
			mManipulatorScales[i] = lerp(mManipulatorScales[i], 1.f, LLSmoothInterpolation::getInterpolant(MANIPULATOR_SCALE_HALF_LIFE));
		}
	}

	LL_DEBUGS("UserInput") << "hover handled by LLManipScale (inactive)" << LL_ENDL;
}


void LLManipScale::renderFaces( const LLBBox& bbox )
{
	// Don't bother to render the drag handles for 1-D scaling if
	// more than one object is selected or if it is an attachment
	if ( mObjectSelection->getObjectCount() > 1 )
	{
		return;
	}

	// This is a flattened representation of the box as render here
	//                                       .
	//              (+++)        (++-)      /|\t
	//                +------------+         | (texture coordinates)
	//                |            |         |
	//                |     1      |        (*) --->s
	//                |    +X      |   
	//                |            |
	// (+++)     (+-+)|            |(+--)     (++-)        (+++)
	//   +------------+------------+------------+------------+
	//   |0          3|3          7|7          4|4          0|
	//   |     0      |     4      |     5      |     2	     |
	//   |    +Z      |    -Y      |    -Z      |    +Y      |
	//   |	          |            |            |            |
	//   |1          2|2          6|6          5|5          1|
	//   +------------+------------+------------+------------+
	// (-++)     (--+)|            |(---)     (-+-)        (-++)
	//                |     3      |
	//                |    -X      |
	//                |            |
	//                |            |
	//                +------------+
	//              (-++)        (-+-)

	LLColor4 highlight_color( 1.f, 1.f, 1.f, 0.5f);
	LLColor4 normal_color(	1.f, 1.f, 1.f, 0.3f);

	LLColor4 x_highlight_color( 1.f, 0.2f, 0.2f, 1.0f);
	LLColor4 x_normal_color(	0.6f, 0.f, 0.f, 0.4f);

	LLColor4 y_highlight_color( 0.2f, 1.f, 0.2f, 1.0f);
	LLColor4 y_normal_color(	0.f, 0.6f, 0.f, 0.4f);

	LLColor4 z_highlight_color( 0.2f, 0.2f, 1.f, 1.0f);
	LLColor4 z_normal_color(	0.f, 0.f, 0.6f, 0.4f);

	LLColor4 default_normal_color( 0.7f, 0.7f, 0.7f, 0.15f );

	const LLVector3& min = bbox.getMinLocal();
	const LLVector3& max = bbox.getMaxLocal();
	LLVector3 ctr = bbox.getCenterLocal();

	if (mManipPart == LL_NO_PART)
	{
		gGL.color4fv( default_normal_color.mV );
		LLGLDepthTest gls_depth(GL_FALSE);
		gGL.begin(LLRender::QUADS);
		{
			// Face 0
			gGL.vertex3f(min.mV[VX], max.mV[VY], max.mV[VZ]);
			gGL.vertex3f(min.mV[VX], min.mV[VY], max.mV[VZ]);
			gGL.vertex3f(max.mV[VX], min.mV[VY], max.mV[VZ]);
			gGL.vertex3f(max.mV[VX], max.mV[VY], max.mV[VZ]);

			// Face 1
			gGL.vertex3f(max.mV[VX], min.mV[VY], max.mV[VZ]);
			gGL.vertex3f(max.mV[VX], min.mV[VY], min.mV[VZ]);
			gGL.vertex3f(max.mV[VX], max.mV[VY], min.mV[VZ]);
			gGL.vertex3f(max.mV[VX], max.mV[VY], max.mV[VZ]);

			// Face 2
			gGL.vertex3f(min.mV[VX], max.mV[VY], min.mV[VZ]);
			gGL.vertex3f(min.mV[VX], max.mV[VY], max.mV[VZ]);
			gGL.vertex3f(max.mV[VX], max.mV[VY], max.mV[VZ]);
			gGL.vertex3f(max.mV[VX], max.mV[VY], min.mV[VZ]);

			// Face 3
			gGL.vertex3f(min.mV[VX], max.mV[VY], max.mV[VZ]);
			gGL.vertex3f(min.mV[VX], max.mV[VY], min.mV[VZ]);
			gGL.vertex3f(min.mV[VX], min.mV[VY], min.mV[VZ]);
			gGL.vertex3f(min.mV[VX], min.mV[VY], max.mV[VZ]);

			// Face 4
			gGL.vertex3f(min.mV[VX], min.mV[VY], max.mV[VZ]);
			gGL.vertex3f(min.mV[VX], min.mV[VY], min.mV[VZ]);
			gGL.vertex3f(max.mV[VX], min.mV[VY], min.mV[VZ]);
			gGL.vertex3f(max.mV[VX], min.mV[VY], max.mV[VZ]);

			// Face 5
			gGL.vertex3f(min.mV[VX], min.mV[VY], min.mV[VZ]);
			gGL.vertex3f(min.mV[VX], max.mV[VY], min.mV[VZ]);
			gGL.vertex3f(max.mV[VX], max.mV[VY], min.mV[VZ]);
			gGL.vertex3f(max.mV[VX], min.mV[VY], min.mV[VZ]);
		}
		gGL.end();
	}

	// Find nearest vertex
	LLVector3 orientWRTHead = bbox.agentToLocalBasis( bbox.getCenterAgent() - gAgentCamera.getCameraPositionAgent() );
	U32 nearest =
		(orientWRTHead.mV[0] < 0.0f ? 1 : 0) +
		(orientWRTHead.mV[1] < 0.0f ? 2 : 0) +
		(orientWRTHead.mV[2] < 0.0f ? 4 : 0);

	// opposite faces on Linden cubes:
	// 0 & 5
	// 1 & 3
	// 2 & 4

	// Table of order to draw faces, based on nearest vertex
	static U32 face_list[8][6] = {
		{ 2,0,1, 4,5,3 }, // v6  F201 F453
		{ 2,0,3, 4,5,1 }, // v7  F203 F451
		{ 4,0,1, 2,5,3 }, // v5  F401 F253
		{ 4,0,3, 2,5,1 }, // v4  F403 F251
		{ 2,5,1, 4,0,3 }, // v2  F251 F403
		{ 2,5,3, 4,0,1 }, // v3  F253 F401
		{ 4,5,1, 2,0,3 }, // v1  F451 F203
		{ 4,5,3, 2,0,1 }  // v0  F453 F201
	};

	{
		LLGLDepthTest gls_depth(GL_FALSE);

		for (S32 i = 0; i < 6; i++)
		{
			U32 face = face_list[nearest][i];
			switch( face )
			{
			  case 0:
				conditionalHighlight( LL_FACE_POSZ, &z_highlight_color, &z_normal_color );
				renderAxisHandle( 8, ctr, LLVector3( ctr.mV[VX], ctr.mV[VY], max.mV[VZ] ) );
				break;

			  case 1:
				conditionalHighlight( LL_FACE_POSX, &x_highlight_color, &x_normal_color );
				renderAxisHandle( 9, ctr, LLVector3( max.mV[VX], ctr.mV[VY], ctr.mV[VZ] ) );
				break;

			  case 2:
				conditionalHighlight( LL_FACE_POSY, &y_highlight_color, &y_normal_color );
				renderAxisHandle( 10, ctr, LLVector3( ctr.mV[VX], max.mV[VY], ctr.mV[VZ] ) );
				break;

			  case 3:
				conditionalHighlight( LL_FACE_NEGX, &x_highlight_color, &x_normal_color );
				renderAxisHandle( 11, ctr, LLVector3( min.mV[VX], ctr.mV[VY], ctr.mV[VZ] ) );
				break;

			  case 4:
				conditionalHighlight( LL_FACE_NEGY, &y_highlight_color, &y_normal_color );
				renderAxisHandle( 12, ctr, LLVector3( ctr.mV[VX], min.mV[VY], ctr.mV[VZ] ) );
				break;

			  case 5:
				conditionalHighlight( LL_FACE_NEGZ, &z_highlight_color, &z_normal_color );
				renderAxisHandle( 13, ctr, LLVector3( ctr.mV[VX], ctr.mV[VY], min.mV[VZ] ) );
				break;
			}
		}
	}
}

void LLManipScale::renderCorners( const LLBBox& bbox )
{
	U32 part = LL_CORNER_NNN;

	F32 x_offset = bbox.getMinLocal().mV[VX];
	for( S32 i=0; i < 2; i++ )
	{
		F32 y_offset = bbox.getMinLocal().mV[VY];
		for( S32 j=0; j < 2; j++ )
		{
			F32 z_offset = bbox.getMinLocal().mV[VZ];
			for( S32 k=0; k < 2; k++ )
			{
				conditionalHighlight( part );
				renderBoxHandle( x_offset, y_offset, z_offset );
				part++;

				z_offset = bbox.getMaxLocal().mV[VZ];

			}
			y_offset = bbox.getMaxLocal().mV[VY];
		}
		x_offset = bbox.getMaxLocal().mV[VX];
	}
}


void LLManipScale::renderBoxHandle( F32 x, F32 y, F32 z )
{
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	LLGLDepthTest gls_depth(GL_FALSE);
	//LLGLDisable gls_stencil(GL_STENCIL_TEST);

	gGL.pushMatrix();
	{
		gGL.translatef( x, y, z );
		gGL.scalef( mScaledBoxHandleSize, mScaledBoxHandleSize, mScaledBoxHandleSize );
		gBox.render();
	}
	gGL.popMatrix();
}


void LLManipScale::renderAxisHandle( U32 handle_index, const LLVector3& start, const LLVector3& end )
{
	if( getShowAxes() )
	{
		// Draws a single "jacks" style handle: a long, retangular box from start to end.
		LLVector3 offset_start = end - start;
		offset_start.normalize();
		offset_start = start + mBoxHandleSize[handle_index] * offset_start;

		LLVector3 delta = end - offset_start;
		LLVector3 pos = offset_start + 0.5f * delta;

		gGL.pushMatrix();
		{
			gGL.translatef( pos.mV[VX], pos.mV[VY], pos.mV[VZ] );
			gGL.scalef(
				mBoxHandleSize[handle_index] + llabs(delta.mV[VX]),
				mBoxHandleSize[handle_index] + llabs(delta.mV[VY]),
				mBoxHandleSize[handle_index] + llabs(delta.mV[VZ]));
			gBox.render();
		}
		gGL.popMatrix();
	}
	else
	{
		renderBoxHandle( end.mV[VX], end.mV[VY], end.mV[VZ] );
	}
}

// General scale call
void LLManipScale::drag( S32 x, S32 y )
{
	if( (LL_FACE_MIN <= (S32)mManipPart)
		&& ((S32)mManipPart <= LL_FACE_MAX) )
	{
		dragFace( x, y );
	}
	else if( (LL_CORNER_MIN <= (S32)mManipPart)
		&& ((S32)mManipPart <= LL_CORNER_MAX) )
	{
		dragCorner( x, y );
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
			selectNode->mLastScale = cur->getScale();
			selectNode->mLastPositionLocal = cur->getPosition();
		}
	}

	LLSelectMgr::getInstance()->updateSelectionCenter();
	gAgentCamera.clearFocusObject();
}

// Scale on three axis simultaneously
void LLManipScale::dragCorner( S32 x, S32 y )
{
	// Suppress scale if mouse hasn't moved.
	if (x == mLastMouseX && y == mLastMouseY)
	{
		return;
	}
	mLastMouseX = x;
	mLastMouseY = y;

	LLVector3 drag_start_point_agent = gAgent.getPosAgentFromGlobal(mDragStartPointGlobal);
	LLVector3 drag_start_center_agent = gAgent.getPosAgentFromGlobal(mDragStartCenterGlobal);

	LLVector3d drag_start_dir_d;
	drag_start_dir_d.set(mDragStartPointGlobal - mDragStartCenterGlobal);

	F32 s = 0;
	F32 t = 0;
	nearestPointOnLineFromMouse(x, y,
								drag_start_center_agent,
								drag_start_point_agent,
								s, t );

	if( s <= 0 )  // we only care about intersections in front of the camera
	{
		return;
	}
	mDragPointGlobal = lerp(mDragStartCenterGlobal, mDragStartPointGlobal, t);

	LLBBox bbox	     = LLSelectMgr::getInstance()->getBBoxOfSelection();
	F32 scale_factor = 1.f;
	F32 max_scale    = partToMaxScale(mManipPart, bbox);
	F32 min_scale    = partToMinScale(mManipPart, bbox);
	bool uniform     = LLManipScale::getUniform();

	// check for snapping
	LLVector3 mouse_on_plane1;
	getMousePointOnPlaneAgent(mouse_on_plane1, x, y, mScaleCenter, mScalePlaneNormal1);
	mouse_on_plane1 -= mScaleCenter;

	LLVector3 mouse_on_plane2;
	getMousePointOnPlaneAgent(mouse_on_plane2, x, y, mScaleCenter, mScalePlaneNormal2);
	mouse_on_plane2 -= mScaleCenter;

	LLVector3 projected_drag_pos1 = inverse_projected_vec(mScaleDir, orthogonal_component(mouse_on_plane1, mSnapGuideDir1));
	LLVector3 projected_drag_pos2 = inverse_projected_vec(mScaleDir, orthogonal_component(mouse_on_plane2, mSnapGuideDir2));

	bool snap_enabled = gSavedSettings.getBOOL("SnapEnabled");
	if (snap_enabled && (mouse_on_plane1 - projected_drag_pos1) * mSnapGuideDir1 > mSnapRegimeOffset)
	{
		F32 drag_dist = mScaleDir * projected_drag_pos1; // Projecting the drag position allows for negative results, vs using the length which will result in a "reverse scaling" bug.

		F32 cur_subdivisions = llclamp(getSubdivisionLevel(mScaleCenter + projected_drag_pos1, mScaleDir, mScaleSnapUnit1, mTickPixelSpacing1), sGridMinSubdivisionLevel, sGridMaxSubdivisionLevel);
		F32 snap_dist = mScaleSnapUnit1 / (2.f * cur_subdivisions);
		F32 relative_snap_dist = fmodf(drag_dist + snap_dist, mScaleSnapUnit1 / cur_subdivisions);

		mScaleSnappedValue = llclamp((drag_dist - (relative_snap_dist - snap_dist)), min_scale, max_scale);
		scale_factor = mScaleSnappedValue / dist_vec(drag_start_point_agent, drag_start_center_agent);
		mScaleSnappedValue /= mScaleSnapUnit1 * 2.f;
		mSnapRegime = SNAP_REGIME_UPPER;

		if (!uniform)
		{
			scale_factor *= 0.5f;
		}
	}
	else if (snap_enabled && (mouse_on_plane2 - projected_drag_pos2) * mSnapGuideDir2 > mSnapRegimeOffset )
	{
		F32 drag_dist = mScaleDir * projected_drag_pos2; // Projecting the drag position allows for negative results, vs using the length which will result in a "reverse scaling" bug.

		F32 cur_subdivisions = llclamp(getSubdivisionLevel(mScaleCenter + projected_drag_pos2, mScaleDir, mScaleSnapUnit2, mTickPixelSpacing2), sGridMinSubdivisionLevel, sGridMaxSubdivisionLevel);
		F32 snap_dist = mScaleSnapUnit2 / (2.f * cur_subdivisions);
		F32 relative_snap_dist = fmodf(drag_dist + snap_dist, mScaleSnapUnit2 / cur_subdivisions);

		mScaleSnappedValue = llclamp((drag_dist - (relative_snap_dist - snap_dist)), min_scale, max_scale);
		scale_factor = mScaleSnappedValue / dist_vec(drag_start_point_agent, drag_start_center_agent);
		mScaleSnappedValue /= mScaleSnapUnit2 * 2.f;
		mSnapRegime = SNAP_REGIME_LOWER;

		if (!uniform)
		{
			scale_factor *= 0.5f;
		}
	}
	else
	{
		mSnapRegime = SNAP_REGIME_NONE;
		scale_factor = t;
		if (!uniform)
		{
			scale_factor = 0.5f + (scale_factor * 0.5f);
		}
	}


	F32 max_scale_factor = get_default_max_prim_scale() / MIN_PRIM_SCALE;
	F32 min_scale_factor = MIN_PRIM_SCALE / get_default_max_prim_scale();

	// find max and min scale factors that will make biggest object hit max absolute scale and smallest object hit min absolute scale
	for (LLObjectSelection::iterator iter = mObjectSelection->begin();
		 iter != mObjectSelection->end(); iter++)
	{
		LLSelectNode* selectNode = *iter;
		LLViewerObject* cur = selectNode->getObject();
		LLViewerObject *root_object = (cur == NULL) ? NULL : cur->getRootEdit();
		if(  cur->permModify() && cur->permMove() && !cur->isPermanentEnforced() &&
			((root_object == NULL) || !root_object->isPermanentEnforced()) &&
			!cur->isAvatar() )
		{
			const LLVector3& scale = selectNode->mSavedScale;

			F32 cur_max_scale_factor = llmin( get_default_max_prim_scale(LLPickInfo::isFlora(cur)) / scale.mV[VX], get_default_max_prim_scale(LLPickInfo::isFlora(cur)) / scale.mV[VY], get_default_max_prim_scale(LLPickInfo::isFlora(cur)) / scale.mV[VZ] );
			max_scale_factor = llmin( max_scale_factor, cur_max_scale_factor );

			F32 cur_min_scale_factor = llmax( MIN_PRIM_SCALE / scale.mV[VX], MIN_PRIM_SCALE / scale.mV[VY], MIN_PRIM_SCALE / scale.mV[VZ] );
			min_scale_factor = llmax( min_scale_factor, cur_min_scale_factor );
		}
	}

	scale_factor = llclamp( scale_factor, min_scale_factor, max_scale_factor );

	LLVector3d drag_global = uniform ? mDragStartCenterGlobal : mDragFarHitGlobal;

	// do the root objects i.e. (true == cur->isRootEdit())
	for (LLObjectSelection::iterator iter = mObjectSelection->begin();
		 iter != mObjectSelection->end(); iter++)
	{
		LLSelectNode* selectNode = *iter;
		LLViewerObject* cur = selectNode->getObject();
		LLViewerObject *root_object = (cur == NULL) ? NULL : cur->getRootEdit();
		if( cur->permModify() && cur->permMove() && !cur->isPermanentEnforced() &&
			((root_object == NULL) || !root_object->isPermanentEnforced()) &&
			!cur->isAvatar() && cur->isRootEdit() )
		{
			const LLVector3& scale = selectNode->mSavedScale;
			cur->setScale( scale_factor * scale );

			LLVector3 delta_pos;
			LLVector3 original_pos = cur->getPositionEdit();
			LLVector3d new_pos_global = drag_global + (selectNode->mSavedPositionGlobal - drag_global) * scale_factor;
			if (!cur->isAttachment())
			{
				new_pos_global = LLWorld::getInstance()->clipToVisibleRegions(selectNode->mSavedPositionGlobal, new_pos_global);
			}
			cur->setPositionAbsoluteGlobal( new_pos_global );
			rebuild(cur);

			delta_pos = cur->getPositionEdit() - original_pos;

			if (selectNode->mIndividualSelection)
			{
				// counter-translate child objects if we are moving the root as an individual
				LLViewerObject::const_child_list_t& child_list = cur->getChildren();
				for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
					 iter != child_list.end(); iter++)
				{
					LLViewerObject* childp = *iter;

					if (cur->isAttachment())
					{
						LLVector3 child_pos = childp->getPosition() - (delta_pos * ~cur->getRotationEdit());
						childp->setPosition(child_pos);
					}
					else
					{
						LLVector3d child_pos_delta(delta_pos);
						// RN: this updates drawable position instantly
						childp->setPositionAbsoluteGlobal(childp->getPositionGlobal() - child_pos_delta);
					}
					rebuild(childp);
				}
			}
		}
	}
	// do the child objects i.e. (false == cur->isRootEdit())
	for (LLObjectSelection::iterator iter = mObjectSelection->begin();
		 iter != mObjectSelection->end(); iter++)
	{
		LLSelectNode* selectNode = *iter;
		LLViewerObject*cur = selectNode->getObject();
		LLViewerObject *root_object = (cur == NULL) ? NULL : cur->getRootEdit();
		if( cur->permModify() && cur->permMove() && !cur->isPermanentEnforced() &&
			((root_object == NULL) || !root_object->isPermanentEnforced()) &&
			!cur->isAvatar() && !cur->isRootEdit() )
		{
			const LLVector3& scale = selectNode->mSavedScale;
			cur->setScale( scale_factor * scale, false );

			if (!selectNode->mIndividualSelection)
			{
				cur->setPosition(selectNode->mSavedPositionLocal * scale_factor);
			}

			rebuild(cur);
		}
	}
}

// Scale on a single axis
void LLManipScale::dragFace( S32 x, S32 y )
{
	// Suppress scale if mouse hasn't moved.
	if (x == mLastMouseX && y == mLastMouseY)
	{
		return;
	}

	mLastMouseX = x;
	mLastMouseY = y;

	LLVector3d drag_start_point_global	= mDragStartPointGlobal;
	LLVector3d drag_start_center_global = mDragStartCenterGlobal;
	LLVector3 drag_start_point_agent = gAgent.getPosAgentFromGlobal(drag_start_point_global);
	LLVector3 drag_start_center_agent = gAgent.getPosAgentFromGlobal(drag_start_center_global);

	LLVector3d drag_start_dir_d;
	drag_start_dir_d.set(drag_start_point_global - drag_start_center_global);
	LLVector3 drag_start_dir_f;
	drag_start_dir_f.set(drag_start_dir_d);

	LLBBox bbox	= LLSelectMgr::getInstance()->getBBoxOfSelection();

	F32 s = 0;
	F32 t = 0;

	nearestPointOnLineFromMouse(x,
						y,
						drag_start_center_agent,
						drag_start_point_agent,
						s, t );

	if( s <= 0 )  // we only care about intersections in front of the camera
	{
		return;
	}

	LLVector3d drag_point_global = drag_start_center_global + t * drag_start_dir_d;
	LLVector3 part_dir_local	= partToUnitVector( mManipPart );

	// check for snapping
	LLVector3 mouse_on_plane;
	getMousePointOnPlaneAgent(mouse_on_plane, x, y, mScaleCenter, mScalePlaneNormal1 );

	LLVector3 mouse_on_scale_line = mScaleCenter + projected_vec(mouse_on_plane - mScaleCenter, mScaleDir);
	LLVector3 drag_delta(mouse_on_scale_line - drag_start_point_agent);
	F32 max_drag_dist = partToMaxScale(mManipPart, bbox);
	F32 min_drag_dist = partToMinScale(mManipPart, bbox);

	bool uniform = LLManipScale::getUniform();
	if( uniform )
	{
		drag_delta *= 2.f;
	}

	LLVector3 scale_center_to_mouse = mouse_on_plane - mScaleCenter;
	F32 dist_from_scale_line = dist_vec(scale_center_to_mouse, (mouse_on_scale_line - mScaleCenter));
	F32 dist_along_scale_line = scale_center_to_mouse * mScaleDir;

	bool snap_enabled = gSavedSettings.getBOOL("SnapEnabled");

	if (snap_enabled && dist_from_scale_line > mSnapRegimeOffset)
	{
		mSnapRegime = static_cast<ESnapRegimes>(SNAP_REGIME_UPPER | SNAP_REGIME_LOWER); // A face drag doesn't have split regimes.

		if (dist_along_scale_line > max_drag_dist)
		{
			mScaleSnappedValue = max_drag_dist;

			LLVector3 clamp_point = mScaleCenter + max_drag_dist * mScaleDir;
			drag_delta.set(clamp_point - drag_start_point_agent);
		}
		else if (dist_along_scale_line < min_drag_dist)
		{
			mScaleSnappedValue = min_drag_dist;

			LLVector3 clamp_point = mScaleCenter + min_drag_dist * mScaleDir;
			drag_delta.set(clamp_point - drag_start_point_agent);
		}
		else
		{
			F32 drag_dist = scale_center_to_mouse * mScaleDir;
			F32 cur_subdivisions = llclamp(getSubdivisionLevel(mScaleCenter + mScaleDir * drag_dist, mScaleDir, mScaleSnapUnit1, mTickPixelSpacing1), sGridMinSubdivisionLevel, sGridMaxSubdivisionLevel);
			F32 snap_dist = mScaleSnapUnit1 / (2.f * cur_subdivisions);
			F32 relative_snap_dist = fmodf(drag_dist + snap_dist, mScaleSnapUnit1 / cur_subdivisions);
			relative_snap_dist -= snap_dist;

			//make sure that values that the scale is "snapped to"
			//do not exceed/go under the applicable max/mins
			//this causes the box to shift displacements ever so slightly
			//although the "snap value" should go down to 0
			//see Jira 1027
			relative_snap_dist = llclamp(relative_snap_dist,
										 drag_dist - max_drag_dist,
										 drag_dist - min_drag_dist);

			mScaleSnappedValue = (drag_dist - relative_snap_dist) / (mScaleSnapUnit1 * 2.f);

			if (llabs(relative_snap_dist) < snap_dist)
			{
				LLVector3 drag_correction = relative_snap_dist * mScaleDir;
				if (uniform)
				{
					drag_correction *= 2.f;
				}

				drag_delta -= drag_correction;
			}
		}
	}
	else
	{
		mSnapRegime = SNAP_REGIME_NONE;
	}

	LLVector3 dir_agent;
	if( part_dir_local.mV[VX] )
	{
		dir_agent = bbox.localToAgentBasis( LLVector3::x_axis );
	}
	else if( part_dir_local.mV[VY] )
	{
		dir_agent = bbox.localToAgentBasis( LLVector3::y_axis );
	}
	else if( part_dir_local.mV[VZ] )
	{
		dir_agent = bbox.localToAgentBasis( LLVector3::z_axis );
	}
	stretchFace(
		projected_vec(drag_start_dir_f, dir_agent) + drag_start_center_agent,
		projected_vec(drag_delta, dir_agent));

	mDragPointGlobal = drag_point_global;
}

void LLManipScale::sendUpdates( bool send_position_update, bool send_scale_update, bool corner )
{
	// Throttle updates to 10 per second.
	static LLTimer	update_timer;
	F32 elapsed_time = update_timer.getElapsedTimeF32();
	const F32 UPDATE_DELAY = 0.1f;						//  min time between transmitted updates

	if( send_scale_update || send_position_update )
	{
		U32 update_flags = UPD_NONE;
		if (send_position_update)	update_flags |= UPD_POSITION;
		if (send_scale_update)		update_flags |= UPD_SCALE;

// 		bool send_type = SEND_INDIVIDUALS;
		if (corner)
		{
			update_flags |= UPD_UNIFORM;
		}
		// keep this up to date for sendonmouseup
		mLastUpdateFlags = update_flags;

		// enforce minimum update delay and don't stream updates on sub-object selections
		if( elapsed_time > UPDATE_DELAY && !gSavedSettings.getBOOL("EditLinkedParts") )
		{
			LLSelectMgr::getInstance()->sendMultipleUpdate( update_flags );
			update_timer.reset();
			mSendUpdateOnMouseUp = false;
		}
		else
		{
			mSendUpdateOnMouseUp = true;
		}
		dialog_refresh_all();
	}
}

// Rescales in a single dimension.  Either uniform (standard) or one-sided (scale plus translation)
// depending on mUniform.  Handles multiple selection and objects that are not aligned to the bounding box.
void LLManipScale::stretchFace( const LLVector3& drag_start_agent, const LLVector3& drag_delta_agent )
{
	LLVector3 drag_start_center_agent = gAgent.getPosAgentFromGlobal(mDragStartCenterGlobal);

	for (LLObjectSelection::iterator iter = mObjectSelection->begin();
		 iter != mObjectSelection->end(); iter++)
	{
		LLSelectNode* selectNode = *iter;
		LLViewerObject*cur = selectNode->getObject();
		LLViewerObject *root_object = (cur == NULL) ? NULL : cur->getRootEdit();
		if( cur->permModify() && cur->permMove() && !cur->isPermanentEnforced() &&
			((root_object == NULL) || !root_object->isPermanentEnforced()) &&
			!cur->isAvatar() )
		{
			LLBBox cur_bbox			= cur->getBoundingBoxAgent();
			LLVector3 start_local	= cur_bbox.agentToLocal( drag_start_agent );
			LLVector3 end_local		= cur_bbox.agentToLocal( drag_start_agent + drag_delta_agent);
			LLVector3 start_center_local = cur_bbox.agentToLocal( drag_start_center_agent );
			LLVector3 axis			= nearestAxis( start_local - start_center_local );
			S32 axis_index			= axis.mV[0] ? 0 : (axis.mV[1] ? 1 : 2 );

			LLVector3 delta_local	= end_local - start_local;
			F32 delta_local_mag		= delta_local.length();
			LLVector3 dir_local;
			if (delta_local_mag == 0.f)
			{
				dir_local = axis;
			}
			else
			{
				dir_local = delta_local / delta_local_mag; // normalized delta_local
			}

			F32 denom = axis * dir_local;
			F32 desired_delta_size	= is_approx_zero(denom) ? 0.f : (delta_local_mag / denom);  // in meters
			F32 desired_scale		= llclamp(selectNode->mSavedScale.mV[axis_index] + desired_delta_size, MIN_PRIM_SCALE, get_default_max_prim_scale(LLPickInfo::isFlora(cur)));
			// propagate scale constraint back to position offset
			desired_delta_size		= desired_scale - selectNode->mSavedScale.mV[axis_index]; // propagate constraint back to position

			LLVector3 scale			= cur->getScale();
			scale.mV[axis_index]	= desired_scale;
			cur->setScale(scale, false);
			rebuild(cur);
			LLVector3 delta_pos;
			if( !getUniform() )
			{
				LLVector3 delta_pos_local = axis * (0.5f * desired_delta_size);
				LLVector3d delta_pos_global;
				delta_pos_global.set(cur_bbox.localToAgent( delta_pos_local ) - cur_bbox.getCenterAgent());
				LLVector3 cur_pos = cur->getPositionEdit();

				if (cur->isRootEdit() && !cur->isAttachment())
				{
					LLVector3d new_pos_global = LLWorld::getInstance()->clipToVisibleRegions(selectNode->mSavedPositionGlobal, selectNode->mSavedPositionGlobal + delta_pos_global);
					cur->setPositionGlobal( new_pos_global );
				}
				else
				{
					LLXform* parent_xform = cur->mDrawable->getXform()->getParent();
					LLVector3 new_pos_local;
					// this works in attachment point space using world space delta
					if (parent_xform)
					{
						new_pos_local = selectNode->mSavedPositionLocal + (LLVector3(delta_pos_global) * ~parent_xform->getWorldRotation());
					}
					else
					{
						new_pos_local = selectNode->mSavedPositionLocal + LLVector3(delta_pos_global);
					}
					cur->setPosition(new_pos_local);
				}
				delta_pos = cur->getPositionEdit() - cur_pos;
			}
			if (cur->isRootEdit() && selectNode->mIndividualSelection)
			{
				// counter-translate child objects if we are moving the root as an individual
				LLViewerObject::const_child_list_t& child_list = cur->getChildren();
				for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
					 iter != child_list.end(); iter++)
				{
					LLViewerObject* childp = *iter;
					if (!getUniform())
					{
						LLVector3 child_pos = childp->getPosition() - (delta_pos * ~cur->getRotationEdit());
						childp->setPosition(child_pos);
						rebuild(childp);
					}
				}
			}
		}
	}
}


void LLManipScale::renderGuidelinesPart( const LLBBox& bbox )
{
	LLVector3 guideline_start = bbox.getCenterLocal();

	LLVector3 guideline_end = unitVectorToLocalBBoxExtent( partToUnitVector( mManipPart ), bbox );

	if (!getUniform())
	{
		guideline_start = unitVectorToLocalBBoxExtent( -partToUnitVector( mManipPart ), bbox );
	}

	guideline_end -= guideline_start;
	guideline_end.normalize();
	guideline_end *= LLWorld::getInstance()->getRegionWidthInMeters();
	guideline_end += guideline_start;

	{
		LLGLDepthTest gls_depth(GL_TRUE);
		gl_line_3d( guideline_start, guideline_end, LLColor4(1.f, 1.f, 1.f, 0.5f) );
	}
	{
		LLGLDepthTest gls_depth(GL_FALSE);
		gl_line_3d( guideline_start, guideline_end, LLColor4(1.f, 1.f, 1.f, 0.25f) );
	}
}

void LLManipScale::updateSnapGuides(const LLBBox& bbox)
{
	LLVector3 grid_origin;
	LLVector3 grid_scale;
	LLQuaternion grid_rotation;
	LLSelectMgr::getInstance()->getGrid(grid_origin, grid_rotation, grid_scale);

	bool uniform = LLManipScale::getUniform();

	LLVector3 box_corner_agent = bbox.localToAgent(unitVectorToLocalBBoxExtent( partToUnitVector( mManipPart ), bbox ));
	mScaleCenter = uniform ? bbox.getCenterAgent() : bbox.localToAgent(unitVectorToLocalBBoxExtent( -1.f * partToUnitVector( mManipPart ), bbox ));
	mScaleDir = box_corner_agent - mScaleCenter;
	mScaleDir.normalize();

	if(mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
	{
		mSnapRegimeOffset = SNAP_GUIDE_SCREEN_OFFSET / gAgentCamera.mHUDCurZoom;
	}
	else
	{
		F32 object_distance = dist_vec(box_corner_agent, LLViewerCamera::getInstance()->getOrigin());
		mSnapRegimeOffset = (SNAP_GUIDE_SCREEN_OFFSET * gViewerWindow->getWorldViewWidthRaw() * object_distance) / LLViewerCamera::getInstance()->getPixelMeterRatio();
	}
	LLVector3 cam_at_axis;
	F32 snap_guide_length;
	if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
	{
		cam_at_axis.set(1.f, 0.f, 0.f);
		snap_guide_length = SNAP_GUIDE_SCREEN_LENGTH / gAgentCamera.mHUDCurZoom;
	}
	else
	{
		cam_at_axis = LLViewerCamera::getInstance()->getAtAxis();
		F32 manipulator_distance = dist_vec(box_corner_agent, LLViewerCamera::getInstance()->getOrigin());
		snap_guide_length = (SNAP_GUIDE_SCREEN_LENGTH * gViewerWindow->getWorldViewWidthRaw() * manipulator_distance) / LLViewerCamera::getInstance()->getPixelMeterRatio();
	}

	mSnapGuideLength = snap_guide_length / llmax(0.1f, (llmin(mSnapGuideDir1 * cam_at_axis, mSnapGuideDir2 * cam_at_axis)));

	LLVector3 off_axis_dir = mScaleDir % cam_at_axis;
	off_axis_dir.normalize();

	if( (LL_FACE_MIN <= (S32)mManipPart) && ((S32)mManipPart <= LL_FACE_MAX) )
	{
		LLVector3 bbox_relative_cam_dir = off_axis_dir * ~bbox.getRotation();
		bbox_relative_cam_dir.abs();
		if (bbox_relative_cam_dir.mV[VX] > bbox_relative_cam_dir.mV[VY] && bbox_relative_cam_dir.mV[VX] > bbox_relative_cam_dir.mV[VZ])
		{
			mSnapGuideDir1 = LLVector3::x_axis * bbox.getRotation();
		}
		else if (bbox_relative_cam_dir.mV[VY] > bbox_relative_cam_dir.mV[VZ])
		{
			mSnapGuideDir1 = LLVector3::y_axis * bbox.getRotation();
		}
		else
		{
			mSnapGuideDir1 = LLVector3::z_axis * bbox.getRotation();
		}

		LLVector3 scale_snap = grid_scale;
		mScaleSnapUnit1 = scale_snap.scaleVec(partToUnitVector( mManipPart )).length();
		mScaleSnapUnit2 = mScaleSnapUnit1;
		mSnapGuideDir1 *= mSnapGuideDir1 * LLViewerCamera::getInstance()->getUpAxis() > 0.f ? 1.f : -1.f;
		mSnapGuideDir2 = mSnapGuideDir1 * -1.f;
		mSnapDir1 = mScaleDir;
		mSnapDir2 = mScaleDir;
	}
	else if( (LL_CORNER_MIN <= (S32)mManipPart) && ((S32)mManipPart <= LL_CORNER_MAX) )
	{
		LLVector3 local_camera_dir;
		if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
		{
			local_camera_dir = LLVector3(-1.f, 0.f, 0.f) * ~bbox.getRotation();
		}
		else
		{
			local_camera_dir = (LLViewerCamera::getInstance()->getOrigin() - box_corner_agent) * ~bbox.getRotation();
			local_camera_dir.normalize();
		}

		LLVector3 axis_flip;
		switch (mManipPart)
		{
		case LL_CORNER_NNN:
			axis_flip.set(1.f, 1.f, 1.f);
			break;
		case LL_CORNER_NNP:
			axis_flip.set(1.f, 1.f, -1.f);
			break;
		case LL_CORNER_NPN:
			axis_flip.set(1.f, -1.f, 1.f);
			break;
		case LL_CORNER_NPP:
			axis_flip.set(1.f, -1.f, -1.f);
			break;
		case LL_CORNER_PNN:
			axis_flip.set(-1.f, 1.f, 1.f);
			break;
		case LL_CORNER_PNP:
			axis_flip.set(-1.f, 1.f, -1.f);
			break;
		case LL_CORNER_PPN:
			axis_flip.set(-1.f, -1.f, 1.f);
			break;
		case LL_CORNER_PPP:
			axis_flip.set(-1.f, -1.f, -1.f);
			break;
		default:
			break;
		}

		// account for which side of the object the camera is located and negate appropriate axes
		local_camera_dir.scaleVec(axis_flip);

		// normalize to object scale
		LLVector3 bbox_extent = bbox.getExtentLocal();
		local_camera_dir.scaleVec(LLVector3(1.f / bbox_extent.mV[VX], 1.f / bbox_extent.mV[VY], 1.f / bbox_extent.mV[VZ]));

		S32 scale_face = -1;

		if ((local_camera_dir.mV[VX] > 0.f) == (local_camera_dir.mV[VY] > 0.f))
		{
			if ((local_camera_dir.mV[VZ] > 0.f) == (local_camera_dir.mV[VY] > 0.f))
			{
				LLVector3 local_camera_dir_abs = local_camera_dir;
				local_camera_dir_abs.abs();
				// all neighboring faces of bbox are pointing towards camera or away from camera
				// use largest magnitude face for snap guides
				if (local_camera_dir_abs.mV[VX] > local_camera_dir_abs.mV[VY])
				{
					if (local_camera_dir_abs.mV[VX] > local_camera_dir_abs.mV[VZ])
					{
						scale_face = VX;
					}
					else
					{
						scale_face = VZ;
					}
				}
				else // y > x
				{
					if (local_camera_dir_abs.mV[VY] > local_camera_dir_abs.mV[VZ])
					{
						scale_face = VY;
					}
					else
					{
						scale_face = VZ;
					}
				}
			}
			else
			{
				// z axis facing opposite direction from x and y relative to camera, use x and y for snap guides
				scale_face = VZ;
			}
		}
		else // x and y axes are facing in opposite directions relative to camera
		{
			if ((local_camera_dir.mV[VZ] > 0.f) == (local_camera_dir.mV[VY] > 0.f))
			{
				// x axis facing opposite direction from y and z relative to camera, use y and z for snap guides
				scale_face = VX;
			}
			else
			{
				// y axis facing opposite direction from x and z relative to camera, use x and z for snap guides
				scale_face = VY;
			}
		}

		switch(scale_face)
		{
		case VX:
			// x axis face being scaled, use y and z for snap guides
			mSnapGuideDir1 = LLVector3::y_axis.scaledVec(axis_flip);
			mScaleSnapUnit1 = grid_scale.mV[VZ];
			mSnapGuideDir2 = LLVector3::z_axis.scaledVec(axis_flip);
			mScaleSnapUnit2 = grid_scale.mV[VY];
			break;
		case VY:
			// y axis facing being scaled, use x and z for snap guides
			mSnapGuideDir1 = LLVector3::x_axis.scaledVec(axis_flip);
			mScaleSnapUnit1 = grid_scale.mV[VZ];
			mSnapGuideDir2 = LLVector3::z_axis.scaledVec(axis_flip);
			mScaleSnapUnit2 = grid_scale.mV[VX];
			break;
		case VZ:
			// z axis facing being scaled, use x and y for snap guides
			mSnapGuideDir1 = LLVector3::x_axis.scaledVec(axis_flip);
			mScaleSnapUnit1 = grid_scale.mV[VY];
			mSnapGuideDir2 = LLVector3::y_axis.scaledVec(axis_flip);
			mScaleSnapUnit2 = grid_scale.mV[VX];
			break;
		default:
			mSnapGuideDir1.setZero();
			mScaleSnapUnit1 = 0.f;

			mSnapGuideDir2.setZero();
			mScaleSnapUnit2 = 0.f;
			break;
		}

		mSnapGuideDir1.rotVec(bbox.getRotation());
		mSnapGuideDir2.rotVec(bbox.getRotation());
		mSnapDir1 = -1.f * mSnapGuideDir2;
		mSnapDir2 = -1.f * mSnapGuideDir1;
	}

	mScalePlaneNormal1 = mSnapGuideDir1 % mScaleDir;
	mScalePlaneNormal1.normalize();

	mScalePlaneNormal2 = mSnapGuideDir2 % mScaleDir;
	mScalePlaneNormal2.normalize();

	mScaleSnapUnit1 = mScaleSnapUnit1 / (mSnapDir1 * mScaleDir);
	mScaleSnapUnit2 = mScaleSnapUnit2 / (mSnapDir2 * mScaleDir);

	mTickPixelSpacing1 = ll_round((F32)MIN_DIVISION_PIXEL_WIDTH / (mScaleDir % mSnapGuideDir1).length());
	mTickPixelSpacing2 = ll_round((F32)MIN_DIVISION_PIXEL_WIDTH / (mScaleDir % mSnapGuideDir2).length());

	if (uniform)
	{
		mScaleSnapUnit1 *= 0.5f;
		mScaleSnapUnit2 *= 0.5f;
	}
}

void LLManipScale::renderSnapGuides(const LLBBox& bbox)
{
	if (!gSavedSettings.getBOOL("SnapEnabled"))
	{
		return;
	}

	F32 grid_alpha = gSavedSettings.getF32("GridOpacity");

	F32 max_point_on_scale_line = partToMaxScale(mManipPart, bbox);
	LLVector3 drag_point = gAgent.getPosAgentFromGlobal(mDragPointGlobal);

	updateGridSettings();

	S32 pass;
	for (pass = 0; pass < 3; pass++)
	{
		LLColor4 tick_color = setupSnapGuideRenderPass(pass);

		gGL.begin(LLRender::LINES);
		LLVector3 line_mid = mScaleCenter + (mScaleSnappedValue * mScaleDir) + (mSnapGuideDir1 * mSnapRegimeOffset);
		LLVector3 line_start = line_mid - (mScaleDir * (llmin(mScaleSnappedValue, mSnapGuideLength * 0.5f)));
		LLVector3 line_end = line_mid + (mScaleDir * llmin(max_point_on_scale_line - mScaleSnappedValue, mSnapGuideLength * 0.5f));

		gGL.color4f(tick_color.mV[VRED], tick_color.mV[VGREEN], tick_color.mV[VBLUE], tick_color.mV[VALPHA] * 0.1f);
		gGL.vertex3fv(line_start.mV);
		gGL.color4fv(tick_color.mV);
		gGL.vertex3fv(line_mid.mV);
		gGL.vertex3fv(line_mid.mV);
		gGL.color4f(tick_color.mV[VRED], tick_color.mV[VGREEN], tick_color.mV[VBLUE], tick_color.mV[VALPHA] * 0.1f);
		gGL.vertex3fv(line_end.mV);

		line_mid = mScaleCenter + (mScaleSnappedValue * mScaleDir) + (mSnapGuideDir2 * mSnapRegimeOffset);
		line_start = line_mid - (mScaleDir * (llmin(mScaleSnappedValue, mSnapGuideLength * 0.5f)));
		line_end = line_mid + (mScaleDir * llmin(max_point_on_scale_line - mScaleSnappedValue, mSnapGuideLength * 0.5f));
		gGL.vertex3fv(line_start.mV);
		gGL.color4fv(tick_color.mV);
		gGL.vertex3fv(line_mid.mV);
		gGL.vertex3fv(line_mid.mV);
		gGL.color4f(tick_color.mV[VRED], tick_color.mV[VGREEN], tick_color.mV[VBLUE], tick_color.mV[VALPHA] * 0.1f);
		gGL.vertex3fv(line_end.mV);
		gGL.end();
	}

	{
		LLGLDepthTest gls_depth(GL_FALSE);

		F32 dist_grid_axis = llmax(0.f, (drag_point - mScaleCenter) * mScaleDir);
		
		F32 smallest_subdivision1 = mScaleSnapUnit1 / sGridMaxSubdivisionLevel;
		F32 smallest_subdivision2 = mScaleSnapUnit2 / sGridMaxSubdivisionLevel;
		
		F32 dist_scale_units_1 = dist_grid_axis / smallest_subdivision1;
		F32 dist_scale_units_2 = dist_grid_axis / smallest_subdivision2;
		
		// find distance to nearest smallest grid unit
		F32 grid_multiple1 = llfloor(dist_scale_units_1);
		F32 grid_multiple2 = llfloor(dist_scale_units_2);
		F32 grid_offset1 = fmodf(dist_grid_axis, smallest_subdivision1);
		F32 grid_offset2 = fmodf(dist_grid_axis, smallest_subdivision2);

		// how many smallest grid units are we away from largest grid scale?
		S32 sub_div_offset_1 = ll_round(fmod(dist_grid_axis - grid_offset1, mScaleSnapUnit1 / sGridMinSubdivisionLevel) / smallest_subdivision1);
		S32 sub_div_offset_2 = ll_round(fmod(dist_grid_axis - grid_offset2, mScaleSnapUnit2 / sGridMinSubdivisionLevel) / smallest_subdivision2);

		S32 num_ticks_per_side1 = llmax(1, lltrunc(0.5f * mSnapGuideLength / smallest_subdivision1));
		S32 num_ticks_per_side2 = llmax(1, lltrunc(0.5f * mSnapGuideLength / smallest_subdivision2));
		S32 ticks_from_scale_center_1 = lltrunc(dist_scale_units_1);
		S32 ticks_from_scale_center_2 = lltrunc(dist_scale_units_2);
		S32 max_ticks1 = llceil(max_point_on_scale_line / smallest_subdivision1 - dist_scale_units_1);
		S32 max_ticks2 = llceil(max_point_on_scale_line / smallest_subdivision2 - dist_scale_units_2);
		S32 start_tick = 0;
		S32 stop_tick = 0;

		if (mSnapRegime != SNAP_REGIME_NONE)
		{
			// draw snap guide line
			gGL.begin(LLRender::LINES);
			LLVector3 snap_line_center = bbox.localToAgent(unitVectorToLocalBBoxExtent( partToUnitVector( mManipPart ), bbox ));

			LLVector3 snap_line_start = snap_line_center + (mSnapGuideDir1 * mSnapRegimeOffset);
			LLVector3 snap_line_end = snap_line_center + (mSnapGuideDir2 * mSnapRegimeOffset);

			gGL.color4f(1.f, 1.f, 1.f, grid_alpha);
			gGL.vertex3fv(snap_line_start.mV);
			gGL.vertex3fv(snap_line_center.mV);
			gGL.vertex3fv(snap_line_center.mV);
			gGL.vertex3fv(snap_line_end.mV);
			gGL.end();

			// draw snap guide arrow
			gGL.begin(LLRender::TRIANGLES);
			{
				//gGLSNoCullFaces.set();
				gGL.color4f(1.f, 1.f, 1.f, grid_alpha);

				LLVector3 arrow_dir;
				LLVector3 arrow_span = mScaleDir;

				arrow_dir = snap_line_start - snap_line_center;
				arrow_dir.normalize();
				gGL.vertex3fv((snap_line_start + arrow_dir * mSnapRegimeOffset * 0.1f).mV);
				gGL.vertex3fv((snap_line_start + arrow_span * mSnapRegimeOffset * 0.1f).mV);
				gGL.vertex3fv((snap_line_start - arrow_span * mSnapRegimeOffset * 0.1f).mV);

				arrow_dir = snap_line_end - snap_line_center;
				arrow_dir.normalize();
				gGL.vertex3fv((snap_line_end + arrow_dir * mSnapRegimeOffset * 0.1f).mV);
				gGL.vertex3fv((snap_line_end + arrow_span * mSnapRegimeOffset * 0.1f).mV);
				gGL.vertex3fv((snap_line_end - arrow_span * mSnapRegimeOffset * 0.1f).mV);
			}
			gGL.end();
		}

		LLVector2 screen_translate_axis(llabs(mScaleDir * LLViewerCamera::getInstance()->getLeftAxis()), llabs(mScaleDir * LLViewerCamera::getInstance()->getUpAxis()));
		screen_translate_axis.normalize();

		S32 tick_label_spacing = ll_round(screen_translate_axis * sTickLabelSpacing);

		for (pass = 0; pass < 3; pass++)
		{
			LLColor4 tick_color = setupSnapGuideRenderPass(pass);

			start_tick = -(llmin(ticks_from_scale_center_1, num_ticks_per_side1));
			stop_tick = llmin(max_ticks1, num_ticks_per_side1);

			gGL.begin(LLRender::LINES);
			// draw first row of ticks
			for (S32 i = start_tick; i <= stop_tick; i++)
			{
				F32 alpha = (1.f - (1.f *  ((F32)llabs(i) / (F32)num_ticks_per_side1)));
				LLVector3 tick_pos = mScaleCenter + (mScaleDir * (grid_multiple1 + i) * smallest_subdivision1);

				//No need check this condition to prevent tick position scaling (FIX MAINT-5207/5208)
				//F32 cur_subdivisions = llclamp(getSubdivisionLevel(tick_pos, mScaleDir, mScaleSnapUnit1, mTickPixelSpacing1), sGridMinSubdivisionLevel, sGridMaxSubdivisionLevel);
				/*if (fmodf((F32)(i + sub_div_offset_1), (sGridMaxSubdivisionLevel / cur_subdivisions)) != 0.f)
				{
					continue;
				}*/

				F32 tick_scale = 1.f;
				for (F32 division_level = sGridMaxSubdivisionLevel; division_level >= sGridMinSubdivisionLevel; division_level /= 2.f)
				{
					if (fmodf((F32)(i + sub_div_offset_1), division_level) == 0.f)
					{
						break;
					}
					tick_scale *= 0.7f;
				}

				gGL.color4f(tick_color.mV[VRED], tick_color.mV[VGREEN], tick_color.mV[VBLUE], tick_color.mV[VALPHA] * alpha);
				LLVector3 tick_start = tick_pos + (mSnapGuideDir1 * mSnapRegimeOffset);
				LLVector3 tick_end = tick_start + (mSnapGuideDir1 * mSnapRegimeOffset * tick_scale);
				gGL.vertex3fv(tick_start.mV);
				gGL.vertex3fv(tick_end.mV);
			}

			// draw opposite row of ticks
			start_tick = -(llmin(ticks_from_scale_center_2, num_ticks_per_side2));
			stop_tick = llmin(max_ticks2, num_ticks_per_side2);

			for (S32 i = start_tick; i <= stop_tick; i++)
			{
				F32 alpha = (1.f - (1.f *  ((F32)llabs(i) / (F32)num_ticks_per_side2)));
				LLVector3 tick_pos = mScaleCenter + (mScaleDir * (grid_multiple2 + i) * smallest_subdivision2);
				
				//No need check this condition to prevent tick position scaling (FIX MAINT-5207/5208)
				//F32 cur_subdivisions = llclamp(getSubdivisionLevel(tick_pos, mScaleDir, mScaleSnapUnit2, mTickPixelSpacing2), sGridMinSubdivisionLevel, sGridMaxSubdivisionLevel);
				/*if (fmodf((F32)(i + sub_div_offset_2), (sGridMaxSubdivisionLevel / cur_subdivisions)) != 0.f)
				{
					continue;
				}*/

				F32 tick_scale = 1.f;
				for (F32 division_level = sGridMaxSubdivisionLevel; division_level >= sGridMinSubdivisionLevel; division_level /= 2.f)
				{
					if (fmodf((F32)(i + sub_div_offset_2), division_level) == 0.f)
					{
						break;
					}
					tick_scale *= 0.7f;
				}

				gGL.color4f(tick_color.mV[VRED], tick_color.mV[VGREEN], tick_color.mV[VBLUE], tick_color.mV[VALPHA] * alpha);
				LLVector3 tick_start = tick_pos + (mSnapGuideDir2 * mSnapRegimeOffset);
				LLVector3 tick_end = tick_start + (mSnapGuideDir2 * mSnapRegimeOffset * tick_scale);
				gGL.vertex3fv(tick_start.mV);
				gGL.vertex3fv(tick_end.mV);
			}
			gGL.end();
		}

		// render upper tick labels
		start_tick = -(llmin(ticks_from_scale_center_1, num_ticks_per_side1));
		stop_tick = llmin(max_ticks1, num_ticks_per_side1);

		F32 grid_resolution = mObjectSelection->getSelectType() == SELECT_TYPE_HUD ? 0.25f : llmax(gSavedSettings.getF32("GridResolution"), 0.001f);
		S32 label_sub_div_offset_1 = ll_round(fmod(dist_grid_axis - grid_offset1, mScaleSnapUnit1  * 32.f) / smallest_subdivision1);
		S32 label_sub_div_offset_2 = ll_round(fmod(dist_grid_axis - grid_offset2, mScaleSnapUnit2  * 32.f) / smallest_subdivision2);

		for (S32 i = start_tick; i <= stop_tick; i++)
		{
			F32 tick_scale = 1.f;
			F32 alpha = grid_alpha * (1.f - (0.5f *  ((F32)llabs(i) / (F32)num_ticks_per_side1)));
			LLVector3 tick_pos = mScaleCenter + (mScaleDir * (grid_multiple1 + i) * smallest_subdivision1);

			for (F32 division_level = sGridMaxSubdivisionLevel; division_level >= sGridMinSubdivisionLevel; division_level /= 2.f)
			{
				if (fmodf((F32)(i + label_sub_div_offset_1), division_level) == 0.f)
				{
					break;
				}
				tick_scale *= 0.7f;
			}

			if (fmodf((F32)(i + label_sub_div_offset_1), (sGridMaxSubdivisionLevel / llmin(sGridMaxSubdivisionLevel, getSubdivisionLevel(tick_pos, mScaleDir, mScaleSnapUnit1, tick_label_spacing)))) == 0.f)
			{
				LLVector3 text_origin = tick_pos + (mSnapGuideDir1 * mSnapRegimeOffset * (1.f + tick_scale));
				
				EGridMode grid_mode = LLSelectMgr::getInstance()->getGridMode();
				F32 tick_value;
				if (grid_mode == GRID_MODE_WORLD)
				{
					tick_value = (grid_multiple1 + i) / (sGridMaxSubdivisionLevel / grid_resolution);
				}
				else
				{
					tick_value = (grid_multiple1 + i) / (2.f * sGridMaxSubdivisionLevel);
				}

				F32 text_highlight = 0.8f;

				// Highlight this text if the tick value matches the snapped to value, and if either the second set of ticks isn't going to be shown or cursor is in the first snap regime.
				if (is_approx_equal(tick_value, mScaleSnappedValue) && (mScaleSnapUnit2 == mScaleSnapUnit1 || (mSnapRegime & SNAP_REGIME_UPPER)))
				{
					text_highlight = 1.f;
				}

				renderTickValue(text_origin, tick_value, grid_mode == GRID_MODE_WORLD ? std::string("m") : std::string("x"), LLColor4(text_highlight, text_highlight, text_highlight, alpha));
			}
		}

		// label ticks on opposite side, only can happen in scaling modes that effect more than one axis and when the object's axis don't have the same scale.  A differing scale indicates both conditions.
		if (mScaleSnapUnit2 != mScaleSnapUnit1)
		{
			start_tick = -(llmin(ticks_from_scale_center_2, num_ticks_per_side2));
			stop_tick = llmin(max_ticks2, num_ticks_per_side2);
			for (S32 i = start_tick; i <= stop_tick; i++)
			{
				F32 tick_scale = 1.f;
				F32 alpha = grid_alpha * (1.f - (0.5f *  ((F32)llabs(i) / (F32)num_ticks_per_side2)));
				LLVector3 tick_pos = mScaleCenter + (mScaleDir * (grid_multiple2 + i) * smallest_subdivision2);

				for (F32 division_level = sGridMaxSubdivisionLevel; division_level >= sGridMinSubdivisionLevel; division_level /= 2.f)
				{
					if (fmodf((F32)(i + label_sub_div_offset_2), division_level) == 0.f)
					{
						break;
					}
					tick_scale *= 0.7f;
				}

				if (fmodf((F32)(i + label_sub_div_offset_2), (sGridMaxSubdivisionLevel / llmin(sGridMaxSubdivisionLevel, getSubdivisionLevel(tick_pos, mScaleDir, mScaleSnapUnit2, tick_label_spacing)))) == 0.f)
				{
					LLVector3 text_origin = tick_pos + (mSnapGuideDir2 * mSnapRegimeOffset * (1.f + tick_scale));

					EGridMode grid_mode = LLSelectMgr::getInstance()->getGridMode();
					F32 tick_value;
					if (grid_mode == GRID_MODE_WORLD)
					{
						tick_value = (grid_multiple2 + i) / (sGridMaxSubdivisionLevel / grid_resolution);
					}
					else
					{
						tick_value = (grid_multiple2 + i) / (2.f * sGridMaxSubdivisionLevel);
					}

					F32 text_highlight = 0.8f;

					if (is_approx_equal(tick_value, mScaleSnappedValue) && (mSnapRegime & SNAP_REGIME_LOWER))
					{
						text_highlight = 1.f;
					}

					renderTickValue(text_origin, tick_value, grid_mode == GRID_MODE_WORLD ? std::string("m") : std::string("x"), LLColor4(text_highlight, text_highlight, text_highlight, alpha));
				}
			}
		}


		// render help text
		if (mObjectSelection->getSelectType() != SELECT_TYPE_HUD)
		{
			if (mHelpTextTimer.getElapsedTimeF32() < sHelpTextVisibleTime + sHelpTextFadeTime && sNumTimesHelpTextShown < sMaxTimesShowHelpText)
			{
				LLVector3 selection_center_start = LLSelectMgr::getInstance()->getSavedBBoxOfSelection().getCenterAgent();

				LLVector3 offset_dir;
				if (mSnapGuideDir1 * LLViewerCamera::getInstance()->getAtAxis() > mSnapGuideDir2 * LLViewerCamera::getInstance()->getAtAxis())
				{
					offset_dir = mSnapGuideDir2;
				}
				else
				{
					offset_dir = mSnapGuideDir1;
				}

				LLVector3 help_text_pos = selection_center_start + (mSnapRegimeOffset * 5.f * offset_dir);
				const LLFontGL* big_fontp = LLFontGL::getFontSansSerif();

				std::string help_text = LLTrans::getString("manip_hint1");
				LLColor4 help_text_color = LLColor4::white;
				help_text_color.mV[VALPHA] = clamp_rescale(mHelpTextTimer.getElapsedTimeF32(), sHelpTextVisibleTime, sHelpTextVisibleTime + sHelpTextFadeTime, grid_alpha, 0.f);
				hud_render_utf8text(help_text, help_text_pos, *big_fontp, LLFontGL::NORMAL, LLFontGL::NO_SHADOW, -0.5f * big_fontp->getWidthF32(help_text), 3.f, help_text_color, false);
				help_text = LLTrans::getString("manip_hint2");
				help_text_pos -= LLViewerCamera::getInstance()->getUpAxis() * mSnapRegimeOffset * 0.4f;
				hud_render_utf8text(help_text, help_text_pos, *big_fontp, LLFontGL::NORMAL, LLFontGL::NO_SHADOW, -0.5f * big_fontp->getWidthF32(help_text), 3.f, help_text_color, false);
			}
		}
	}
}

// Returns unit vector in direction of part of an origin-centered cube
LLVector3 LLManipScale::partToUnitVector( S32 part ) const
{
	if ( (LL_FACE_MIN <= part) && (part <= LL_FACE_MAX) )
	{
		return faceToUnitVector( part );
	}
	else if ( (LL_CORNER_MIN <= part) && (part <= LL_CORNER_MAX) )
	{
		return cornerToUnitVector( part );
	}
	else if ( (LL_EDGE_MIN <= part) && (part <= LL_EDGE_MAX ) )
	{
		return edgeToUnitVector( part );
	}
	return LLVector3();
}


// Returns unit vector in direction of face of an origin-centered cube
LLVector3 LLManipScale::faceToUnitVector( S32 part ) const
{
	llassert( (LL_FACE_MIN <= part) && (part <= LL_FACE_MAX) );
	LLVector3 vec;
	switch( part )
	{
		case LL_FACE_POSX:
			vec.set(  1.f,  0.f,  0.f );
			break;
		case LL_FACE_NEGX:
			vec.set( -1.f,  0.f,  0.f );
			break;
		case LL_FACE_POSY:
			vec.set(  0.f,  1.f,  0.f );
			break;
		case LL_FACE_NEGY:
			vec.set(  0.f, -1.f,  0.f );
			break;
		case LL_FACE_POSZ:
			vec.set(  0.f,  0.f,  1.f );
			break;
		case LL_FACE_NEGZ:
			vec.set(  0.f,  0.f, -1.f );
			break;
		default:
			vec.clear();
	}

	return vec;
}


// Returns unit vector in direction of corner of an origin-centered cube
LLVector3 LLManipScale::cornerToUnitVector( S32 part ) const
{
	llassert( (LL_CORNER_MIN <= part) && (part <= LL_CORNER_MAX) );
	LLVector3 vec;
	switch(part)
	{
		case LL_CORNER_NNN:
			vec.set(-OO_SQRT3, -OO_SQRT3, -OO_SQRT3);
			break;
		case LL_CORNER_NNP:
			vec.set(-OO_SQRT3, -OO_SQRT3, OO_SQRT3);
			break;
		case LL_CORNER_NPN:
			vec.set(-OO_SQRT3, OO_SQRT3, -OO_SQRT3);
			break;
		case LL_CORNER_NPP:
			vec.set(-OO_SQRT3, OO_SQRT3, OO_SQRT3);
			break;
		case LL_CORNER_PNN:
			vec.set(OO_SQRT3, -OO_SQRT3, -OO_SQRT3);
			break;
		case LL_CORNER_PNP:
			vec.set(OO_SQRT3, -OO_SQRT3, OO_SQRT3);
			break;
		case LL_CORNER_PPN:
			vec.set(OO_SQRT3, OO_SQRT3, -OO_SQRT3);
			break;
		case LL_CORNER_PPP:
			vec.set(OO_SQRT3, OO_SQRT3, OO_SQRT3);
			break;
		default:
			vec.clear();
	}

	return vec;
}

// Returns unit vector in direction of edge of an origin-centered cube
LLVector3 LLManipScale::edgeToUnitVector( S32 part ) const
{
	llassert( (LL_EDGE_MIN <= part) && (part <= LL_EDGE_MAX) );
	part -= LL_EDGE_MIN;
	S32 rotation = part >> 2;				// Edge between which faces: 0 => XY, 1 => YZ, 2 => ZX
	LLVector3 v;
	v.mV[rotation]			= (part & 1) ? F_SQRT2 : -F_SQRT2;
	v.mV[(rotation+1) % 3]	= (part & 2) ? F_SQRT2 : -F_SQRT2;
	// v.mV[(rotation+2) % 3] defaults to 0.
	return v;
}

// Non-linear scale of origin-centered unit cube to non-origin-centered, non-symetrical bounding box
LLVector3 LLManipScale::unitVectorToLocalBBoxExtent( const LLVector3& v, const LLBBox& bbox ) const
{
	const LLVector3& min = bbox.getMinLocal();
	const LLVector3& max = bbox.getMaxLocal();
	LLVector3 ctr = bbox.getCenterLocal();

	return LLVector3(
		v.mV[0] ? (v.mV[0]>0 ? max.mV[0] : min.mV[0] ) : ctr.mV[0],
		v.mV[1] ? (v.mV[1]>0 ? max.mV[1] : min.mV[1] ) : ctr.mV[1],
		v.mV[2] ? (v.mV[2]>0 ? max.mV[2] : min.mV[2] ) : ctr.mV[2] );
}

// returns max allowable scale along a given stretch axis
F32		LLManipScale::partToMaxScale( S32 part, const LLBBox &bbox ) const
{
	F32 max_scale_factor = 0.f;
	LLVector3 bbox_extents = unitVectorToLocalBBoxExtent( partToUnitVector( part ), bbox );
	bbox_extents.abs();
	F32 max_extent = 0.f;
	for (U32 i = VX; i <= VZ; i++)
	{
		if (bbox_extents.mV[i] > max_extent)
		{
			max_extent = bbox_extents.mV[i];
		}
	}
	max_scale_factor = bbox_extents.length() * get_default_max_prim_scale() / max_extent;

	if (getUniform())
	{
		max_scale_factor *= 0.5f;
	}

	return max_scale_factor;
}

// returns min allowable scale along a given stretch axis
F32		LLManipScale::partToMinScale( S32 part, const LLBBox &bbox ) const
{
	LLVector3 bbox_extents = unitVectorToLocalBBoxExtent( partToUnitVector( part ), bbox );
	bbox_extents.abs();
	F32 min_extent = get_default_max_prim_scale();
	for (U32 i = VX; i <= VZ; i++)
	{
		if (bbox_extents.mV[i] > 0.f && bbox_extents.mV[i] < min_extent)
		{
			min_extent = bbox_extents.mV[i];
		}
	}
	F32 min_scale_factor = bbox_extents.length() * MIN_PRIM_SCALE / min_extent;

	if (getUniform())
	{
		min_scale_factor *= 0.5f;
	}

	return min_scale_factor;
}

// Returns the axis aligned unit vector closest to v.
LLVector3 LLManipScale::nearestAxis( const LLVector3& v ) const
{
	// Note: yes, this is a slow but easy implementation
	// assumes v is normalized

	F32 coords[][3] =
		{
			{ 1.f, 0.f, 0.f },
			{ 0.f, 1.f, 0.f },
			{ 0.f, 0.f, 1.f },
			{-1.f, 0.f, 0.f },
			{ 0.f,-1.f, 0.f },
			{ 0.f, 0.f,-1.f }
		};

	F32 cosine[6];
	cosine[0] = v * LLVector3( coords[0] );
	cosine[1] = v * LLVector3( coords[1] );
	cosine[2] = v * LLVector3( coords[2] );
	cosine[3] = -cosine[0];
	cosine[4] = -cosine[1];
	cosine[5] = -cosine[2];

	F32 greatest_cos = cosine[0];
	S32 greatest_index = 0;
	for( S32 i=1; i<6; i++ )
	{
		if( greatest_cos < cosine[i] )
		{
			greatest_cos = cosine[i];
			greatest_index = i;
		}
	}

	return LLVector3( coords[greatest_index] );
}

// virtual
bool LLManipScale::canAffectSelection()
{
	// An selection is scalable if you are allowed to both edit and move
	// everything in it, and it does not have any sitting agents
	bool can_scale = mObjectSelection->getObjectCount() != 0;
	if (can_scale)
	{
		struct f : public LLSelectedObjectFunctor
		{
			virtual bool apply(LLViewerObject* objectp)
			{
				LLViewerObject *root_object = (objectp == NULL) ? NULL : objectp->getRootEdit();
				return objectp->permModify() && objectp->permMove() && !objectp->isPermanentEnforced() &&
					(root_object == NULL || (!root_object->isPermanentEnforced() && !root_object->isSeat())) &&
					!objectp->isSeat();
			}
		} func;
		can_scale = mObjectSelection->applyToObjects(&func);
	}
	return can_scale;
}
