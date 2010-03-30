/** 
 * @file llnetmap.cpp
 * @author James Cook
 * @brief Display of surrounding regions, objects, and agents. 
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llnetmap.h"

// Library includes (should move below)
#include "indra_constants.h"
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
#include "lltracker.h"
#include "llsurface.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
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

LLNetMap::LLNetMap (const Params & p)
:	LLUICtrl (p),
	mBackgroundColor (p.bg_color()),
	mScale( MAP_SCALE_MID ),
	mPixelsPerMeter( MAP_SCALE_MID / REGION_WIDTH_METERS ),
	mObjectMapTPM(0.f),
	mObjectMapPixels(0.f),
	mTargetPanX(0.f),
	mTargetPanY(0.f),
	mCurPanX(0.f),
	mCurPanY(0.f),
	mUpdateNow(FALSE),
	mObjectImageCenterGlobal( gAgentCamera.getCameraPositionGlobal() ),
	mObjectRawImagep(),
	mObjectImagep(),
	mClosestAgentToCursor(),
	mClosestAgentAtLastRightClick(),
	mToolTipMsg()
{
	mDotRadius = llmax(DOT_SCALE * mPixelsPerMeter, MIN_DOT_RADIUS);
}

LLNetMap::~LLNetMap()
{
}

void LLNetMap::setScale( F32 scale )
{
	mScale = llclamp(scale, 0.1f, 16.f*1024.f); // [reasonably small , unreasonably large]
	
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

	mUpdateNow = TRUE;
}

void LLNetMap::translatePan( F32 delta_x, F32 delta_y )
{
	mTargetPanX += delta_x;
	mTargetPanY += delta_y;
}


///////////////////////////////////////////////////////////////////////////////////

void LLNetMap::draw()
{
 	static LLFrameTimer map_timer;
	static LLUIColor map_avatar_color = LLUIColorTable::instance().getColor("MapAvatarColor", LLColor4::white);
	static LLUIColor map_avatar_friend_color = LLUIColorTable::instance().getColor("MapAvatarFriendColor", LLColor4::white);
	static LLUIColor map_track_color = LLUIColorTable::instance().getColor("MapTrackColor", LLColor4::white);
	static LLUIColor map_track_disabled_color = LLUIColorTable::instance().getColor("MapTrackDisabledColor", LLColor4::white);
	static LLUIColor map_frustum_color = LLUIColorTable::instance().getColor("MapFrustumColor", LLColor4::white);
	static LLUIColor map_frustum_rotating_color = LLUIColorTable::instance().getColor("MapFrustumRotatingColor", LLColor4::white);
	
	if (mObjectImagep.isNull())
	{
		createObjectImage();
	}
	
	mCurPanX = lerp(mCurPanX, mTargetPanX, LLCriticalDamp::getInterpolant(0.1f));
	mCurPanY = lerp(mCurPanY, mTargetPanY, LLCriticalDamp::getInterpolant(0.1f));

	// Prepare a scissor region
	F32 rotation = 0;

	gGL.pushMatrix();
	gGL.pushUIMatrix();
	
	LLVector3 offset = gGL.getUITranslation();
	LLVector3 scale = gGL.getUIScale();

	glLoadIdentity();
	gGL.loadUIIdentity();

	glScalef(scale.mV[0], scale.mV[1], scale.mV[2]);
	gGL.translatef(offset.mV[0], offset.mV[1], offset.mV[2]);
	
	{
		LLLocalClipRect clip(getLocalRect());
		{
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

			glMatrixMode(GL_MODELVIEW);

			// Draw background rectangle
			LLColor4 background_color = mBackgroundColor.get();
			gGL.color4fv( background_color.mV );
			gl_rect_2d(0, getRect().getHeight(), getRect().getWidth(), 0);
		}

		// region 0,0 is in the middle
		S32 center_sw_left = getRect().getWidth() / 2 + llfloor(mCurPanX);
		S32 center_sw_bottom = getRect().getHeight() / 2 + llfloor(mCurPanY);

		gGL.pushMatrix();

		gGL.translatef( (F32) center_sw_left, (F32) center_sw_bottom, 0.f);

		static LLUICachedControl<bool> rotate_map("MiniMapRotate", true);
		if( rotate_map )
		{
			// rotate subsequent draws to agent rotation
			rotation = atan2( LLViewerCamera::getInstance()->getAtAxis().mV[VX], LLViewerCamera::getInstance()->getAtAxis().mV[VY] );
			glRotatef( rotation * RAD_TO_DEG, 0.f, 0.f, 1.f);
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
		

		LLVector3d old_center = mObjectImageCenterGlobal;
		LLVector3d new_center = gAgentCamera.getCameraPositionGlobal();

		new_center.mdV[0] = (5.f/mObjectMapTPM)*floor(0.2f*mObjectMapTPM*new_center.mdV[0]);
		new_center.mdV[1] = (5.f/mObjectMapTPM)*floor(0.2f*mObjectMapTPM*new_center.mdV[1]);
		new_center.mdV[2] = 0.f;

		if (mUpdateNow || (map_timer.getElapsedTimeF32() > 0.5f))
		{
			mUpdateNow = FALSE;
			mObjectImageCenterGlobal = new_center;

			// Center moved enough.
			// Create the base texture.
			U8 *default_texture = mObjectRawImagep->getData();
			memset( default_texture, 0, mObjectImagep->getWidth() * mObjectImagep->getHeight() * mObjectImagep->getComponents() );

			// Draw buildings
			gObjectList.renderObjectsForMap(*this);

			mObjectImagep->setSubImage(mObjectRawImagep, 0, 0, mObjectImagep->getWidth(), mObjectImagep->getHeight());
			
			map_timer.reset();
		}

		LLVector3 map_center_agent = gAgent.getPosAgentFromGlobal(mObjectImageCenterGlobal);
		map_center_agent -= gAgentCamera.getCameraPositionAgent();
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

		LLVector3d pos_global;
		LLVector3 pos_map;

		// Mouse pointer in local coordinates
		S32 local_mouse_x;
		S32 local_mouse_y;
		//localMouse(&local_mouse_x, &local_mouse_y);
		LLUI::getMousePositionLocal(this, &local_mouse_x, &local_mouse_y);
		mClosestAgentToCursor.setNull();
		F32 closest_dist = F32_MAX;
		F32 min_pick_dist = mDotRadius * MIN_PICK_SCALE; 

		// Draw avatars
		for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
			 iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
		{
			LLViewerRegion* regionp = *iter;
			const LLVector3d& origin_global = regionp->getOriginGlobal();

			S32 count = regionp->mMapAvatars.count();
			S32 i;
			LLVector3 pos_local;
			U32 compact_local;
			U8 bits;
			// TODO: it'd be very cool to draw these in sorted order from lowest Z to highest.
			// just be careful to sort the avatar IDs along with the positions. -MG
			for (i = 0; i < count; i++)
			{
				compact_local = regionp->mMapAvatars.get(i);

				bits = compact_local & 0xFF;
				pos_local.mV[VZ] = F32(bits) * 4.f;
				compact_local >>= 8;

				bits = compact_local & 0xFF;
				pos_local.mV[VY] = (F32)bits;
				compact_local >>= 8;

				bits = compact_local & 0xFF;
				pos_local.mV[VX] = (F32)bits;

				pos_global.setVec( pos_local );
				pos_global += origin_global;

				pos_map = globalPosToView(pos_global);

				BOOL show_as_friend = FALSE;
				if( i < regionp->mMapAvatarIDs.count())
				{
					show_as_friend = (LLAvatarTracker::instance().getBuddyInfo(regionp->mMapAvatarIDs.get(i)) != NULL);
				}
				LLWorldMapView::drawAvatar(
					pos_map.mV[VX], pos_map.mV[VY], 
					show_as_friend ? map_avatar_friend_color : map_avatar_color, 
					pos_map.mV[VZ], mDotRadius);

				F32	dist_to_cursor = dist_vec(LLVector2(pos_map.mV[VX], pos_map.mV[VY]), LLVector2(local_mouse_x,local_mouse_y));
				if(dist_to_cursor < min_pick_dist && dist_to_cursor < closest_dist)
				{
					closest_dist = dist_to_cursor;
					mClosestAgentToCursor = regionp->mMapAvatarIDs.get(i);
				}
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
		pos_global = gAgent.getPositionGlobal();
		pos_map = globalPosToView(pos_global);
		LLUIImagePtr you = LLWorldMapView::sAvatarYouLargeImage;
		S32 dot_width = llround(mDotRadius * 2.f);
		you->draw(llround(pos_map.mV[VX] - mDotRadius),
				  llround(pos_map.mV[VY] - mDotRadius),
				  dot_width,
				  dot_width);

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
				glRotatef( atan2( LLViewerCamera::getInstance()->getAtAxis().mV[VX], LLViewerCamera::getInstance()->getAtAxis().mV[VY] ) * RAD_TO_DEG, 0.f, 0.f, -1.f);
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

LLVector3 LLNetMap::globalPosToView( const LLVector3d& global_pos )
{
	LLVector3d relative_pos_global = global_pos - gAgentCamera.getCameraPositionGlobal();
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

	pos_local.mV[VX] += getRect().getWidth() / 2 + mCurPanX;
	pos_local.mV[VY] += getRect().getHeight() / 2 + mCurPanY;

	return pos_local;
}

void LLNetMap::drawTracking(const LLVector3d& pos_global, const LLColor4& color, 
							BOOL draw_arrow )
{
	LLVector3 pos_local = globalPosToView( pos_global );
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
	x -= llround(getRect().getWidth() / 2 + mCurPanX);
	y -= llround(getRect().getHeight() / 2 + mCurPanY);

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
	F32 scale = mScale;
        
	scale *= pow(MAP_SCALE_ZOOM_FACTOR, -clicks);
	setScale(llclamp(scale, MAP_SCALE_MIN, MAP_SCALE_MAX));

	return TRUE;
}

BOOL LLNetMap::handleToolTip( S32 x, S32 y, MASK mask )
{
	if (gDisconnected)
	{
		return FALSE;
	}
	
	// mToolTipMsg = "[AGENT][REGION](Double-click to open Map)"
	
	LLStringUtil::format_map_t args;
	std::string fullname;
	if(mClosestAgentToCursor.notNull() && gCacheName->getFullName(mClosestAgentToCursor, fullname))
	{
		args["[AGENT]"] = fullname + "\n";
	}
	else
	{
		args["[AGENT]"] = "";
	}
	
	LLViewerRegion*	region = LLWorld::getInstance()->getRegionFromPosGlobal( viewPosToGlobal( x, y ) );
	if( region )
	{
		args["[REGION]"] = region->getName() + "\n";
	}
	else
	{
		args["[REGION]"] = "";
	}
	
	std::string msg = mToolTipMsg;
	LLStringUtil::format(msg, args);
	
	LLRect sticky_rect;
	// set sticky_rect
	if (region)
	{
		S32 SLOP = 4;
		localPointToScreen( 
			x - SLOP, y - SLOP, 
			&(sticky_rect.mLeft), &(sticky_rect.mBottom) );
		sticky_rect.mRight = sticky_rect.mLeft + 2 * SLOP;
		sticky_rect.mTop = sticky_rect.mBottom + 2 * SLOP;
	}

	LLToolTipMgr::instance().show(LLToolTip::Params()
		.message(msg)
		.sticky_rect(sticky_rect));
		
	return TRUE;
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
	mUpdateNow = TRUE;
}
