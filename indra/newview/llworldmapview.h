/** 
 * @file llworldmapview.h
 * @brief LLWorldMapView class header file
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

// Global map of the world.

#ifndef LL_LLWORLDMAPVIEW_H
#define LL_LLWORLDMAPVIEW_H

#include "llpanel.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v4color.h"
#include "llviewerimage.h"
#include "llmapimagetype.h"
#include "llworldmap.h"

class LLItemInfo;

const S32 DEFAULT_TRACKING_ARROW_SIZE = 16;

class LLColor4;
class LLColor4U;
class LLCoordGL;
class LLViewerImage;
class LLTextBox;


class LLWorldMapView : public LLPanel
{
public:
	static void initClass();
	static void cleanupClass();

	LLWorldMapView(const std::string& name, const LLRect& rect );
	virtual ~LLWorldMapView();

	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE );
	virtual void	setVisible(BOOL visible);

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDoubleClick( S32 x, S32 y, MASK mask );
	virtual BOOL	handleHover( S32 x, S32 y, MASK mask );
	virtual BOOL	handleToolTip( S32 x, S32 y, LLString& msg, LLRect* sticky_rect_screen );

	bool			checkItemHit(S32 x, S32 y, LLItemInfo& item, LLUUID* id, bool track);
	void			handleClick(S32 x, S32 y, MASK mask, S32* hit_type, LLUUID* id);

	// Scale and pan are shared across all instances.
	static void		setScale( F32 scale );
	static void		translatePan( S32 delta_x, S32 delta_y );
	static void		setPan( S32 x, S32 y, BOOL snap = TRUE );

	LLVector3		globalPosToView(const LLVector3d& global_pos);
	LLVector3d		viewPosToGlobal(S32 x,S32 y);

	virtual void	draw();
	void			drawGenericItems(const LLWorldMap::item_info_list_t& items, LLUIImagePtr image);
	void			drawGenericItem(const LLItemInfo& item, LLUIImagePtr image);
	void			drawImage(const LLVector3d& global_pos, LLUIImagePtr image, const LLColor4& color = LLColor4::white);
	void			drawImageStack(const LLVector3d& global_pos, LLUIImagePtr image, U32 count, F32 offset, const LLColor4& color);
	void			drawAgents();
	void			drawEvents();
	void			drawFrustum();

	static void		cleanupTextures();

	// Draw the tracking indicator, doing the right thing if it's outside
	// the view area.
	void			drawTracking( const LLVector3d& pos_global, 
								  const LLColor4& color,
								  BOOL draw_arrow = TRUE, LLString label = LLString(), LLString tooltip = "", S32 vert_offset = 0);
	static void		drawTrackingArrow(const LLRect& view_rect, S32 x, S32 y, 
									  const LLColor4& color,
									  S32 arrow_size = DEFAULT_TRACKING_ARROW_SIZE);
	static void		drawTrackingDot(F32 x_pixels, 
									F32 y_pixels, 
									const LLColor4& color,
									F32 relative_z = 0.f,
									F32 dot_radius = 3.f);

	static void		drawTrackingCircle( const LLRect& rect, S32 x, S32 y, 
										const LLColor4& color, 
										S32 min_thickness, 
										S32 overlap );
	static void		drawAvatar(	F32 x_pixels, 
								F32 y_pixels, 
								const LLColor4& color,
								F32 relative_z = 0.f,
								F32 dot_radius = 3.f);
	static void		drawIconName(F32 x_pixels, 
									F32 y_pixels, 
									const LLColor4& color,
									const std::string& first_line,
									const std::string& second_line);

	// Prevents accidental double clicks
	static void		clearLastClick() { sHandledLastClick = FALSE; }

	// if the view changes, download additional sim info as needed
	void			updateBlock(S32 block_x, S32 block_y);
	void			updateVisibleBlocks();

protected:
	void			setDirectionPos( LLTextBox* text_box, F32 rotation );
	void			updateDirections();

public:
	LLColor4		mBackgroundColor;

	static LLUIImagePtr	sAvatarYouSmallImage;
	static LLUIImagePtr	sAvatarSmallImage;
	static LLUIImagePtr	sAvatarLargeImage;
	static LLUIImagePtr	sAvatarAboveImage;
	static LLUIImagePtr	sAvatarBelowImage;
	static LLUIImagePtr	sTelehubImage;
	static LLUIImagePtr	sInfohubImage;
	static LLUIImagePtr	sHomeImage;
	static LLUIImagePtr	sEventImage;
	static LLUIImagePtr	sEventMatureImage;
	static LLUIImagePtr	sTrackCircleImage;
	static LLUIImagePtr	sTrackArrowImage;
	static LLUIImagePtr	sClassifiedsImage;
	static LLUIImagePtr	sForSaleImage;

	static F32		sThresholdA;
	static F32		sThresholdB;
	static F32		sPixelsPerMeter;		// world meters to map pixels

	BOOL			mItemPicked;

	static F32		sPanX;		// in pixels
	static F32		sPanY;		// in pixels
	static F32		sTargetPanX;		// in pixels
	static F32		sTargetPanY;		// in pixels
	static S32		sTrackingArrowX;
	static S32		sTrackingArrowY;

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

	typedef std::vector<U64> handle_list_t;
	handle_list_t mVisibleRegions; // set every frame

	static std::map<std::string,LLString> sStringsMap;
};

#endif
