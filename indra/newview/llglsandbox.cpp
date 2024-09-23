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
#include "llsky.h"
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
#include "llviewershadermgr.h"

#include <vector>

// Height of the yellow selection highlight posts for land
constexpr F32 PARCEL_POST_HEIGHT = 0.666f;

// Returns true if you got at least one object
void LLToolSelectRect::handleRectangleSelection(S32 x, S32 y, MASK mask)
{
    LLVector3 av_pos = gAgent.getPositionAgent();
    F32 select_dist_squared = gSavedSettings.getF32("MaxSelectDistance");
    select_dist_squared = select_dist_squared * select_dist_squared;

    bool deselect = (mask == MASK_CONTROL);
    S32 left =  llmin(x, mDragStartX);
    S32 right = llmax(x, mDragStartX);
    S32 top =   llmax(y, mDragStartY);
    S32 bottom =llmin(y, mDragStartY);

    left = ll_round((F32) left * LLUI::getScaleFactor().mV[VX]);
    right = ll_round((F32) right * LLUI::getScaleFactor().mV[VX]);
    top = ll_round((F32) top * LLUI::getScaleFactor().mV[VY]);
    bottom = ll_round((F32) bottom * LLUI::getScaleFactor().mV[VY]);

    F32 old_far_plane = LLViewerCamera::getInstance()->getFar();
    F32 old_near_plane = LLViewerCamera::getInstance()->getNear();

    S32 width = right - left + 1;
    S32 height = top - bottom + 1;

    bool grow_selection = false;
    bool shrink_selection = false;

    if (height > mDragLastHeight || width > mDragLastWidth)
    {
        grow_selection = true;
    }
    if (height < mDragLastHeight || width < mDragLastWidth)
    {
        shrink_selection = true;
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

    bool limit_select_distance = gSavedSettings.getBOOL("LimitSelectDistance");
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

        for (LLViewerRegion* region : LLWorld::getInstance()->getRegionList())
        {
            for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
            {
                if (LLSpatialPartition* part = region->getSpatialPartition(i))
                {
                    part->cull(*LLViewerCamera::getInstance(), &potentials, true);
                }
            }
        }

        for (LLDrawable* drawable : potentials)
        {
            if (!drawable)
            {
                continue;
            }

            LLViewerObject* vobjp = drawable->getVObj();

            if (!vobjp ||
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

constexpr F32 WIND_RELATIVE_ALTITUDE = 25.f;

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
            gGL.translatef((F32)i * region_width_meters/mSize, (F32)j * region_width_meters/mSize, 0.f);
            gGL.color3f(0.f, 1.f, 0.f);
            gGL.begin(LLRender::POINTS);
                gGL.vertex3f(0.f, 0.f, 0.f);
            gGL.end();
            gGL.color3f(1.f, 0.f, 0.f);
            gGL.begin(LLRender::LINES);
                gGL.vertex3f(x * 0.1f, y * 0.1f, 0.f);
                gGL.vertex3f(x, y, 0.f);
            gGL.end();
            gGL.popMatrix();
        }
    }
    gGL.popMatrix();
}




// Used by lltoolselectland
void LLViewerParcelMgr::renderRect(const LLVector3d &west_south_bottom_global,
                                   const LLVector3d &east_north_top_global)
{
    LLGLSUIDefault gls_ui;
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    LLGLDepthTest gls_depth(GL_TRUE);

    LLVector3 west_south_bottom_agent = gAgent.getPosAgentFromGlobal(west_south_bottom_global);
    F32 west    = west_south_bottom_agent.mV[VX];
    F32 south   = west_south_bottom_agent.mV[VY];
//  F32 bottom  = west_south_bottom_agent.mV[VZ] - 1.f;

    LLVector3 east_north_top_agent = gAgent.getPosAgentFromGlobal(east_north_top_global);
    F32 east    = east_north_top_agent.mV[VX];
    F32 north   = east_north_top_agent.mV[VY];
//  F32 top     = east_north_top_agent.mV[VZ] + 1.f;

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
    gGL.begin(LLRender::TRIANGLE_STRIP);
    {
        gGL.vertex3f(west, north, nw_bottom);
        gGL.vertex3f(west, north, nw_top);
        gGL.vertex3f(east, north, ne_bottom);
        gGL.vertex3f(east, north, ne_top);
        gGL.vertex3f(east, south, se_bottom);
        gGL.vertex3f(east, south, se_top);
        gGL.vertex3f(west, south, sw_top);
        gGL.vertex3f(west, south, sw_bottom);
        gGL.vertex3f(west, north, nw_top);
        gGL.vertex3f(west, north, nw_bottom);
    }
    gGL.end();

    LLUI::setLineWidth(1.f);
}


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

        gGL.vertex3f(x1, y1, z);

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


        gGL.texCoord2f(tex_coord1 * 0.5f + 0.5f, z1 * 0.5f);
        gGL.vertex3f(x1, y1, z1);

        gGL.texCoord2f(tex_coord2 * 0.5f + 0.5f, z2 * 0.5f);
        gGL.vertex3f(x2, y2, z2);

        // top edge stairsteps
        z = llmax(z2 + height, z1 + height);
        gGL.texCoord2f(tex_coord2 * 0.5f + 0.5f, z * 0.5f);
        gGL.vertex3f(x2, y2, z);

        gGL.texCoord2f(tex_coord1 * 0.5f + 0.5f, z1 * 0.5f);
        gGL.vertex3f(x1, y1, z1);

        gGL.texCoord2f(tex_coord2 * 0.5f + 0.5f, z * 0.5f);
        gGL.vertex3f(x2, y2, z);

        gGL.texCoord2f(tex_coord1 * 0.5f + 0.5f, z * 0.5f);
        gGL.vertex3f(x1, y1, z);
    }
}


void LLViewerParcelMgr::renderHighlightSegments(const U8* segments, LLViewerRegion* regionp)
{
    S32 x, y;
    F32 x1, y1; // start point
    F32 x2, y2; // end point
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
                    gGL.begin(LLRender::TRIANGLES);
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
                    gGL.begin(LLRender::TRIANGLES);
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


void LLViewerParcelMgr::renderCollisionSegments(U8* segments, bool use_pass, LLViewerRegion* regionp)
{

    S32 x, y;
    F32 x1, y1; // start point
    F32 x2, y2; // end point
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

    if (mCollisionBanned == BA_BANNED ||
        regionp->getRegionFlag(REGION_FLAGS_BLOCK_FLYOVER))
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

    gGL.begin(LLRender::TRIANGLES);

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
                else        direction = NORTH_MASK;

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
                else        direction = EAST_MASK;

                // avoid Z fighting
                renderOneSegment(x1+0.1f, y1+0.1f, x2+0.1f, y2+0.1f, collision_height, direction, regionp);

            }
        }
    }

    gGL.end();
}

void LLViewerParcelMgr::resetCollisionTimer()
{
    mCollisionTimer.reset();
    mRenderCollision = true;
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

void draw_cross_lines(const LLVector3& center, F32 dx, F32 dy, F32 dz)
{
    gGL.vertex3f(center.mV[VX] - dx, center.mV[VY], center.mV[VZ]);
    gGL.vertex3f(center.mV[VX] + dx, center.mV[VY], center.mV[VZ]);
    gGL.vertex3f(center.mV[VX], center.mV[VY] - dy, center.mV[VZ]);
    gGL.vertex3f(center.mV[VX], center.mV[VY] + dy, center.mV[VZ]);
    gGL.vertex3f(center.mV[VX], center.mV[VY], center.mV[VZ] - dz);
    gGL.vertex3f(center.mV[VX], center.mV[VY], center.mV[VZ] + dz);
}

void LLViewerObjectList::renderObjectBeacons()
{
    if (mDebugBeacons.empty())
    {
        return;
    }

    LLGLSUIDefault gls_ui;

    gUIProgram.bind();

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
            gGL.color4fv(linearColor4(color).mV);
            draw_cross_lines(thisline, 2.0f, 2.0f, 50.f);
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
            gGL.color4fv(linearColor4(debug_beacon.mColor).mV);
            draw_cross_lines(thisline, 0.5f, 0.5f, 0.5f);
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

            hud_textp->setZCompare(false);
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

void LLSky::renderSunMoonBeacons(const LLVector3& pos_agent, const LLVector3& direction, LLColor4 color)
{
    LLGLSUIDefault gls_ui;
    gUIProgram.bind();
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

    LLVector3 pos_end;
    for (S32 i = 0; i < 3; ++i)
    {
        pos_end.mV[i] = pos_agent.mV[i] + (50 * direction.mV[i]);
    }
    glLineWidth((GLfloat)LLPipeline::DebugBeaconLineWidth);
    gGL.begin(LLRender::LINES);
    color.mV[3] *= 0.5f;
    gGL.color4fv(color.mV);
    draw_cross_lines(pos_agent, 0.5f, 0.5f, 0.5f);
    draw_cross_lines(pos_end, 2.f, 2.f, 2.f);
    gGL.vertex3fv(pos_agent.mV);
    gGL.vertex3fv(pos_end.mV);
    gGL.end();

    gGL.flush();
    glLineWidth(1.f);

}

//-----------------------------------------------------------------------------
// gpu_benchmark() helper classes
//-----------------------------------------------------------------------------

// This struct is used to ensure that once we call initProfile(), it will
// definitely be matched by a corresponding call to finishProfile(). It's
// a struct rather than a class simply because every member is public.
struct ShaderProfileHelper
{
    ShaderProfileHelper()
    {
        LLGLSLShader::initProfile();
    }
    ~ShaderProfileHelper()
    {
        LLGLSLShader::finishProfile();
    }
};

// This helper class is used to ensure that each generateTextures() call
// is matched by a corresponding deleteTextures() call. It also handles
// the bindManual() calls using those textures.
class TextureHolder
{
public:
    TextureHolder(U32 unit, U32 size) :
        texUnit(gGL.getTexUnit(unit)),
        source(size)            // preallocate vector
    {
        // takes (count, pointer)
        // &vector[0] gets pointer to contiguous array
        LLImageGL::generateTextures(static_cast<S32>(source.size()), &source[0]);
    }

    ~TextureHolder()
    {
        // unbind
        if (texUnit)
        {
                texUnit->unbind(LLTexUnit::TT_TEXTURE);
        }
        // ensure that we delete these textures regardless of how we exit
        LLImageGL::deleteTextures(static_cast<S32>(source.size()), &source[0]);
    }

    bool bind(U32 index)
    {
        if (texUnit) // should always be there with dummy (-1), but just in case
        {
            return texUnit->bindManual(LLTexUnit::TT_TEXTURE, source[index]);
        }
        return false;
    }

private:
    // capture which LLTexUnit we're going to use
    LLTexUnit* texUnit;

    // use std::vector for implicit resource management
    std::vector<U32> source;
};

class ShaderBinder
{
public:
    ShaderBinder(LLGLSLShader& shader) :
        mShader(shader)
    {
        mShader.bind();
    }
    ~ShaderBinder()
    {
        mShader.unbind();
    }

private:
    LLGLSLShader& mShader;
};


//-----------------------------------------------------------------------------
// gpu_benchmark()
//  returns measured memory bandwidth of GPU in gigabytes per second
//-----------------------------------------------------------------------------
F32 gpu_benchmark()
{
    if (gGLManager.mGLVersion < 3.3f)
    { // don't bother benchmarking venerable drivers which don't support accurate timing anyway
        return -1.f;
    }

    if (gBenchmarkProgram.mProgramObject == 0)
    {
        LLViewerShaderMgr::instance()->initAttribsAndUniforms();

        gBenchmarkProgram.mName = "Benchmark Shader";
        gBenchmarkProgram.mFeatures.attachNothing = true;
        gBenchmarkProgram.mShaderFiles.clear();
        gBenchmarkProgram.mShaderFiles.push_back(std::make_pair("interface/benchmarkV.glsl", GL_VERTEX_SHADER));
        gBenchmarkProgram.mShaderFiles.push_back(std::make_pair("interface/benchmarkF.glsl", GL_FRAGMENT_SHADER));
        gBenchmarkProgram.mShaderLevel = 1;
        if (!gBenchmarkProgram.createShader())
        {
            return -1.f;
        }
    }

    LLGLDisable blend(GL_BLEND);

    //measure memory bandwidth by:
    // - allocating a batch of textures and render targets
    // - rendering those textures to those render targets
    // - recording time taken
    // - taking the median time for a given number of samples

    //resolution of textures/render targets
    const U32 res = 1024;

    //number of textures
    const U32 count = 32;

    //number of samples to take
    const S32 samples = 64;

    //time limit, allocation operations shouldn't take longer then 30 seconds, same for actual benchmark.
    const F32 time_limit = 30;

    std::vector<LLRenderTarget> dest(count);
    TextureHolder texHolder(0, count);
    std::vector<F32> results;

    //build a random texture
    U8* pixels = new U8[res*res*4];

    for (U32 i = 0; i < res*res*4; ++i)
    {
        pixels[i] = (U8) ll_rand(255);
    }

    gGL.setColorMask(true, true);
    LLGLDepthTest depth(GL_FALSE);

    LLTimer alloc_timer;
    alloc_timer.start();
    for (U32 i = 0; i < count; ++i)
    {
        //allocate render targets and textures
        if (!dest[i].allocate(res, res, GL_RGBA))
        {
            LL_WARNS("Benchmark") << "Failed to allocate render target." << LL_ENDL;
            // abandon the benchmark test
            delete[] pixels;
            return -1.f;
        }
        dest[i].bindTarget();
        dest[i].clear();
        dest[i].flush();

        if (!texHolder.bind(i))
        {
            // can use a dummy value mDummyTexUnit = new LLTexUnit(-1);
            LL_WARNS("Benchmark") << "Failed to bind tex unit." << LL_ENDL;
            // abandon the benchmark test
            delete[] pixels;
            return -1.f;
        }
        LLImageGL::setManualImage(GL_TEXTURE_2D, 0, GL_RGBA, res,res,GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        // disable mipmaps and use point filtering to cause cache misses
        gGL.getTexUnit(0)->setHasMipMaps(false);
        gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);

        if (alloc_timer.getElapsedTimeF32() > time_limit)
        {
            // abandon the benchmark test
            LL_WARNS("Benchmark") << "Allocation operation took longer then 30 seconds, stopping." << LL_ENDL;
            delete[] pixels;
            return -1.f;
        }
    }

    delete [] pixels;

    //make a dummy triangle to draw with
    LLPointer<LLVertexBuffer> buff = new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX);

    if (!buff->allocateBuffer(3, 0))
    {
        LL_WARNS("Benchmark") << "Failed to allocate buffer during benchmark." << LL_ENDL;
        // abandon the benchmark test
        return -1.f;
    }

    LLStrider<LLVector3> v;

    if (! buff->getVertexStrider(v))
    {
        LL_WARNS("Benchmark") << "GL LLVertexBuffer::getVertexStrider() returned false, "
                   << "buff->getMappedData() is"
                   << (buff->getMappedData()? " not" : "")
                   << " NULL" << LL_ENDL;
        // abandon the benchmark test
        return -1.f;
    }

    // generate dummy triangle
    v[0].set(-1, 1, 0);
    v[1].set(-1, -3, 0);
    v[2].set(3, 1, 0);

    buff->unmapBuffer();

    LLGLSLShader::unbind();

    // run GPU timer benchmark
    {
        ShaderProfileHelper initProfile;
        dest[0].bindTarget();
        gBenchmarkProgram.bind();
        for (S32 c = 0; c < samples; ++c)
        {
            for (U32 i = 0; i < count; ++i)
            {
                texHolder.bind(i);
                buff->setBuffer();
                buff->drawArrays(LLRender::TRIANGLES, 0, 3);
            }
        }
        gBenchmarkProgram.unbind();
        dest[0].flush();
    }

    F32 ms = gBenchmarkProgram.mTimeElapsed/1000000.f;
    F32 seconds = ms/1000.f;

    F64 samples_drawn = (F64)gBenchmarkProgram.mSamplesDrawn;
    F64 gpixels_drawn = samples_drawn / 1000000000.0;
    F32 samples_sec = (F32)(gpixels_drawn/seconds);
    F32 gbps = samples_sec*4;  // 4 bytes per sample

    LL_INFOS("Benchmark") << "Memory bandwidth is " << llformat("%.3f", gbps) << " GB/sec according to ARB_timer_query, total time " << seconds << " seconds" << LL_ENDL;

    return gbps;
}
