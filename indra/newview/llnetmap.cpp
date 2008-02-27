/** 
 * @file llnetmap.cpp
 * @author James Cook
 * @brief Display of surrounding regions, objects, and agents. View contained by LLFloaterMap.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "indra_constants.h"
#include "llui.h"
#include "linked_lists.h"
#include "llmath.h"		// clampf()
#include "llfocusmgr.h"
#include "llglimmediate.h"

#include "llagent.h"
#include "llcallingcard.h"
#include "llcolorscheme.h"
#include "llviewercontrol.h"
#include "llfloaterworldmap.h"
#include "llfloatermap.h"
#include "llframetimer.h"
#include "lltracker.h"
#include "llmenugl.h"
#include "llstatgraph.h"
#include "llsurface.h"
#include "lltextbox.h"
#include "lluuid.h"
#include "llviewercamera.h"
#include "llviewerimage.h"
#include "llviewerimagelist.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewermenu.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llworld.h"
#include "llworldmapview.h"		// shared draw code
#include "llappviewer.h"				// Only for constants!

#include "llglheaders.h"

const F32 MAP_SCALE_MIN = 64;
const F32 MAP_SCALE_MID = 172;
const F32 MAP_SCALE_MAX = 512;
const F32 MAP_SCALE_INCREMENT = 16;

const S32 TRACKING_RADIUS = 3;

//static
BOOL LLNetMap::sRotateMap = FALSE;
LLNetMap* LLNetMap::sInstance = NULL;

LLNetMap::LLNetMap(
	const std::string& name,
	const LLRect& rect,
	const LLColor4& bg_color )
	:
	LLUICtrl(name, rect, FALSE, NULL, NULL), mBackgroundColor( bg_color ),
	mObjectMapTPM(1.f),
	mObjectMapPixels(255.f),
	mTargetPanX( 0.f ),
	mTargetPanY( 0.f ),
	mCurPanX( 0.f ),
	mCurPanY( 0.f ),
	mUpdateNow( FALSE )
{
	mPixelsPerMeter = gMiniMapScale / REGION_WIDTH_METERS;

	LLNetMap::sRotateMap = gSavedSettings.getBOOL( "MiniMapRotate" );
	
	// Surface texture is dynamically generated/updated.
// 	createObjectImage();

	mObjectImageCenterGlobal = gAgent.getCameraPositionGlobal();
	
	const S32 DIR_WIDTH = 10;
	const S32 DIR_HEIGHT = 10;
	LLRect major_dir_rect(  0, DIR_HEIGHT, DIR_WIDTH, 0 );

	mTextBoxNorth = new LLTextBox( "N", major_dir_rect );
	mTextBoxNorth->setFontStyle(LLFontGL::DROP_SHADOW_SOFT);
	addChild( mTextBoxNorth );

	LLColor4 minor_color( 1.f, 1.f, 1.f, .7f );
	
	mTextBoxEast =	new LLTextBox( "E", major_dir_rect );
	mTextBoxEast->setColor( minor_color );
	addChild( mTextBoxEast );
	
	major_dir_rect.mRight += 1 ;
	mTextBoxWest =	new LLTextBox( "W", major_dir_rect );
	mTextBoxWest->setColor( minor_color );
	addChild( mTextBoxWest );
	major_dir_rect.mRight -= 1 ;

	mTextBoxSouth = new LLTextBox( "S", major_dir_rect );
	mTextBoxSouth->setColor( minor_color );
	addChild( mTextBoxSouth );

	LLRect minor_dir_rect(  0, DIR_HEIGHT, DIR_WIDTH * 2, 0 );

	mTextBoxSouthEast =	new LLTextBox( "SE", minor_dir_rect );
	mTextBoxSouthEast->setColor( minor_color );
	addChild( mTextBoxSouthEast );
	
	mTextBoxNorthEast = new LLTextBox( "NE", minor_dir_rect );
	mTextBoxNorthEast->setColor( minor_color );
	addChild( mTextBoxNorthEast );
	
	mTextBoxSouthWest =	new LLTextBox( "SW", minor_dir_rect );
	mTextBoxSouthWest->setColor( minor_color );
	addChild( mTextBoxSouthWest );

	mTextBoxNorthWest = new LLTextBox( "NW", minor_dir_rect );
	mTextBoxNorthWest->setColor( minor_color );
	addChild( mTextBoxNorthWest );

	// Right-click menu
	LLMenuGL* menu;
	menu = new LLMenuGL("popup");
	menu->setCanTearOff(FALSE);
	menu->append(new LLMenuItemCallGL("Zoom Close", handleZoomLevel,
										NULL, (void*)2) );
	menu->append(new LLMenuItemCallGL("Zoom Medium", handleZoomLevel,
										NULL, (void*)1) );
	menu->append(new LLMenuItemCallGL("Zoom Far", handleZoomLevel,
										NULL, (void*)0) );
	menu->appendSeparator();
	menu->append(new LLMenuItemCallGL("Stop Tracking", &LLTracker::stopTracking,
										&LLTracker::isTracking, NULL) );
	menu->setVisible(FALSE);
	addChild(menu);
	mPopupMenuHandle = menu->getHandle();

	sInstance = this;

	gSavedSettings.getControl("MiniMapRotate")->addListener(&mNetMapListener);
}

LLNetMap::~LLNetMap()
{
	sInstance = NULL;
}

EWidgetType LLNetMap::getWidgetType() const
{
	return WIDGET_TYPE_NET_MAP;
}

LLString LLNetMap::getWidgetTag() const
{
	return LL_NET_MAP_TAG;
}


void LLNetMap::setScale( F32 scale )
{
	gMiniMapScale = scale;
	if (gMiniMapScale == 0.f)
	{
		gMiniMapScale = 0.1f;
	}

	if (mObjectImagep.notNull())
	{
		F32 half_width = (F32)(getRect().getWidth() / 2);
		F32 half_height = (F32)(getRect().getHeight() / 2);
		F32 radius = sqrt( half_width * half_width + half_height * half_height );

		F32 region_widths = (2.f*radius)/gMiniMapScale;

		F32 meters;
		if (!gWorldPointer)
		{
			// Hack!  Sometimes world hasn't been initialized at this point.
			meters = 256.f*region_widths;
		}
		else
		{
			meters = region_widths * gWorldPointer->getRegionWidthInMeters();
		}

		F32 num_pixels = (F32)mObjectImagep->getWidth();
		mObjectMapTPM = num_pixels/meters;
		mObjectMapPixels = 2.f*radius;
	}

	mPixelsPerMeter = gMiniMapScale / REGION_WIDTH_METERS;

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

	if (!getVisible() || !gWorldPointer)
	{
		return;
	}
	if (mObjectImagep.isNull())
	{
		createObjectImage();
	}
	
	mCurPanX = lerp(mCurPanX, mTargetPanX, LLCriticalDamp::getInterpolant(0.1f));
	mCurPanY = lerp(mCurPanY, mTargetPanY, LLCriticalDamp::getInterpolant(0.1f));

	// Prepare a scissor region
	F32 rotation = 0;

	{
		LLGLEnable scissor(GL_SCISSOR_TEST);
		
		{
			LLGLSNoTexture no_texture;
			LLLocalClipRect clip(getLocalRect());

			glMatrixMode(GL_MODELVIEW);

			// Draw background rectangle
			gGL.color4fv( mBackgroundColor.mV );
			gl_rect_2d(0, getRect().getHeight(), getRect().getWidth(), 0);
		}

		// region 0,0 is in the middle
		S32 center_sw_left = getRect().getWidth() / 2 + llfloor(mCurPanX);
		S32 center_sw_bottom = getRect().getHeight() / 2 + llfloor(mCurPanY);

		gGL.pushMatrix();

		gGL.translatef( (F32) center_sw_left, (F32) center_sw_bottom, 0.f);

		if( LLNetMap::sRotateMap )
		{
			// rotate subsequent draws to agent rotation
			rotation = atan2( gCamera->getAtAxis().mV[VX], gCamera->getAtAxis().mV[VY] );
			glRotatef( rotation * RAD_TO_DEG, 0.f, 0.f, 1.f);
		}

		// figure out where agent is
		S32 region_width = llround(gWorldPointer->getRegionWidthInMeters());

		for (LLWorld::region_list_t::iterator iter = gWorldp->mActiveRegionList.begin();
			 iter != gWorldp->mActiveRegionList.end(); ++iter)
		{
			LLViewerRegion* regionp = *iter;
			// Find x and y position relative to camera's center.
			LLVector3 origin_agent = regionp->getOriginAgent();
			LLVector3 rel_region_pos = origin_agent - gAgent.getCameraPositionAgent();
			F32 relative_x = (rel_region_pos.mV[0] / region_width) * gMiniMapScale;
			F32 relative_y = (rel_region_pos.mV[1] / region_width) * gMiniMapScale;

			// background region rectangle
			F32 bottom =	relative_y;
			F32 left =		relative_x;
			F32 top =		bottom + gMiniMapScale ;
			F32 right =		left + gMiniMapScale ;

			if (regionp == gAgent.getRegion())
			{
				gGL.color4f(1.f, 1.f, 1.f, 1.f);
			}
			else
			{
				gGL.color4f(0.8f, 0.8f, 0.8f, 1.f);
			}

			if (!regionp->mAlive)
			{
				gGL.color4f(1.f, 0.5f, 0.5f, 1.f);
			}


			// Draw using texture.
			LLViewerImage::bindTexture(regionp->getLand().getSTexture());
			gGL.begin(GL_QUADS);
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
			glAlphaFunc(GL_GREATER, ABOVE_WATERLINE_ALPHA / 255.f );
			{
				if (regionp->getLand().getWaterTexture())
				{
					LLViewerImage::bindTexture(regionp->getLand().getWaterTexture());
					gGL.begin(GL_QUADS);
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
			glAlphaFunc(GL_GREATER,0.01f);
		}
		

		LLVector3d old_center = mObjectImageCenterGlobal;
		LLVector3d new_center = gAgent.getCameraPositionGlobal();

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
		map_center_agent -= gAgent.getCameraPositionAgent();
		map_center_agent.mV[VX] *= gMiniMapScale/region_width;
		map_center_agent.mV[VY] *= gMiniMapScale/region_width;

		LLViewerImage::bindTexture(mObjectImagep);
		F32 image_half_width = 0.5f*mObjectMapPixels;
		F32 image_half_height = 0.5f*mObjectMapPixels;

		gGL.begin(GL_QUADS);
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

		// Draw avatars
		for (LLWorld::region_list_t::iterator iter = gWorldp->mActiveRegionList.begin();
			 iter != gWorldp->mActiveRegionList.end(); ++iter)
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
					show_as_friend = is_agent_friend(regionp->mMapAvatarIDs.get(i));
				}
				LLWorldMapView::drawAvatar(
					pos_map.mV[VX], pos_map.mV[VY], 
					show_as_friend ? gFriendMapColor : gAvatarMapColor, 
					pos_map.mV[VZ]);
			}
		}

		// Draw dot for autopilot target
		if (gAgent.getAutoPilot())
		{
			drawTracking( gAgent.getAutoPilotTargetGlobal(), gTrackColor );
		}
		else
		{
			LLTracker::ETrackingStatus tracking_status = LLTracker::getTrackingStatus();
			if (  LLTracker::TRACKING_AVATAR == tracking_status )
			{
				drawTracking( LLAvatarTracker::instance().getGlobalPos(), gTrackColor );
			} 
			else if ( LLTracker::TRACKING_LANDMARK == tracking_status 
					|| LLTracker::TRACKING_LOCATION == tracking_status )
			{
				drawTracking( LLTracker::getTrackedPositionGlobal(), gTrackColor );
			}
		}

		// Draw dot for self avatar position
		//drawTracking( gAgent.getPosGlobalFromAgent(gAgent.getFrameAgent().getCenter()), gSelfMapColor );
		pos_global = gAgent.getPositionGlobal();
		pos_map = globalPosToView(pos_global);
		gl_draw_image(llround(pos_map.mV[VX]) - 4,
					llround(pos_map.mV[VY]) - 4,
					LLWorldMapView::sAvatarYouSmallImage,
					LLColor4::white);

		// Draw frustum
		F32 meters_to_pixels = gMiniMapScale/ gWorldPointer->getRegionWidthInMeters();

		F32 horiz_fov = gCamera->getView() * gCamera->getAspect();
		F32 far_clip_meters = gCamera->getFar();
		F32 far_clip_pixels = far_clip_meters * meters_to_pixels;

		F32 half_width_meters = far_clip_meters * tan( horiz_fov / 2 );
		F32 half_width_pixels = half_width_meters * meters_to_pixels;
		
		F32 ctr_x = (F32)center_sw_left;
		F32 ctr_y = (F32)center_sw_bottom;


		LLGLSNoTexture no_texture;

		if( LLNetMap::sRotateMap )
		{
			gGL.color4fv(gFrustumMapColor.mV);

			gGL.begin( GL_TRIANGLES  );
				gGL.vertex2f( ctr_x, ctr_y );
				gGL.vertex2f( ctr_x - half_width_pixels, ctr_y + far_clip_pixels );
				gGL.vertex2f( ctr_x + half_width_pixels, ctr_y + far_clip_pixels );
			gGL.end();
		}
		else
		{
			gGL.color4fv(gRotatingFrustumMapColor.mV);
			
			// If we don't rotate the map, we have to rotate the frustum.
			gGL.pushMatrix();
				gGL.translatef( ctr_x, ctr_y, 0 );
				glRotatef( atan2( gCamera->getAtAxis().mV[VX], gCamera->getAtAxis().mV[VY] ) * RAD_TO_DEG, 0.f, 0.f, -1.f);
				gGL.begin( GL_TRIANGLES  );
					gGL.vertex2f( 0, 0 );
					gGL.vertex2f( -half_width_pixels, far_clip_pixels );
					gGL.vertex2f(  half_width_pixels, far_clip_pixels );
				gGL.end();
			gGL.popMatrix();
		}
	}
	
	// Rotation of 0 means that North is up
	setDirectionPos( mTextBoxEast,  rotation );
	setDirectionPos( mTextBoxNorth, rotation + F_PI_BY_TWO );
	setDirectionPos( mTextBoxWest,  rotation + F_PI );
	setDirectionPos( mTextBoxSouth, rotation + F_PI + F_PI_BY_TWO );

	setDirectionPos( mTextBoxNorthEast, rotation +						F_PI_BY_TWO / 2);
	setDirectionPos( mTextBoxNorthWest, rotation + F_PI_BY_TWO +		F_PI_BY_TWO / 2);
	setDirectionPos( mTextBoxSouthWest, rotation + F_PI +				F_PI_BY_TWO / 2);
	setDirectionPos( mTextBoxSouthEast, rotation + F_PI + F_PI_BY_TWO + F_PI_BY_TWO / 2);

	LLUICtrl::draw();
}

LLVector3 LLNetMap::globalPosToView( const LLVector3d& global_pos )
{
	LLVector3d relative_pos_global = global_pos - gAgent.getCameraPositionGlobal();
	LLVector3 pos_local;
	pos_local.setVec(relative_pos_global);  // convert to floats from doubles

	pos_local.mV[VX] *= mPixelsPerMeter;
	pos_local.mV[VY] *= mPixelsPerMeter;
	// leave Z component in meters

	if( LLNetMap::sRotateMap )
	{
		F32 radians = atan2( gCamera->getAtAxis().mV[VX], gCamera->getAtAxis().mV[VY] );
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

	F32 radians = - atan2( gCamera->getAtAxis().mV[VX], gCamera->getAtAxis().mV[VY] );

	if( LLNetMap::sRotateMap )
	{
		LLQuaternion rot(radians, LLVector3(0.f, 0.f, 1.f));
		pos_local.rotVec( rot );
	}

	pos_local *= ( gWorldPointer->getRegionWidthInMeters() / gMiniMapScale );
	
	LLVector3d pos_global;
	pos_global.setVec( pos_local );
	pos_global += gAgent.getCameraPositionGlobal();

	return pos_global;
}

BOOL LLNetMap::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	// note that clicks are reversed from what you'd think
	setScale(llclamp(gMiniMapScale - clicks*MAP_SCALE_INCREMENT, MAP_SCALE_MIN, MAP_SCALE_MAX));
	return TRUE;
}

BOOL LLNetMap::handleToolTip( S32 x, S32 y, LLString& msg, LLRect* sticky_rect_screen )
{
	BOOL handled = FALSE;
	if (gDisconnected)
	{
		return FALSE;
	}
	LLViewerRegion*	region = gWorldPointer->getRegionFromPosGlobal( viewPosToGlobal( x, y ) );
	if( region )
	{
		msg.assign( region->getName() );

#ifndef LL_RELEASE_FOR_DOWNLOAD
		char buffer[MAX_STRING];		/*Flawfinder: ignore*/
		msg.append("\n");
		region->getHost().getHostName(buffer, MAX_STRING);
		msg.append(buffer);
		msg.append("\n");
		region->getHost().getString(buffer, MAX_STRING);
		msg.append(buffer);
#endif
		// *TODO: put this under the control of XUI so it can be
		// translated.
		msg.append("\n(Double-click to open Map)");

		S32 SLOP = 4;
		localPointToScreen( 
			x - SLOP, y - SLOP, 
			&(sticky_rect_screen->mLeft), &(sticky_rect_screen->mBottom) );
		sticky_rect_screen->mRight = sticky_rect_screen->mLeft + 2 * SLOP;
		sticky_rect_screen->mTop = sticky_rect_screen->mBottom + 2 * SLOP;
	}
	handled = TRUE;
	return handled;
}


void LLNetMap::setDirectionPos( LLTextBox* text_box, F32 rotation )
{
	// Rotation is in radians.
	// Rotation of 0 means x = 1, y = 0 on the unit circle.


	F32 map_half_height = (F32)(getRect().getHeight() / 2);
	F32 map_half_width = (F32)(getRect().getWidth() / 2);
	F32 text_half_height = (F32)(text_box->getRect().getHeight() / 2);
	F32 text_half_width = (F32)(text_box->getRect().getWidth() / 2);
	F32 radius = llmin( map_half_height - text_half_height, map_half_width - text_half_width );

	// Inset by a little to account for position display.
	radius -= 8.f;

	text_box->setOrigin( 
		llround(map_half_width - text_half_width + radius * cos( rotation )),
		llround(map_half_height - text_half_height + radius * sin( rotation )) );
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
	F32 half_width = (F32)(getRect().getWidth() / 2);
	F32 half_height = (F32)(getRect().getHeight() / 2);
	F32 radius = sqrt( half_width * half_width + half_height * half_height );
	S32 square_size = S32( 2 * radius );

	// Find the least power of two >= the minimum size.
	const S32 MIN_SIZE = 32;
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
		mObjectImagep = new LLImageGL( mObjectRawImagep, FALSE);
		setScale(gMiniMapScale);
	}
	mUpdateNow = TRUE;
}

BOOL LLNetMap::handleDoubleClick( S32 x, S32 y, MASK mask )
{
	LLFloaterWorldMap::show(NULL, FALSE);
	return TRUE;
}

BOOL LLNetMap::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
	if (menu)
	{
		menu->buildDrawLabels();
		menu->updateParent(LLMenuGL::sMenuContainer);
		LLMenuGL::showPopup(this, menu, x, y);
	}
	return TRUE;
}


// static
void LLNetMap::handleZoomLevel(void* which)
{
	intptr_t level = (intptr_t)which;

	switch(level)
	{
	case 0:
		LLNetMap::sInstance->setScale(MAP_SCALE_MIN);
		break;
	case 1:
		LLNetMap::sInstance->setScale(MAP_SCALE_MID);
		break;
	case 2:
		LLNetMap::sInstance->setScale(MAP_SCALE_MAX);
		break;
	default:
		break;
	}
}

bool LLRotateNetMapListener::handleEvent(LLPointer<LLEvent> event, const LLSD& user_data)
{
	LLNetMap::setRotateMap(event->getValue().asBoolean());
	return true;
}
