/**
 * @file llnetmap.cpp
 * @author James Cook
 * @brief Display of surrounding regions, objects, and agents.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2001-2010, Linden Research, Inc.
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

#include "llnetmap.h"

// Library includes (should move below)
#include "indra_constants.h"
#include "llavatarnamecache.h"
#include "llmath.h"
#include "llfloaterreg.h"
#include "llfocusmgr.h"
#include "lllocalcliprect.h"
#include "llrender.h"
#include "llresmgr.h"
#include "llui.h"
#include "lltooltip.h"

#include "llglheaders.h"

// Viewer includes
#include "llagent.h"
#include "llagentcamera.h"
#include "llappviewer.h" // for gDisconnected
#include "llcallingcard.h" // LLAvatarTracker
#include "llfloaterland.h"
#include "llfloaterworldmap.h"
#include "llparcel.h"
#include "lltracker.h"
#include "llsurface.h"
#include "llurlmatch.h"
#include "llurlregistry.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llworld.h"
#include "llworldmapview.h"     // shared draw code

static LLDefaultChildRegistry::Register<LLNetMap> r1("net_map");

constexpr F32 LLNetMap::MAP_SCALE_MIN = 32;
constexpr F32 LLNetMap::MAP_SCALE_FAR = 32;
constexpr F32 LLNetMap::MAP_SCALE_MEDIUM = 128;
constexpr F32 LLNetMap::MAP_SCALE_CLOSE = 256;
constexpr F32 LLNetMap::MAP_SCALE_VERY_CLOSE = 1024;
constexpr F32 LLNetMap::MAP_SCALE_MAX = 4096;

constexpr F32 MAP_SCALE_ZOOM_FACTOR = 1.04f; // Zoom in factor per click of scroll wheel (4%)
constexpr F32 MIN_DOT_RADIUS = 3.5f;
constexpr F32 DOT_SCALE = 0.75f;
constexpr F32 MIN_PICK_SCALE = 2.f;
constexpr S32 MOUSE_DRAG_SLOP = 2;      // How far the mouse needs to move before we think it's a drag

constexpr F64 COARSEUPDATE_MAX_Z = 1020.0f;

LLNetMap::LLNetMap (const Params & p)
:   LLUICtrl (p),
    mBackgroundColor (p.bg_color()),
    mScale( MAP_SCALE_MEDIUM ),
    mPixelsPerMeter( MAP_SCALE_MEDIUM / REGION_WIDTH_METERS ),
    mObjectMapTPM(0.f),
    mObjectMapPixels(0.f),
    mCurPan(0.f, 0.f),
    mStartPan(0.f, 0.f),
    mPopupWorldPos(0.f, 0.f, 0.f),
    mMouseDown(0, 0),
    mPanning(false),
    mCentering(false),
    mUpdateNow(false),
    mObjectImageCenterGlobal( gAgentCamera.getCameraPositionGlobal() ),
    mObjectRawImagep(),
    mObjectImagep(),
    mClosestAgentToCursor(),
    mClosestAgentAtLastRightClick(),
    mToolTipMsg()
{
    mScale = gSavedSettings.getF32("MiniMapScale");
    if (gAgent.isFirstLogin())
    {
        // *HACK: On first run, set this to false for new users, otherwise the
        // default is true to maintain consistent experience for existing
        // users.
        gSavedSettings.setBOOL("MiniMapRotate", false);
    }
    mPixelsPerMeter = mScale / REGION_WIDTH_METERS;
    mDotRadius = llmax(DOT_SCALE * mPixelsPerMeter, MIN_DOT_RADIUS);
}

LLNetMap::~LLNetMap()
{
    auto menu = static_cast<LLMenuGL*>(mPopupMenuHandle.get());
    if (menu)
    {
        menu->die();
        mPopupMenuHandle.markDead();
    }
}

bool LLNetMap::postBuild()
{
    LLUICtrl::ScopedRegistrarHelper commitRegistrar;
    LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enableRegistrar;

    enableRegistrar.add("Minimap.Zoom.Check", boost::bind(&LLNetMap::isZoomChecked, this, _2));
    commitRegistrar.add("Minimap.Zoom.Set", boost::bind(&LLNetMap::setZoom, this, _2));
    commitRegistrar.add("Minimap.Tracker", boost::bind(&LLNetMap::handleStopTracking, this, _2));
    commitRegistrar.add("Minimap.Center.Activate", boost::bind(&LLNetMap::activateCenterMap, this, _2));
    enableRegistrar.add("Minimap.MapOrientation.Check", boost::bind(&LLNetMap::isMapOrientationChecked, this, _2));
    commitRegistrar.add("Minimap.MapOrientation.Set", boost::bind(&LLNetMap::setMapOrientation, this, _2));
    commitRegistrar.add("Minimap.AboutLand", boost::bind(&LLNetMap::popupShowAboutLand, this, _2));

    LLMenuGL* menu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_mini_map.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
    mPopupMenuHandle = menu->getHandle();
    menu->setItemEnabled("Re-center map", false);
    return true;
}

void LLNetMap::setScale( F32 scale )
{
    scale = llclamp(scale, MAP_SCALE_MIN, MAP_SCALE_MAX);
    mCurPan *= scale / mScale;
    mScale = scale;

    if (mObjectImagep.notNull())
    {
        F32 width = (F32)(getRect().getWidth());
        F32 height = (F32)(getRect().getHeight());
        F32 diameter = sqrt(width * width + height * height);
        F32 region_widths = diameter / mScale;
        F32 meters = region_widths * LLWorld::getInstance()->getRegionWidthInMeters();
        F32 num_pixels = (F32)mObjectImagep->getWidth();
        mObjectMapTPM = num_pixels / meters;
        mObjectMapPixels = diameter;
    }

    mPixelsPerMeter = mScale / REGION_WIDTH_METERS;
    mDotRadius = llmax(DOT_SCALE * mPixelsPerMeter, MIN_DOT_RADIUS);

    gSavedSettings.setF32("MiniMapScale", mScale);

    mUpdateNow = true;
}


///////////////////////////////////////////////////////////////////////////////////

void LLNetMap::draw()
{
    if (!LLWorld::instanceExists())
    {
        return;
    }
    LL_PROFILE_ZONE_SCOPED;
    static LLFrameTimer map_timer;
    static LLUIColor map_avatar_color = LLUIColorTable::instance().getColor("MapAvatarColor", LLColor4::white);
    static LLUIColor map_avatar_friend_color = LLUIColorTable::instance().getColor("MapAvatarFriendColor", LLColor4::white);
    static LLUIColor map_track_color = LLUIColorTable::instance().getColor("MapTrackColor", LLColor4::white);
    //static LLUIColor map_track_disabled_color = LLUIColorTable::instance().getColor("MapTrackDisabledColor", LLColor4::white);
    static LLUIColor map_frustum_color = LLUIColorTable::instance().getColor("MapFrustumColor", LLColor4::white);
    static LLUIColor map_parcel_outline_color = LLUIColorTable::instance().getColor("MapParcelOutlineColor", LLColor4(LLColor3(LLColor4::yellow), 0.5f));

    if (mObjectImagep.isNull())
    {
        createObjectImage();
    }

    static LLUICachedControl<bool> auto_center("MiniMapAutoCenter", true);
    bool auto_centering = auto_center && !mPanning;
    mCentering = mCentering && !mPanning;

    if (auto_centering || mCentering)
    {
        mCurPan = lerp(mCurPan, LLVector2(0.0f, 0.0f) , LLSmoothInterpolation::getInterpolant(0.1f));
    }
    bool centered = abs(mCurPan.mV[VX]) < 0.5f && abs(mCurPan.mV[VY]) < 0.5f;
    if (centered)
    {
        mCurPan.mV[0] = 0.0f;
        mCurPan.mV[1] = 0.0f;
        mCentering = false;
    }

    auto menu = static_cast<LLMenuGL*>(mPopupMenuHandle.get());
    if (menu)
    {
        bool can_recenter_map = !(centered || mCentering || auto_centering);
        menu->setItemEnabled("Re-center map", can_recenter_map);
    }
    updateAboutLandPopupButton();

    // Prepare a scissor region
    F32 rotation = 0;

    gGL.pushMatrix();
    gGL.pushUIMatrix();

    LLVector3 offset = gGL.getUITranslation();
    LLVector3 scale = gGL.getUIScale();

    gGL.loadIdentity();
    gGL.loadUIIdentity();

    gGL.scalef(scale.mV[0], scale.mV[1], scale.mV[2]);
    gGL.translatef(offset.mV[0], offset.mV[1], offset.mV[2]);

    {
        LLLocalClipRect clip(getLocalRect());
        {
            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

            gGL.matrixMode(LLRender::MM_MODELVIEW);

            // Draw background rectangle
            LLColor4 background_color = mBackgroundColor.get();
            gGL.color4fv( background_color.mV );
            gl_rect_2d(0, getRect().getHeight(), getRect().getWidth(), 0);
        }

        // region 0,0 is in the middle
        S32 center_sw_left = getRect().getWidth() / 2 + llfloor(mCurPan.mV[VX]);
        S32 center_sw_bottom = getRect().getHeight() / 2 + llfloor(mCurPan.mV[VY]);

        gGL.pushMatrix();

        gGL.translatef( (F32) center_sw_left, (F32) center_sw_bottom, 0.f);

        static LLUICachedControl<bool> rotate_map("MiniMapRotate", true);
        if( rotate_map )
        {
            // rotate subsequent draws to agent rotation
            rotation = atan2( LLViewerCamera::getInstance()->getAtAxis().mV[VX], LLViewerCamera::getInstance()->getAtAxis().mV[VY] );
            gGL.rotatef( rotation * RAD_TO_DEG, 0.f, 0.f, 1.f);
        }

        // figure out where agent is
        const S32 region_width = ll_round(LLWorld::getInstance()->getRegionWidthInMeters());
        const F32 scale_pixels_per_meter = mScale / region_width;

        for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
             iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
        {
            LLViewerRegion* regionp = *iter;
            // Find x and y position relative to camera's center.
            LLVector3 origin_agent = regionp->getOriginAgent();
            LLVector3 rel_region_pos = origin_agent - gAgentCamera.getCameraPositionAgent();
            F32 relative_x = rel_region_pos.mV[0] * scale_pixels_per_meter;
            F32 relative_y = rel_region_pos.mV[1] * scale_pixels_per_meter;

            // background region rectangle
            F32 bottom =    relative_y;
            F32 left =      relative_x;
            F32 top =       bottom + mScale ;
            F32 right =     left + mScale ;

            if (regionp == gAgent.getRegion())
            {
                gGL.color4f(1.f, 1.f, 1.f, 1.f);
            }
            else
            {
                gGL.color4f(0.8f, 0.8f, 0.8f, 1.f);
            }

            if (!regionp->isAlive())
            {
                gGL.color4f(1.f, 0.5f, 0.5f, 1.f);
            }

            // Draw using texture.
            gGL.getTexUnit(0)->bind(regionp->getLand().getSTexture());
            gGL.begin(LLRender::TRIANGLES);
            {
                gGL.texCoord2f(0.f, 1.f);
                gGL.vertex2f(left, top);
                gGL.texCoord2f(0.f, 0.f);
                gGL.vertex2f(left, bottom);
                gGL.texCoord2f(1.f, 0.f);
                gGL.vertex2f(right, bottom);

                gGL.texCoord2f(0.f, 1.f);
                gGL.vertex2f(left, top);
                gGL.texCoord2f(1.f, 0.f);
                gGL.vertex2f(right, bottom);
                gGL.texCoord2f(1.f, 1.f);
                gGL.vertex2f(right, top);
            }
            gGL.end();

            gGL.flush();
        }

        // Redraw object layer periodically
        if (mUpdateNow || (map_timer.getElapsedTimeF32() > 0.5f))
        {
            mUpdateNow = false;

            // Locate the centre of the object layer, accounting for panning
            LLVector3 new_center = globalPosToView(gAgentCamera.getCameraPositionGlobal());
            new_center.mV[VX] -= mCurPan.mV[VX];
            new_center.mV[VY] -= mCurPan.mV[VY];
            new_center.mV[VZ] = 0.f;
            mObjectImageCenterGlobal = viewPosToGlobal(llfloor(new_center.mV[VX]), llfloor(new_center.mV[VY]));

            // Create the base texture.
            LLImageDataLock lock(mObjectRawImagep);
            U8 *default_texture = mObjectRawImagep->getData();
            memset( default_texture, 0, mObjectImagep->getWidth() * mObjectImagep->getHeight() * mObjectImagep->getComponents() );

            // Draw objects
            gObjectList.renderObjectsForMap(*this);

            mObjectImagep->setSubImage(mObjectRawImagep, 0, 0, mObjectImagep->getWidth(), mObjectImagep->getHeight());

            map_timer.reset();
        }

        LLVector3 map_center_agent = gAgent.getPosAgentFromGlobal(mObjectImageCenterGlobal);
        LLVector3 camera_position = gAgentCamera.getCameraPositionAgent();
        map_center_agent -= camera_position;
        map_center_agent.mV[VX] *= scale_pixels_per_meter;
        map_center_agent.mV[VY] *= scale_pixels_per_meter;

        gGL.getTexUnit(0)->bind(mObjectImagep);
        F32 image_half_width = 0.5f*mObjectMapPixels;
        F32 image_half_height = 0.5f*mObjectMapPixels;

        gGL.begin(LLRender::TRIANGLES);
        {
            gGL.texCoord2f(0.f, 1.f);
            gGL.vertex2f(map_center_agent.mV[VX] - image_half_width, image_half_height + map_center_agent.mV[VY]);
            gGL.texCoord2f(0.f, 0.f);
            gGL.vertex2f(map_center_agent.mV[VX] - image_half_width, map_center_agent.mV[VY] - image_half_height);
            gGL.texCoord2f(1.f, 0.f);
            gGL.vertex2f(image_half_width + map_center_agent.mV[VX], map_center_agent.mV[VY] - image_half_height);

            gGL.texCoord2f(0.f, 1.f);
            gGL.vertex2f(map_center_agent.mV[VX] - image_half_width, image_half_height + map_center_agent.mV[VY]);
            gGL.texCoord2f(1.f, 0.f);
            gGL.vertex2f(image_half_width + map_center_agent.mV[VX], map_center_agent.mV[VY] - image_half_height);
            gGL.texCoord2f(1.f, 1.f);
            gGL.vertex2f(image_half_width + map_center_agent.mV[VX], image_half_height + map_center_agent.mV[VY]);
        }
        gGL.end();

        for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
             iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
        {
            LLViewerRegion* regionp = *iter;
            regionp->renderPropertyLinesOnMinimap(scale_pixels_per_meter, map_parcel_outline_color.get().mV);
        }

        gGL.popMatrix();

        // Mouse pointer in local coordinates
        S32 local_mouse_x;
        S32 local_mouse_y;
        //localMouse(&local_mouse_x, &local_mouse_y);
        LLUI::getInstance()->getMousePositionLocal(this, &local_mouse_x, &local_mouse_y);
        mClosestAgentToCursor.setNull();
        F32 closest_dist_squared = F32_MAX; // value will be overridden in the loop
        F32 min_pick_dist_squared = (mDotRadius * MIN_PICK_SCALE) * (mDotRadius * MIN_PICK_SCALE);

        LLVector3 pos_map;
        uuid_vec_t avatar_ids;
        std::vector<LLVector3d> positions;
        bool unknown_relative_z;

        LLWorld::getInstance()->getAvatars(&avatar_ids, &positions, gAgentCamera.getCameraPositionGlobal());

        // Draw avatars
        for (U32 i = 0; i < avatar_ids.size(); i++)
        {
            LLUUID uuid = avatar_ids[i];
            // Skip self, we'll draw it later
            if (uuid == gAgent.getID()) continue;

            pos_map = globalPosToView(positions[i]);

            bool show_as_friend = (LLAvatarTracker::instance().getBuddyInfo(uuid) != NULL);

            LLColor4 color = show_as_friend ? map_avatar_friend_color : map_avatar_color;

            unknown_relative_z = positions[i].mdV[VZ] >= COARSEUPDATE_MAX_Z &&
                    camera_position.mV[VZ] >= COARSEUPDATE_MAX_Z;

            LLWorldMapView::drawAvatar(
                pos_map.mV[VX], pos_map.mV[VY],
                color,
                pos_map.mV[VZ], mDotRadius,
                unknown_relative_z);

            if(uuid.notNull())
            {
                bool selected = false;
                uuid_vec_t::iterator sel_iter = gmSelected.begin();
                for (; sel_iter != gmSelected.end(); sel_iter++)
                {
                    if(*sel_iter == uuid)
                    {
                        selected = true;
                        break;
                    }
                }
                if(selected)
                {
                    if( (pos_map.mV[VX] < 0) ||
                        (pos_map.mV[VY] < 0) ||
                        (pos_map.mV[VX] >= getRect().getWidth()) ||
                        (pos_map.mV[VY] >= getRect().getHeight()) )
                    {
                        S32 x = ll_round( pos_map.mV[VX] );
                        S32 y = ll_round( pos_map.mV[VY] );
                        LLWorldMapView::drawTrackingCircle( getRect(), x, y, color, 1, 10);
                    } else
                    {
                        LLWorldMapView::drawTrackingDot(pos_map.mV[VX],pos_map.mV[VY],color,0.f);
                    }
                }
            }

            F32 dist_to_cursor_squared = dist_vec_squared(LLVector2(pos_map.mV[VX], pos_map.mV[VY]),
                                          LLVector2((F32)local_mouse_x, (F32)local_mouse_y));
            if(dist_to_cursor_squared < min_pick_dist_squared && dist_to_cursor_squared < closest_dist_squared)
            {
                closest_dist_squared = dist_to_cursor_squared;
                mClosestAgentToCursor = uuid;
            }
        }

        // Draw dot for autopilot target
        if (gAgent.getAutoPilot())
        {
            drawTracking( gAgent.getAutoPilotTargetGlobal(), map_track_color );
        }
        else
        {
            LLTracker::ETrackingStatus tracking_status = LLTracker::getTrackingStatus();
            if (  LLTracker::TRACKING_AVATAR == tracking_status )
            {
                drawTracking( LLAvatarTracker::instance().getGlobalPos(), map_track_color );
            }
            else if ( LLTracker::TRACKING_LANDMARK == tracking_status
                    || LLTracker::TRACKING_LOCATION == tracking_status )
            {
                drawTracking( LLTracker::getTrackedPositionGlobal(), map_track_color );
            }
        }

        // Draw dot for self avatar position
        LLVector3d pos_global = gAgent.getPositionGlobal();
        pos_map = globalPosToView(pos_global);
        S32 dot_width = ll_round(mDotRadius * 2.f);
        LLUIImagePtr you = LLWorldMapView::sAvatarYouLargeImage;
        if (you)
        {
            you->draw(ll_round(pos_map.mV[VX] - mDotRadius),
                      ll_round(pos_map.mV[VY] - mDotRadius),
                      dot_width,
                      dot_width);

            F32 dist_to_cursor_squared = dist_vec_squared(LLVector2(pos_map.mV[VX], pos_map.mV[VY]),
                                          LLVector2((F32)local_mouse_x, (F32)local_mouse_y));
            if(dist_to_cursor_squared < min_pick_dist_squared && dist_to_cursor_squared < closest_dist_squared)
            {
                mClosestAgentToCursor = gAgent.getID();
            }
        }

        // Draw frustum
        F32 meters_to_pixels = mScale/ LLWorld::getInstance()->getRegionWidthInMeters();

        F32 horiz_fov = LLViewerCamera::getInstance()->getView() * LLViewerCamera::getInstance()->getAspect();
        F32 far_clip_meters = LLViewerCamera::getInstance()->getFar();
        F32 far_clip_pixels = far_clip_meters * meters_to_pixels;

        F32 ctr_x = (F32)center_sw_left;
        F32 ctr_y = (F32)center_sw_bottom;

        const F32 steps_per_circle = 40.0f;
        const F32 steps_per_radian = steps_per_circle / F_TWO_PI;
        const F32 arc_start = -(horiz_fov / 2.0f) + F_PI_BY_TWO;
        const F32 arc_end = (horiz_fov / 2.0f) + F_PI_BY_TWO;
        const S32 steps = llmax(1, (S32)((horiz_fov * steps_per_radian) + 0.5f));

        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

        if( rotate_map )
        {
            gGL.pushMatrix();
                gGL.translatef( ctr_x, ctr_y, 0 );
                gl_washer_segment_2d(far_clip_pixels, 0, arc_start, arc_end, steps, map_frustum_color(), map_frustum_color());
            gGL.popMatrix();
        }
        else
        {
            gGL.pushMatrix();
                gGL.translatef( ctr_x, ctr_y, 0 );
                // If we don't rotate the map, we have to rotate the frustum.
                gGL.rotatef( atan2( LLViewerCamera::getInstance()->getAtAxis().mV[VX], LLViewerCamera::getInstance()->getAtAxis().mV[VY] ) * RAD_TO_DEG, 0.f, 0.f, -1.f);
                gl_washer_segment_2d(far_clip_pixels, 0, arc_start, arc_end, steps, map_frustum_color(), map_frustum_color());
            gGL.popMatrix();
        }
    }

    gGL.popMatrix();
    gGL.popUIMatrix();

    LLUICtrl::draw();
}

void LLNetMap::reshape(S32 width, S32 height, bool called_from_parent)
{
    LLUICtrl::reshape(width, height, called_from_parent);
    createObjectImage();
}

LLVector3 LLNetMap::globalPosToView(const LLVector3d& global_pos)
{
    LLVector3d camera_position = gAgentCamera.getCameraPositionGlobal();

    LLVector3d relative_pos_global = global_pos - camera_position;
    LLVector3 pos_local;
    pos_local.setVec(relative_pos_global);  // convert to floats from doubles

    pos_local.mV[VX] *= mPixelsPerMeter;
    pos_local.mV[VY] *= mPixelsPerMeter;
    // leave Z component in meters

    static LLUICachedControl<bool> rotate_map("MiniMapRotate", true);
    if( rotate_map )
    {
        F32 radians = atan2( LLViewerCamera::getInstance()->getAtAxis().mV[VX], LLViewerCamera::getInstance()->getAtAxis().mV[VY] );
        LLQuaternion rot(radians, LLVector3(0.f, 0.f, 1.f));
        pos_local.rotVec( rot );
    }

    pos_local.mV[VX] += getRect().getWidth() / 2 + mCurPan.mV[VX];
    pos_local.mV[VY] += getRect().getHeight() / 2 + mCurPan.mV[VY];

    return pos_local;
}

void LLNetMap::drawTracking(const LLVector3d& pos_global, const LLColor4& color,
                            bool draw_arrow )
{
    LLVector3 pos_local = globalPosToView(pos_global);
    if( (pos_local.mV[VX] < 0) ||
        (pos_local.mV[VY] < 0) ||
        (pos_local.mV[VX] >= getRect().getWidth()) ||
        (pos_local.mV[VY] >= getRect().getHeight()) )
    {
        if (draw_arrow)
        {
            S32 x = ll_round( pos_local.mV[VX] );
            S32 y = ll_round( pos_local.mV[VY] );
            LLWorldMapView::drawTrackingCircle( getRect(), x, y, color, 1, 10 );
            LLWorldMapView::drawTrackingArrow( getRect(), x, y, color );
        }
    }
    else
    {
        LLWorldMapView::drawTrackingDot(pos_local.mV[VX],
                                        pos_local.mV[VY],
                                        color,
                                        pos_local.mV[VZ]);
    }
}

bool LLNetMap::isMouseOnPopupMenu()
{
    auto menu = static_cast<LLMenuGL*>(mPopupMenuHandle.get());
    if (!menu || !menu->isOpen())
    {
        return false;
    }

    S32 popup_x;
    S32 popup_y;
    LLUI::getInstance()->getMousePositionLocal(menu, &popup_x, &popup_y);
    // *NOTE: Tolerance is larger than it needs to be because the context menu is offset from the mouse when the menu is opened from certain
    // directions. This may be a quirk of LLMenuGL::showPopup. -Cosmic,2022-03-22
    constexpr S32 tolerance = 10;
    // Test tolerance from all four corners, as the popup menu can appear from a different direction if there's not enough space.
    // Assume the size of the popup menu is much larger than the provided tolerance.
    // In practice, this is a [tolerance]px margin around the popup menu.
    for (S32 sign_x = -1; sign_x <= 1; sign_x += 2)
    {
        for (S32 sign_y = -1; sign_y <= 1; sign_y += 2)
        {
            if (menu->pointInView(popup_x + (sign_x * tolerance), popup_y + (sign_y * tolerance)))
            {
                return true;
            }
        }
    }
    return false;
}

void LLNetMap::updateAboutLandPopupButton()
{
    auto menu = static_cast<LLMenuGL*>(mPopupMenuHandle.get());
    if (!menu || !menu->isOpen())
    {
        return;
    }

    LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosGlobal(mPopupWorldPos);
    if (!region)
    {
        menu->setItemEnabled("About Land", false);
    }
    else
    {
        // Check if the mouse is in the bounds of the popup. If so, it's safe to assume no other hover function will be called, so the hover
        // parcel can be used to check if location-sensitive tooltip options are available.
        if (isMouseOnPopupMenu())
        {
            LLViewerParcelMgr::getInstance()->setHoverParcel(mPopupWorldPos);
            LLParcel *hover_parcel = LLViewerParcelMgr::getInstance()->getHoverParcel();
            bool      valid_parcel = false;
            if (hover_parcel)
            {
                valid_parcel = hover_parcel->getOwnerID().notNull();
            }
            menu->setItemEnabled("About Land", valid_parcel);
        }
    }
}

LLVector3d LLNetMap::viewPosToGlobal( S32 x, S32 y )
{
    x -= ll_round(getRect().getWidth() / 2 + mCurPan.mV[VX]);
    y -= ll_round(getRect().getHeight() / 2 + mCurPan.mV[VY]);

    LLVector3 pos_local( (F32)x, (F32)y, 0 );

    F32 radians = - atan2( LLViewerCamera::getInstance()->getAtAxis().mV[VX], LLViewerCamera::getInstance()->getAtAxis().mV[VY] );

    static LLUICachedControl<bool> rotate_map("MiniMapRotate", true);
    if( rotate_map )
    {
        LLQuaternion rot(radians, LLVector3(0.f, 0.f, 1.f));
        pos_local.rotVec( rot );
    }

    pos_local *= ( LLWorld::getInstance()->getRegionWidthInMeters() / mScale );

    LLVector3d pos_global;
    pos_global.setVec( pos_local );
    pos_global += gAgentCamera.getCameraPositionGlobal();

    return pos_global;
}

bool LLNetMap::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
    // note that clicks are reversed from what you'd think: i.e. > 0  means zoom out, < 0 means zoom in
    F32 new_scale = mScale * (F32)pow(MAP_SCALE_ZOOM_FACTOR, -clicks);
    F32 old_scale = mScale;

    setScale(new_scale);

    static LLUICachedControl<bool> auto_center("MiniMapAutoCenter", true);
    if (!auto_center)
    {
        // Adjust pan to center the zoom on the mouse pointer
        LLVector2 zoom_offset;
        zoom_offset.mV[VX] = (F32)(x - getRect().getWidth() / 2);
        zoom_offset.mV[VY] = (F32)(y - getRect().getHeight() / 2);
        mCurPan -= zoom_offset * mScale / old_scale - zoom_offset;
    }

    return true;
}

bool LLNetMap::handleToolTip(S32 x, S32 y, MASK mask)
{
    if (gDisconnected)
    {
        return false;
    }

    // If the cursor is near an avatar on the minimap, a mini-inspector will be
    // shown for the avatar, instead of the normal map tooltip.
    if (handleToolTipAgent(mClosestAgentToCursor))
    {
        return true;
    }

    // The popup menu uses the hover parcel when it is open and the mouse is on
    // top of it, with some additional tolerance. Returning early here prevents
    // fighting over that hover parcel when getting tooltip info in the
    // tolerance region.
    if (isMouseOnPopupMenu())
    {
        return false;
    }

    LLRect sticky_rect;
    S32 SLOP = 4;
    localPointToScreen(x - SLOP, y - SLOP, &(sticky_rect.mLeft), &(sticky_rect.mBottom));
    sticky_rect.mRight = sticky_rect.mLeft + 2 * SLOP;
    sticky_rect.mTop   = sticky_rect.mBottom + 2 * SLOP;

    std::string parcel_name_msg;
    std::string parcel_sale_price_msg;
    std::string parcel_sale_area_msg;
    std::string parcel_owner_msg;
    std::string region_name_msg;

    LLVector3d      posGlobal = viewPosToGlobal(x, y);
    LLViewerRegion *region    = LLWorld::getInstance()->getRegionFromPosGlobal(posGlobal);
    if (region)
    {
        std::string region_name = region->getName();
        if (!region_name.empty())
        {
            region_name_msg = mRegionNameMsg;
            LLStringUtil::format(region_name_msg, {{"[REGION_NAME]", region_name}});
        }

        // Only show parcel information in the tooltip if property lines are visible. Otherwise, the parcel the tooltip is referring to is
        // ambiguous.
        if (gSavedSettings.getBOOL("MiniMapShowPropertyLines"))
        {
            LLViewerParcelMgr::getInstance()->setHoverParcel(posGlobal);
            LLParcel *hover_parcel = LLViewerParcelMgr::getInstance()->getHoverParcel();
            if (hover_parcel)
            {
                std::string parcel_name = hover_parcel->getName();
                if (!parcel_name.empty())
                {
                    parcel_name_msg = mParcelNameMsg;
                    LLStringUtil::format(parcel_name_msg, {{"[PARCEL_NAME]", parcel_name}});
                }

                const LLUUID      parcel_owner          = hover_parcel->getOwnerID();
                std::string       parcel_owner_name_url = LLSLURL("agent", parcel_owner, "inspect").getSLURLString();
                static LLUrlMatch parcel_owner_name_url_match;
                LLUrlRegistry::getInstance()->findUrl(parcel_owner_name_url, parcel_owner_name_url_match);
                if (!parcel_owner_name_url_match.empty())
                {
                    parcel_owner_msg              = mParcelOwnerMsg;
                    std::string parcel_owner_name = parcel_owner_name_url_match.getLabel();
                    LLStringUtil::format(parcel_owner_msg, {{"[PARCEL_OWNER]", parcel_owner_name}});
                }

                if (hover_parcel->getForSale())
                {
                    const LLUUID auth_buyer_id = hover_parcel->getAuthorizedBuyerID();
                    const LLUUID agent_id      = gAgent.getID();
                    bool         show_for_sale = auth_buyer_id.isNull() || auth_buyer_id == agent_id || parcel_owner == agent_id;
                    if (show_for_sale)
                    {
                        S32 price        = hover_parcel->getSalePrice();
                        S32 area         = hover_parcel->getArea();
                        F32 cost_per_sqm = 0.0f;
                        if (area > 0)
                        {
                            cost_per_sqm = F32(price) / area;
                        }
                        std::string formatted_price          = LLResMgr::getInstance()->getMonetaryString(price);
                        std::string formatted_cost_per_meter = llformat("%.1f", cost_per_sqm);
                        parcel_sale_price_msg                = mParcelSalePriceMsg;
                        LLStringUtil::format(parcel_sale_price_msg,
                                             {{"[PRICE]", formatted_price}, {"[PRICE_PER_SQM]", formatted_cost_per_meter}});
                        std::string formatted_area = llformat("%d", area);
                        parcel_sale_area_msg       = mParcelSaleAreaMsg;
                        LLStringUtil::format(parcel_sale_area_msg, {{"[AREA]", formatted_area}});
                    }
                }
            }
        }
    }

    std::string tool_tip_hint_msg;
    if (gSavedSettings.getBOOL("DoubleClickTeleport"))
    {
        tool_tip_hint_msg = mAltToolTipHintMsg;
    }
    else if (gSavedSettings.getBOOL("DoubleClickShowWorldMap"))
    {
        tool_tip_hint_msg = mToolTipHintMsg;
    }

    LLStringUtil::format_map_t args;
    args["[PARCEL_NAME_MSG]"]       = parcel_name_msg.empty() ? "" : parcel_name_msg + '\n';
    args["[PARCEL_SALE_PRICE_MSG]"] = parcel_sale_price_msg.empty() ? "" : parcel_sale_price_msg + '\n';
    args["[PARCEL_SALE_AREA_MSG]"]  = parcel_sale_area_msg.empty() ? "" : parcel_sale_area_msg + '\n';
    args["[PARCEL_OWNER_MSG]"]      = parcel_owner_msg.empty() ? "" : parcel_owner_msg + '\n';
    args["[REGION_NAME_MSG]"]       = region_name_msg.empty() ? "" : region_name_msg + '\n';
    args["[TOOL_TIP_HINT_MSG]"]     = tool_tip_hint_msg.empty() ? "" : tool_tip_hint_msg + '\n';

    std::string msg                 = mToolTipMsg;
    LLStringUtil::format(msg, args);
    if (msg.back() == '\n')
    {
        msg.resize(msg.size() - 1);
    }
    LLToolTipMgr::instance().show(LLToolTip::Params().message(msg).sticky_rect(sticky_rect));

    return true;
}

bool LLNetMap::handleToolTipAgent(const LLUUID& avatar_id)
{
    LLAvatarName av_name;
    if (avatar_id.isNull() || !LLAvatarNameCache::get(avatar_id, &av_name))
    {
        return false;
    }

    // only show tooltip if same inspector not already open
    LLFloater* existing_inspector = LLFloaterReg::findInstance("inspect_avatar");
    if (!existing_inspector
        || !existing_inspector->getVisible()
        || existing_inspector->getKey()["avatar_id"].asUUID() != avatar_id)
    {
        LLInspector::Params p;
        p.fillFrom(LLUICtrlFactory::instance().getDefaultParams<LLInspector>());
        p.message(av_name.getCompleteName());
        p.image.name("Inspector_I");
        p.click_callback(boost::bind(showAvatarInspector, avatar_id));
        p.visible_time_near(6.f);
        p.visible_time_far(3.f);
        p.delay_time(0.35f);
        p.wrap(false);

        LLToolTipMgr::instance().show(p);
    }
    return true;
}

// static
void LLNetMap::showAvatarInspector(const LLUUID& avatar_id)
{
    LLSD params;
    params["avatar_id"] = avatar_id;

    if (LLToolTipMgr::instance().toolTipVisible())
    {
        LLRect rect = LLToolTipMgr::instance().getToolTipRect();
        params["pos"]["x"] = rect.mLeft;
        params["pos"]["y"] = rect.mTop;
    }

    LLFloaterReg::showInstance("inspect_avatar", params);
}

void LLNetMap::renderScaledPointGlobal( const LLVector3d& pos, const LLColor4U &color, F32 radius_meters )
{
    LLVector3 local_pos;
    local_pos.setVec( pos - mObjectImageCenterGlobal );

    S32 diameter_pixels = ll_round(2 * radius_meters * mObjectMapTPM);
    renderPoint( local_pos, color, diameter_pixels );
}


void LLNetMap::renderPoint(const LLVector3 &pos_local, const LLColor4U &color,
                           S32 diameter, S32 relative_height)
{
    if (diameter <= 0)
    {
        return;
    }

    const S32 image_width = (S32)mObjectImagep->getWidth();
    const S32 image_height = (S32)mObjectImagep->getHeight();

    S32 x_offset = ll_round(pos_local.mV[VX] * mObjectMapTPM + image_width / 2);
    S32 y_offset = ll_round(pos_local.mV[VY] * mObjectMapTPM + image_height / 2);

    if ((x_offset < 0) || (x_offset >= image_width))
    {
        return;
    }
    if ((y_offset < 0) || (y_offset >= image_height))
    {
        return;
    }

    LLImageDataLock lock(mObjectRawImagep);
    U8 *datap = mObjectRawImagep->getData();

    S32 neg_radius = diameter / 2;
    S32 pos_radius = diameter - neg_radius;
    S32 x, y;

    if (relative_height > 0)
    {
        // ...point above agent
        S32 px, py;

        // vertical line
        px = x_offset;
        for (y = -neg_radius; y < pos_radius; y++)
        {
            py = y_offset + y;
            if ((py < 0) || (py >= image_height))
            {
                continue;
            }
            S32 offset = px + py * image_width;
            ((U32*)datap)[offset] = color.asRGBA();
        }

        // top line
        py = y_offset + pos_radius - 1;
        for (x = -neg_radius; x < pos_radius; x++)
        {
            px = x_offset + x;
            if ((px < 0) || (px >= image_width))
            {
                continue;
            }
            S32 offset = px + py * image_width;
            ((U32*)datap)[offset] = color.asRGBA();
        }
    }
    else
    {
        // ...point level with agent
        for (x = -neg_radius; x < pos_radius; x++)
        {
            S32 p_x = x_offset + x;
            if ((p_x < 0) || (p_x >= image_width))
            {
                continue;
            }

            for (y = -neg_radius; y < pos_radius; y++)
            {
                S32 p_y = y_offset + y;
                if ((p_y < 0) || (p_y >= image_height))
                {
                    continue;
                }
                S32 offset = p_x + p_y * image_width;
                ((U32*)datap)[offset] = color.asRGBA();
            }
        }
    }
}

void LLNetMap::createObjectImage()
{
    // Find the size of the side of a square that surrounds the circle that surrounds getRect().
    // ... which is, the diagonal of the rect.
    F32 width = (F32)getRect().getWidth();
    F32 height = (F32)getRect().getHeight();
    S32 square_size = ll_round( sqrt(width*width + height*height) );

    // Find the least power of two >= the minimum size.
    const S32 MIN_SIZE = 64;
    const S32 MAX_SIZE = 256;
    S32 img_size = MIN_SIZE;
    while( (img_size*2 < square_size ) && (img_size < MAX_SIZE) )
    {
        img_size <<= 1;
    }

    if( mObjectImagep.isNull() ||
        (mObjectImagep->getWidth() != img_size) ||
        (mObjectImagep->getHeight() != img_size) )
    {
        mObjectRawImagep = new LLImageRaw(img_size, img_size, 4);
        U8* data = mObjectRawImagep->getData();
        memset( data, 0, img_size * img_size * 4 );
        mObjectImagep = LLViewerTextureManager::getLocalTexture( mObjectRawImagep.get(), false);
    }
    setScale(mScale);
    mUpdateNow = true;
}

bool LLNetMap::handleMouseDown(S32 x, S32 y, MASK mask)
{
    // Start panning
    gFocusMgr.setMouseCapture(this);

    mStartPan     = mCurPan;
    mMouseDown.mX = x;
    mMouseDown.mY = y;
    return true;
}

bool LLNetMap::handleMouseUp(S32 x, S32 y, MASK mask)
{
    if (abs(mMouseDown.mX - x) < 3 && abs(mMouseDown.mY - y) < 3)
    {
        handleClick(x, y, mask);
    }

    if (hasMouseCapture())
    {
        if (mPanning)
        {
            // restore mouse cursor
            S32 local_x, local_y;
            local_x          = mMouseDown.mX + llfloor(mCurPan.mV[VX] - mStartPan.mV[VX]);
            local_y          = mMouseDown.mY + llfloor(mCurPan.mV[VY] - mStartPan.mV[VY]);
            LLRect clip_rect = getRect();
            clip_rect.stretch(-8);
            clip_rect.clipPointToRect(mMouseDown.mX, mMouseDown.mY, local_x, local_y);
            LLUI::getInstance()->setMousePositionLocal(this, local_x, local_y);

            // finish the pan
            mPanning = false;

            mMouseDown.set(0, 0);
        }
        gViewerWindow->showCursor();
        gFocusMgr.setMouseCapture(NULL);
        return true;
    }

    return false;
}

bool LLNetMap::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    auto menu = static_cast<LLMenuGL*>(mPopupMenuHandle.get());
    if (menu)
    {
        mPopupWorldPos = viewPosToGlobal(x, y);
        menu->buildDrawLabels();
        menu->updateParent(LLMenuGL::sMenuContainer);
        menu->setItemEnabled("Stop Tracking", LLTracker::isTracking(0));
        LLMenuGL::showPopup(this, menu, x, y);
    }
    return true;
}

bool LLNetMap::handleClick(S32 x, S32 y, MASK mask)
{
    // TODO: allow clicking an avatar on minimap to select avatar in the nearby avatar list
    // if(mClosestAgentToCursor.notNull())
    //     mNearbyList->selectUser(mClosestAgentToCursor);
    // Needs a registered observer i guess to accomplish this without using
    // globals to tell the mNearbyList in llpeoplepanel to select the user
    return true;
}

bool LLNetMap::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    LLVector3d pos_global = viewPosToGlobal(x, y);

    bool double_click_teleport = gSavedSettings.getBOOL("DoubleClickTeleport");
    bool double_click_show_world_map = gSavedSettings.getBOOL("DoubleClickShowWorldMap");

    if (double_click_teleport || double_click_show_world_map)
    {
        // If we're not tracking a beacon already, double-click will set one
        if (!LLTracker::isTracking(NULL))
        {
            LLFloaterWorldMap* world_map = LLFloaterWorldMap::getInstance();
            if (world_map)
            {
                world_map->trackLocation(pos_global);
            }
        }
    }

    if (double_click_teleport)
    {
        // If DoubleClickTeleport is on, double clicking the minimap will teleport there
        gAgent.teleportViaLocationLookAt(pos_global);
    }
    else if (double_click_show_world_map)
    {
        LLFloaterReg::showInstance("world_map");
    }
    return true;
}

F32 LLNetMap::getScaleForName(std::string scale_name)
{
    if (scale_name == "very close")
    {
        return LLNetMap::MAP_SCALE_VERY_CLOSE;
    }
    else if (scale_name == "close")
    {
        return LLNetMap::MAP_SCALE_CLOSE;
    }
    else if (scale_name == "medium")
    {
        return LLNetMap::MAP_SCALE_MEDIUM;
    }
    else if (scale_name == "far")
    {
        return LLNetMap::MAP_SCALE_FAR;
    }
    return 0.0f;
}

// static
bool LLNetMap::outsideSlop( S32 x, S32 y, S32 start_x, S32 start_y, S32 slop )
{
    S32 dx = x - start_x;
    S32 dy = y - start_y;

    return (dx <= -slop || slop <= dx || dy <= -slop || slop <= dy);
}

bool LLNetMap::handleHover( S32 x, S32 y, MASK mask )
{
    if (hasMouseCapture())
    {
        if (mPanning || outsideSlop(x, y, mMouseDown.mX, mMouseDown.mY, MOUSE_DRAG_SLOP))
        {
            if (!mPanning)
            {
                // Just started panning. Hide cursor.
                mPanning = true;
                gViewerWindow->hideCursor();
            }

            LLVector2 delta(static_cast<F32>(gViewerWindow->getCurrentMouseDX()),
                            static_cast<F32>(gViewerWindow->getCurrentMouseDY()));

            // Set pan to value at start of drag + offset
            mCurPan += delta;

            gViewerWindow->moveCursorToCenter();
        }
    }

    if (mask & MASK_SHIFT)
    {
        // If shift is held, change the cursor to hint that the map can be
        // dragged. However, holding shift is not required to drag the map.
        gViewerWindow->setCursor( UI_CURSOR_TOOLPAN );
    }
    else
    {
        gViewerWindow->setCursor( UI_CURSOR_CROSS );
    }

    return true;
}

bool LLNetMap::isZoomChecked(const LLSD &userdata)
{
    std::string level = userdata.asString();
    F32         scale = getScaleForName(level);
    return scale == mScale;
}

void LLNetMap::setZoom(const LLSD &userdata)
{
    std::string level = userdata.asString();
    F32         scale = getScaleForName(level);
    if (scale != 0.0f)
    {
        setScale(scale);
    }
}

void LLNetMap::handleStopTracking (const LLSD& userdata)
{
    auto menu = static_cast<LLMenuGL*>(mPopupMenuHandle.get());
    if (menu)
    {
        menu->setItemEnabled ("Stop Tracking", false);
        LLTracker::stopTracking (LLTracker::isTracking(NULL));
    }
}

void LLNetMap::activateCenterMap(const LLSD &userdata) { mCentering = true; }

bool LLNetMap::isMapOrientationChecked(const LLSD &userdata)
{
    const std::string command_name = userdata.asString();
    const bool        rotate_map   = gSavedSettings.getBOOL("MiniMapRotate");
    if (command_name == "north_at_top")
    {
        return !rotate_map;
    }

    if (command_name == "camera_at_top")
    {
        return rotate_map;
    }

    return false;
}

void LLNetMap::setMapOrientation(const LLSD &userdata)
{
    const std::string command_name = userdata.asString();
    if (command_name == "north_at_top")
    {
        gSavedSettings.setBOOL("MiniMapRotate", false);
    }
    else if (command_name == "camera_at_top")
    {
        gSavedSettings.setBOOL("MiniMapRotate", true);
    }
}

void LLNetMap::popupShowAboutLand(const LLSD &userdata)
{
    // Update parcel selection. It's important to deselect land first so the "About Land" floater doesn't refresh with the old selection.
    LLViewerParcelMgr::getInstance()->deselectLand();
    LLParcelSelectionHandle selection = LLViewerParcelMgr::getInstance()->selectParcelAt(mPopupWorldPos);
    gMenuHolder->setParcelSelection(selection);

    LLFloaterReg::showInstance("about_land", LLSD(), false);
}
