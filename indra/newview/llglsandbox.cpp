/** 
 * @file llglsandbox.cpp
 * @brief GL functionality access
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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
 * Contains ALL methods which directly access GL functionality 
 * except for core rendering engine functionality.
 */

#include "llviewerprecompiledheaders.h"

#include "llviewercontrol.h"

#include "llgl.h"
#include "llrender.h"
#include "llglheaders.h"
#include "llparcel.h"
#include "llui.h"

#include "lldrawable.h"
#include "lltextureentry.h"
#include "llviewercamera.h"

#include "llvoavatarself.h"
#include "llagent.h"
#include "lltoolmgr.h"
#include "llselectmgr.h"
#include "llhudmanager.h"
#include "llhudtext.h"
#include "llrendersphere.h"
#include "llviewerobjectlist.h"
#include "lltoolselectrect.h"
#include "llviewerwindow.h"
#include "llsurface.h"
#include "llwind.h"
#include "llworld.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llpreviewtexture.h"
#include "llresmgr.h"
#include "pipeline.h"
#include "llspatialpartition.h"

// Height of the yellow selection highlight posts for land
const F32 PARCEL_POST_HEIGHT = 0.666f;

// Returns true if you got at least one object
void LLToolSelectRect::handleRectangleSelection(S32 x, S32 y, MASK mask)
{
	LLVector3 av_pos = gAgent.getPositionAgent();
	F32 select_dist_squared = gSavedSettings.getF32("MaxSelectDistance");
	select_dist_squared = select_dist_squared * select_dist_squared;

	BOOL deselect = (mask == MASK_CONTROL);
	S32 left =	llmin(x, mDragStartX);
	S32 right =	llmax(x, mDragStartX);
	S32 top =	llmax(y, mDragStartY);
	S32 bottom =llmin(y, mDragStartY);

	left = llround((F32) left * LLUI::getScaleFactor().mV[VX]);
	right = llround((F32) right * LLUI::getScaleFactor().mV[VX]);
	top = llround((F32) top * LLUI::getScaleFactor().mV[VY]);
	bottom = llround((F32) bottom * LLUI::getScaleFactor().mV[VY]);

	F32 old_far_plane = LLViewerCamera::getInstance()->getFar();
	F32 old_near_plane = LLViewerCamera::getInstance()->getNear();

	S32 width = right - left + 1;
	S32 height = top - bottom + 1;

	BOOL grow_selection = FALSE;
	BOOL shrink_selection = FALSE;

	if (height > mDragLastHeight || width > mDragLastWidth)
	{
		grow_selection = TRUE;
	}
	if (height < mDragLastHeight || width < mDragLastWidth)
	{
		shrink_selection = TRUE;
	}

	if (!grow_selection && !shrink_selection)
	{
		// nothing to do
		return;
	}

	mDragLastHeight = height;
	mDragLastWidth = width;

	S32 center_x = (left + right) / 2;
	S32 center_y = (top + bottom) / 2;

	// save drawing mode
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();

	BOOL limit_select_distance = gSavedSettings.getBOOL("LimitSelectDistance");
	if (limit_select_distance)
	{
		// ...select distance from control
		LLVector3 relative_av_pos = av_pos;
		relative_av_pos -= LLViewerCamera::getInstance()->getOrigin();

		F32 new_far = relative_av_pos * LLViewerCamera::getInstance()->getAtAxis() + gSavedSettings.getF32("MaxSelectDistance");
		F32 new_near = relative_av_pos * LLViewerCamera::getInstance()->getAtAxis() - gSavedSettings.getF32("MaxSelectDistance");

		new_near = llmax(new_near, 0.1f);

		LLViewerCamera::getInstance()->setFar(new_far);
		LLViewerCamera::getInstance()->setNear(new_near);
	}
	LLViewerCamera::getInstance()->setPerspective(FOR_SELECTION, 
							center_x-width/2, center_y-height/2, width, height, 
							limit_select_distance);

	if (shrink_selection)
	{
		struct f : public LLSelectedObjectFunctor
		{
			virtual bool apply(LLViewerObject* vobjp)
			{
				LLDrawable* drawable = vobjp->mDrawable;
				if (!drawable || vobjp->getPCode() != LL_PCODE_VOLUME || vobjp->isAttachment())
				{
					return true;
				}
				S32 result = LLViewerCamera::getInstance()->sphereInFrustum(drawable->getPositionAgent(), drawable->getRadius());
				switch (result)
				{
				  case 0:
					LLSelectMgr::getInstance()->unhighlightObjectOnly(vobjp);
					break;
				  case 1:
					// check vertices
					if (!LLViewerCamera::getInstance()->areVertsVisible(vobjp, LLSelectMgr::sRectSelectInclusive))
					{
						LLSelectMgr::getInstance()->unhighlightObjectOnly(vobjp);
					}
					break;
				  default:
					break;
				}
				return true;
			}
		} func;
		LLSelectMgr::getInstance()->getHighlightedObjects()->applyToObjects(&func);
	}

	if (grow_selection)
	{
		std::vector<LLDrawable*> potentials;
				
		for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin(); 
			iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
		{
			LLViewerRegion* region = *iter;
			for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
			{
				LLSpatialPartition* part = region->getSpatialPartition(i);
				if (part)
				{	
					part->cull(*LLViewerCamera::getInstance(), &potentials, TRUE);
				}
			}
		}
		
		for (std::vector<LLDrawable*>::iterator iter = potentials.begin();
			 iter != potentials.end(); iter++)
		{
			LLDrawable* drawable = *iter;
			LLViewerObject* vobjp = drawable->getVObj();

			if (!drawable || !vobjp ||
				vobjp->getPCode() != LL_PCODE_VOLUME || 
				vobjp->isAttachment() ||
				(deselect && !vobjp->isSelected()))
			{
				continue;
			}

			if (limit_select_distance && dist_vec_squared(drawable->getWorldPosition(), av_pos) > select_dist_squared)
			{
				continue;
			}

			S32 result = LLViewerCamera::getInstance()->sphereInFrustum(drawable->getPositionAgent(), drawable->getRadius());
			if (result)
			{
				switch (result)
				{
				case 1:
					// check vertices
					if (LLViewerCamera::getInstance()->areVertsVisible(vobjp, LLSelectMgr::sRectSelectInclusive))
					{
						LLSelectMgr::getInstance()->highlightObjectOnly(vobjp);
					}
					break;
				case 2:
					LLSelectMgr::getInstance()->highlightObjectOnly(vobjp);
					break;
				default:
					break;
				}
			}
		}
	}

	// restore drawing mode
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);

	// restore camera
	LLViewerCamera::getInstance()->setFar(old_far_plane);
	LLViewerCamera::getInstance()->setNear(old_near_plane);
	gViewerWindow->setup3DRender();
}

const F32 WIND_RELATIVE_ALTITUDE			= 25.f;

void LLWind::renderVectors()
{
	// Renders the wind as vectors (used for debug)
	S32 i,j;
	F32 x,y;

	F32 region_width_meters = LLWorld::getInstance()->getRegionWidthInMeters();

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.pushMatrix();
	LLVector3 origin_agent;
	origin_agent = gAgent.getPosAgentFromGlobal(mOriginGlobal);
	gGL.translatef(origin_agent.mV[VX], origin_agent.mV[VY], gAgent.getPositionAgent().mV[VZ] + WIND_RELATIVE_ALTITUDE);
	for (j = 0; j < mSize; j++)
	{
		for (i = 0; i < mSize; i++)
		{
			x = mVelX[i + j*mSize] * WIND_SCALE_HACK;
			y = mVelY[i + j*mSize] * WIND_SCALE_HACK;
			gGL.pushMatrix();
			gGL.translatef((F32)i * region_width_meters/mSize, (F32)j * region_width_meters/mSize, 0.0);
			gGL.color3f(0,1,0);
			gGL.begin(LLRender::POINTS);
				gGL.vertex3f(0,0,0);
			gGL.end();
			gGL.color3f(1,0,0);
			gGL.begin(LLRender::LINES);
				gGL.vertex3f(x * 0.1f, y * 0.1f ,0.f);
				gGL.vertex3f(x, y, 0.f);
			gGL.end();
			gGL.popMatrix();
		}
	}
	gGL.popMatrix();
}




// Used by lltoolselectland
void LLViewerParcelMgr::renderRect(const LLVector3d &west_south_bottom_global, 
								   const LLVector3d &east_north_top_global )
{
	LLGLSUIDefault gls_ui;
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	LLGLDepthTest gls_depth(GL_TRUE);

	LLVector3 west_south_bottom_agent = gAgent.getPosAgentFromGlobal(west_south_bottom_global);
	F32 west	= west_south_bottom_agent.mV[VX];
	F32 south	= west_south_bottom_agent.mV[VY];
//	F32 bottom	= west_south_bottom_agent.mV[VZ] - 1.f;

	LLVector3 east_north_top_agent = gAgent.getPosAgentFromGlobal(east_north_top_global);
	F32 east	= east_north_top_agent.mV[VX];
	F32 north	= east_north_top_agent.mV[VY];
//	F32 top		= east_north_top_agent.mV[VZ] + 1.f;

	// HACK: At edge of last region of world, we need to make sure the region
	// resolves correctly so we can get a height value.
	const F32 FUDGE = 0.01f;

	F32 sw_bottom = LLWorld::getInstance()->resolveLandHeightAgent( LLVector3( west, south, 0.f ) );
	F32 se_bottom = LLWorld::getInstance()->resolveLandHeightAgent( LLVector3( east-FUDGE, south, 0.f ) );
	F32 ne_bottom = LLWorld::getInstance()->resolveLandHeightAgent( LLVector3( east-FUDGE, north-FUDGE, 0.f ) );
	F32 nw_bottom = LLWorld::getInstance()->resolveLandHeightAgent( LLVector3( west, north-FUDGE, 0.f ) );

	F32 sw_top = sw_bottom + PARCEL_POST_HEIGHT;
	F32 se_top = se_bottom + PARCEL_POST_HEIGHT;
	F32 ne_top = ne_bottom + PARCEL_POST_HEIGHT;
	F32 nw_top = nw_bottom + PARCEL_POST_HEIGHT;

	LLUI::setLineWidth(2.f);
	gGL.color4f(1.f, 1.f, 0.f, 1.f);

	// Cheat and give this the same pick-name as land
	gGL.begin(LLRender::LINES);

	gGL.vertex3f(west, north, nw_bottom);
	gGL.vertex3f(west, north, nw_top);

	gGL.vertex3f(east, north, ne_bottom);
	gGL.vertex3f(east, north, ne_top);

	gGL.vertex3f(east, south, se_bottom);
	gGL.vertex3f(east, south, se_top);

	gGL.vertex3f(west, south, sw_bottom);
	gGL.vertex3f(west, south, sw_top);

	gGL.end();

	gGL.color4f(1.f, 1.f, 0.f, 0.2f);
	gGL.begin(LLRender::QUADS);

	gGL.vertex3f(west, north, nw_bottom);
	gGL.vertex3f(west, north, nw_top);
	gGL.vertex3f(east, north, ne_top);
	gGL.vertex3f(east, north, ne_bottom);

	gGL.vertex3f(east, north, ne_bottom);
	gGL.vertex3f(east, north, ne_top);
	gGL.vertex3f(east, south, se_top);
	gGL.vertex3f(east, south, se_bottom);

	gGL.vertex3f(east, south, se_bottom);
	gGL.vertex3f(east, south, se_top);
	gGL.vertex3f(west, south, sw_top);
	gGL.vertex3f(west, south, sw_bottom);

	gGL.vertex3f(west, south, sw_bottom);
	gGL.vertex3f(west, south, sw_top);
	gGL.vertex3f(west, north, nw_top);
	gGL.vertex3f(west, north, nw_bottom);

	gGL.end();

	LLUI::setLineWidth(1.f);
}

/*
void LLViewerParcelMgr::renderParcel(LLParcel* parcel )
{
	S32 i;
	S32 count = parcel->getBoxCount();
	for (i = 0; i < count; i++)
	{
		const LLParcelBox& box = parcel->getBox(i);

		F32 west = box.mMin.mV[VX];
		F32 south = box.mMin.mV[VY];

		F32 east = box.mMax.mV[VX];
		F32 north = box.mMax.mV[VY];

		// HACK: At edge of last region of world, we need to make sure the region
		// resolves correctly so we can get a height value.
		const F32 FUDGE = 0.01f;

		F32 sw_bottom = LLWorld::getInstance()->resolveLandHeightAgent( LLVector3( west, south, 0.f ) );
		F32 se_bottom = LLWorld::getInstance()->resolveLandHeightAgent( LLVector3( east-FUDGE, south, 0.f ) );
		F32 ne_bottom = LLWorld::getInstance()->resolveLandHeightAgent( LLVector3( east-FUDGE, north-FUDGE, 0.f ) );
		F32 nw_bottom = LLWorld::getInstance()->resolveLandHeightAgent( LLVector3( west, north-FUDGE, 0.f ) );

		// little hack to make nearby lines not Z-fight
		east -= 0.1f;
		north -= 0.1f;

		F32 sw_top = sw_bottom + POST_HEIGHT;
		F32 se_top = se_bottom + POST_HEIGHT;
		F32 ne_top = ne_bottom + POST_HEIGHT;
		F32 nw_top = nw_bottom + POST_HEIGHT;

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		LLGLDepthTest gls_depth(GL_TRUE);

		LLUI::setLineWidth(2.f);
		gGL.color4f(0.f, 1.f, 1.f, 1.f);

		// Cheat and give this the same pick-name as land
		gGL.begin(LLRender::LINES);

		gGL.vertex3f(west, north, nw_bottom);
		gGL.vertex3f(west, north, nw_top);

		gGL.vertex3f(east, north, ne_bottom);
		gGL.vertex3f(east, north, ne_top);

		gGL.vertex3f(east, south, se_bottom);
		gGL.vertex3f(east, south, se_top);

		gGL.vertex3f(west, south, sw_bottom);
		gGL.vertex3f(west, south, sw_top);

		gGL.end();

		gGL.color4f(0.f, 1.f, 1.f, 0.2f);
		gGL.begin(LLRender::QUADS);

		gGL.vertex3f(west, north, nw_bottom);
		gGL.vertex3f(west, north, nw_top);
		gGL.vertex3f(east, north, ne_top);
		gGL.vertex3f(east, north, ne_bottom);

		gGL.vertex3f(east, north, ne_bottom);
		gGL.vertex3f(east, north, ne_top);
		gGL.vertex3f(east, south, se_top);
		gGL.vertex3f(east, south, se_bottom);

		gGL.vertex3f(east, south, se_bottom);
		gGL.vertex3f(east, south, se_top);
		gGL.vertex3f(west, south, sw_top);
		gGL.vertex3f(west, south, sw_bottom);

		gGL.vertex3f(west, south, sw_bottom);
		gGL.vertex3f(west, south, sw_top);
		gGL.vertex3f(west, north, nw_top);
		gGL.vertex3f(west, north, nw_bottom);

		gGL.end();

		LLUI::setLineWidth(1.f);
	}
}
*/


// north = a wall going north/south.  Need that info to set up texture
// coordinates correctly.
void LLViewerParcelMgr::renderOneSegment(F32 x1, F32 y1, F32 x2, F32 y2, F32 height, U8 direction, LLViewerRegion* regionp)
{
	// HACK: At edge of last region of world, we need to make sure the region
	// resolves correctly so we can get a height value.
	const F32 BORDER = REGION_WIDTH_METERS - 0.1f;

	F32 clamped_x1 = x1;
	F32 clamped_y1 = y1;
	F32 clamped_x2 = x2;
	F32 clamped_y2 = y2;

	if (clamped_x1 > BORDER) clamped_x1 = BORDER;
	if (clamped_y1 > BORDER) clamped_y1 = BORDER;
	if (clamped_x2 > BORDER) clamped_x2 = BORDER;
	if (clamped_y2 > BORDER) clamped_y2 = BORDER;

	F32 z;
	F32 z1;
	F32 z2;

	z1 = regionp->getLand().resolveHeightRegion( LLVector3( clamped_x1, clamped_y1, 0.f ) );
	z2 = regionp->getLand().resolveHeightRegion( LLVector3( clamped_x2, clamped_y2, 0.f ) );

	// Convert x1 and x2 from region-local to agent coords.
	LLVector3 origin = regionp->getOriginAgent();
	x1 += origin.mV[VX];
	x2 += origin.mV[VX];
	y1 += origin.mV[VY];
	y2 += origin.mV[VY];

	if (height < 1.f)
	{
		z = z1+height;
		gGL.vertex3f(x1, y1, z);

		gGL.vertex3f(x1, y1, z1);

		gGL.vertex3f(x2, y2, z2);

		z = z2+height;
		gGL.vertex3f(x2, y2, z);
	}
	else
	{
		F32 tex_coord1;
		F32 tex_coord2;

		if (WEST_MASK == direction)
		{
			tex_coord1 = y1;
			tex_coord2 = y2;
		}
		else if (SOUTH_MASK == direction)
		{
			tex_coord1 = x1;
			tex_coord2 = x2;
		}
		else if (EAST_MASK == direction)
		{
			tex_coord1 = y2;
			tex_coord2 = y1;
		}
		else /* (NORTH_MASK == direction) */
		{
			tex_coord1 = x2;
			tex_coord2 = x1;
		}


		gGL.texCoord2f(tex_coord1*0.5f+0.5f, z1*0.5f);
		gGL.vertex3f(x1, y1, z1);

		gGL.texCoord2f(tex_coord2*0.5f+0.5f, z2*0.5f);
		gGL.vertex3f(x2, y2, z2);

		// top edge stairsteps
		z = llmax(z2+height, z1+height);
		gGL.texCoord2f(tex_coord2*0.5f+0.5f, z*0.5f);
		gGL.vertex3f(x2, y2, z);

		gGL.texCoord2f(tex_coord1*0.5f+0.5f, z*0.5f);
		gGL.vertex3f(x1, y1, z);
	}
}


void LLViewerParcelMgr::renderHighlightSegments(const U8* segments, LLViewerRegion* regionp)
{
	S32 x, y;
	F32 x1, y1;	// start point
	F32 x2, y2;	// end point
	bool has_segments = false;

	LLGLSUIDefault gls_ui;
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	LLGLDepthTest gls_depth(GL_TRUE);

	gGL.color4f(1.f, 1.f, 0.f, 0.2f);

	const S32 STRIDE = (mParcelsPerEdge+1);

	// Cheat and give this the same pick-name as land
	
	
	for (y = 0; y < STRIDE; y++)
	{
		for (x = 0; x < STRIDE; x++)
		{
			U8 segment_mask = segments[x + y*STRIDE];

			if (segment_mask & SOUTH_MASK)
			{
				x1 = x * PARCEL_GRID_STEP_METERS;
				y1 = y * PARCEL_GRID_STEP_METERS;

				x2 = x1 + PARCEL_GRID_STEP_METERS;
				y2 = y1;
				
				if (!has_segments)
				{
					has_segments = true;
					gGL.begin(LLRender::QUADS);
				}
				renderOneSegment(x1, y1, x2, y2, PARCEL_POST_HEIGHT, SOUTH_MASK, regionp);
			}

			if (segment_mask & WEST_MASK)
			{
				x1 = x * PARCEL_GRID_STEP_METERS;
				y1 = y * PARCEL_GRID_STEP_METERS;

				x2 = x1;
				y2 = y1 + PARCEL_GRID_STEP_METERS;

				if (!has_segments)
				{
					has_segments = true;
					gGL.begin(LLRender::QUADS);
				}
				renderOneSegment(x1, y1, x2, y2, PARCEL_POST_HEIGHT, WEST_MASK, regionp);
			}
		}
	}

	if (has_segments)
	{
		gGL.end();
	}
}


void LLViewerParcelMgr::renderCollisionSegments(U8* segments, BOOL use_pass, LLViewerRegion* regionp)
{

	S32 x, y;
	F32 x1, y1;	// start point
	F32 x2, y2;	// end point
	F32 alpha = 0;
	F32 dist = 0;
	F32 dx, dy;
	F32 collision_height;

	const S32 STRIDE = (mParcelsPerEdge+1);
	
	LLVector3 pos = gAgent.getPositionAgent();

	F32 pos_x = pos.mV[VX];
	F32 pos_y = pos.mV[VY];

	LLGLSUIDefault gls_ui;
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
	LLGLDisable cull(GL_CULL_FACE);
	
	if (mCollisionBanned == BA_BANNED)
	{
		collision_height = BAN_HEIGHT;
	}
	else
	{
		collision_height = PARCEL_HEIGHT;
	}

	
	if (use_pass && (mCollisionBanned == BA_NOT_ON_LIST))
	{
		gGL.getTexUnit(0)->bind(mPassImage);
	}
	else
	{
		gGL.getTexUnit(0)->bind(mBlockedImage);
	}

	gGL.begin(LLRender::QUADS);

	for (y = 0; y < STRIDE; y++)
	{
		for (x = 0; x < STRIDE; x++)
		{
			U8 segment_mask = segments[x + y*STRIDE];
			U8 direction;
			const F32 MAX_ALPHA = 0.95f;
			const S32 DIST_OFFSET = 5;
			const S32 MIN_DIST_SQ = DIST_OFFSET*DIST_OFFSET;
			const S32 MAX_DIST_SQ = 169;

			if (segment_mask & SOUTH_MASK)
			{
				x1 = x * PARCEL_GRID_STEP_METERS;
				y1 = y * PARCEL_GRID_STEP_METERS;

				x2 = x1 + PARCEL_GRID_STEP_METERS;
				y2 = y1;

				dy = (pos_y - y1) + DIST_OFFSET;
					
				if (pos_x < x1)
					dx = pos_x - x1;
				else if (pos_x > x2)
					dx = pos_x - x2;
				else 
					dx = 0;
				
				dist = dx*dx+dy*dy;
				
				if (dist < MIN_DIST_SQ)
					alpha = MAX_ALPHA;
				else if (dist > MAX_DIST_SQ)
					alpha = 0.0f;
				else
					alpha = 30/dist;
				
				alpha = llclamp(alpha, 0.0f, MAX_ALPHA);
				
				gGL.color4f(1.f, 1.f, 1.f, alpha);

				if ((pos_y - y1) < 0) direction = SOUTH_MASK;
				else 		direction = NORTH_MASK;

				// avoid Z fighting
				renderOneSegment(x1+0.1f, y1+0.1f, x2+0.1f, y2+0.1f, collision_height, direction, regionp);

			}

			if (segment_mask & WEST_MASK)
			{
				x1 = x * PARCEL_GRID_STEP_METERS;
				y1 = y * PARCEL_GRID_STEP_METERS;

				x2 = x1;
				y2 = y1 + PARCEL_GRID_STEP_METERS;

				dx = (pos_x - x1) + DIST_OFFSET;
				
				if (pos_y < y1) 
					dy = pos_y - y1;
				else if (pos_y > y2)
					dy = pos_y - y2;
				else 
					dy = 0;
				
				dist = dx*dx+dy*dy;
				
				if (dist < MIN_DIST_SQ) 
					alpha = MAX_ALPHA;
				else if (dist > MAX_DIST_SQ)
					alpha = 0.0f;
				else
					alpha = 30/dist;
				
				alpha = llclamp(alpha, 0.0f, MAX_ALPHA);

				gGL.color4f(1.f, 1.f, 1.f, alpha);

				if ((pos_x - x1) > 0) direction = WEST_MASK;
				else 		direction = EAST_MASK;
				
				// avoid Z fighting
				renderOneSegment(x1+0.1f, y1+0.1f, x2+0.1f, y2+0.1f, collision_height, direction, regionp);

			}
		}
	}

	gGL.end();
}

void draw_line_cube(F32 width, const LLVector3& center)
{
	width = 0.5f * width;
	gGL.vertex3f(center.mV[VX] + width ,center.mV[VY] + width,center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] - width ,center.mV[VY] + width,center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] - width ,center.mV[VY] + width,center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] - width ,center.mV[VY] - width,center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] - width ,center.mV[VY] - width,center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] + width ,center.mV[VY] - width,center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] + width ,center.mV[VY] - width,center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] + width ,center.mV[VY] + width,center.mV[VZ] + width);

	gGL.vertex3f(center.mV[VX] + width ,center.mV[VY] + width,center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] - width ,center.mV[VY] + width,center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] - width ,center.mV[VY] + width,center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] - width ,center.mV[VY] - width,center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] - width ,center.mV[VY] - width,center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] + width ,center.mV[VY] - width,center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] + width ,center.mV[VY] - width,center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] + width ,center.mV[VY] + width,center.mV[VZ] - width);

	gGL.vertex3f(center.mV[VX] + width ,center.mV[VY] + width,center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] + width ,center.mV[VY] + width,center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] - width ,center.mV[VY] + width,center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] - width ,center.mV[VY] + width,center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] - width ,center.mV[VY] - width,center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] - width ,center.mV[VY] - width,center.mV[VZ] - width);
	gGL.vertex3f(center.mV[VX] + width ,center.mV[VY] - width,center.mV[VZ] + width);
	gGL.vertex3f(center.mV[VX] + width ,center.mV[VY] - width,center.mV[VZ] - width);
}


void LLViewerObjectList::renderObjectBeacons()
{
	if (mDebugBeacons.empty())
	{
		return;
	}

	LLGLSUIDefault gls_ui;

	if (LLGLSLShader::sNoFixedFunction)
	{
		gUIProgram.bind();
	}

	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		S32 last_line_width = -1;
		// gGL.begin(LLRender::LINES); // Always happens in (line_width != last_line_width)
		
		for (std::vector<LLDebugBeacon>::iterator iter = mDebugBeacons.begin(); iter != mDebugBeacons.end(); ++iter)
		{
			const LLDebugBeacon &debug_beacon = *iter;
			LLColor4 color = debug_beacon.mColor;
			color.mV[3] *= 0.25f;
			S32 line_width = debug_beacon.mLineWidth;
			if (line_width != last_line_width)
			{
				gGL.flush();
				glLineWidth( (F32)line_width );
				last_line_width = line_width;
			}

			const LLVector3 &thisline = debug_beacon.mPositionAgent;
		
			gGL.begin(LLRender::LINES);
			gGL.color4fv(color.mV);
			gGL.vertex3f(thisline.mV[VX],thisline.mV[VY],thisline.mV[VZ] - 50.f);
			gGL.vertex3f(thisline.mV[VX],thisline.mV[VY],thisline.mV[VZ] + 50.f);
			gGL.vertex3f(thisline.mV[VX] - 2.f,thisline.mV[VY],thisline.mV[VZ]);
			gGL.vertex3f(thisline.mV[VX] + 2.f,thisline.mV[VY],thisline.mV[VZ]);
			gGL.vertex3f(thisline.mV[VX],thisline.mV[VY] - 2.f,thisline.mV[VZ]);
			gGL.vertex3f(thisline.mV[VX],thisline.mV[VY] + 2.f,thisline.mV[VZ]);

			draw_line_cube(0.10f, thisline);
			
			gGL.end();
		}
	}

	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		LLGLDepthTest gls_depth(GL_TRUE);
		
		S32 last_line_width = -1;
		// gGL.begin(LLRender::LINES); // Always happens in (line_width != last_line_width)
		
		for (std::vector<LLDebugBeacon>::iterator iter = mDebugBeacons.begin(); iter != mDebugBeacons.end(); ++iter)
		{
			const LLDebugBeacon &debug_beacon = *iter;

			S32 line_width = debug_beacon.mLineWidth;
			if (line_width != last_line_width)
			{
				gGL.flush();
				glLineWidth( (F32)line_width );
				last_line_width = line_width;
			}

			const LLVector3 &thisline = debug_beacon.mPositionAgent;
			gGL.begin(LLRender::LINES);
			gGL.color4fv(debug_beacon.mColor.mV);
			gGL.vertex3f(thisline.mV[VX],thisline.mV[VY],thisline.mV[VZ] - 0.5f);
			gGL.vertex3f(thisline.mV[VX],thisline.mV[VY],thisline.mV[VZ] + 0.5f);
			gGL.vertex3f(thisline.mV[VX] - 0.5f,thisline.mV[VY],thisline.mV[VZ]);
			gGL.vertex3f(thisline.mV[VX] + 0.5f,thisline.mV[VY],thisline.mV[VZ]);
			gGL.vertex3f(thisline.mV[VX],thisline.mV[VY] - 0.5f,thisline.mV[VZ]);
			gGL.vertex3f(thisline.mV[VX],thisline.mV[VY] + 0.5f,thisline.mV[VZ]);

			draw_line_cube(0.10f, thisline);

			gGL.end();
		}
		
		gGL.flush();
		glLineWidth(1.f);

		for (std::vector<LLDebugBeacon>::iterator iter = mDebugBeacons.begin(); iter != mDebugBeacons.end(); ++iter)
		{
			LLDebugBeacon &debug_beacon = *iter;
			if (debug_beacon.mString == "")
			{
				continue;
			}
			LLHUDText *hud_textp = (LLHUDText *)LLHUDObject::addHUDObject(LLHUDObject::LL_HUD_TEXT);

			hud_textp->setZCompare(FALSE);
			LLColor4 color;
			color = debug_beacon.mTextColor;
			color.mV[3] *= 1.f;

			hud_textp->setString(debug_beacon.mString);
			hud_textp->setColor(color);
			hud_textp->setPositionAgent(debug_beacon.mPositionAgent);
			debug_beacon.mHUDObject = hud_textp;
		}
	}
}


