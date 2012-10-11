/** 
 * @file llworldmapview.cpp
 * @brief LLWorldMapView class implementation
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

#include "llworldmapview.h"

#include "indra_constants.h"
#include "llui.h"
#include "llmath.h"		// clampf()
#include "llregionhandle.h"
#include "lleventflags.h"
#include "llfloaterreg.h"
#include "llrender.h"
#include "lltooltip.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llcallingcard.h"
#include "llcommandhandler.h"
#include "llviewercontrol.h"
#include "llfloatermap.h"
#include "llfloaterworldmap.h"
#include "llfocusmgr.h"
#include "lllocalcliprect.h"
#include "lltextbox.h"
#include "lltextureview.h"
#include "lltracker.h"
#include "llviewercamera.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "lltrans.h"

#include "llglheaders.h"

// Basically a C++ implementation of the OCEAN_COLOR defined in mapstitcher.py 
// Please ensure consistency between those 2 files (TODO: would be better to get that color from an asset source...)
// # Constants
// OCEAN_COLOR = "#1D475F"
const F32 OCEAN_RED   = (F32)(0x1D)/255.f;
const F32 OCEAN_GREEN = (F32)(0x47)/255.f;
const F32 OCEAN_BLUE  = (F32)(0x5F)/255.f;

const F32 GODLY_TELEPORT_HEIGHT = 200.f;
const S32 SCROLL_HINT_WIDTH = 65;
const F32 BIG_DOT_RADIUS = 5.f;
BOOL LLWorldMapView::sHandledLastClick = FALSE;

LLUIImagePtr LLWorldMapView::sAvatarSmallImage = NULL;
LLUIImagePtr LLWorldMapView::sAvatarYouImage = NULL;
LLUIImagePtr LLWorldMapView::sAvatarYouLargeImage = NULL;
LLUIImagePtr LLWorldMapView::sAvatarLevelImage = NULL;
LLUIImagePtr LLWorldMapView::sAvatarAboveImage = NULL;
LLUIImagePtr LLWorldMapView::sAvatarBelowImage = NULL;
LLUIImagePtr LLWorldMapView::sAvatarUnknownImage = NULL;

LLUIImagePtr LLWorldMapView::sTelehubImage = NULL;
LLUIImagePtr LLWorldMapView::sInfohubImage = NULL;
LLUIImagePtr LLWorldMapView::sHomeImage = NULL;
LLUIImagePtr LLWorldMapView::sEventImage = NULL;
LLUIImagePtr LLWorldMapView::sEventMatureImage = NULL;
LLUIImagePtr LLWorldMapView::sEventAdultImage = NULL;

LLUIImagePtr LLWorldMapView::sTrackCircleImage = NULL;
LLUIImagePtr LLWorldMapView::sTrackArrowImage = NULL;

LLUIImagePtr LLWorldMapView::sClassifiedsImage = NULL;
LLUIImagePtr LLWorldMapView::sForSaleImage = NULL;
LLUIImagePtr LLWorldMapView::sForSaleAdultImage = NULL;

F32 LLWorldMapView::sPanX = 0.f;
F32 LLWorldMapView::sPanY = 0.f;
F32 LLWorldMapView::sTargetPanX = 0.f;
F32 LLWorldMapView::sTargetPanY = 0.f;
S32 LLWorldMapView::sTrackingArrowX = 0;
S32 LLWorldMapView::sTrackingArrowY = 0;
bool LLWorldMapView::sVisibleTilesLoaded = false;
F32 LLWorldMapView::sMapScale = 128.f;

std::map<std::string,std::string> LLWorldMapView::sStringsMap;

// Fetch and draw info thresholds
const F32 DRAW_TEXT_THRESHOLD = 96.f;		// Don't draw text under that resolution value (res = width region in meters)
const S32 DRAW_SIMINFO_THRESHOLD = 3;		// Max level for which we load or display sim level information (level in LLWorldMipmap sense)
const S32 DRAW_LANDFORSALE_THRESHOLD = 2;	// Max level for which we load or display land for sale picture data (level in LLWorldMipmap sense)

// When on, draw an outline for each mipmap tile gotten from S3
#define DEBUG_DRAW_TILE 0


void LLWorldMapView::initClass()
{
	sAvatarSmallImage =		LLUI::getUIImage("map_avatar_8.tga");
	sAvatarYouImage =		LLUI::getUIImage("map_avatar_16.tga");
	sAvatarYouLargeImage =	LLUI::getUIImage("map_avatar_you_32.tga");
	sAvatarLevelImage =		LLUI::getUIImage("map_avatar_32.tga");
	sAvatarAboveImage =		LLUI::getUIImage("map_avatar_above_32.tga");
	sAvatarBelowImage =		LLUI::getUIImage("map_avatar_below_32.tga");
	sAvatarUnknownImage =	LLUI::getUIImage("map_avatar_unknown_32.tga");

	sHomeImage =			LLUI::getUIImage("map_home.tga");
	sTelehubImage = 		LLUI::getUIImage("map_telehub.tga");
	sInfohubImage = 		LLUI::getUIImage("map_infohub.tga");
	sEventImage =			LLUI::getUIImage("Parcel_PG_Dark");
	sEventMatureImage =		LLUI::getUIImage("Parcel_M_Dark");
	// To Do: update the image resource for adult events.
	sEventAdultImage =		LLUI::getUIImage("Parcel_R_Dark");

	sTrackCircleImage =		LLUI::getUIImage("map_track_16.tga");
	sTrackArrowImage =		LLUI::getUIImage("direction_arrow.tga");
	sClassifiedsImage =		LLUI::getUIImage("icon_top_pick.tga");
	sForSaleImage =			LLUI::getUIImage("icon_for_sale.tga");
	// To Do: update the image resource for adult lands on sale.
	sForSaleAdultImage =    LLUI::getUIImage("icon_for_sale_adult.tga");
	
	sStringsMap["loading"] = LLTrans::getString("texture_loading");
	sStringsMap["offline"] = LLTrans::getString("worldmap_offline");
}

// static
void LLWorldMapView::cleanupClass()
{
	sAvatarSmallImage = NULL;
	sAvatarYouImage = NULL;
	sAvatarYouLargeImage = NULL;
	sAvatarLevelImage = NULL;
	sAvatarAboveImage = NULL;
	sAvatarBelowImage = NULL;
	sAvatarUnknownImage = NULL;

	sTelehubImage = NULL;
	sInfohubImage = NULL;
	sHomeImage = NULL;
	sEventImage = NULL;
	sEventMatureImage = NULL;
	sEventAdultImage = NULL;

	sTrackCircleImage = NULL;
	sTrackArrowImage = NULL;
	sClassifiedsImage = NULL;
	sForSaleImage = NULL;
	sForSaleAdultImage = NULL;
}

LLWorldMapView::LLWorldMapView()
:	LLPanel(),
	mBackgroundColor( LLColor4( OCEAN_RED, OCEAN_GREEN, OCEAN_BLUE, 1.f ) ),
	mItemPicked(FALSE),
	mPanning( FALSE ),
	mMouseDownPanX( 0 ),
	mMouseDownPanY( 0 ),
	mMouseDownX( 0 ),
	mMouseDownY( 0 ),
	mSelectIDStart(0)
{
	//LL_INFOS("World Map") << "Creating the Map -> LLWorldMapView::LLWorldMapView()" << LL_ENDL;

	clearLastClick();
}

BOOL LLWorldMapView::postBuild()
{
	mTextBoxNorth = getChild<LLTextBox> ("floater_map_north");
	mTextBoxEast = getChild<LLTextBox> ("floater_map_east");
	mTextBoxWest = getChild<LLTextBox> ("floater_map_west");
	mTextBoxSouth = getChild<LLTextBox> ("floater_map_south");
	mTextBoxSouthEast = getChild<LLTextBox> ("floater_map_southeast");
	mTextBoxNorthEast = getChild<LLTextBox> ("floater_map_northeast");
	mTextBoxSouthWest = getChild<LLTextBox> ("floater_map_southwest");
	mTextBoxNorthWest = getChild<LLTextBox> ("floater_map_northwest");
	
	mTextBoxNorth->setText(getString("world_map_north"));
	mTextBoxEast->setText(getString ("world_map_east"));
	mTextBoxWest->setText(getString("world_map_west"));
	mTextBoxSouth->setText(getString ("world_map_south"));
	mTextBoxSouthEast ->setText(getString ("world_map_southeast"));
	mTextBoxNorthEast ->setText(getString ("world_map_northeast"));
	mTextBoxSouthWest->setText(getString ("world_map_southwest"));
	mTextBoxNorthWest ->setText(getString("world_map_northwest"));
	
	mTextBoxNorth->reshapeToFitText();
	mTextBoxEast->reshapeToFitText();
	mTextBoxWest->reshapeToFitText();
	mTextBoxSouth->reshapeToFitText();
	mTextBoxSouthEast ->reshapeToFitText();
	mTextBoxNorthEast ->reshapeToFitText();
	mTextBoxSouthWest->reshapeToFitText();
	mTextBoxNorthWest ->reshapeToFitText();

	return true;
}


LLWorldMapView::~LLWorldMapView()
{
	//LL_INFOS("World Map") << "Destroying the map -> LLWorldMapView::~LLWorldMapView()" << LL_ENDL;
	cleanupTextures();
}


// static
void LLWorldMapView::cleanupTextures()
{
}


// static
void LLWorldMapView::setScale( F32 scale )
{
	if (scale != sMapScale)
	{
		F32 old_scale = sMapScale;

		sMapScale = scale;
		if (sMapScale <= 0.f)
		{
			sMapScale = 0.1f;
		}

		F32 ratio = (scale / old_scale);
		sPanX *= ratio;
		sPanY *= ratio;
		sTargetPanX = sPanX;
		sTargetPanY = sPanY;
		sVisibleTilesLoaded = false;
	}
}


// static
void LLWorldMapView::translatePan( S32 delta_x, S32 delta_y )
{
	sPanX += delta_x;
	sPanY += delta_y;
	sTargetPanX = sPanX;
	sTargetPanY = sPanY;
	sVisibleTilesLoaded = false;
}


// static
void LLWorldMapView::setPan( S32 x, S32 y, BOOL snap )
{
	sTargetPanX = (F32)x;
	sTargetPanY = (F32)y;
	if (snap)
	{
		sPanX = sTargetPanX;
		sPanY = sTargetPanY;
	}
	sVisibleTilesLoaded = false;
}

bool LLWorldMapView::showRegionInfo()
{
	return (LLWorldMipmap::scaleToLevel(sMapScale) <= DRAW_SIMINFO_THRESHOLD ? true : false);
}

///////////////////////////////////////////////////////////////////////////////////
// HELPERS

BOOL is_agent_in_region(LLViewerRegion* region, LLSimInfo* info)
{
	return (region && info && info->isName(region->getName()));
}

///////////////////////////////////////////////////////////////////////////////////

void LLWorldMapView::draw()
{
	static LLUIColor map_track_color = LLUIColorTable::instance().getColor("MapTrackColor", LLColor4::white);
	
	LLTextureView::clearDebugImages();

	F64 current_time = LLTimer::getElapsedSeconds();

	mVisibleRegions.clear();

	// animate pan if necessary
	sPanX = lerp(sPanX, sTargetPanX, LLCriticalDamp::getInterpolant(0.1f));
	sPanY = lerp(sPanY, sTargetPanY, LLCriticalDamp::getInterpolant(0.1f));

	const S32 width = getRect().getWidth();
	const S32 height = getRect().getHeight();
	const F32 half_width = F32(width) / 2.0f;
	const F32 half_height = F32(height) / 2.0f;
	LLVector3d camera_global = gAgentCamera.getCameraPositionGlobal();

	S32 level = LLWorldMipmap::scaleToLevel(sMapScale);

	LLLocalClipRect clip(getLocalRect());
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		gGL.matrixMode(LLRender::MM_MODELVIEW);

		// Clear the background alpha to 0
		gGL.flush();
		gGL.setColorMask(false, true);
		gGL.setAlphaRejectSettings(LLRender::CF_GREATER_EQUAL, 0.f);
		gGL.setSceneBlendType(LLRender::BT_REPLACE);
		gGL.color4f(0.0f, 0.0f, 0.0f, 0.0f);
		gl_rect_2d(0, height, width, 0);
	}

	gGL.flush();

	gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
	gGL.setColorMask(true, true);

	// Draw the image tiles
	drawMipmap(width, height);
	gGL.flush();

	gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
	gGL.setColorMask(true, true);

	// Draw per sim overlayed information (names, mature, offline...)
	for (LLWorldMap::sim_info_map_t::const_iterator it = LLWorldMap::getInstance()->getRegionMap().begin();
		 it != LLWorldMap::getInstance()->getRegionMap().end(); ++it)
	{
		U64 handle = it->first;
		LLSimInfo* info = it->second;

		LLVector3d origin_global = from_region_handle(handle);

		// Find x and y position relative to camera's center.
		LLVector3d rel_region_pos = origin_global - camera_global;
		F32 relative_x = (rel_region_pos.mdV[0] / REGION_WIDTH_METERS) * sMapScale;
		F32 relative_y = (rel_region_pos.mdV[1] / REGION_WIDTH_METERS) * sMapScale;

		// Coordinates of the sim in pixels in the UI panel
		// When the view isn't panned, 0,0 = center of rectangle
		F32 bottom =    sPanY + half_height + relative_y;
		F32 left =      sPanX + half_width + relative_x;
		F32 top =       bottom + sMapScale ;
		F32 right =     left + sMapScale ;

		// Discard if region is outside the screen rectangle (not visible on screen)
		if ((top < 0.f)   || (bottom > height) ||
			(right < 0.f) || (left > width)       )
		{
			// Drop the "land for sale" fetching priority since it's outside the view rectangle
			info->dropImagePriority();
			continue;
		}

		// This list is used by other methods to know which regions are indeed displayed on screen

		mVisibleRegions.push_back(handle);

		// Update the agent count for that region if we're not too zoomed out already
		if (level <= DRAW_SIMINFO_THRESHOLD)
		{
			info->updateAgentCount(current_time);
		}

		if (info->isDown())
		{
			// Draw a transparent red square over down sims
			gGL.blendFunc(LLRender::BF_DEST_ALPHA, LLRender::BF_SOURCE_ALPHA);
			gGL.color4f(0.2f, 0.0f, 0.0f, 0.4f);

			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			gGL.begin(LLRender::QUADS);
				gGL.vertex2f(left, top);
				gGL.vertex2f(left, bottom);
				gGL.vertex2f(right, bottom);
				gGL.vertex2f(right, top);
			gGL.end();
		}
        // As part of the AO project, we no longer want to draw access indicators;
		// it's too complicated to get all the rules straight and will only
		// cause confusion.
		/**********************
        else if (!info->isPG() && gAgent.isTeen())
		{
			// If this is a mature region, and you are not, draw a line across it
			gGL.blendFunc(LLRender::BF_DEST_ALPHA, LLRender::BF_ZERO);

			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			gGL.color3f(1.f, 0.f, 0.f);
			gGL.begin(LLRender::LINES);
				gGL.vertex2f(left, top);
				gGL.vertex2f(right, bottom);
				gGL.vertex2f(left, bottom);
				gGL.vertex2f(right, top);
			gGL.end();
		}
		 **********************/
		else if (gSavedSettings.getBOOL("MapShowLandForSale") && (level <= DRAW_LANDFORSALE_THRESHOLD))
		{
			// Draw the overlay image "Land for Sale / Land for Auction"
			LLViewerFetchedTexture* overlayimage = info->getLandForSaleImage();
			if (overlayimage)
			{
				// Inform the fetch mechanism of the size we need
				S32 draw_size = llround(sMapScale);
				overlayimage->setKnownDrawSize(llround(draw_size * LLUI::getScaleFactor().mV[VX]), llround(draw_size * LLUI::getScaleFactor().mV[VY]));
				// Draw something whenever we have enough info
				if (overlayimage->hasGLTexture())
				{
					gGL.blendFunc(LLRender::BF_SOURCE_ALPHA, LLRender::BF_ONE_MINUS_SOURCE_ALPHA);	
					gGL.getTexUnit(0)->bind(overlayimage);
					gGL.color4f(1.f, 1.f, 1.f, 1.f);
					gGL.begin(LLRender::QUADS);
						gGL.texCoord2f(0.f, 1.f);
						gGL.vertex3f(left, top, -0.5f);
						gGL.texCoord2f(0.f, 0.f);
						gGL.vertex3f(left, bottom, -0.5f);
						gGL.texCoord2f(1.f, 0.f);
						gGL.vertex3f(right, bottom, -0.5f);
						gGL.texCoord2f(1.f, 1.f);
						gGL.vertex3f(right, top, -0.5f);
					gGL.end();
				}
			}
		}
		else
		{
			// If we're not displaying the "land for sale", drop its fetching priority
			info->dropImagePriority();
		}

		// Draw the region name in the lower left corner
		if (sMapScale >= DRAW_TEXT_THRESHOLD)
		{
			LLFontGL* font = LLFontGL::getFont(LLFontDescriptor("SansSerif", "Small", LLFontGL::BOLD));
			std::string mesg;
			if (info->isDown())
			{
				mesg = llformat( "%s (%s)", info->getName().c_str(), sStringsMap["offline"].c_str());
			}
			else
			{
				mesg = info->getName();
			}
			if (!mesg.empty())
			{
				font->renderUTF8(
					mesg, 0,
					llfloor(left + 3), llfloor(bottom + 2),
					LLColor4::white,
					LLFontGL::LEFT, LLFontGL::BASELINE, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW);
			}
		}
	}



	// Draw background rectangle
	LLGLSUIDefault gls_ui;
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.setAlphaRejectSettings(LLRender::CF_GREATER_EQUAL, 0.f);
		gGL.blendFunc(LLRender::BF_ONE_MINUS_DEST_ALPHA, LLRender::BF_DEST_ALPHA);
		gGL.color4fv( mBackgroundColor.mV );
		gl_rect_2d(0, height, width, 0);
	}

	gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);

	// Draw item infos if we're not zoomed out too much and there's something to draw
	if ((level <= DRAW_SIMINFO_THRESHOLD) && (gSavedSettings.getBOOL("MapShowInfohubs") || 
											  gSavedSettings.getBOOL("MapShowTelehubs") ||
											  gSavedSettings.getBOOL("MapShowLandForSale") || 
											  gSavedSettings.getBOOL("MapShowEvents") || 
											  gSavedSettings.getBOOL("ShowMatureEvents") ||
											  gSavedSettings.getBOOL("ShowAdultEvents")))
	{
		drawItems();
	}

	// Draw the Home location (always)
	LLVector3d home;
	if (gAgent.getHomePosGlobal(&home))
	{
		drawImage(home, sHomeImage);
	}

	// Draw the current agent after all that other stuff.
	LLVector3d pos_global = gAgent.getPositionGlobal();
	drawImage(pos_global, sAvatarYouImage);

	LLVector3 pos_map = globalPosToView(pos_global);
	if (!pointInView(llround(pos_map.mV[VX]), llround(pos_map.mV[VY])))
	{
		drawTracking(pos_global,
					 lerp(LLColor4::yellow, LLColor4::orange, 0.4f),
					 TRUE,
					 "You are here",
					 "",
					 LLFontGL::getFontSansSerifSmall()->getLineHeight()); // offset vertically by one line, to avoid overlap with target tracking
	}

	// Draw the current agent viewing angle
	drawFrustum();

	// Draw icons for the avatars in each region.
	// Drawn this after the current agent avatar so one can see nearby people
	if (gSavedSettings.getBOOL("MapShowPeople") && (level <= DRAW_SIMINFO_THRESHOLD))
	{
		drawAgents();
	}

	// Always draw tracking information
	LLTracker::ETrackingStatus tracking_status = LLTracker::getTrackingStatus();
	if ( LLTracker::TRACKING_AVATAR == tracking_status )
	{
		drawTracking( LLAvatarTracker::instance().getGlobalPos(), map_track_color, TRUE, LLTracker::getLabel(), "" );
	}
	else if ( LLTracker::TRACKING_LANDMARK == tracking_status
			  || LLTracker::TRACKING_LOCATION == tracking_status )
	{
		// While fetching landmarks, will have 0,0,0 location for a while,
		// so don't draw. JC
		LLVector3d pos_global = LLTracker::getTrackedPositionGlobal();
		if (!pos_global.isExactlyZero())
		{
			drawTracking( pos_global, map_track_color, TRUE, LLTracker::getLabel(), LLTracker::getToolTip() );
		}
	}
	else if (LLWorldMap::getInstance()->isTracking())
	{
		if (LLWorldMap::getInstance()->isTrackingInvalidLocation())
		{
			// We know this location to be invalid, draw a blue circle
			LLColor4 loading_color(0.0, 0.5, 1.0, 1.0);
			drawTracking( LLWorldMap::getInstance()->getTrackedPositionGlobal(), loading_color, TRUE, getString("InvalidLocation"), "");
		}
		else
		{
			// We don't know yet what that location is, draw a throbing blue circle
			double value = fmod(current_time, 2);
			value = 0.5 + 0.5*cos(value * F_PI);
			LLColor4 loading_color(0.0, F32(value/2), F32(value), 1.0);
			drawTracking( LLWorldMap::getInstance()->getTrackedPositionGlobal(), loading_color, TRUE, getString("Loading"), "");
		}
	}


	// turn off the scissor
	LLGLDisable no_scissor(GL_SCISSOR_TEST);

	updateDirections();

	LLView::draw();

	// Get sim info for all sims in view
	updateVisibleBlocks();
} // end draw()


//virtual
void LLWorldMapView::setVisible(BOOL visible)
{
	LLPanel::setVisible(visible);
	if (!visible)
	{
		// Drop the download of tiles and images priority to nil if we hide the map
		LLWorldMap::getInstance()->dropImagePriorities();
	}
}

void LLWorldMapView::drawMipmap(S32 width, S32 height)
{
	// Compute the level of the mipmap to use for the current scale level
	S32 level = LLWorldMipmap::scaleToLevel(sMapScale);
	// Set the tile boost level so that unused tiles get to 0
	LLWorldMap::getInstance()->equalizeBoostLevels();

	// Render whatever we already have loaded if we haven't the current level
	// complete and use it as a background (scaled up or scaled down)
	if (!sVisibleTilesLoaded)
	{
		// Note: the (load = false) parameter avoids missing tiles to be fetched (i.e. we render what we have, no more)
		// Check all the lower res levels and render them in reverse order (worse to best)
		// We need to traverse all the levels as the user can zoom in very fast
		for (S32 l = LLWorldMipmap::MAP_LEVELS; l > level; l--)
		{
			drawMipmapLevel(width, height, l, false);
		}
		// Skip the current level, as we'll do it anyway here under...

		// Just go one level down in res as it can really get too much stuff 
		// when zooming out and too small to see anyway...
		if (level > 1)
		{
			drawMipmapLevel(width, height, level - 1, false);
		}
	}
	else
	{
		//LL_INFOS("World Map") << "Render complete, don't draw background..." << LL_ENDL;
	}

	// Render the current level
	sVisibleTilesLoaded = drawMipmapLevel(width, height, level);

	return;
}

// Return true if all the tiles required to render that level have been fetched or are truly missing
bool LLWorldMapView::drawMipmapLevel(S32 width, S32 height, S32 level, bool load)
{
	// Check input level
	llassert (level > 0);
	if (level <= 0)
		return false;

	// Count tiles hit and completed
	S32 completed_tiles = 0;
	S32 total_tiles = 0;

	// Size in meters (global) of each tile of that level
	S32 tile_width = LLWorldMipmap::MAP_TILE_SIZE * (1 << (level - 1));
	// Dimension of the screen in meter at that scale
	LLVector3d pos_SW = viewPosToGlobal(0, 0);
	LLVector3d pos_NE = viewPosToGlobal(width, height);
	// Add external band of tiles on the outskirt so to hit the partially displayed tiles right and top
	pos_NE[VX] += tile_width;
	pos_NE[VY] += tile_width;

	// Iterate through the tiles on screen: we just need to ask for one tile every tile_width meters
	U32 grid_x, grid_y;
	for (F64 index_y = pos_SW[VY]; index_y < pos_NE[VY]; index_y += tile_width)
	{
		for (F64 index_x = pos_SW[VX]; index_x < pos_NE[VX]; index_x += tile_width)
		{
			// Compute the world coordinates of the current point
			LLVector3d pos_global(index_x, index_y, pos_SW[VZ]);
			// Convert to the mipmap level coordinates for that point (i.e. which tile to we hit)
			LLWorldMipmap::globalToMipmap(pos_global[VX], pos_global[VY], level, &grid_x, &grid_y);
			// Get the tile. Note: NULL means that the image does not exist (so it's considered "complete" as far as fetching is concerned)
			LLPointer<LLViewerFetchedTexture> simimage = LLWorldMap::getInstance()->getObjectsTile(grid_x, grid_y, level, load);
			if (simimage)
			{
				// Checks that the image has a valid texture
				if (simimage->hasGLTexture())
				{
					// Increment the number of completly fetched tiles
					completed_tiles++;

					// Convert those coordinates (SW corner of the mipmap tile) into world (meters) coordinates
					pos_global[VX] = grid_x * REGION_WIDTH_METERS;
					pos_global[VY] = grid_y * REGION_WIDTH_METERS;
					// Now to screen coordinates for SW corner of that tile
					LLVector3 pos_screen = globalPosToView (pos_global);
					F32 left   = pos_screen[VX];
					F32 bottom = pos_screen[VY];
					// Compute the NE corner coordinates of the tile now
					pos_global[VX] += tile_width;
					pos_global[VY] += tile_width;
					pos_screen = globalPosToView (pos_global);
					F32 right  = pos_screen[VX];
					F32 top    = pos_screen[VY];

					// Draw the tile
					LLGLSUIDefault gls_ui;
					gGL.getTexUnit(0)->bind(simimage.get());
					simimage->setAddressMode(LLTexUnit::TAM_CLAMP);

					gGL.setSceneBlendType(LLRender::BT_ALPHA);
					gGL.color4f(1.f, 1.0f, 1.0f, 1.0f);

					gGL.begin(LLRender::QUADS);
						gGL.texCoord2f(0.f, 1.f);
						gGL.vertex3f(left, top, 0.f);
						gGL.texCoord2f(0.f, 0.f);
						gGL.vertex3f(left, bottom, 0.f);
						gGL.texCoord2f(1.f, 0.f);
						gGL.vertex3f(right, bottom, 0.f);
						gGL.texCoord2f(1.f, 1.f);
						gGL.vertex3f(right, top, 0.f);
					gGL.end();
#if DEBUG_DRAW_TILE
					drawTileOutline(level, top, left, bottom, right);
#endif // DEBUG_DRAW_TILE
				}
				//else
				//{
				//	Waiting for a tile -> the level is not complete
				//	LL_INFOS("World Map") << "Unfetched tile. level = " << level << LL_ENDL;
				//}
			}
			else
			{
				// Unexistent tiles are counted as "completed"
				completed_tiles++;
			}
			// Increment the number of tiles in that level / screen
			total_tiles++;
		}
	}
	return (completed_tiles == total_tiles);
}

// Draw lines (rectangle outline and cross) to visualize the position of the tile
// Used for debug only
void LLWorldMapView::drawTileOutline(S32 level, F32 top, F32 left, F32 bottom, F32 right)
{
	gGL.blendFunc(LLRender::BF_DEST_ALPHA, LLRender::BF_ZERO);
	
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	if (level == 1)
		gGL.color3f(1.f, 0.f, 0.f);		// red
	else if (level == 2)
		gGL.color3f(0.f, 1.f, 0.f);		// green
	else if (level == 3)
		gGL.color3f(0.f, 0.f, 1.f);		// blue
	else if (level == 4)
		gGL.color3f(1.f, 1.f, 0.f);		// yellow
	else if (level == 5)
		gGL.color3f(1.f, 0.f, 1.f);		// magenta
	else if (level == 6)
		gGL.color3f(0.f, 1.f, 1.f);		// cyan
	else if (level == 7)
		gGL.color3f(1.f, 1.f, 1.f);		// white
	else
		gGL.color3f(0.f, 0.f, 0.f);		// black
	gGL.begin(LLRender::LINE_STRIP);
		gGL.vertex2f(left, top);
		gGL.vertex2f(right, bottom);
		gGL.vertex2f(left, bottom);
		gGL.vertex2f(right, top);
		gGL.vertex2f(left, top);
		gGL.vertex2f(left, bottom);
		gGL.vertex2f(right, bottom);
		gGL.vertex2f(right, top);
	gGL.end();
}

void LLWorldMapView::drawGenericItems(const LLSimInfo::item_info_list_t& items, LLUIImagePtr image)
{
	LLSimInfo::item_info_list_t::const_iterator e;
	for (e = items.begin(); e != items.end(); ++e)
	{
		drawGenericItem(*e, image);
	}
}

void LLWorldMapView::drawGenericItem(const LLItemInfo& item, LLUIImagePtr image)
{
	drawImage(item.getGlobalPosition(), image);
}


void LLWorldMapView::drawImage(const LLVector3d& global_pos, LLUIImagePtr image, const LLColor4& color)
{
	LLVector3 pos_map = globalPosToView( global_pos );
	image->draw(llround(pos_map.mV[VX] - image->getWidth() /2.f),
				llround(pos_map.mV[VY] - image->getHeight()/2.f),
				color);
}

void LLWorldMapView::drawImageStack(const LLVector3d& global_pos, LLUIImagePtr image, U32 count, F32 offset, const LLColor4& color)
{
	LLVector3 pos_map = globalPosToView( global_pos );
	for(U32 i=0; i<count; i++)
	{
		image->draw(llround(pos_map.mV[VX] - image->getWidth() /2.f),
					llround(pos_map.mV[VY] - image->getHeight()/2.f + i*offset),
					color);
	}
}

void LLWorldMapView::drawItems()
{
	bool mature_enabled = gAgent.canAccessMature();
	bool adult_enabled = gAgent.canAccessAdult();

    BOOL show_mature = mature_enabled && gSavedSettings.getBOOL("ShowMatureEvents");
	BOOL show_adult = adult_enabled && gSavedSettings.getBOOL("ShowAdultEvents");

	for (handle_list_t::iterator iter = mVisibleRegions.begin(); iter != mVisibleRegions.end(); ++iter)
	{
		U64 handle = *iter;
		LLSimInfo* info = LLWorldMap::getInstance()->simInfoFromHandle(handle);
		if ((info == NULL) || (info->isDown()))
		{
			continue;
		}
		// Infohubs
		if (gSavedSettings.getBOOL("MapShowInfohubs"))
		{
			drawGenericItems(info->getInfoHub(), sInfohubImage);
		}
		// Telehubs
		if (gSavedSettings.getBOOL("MapShowTelehubs"))
		{
			drawGenericItems(info->getTeleHub(), sTelehubImage);
		}
		// Land for sale
		if (gSavedSettings.getBOOL("MapShowLandForSale"))
		{
			drawGenericItems(info->getLandForSale(), sForSaleImage);
			// for 1.23, we're showing normal land and adult land in the same UI; you don't
			// get a choice about which ones you want. If you're currently asking for adult
			// content and land you'll get the adult land.
			if (gAgent.canAccessAdult())
			{
				drawGenericItems(info->getLandForSaleAdult(), sForSaleAdultImage);
			}
		}
		// PG Events
		if (gSavedSettings.getBOOL("MapShowEvents"))
		{
			drawGenericItems(info->getPGEvent(), sEventImage);
		}
		// Mature Events
		if (show_mature)
		{
			drawGenericItems(info->getMatureEvent(), sEventMatureImage);
		}
		// Adult Events
		if (show_adult)
		{
			drawGenericItems(info->getAdultEvent(), sEventAdultImage);
		}
	}
}

void LLWorldMapView::drawAgents()
{
	static LLUIColor map_avatar_color = LLUIColorTable::instance().getColor("MapAvatarColor", LLColor4::white);

	for (handle_list_t::iterator iter = mVisibleRegions.begin(); iter != mVisibleRegions.end(); ++iter)
	{
		U64 handle = *iter;
		LLSimInfo* siminfo = LLWorldMap::getInstance()->simInfoFromHandle(handle);
		if ((siminfo == NULL) || (siminfo->isDown()))
		{
			continue;
		}
		LLSimInfo::item_info_list_t::const_iterator it = siminfo->getAgentLocation().begin();
		while (it != siminfo->getAgentLocation().end())
		{
			// Show Individual agents (or little stacks where real agents are)

			// Here's how we'd choose the color if info.mID were available but it's not being sent:
			// LLColor4 color = (agent_count == 1 && is_agent_friend(info.mID)) ? friend_color : avatar_color;
			drawImageStack(it->getGlobalPosition(), sAvatarSmallImage, it->getCount(), 3.f, map_avatar_color);
			++it;
		}
	}
}

void LLWorldMapView::drawFrustum()
{
	// Draw frustum
	F32 meters_to_pixels = sMapScale/ REGION_WIDTH_METERS;

	F32 horiz_fov = LLViewerCamera::getInstance()->getView() * LLViewerCamera::getInstance()->getAspect();
	F32 far_clip_meters = LLViewerCamera::getInstance()->getFar();
	F32 far_clip_pixels = far_clip_meters * meters_to_pixels;

	F32 half_width_meters = far_clip_meters * tan( horiz_fov / 2 );
	F32 half_width_pixels = half_width_meters * meters_to_pixels;
	
	// Compute the frustum coordinates. Take the UI scale into account.
	F32 ui_scale_factor = gSavedSettings.getF32("UIScaleFactor");
	F32 ctr_x = (getLocalRect().getWidth() * 0.5f + sPanX)  * ui_scale_factor;
	F32 ctr_y = (getLocalRect().getHeight() * 0.5f + sPanY) * ui_scale_factor;

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	// Since we don't rotate the map, we have to rotate the frustum.
	gGL.pushMatrix();
	{
		gGL.translatef( ctr_x, ctr_y, 0 );

		// Draw triangle with more alpha in far pixels to make it 
		// fade out in distance.
		gGL.begin( LLRender::TRIANGLES  );
		{
			// get camera look at and left axes
			LLVector3 at_axis = LLViewerCamera::instance().getAtAxis();
			LLVector3 left_axis = LLViewerCamera::instance().getLeftAxis();

			// grab components along XY plane
			LLVector2 cam_lookat(at_axis.mV[VX], at_axis.mV[VY]);
			LLVector2 cam_left(left_axis.mV[VX], left_axis.mV[VY]);

			// but, when looking near straight up or down...
			if (is_approx_zero(cam_lookat.magVecSquared()))
			{
				//...just fall back to looking down the x axis
				cam_lookat = LLVector2(1.f, 0.f); // x axis
				cam_left = LLVector2(0.f, 1.f); // y axis
			}

			// normalize to unit length
			cam_lookat.normVec();
			cam_left.normVec();

			gGL.color4f(1.f, 1.f, 1.f, 0.25f);
			gGL.vertex2f( 0, 0 );

			gGL.color4f(1.f, 1.f, 1.f, 0.02f);
			
			// use 2d camera vectors to render frustum triangle
			LLVector2 vert = cam_lookat * far_clip_pixels + cam_left * half_width_pixels;
			gGL.vertex2f(vert.mV[VX], vert.mV[VY]);

			vert = cam_lookat * far_clip_pixels - cam_left * half_width_pixels;
			gGL.vertex2f(vert.mV[VX], vert.mV[VY]);
		}
		gGL.end();
	}
	gGL.popMatrix();
}


LLVector3 LLWorldMapView::globalPosToView( const LLVector3d& global_pos )
{
	LLVector3d relative_pos_global = global_pos - gAgentCamera.getCameraPositionGlobal();
	LLVector3 pos_local;
	pos_local.setVec(relative_pos_global);  // convert to floats from doubles

	pos_local.mV[VX] *= sMapScale / REGION_WIDTH_METERS;
	pos_local.mV[VY] *= sMapScale / REGION_WIDTH_METERS;
	// leave Z component in meters


	pos_local.mV[VX] += getRect().getWidth() / 2 + sPanX;
	pos_local.mV[VY] += getRect().getHeight() / 2 + sPanY;

	return pos_local;
}


void LLWorldMapView::drawTracking(const LLVector3d& pos_global, const LLColor4& color, BOOL draw_arrow,
								  const std::string& label, const std::string& tooltip, S32 vert_offset )
{
	LLVector3 pos_local = globalPosToView( pos_global );
	S32 x = llround( pos_local.mV[VX] );
	S32 y = llround( pos_local.mV[VY] );
	LLFontGL* font = LLFontGL::getFontSansSerifSmall();
	S32 text_x = x;
	S32 text_y = (S32)(y - sTrackCircleImage->getHeight()/2 - font->getLineHeight());

	if(    x < 0 
		|| y < 0 
		|| x >= getRect().getWidth() 
		|| y >= getRect().getHeight() )
	{
		if (draw_arrow)
		{
			drawTrackingCircle( getRect(), x, y, color, 3, 15 );
			drawTrackingArrow( getRect(), x, y, color );
			text_x = sTrackingArrowX;
			text_y = sTrackingArrowY;
		}
	}
	else if (LLTracker::getTrackingStatus() == LLTracker::TRACKING_LOCATION &&
		LLTracker::getTrackedLocationType() != LLTracker::LOCATION_NOTHING)
	{
		drawTrackingCircle( getRect(), x, y, color, 3, 15 );
	}
	else
	{
		drawImage(pos_global, sTrackCircleImage, color);
	}

	// clamp text position to on-screen
	const S32 TEXT_PADDING = DEFAULT_TRACKING_ARROW_SIZE + 2;
	S32 half_text_width = llfloor(font->getWidthF32(label) * 0.5f);
	text_x = llclamp(text_x, half_text_width + TEXT_PADDING, getRect().getWidth() - half_text_width - TEXT_PADDING);
	text_y = llclamp(text_y + vert_offset, TEXT_PADDING + vert_offset, getRect().getHeight() - font->getLineHeight() - TEXT_PADDING - vert_offset);

	if (label != "")
	{
		font->renderUTF8(
			label, 0,
			text_x, 
			text_y,
			LLColor4::white, LLFontGL::HCENTER,
			LLFontGL::BASELINE, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW);

		if (tooltip != "")
		{
			text_y -= font->getLineHeight();

			font->renderUTF8(
				tooltip, 0,
				text_x, 
				text_y,
				LLColor4::white, LLFontGL::HCENTER,
				LLFontGL::BASELINE, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW);
		}
	}
}

// If you change this, then you need to change LLTracker::getTrackedPositionGlobal() as well
LLVector3d LLWorldMapView::viewPosToGlobal( S32 x, S32 y )
{
	x -= llfloor((getRect().getWidth() / 2 + sPanX));
	y -= llfloor((getRect().getHeight() / 2 + sPanY));

	LLVector3 pos_local( (F32)x, (F32)y, 0.f );

	pos_local *= ( REGION_WIDTH_METERS / sMapScale );
	
	LLVector3d pos_global;
	pos_global.setVec( pos_local );
	pos_global += gAgentCamera.getCameraPositionGlobal();
	if(gAgent.isGodlike())
	{
		pos_global.mdV[VZ] = GODLY_TELEPORT_HEIGHT; // Godly height should always be 200.
	}
	else
	{
		pos_global.mdV[VZ] = gAgent.getPositionAgent().mV[VZ]; // Want agent's height, not camera's
	}

	return pos_global;
}


BOOL LLWorldMapView::handleToolTip( S32 x, S32 y, MASK mask )
{
	LLVector3d pos_global = viewPosToGlobal(x, y);
	U64 handle = to_region_handle(pos_global);
	std::string tooltip_msg;

	LLSimInfo* info = LLWorldMap::getInstance()->simInfoFromHandle(handle);
	if (info)
	{
		LLViewerRegion *region = gAgent.getRegion();

		std::string message = llformat("%s (%s)", info->getName().c_str(), info->getAccessString().c_str());

		if (!info->isDown())
		{
			S32 agent_count = info->getAgentCount();			
			if (region && (region->getHandle() == handle))
			{
				++agent_count; // Bump by 1 if we're here
			}

			// We may not have an agent count when the map is really
			// zoomed out, so don't display anything about the count. JC
			if (agent_count > 0)
			{
				LLStringUtil::format_map_t string_args;
				string_args["[NUMBER]"] = llformat("%d", agent_count);
				message += '\n';
				message += getString((agent_count == 1 ? "world_map_person" : "world_map_people") , string_args);
			}
		}
		tooltip_msg.assign( message );

		// Optionally show region flags
		std::string region_flags = info->getFlagsString();

		if (!region_flags.empty())
		{
			tooltip_msg += '\n';
			tooltip_msg += region_flags;
		}
					
		const S32 SLOP = 9;
		S32 screen_x, screen_y;

		localPointToScreen(x, y, &screen_x, &screen_y);
		LLRect sticky_rect_screen;
		sticky_rect_screen.setCenterAndSize(screen_x, screen_y, SLOP, SLOP);

		LLToolTipMgr::instance().show(LLToolTip::Params()
			.message(tooltip_msg)
			.sticky_rect(sticky_rect_screen));
	}
	return TRUE;
}

// Pass relative Z of 0 to draw at same level.
// static
static void drawDot(F32 x_pixels, F32 y_pixels,
			 const LLColor4& color,
			 F32 relative_z,
			 F32 dot_radius,
			 LLUIImagePtr dot_image)
{
	const F32 HEIGHT_THRESHOLD = 7.f;

	if(-HEIGHT_THRESHOLD <= relative_z && relative_z <= HEIGHT_THRESHOLD)
	{
		dot_image->draw(llround(x_pixels) - dot_image->getWidth()/2,
						llround(y_pixels) - dot_image->getHeight()/2, 
						color);
	}
	else
	{
		// Draw V indicator for above or below
		// *TODO: Replace this vector drawing with icons
		
		F32 left =		x_pixels - dot_radius;
		F32 right =		x_pixels + dot_radius;
		F32 center = (left + right) * 0.5f;
		F32 top =		y_pixels + dot_radius;
		F32 bottom =	y_pixels - dot_radius;

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4fv( color.mV );
		LLUI::setLineWidth(3.0f);
		F32 point = relative_z > HEIGHT_THRESHOLD ? top : bottom; // Y pos of the point of the V
		F32 back = relative_z > HEIGHT_THRESHOLD ? bottom : top; // Y pos of the ends of the V
		gGL.begin( LLRender::LINES );
			gGL.vertex2f(left, back);
			gGL.vertex2f(center, point);
			gGL.vertex2f(center, point);
			gGL.vertex2f(right, back);
		gGL.end();
		LLUI::setLineWidth(1.0f);
	}
}

// Pass relative Z of 0 to draw at same level.
// static
void LLWorldMapView::drawAvatar(F32 x_pixels, 
								F32 y_pixels,
								const LLColor4& color,
								F32 relative_z,
								F32 dot_radius,
								bool unknown_relative_z)
{
	const F32 HEIGHT_THRESHOLD = 7.f;
	LLUIImagePtr dot_image = sAvatarLevelImage;
	if (unknown_relative_z)
	{
		dot_image = sAvatarUnknownImage;
	}
	else
	{
		if(relative_z < -HEIGHT_THRESHOLD)
		{
			dot_image = sAvatarBelowImage; 
		}
		else if(relative_z > HEIGHT_THRESHOLD) 
		{ 
			dot_image = sAvatarAboveImage;
		}
	}
	S32 dot_width = llround(dot_radius * 2.f);
	dot_image->draw(llround(x_pixels - dot_radius),
					llround(y_pixels - dot_radius),
					dot_width,
					dot_width,
					color);
}

// Pass relative Z of 0 to draw at same level.
// static
void LLWorldMapView::drawTrackingDot( F32 x_pixels, 
									  F32 y_pixels,
									  const LLColor4& color,
									  F32 relative_z,
									  F32 dot_radius)
{
	drawDot(x_pixels, y_pixels, color, relative_z, dot_radius, sTrackCircleImage);
}


// Pass relative Z of 0 to draw at same level.
// static
void LLWorldMapView::drawIconName(F32 x_pixels, 
								  F32 y_pixels,
								  const LLColor4& color,
								  const std::string& first_line,
								  const std::string& second_line)
{
	const S32 VERT_PAD = 8;
	S32 text_x = llround(x_pixels);
	S32 text_y = llround(y_pixels
						 - BIG_DOT_RADIUS
						 - VERT_PAD);

	// render text
	LLFontGL::getFontSansSerif()->renderUTF8(first_line, 0,
		text_x,
		text_y,
		color,
		LLFontGL::HCENTER,
		LLFontGL::TOP,
		LLFontGL::NORMAL, 
		LLFontGL::DROP_SHADOW);

	text_y -= LLFontGL::getFontSansSerif()->getLineHeight();

	// render text
	LLFontGL::getFontSansSerif()->renderUTF8(second_line, 0,
		text_x,
		text_y,
		color,
		LLFontGL::HCENTER,
		LLFontGL::TOP,
		LLFontGL::NORMAL, 
		LLFontGL::DROP_SHADOW);
}


//static 
void LLWorldMapView::drawTrackingCircle( const LLRect& rect, S32 x, S32 y, const LLColor4& color, S32 min_thickness, S32 overlap )
{
	F32 start_theta = 0.f;
	F32 end_theta = F_TWO_PI;
	F32 x_delta = 0.f;
	F32 y_delta = 0.f;

	if (x < 0)
	{
		x_delta = 0.f - (F32)x;
		start_theta = F_PI + F_PI_BY_TWO;
		end_theta = F_TWO_PI + F_PI_BY_TWO;
	}
	else if (x > rect.getWidth())
	{
		x_delta = (F32)(x - rect.getWidth());
		start_theta = F_PI_BY_TWO;
		end_theta = F_PI + F_PI_BY_TWO;
	}

	if (y < 0)
	{
		y_delta = 0.f - (F32)y;
		if (x < 0)
		{
			start_theta = 0.f;
			end_theta = F_PI_BY_TWO;
		}
		else if (x > rect.getWidth())
		{
			start_theta = F_PI_BY_TWO;
			end_theta = F_PI;
		}
		else
		{
			start_theta = 0.f;
			end_theta = F_PI;
		}
	}
	else if (y > rect.getHeight())
	{
		y_delta = (F32)(y - rect.getHeight());
		if (x < 0)
		{
			start_theta = F_PI + F_PI_BY_TWO;
			end_theta = F_TWO_PI;
		}
		else if (x > rect.getWidth())
		{
			start_theta = F_PI;
			end_theta = F_PI + F_PI_BY_TWO;
		}
		else
		{
			start_theta = F_PI;
			end_theta = F_TWO_PI;
		}
	}

	F32 distance = sqrtf(x_delta * x_delta + y_delta * y_delta);

	distance = llmax(0.1f, distance);

	F32 outer_radius = distance + (1.f + (9.f * sqrtf(x_delta * y_delta) / distance)) * (F32)overlap;
	F32 inner_radius = outer_radius - (F32)min_thickness;
	
	F32 angle_adjust_x = asin(x_delta / outer_radius);
	F32 angle_adjust_y = asin(y_delta / outer_radius);

	if (angle_adjust_x)
	{
		if (angle_adjust_y)
		{
			F32 angle_adjust = llmin(angle_adjust_x, angle_adjust_y);
			start_theta += angle_adjust;
			end_theta -= angle_adjust;
		}
		else
		{
			start_theta += angle_adjust_x;
			end_theta -= angle_adjust_x;
		}
	}
	else if (angle_adjust_y)
	{
		start_theta += angle_adjust_y;
		end_theta -= angle_adjust_y;
	}

	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	gGL.translatef((F32)x, (F32)y, 0.f);
	gl_washer_segment_2d(inner_radius, outer_radius, start_theta, end_theta, 40, color, color);
	gGL.popMatrix();

}

// static
void LLWorldMapView::drawTrackingArrow(const LLRect& rect, S32 x, S32 y, 
									   const LLColor4& color,
									   S32 arrow_size)
{
	F32 x_center = (F32)rect.getWidth() / 2.f;
	F32 y_center = (F32)rect.getHeight() / 2.f;

	F32 x_clamped = (F32)llclamp( x, 0, rect.getWidth() - arrow_size );
	F32 y_clamped = (F32)llclamp( y, 0, rect.getHeight() - arrow_size );

	F32 slope = (F32)(y - y_center) / (F32)(x - x_center);
	F32 window_ratio = (F32)rect.getHeight() / (F32)rect.getWidth();

	if (llabs(slope) > window_ratio && y_clamped != (F32)y)
	{
		// clamp by y 
		x_clamped = (y_clamped - y_center) / slope + x_center;
		// adjust for arrow size
		x_clamped  = llclamp(x_clamped , 0.f, (F32)(rect.getWidth() - arrow_size) );
	}
	else if (x_clamped != (F32)x)
	{
		// clamp by x
		y_clamped = (x_clamped - x_center) * slope + y_center;
		// adjust for arrow size 
		y_clamped = llclamp( y_clamped, 0.f, (F32)(rect.getHeight() - arrow_size) );
	}

	// *FIX: deal with non-square window properly.
	// I do not understand what this comment means -- is it actually
	// broken or is it correctly dealing with non-square
	// windows. Phoenix 2007-01-03.
	S32 half_arrow_size = (S32) (0.5f * arrow_size);

	F32 angle = atan2( y + half_arrow_size - y_center, x + half_arrow_size - x_center);

	sTrackingArrowX = llfloor(x_clamped);
	sTrackingArrowY = llfloor(y_clamped);

	gl_draw_scaled_rotated_image(
		sTrackingArrowX,
		sTrackingArrowY,
		arrow_size, arrow_size, 
		RAD_TO_DEG * angle, 
		sTrackArrowImage->getImage(), 
		color);
}

void LLWorldMapView::setDirectionPos( LLTextBox* text_box, F32 rotation )
{
	// Rotation is in radians.
	// Rotation of 0 means x = 1, y = 0 on the unit circle.


	F32 map_half_height = getRect().getHeight() * 0.5f;
	F32 map_half_width = getRect().getWidth() * 0.5f;
	F32 text_half_height = text_box->getRect().getHeight() * 0.5f;
	F32 text_half_width = text_box->getRect().getWidth() * 0.5f;
	F32 radius = llmin( map_half_height - text_half_height, map_half_width - text_half_width );

	text_box->setOrigin( 
		llround(map_half_width - text_half_width + radius * cos( rotation )),
		llround(map_half_height - text_half_height + radius * sin( rotation )) );
}


void LLWorldMapView::updateDirections()
{
	S32 width = getRect().getWidth();
	S32 height = getRect().getHeight();

	S32 text_height = mTextBoxNorth->getRect().getHeight();
	S32 text_width = mTextBoxNorth->getRect().getWidth();

	const S32 PAD = 2;
	S32 top = height - text_height - PAD;
	S32 left = PAD*2;
	S32 bottom = PAD;
	S32 right = width - text_width - PAD;
	S32 center_x = width/2 - text_width/2;
	S32 center_y = height/2 - text_height/2;

	mTextBoxNorth->setOrigin( center_x, top );
	mTextBoxEast->setOrigin( right, center_y );
	mTextBoxSouth->setOrigin( center_x, bottom );
	mTextBoxWest->setOrigin( left, center_y );

	// These have wider text boxes
	text_width = mTextBoxNorthWest->getRect().getWidth();
	right = width - text_width - PAD;

	mTextBoxNorthWest->setOrigin(left, top);
	mTextBoxNorthEast->setOrigin(right, top);
	mTextBoxSouthWest->setOrigin(left, bottom);
	mTextBoxSouthEast->setOrigin(right, bottom);

// 	S32 hint_width = mTextBoxScrollHint->getRect().getWidth();
// 	mTextBoxScrollHint->setOrigin( width - hint_width - text_width - 2 * PAD, 
// 			PAD * 2 + text_height );
}


void LLWorldMapView::reshape( S32 width, S32 height, BOOL called_from_parent )
{
	LLView::reshape( width, height, called_from_parent );
}

bool LLWorldMapView::checkItemHit(S32 x, S32 y, LLItemInfo& item, LLUUID* id, bool track)
{
	LLVector3 pos_view = globalPosToView(item.getGlobalPosition());
	S32 item_x = llround(pos_view.mV[VX]);
	S32 item_y = llround(pos_view.mV[VY]);

	if (x < item_x - BIG_DOT_RADIUS) return false;
	if (x > item_x + BIG_DOT_RADIUS) return false;
	if (y < item_y - BIG_DOT_RADIUS) return false;
	if (y > item_y + BIG_DOT_RADIUS) return false;

	LLSimInfo* sim_info = LLWorldMap::getInstance()->simInfoFromHandle(item.getRegionHandle());
	if (sim_info)
	{
		if (track)
		{
			gFloaterWorldMap->trackLocation(item.getGlobalPosition());
		}
	}

	if (track)
	{
		gFloaterWorldMap->trackGenericItem(item);
	}

//	item.setSelected(true);
	*id = item.getUUID();

	return true;
}

// Handle a click, which might be on a dot
void LLWorldMapView::handleClick(S32 x, S32 y, MASK mask,
								 S32* hit_type,
								 LLUUID* id)
{
	LLVector3d pos_global = viewPosToGlobal(x, y);

	// *HACK: Adjust Z values automatically for liaisons & gods so
	// we swoop down when they click on the map. Sadly, the P2P
	// branch does not pay attention to this value; however, the
	// Distributed Messaging branch honors it.
	if(gAgent.isGodlike())
	{
		pos_global.mdV[VZ] = 200.0;
	}

	*hit_type = 0; // hit nothing

	LLWorldMap::getInstance()->cancelTracking();

	S32 level = LLWorldMipmap::scaleToLevel(sMapScale);
	// If the zoom level is not too far out already, test hits
	if (level <= DRAW_SIMINFO_THRESHOLD)
	{
		bool show_mature = gAgent.canAccessMature() && gSavedSettings.getBOOL("ShowMatureEvents");
		bool show_adult = gAgent.canAccessAdult() && gSavedSettings.getBOOL("ShowAdultEvents");

		// Test hits if trackable data are displayed, otherwise, we don't even bother
		if (gSavedSettings.getBOOL("MapShowEvents") || show_mature || show_adult || gSavedSettings.getBOOL("MapShowLandForSale"))
		{
			// Iterate through the visible regions
			for (handle_list_t::iterator iter = mVisibleRegions.begin(); iter != mVisibleRegions.end(); ++iter)
			{
				U64 handle = *iter;
				LLSimInfo* siminfo = LLWorldMap::getInstance()->simInfoFromHandle(handle);
				if ((siminfo == NULL) || (siminfo->isDown()))
				{
					continue;
				}
				// If on screen check hits with the visible item lists
				if (gSavedSettings.getBOOL("MapShowEvents"))
				{
					LLSimInfo::item_info_list_t::const_iterator it = siminfo->getPGEvent().begin();
					while (it != siminfo->getPGEvent().end())
					{
						LLItemInfo event = *it;
						if (checkItemHit(x, y, event, id, false))
						{
							*hit_type = MAP_ITEM_PG_EVENT;
							mItemPicked = TRUE;
							gFloaterWorldMap->trackEvent(event);
							return;
						}
						++it;
					}
				}
				if (show_mature)
				{
					LLSimInfo::item_info_list_t::const_iterator it = siminfo->getMatureEvent().begin();
					while (it != siminfo->getMatureEvent().end())
					{
						LLItemInfo event = *it;
						if (checkItemHit(x, y, event, id, false))
						{
							*hit_type = MAP_ITEM_MATURE_EVENT;
							mItemPicked = TRUE;
							gFloaterWorldMap->trackEvent(event);
							return;
						}
						++it;
					}
				}
				if (show_adult)
				{
					LLSimInfo::item_info_list_t::const_iterator it = siminfo->getAdultEvent().begin();
					while (it != siminfo->getAdultEvent().end())
					{
						LLItemInfo event = *it;
						if (checkItemHit(x, y, event, id, false))
						{
							*hit_type = MAP_ITEM_ADULT_EVENT;
							mItemPicked = TRUE;
							gFloaterWorldMap->trackEvent(event);
							return;
						}
						++it;
					}
				}
				if (gSavedSettings.getBOOL("MapShowLandForSale"))
				{
					LLSimInfo::item_info_list_t::const_iterator it = siminfo->getLandForSale().begin();
					while (it != siminfo->getLandForSale().end())
					{
						LLItemInfo event = *it;
						if (checkItemHit(x, y, event, id, true))
						{
							*hit_type = MAP_ITEM_LAND_FOR_SALE;
							mItemPicked = TRUE;
							return;
						}
						++it;
					}
					// for 1.23, we're showing normal land and adult land in the same UI; you don't
					// get a choice about which ones you want. If you're currently asking for adult
					// content and land you'll get the adult land.
					if (gAgent.canAccessAdult())
					{
						LLSimInfo::item_info_list_t::const_iterator it = siminfo->getLandForSaleAdult().begin();
						while (it != siminfo->getLandForSaleAdult().end())
						{
							LLItemInfo event = *it;
							if (checkItemHit(x, y, event, id, true))
							{
								*hit_type = MAP_ITEM_LAND_FOR_SALE_ADULT;
								mItemPicked = TRUE;
								return;
							}
							++it;
						}
					}
				}
			}
		}
	}

	// If we get here, we haven't clicked on anything
	gFloaterWorldMap->trackLocation(pos_global);
	mItemPicked = FALSE;
	*id = LLUUID::null;
	return;
}


BOOL outside_slop(S32 x, S32 y, S32 start_x, S32 start_y)
{
	S32 dx = x - start_x;
	S32 dy = y - start_y;

	return (dx <= -2 || 2 <= dx || dy <= -2 || 2 <= dy);
}

BOOL LLWorldMapView::handleMouseDown( S32 x, S32 y, MASK mask )
{
	gFocusMgr.setMouseCapture( this );

	mMouseDownPanX = llround(sPanX);
	mMouseDownPanY = llround(sPanY);
	mMouseDownX = x;
	mMouseDownY = y;
	sHandledLastClick = TRUE;
	return TRUE;
}

BOOL LLWorldMapView::handleMouseUp( S32 x, S32 y, MASK mask )
{
	if (hasMouseCapture())
	{
		if (mPanning)
		{
			// restore mouse cursor
			S32 local_x, local_y;
			local_x = mMouseDownX + llfloor(sPanX - mMouseDownPanX);
			local_y = mMouseDownY + llfloor(sPanY - mMouseDownPanY);
			LLRect clip_rect = getRect();
			clip_rect.stretch(-8);
			clip_rect.clipPointToRect(mMouseDownX, mMouseDownY, local_x, local_y);
			LLUI::setMousePositionLocal(this, local_x, local_y);

			// finish the pan
			mPanning = FALSE;
			
			mMouseDownX = 0;
			mMouseDownY = 0;
		}
		else
		{
			// ignore whether we hit an event or not
			S32 hit_type;
			LLUUID id;
			handleClick(x, y, mask, &hit_type, &id);
		}
		gViewerWindow->showCursor();
		gFocusMgr.setMouseCapture( NULL );
		return TRUE;
	}
	return FALSE;
}

void LLWorldMapView::updateVisibleBlocks()
{
	if (LLWorldMipmap::scaleToLevel(sMapScale) > DRAW_SIMINFO_THRESHOLD)
	{
		// If we're zoomed out too much, we just don't load all those sim info: too much!
		return;
	}

	// Load the blocks visible in the current World Map view

	// Get the World Map view coordinates and boundaries
	LLVector3d camera_global = gAgentCamera.getCameraPositionGlobal();
	const S32 width = getRect().getWidth();
	const S32 height = getRect().getHeight();
	const F32 half_width = F32(width) / 2.0f;
	const F32 half_height = F32(height) / 2.0f;

	// Compute center into sim grid coordinates
	S32 world_center_x = S32((-sPanX / sMapScale) + (camera_global.mdV[0] / REGION_WIDTH_METERS));
	S32 world_center_y = S32((-sPanY / sMapScale) + (camera_global.mdV[1] / REGION_WIDTH_METERS));

	// Compute the boundaries into sim grid coordinates
	S32 world_left   = world_center_x - S32(half_width  / sMapScale) - 1;
	S32 world_right  = world_center_x + S32(half_width  / sMapScale) + 1;
	S32 world_bottom = world_center_y - S32(half_height / sMapScale) - 1;
	S32 world_top    = world_center_y + S32(half_height / sMapScale) + 1;

	//LL_INFOS("World Map") << "LLWorldMapView::updateVisibleBlocks() : sMapScale = " << sMapScale << ", left = " << world_left << ", right = " << world_right << ", bottom  = " << world_bottom << ", top = " << world_top << LL_ENDL;
	LLWorldMap::getInstance()->updateRegions(world_left, world_bottom, world_right, world_top);
}

BOOL LLWorldMapView::handleHover( S32 x, S32 y, MASK mask )
{
	if (hasMouseCapture())
	{
		if (mPanning || outside_slop(x, y, mMouseDownX, mMouseDownY))
		{
			// just started panning, so hide cursor
			if (!mPanning)
			{
				mPanning = TRUE;
				gViewerWindow->hideCursor();
			}

			F32 delta_x = (F32)(gViewerWindow->getCurrentMouseDX());
			F32 delta_y = (F32)(gViewerWindow->getCurrentMouseDY());

			// Set pan to value at start of drag + offset
			sPanX += delta_x;
			sPanY += delta_y;
			sTargetPanX = sPanX;
			sTargetPanY = sPanY;

			gViewerWindow->moveCursorToCenter();
		}

		// doesn't matter, cursor should be hidden
		gViewerWindow->setCursor(UI_CURSOR_CROSS );
		return TRUE;
	}
	else
	{
		// While we're waiting for data from the tracker, we're busy. JC
		LLVector3d pos_global = LLTracker::getTrackedPositionGlobal();
		if (LLTracker::isTracking(NULL)
			&& pos_global.isExactlyZero())
		{
			gViewerWindow->setCursor( UI_CURSOR_WAIT );
		}
		else
		{
			gViewerWindow->setCursor( UI_CURSOR_CROSS );
		}
		lldebugst(LLERR_USER_INPUT) << "hover handled by LLWorldMapView" << llendl;		
		return TRUE;
	}
}


BOOL LLWorldMapView::handleDoubleClick( S32 x, S32 y, MASK mask )
{
	if( sHandledLastClick )
	{
		S32 hit_type;
		LLUUID id;
		handleClick(x, y, mask, &hit_type, &id);

		switch (hit_type)
		{
		case MAP_ITEM_PG_EVENT:
		case MAP_ITEM_MATURE_EVENT:
		case MAP_ITEM_ADULT_EVENT:
			{
				LLFloaterReg::hideInstance("world_map");
				// This is an ungainly hack
				std::string uuid_str;
				S32 event_id;
				id.toString(uuid_str);
				uuid_str = uuid_str.substr(28);
				sscanf(uuid_str.c_str(), "%X", &event_id);
				// Invoke the event details floater if someone is clicking on an event.
				LLSD params(LLSD::emptyArray());
				params.append(event_id);
				LLCommandDispatcher::dispatch("event", params, LLSD(), NULL, "clicked", true);
				break;
			}
		case MAP_ITEM_LAND_FOR_SALE:
		case MAP_ITEM_LAND_FOR_SALE_ADULT:
			{
				LLFloaterReg::hideInstance("world_map");
				LLFloaterReg::showInstance("search", LLSD().with("category", "destinations").with("query", id));
				break;
			}
		case MAP_ITEM_CLASSIFIED:
			{
				LLFloaterReg::hideInstance("world_map");
				LLFloaterReg::showInstance("search", LLSD().with("category", "classifieds").with("query", id));
				break;
			}
		default:
			{
				if (LLWorldMap::getInstance()->isTracking())
				{
					LLWorldMap::getInstance()->setTrackingDoubleClick();
				}
				else
				{
					// Teleport if we got a valid location
					LLVector3d pos_global = viewPosToGlobal(x,y);
					LLSimInfo* sim_info = LLWorldMap::getInstance()->simInfoFromPosGlobal(pos_global);
					if (sim_info && !sim_info->isDown())
					{
						gAgent.teleportViaLocation( pos_global );
					}
				}
			}
		};

		return TRUE;
	}
	return FALSE;
}
