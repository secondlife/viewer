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
#include "llui.h"
#include "lltooltip.h"

#include "llglheaders.h"

// Viewer includes
#include "llagent.h"
#include "llagentcamera.h"
#include "llappviewer.h" // for gDisconnected
#include "llcallingcard.h" // LLAvatarTracker
#include "llfloaterworldmap.h"
#include "lltracker.h"
#include "llsurface.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llworld.h"
#include "llworldmapview.h"		// shared draw code

static LLDefaultChildRegistry::Register<LLNetMap> r1("net_map");

const F32 LLNetMap::MAP_SCALE_MIN = 32;
const F32 LLNetMap::MAP_SCALE_MID = 1024;
const F32 LLNetMap::MAP_SCALE_MAX = 4096;

const F32 MAP_SCALE_INCREMENT = 16;
const F32 MAP_SCALE_ZOOM_FACTOR = 1.04f; // Zoom in factor per click of scroll wheel (4%)
const F32 MIN_DOT_RADIUS = 3.5f;
const F32 DOT_SCALE = 0.75f;
const F32 MIN_PICK_SCALE = 2.f;
const S32 MOUSE_DRAG_SLOP = 2;		// How far the mouse needs to move before we think it's a drag

const F64 COARSEUPDATE_MAX_Z = 1020.0f;

LLNetMap::LLNetMap (const Params & p)
:	LLUICtrl (p),
	mBackgroundColor (p.bg_color()),
	mScale( MAP_SCALE_MID ),
	mPixelsPerMeter( MAP_SCALE_MID / REGION_WIDTH_METERS ),
	mObjectMapTPM(0.f),
	mObjectMapPixels(0.f),
	mTargetPan(0.f, 0.f),
	mCurPan(0.f, 0.f),
	mStartPan(0.f, 0.f),
	mMouseDown(0, 0),
	mPanning(false),
	mUpdateNow(false),
	mObjectImageCenterGlobal( gAgentCamera.getCameraPositionGlobal() ),
	mObjectRawImagep(),
	mObjectImagep(),
	mClosestAgentToCursor(),
	mClosestAgentAtLastRightClick(),
	mToolTipMsg(),
	mPopupMenu(NULL)
{
	mDotRadius = llmax(DOT_SCALE * mPixelsPerMeter, MIN_DOT_RADIUS);
	setScale(gSavedSettings.getF32("MiniMapScale"));
}

LLNetMap::~LLNetMap()
{
	gSavedSettings.setF32("MiniMapScale", mScale);
}

BOOL LLNetMap::postBuild()
{
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	
	registrar.add("Minimap.Zoom", boost::bind(&LLNetMap::handleZoom, this, _2));
	registrar.add("Minimap.Tracker", boost::bind(&LLNetMap::handleStopTracking, this, _2));

	mPopupMenu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_mini_map.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	return TRUE;
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

	mUpdateNow = true;
}


///////////////////////////////////////////////////////////////////////////////////

void LLNetMap::draw()
{
 	static LLFrameTimer map_timer;
	static LLUIColor map_avatar_color = LLUIColorTable::instance().getColor("MapAvatarColor", LLColor4::white);
	static LLUIColor map_avatar_friend_color = LLUIColorTable::instance().getColor("MapAvatarFriendColor", LLColor4::white);
	static LLUIColor map_track_color = LLUIColorTable::instance().getColor("MapTrackColor", LLColor4::white);
	//static LLUIColor map_track_disabled_color = LLUIColorTable::instance().getColor("MapTrackDisabledColor", LLColor4::white);
	static LLUIColor map_frustum_color = LLUIColorTable::instance().getColor("MapFrustumColor", LLColor4::white);
	static LLUIColor map_frustum_rotating_color = LLUIColorTable::instance().getColor("MapFrustumRotatingColor", LLColor4::white);
	
	if (mObjectImagep.isNull())
	{
		createObjectImage();
	}

	static LLUICachedControl<bool> auto_center("MiniMapAutoCenter", true);
	if (auto_center)
	{
		mCurPan = lerp(mCurPan, mTargetPan, LLCriticalDamp::getInterpolant(0.1f));
	}

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
		S32 region_width = llround(LLWorld::getInstance()->getRegionWidthInMeters());

		for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
			 iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
		{
			LLViewerRegion* regionp = *iter;
			// Find x and y position relative to camera's center.
			LLVector3 origin_agent = regionp->getOriginAgent();
			LLVector3 rel_region_pos = origin_agent - gAgentCamera.getCameraPositionAgent();
			F32 relative_x = (rel_region_pos.mV[0] / region_width) * mScale;
			F32 relative_y = (rel_region_pos.mV[1] / region_width) * mScale;

			// background region rectangle
			F32 bottom =	relative_y;
			F32 left =		relative_x;
			F32 top =		bottom + mScale ;
			F32 right =		left + mScale ;

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
			gGL.begin(LLRender::QUADS);
				gGL.texCoord2f(0.f, 1.f);
				gGL.vertex2f(left, top);
				gGL.texCoord2f(0.f, 0.f);
				gGL.vertex2f(left, bottom);
				gGL.texCoord2f(1.f, 0.f);
				gGL.vertex2f(right, bottom);
				gGL.texCoord2f(1.f, 1.f);
				gGL.vertex2f(right, top);
			gGL.end();

			// Draw water
			gGL.setAlphaRejectSettings(LLRender::CF_GREATER, ABOVE_WATERLINE_ALPHA / 255.f);
			{
				if (regionp->getLand().getWaterTexture())
				{
					gGL.getTexUnit(0)->bind(regionp->getLand().getWaterTexture());
					gGL.begin(LLRender::QUADS);
						gGL.texCoord2f(0.f, 1.f);
						gGL.vertex2f(left, top);
						gGL.texCoord2f(0.f, 0.f);
						gGL.vertex2f(left, bottom);
						gGL.texCoord2f(1.f, 0.f);
						gGL.vertex2f(right, bottom);
						gGL.texCoord2f(1.f, 1.f);
						gGL.vertex2f(right, top);
					gGL.end();
				}
			}
			gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
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
		map_center_agent.mV[VX] *= mScale/region_width;
		map_center_agent.mV[VY] *= mScale/region_width;

		gGL.getTexUnit(0)->bind(mObjectImagep);
		F32 image_half_width = 0.5f*mObjectMapPixels;
		F32 image_half_height = 0.5f*mObjectMapPixels;

		gGL.begin(LLRender::QUADS);
			gGL.texCoord2f(0.f, 1.f);
			gGL.vertex2f(map_center_agent.mV[VX] - image_half_width, image_half_height + map_center_agent.mV[VY]);
			gGL.texCoord2f(0.f, 0.f);
			gGL.vertex2f(map_center_agent.mV[VX] - image_half_width, map_center_agent.mV[VY] - image_half_height);
			gGL.texCoord2f(1.f, 0.f);
			gGL.vertex2f(image_half_width + map_center_agent.mV[VX], map_center_agent.mV[VY] - image_half_height);
			gGL.texCoord2f(1.f, 1.f);
			gGL.vertex2f(image_half_width + map_center_agent.mV[VX], image_half_height + map_center_agent.mV[VY]);
		gGL.end();

		gGL.popMatrix();

		// Mouse pointer in local coordinates
		S32 local_mouse_x;
		S32 local_mouse_y;
		//localMouse(&local_mouse_x, &local_mouse_y);
		LLUI::getMousePositionLocal(this, &local_mouse_x, &local_mouse_y);
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
			pos_map = globalPosToView(positions[i]);
			LLUUID uuid = avatar_ids[i];

			bool show_as_friend = (LLAvatarTracker::instance().getBuddyInfo(uuid) != NULL);

			LLColor4 color = show_as_friend ? map_avatar_friend_color : map_avatar_color;

			unknown_relative_z = positions[i].mdV[VZ] == COARSEUPDATE_MAX_Z &&
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
						S32 x = llround( pos_map.mV[VX] );
						S32 y = llround( pos_map.mV[VY] );
						LLWorldMapView::drawTrackingCircle( getRect(), x, y, color, 1, 10);
					} else
					{
						LLWorldMapView::drawTrackingDot(pos_map.mV[VX],pos_map.mV[VY],color,0.f);
					}
				}
			}

			F32	dist_to_cursor_squared = dist_vec_squared(LLVector2(pos_map.mV[VX], pos_map.mV[VY]),
										  LLVector2(local_mouse_x,local_mouse_y));
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
		S32 dot_width = llround(mDotRadius * 2.f);
		LLUIImagePtr you = LLWorldMapView::sAvatarYouLargeImage;
		if (you)
		{
			you->draw(llround(pos_map.mV[VX] - mDotRadius),
					  llround(pos_map.mV[VY] - mDotRadius),
					  dot_width,
					  dot_width);

			F32	dist_to_cursor_squared = dist_vec_squared(LLVector2(pos_map.mV[VX], pos_map.mV[VY]),
										  LLVector2(local_mouse_x,local_mouse_y));
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

		F32 half_width_meters = far_clip_meters * tan( horiz_fov / 2 );
		F32 half_width_pixels = half_width_meters * meters_to_pixels;
		
		F32 ctr_x = (F32)center_sw_left;
		F32 ctr_y = (F32)center_sw_bottom;


		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		if( rotate_map )
		{
			gGL.color4fv((map_frustum_color()).mV);

			gGL.begin( LLRender::TRIANGLES  );
				gGL.vertex2f( ctr_x, ctr_y );
				gGL.vertex2f( ctr_x - half_width_pixels, ctr_y + far_clip_pixels );
				gGL.vertex2f( ctr_x + half_width_pixels, ctr_y + far_clip_pixels );
			gGL.end();
		}
		else
		{
			gGL.color4fv((map_frustum_rotating_color()).mV);
			
			// If we don't rotate the map, we have to rotate the frustum.
			gGL.pushMatrix();
				gGL.translatef( ctr_x, ctr_y, 0 );
				gGL.rotatef( atan2( LLViewerCamera::getInstance()->getAtAxis().mV[VX], LLViewerCamera::getInstance()->getAtAxis().mV[VY] ) * RAD_TO_DEG, 0.f, 0.f, -1.f);
				gGL.begin( LLRender::TRIANGLES  );
					gGL.vertex2f( 0, 0 );
					gGL.vertex2f( -half_width_pixels, far_clip_pixels );
					gGL.vertex2f(  half_width_pixels, far_clip_pixels );
				gGL.end();
			gGL.popMatrix();
		}
	}
	
	gGL.popMatrix();
	gGL.popUIMatrix();

	LLUICtrl::draw();
}

void LLNetMap::reshape(S32 width, S32 height, BOOL called_from_parent)
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
							BOOL draw_arrow )
{
	LLVector3 pos_local = globalPosToView(pos_global);
	if( (pos_local.mV[VX] < 0) ||
		(pos_local.mV[VY] < 0) ||
		(pos_local.mV[VX] >= getRect().getWidth()) ||
		(pos_local.mV[VY] >= getRect().getHeight()) )
	{
		if (draw_arrow)
		{
			S32 x = llround( pos_local.mV[VX] );
			S32 y = llround( pos_local.mV[VY] );
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

LLVector3d LLNetMap::viewPosToGlobal( S32 x, S32 y )
{
	x -= llround(getRect().getWidth() / 2 + mCurPan.mV[VX]);
	y -= llround(getRect().getHeight() / 2 + mCurPan.mV[VY]);

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

BOOL LLNetMap::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	// note that clicks are reversed from what you'd think: i.e. > 0  means zoom out, < 0 means zoom in
	F32 new_scale = mScale * pow(MAP_SCALE_ZOOM_FACTOR, -clicks);
	F32 old_scale = mScale;

	setScale(new_scale);

	static LLUICachedControl<bool> auto_center("MiniMapAutoCenter", true);
	if (!auto_center)
	{
		// Adjust pan to center the zoom on the mouse pointer
		LLVector2 zoom_offset;
		zoom_offset.mV[VX] = x - getRect().getWidth() / 2;
		zoom_offset.mV[VY] = y - getRect().getHeight() / 2;
		mCurPan -= zoom_offset * mScale / old_scale - zoom_offset;
	}

	return TRUE;
}

BOOL LLNetMap::handleToolTip( S32 x, S32 y, MASK mask )
{
	if (gDisconnected)
	{
		return FALSE;
	}

	// If the cursor is near an avatar on the minimap, a mini-inspector will be
	// shown for the avatar, instead of the normal map tooltip.
	if (handleToolTipAgent(mClosestAgentToCursor))
	{
		return TRUE;
	}

	LLRect sticky_rect;
	std::string region_name;
	LLViewerRegion*	region = LLWorld::getInstance()->getRegionFromPosGlobal( viewPosToGlobal( x, y ) );
	if(region)
	{
		// set sticky_rect
		S32 SLOP = 4;
		localPointToScreen(x - SLOP, y - SLOP, &(sticky_rect.mLeft), &(sticky_rect.mBottom));
		sticky_rect.mRight = sticky_rect.mLeft + 2 * SLOP;
		sticky_rect.mTop = sticky_rect.mBottom + 2 * SLOP;

		region_name = region->getName();
		if (!region_name.empty())
		{
			region_name += "\n";
		}
	}

	LLStringUtil::format_map_t args;
	args["[REGION]"] = region_name;
	std::string msg = mToolTipMsg;
	LLStringUtil::format(msg, args);
	LLToolTipMgr::instance().show(LLToolTip::Params()
		.message(msg)
		.sticky_rect(sticky_rect));
		
	return TRUE;
}

BOOL LLNetMap::handleToolTipAgent(const LLUUID& avatar_id)
{
	LLAvatarName av_name;
	if (avatar_id.isNull() || !LLAvatarNameCache::get(avatar_id, &av_name))
	{
		return FALSE;
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
	return TRUE;
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

	S32 diameter_pixels = llround(2 * radius_meters * mObjectMapTPM);
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

	S32 x_offset = llround(pos_local.mV[VX] * mObjectMapTPM + image_width / 2);
	S32 y_offset = llround(pos_local.mV[VY] * mObjectMapTPM + image_height / 2);

	if ((x_offset < 0) || (x_offset >= image_width))
	{
		return;
	}
	if ((y_offset < 0) || (y_offset >= image_height))
	{
		return;
	}

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
			((U32*)datap)[offset] = color.mAll;
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
			((U32*)datap)[offset] = color.mAll;
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
				((U32*)datap)[offset] = color.mAll;
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
	S32 square_size = llround( sqrt(width*width + height*height) );

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
		mObjectImagep = LLViewerTextureManager::getLocalTexture( mObjectRawImagep.get(), FALSE);
	}
	setScale(mScale);
	mUpdateNow = true;
}

BOOL LLNetMap::handleMouseDown( S32 x, S32 y, MASK mask )
{
	if (!(mask & MASK_SHIFT)) return FALSE;

	// Start panning
	gFocusMgr.setMouseCapture(this);

	mStartPan = mCurPan;
	mMouseDown.mX = x;
	mMouseDown.mY = y;
	return TRUE;
}

BOOL LLNetMap::handleMouseUp( S32 x, S32 y, MASK mask )
{
	if(abs(mMouseDown.mX-x)<3 && abs(mMouseDown.mY-y)<3)
		handleClick(x,y,mask);

	if (hasMouseCapture())
	{
		if (mPanning)
		{
			// restore mouse cursor
			S32 local_x, local_y;
			local_x = mMouseDown.mX + llfloor(mCurPan.mV[VX] - mStartPan.mV[VX]);
			local_y = mMouseDown.mY + llfloor(mCurPan.mV[VY] - mStartPan.mV[VY]);
			LLRect clip_rect = getRect();
			clip_rect.stretch(-8);
			clip_rect.clipPointToRect(mMouseDown.mX, mMouseDown.mY, local_x, local_y);
			LLUI::setMousePositionLocal(this, local_x, local_y);

			// finish the pan
			mPanning = false;

			mMouseDown.set(0, 0);

			// auto centre
			mTargetPan.setZero();
		}
		gViewerWindow->showCursor();
		gFocusMgr.setMouseCapture(NULL);
		return TRUE;
	}
	return FALSE;
}

BOOL LLNetMap::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if (mPopupMenu)
	{
		mPopupMenu->buildDrawLabels();
		mPopupMenu->updateParent(LLMenuGL::sMenuContainer);
		mPopupMenu->setItemEnabled("Stop Tracking", LLTracker::isTracking(0));
		LLMenuGL::showPopup(this, mPopupMenu, x, y);
	}
	return TRUE;
}

BOOL LLNetMap::handleClick(S32 x, S32 y, MASK mask)
{
	// TODO: allow clicking an avatar on minimap to select avatar in the nearby avatar list
	// if(mClosestAgentToCursor.notNull())
	//     mNearbyList->selectUser(mClosestAgentToCursor);
	// Needs a registered observer i guess to accomplish this without using
	// globals to tell the mNearbyList in llpeoplepanel to select the user
	return TRUE;
}

BOOL LLNetMap::handleDoubleClick(S32 x, S32 y, MASK mask)
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
	return TRUE;
}

// static
bool LLNetMap::outsideSlop( S32 x, S32 y, S32 start_x, S32 start_y, S32 slop )
{
	S32 dx = x - start_x;
	S32 dy = y - start_y;

	return (dx <= -slop || slop <= dx || dy <= -slop || slop <= dy);
}

BOOL LLNetMap::handleHover( S32 x, S32 y, MASK mask )
{
	if (hasMouseCapture())
	{
		if (mPanning || outsideSlop(x, y, mMouseDown.mX, mMouseDown.mY, MOUSE_DRAG_SLOP))
		{
			if (!mPanning)
			{
				// just started panning, so hide cursor
				mPanning = true;
				gViewerWindow->hideCursor();
			}

			LLVector2 delta(static_cast<F32>(gViewerWindow->getCurrentMouseDX()),
							static_cast<F32>(gViewerWindow->getCurrentMouseDY()));

			// Set pan to value at start of drag + offset
			mCurPan += delta;
			mTargetPan = mCurPan;

			gViewerWindow->moveCursorToCenter();
		}

		// Doesn't really matter, cursor should be hidden
		gViewerWindow->setCursor( UI_CURSOR_TOOLPAN );
	}
	else
	{
		if (mask & MASK_SHIFT)
		{
			// If shift is held, change the cursor to hint that the map can be dragged
			gViewerWindow->setCursor( UI_CURSOR_TOOLPAN );
		}
		else
		{
			gViewerWindow->setCursor( UI_CURSOR_CROSS );
		}
	}

	return TRUE;
}

void LLNetMap::handleZoom(const LLSD& userdata)
{
	std::string level = userdata.asString();
	
	F32 scale = 0.0f;
	if (level == std::string("default"))
	{
		LLControlVariable *pvar = gSavedSettings.getControl("MiniMapScale");
		if(pvar)
		{
			pvar->resetToDefault();
			scale = gSavedSettings.getF32("MiniMapScale");
		}
	}
	else if (level == std::string("close"))
		scale = LLNetMap::MAP_SCALE_MAX;
	else if (level == std::string("medium"))
		scale = LLNetMap::MAP_SCALE_MID;
	else if (level == std::string("far"))
		scale = LLNetMap::MAP_SCALE_MIN;
	if (scale != 0.0f)
	{
		setScale(scale);
	}
}

void LLNetMap::handleStopTracking (const LLSD& userdata)
{
	if (mPopupMenu)
	{
		mPopupMenu->setItemEnabled ("Stop Tracking", false);
		LLTracker::stopTracking ((void*)LLTracker::isTracking(NULL));
	}
}
