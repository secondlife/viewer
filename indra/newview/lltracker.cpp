/** 
 * @file lltracker.cpp
 * @brief Container for objects user is tracking.
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

#include "llviewerprecompiledheaders.h"

// library includes
#include "llcoord.h"
#include "lldarray.h"
#include "llfontgl.h"
#include "llgl.h"
#include "llrender.h"
#include "llinventory.h"
#include "llinventorydefines.h"
#include "llpointer.h"
#include "llstring.h"
#include "lluuid.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v4color.h"

// viewer includes
#include "llappviewer.h"
#include "lltracker.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llcallingcard.h"
#include "llfloaterworldmap.h"
#include "llhudtext.h"
#include "llhudview.h"
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "lllandmarklist.h"
#include "llprogressview.h"
#include "llsky.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewerinventory.h"
#include "llviewerwindow.h"
#include "llworld.h"
#include "llworldmapview.h"
#include "llviewercontrol.h"

const F32 DESTINATION_REACHED_RADIUS    = 3.0f;
const F32 DESTINATION_VISITED_RADIUS    = 6.0f;

// this last one is useful for when the landmark is
// very close to agent when tracking is turned on
const F32 DESTINATION_UNVISITED_RADIUS = 12.0f;

const S32 ARROW_OFF_RADIUS_SQRD = 100;

const S32 HUD_ARROW_SIZE = 32;

// static
LLTracker *LLTracker::sTrackerp = NULL;
BOOL LLTracker::sCheesyBeacon = FALSE;

LLTracker::LLTracker()
:	mTrackingStatus(TRACKING_NOTHING),
	mTrackingLocationType(LOCATION_NOTHING),
	mHUDArrowCenterX(0),
	mHUDArrowCenterY(0),
	mToolTip( "" ),
	mTrackedLandmarkName(""),
	mHasReachedLandmark(FALSE),
	mHasLandmarkPosition(FALSE),
	mLandmarkHasBeenVisited(FALSE),
	mTrackedLocationName( "" ),
	mIsTrackingLocation(FALSE),
	mHasReachedLocation(FALSE)
{ }


LLTracker::~LLTracker()
{ 
	purgeBeaconText();
}


// static
void LLTracker::stopTracking(void* userdata)
{
	BOOL clear_ui = ((BOOL)(intptr_t)userdata);
	instance()->stopTrackingAll(clear_ui);
}


// static virtual
void LLTracker::drawHUDArrow()
{
	if (!gSavedSettings.getBOOL("RenderTrackerBeacon")) return;

	if (gViewerWindow->getProgressView()->getVisible()) return;

	static LLUIColor map_track_color = LLUIColorTable::instance().getColor("MapTrackColor", LLColor4::white);
	
	/* tracking autopilot destination has been disabled 
	   -- 2004.01.09, Leviathan
	// Draw dot for autopilot target
	if (gAgent.getAutoPilot())
	{
		instance()->drawMarker( gAgent.getAutoPilotTargetGlobal(), map_track_color );
		return;
	}
	*/
	switch (getTrackingStatus())
	{ 
	case TRACKING_AVATAR:
		// Tracked avatar
		if(LLAvatarTracker::instance().haveTrackingInfo())
		{
			instance()->drawMarker( LLAvatarTracker::instance().getGlobalPos(), map_track_color );
		} 
		break;

	case TRACKING_LANDMARK:
		instance()->drawMarker( getTrackedPositionGlobal(), map_track_color );
		break;

	case TRACKING_LOCATION:
		// HACK -- try to keep the location just above the terrain
#if 0
		// UNHACKED by CRO - keep location where the location is
		instance()->mTrackedPositionGlobal.mdV[VZ] = 
				0.9f * instance()->mTrackedPositionGlobal.mdV[VZ]
				+ 0.1f * (LLWorld::getInstance()->resolveLandHeightGlobal(getTrackedPositionGlobal()) + 1.5f);
#endif
		instance()->mTrackedPositionGlobal.mdV[VZ] = llclamp((F32)instance()->mTrackedPositionGlobal.mdV[VZ], LLWorld::getInstance()->resolveLandHeightGlobal(getTrackedPositionGlobal()) + 1.5f, (F32)instance()->getTrackedPositionGlobal().mdV[VZ]);
		instance()->drawMarker( getTrackedPositionGlobal(), map_track_color );
		break;

	default:
		break;
	}
}


// static 
void LLTracker::render3D()
{
	if (!gFloaterWorldMap || !gSavedSettings.getBOOL("RenderTrackerBeacon"))
	{
		return;
	}
	
	static LLUIColor map_track_color = LLUIColorTable::instance().getColor("MapTrackColor", LLColor4::white);
	
	// Arbitary location beacon
	if( instance()->mIsTrackingLocation )
 	{
		if (!instance()->mBeaconText)
		{
			instance()->mBeaconText = (LLHUDText *)LLHUDObject::addHUDObject(LLHUDObject::LL_HUD_TEXT);
			instance()->mBeaconText->setDoFade(FALSE);
		}

		LLVector3d pos_global = instance()->mTrackedPositionGlobal;
		// (z-attenuation < 1) means compute "shorter" distance in z-axis,
		// so cancel tracking even if avatar is a little above or below.
		F32 dist = gFloaterWorldMap->getDistanceToDestination(pos_global, 0.5f);
		if (dist < DESTINATION_REACHED_RADIUS)
		{
			instance()->stopTrackingLocation();
		}
		else
		{
			renderBeacon( instance()->mTrackedPositionGlobal, map_track_color, 
					  	instance()->mBeaconText, instance()->mTrackedLocationName );
		}
	}

	// Landmark beacon
	else if( !instance()->mTrackedLandmarkAssetID.isNull() )
	{
		if (!instance()->mBeaconText)
		{
			instance()->mBeaconText = (LLHUDText *)LLHUDObject::addHUDObject(LLHUDObject::LL_HUD_TEXT);
			instance()->mBeaconText->setDoFade(FALSE);
		}

		if (instance()->mHasLandmarkPosition)
		{
			F32 dist = gFloaterWorldMap->getDistanceToDestination(instance()->mTrackedPositionGlobal, 1.0f);

			if (   !instance()->mLandmarkHasBeenVisited
				&& dist < DESTINATION_VISITED_RADIUS )
			{
				// its close enough ==> flag as visited
				instance()->setLandmarkVisited();
			}

			if (   !instance()->mHasReachedLandmark 
				&& dist < DESTINATION_REACHED_RADIUS )
			{
				// its VERY CLOSE ==> automatically stop tracking
				instance()->stopTrackingLandmark();
			}
			else
			{
				if (    instance()->mHasReachedLandmark 
					 && dist > DESTINATION_UNVISITED_RADIUS )
				{
					// this is so that landmark beacons don't immediately 
					// disappear when they're created only a few meters 
					// away, yet disappear when the agent wanders away 
					// and back again
					instance()->mHasReachedLandmark = FALSE;
				}
				renderBeacon( instance()->mTrackedPositionGlobal, map_track_color, 
							  instance()->mBeaconText, instance()->mTrackedLandmarkName );
			}
		}
		else
		{
			// probably just finished downloading the asset
			instance()->cacheLandmarkPosition();
		}
	}
	else
	{
		// Avatar beacon
		LLAvatarTracker& av_tracker = LLAvatarTracker::instance();
		if(av_tracker.haveTrackingInfo())
		{
			if (!instance()->mBeaconText)
			{
				instance()->mBeaconText = (LLHUDText *)LLHUDObject::addHUDObject(LLHUDObject::LL_HUD_TEXT);
				instance()->mBeaconText->setDoFade(FALSE);
			}
			
			F32 dist = gFloaterWorldMap->getDistanceToDestination(instance()->getTrackedPositionGlobal(), 0.0f);
			if (dist < DESTINATION_REACHED_RADIUS)
			{
				instance()->stopTrackingAvatar();
			}
			else
			{
				renderBeacon( av_tracker.getGlobalPos(), map_track_color, 
						  	instance()->mBeaconText, av_tracker.getName() );
			}
		}
		else
		{
			BOOL stop_tracking = FALSE;
			const LLUUID& avatar_id = av_tracker.getAvatarID();
			if(avatar_id.isNull())
			{
				stop_tracking = TRUE;
			}
			else 
			{
				const LLRelationship* buddy = av_tracker.getBuddyInfo(avatar_id);
				if(buddy && !buddy->isOnline() && !gAgent.isGodlike())
				{
					stop_tracking = TRUE;
				}
				if(!buddy && !gAgent.isGodlike())
				{
					stop_tracking = TRUE;
				}
			}
			if(stop_tracking)
			{
				instance()->stopTrackingAvatar();
			}
		}
	}
}


// static 
void LLTracker::trackAvatar( const LLUUID& avatar_id, const std::string& name )
{
	instance()->stopTrackingLandmark();
	instance()->stopTrackingLocation();
	
	LLAvatarTracker::instance().track( avatar_id, name );
	instance()->mTrackingStatus = TRACKING_AVATAR;
	instance()->mLabel = name;
	instance()->mToolTip = "";
}


// static 
void LLTracker::trackLandmark( const LLUUID& asset_id, const LLUUID& item_id, const std::string& name)
{
	instance()->stopTrackingAvatar();
	instance()->stopTrackingLocation();
	
 	instance()->mTrackedLandmarkAssetID = asset_id;
 	instance()->mTrackedLandmarkItemID = item_id;
 	instance()->mTrackedLandmarkName = name;
	instance()->cacheLandmarkPosition();
	instance()->mTrackingStatus = TRACKING_LANDMARK;
	instance()->mLabel = name;
	instance()->mToolTip = "";
}


// static 
void LLTracker::trackLocation(const LLVector3d& pos_global, const std::string& full_name, const std::string& tooltip, ETrackingLocationType location_type)
{
	instance()->stopTrackingAvatar();
	instance()->stopTrackingLandmark();

	instance()->mTrackedPositionGlobal = pos_global;
	instance()->mTrackedLocationName = full_name;
	instance()->mIsTrackingLocation = TRUE;
	instance()->mTrackingStatus = TRACKING_LOCATION;
	instance()->mTrackingLocationType = location_type;
	instance()->mLabel = full_name;
	instance()->mToolTip = tooltip;
}


// static 
BOOL LLTracker::handleMouseDown(S32 x, S32 y)
{
	BOOL eat_mouse_click = FALSE;
	// fortunately, we can always compute the tracking arrow center
	S32 dist_sqrd = (x - instance()->mHUDArrowCenterX) * (x - instance()->mHUDArrowCenterX) + 
					(y - instance()->mHUDArrowCenterY) * (y - instance()->mHUDArrowCenterY);
	if (dist_sqrd < ARROW_OFF_RADIUS_SQRD)
	{
		/* tracking autopilot destination has been disabled
		   -- 2004.01.09, Leviathan
		// turn off tracking
		if (gAgent.getAutoPilot())
		{
			gAgent.stopAutoPilot(TRUE);	// TRUE because cancelled by user
			eat_mouse_click = TRUE;
		}
		*/
		if (getTrackingStatus())
		{
			instance()->stopTrackingAll();
			eat_mouse_click = TRUE;
		}
	}
	return eat_mouse_click;
}


// static 
LLVector3d LLTracker::getTrackedPositionGlobal()
{
	LLVector3d pos_global;
	switch (getTrackingStatus())
	{
	case TRACKING_AVATAR:
	{
		LLAvatarTracker& av_tracker = LLAvatarTracker::instance();
		if (av_tracker.haveTrackingInfo())
		{
			pos_global = av_tracker.getGlobalPos(); }
		break;
	}
	case TRACKING_LANDMARK:
		if( instance()->mHasLandmarkPosition )
		{
			pos_global = instance()->mTrackedPositionGlobal;
		}
		break;
	case TRACKING_LOCATION:
		pos_global = instance()->mTrackedPositionGlobal;
		break;
	default:
		break;
	}
	return pos_global;
}


// static
BOOL LLTracker::hasLandmarkPosition()
{
	if (!instance()->mHasLandmarkPosition)
	{
		// maybe we just received the landmark position info
		instance()->cacheLandmarkPosition();
	}
	return instance()->mHasLandmarkPosition;
}


// static
const std::string& LLTracker::getTrackedLocationName()
{
	return instance()->mTrackedLocationName;
}

F32 pulse_func(F32 t, F32 z)
{
	if (!LLTracker::sCheesyBeacon)
	{
		return 0.f;
	}
	
	t *= F_PI;
	z -= t*64.f - 256.f;
	
	F32 a = cosf(z*F_PI/512.f)*10.0f;
	a = llmax(a, 9.9f);
	a -= 9.9f;
	a *= 10.f;
	return a;
}

void draw_shockwave(F32 center_z, F32 t, S32 steps, LLColor4 color)
{
	if (!LLTracker::sCheesyBeacon)
	{
		return;
	}
	
	t *= 0.6284f/F_PI;
	
	t -= (F32) (S32) t;	

	t = llmax(t, 0.5f);
	t -= 0.5f;
	t *= 2.0f;
	
	F32 radius = t*16536.f;
	
	// Inexact, but reasonably fast.
	F32 delta = F_TWO_PI / steps;
	F32 sin_delta = sin( delta );
	F32 cos_delta = cos( delta );
	F32 x = radius;
	F32 y = 0.f;

	LLColor4 ccol = LLColor4(1,1,1,(1.f-t)*0.25f);
	gGL.begin(LLRender::TRIANGLE_FAN);
	gGL.color4fv(ccol.mV);
	gGL.vertex3f(0.f, 0.f, center_z);
	// make sure circle is complete
	steps += 1;
	
	color.mV[3] = (1.f-t*t);
	
	gGL.color4fv(color.mV);
	while( steps-- )
	{
		// Successive rotations
		gGL.vertex3f( x, y, center_z );
		F32 x_new = x * cos_delta - y * sin_delta;
		y = x * sin_delta +  y * cos_delta;
		x = x_new;
	}
	gGL.end();
}


// static 
void LLTracker::renderBeacon(LLVector3d pos_global, 
							 const LLColor4& color, 
							 LLHUDText* hud_textp, 
							 const std::string& label )
{
	sCheesyBeacon = gSavedSettings.getBOOL("CheesyBeacon");
	LLVector3d to_vec = pos_global - gAgentCamera.getCameraPositionGlobal();

	F32 dist = (F32)to_vec.magVec();
	F32 color_frac = 1.f;
	if (dist > 0.99f * LLViewerCamera::getInstance()->getFar())
	{
		color_frac = 0.4f;
	//	pos_global = gAgentCamera.getCameraPositionGlobal() + 0.99f*(LLViewerCamera::getInstance()->getFar()/dist)*to_vec;
	}
	else
	{
		color_frac = 1.f - 0.6f*(dist/LLViewerCamera::getInstance()->getFar());
	}

	LLColor4 fogged_color = color_frac * color + (1 - color_frac)*gSky.getFogColor();

	F32 FADE_DIST = 3.f;
	fogged_color.mV[3] = llmax(0.2f, llmin(0.5f,(dist-FADE_DIST)/FADE_DIST));

	LLVector3 pos_agent = gAgent.getPosAgentFromGlobal(pos_global);

	LLGLSTracker gls_tracker; // default+ CULL_FACE + LIGHTING + GL_BLEND + GL_ALPHA_TEST
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	LLGLDisable cull_face(GL_CULL_FACE);
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
	
	
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	{
		gGL.translatef(pos_agent.mV[0], pos_agent.mV[1], pos_agent.mV[2]);
		
		draw_shockwave(1024.f, gRenderStartTime.getElapsedTimeF32(), 32, fogged_color);

		gGL.color4fv(fogged_color.mV);
		const U32 BEACON_VERTS = 256;
		const F32 step = 1024.0f/BEACON_VERTS;
		
		LLVector3 x_axis = LLViewerCamera::getInstance()->getLeftAxis();
		F32 t = gRenderStartTime.getElapsedTimeF32();
		F32 dr = dist/LLViewerCamera::getInstance()->getFar();
		
		for (U32 i = 0; i < BEACON_VERTS; i++)
		{
			F32 x = x_axis.mV[0];
			F32 y = x_axis.mV[1];
			
			F32 z = i * step;
			F32 z_next = (i+1)*step;
		
			F32 a = pulse_func(t, z);
			F32 an = pulse_func(t, z_next);
			
			LLColor4 c_col = fogged_color + LLColor4(a,a,a,a);
			LLColor4 col_next = fogged_color + LLColor4(an,an,an,an);
			LLColor4 col_edge = fogged_color * LLColor4(a,a,a,0.0f);
			LLColor4 col_edge_next = fogged_color * LLColor4(an,an,an,0.0f);
			
			a *= 2.f;
			a += 1.0f+dr;
			
			an *= 2.f;
			an += 1.0f+dr;
		
			gGL.begin(LLRender::TRIANGLE_STRIP);
			gGL.color4fv(col_edge.mV);
			gGL.vertex3f(-x*a, -y*a, z);
			gGL.color4fv(col_edge_next.mV);
			gGL.vertex3f(-x*an, -y*an, z_next);
			
			gGL.color4fv(c_col.mV);
			gGL.vertex3f(0, 0, z);
			gGL.color4fv(col_next.mV);
			gGL.vertex3f(0, 0, z_next);
			
			gGL.color4fv(col_edge.mV);
			gGL.vertex3f(x*a,y*a,z);
			gGL.color4fv(col_edge_next.mV);
			gGL.vertex3f(x*an,y*an,z_next);
			
			gGL.end();
		}
	}
	gGL.popMatrix();

	std::string text;
	text = llformat( "%.0f m", to_vec.magVec());

	std::string str;
	str += label;
	str += '\n';
	str += text;

	hud_textp->setFont(LLFontGL::getFontSansSerif());
	hud_textp->setZCompare(FALSE);
	hud_textp->setColor(LLColor4(1.f, 1.f, 1.f, llmax(0.2f, llmin(1.f,(dist-FADE_DIST)/FADE_DIST))));

	hud_textp->setString(str);
	hud_textp->setVertAlignment(LLHUDText::ALIGN_VERT_CENTER);
	hud_textp->setPositionAgent(pos_agent);
}


void LLTracker::stopTrackingAll(BOOL clear_ui)
{
	switch (mTrackingStatus)
	{
	case TRACKING_AVATAR :
		stopTrackingAvatar(clear_ui);
		break;
	case TRACKING_LANDMARK :
		stopTrackingLandmark(clear_ui);
		break;
	case TRACKING_LOCATION :
		stopTrackingLocation(clear_ui);
		break;
	default:
		mTrackingStatus = TRACKING_NOTHING;
		break;
	}
}


void LLTracker::stopTrackingAvatar(BOOL clear_ui)
{
	LLAvatarTracker& av_tracker = LLAvatarTracker::instance();
	if( !av_tracker.getAvatarID().isNull() )
	{
		av_tracker.untrack( av_tracker.getAvatarID() );
	}

	purgeBeaconText();
	gFloaterWorldMap->clearAvatarSelection(clear_ui);
	mTrackingStatus = TRACKING_NOTHING;
}


void LLTracker::stopTrackingLandmark(BOOL clear_ui)
{
	purgeBeaconText();
	mTrackedLandmarkAssetID.setNull();
	mTrackedLandmarkItemID.setNull();
	mTrackedLandmarkName.assign("");
	mTrackedPositionGlobal.zeroVec();
	mHasLandmarkPosition = FALSE;
	mHasReachedLandmark = FALSE;
	mLandmarkHasBeenVisited = TRUE;
	gFloaterWorldMap->clearLandmarkSelection(clear_ui);
	mTrackingStatus = TRACKING_NOTHING;
}


void LLTracker::stopTrackingLocation(BOOL clear_ui)
{
	purgeBeaconText();
	mTrackedLocationName.assign("");
	mIsTrackingLocation = FALSE;
	mTrackedPositionGlobal.zeroVec();
	gFloaterWorldMap->clearLocationSelection(clear_ui);
	mTrackingStatus = TRACKING_NOTHING;
	mTrackingLocationType = LOCATION_NOTHING;
}

void LLTracker::clearFocus()
{
	instance()->mTrackingStatus = TRACKING_NOTHING;
}

void LLTracker::drawMarker(const LLVector3d& pos_global, const LLColor4& color)
{
	// get position
	LLVector3 pos_local = gAgent.getPosAgentFromGlobal(pos_global);

	// check in frustum
	LLCoordGL screen;
	S32 x = 0;
	S32 y = 0;
	const BOOL CLAMP = TRUE;

	if (LLViewerCamera::getInstance()->projectPosAgentToScreen(pos_local, screen, CLAMP)
		|| LLViewerCamera::getInstance()->projectPosAgentToScreenEdge(pos_local, screen) )
	{
		gHUDView->screenPointToLocal(screen.mX, screen.mY, &x, &y);

		// the center of the rendered position of the arrow obeys 
		// the following rules:
		// (1) it lies on an ellipse centered on the target position 
		// (2) it lies on the line between the target and the window center
		// (3) right now the radii of the ellipse are fixed, but eventually
		//     they will be a function of the target text
		// 
		// from those rules we can compute the position of the 
		// lower left corner of the image
		LLRect rect = gHUDView->getRect();
		S32 x_center = lltrunc(0.5f * (F32)rect.getWidth());
		S32 y_center = lltrunc(0.5f * (F32)rect.getHeight());
		x = x - x_center;	// x and y relative to center
		y = y - y_center;
		F32 dist = sqrt((F32)(x*x + y*y));
		S32 half_arrow_size = lltrunc(0.5f * HUD_ARROW_SIZE);
		if (dist > 0.f)
		{
			const F32 ARROW_ELLIPSE_RADIUS_X = 2 * HUD_ARROW_SIZE;
			const F32 ARROW_ELLIPSE_RADIUS_Y = HUD_ARROW_SIZE;

			// compute where the arrow should be
			F32 x_target = (F32)(x + x_center) - (ARROW_ELLIPSE_RADIUS_X * ((F32)x / dist) );	
			F32 y_target = (F32)(y + y_center) - (ARROW_ELLIPSE_RADIUS_Y * ((F32)y / dist) );

			// keep the arrow within the window
			F32 x_clamped = llclamp( x_target, (F32)half_arrow_size, (F32)(rect.getWidth() - half_arrow_size));
			F32 y_clamped = llclamp( y_target, (F32)half_arrow_size, (F32)(rect.getHeight() - half_arrow_size));

			F32 slope = (F32)(y) / (F32)(x);
			F32 window_ratio = (F32)(rect.getHeight() - HUD_ARROW_SIZE) / (F32)(rect.getWidth() - HUD_ARROW_SIZE);

			// if the arrow has been clamped on one axis
			// then we need to compute the other axis
			if (llabs(slope) > window_ratio)
			{  
				if (y_clamped != (F32)y_target)
				{
					// clamp by y 
					x_clamped = (y_clamped - (F32)y_center) / slope + (F32)x_center;
				}
			}
			else if (x_clamped != (F32)x_target)
			{
				// clamp by x
				y_clamped = (x_clamped - (F32)x_center) * slope + (F32)y_center;
			}
			mHUDArrowCenterX = lltrunc(x_clamped);
			mHUDArrowCenterY = lltrunc(y_clamped);
		}
		else
		{
			// recycle the old values
			x = mHUDArrowCenterX - x_center;
			y = mHUDArrowCenterY - y_center;
		}

		F32 angle = atan2( (F32)y, (F32)x );

		gl_draw_scaled_rotated_image(mHUDArrowCenterX - half_arrow_size, 
									 mHUDArrowCenterY - half_arrow_size, 
									 HUD_ARROW_SIZE, HUD_ARROW_SIZE, 
									 RAD_TO_DEG * angle, 
									 LLWorldMapView::sTrackArrowImage->getImage(), 
									 color);
	}
}


void LLTracker::setLandmarkVisited()
{
	// poke the inventory item
	if (!mTrackedLandmarkItemID.isNull())
	{
		LLInventoryItem* i = gInventory.getItem( mTrackedLandmarkItemID );
		LLViewerInventoryItem* item = (LLViewerInventoryItem*)i;
		if (   item 
			&& !(item->getFlags()&LLInventoryItemFlags::II_FLAGS_LANDMARK_VISITED))
		{
			U32 flags = item->getFlags();
			flags |= LLInventoryItemFlags::II_FLAGS_LANDMARK_VISITED;
			item->setFlags(flags);
			LLMessageSystem* msg = gMessageSystem;
			msg->newMessage("ChangeInventoryItemFlags");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgent.getID());
			msg->addUUID("SessionID", gAgent.getSessionID());
			msg->nextBlock("InventoryData");
			msg->addUUID("ItemID", mTrackedLandmarkItemID);
			msg->addU32("Flags", flags);
			gAgent.sendReliableMessage();

			LLInventoryModel::LLCategoryUpdate up(item->getParentUUID(), 0);
			gInventory.accountForUpdate(up);

			// need to communicate that the icon needs to change...
			gInventory.addChangedMask(LLInventoryObserver::INTERNAL, item->getUUID());
			gInventory.notifyObservers();
		}
	}
}


void LLTracker::cacheLandmarkPosition()
{
	// the landmark asset download may have finished, in which case
	// we'll now be able to figure out where we're trying to go
	BOOL found_landmark = FALSE;
	if( mTrackedLandmarkAssetID == LLFloaterWorldMap::getHomeID())
	{
		LLVector3d pos_global;
		if ( gAgent.getHomePosGlobal( &mTrackedPositionGlobal ))
		{
			found_landmark = TRUE;
		}
		else
		{
			llwarns << "LLTracker couldn't find home pos" << llendl;
			mTrackedLandmarkAssetID.setNull();
			mTrackedLandmarkItemID.setNull();
		}
	}
	else
	{
		LLLandmark* landmark = gLandmarkList.getAsset(mTrackedLandmarkAssetID);
		if(landmark && landmark->getGlobalPos(mTrackedPositionGlobal))
		{
			found_landmark = TRUE;

			// cache the object's visitation status
			mLandmarkHasBeenVisited = FALSE;
			LLInventoryItem* item = gInventory.getItem(mTrackedLandmarkItemID);
			if (   item 
				&& item->getFlags()&LLInventoryItemFlags::II_FLAGS_LANDMARK_VISITED)
			{
				mLandmarkHasBeenVisited = TRUE;
			}
		}
	}
	if ( found_landmark && gFloaterWorldMap )
	{
		mHasReachedLandmark = FALSE;
		F32 dist = gFloaterWorldMap->getDistanceToDestination(mTrackedPositionGlobal, 1.0f);
		if ( dist < DESTINATION_UNVISITED_RADIUS )
		{
			mHasReachedLandmark = TRUE;
		}
		mHasLandmarkPosition = TRUE;
	}
	mHasLandmarkPosition = found_landmark;
}


void LLTracker::purgeBeaconText()
{
	if(!mBeaconText.isNull())
	{
		mBeaconText->markDead();
		mBeaconText = NULL;
	}
}

