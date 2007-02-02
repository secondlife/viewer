/** 
 * @file llworldmapview.cpp
 * @brief LLWorldMapView class implementation
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llworldmapview.h"

#include "indra_constants.h"
#include "llui.h"
#include "linked_lists.h"
#include "llmath.h"		// clampf()
#include "llregionhandle.h"
#include "lleventflags.h"

#include "llagent.h"
#include "llcallingcard.h"
#include "llcolorscheme.h"
#include "llviewercontrol.h"
#include "llcylinder.h"
#include "llfloaterdirectory.h"
#include "llfloatermap.h"
#include "llfloaterworldmap.h"
#include "llfocusmgr.h"
#include "lltextbox.h"
#include "lltextureview.h"
#include "lltracker.h"
#include "llviewercamera.h"
#include "llviewerimage.h"
#include "llviewerimagelist.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llworld.h"
#include "llworldmap.h"
#include "viewer.h"				// Only for constants!

#include "llglheaders.h"

const F32 GODLY_TELEPORT_HEIGHT = 200.f;
const S32 SCROLL_HINT_WIDTH = 65;
const F32 BIG_DOT_RADIUS = 5.f;
BOOL LLWorldMapView::sHandledLastClick = FALSE;

LLPointer<LLViewerImage> LLWorldMapView::sAvatarYouSmallImage = NULL;
LLPointer<LLViewerImage> LLWorldMapView::sAvatarSmallImage = NULL;
LLPointer<LLViewerImage> LLWorldMapView::sAvatarLargeImage = NULL;

LLPointer<LLViewerImage> LLWorldMapView::sTelehubImage = NULL;
LLPointer<LLViewerImage> LLWorldMapView::sInfohubImage = NULL;
LLPointer<LLViewerImage> LLWorldMapView::sHomeImage = NULL;
LLPointer<LLViewerImage> LLWorldMapView::sEventImage = NULL;
LLPointer<LLViewerImage> LLWorldMapView::sEventMatureImage = NULL;

LLPointer<LLViewerImage> LLWorldMapView::sTrackCircleImage = NULL;
LLPointer<LLViewerImage> LLWorldMapView::sTrackArrowImage = NULL;

LLPointer<LLViewerImage> LLWorldMapView::sClassifiedsImage = NULL;
LLPointer<LLViewerImage> LLWorldMapView::sPopularImage = NULL;
LLPointer<LLViewerImage> LLWorldMapView::sForSaleImage = NULL;

F32 LLWorldMapView::sThresholdA = 48.f;
F32 LLWorldMapView::sThresholdB = 96.f;
F32 LLWorldMapView::sPanX = 0.f;
F32 LLWorldMapView::sPanY = 0.f;
F32 LLWorldMapView::sTargetPanX = 0.f;
F32 LLWorldMapView::sTargetPanY = 0.f;
S32 LLWorldMapView::sTrackingArrowX = 0;
S32 LLWorldMapView::sTrackingArrowY = 0;
F32 LLWorldMapView::sPixelsPerMeter = 1.f;
F32 CONE_SIZE = 0.6f;

void LLWorldMapView::initClass()
{
	LLUUID image_id;
	
	image_id.set( gViewerArt.getString("map_avatar_you_8.tga") );
	sAvatarYouSmallImage = gImageList.getImage( image_id, MIPMAP_FALSE, TRUE);

	image_id.set( gViewerArt.getString("map_avatar_8.tga") );
	sAvatarSmallImage = gImageList.getImage( image_id, MIPMAP_FALSE, TRUE);

	image_id.set( gViewerArt.getString("map_avatar_16.tga") );
	sAvatarLargeImage = gImageList.getImage( image_id, MIPMAP_FALSE, TRUE);

	image_id.set( gViewerArt.getString("map_home.tga") );
	sHomeImage = gImageList.getImage(image_id, MIPMAP_FALSE, TRUE);

	image_id.set( gViewerArt.getString("map_telehub.tga") );
	sTelehubImage = gImageList.getImage(image_id, MIPMAP_FALSE, TRUE);

	image_id.set( gViewerArt.getString("map_infohub.tga") );
	sInfohubImage = gImageList.getImage(image_id, MIPMAP_FALSE, TRUE);

	image_id.set( gViewerArt.getString("map_event.tga") );
	sEventImage = gImageList.getImage(image_id, MIPMAP_FALSE, TRUE);

	image_id.set( gViewerArt.getString("map_event_mature.tga") );
	sEventMatureImage = gImageList.getImage(image_id, MIPMAP_FALSE, TRUE);

	image_id.set( gViewerArt.getString("map_track_16.tga") );
	sTrackCircleImage = gImageList.getImage(image_id, MIPMAP_FALSE, TRUE);

	image_id.set( gViewerArt.getString("direction_arrow.tga") );
	sTrackArrowImage = gImageList.getImage(image_id, MIPMAP_FALSE, TRUE);
	// Make sure tracker arrow doesn't wrap
	sTrackArrowImage->bindTexture(0);
	sTrackArrowImage->setClamp(TRUE, TRUE);

	image_id.set( gViewerArt.getString("icon_top_pick.tga") );
	sClassifiedsImage = gImageList.getImage(image_id, MIPMAP_FALSE, TRUE);

	image_id.set( gViewerArt.getString("icon_popular.tga") );
	sPopularImage = gImageList.getImage(image_id, MIPMAP_FALSE, TRUE);

	image_id.set( gViewerArt.getString("icon_for_sale.tga") );
	sForSaleImage = gImageList.getImage(image_id, MIPMAP_FALSE, TRUE);
}

// static
void LLWorldMapView::cleanupClass()
{
	sAvatarYouSmallImage = NULL;
	sAvatarSmallImage = NULL;
	sAvatarLargeImage = NULL;
	sTelehubImage = NULL;
	sInfohubImage = NULL;
	sHomeImage = NULL;
	sEventImage = NULL;
	sEventMatureImage = NULL;
	sTrackCircleImage = NULL;
	sTrackArrowImage = NULL;
	sClassifiedsImage = NULL;
	sPopularImage = NULL;
	sForSaleImage = NULL;
}

LLWorldMapView::LLWorldMapView(const std::string& name, const LLRect& rect )
:	LLPanel(name, rect, BORDER_NO), 
	mBackgroundColor( LLColor4( 4.f/255.f, 4.f/255.f, 75.f/255.f, 1.f ) ),
	mItemPicked(FALSE),
	mPanning( FALSE ),
	mMouseDownPanX( 0 ),
	mMouseDownPanY( 0 ),
	mMouseDownX( 0 ),
	mMouseDownY( 0 ),
	mSelectIDStart(0),
	mAgentCountsUpdateTime(0)
{
	sPixelsPerMeter = gMapScale / REGION_WIDTH_METERS;
	clearLastClick();

	const S32 DIR_WIDTH = 10;
	const S32 DIR_HEIGHT = 10;
	LLRect major_dir_rect(  0, DIR_HEIGHT, DIR_WIDTH, 0 );

	mTextBoxNorth = new LLTextBox( "N", major_dir_rect );
	mTextBoxNorth->setDropshadowVisible( TRUE );
	addChild( mTextBoxNorth );

	LLColor4 minor_color( 1.f, 1.f, 1.f, .7f );
	
	mTextBoxEast =	new LLTextBox( "E", major_dir_rect );
	mTextBoxEast->setColor( minor_color );
	addChild( mTextBoxEast );
	
	mTextBoxWest =	new LLTextBox( "W", major_dir_rect );
	mTextBoxWest->setColor( minor_color );
	addChild( mTextBoxWest );

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
}


LLWorldMapView::~LLWorldMapView()
{
	cleanupTextures();
}


// static
void LLWorldMapView::cleanupTextures()
{
}


// static
void LLWorldMapView::setScale( F32 scale )
{
	if (scale != gMapScale)
	{
		F32 old_scale = gMapScale;

		gMapScale = scale;
		if (gMapScale == 0.f)
		{
			gMapScale = 0.1f;
		}

		F32 ratio = (scale / old_scale);
		sPanX *= ratio;
		sPanY *= ratio;
		sTargetPanX = sPanX;
		sTargetPanY = sPanY;

		sPixelsPerMeter = gMapScale / REGION_WIDTH_METERS;
	}
}


// static
void LLWorldMapView::translatePan( S32 delta_x, S32 delta_y )
{
	sPanX += delta_x;
	sPanY += delta_y;
	sTargetPanX = sPanX;
	sTargetPanY = sPanY;
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
}


///////////////////////////////////////////////////////////////////////////////////

// dumb helper function
BOOL is_agent_in_region(LLViewerRegion* region, LLSimInfo* info)
{
	if((region && info)
	   && (info->mName == region->getName()))
	{
		return TRUE;
	}
	return FALSE;
}

void LLWorldMapView::draw()
{
	if (!getVisible() || !gWorldPointer)
	{
		return;
	}

	LLTextureView::clearDebugImages();

	F64 current_time = LLTimer::getElapsedSeconds();

	if (current_time - mAgentCountsUpdateTime > AGENT_COUNTS_UPDATE_TIME)
	{
		gWorldMap->sendItemRequest(MAP_ITEM_AGENT_COUNT);
		mAgentCountsUpdateTime = current_time;
	}

	mVisibleRegions.clear();
	
	// animate pan if necessary
	sPanX = lerp(sPanX, sTargetPanX, LLCriticalDamp::getInterpolant(0.1f));
	sPanY = lerp(sPanY, sTargetPanY, LLCriticalDamp::getInterpolant(0.1f));

	LLVector3d pos_global;
	LLVector3 pos_map;

	const S32 width = mRect.getWidth();
	const S32 height = mRect.getHeight();
	const S32 half_width = width / 2;
	const S32 half_height = height / 2;
	LLVector3d camera_global = gAgent.getCameraPositionGlobal();

	LLGLEnable scissor_test(GL_SCISSOR_TEST);
	LLUI::setScissorRegionLocal(LLRect(0, height, width, 0));
	{
		LLGLSNoTexture no_texture;
	

		glMatrixMode(GL_MODELVIEW);

		// Clear the background alpha to 0
		glColorMask(FALSE, FALSE, FALSE, TRUE);
		glAlphaFunc(GL_GEQUAL, 0.00f);
		glBlendFunc(GL_ONE, GL_ZERO);
		glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
		gl_rect_2d(0, height, width, 0);
	}

	glAlphaFunc(GL_GEQUAL, 0.01f);
	glColorMask(TRUE, TRUE, TRUE, TRUE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	F32 layer_alpha = 1.f;

	// Draw one image per layer
	for (U32 layer_idx=0; layer_idx<gWorldMap->mMapLayers[gWorldMap->mCurrentMap].size(); ++layer_idx)
	{
		if (!gWorldMap->mMapLayers[gWorldMap->mCurrentMap][layer_idx].LayerDefined)
		{
			continue;
		}
		LLWorldMapLayer *layer = &gWorldMap->mMapLayers[gWorldMap->mCurrentMap][layer_idx];
		LLViewerImage *current_image = layer->LayerImage;
#if 1 || LL_RELEASE_FOR_DOWNLOAD
		if (current_image->isMissingAsset())
		{
			continue; // better to draw nothing than the missing asset image
		}
#endif
		
		LLVector3d origin_global((F64)layer->LayerExtents.mLeft * REGION_WIDTH_METERS, (F64)layer->LayerExtents.mBottom * REGION_WIDTH_METERS, 0.f);

		// Find x and y position relative to camera's center.
		LLVector3d rel_region_pos = origin_global - camera_global;
		S32 relative_x = lltrunc((rel_region_pos.mdV[0] / REGION_WIDTH_METERS) * gMapScale);
		S32 relative_y = lltrunc((rel_region_pos.mdV[1] / REGION_WIDTH_METERS) * gMapScale);

		F32 pix_width = gMapScale*(layer->LayerExtents.getWidth() + 1);
		F32 pix_height = gMapScale*(layer->LayerExtents.getHeight() + 1);

		// When the view isn't panned, 0,0 = center of rectangle
		F32 bottom =	sPanY + half_height + relative_y;
		F32 left =		sPanX + half_width + relative_x;
		F32 top =		bottom + pix_height;
		F32 right =		left + pix_width;
		F32 pixel_area = pix_width*pix_height;
		// discard layers that are outside the rectangle
		// and discard small layers
		if (top < 0.f ||
			bottom > height ||
			right < 0.f ||
			left > width ||
			(pixel_area < 4*4))
		{
			current_image->setBoostLevel(0);
			continue;
		}
		
		current_image->setBoostLevel(LLViewerImage::BOOST_MAP_LAYER);
		current_image->setKnownDrawSize(llround(pix_width), llround(pix_height));
		
#if 1 || LL_RELEASE_FOR_DOWNLOAD
		if (!current_image->getHasGLTexture())
		{
			continue; // better to draw nothing than the default image
		}
#endif

		LLTextureView::addDebugImage(current_image);
		
		// Draw using the texture.  If we don't clamp we get artifact at
		// the edge.
		LLViewerImage::bindTexture(current_image);

		// Draw map image into RGB
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColorMask(TRUE, TRUE, TRUE, FALSE);
		glColor4f(1.f, 1.f, 1.f, layer_alpha);

		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(left, top, -1.0f);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(left, bottom, -1.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(right, bottom, -1.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(right, top, -1.0f);
		glEnd();

		// draw an alpha of 1 where the sims are visible
		glColorMask(FALSE, FALSE, FALSE, TRUE);
		glColor4f(1.f, 1.f, 1.f, 1.f);

		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f);
			glVertex2f(left, top);
			glTexCoord2f(0.0f, 0.0f);
			glVertex2f(left, bottom);
			glTexCoord2f(1.0f, 0.0f);
			glVertex2f(right, bottom);
			glTexCoord2f(1.0f, 1.0f);
			glVertex2f(right, top);
		glEnd();
	}

	glAlphaFunc(GL_GEQUAL, 0.01f);
	glColorMask(TRUE, TRUE, TRUE, TRUE);

#if 1
	F32 sim_alpha = 1.f;

	// Draw one image per region, centered on the camera position.
	for (LLWorldMap::sim_info_map_t::iterator it = gWorldMap->mSimInfoMap.begin();
		 it != gWorldMap->mSimInfoMap.end(); ++it)
	{
		U64 handle = (*it).first;
		LLSimInfo* info = (*it).second;

		if (info->mCurrentImage.isNull())
		{
			info->mCurrentImage = gImageList.getImage(info->mMapImageID[gWorldMap->mCurrentMap], MIPMAP_TRUE, FALSE);
		}
		if (info->mOverlayImage.isNull() && info->mMapImageID[2].notNull())
		{
			info->mOverlayImage = gImageList.getImage(info->mMapImageID[2], MIPMAP_TRUE, FALSE);
			info->mOverlayImage->bind(0);
			info->mOverlayImage->setClamp(TRUE, TRUE);
		}
		
		LLViewerImage* simimage = info->mCurrentImage;
		LLViewerImage* overlayimage = info->mOverlayImage;

		if (gMapScale < 8.f)
		{
			simimage->setBoostLevel(0);
			if (overlayimage) overlayimage->setBoostLevel(0);
			continue;
		}
		
		LLVector3d origin_global = from_region_handle(handle);
		LLVector3d camera_global = gAgent.getCameraPositionGlobal();

		// Find x and y position relative to camera's center.
		LLVector3d rel_region_pos = origin_global - camera_global;
		S32 relative_x = lltrunc((rel_region_pos.mdV[0] / REGION_WIDTH_METERS) * gMapScale);
		S32 relative_y = lltrunc((rel_region_pos.mdV[1] / REGION_WIDTH_METERS) * gMapScale);

		// When the view isn't panned, 0,0 = center of rectangle
		F32 bottom =	sPanY + half_height + relative_y;
		F32 left =		sPanX + half_width + relative_x;
		F32 top =		bottom + gMapScale ;
		F32 right =		left + gMapScale ;

		// Switch to world map texture (if available for this region) if either:
		// 1. Tiles are zoomed out small enough, or
		// 2. Sim's texture has not been loaded yet
		F32 map_scale_cutoff = SIM_MAP_SCALE;
		if ((info->mRegionFlags & REGION_FLAGS_NULL_LAYER) > 0)
		{
			map_scale_cutoff = SIM_NULL_MAP_SCALE;
		}

		info->mShowAgentLocations = (gMapScale >= SIM_MAP_AGENT_SCALE);

		bool sim_visible =
			(gMapScale >= map_scale_cutoff) &&
			(simimage->getHasGLTexture());

		if (sim_visible)
		{
			// Fade in
			if (info->mAlpha < 0.0f)
				info->mAlpha = 1.f; // don't fade initially
			else
				info->mAlpha = lerp(info->mAlpha, 1.f, LLCriticalDamp::getInterpolant(0.15f));
		}
		else
		{
			// Fade out
			if (info->mAlpha < 0.0f)
				info->mAlpha = 0.f; // don't fade initially
			else
				info->mAlpha = lerp(info->mAlpha, 0.f, LLCriticalDamp::getInterpolant(0.15f));
		}

		// discard regions that are outside the rectangle
		// and discard small regions
		if (top < 0.f ||
			bottom > height ||
			right < 0.f ||
			left > width )
		{
			simimage->setBoostLevel(0);
			if (overlayimage) overlayimage->setBoostLevel(0);
			continue;
		}

		mVisibleRegions.push_back(handle);
		// See if the agents need updating
		if (current_time - info->mAgentsUpdateTime > AGENTS_UPDATE_TIME)
		{
			gWorldMap->sendItemRequest(MAP_ITEM_AGENT_LOCATIONS, info->mHandle);
			info->mAgentsUpdateTime = current_time;
		}
		
		// Bias the priority escalation for images nearer
		LLVector3d center_global = origin_global;
		center_global.mdV[VX] += 128.0;
		center_global.mdV[VY] += 128.0;

		S32 draw_size = llround(gMapScale);
		simimage->setBoostLevel(LLViewerImage::BOOST_MAP);
		simimage->setKnownDrawSize(draw_size, draw_size);

		if (overlayimage)
		{
			overlayimage->setBoostLevel(LLViewerImage::BOOST_MAP);
			overlayimage->setKnownDrawSize(draw_size, draw_size);
		}
			
		LLTextureView::addDebugImage(simimage);

		if (sim_visible && info->mAlpha > 0.001f)
		{
			// Draw using the texture.  If we don't clamp we get artifact at
			// the edge.
			LLGLSUIDefault gls_ui;
			LLViewerImage::bindTexture(simimage);

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			F32 alpha = sim_alpha * info->mAlpha;
			glColor4f(1.f, 1.0f, 1.0f, alpha);

			glBegin(GL_QUADS);
				glTexCoord2f(0.f, 1.f);
				glVertex3f(left, top, 0.f);
				glTexCoord2f(0.f, 0.f);
				glVertex3f(left, bottom, 0.f);
				glTexCoord2f(1.f, 0.f);
				glVertex3f(right, bottom, 0.f);
				glTexCoord2f(1.f, 1.f);
				glVertex3f(right, top, 0.f);
			glEnd();

			if (gSavedSettings.getBOOL("MapShowLandForSale") && overlayimage && overlayimage->getHasGLTexture())
			{
				LLViewerImage::bindTexture(overlayimage);
				glColor4f(1.f, 1.f, 1.f, alpha);
				glBegin(GL_QUADS);
					glTexCoord2f(0.f, 1.f);
					glVertex3f(left, top, -0.5f);
					glTexCoord2f(0.f, 0.f);
					glVertex3f(left, bottom, -0.5f);
					glTexCoord2f(1.f, 0.f);
					glVertex3f(right, bottom, -0.5f);
					glTexCoord2f(1.f, 1.f);
					glVertex3f(right, top, -0.5f);
				glEnd();
			}
			
			if ((info->mRegionFlags & REGION_FLAGS_NULL_LAYER) == 0)
			{
				// draw an alpha of 1 where the sims are visible (except NULL sims)
				glBlendFunc(GL_ONE, GL_ZERO);
				glColorMask(FALSE, FALSE, FALSE, TRUE);
				glColor4f(1.f, 1.f, 1.f, 1.f);

				LLGLSNoTexture gls_no_texture;
				glBegin(GL_QUADS);
					glVertex2f(left, top);
					glVertex2f(left, bottom);
					glVertex2f(right, bottom);
					glVertex2f(right, top);
				glEnd();

				glColorMask(TRUE, TRUE, TRUE, TRUE);
			}
		}

		if (info->mAccess == SIM_ACCESS_DOWN)
		{
			// Draw a transparent red square over down sims
			glBlendFunc(GL_DST_ALPHA, GL_SRC_ALPHA);
			glColor4f(0.2f, 0.0f, 0.0f, 0.4f);

			LLGLSNoTexture gls_no_texture;
			glBegin(GL_QUADS);
				glVertex2f(left, top);
				glVertex2f(left, bottom);
				glVertex2f(right, bottom);
				glVertex2f(right, top);
			glEnd();
		}

		// If this is mature, and you are not, draw a line across it
		if (info->mAccess != SIM_ACCESS_DOWN && info->mAccess > gAgent.mAccess)
		{
			glBlendFunc(GL_DST_ALPHA, GL_ZERO);
			
			LLGLSNoTexture gls_no_texture;
			glColor3f(1.f, 0.f, 0.f);
			glBegin(GL_LINES);
				glVertex2f(left, top);
				glVertex2f(right, bottom);
				glVertex2f(left, bottom);
				glVertex2f(right, top);
			glEnd();
		}

		// Draw the region name in the lower left corner
		LLFontGL* font = LLFontGL::sSansSerifSmall;

		char mesg[MAX_STRING];		/* Flawfinder: ignore */
		if (gMapScale < sThresholdA)
		{
			mesg[0] = '\0';
		}
		else if (gMapScale < sThresholdB)
		{
			//sprintf(mesg, "%d", info->mAgents);
			mesg[0] = '\0';
		}
		else
		{
			//sprintf(mesg, "%d / %s (%s)",
			//			info->mAgents,
			//			info->mName.c_str(),
			//			LLViewerRegion::accessToShortString(info->mAccess) );
			if (info->mAccess == SIM_ACCESS_DOWN)
			{
				snprintf(mesg, MAX_STRING, "%s (Offline)", info->mName.c_str());		/* Flawfinder: ignore */
			}
			else
			{
				snprintf(mesg, MAX_STRING, "%s", info->mName.c_str());		/* Flawfinder: ignore */
			}
		}

		if (mesg[0] != '\0')
		{
			font->renderUTF8(
				mesg, 0,
				llfloor(left + 3), 
				llfloor(bottom + 2),
				LLColor4::white,
				LLFontGL::LEFT,
				LLFontGL::BASELINE,
				LLFontGL::DROP_SHADOW);
		}
	}
#endif


#if 1
	// Draw background rectangle
	LLGLSUIDefault gls_ui;
	{
		LLGLSNoTexture gls_no_texture;
		glAlphaFunc(GL_GEQUAL, 0.0f);
		glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
		glColor4fv( mBackgroundColor.mV );
		gl_rect_2d(0, height, width, 0);
	}
	
	glAlphaFunc(GL_GEQUAL, 0.01f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Infohubs
	// Draw under avatar so you can find yourself.
	if (gSavedSettings.getBOOL("MapShowInfohubs"))   //(gMapScale >= sThresholdB)
	{
		S32 infohub_icon_size = (S32)sInfohubImage->getWidth();
		for (U32 infohub=0; infohub<gWorldMap->mInfohubs.size(); ++infohub)
		{
			LLItemInfo *infohub_info = &gWorldMap->mInfohubs[infohub];

			pos_map = globalPosToView(infohub_info->mPosGlobal);
			gl_draw_image(llround(pos_map.mV[VX])-infohub_icon_size/2,
							llround(pos_map.mV[VY])-infohub_icon_size/2,
							sInfohubImage,
							LLColor4::white);
		}
	}

	// Telehubs
	// Draw under avatar so you can find yourself.
	if (gSavedSettings.getBOOL("MapShowTelehubs"))   //(gMapScale >= sThresholdB)
	{
		S32 telehub_icon_size = (S32)sTelehubImage->getWidth();
		for (U32 telehub=0; telehub<gWorldMap->mTelehubs.size(); ++telehub)
		{
			LLItemInfo *telehub_info = &gWorldMap->mTelehubs[telehub];

			pos_map = globalPosToView(telehub_info->mPosGlobal);
			gl_draw_image(llround(pos_map.mV[VX])-telehub_icon_size/2,
					llround(pos_map.mV[VY])-telehub_icon_size/2,
					sTelehubImage,
					LLColor4::white);
		}
	}

	// Home
	// Always Draw
	if (1)   //(gMapScale >= sThresholdB)
	{
		S32 home_icon_size = (S32)sHomeImage->getWidth();
		if (gAgent.getHomePosGlobal(&pos_global))
		{
			pos_map = globalPosToView(pos_global);
			gl_draw_image(llround(pos_map.mV[VX])-home_icon_size/2,
						  llround(pos_map.mV[VY])-home_icon_size/2,
						  sHomeImage,
						  LLColor4::white);
		}
	}

	// Draw all these under the avatar, so you can find yourself

	if (gSavedSettings.getBOOL("MapShowLandForSale"))
	{
		drawGenericItems(gWorldMap->mLandForSale, sForSaleImage);
	}

	if (gSavedSettings.getBOOL("MapShowClassifieds"))
	{
		drawGenericItems(gWorldMap->mClassifieds, sClassifiedsImage);
	}

	if (gSavedSettings.getBOOL("MapShowPopular"))
	{
		drawGenericItems(gWorldMap->mPopular, sPopularImage);
	}
	
	if (gSavedSettings.getBOOL("MapShowEvents"))
	{
		drawEvents();
	}

	// Your avatar.
	// Draw avatar position, always, and underneath other avatars
	pos_global = gAgent.getPositionGlobal();
	pos_map = globalPosToView(pos_global);
	gl_draw_image(llround(pos_map.mV[VX])-8,
				  llround(pos_map.mV[VY])-8,
				  sAvatarLargeImage,
				  LLColor4::white);

	if (!pointInView(llround(pos_map.mV[VX]), llround(pos_map.mV[VY])))
	{
		drawTracking(pos_global, 
			lerp(LLColor4::yellow, LLColor4::orange, 0.4f), 
			TRUE, 
			"You are here", 
			"", 
			llround(LLFontGL::sSansSerifSmall->getLineHeight())); // offset vertically by one line, to avoid overlap with target tracking
	}

	drawFrustum();

	// Draw icons for the avatars in each region.
	// Draw after avatar so you can see nearby people.
	if (gSavedSettings.getBOOL("MapShowPeople"))
	{
		drawAgents();
	}

	//-----------------------------------------------------------------------

	// Always draw tracking information
	LLTracker::ETrackingStatus tracking_status = LLTracker::getTrackingStatus();
	if ( LLTracker::TRACKING_AVATAR == tracking_status )
	{
		drawTracking( LLAvatarTracker::instance().getGlobalPos(), gTrackColor, TRUE, LLTracker::getLabel(), "" );
	} 
	else if ( LLTracker::TRACKING_LANDMARK == tracking_status 
			 || LLTracker::TRACKING_LOCATION == tracking_status )
	{
		// While fetching landmarks, will have 0,0,0 location for a while,
		// so don't draw. JC
		LLVector3d pos_global = LLTracker::getTrackedPositionGlobal();
		if (!pos_global.isExactlyZero())
		{
			drawTracking( pos_global, gTrackColor, TRUE, LLTracker::getLabel(), LLTracker::getToolTip() );
		}
	}
	else if (gWorldMap->mIsTrackingUnknownLocation)
	{
		if (gWorldMap->mInvalidLocation)
		{
			// We know this location to be invalid
			LLColor4 loading_color(0.0, 0.5, 1.0, 1.0);
			drawTracking( gWorldMap->mUnknownLocation, loading_color, TRUE, "Invalid Location", "");
		}
		else
		{
			double value = fmod(current_time, 2);
			value = 0.5 + 0.5*cos(value * 3.14159f);
			LLColor4 loading_color(0.0, F32(value/2), F32(value), 1.0);
			drawTracking( gWorldMap->mUnknownLocation, loading_color, TRUE, "Loading...", "");
		}
	}
#endif
	
	// turn off the scissor
	LLGLDisable no_scissor(GL_SCISSOR_TEST);
	
	updateDirections();

	LLView::draw();

	updateVisibleBlocks();
}

void LLWorldMapView::drawGenericItems(const LLWorldMap::item_info_list_t& items, LLPointer<LLViewerImage> image)
{
	LLWorldMap::item_info_list_t::const_iterator e;
	for (e = items.begin(); e != items.end(); ++e)
	{
		const LLItemInfo& info = *e;

		drawGenericItem(info, image);
	}
}

void LLWorldMapView::drawGenericItem(const LLItemInfo& item, LLPointer<LLViewerImage> image)
{
	S32 half_width  = image->getWidth()/2;
	S32 half_height = image->getHeight()/2;

	LLVector3 pos_map = globalPosToView( item.mPosGlobal );
	gl_draw_image(llround(pos_map.mV[VX]) - half_width,
					llround(pos_map.mV[VY]) - half_height,
					image,
					LLColor4::white);				
}

void LLWorldMapView::drawAgents()
{
	//S32 half_width  = sPopularImage->getWidth()/2;
	//S32 half_height = sPopularImage->getHeight()/2;

	F32 agents_scale = (gMapScale * 0.9f) / 256.f;

	for (handle_list_t::iterator iter = mVisibleRegions.begin(); iter != mVisibleRegions.end(); ++iter)
	{
		U64 handle = *iter;
		LLSimInfo* siminfo = gWorldMap->simInfoFromHandle(handle);
		if (siminfo && (siminfo->mAccess == SIM_ACCESS_DOWN))
		{
			continue;
		}
		LLWorldMap::agent_list_map_t::iterator countsiter = gWorldMap->mAgentLocationsMap.find(handle);
		if (siminfo && siminfo->mShowAgentLocations && countsiter != gWorldMap->mAgentLocationsMap.end())
		{
			// Show Individual agents
			LLWorldMap::item_info_list_t& agentcounts = countsiter->second;
			S32 sim_agent_count = 0;
			for (LLWorldMap::item_info_list_t::iterator iter = agentcounts.begin();
				 iter != agentcounts.end(); ++iter)
			{
				const LLItemInfo& info = *iter;
				LLVector3 pos_map = globalPosToView( info.mPosGlobal );
				S32 agent_count = info.mExtra;
				if (agent_count > 0)
				{
					sim_agent_count += agent_count;
					F32 y = 0;
					for (S32 cur_agent = 0; cur_agent < agent_count; cur_agent++)
					{
						gl_draw_image(llround(pos_map.mV[VX]) - 4,
									  llround(pos_map.mV[VY] + y) - 4,
									  sAvatarSmallImage,
									  LLColor4::white );
						// move up a bit
						y += 3.f;
					}
				}
			}
			gWorldMap->mNumAgents[handle] = sim_agent_count; // override mNumAgents for this sim
		}
		else
		{
			// Show agent 'stack' at center of sim
			S32 num_agents = gWorldMap->mNumAgents[handle];
			if (num_agents > 0)
			{
				LLVector3d region_pos = from_region_handle(handle);
				region_pos[VX] += REGION_WIDTH_METERS * .5f;
				region_pos[VY] += REGION_WIDTH_METERS * .5f;
				LLVector3 pos_map = globalPosToView(region_pos);
				// Reduce the stack size as you zoom out - always display at lease one agent where there is one or more
				S32 agent_count = (S32)(((num_agents-1) * agents_scale + (num_agents-1) * 0.1f)+.1f) + 1;
				S32 y = 0;
				S32 cur_agent;
				for (cur_agent = 0; cur_agent < agent_count; cur_agent++)
				{
					gl_draw_image(
						llround(pos_map.mV[VX]) - 4,
						llround(pos_map.mV[VY]) + y - 4,
						sAvatarSmallImage,
						LLColor4::white );
					// move up a bit
					y += 3;
				}
			}
		}
	}
}


void LLWorldMapView::drawEvents()
{
	BOOL show_mature = gSavedSettings.getBOOL("ShowMatureEvents");

	// Non-selected events
	// Draw under avatar so you can find yourself.
	LLWorldMap::item_info_list_t::const_iterator e;
	for (e = gWorldMap->mPGEvents.begin(); e != gWorldMap->mPGEvents.end(); ++e)
	{
		const LLItemInfo& event = *e;

		// Draw, but without relative-Z icons
		if (!event.mSelected)
		{
			LLVector3 pos_map = globalPosToView( event.mPosGlobal );
			gl_draw_image(llround(pos_map.mV[VX]) - sEventImage->getWidth()/2,
							llround(pos_map.mV[VY]) - sEventImage->getHeight()/2,
							sEventImage,
							LLColor4::white);				
		}
	}
	if (show_mature)
	{
		for (e = gWorldMap->mMatureEvents.begin(); e != gWorldMap->mMatureEvents.end(); ++e)
		{
			const LLItemInfo& event = *e;

			// Draw, but without relative-Z icons
			if (!event.mSelected)
			{
				LLVector3 pos_map = globalPosToView( event.mPosGlobal );
				gl_draw_image(llround(pos_map.mV[VX]) - sEventMatureImage->getWidth()/2,
								llround(pos_map.mV[VY]) - sEventMatureImage->getHeight()/2,
								sEventMatureImage,
								LLColor4::white);				
			}
		}
	}

	// Selected events
	// Draw under avatar so you can find yourself.
	for (e = gWorldMap->mPGEvents.begin(); e != gWorldMap->mPGEvents.end(); ++e)
	{
		const LLItemInfo& event = *e;

		// Draw, but without relative-Z icons
		if (event.mSelected)
		{
			LLVector3 pos_map = globalPosToView( event.mPosGlobal );
			gl_draw_image(llround(pos_map.mV[VX]) - sEventImage->getWidth()/2,
							llround(pos_map.mV[VY]) - sEventImage->getHeight()/2,
							sEventImage,
							LLColor4::white);				

			//drawIconName(
			//	pos_map.mV[VX], 
			//	pos_map.mV[VY], 
			//	gEventColor,
			//	event.mToolTip.c_str(), 
			//	event.mName.c_str() );
		}
	}
	if (show_mature)
	{
		for (e = gWorldMap->mMatureEvents.begin(); e != gWorldMap->mMatureEvents.end(); ++e)
		{
			const LLItemInfo& event = *e;

			// Draw, but without relative-Z icons
			if (event.mSelected)
			{
				LLVector3 pos_map = globalPosToView( event.mPosGlobal );
				gl_draw_image(llround(pos_map.mV[VX]) - sEventMatureImage->getWidth()/2,
								llround(pos_map.mV[VY]) - sEventMatureImage->getHeight()/2,
								sEventMatureImage,
								LLColor4::white);				

				//drawIconName(
				//	pos_map.mV[VX], 
				//	pos_map.mV[VY], 
				//	gEventColor,
				//	event.mToolTip.c_str(), 
				//	event.mName.c_str() );
			}
		}
	}
}

void LLWorldMapView::drawDots()
{

}


void LLWorldMapView::drawFrustum()
{
	// Draw frustum
	F32 meters_to_pixels = gMapScale/ REGION_WIDTH_METERS;

	F32 horiz_fov = gCamera->getView() * gCamera->getAspect();
	F32 far_clip_meters = gCamera->getFar();
	F32 far_clip_pixels = far_clip_meters * meters_to_pixels;

	F32 half_width_meters = far_clip_meters * tan( horiz_fov / 2 );
	F32 half_width_pixels = half_width_meters * meters_to_pixels;
	
	F32 ctr_x = mRect.getWidth() * 0.5f + sPanX;
	F32 ctr_y = mRect.getHeight() * 0.5f + sPanY;

	LLGLSNoTexture gls_no_texture;

	// Since we don't rotate the map, we have to rotate the frustum.
	glPushMatrix();
		glTranslatef( ctr_x, ctr_y, 0 );
		glRotatef( atan2( gCamera->getAtAxis().mV[VX], gCamera->getAtAxis().mV[VY] ) * RAD_TO_DEG, 0.f, 0.f, -1.f);

		// Draw triangle with more alpha in far pixels to make it 
		// fade out in distance.
		glBegin( GL_TRIANGLES  );
			glColor4f(1.f, 1.f, 1.f, 0.25f);
			glVertex2f( 0, 0 );

			glColor4f(1.f, 1.f, 1.f, 0.02f);
			glVertex2f( -half_width_pixels, far_clip_pixels );

			glColor4f(1.f, 1.f, 1.f, 0.02f);
			glVertex2f(  half_width_pixels, far_clip_pixels );
		glEnd();
	glPopMatrix();
}


LLVector3 LLWorldMapView::globalPosToView( const LLVector3d& global_pos )
{
	LLVector3d relative_pos_global = global_pos - gAgent.getCameraPositionGlobal();
	LLVector3 pos_local;
	pos_local.setVec(relative_pos_global);  // convert to floats from doubles

	pos_local.mV[VX] *= sPixelsPerMeter;
	pos_local.mV[VY] *= sPixelsPerMeter;
	// leave Z component in meters


	pos_local.mV[VX] += mRect.getWidth() / 2 + sPanX;
	pos_local.mV[VY] += mRect.getHeight() / 2 + sPanY;

	return pos_local;
}


void LLWorldMapView::drawTracking(const LLVector3d& pos_global, const LLColor4& color, 
							BOOL draw_arrow, LLString label, LLString tooltip, S32 vert_offset )
{
	LLVector3 pos_local = globalPosToView( pos_global );
	S32 x = llround( pos_local.mV[VX] );
	S32 y = llround( pos_local.mV[VY] );
	LLFontGL* font = LLFontGL::sSansSerifSmall;
	S32 text_x = x;
	S32 text_y = (S32)(y - sTrackCircleImage->getHeight()/2 - font->getLineHeight());

	BOOL is_in_window = true;

	if(    x < 0 
		|| y < 0 
		|| x >= mRect.getWidth() 
		|| y >= mRect.getHeight() )
	{
		if (draw_arrow)
		{
			drawTrackingCircle( mRect, x, y, color, 3, 15 );
			drawTrackingArrow( mRect, x, y, color );
			text_x = sTrackingArrowX;
			text_y = sTrackingArrowY;
		}
		is_in_window = false;
	}
	else if (LLTracker::getTrackingStatus() == LLTracker::TRACKING_LOCATION &&
		LLTracker::getTrackedLocationType() != LLTracker::LOCATION_NOTHING)
	{
		drawTrackingCircle( mRect, x, y, color, 3, 15 );
	}
	else
	{
		// Draw, but without relative Z
		gl_draw_image(x - sTrackCircleImage->getWidth()/2, 
					  y - sTrackCircleImage->getHeight()/2, 
					  sTrackCircleImage, 
					  color);
	}

	// clamp text position to on-screen
	const S32 TEXT_PADDING = DEFAULT_TRACKING_ARROW_SIZE + 2;
	S32 half_text_width = llfloor(font->getWidthF32(label) * 0.5f);
	text_x = llclamp(text_x, half_text_width + TEXT_PADDING, mRect.getWidth() - half_text_width - TEXT_PADDING);
	text_y = llclamp(text_y + vert_offset, TEXT_PADDING + vert_offset, mRect.getHeight() - llround(font->getLineHeight()) - TEXT_PADDING - vert_offset);

	if (label != "")
	{
		font->renderUTF8(
			label, 0,
			text_x, 
			text_y,
			LLColor4::white, LLFontGL::HCENTER,
			LLFontGL::BASELINE, LLFontGL::DROP_SHADOW);

		if (tooltip != "")
		{
			text_y -= (S32)font->getLineHeight();

			font->renderUTF8(
				tooltip, 0,
				text_x, 
				text_y,
				LLColor4::white, LLFontGL::HCENTER,
				LLFontGL::BASELINE, LLFontGL::DROP_SHADOW);
		}
	}
}

// If you change this, then you need to change LLTracker::getTrackedPositionGlobal() as well
LLVector3d LLWorldMapView::viewPosToGlobal( S32 x, S32 y )
{
	x -= llfloor((mRect.getWidth() / 2 + sPanX));
	y -= llfloor((mRect.getHeight() / 2 + sPanY));

	LLVector3 pos_local( (F32)x, (F32)y, 0.f );

	pos_local *= ( REGION_WIDTH_METERS / gMapScale );
	
	LLVector3d pos_global;
	pos_global.setVec( pos_local );
	pos_global += gAgent.getCameraPositionGlobal();
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


BOOL LLWorldMapView::handleToolTip( S32 x, S32 y, LLString& msg, LLRect* sticky_rect_screen )
{
	if( !getVisible() || !pointInView( x, y ) )
	{
		return FALSE;
	}

	LLVector3d pos_global = viewPosToGlobal(x, y);

	LLSimInfo* info = gWorldMap->simInfoFromPosGlobal(pos_global);
	if (info)
	{
		LLViewerRegion *region = gAgent.getRegion();

		std::string message = 
			llformat("%s (%s)",
					 info->mName.c_str(),
					 LLViewerRegion::accessToString(info->mAccess));

		if (info->mAccess != SIM_ACCESS_DOWN)
		{
			S32 agent_count = gWorldMap->mNumAgents[info->mHandle];			
			if (region && region->getHandle() == info->mHandle)
			{
				++agent_count; // Bump by 1 if we're here
			}

			// We may not have an agent count when the map is really
			// zoomed out, so don't display anything about the count. JC
			if (agent_count > 0)
			{
				message += llformat("\n%d ", agent_count);

				if (agent_count == 1)
				{
					message += "person";
				}
				else
				{
					message += "people";
				}
			}
		}
		msg.assign( message );

		// Optionally show region flags
		std::string region_flags = LLViewerRegion::regionFlagsToString(info->mRegionFlags);

		if (!region_flags.empty())
		{
			msg += '\n';
			msg += region_flags;
		}
					
		S32 SLOP = 4;
		localPointToScreen( 
			x - SLOP, y - SLOP, 
			&(sticky_rect_screen->mLeft), &(sticky_rect_screen->mBottom) );
		sticky_rect_screen->mRight = sticky_rect_screen->mLeft + 2 * SLOP;
		sticky_rect_screen->mTop = sticky_rect_screen->mBottom + 2 * SLOP;
	}
	return TRUE;
}

// Pass relative Z of 0 to draw at same level.
// static
void LLWorldMapView::drawAvatar(F32 x_pixels, 
								F32 y_pixels,
								const LLColor4& color,
								F32 relative_z,
								F32 dot_radius)
{	
	F32 left =		x_pixels - dot_radius;
	F32 right =		x_pixels + dot_radius;
	F32 center = (left + right) * 0.5f;
	F32 top =		y_pixels + dot_radius;
	F32 bottom =	y_pixels - dot_radius;

	const F32 HEIGHT_THRESHOLD = 7.f;

	if (relative_z > HEIGHT_THRESHOLD)
	{
		LLGLSNoTexture gls_no_texture;
		glColor4fv( color.mV );
		LLUI::setLineWidth(1.5f);
		glBegin( GL_LINES );
			glVertex2f(left, top);
			glVertex2f(right, top);
			glVertex2f(center, top);
			glVertex2f(center, bottom);
		glEnd();
		LLUI::setLineWidth(1.0f);
	}
	else if (relative_z > -HEIGHT_THRESHOLD)
	{
		gl_draw_image(	llround(x_pixels) - sAvatarSmallImage->getWidth()/2, 
						llround(y_pixels) - sAvatarSmallImage->getHeight()/2, 
						sAvatarSmallImage, 
						color);
	}
	else
	{
		LLGLSNoTexture gls_no_texture;
		glColor4fv( color.mV );
		LLUI::setLineWidth(1.5f);
		glBegin( GL_LINES );
			glVertex2f(center, top);
			glVertex2f(center, bottom);
			glVertex2f(left, bottom);
			glVertex2f(right, bottom);
		glEnd();
		LLUI::setLineWidth(1.0f);
	}
}

// Pass relative Z of 0 to draw at same level.
// static
void LLWorldMapView::drawTrackingDot( F32 x_pixels, 
									  F32 y_pixels,
									  const LLColor4& color,
									  F32 relative_z,
									  F32 dot_radius)
{	
	F32 left =		x_pixels - dot_radius;
	F32 right =		x_pixels + dot_radius;
	F32 center = (left + right) * 0.5f;
	F32 top =		y_pixels + dot_radius;
	F32 bottom =	y_pixels - dot_radius;

	const F32 HEIGHT_THRESHOLD = 7.f;

	if (relative_z > HEIGHT_THRESHOLD)
	{
		LLGLSNoTexture gls_no_texture;
		glColor4fv( color.mV );
		LLUI::setLineWidth(1.5f);
		glBegin( GL_LINES );
			glVertex2f(left, top);
			glVertex2f(right, top);
			glVertex2f(center, top);
			glVertex2f(center, bottom);
		glEnd();
		LLUI::setLineWidth(1.0f);
	}
	else if (relative_z > -HEIGHT_THRESHOLD)
	{
		gl_draw_image(	llround(x_pixels) - sAvatarSmallImage->getWidth()/2, 
						llround(y_pixels) - sAvatarSmallImage->getHeight()/2, 
						sTrackCircleImage, 
						color);
	}
	else
	{
		LLGLSNoTexture gls_no_texture;
		glColor4fv( color.mV );
		LLUI::setLineWidth(1.5f);
		glBegin( GL_LINES );
			glVertex2f(center, top);
			glVertex2f(center, bottom);
			glVertex2f(left, bottom);
			glVertex2f(right, bottom);
		glEnd();
		LLUI::setLineWidth(1.0f);
	}
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
	LLFontGL::sSansSerif->renderUTF8(first_line, 0,
		text_x,
		text_y,
		color,
		LLFontGL::HCENTER,
		LLFontGL::TOP,
		LLFontGL::DROP_SHADOW);

	text_y -= llround(LLFontGL::sSansSerif->getLineHeight());

	// render text
	LLFontGL::sSansSerif->renderUTF8(second_line, 0,
		text_x,
		text_y,
		color,
		LLFontGL::HCENTER,
		LLFontGL::TOP,
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

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef((F32)x, (F32)y, 0.f);
	gl_washer_segment_2d(inner_radius, outer_radius, start_theta, end_theta, 40, color, color);
	glPopMatrix();

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
		sTrackArrowImage, 
		color);
}

void LLWorldMapView::setDirectionPos( LLTextBox* text_box, F32 rotation )
{
	// Rotation is in radians.
	// Rotation of 0 means x = 1, y = 0 on the unit circle.


	F32 map_half_height = mRect.getHeight() * 0.5f;
	F32 map_half_width = mRect.getWidth() * 0.5f;
	F32 text_half_height = text_box->getRect().getHeight() * 0.5f;
	F32 text_half_width = text_box->getRect().getWidth() * 0.5f;
	F32 radius = llmin( map_half_height - text_half_height, map_half_width - text_half_width );

	text_box->setOrigin( 
		llround(map_half_width - text_half_width + radius * cos( rotation )),
		llround(map_half_height - text_half_height + radius * sin( rotation )) );
}


void LLWorldMapView::updateDirections()
{
	S32 width = mRect.getWidth();
	S32 height = mRect.getHeight();

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
	LLVector3 pos_view = globalPosToView(item.mPosGlobal);
	S32 item_x = llround(pos_view.mV[VX]);
	S32 item_y = llround(pos_view.mV[VY]);

	if (x < item_x - BIG_DOT_RADIUS) return false;
	if (x > item_x + BIG_DOT_RADIUS) return false;
	if (y < item_y - BIG_DOT_RADIUS) return false;
	if (y > item_y + BIG_DOT_RADIUS) return false;

	LLSimInfo* sim_info = gWorldMap->simInfoFromHandle(item.mRegionHandle);
	if (sim_info)
	{
		if (track)
		{
			gFloaterWorldMap->trackLocation(item.mPosGlobal);
		}
	}

	if (track)
	{
		gFloaterWorldMap->trackGenericItem(item);
	}

	item.mSelected = TRUE;
	*id = item.mID;

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
	// Distributed Messaging branch honors kt.
	if(gAgent.isGodlike())
	{
		pos_global.mdV[VZ] = 200.0;
	}

	*hit_type = 0; // hit nothing

	gWorldMap->mIsTrackingUnknownLocation = FALSE;
	gWorldMap->mIsTrackingDoubleClick = FALSE;
	gWorldMap->mIsTrackingCommit = FALSE;

	LLWorldMap::item_info_list_t::iterator it;

	// clear old selected stuff
	for (it = gWorldMap->mPGEvents.begin(); it != gWorldMap->mPGEvents.end(); ++it)
	{
		(*it).mSelected = FALSE;
	}
	for (it = gWorldMap->mMatureEvents.begin(); it != gWorldMap->mMatureEvents.end(); ++it)
	{
		(*it).mSelected = FALSE;
	}
	for (it = gWorldMap->mPopular.begin(); it != gWorldMap->mPopular.end(); ++it)
	{
		(*it).mSelected = FALSE;
	}
	for (it = gWorldMap->mLandForSale.begin(); it != gWorldMap->mLandForSale.end(); ++it)
	{
		(*it).mSelected = FALSE;
	}
	for (it = gWorldMap->mClassifieds.begin(); it != gWorldMap->mClassifieds.end(); ++it)
	{
		(*it).mSelected = FALSE;
	}

	// Select event you clicked on
	if (gSavedSettings.getBOOL("MapShowEvents"))
	{
		for (it = gWorldMap->mPGEvents.begin(); it != gWorldMap->mPGEvents.end(); ++it)
		{
			LLItemInfo& event = *it;

			if (checkItemHit(x, y, event, id, false))
			{
				*hit_type = MAP_ITEM_PG_EVENT;
				mItemPicked = TRUE;
				gFloaterWorldMap->trackEvent(event);
				return;
			}
		}
		if (gSavedSettings.getBOOL("ShowMatureEvents"))
		{
			for (it = gWorldMap->mMatureEvents.begin(); it != gWorldMap->mMatureEvents.end(); ++it)
			{
				LLItemInfo& event = *it;

				if (checkItemHit(x, y, event, id, false))
				{
					*hit_type = MAP_ITEM_MATURE_EVENT;
					mItemPicked = TRUE;
					gFloaterWorldMap->trackEvent(event);
					return;
				}
			}
		}
	}

	if (gSavedSettings.getBOOL("MapShowPopular"))
	{
		for (it = gWorldMap->mPopular.begin(); it != gWorldMap->mPopular.end(); ++it)
		{
			LLItemInfo& popular = *it;

			if (checkItemHit(x, y, popular, id, true))
			{
				*hit_type = MAP_ITEM_POPULAR;
				mItemPicked = TRUE;
				return;
			}
		}
	}

	if (gSavedSettings.getBOOL("MapShowLandForSale"))
	{
		for (it = gWorldMap->mLandForSale.begin(); it != gWorldMap->mLandForSale.end(); ++it)
		{
			LLItemInfo& land = *it;

			if (checkItemHit(x, y, land, id, true))
			{
				*hit_type = MAP_ITEM_LAND_FOR_SALE;
				mItemPicked = TRUE;
				return;
			}
		}
	}

	if (gSavedSettings.getBOOL("MapShowClassifieds"))
	{
		for (it = gWorldMap->mClassifieds.begin(); it != gWorldMap->mClassifieds.end(); ++it)
		{
			LLItemInfo& classified = *it;

			if (checkItemHit(x, y, classified, id, true))
			{
				*hit_type = MAP_ITEM_CLASSIFIED;
				mItemPicked = TRUE;
				return;
			}
		}
	}

	// If we get here, we haven't clicked on an icon

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
	gFocusMgr.setMouseCapture( this, NULL );

	mMouseDownPanX = llround(sPanX);
	mMouseDownPanY = llround(sPanY);
	mMouseDownX = x;
	mMouseDownY = y;
	sHandledLastClick = TRUE;
	return TRUE;
}

BOOL LLWorldMapView::handleMouseUp( S32 x, S32 y, MASK mask )
{
	if (this == gFocusMgr.getMouseCapture())
	{
		if (mPanning)
		{
			// restore mouse cursor
			S32 local_x, local_y;
			local_x = mMouseDownX + llfloor(sPanX - mMouseDownPanX);
			local_y = mMouseDownY + llfloor(sPanY - mMouseDownPanY);
			LLRect clip_rect = mRect;
			clip_rect.stretch(-8);
			clip_rect.clipPointToRect(mMouseDownX, mMouseDownY, local_x, local_y);
			LLUI::setCursorPositionLocal(this, local_x, local_y);

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
		gFocusMgr.setMouseCapture( NULL, NULL );
		return TRUE;
	}
	return FALSE;
}

void LLWorldMapView::updateBlock(S32 block_x, S32 block_y)
{
	S32 offset = block_x | (block_y * MAP_BLOCK_RES);
	if (!gWorldMap->mMapBlockLoaded[gWorldMap->mCurrentMap][offset])
	{
// 		llinfos << "Loading Block (" << block_x << "," << block_y << ")" << llendl;
		gWorldMap->sendMapBlockRequest(block_x << 3, block_y << 3, (block_x << 3) + 7, (block_y << 3) + 7);
		gWorldMap->mMapBlockLoaded[gWorldMap->mCurrentMap][offset] = TRUE;
	}
}

void LLWorldMapView::updateVisibleBlocks()
{
	if (gMapScale < SIM_MAP_SCALE)
	{
		// We don't care what is loaded if we're zoomed out
		return;
	}
	// check if we've loaded the 9 potentially visible zones
	LLVector3d camera_global = gAgent.getCameraPositionGlobal();

	// Convert pan to sim coordinates
	S32 world_center_x = S32((-sPanX / gMapScale) + (camera_global.mdV[0] / REGION_WIDTH_METERS));
	S32 world_center_y = S32((-sPanY / gMapScale) + (camera_global.mdV[1] / REGION_WIDTH_METERS));

	// Find the corresponding 8x8 block
	S32 world_block_x = world_center_x >> 3;
	S32 world_block_y = world_center_y >> 3;

	for (S32 block_x = llmax(world_block_x-1, 0); block_x <= llmin(world_block_x+1, MAP_BLOCK_RES-1); ++block_x)
	{
		for (S32 block_y = llmax(world_block_y-1, 0); block_y <= llmin(world_block_y+1, MAP_BLOCK_RES-1); ++block_y)
		{
			updateBlock(block_x, block_y);
		}
	}
}

BOOL LLWorldMapView::handleHover( S32 x, S32 y, MASK mask )
{
	if (this == gFocusMgr.getMouseCapture())
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
			{
				gFloaterWorldMap->close();
				// This is an ungainly hack
				char uuid_str[38];		/* Flawfinder: ignore */
				S32 event_id;
				id.toString(uuid_str);
				sscanf(&uuid_str[28], "%X", &event_id);
				LLFloaterDirectory::showEvents(event_id);
				break;
			}
		case MAP_ITEM_POPULAR:
			{
				gFloaterWorldMap->close();
				LLFloaterDirectory::showPopular(id);
				break;
			}
		case MAP_ITEM_LAND_FOR_SALE:
			{
				gFloaterWorldMap->close();
				LLFloaterDirectory::showLandForSale(id);
				break;
			}
		case MAP_ITEM_CLASSIFIED:
			{
				gFloaterWorldMap->close();
				LLFloaterDirectory::showClassified(id);
				break;
			}
		default:
			{
				if (gWorldMap->mIsTrackingUnknownLocation)
				{
					gWorldMap->mIsTrackingDoubleClick = TRUE;
				}
				else
				{
					// Teleport if we got a valid location
					LLVector3d pos_global = viewPosToGlobal(x,y);
					LLSimInfo* sim_info = gWorldMap->simInfoFromPosGlobal(pos_global);
					if (sim_info && sim_info->mAccess != SIM_ACCESS_DOWN)
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
