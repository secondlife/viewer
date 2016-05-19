/** 
 * @file llworldmapview.h
 * @brief LLWorldMapView class header file
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

// View of the global map of the world

// The data (model) for the global map (a singleton, unique to the application instance) is 
// in LLWorldMap and is typically accessed using LLWorldMap::getInstance()

#ifndef LL_LLWORLDMAPVIEW_H
#define LL_LLWORLDMAPVIEW_H

#include "llpanel.h"
#include "llworldmap.h"
#include "v4color.h"

const S32 DEFAULT_TRACKING_ARROW_SIZE = 16;

class LLUUID;
class LLVector3d;
class LLVector3;
class LLTextBox;


class LLWorldMapView : public LLPanel
{
public:
	static void initClass();
	static void cleanupClass();

	LLWorldMapView();
	virtual ~LLWorldMapView();
	
	virtual BOOL	postBuild();
	
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE );
	virtual void	setVisible(BOOL visible);

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDoubleClick( S32 x, S32 y, MASK mask );
	virtual BOOL	handleHover( S32 x, S32 y, MASK mask );
	virtual BOOL	handleToolTip( S32 x, S32 y, MASK mask);

	bool			checkItemHit(S32 x, S32 y, LLItemInfo& item, LLUUID* id, bool track);
	void			handleClick(S32 x, S32 y, MASK mask, S32* hit_type, LLUUID* id);

	// Scale and pan are shared across all instances! (i.e. Terrain and Objects maps are always registered)
	static void		setScale( F32 scale );
	static void		translatePan( S32 delta_x, S32 delta_y );
	static void		setPan( S32 x, S32 y, BOOL snap = TRUE );
	// Return true if the current scale level is above the threshold for accessing region info
	static bool		showRegionInfo();

	LLVector3		globalPosToView(const LLVector3d& global_pos);
	LLVector3d		viewPosToGlobal(S32 x,S32 y);

	virtual void	draw();
	void			drawGenericItems(const LLSimInfo::item_info_list_t& items, LLUIImagePtr image);
	void			drawGenericItem(const LLItemInfo& item, LLUIImagePtr image);
	void			drawImage(const LLVector3d& global_pos, LLUIImagePtr image, const LLColor4& color = LLColor4::white);
	void			drawImageStack(const LLVector3d& global_pos, LLUIImagePtr image, U32 count, F32 offset, const LLColor4& color);
	void			drawAgents();
	void			drawItems();
	void			drawFrustum();
	void			drawMipmap(S32 width, S32 height);
	bool			drawMipmapLevel(S32 width, S32 height, S32 level, bool load = true);

	static void		cleanupTextures();

	// Draw the tracking indicator, doing the right thing if it's outside
	// the view area.
	void			drawTracking( const LLVector3d& pos_global, const LLColor4& color, BOOL draw_arrow = TRUE,
								  const std::string& label = std::string(), const std::string& tooltip = std::string(),
								  S32 vert_offset = 0);
	static void		drawTrackingArrow(const LLRect& view_rect, S32 x, S32 y, 
									  const LLColor4& color,
									  S32 arrow_size = DEFAULT_TRACKING_ARROW_SIZE);
	static void		drawTrackingDot(F32 x_pixels, 
									F32 y_pixels, 
									const LLColor4& color,
									F32 relative_z = 0.f,
									F32 dot_radius = 5.f);

	static void		drawTrackingCircle( const LLRect& rect, S32 x, S32 y, 
										const LLColor4& color, 
										S32 min_thickness, 
										S32 overlap );
	static void		drawAvatar(	F32 x_pixels, 
								F32 y_pixels, 
								const LLColor4& color,
								F32 relative_z = 0.f,
								F32 dot_radius = 3.f,
								bool reached_max_z = false);
	static void		drawIconName(F32 x_pixels, 
									F32 y_pixels, 
									const LLColor4& color,
									const std::string& first_line,
									const std::string& second_line);

	// Prevents accidental double clicks
	static void		clearLastClick() { sHandledLastClick = FALSE; }

	// if the view changes, download additional sim info as needed
	void			updateVisibleBlocks();

protected:
	void			setDirectionPos( LLTextBox* text_box, F32 rotation );
	void			updateDirections();

public:
	LLColor4		mBackgroundColor;

	static LLUIImagePtr	sAvatarSmallImage;
	static LLUIImagePtr	sAvatarYouImage;
	static LLUIImagePtr	sAvatarYouLargeImage;
	static LLUIImagePtr	sAvatarLevelImage;
	static LLUIImagePtr	sAvatarAboveImage;
	static LLUIImagePtr	sAvatarBelowImage;
	static LLUIImagePtr	sAvatarUnknownImage;

	static LLUIImagePtr	sTelehubImage;
	static LLUIImagePtr	sInfohubImage;
	static LLUIImagePtr	sHomeImage;
	static LLUIImagePtr	sEventImage;
	static LLUIImagePtr	sEventMatureImage;
	static LLUIImagePtr	sEventAdultImage;
	static LLUIImagePtr	sTrackCircleImage;
	static LLUIImagePtr	sTrackArrowImage;
	static LLUIImagePtr	sClassifiedsImage;
	static LLUIImagePtr	sForSaleImage;
	static LLUIImagePtr	sForSaleAdultImage;

	static F32		sMapScale;				// scale = size of a region in pixels

	BOOL			mItemPicked;

	static F32		sPanX;		// in pixels
	static F32		sPanY;		// in pixels
	static F32		sTargetPanX;		// in pixels
	static F32		sTargetPanY;		// in pixels
	static S32		sTrackingArrowX;
	static S32		sTrackingArrowY;
	static bool		sVisibleTilesLoaded;

	// Are we mid-pan from a user drag?
	BOOL			mPanning;
	S32				mMouseDownPanX;		// value at start of drag
	S32				mMouseDownPanY;		// value at start of drag
	S32				mMouseDownX;
	S32				mMouseDownY;

	LLTextBox*		mTextBoxEast;
	LLTextBox*		mTextBoxNorth;
	LLTextBox*		mTextBoxWest;
	LLTextBox*		mTextBoxSouth;

	LLTextBox*		mTextBoxSouthEast;
	LLTextBox*		mTextBoxNorthEast;
	LLTextBox*		mTextBoxNorthWest;
	LLTextBox*		mTextBoxSouthWest;
	LLTextBox*		mTextBoxScrollHint;

	static BOOL		sHandledLastClick;
	S32				mSelectIDStart;

	// Keep the list of regions that are displayed on screen. Avoids iterating through the whole region map after draw().
	typedef std::vector<U64> handle_list_t;
	handle_list_t mVisibleRegions; // set every frame

	static std::map<std::string,std::string> sStringsMap;

private:
	void drawTileOutline(S32 level, F32 top, F32 left, F32 bottom, F32 right);
};

#endif
