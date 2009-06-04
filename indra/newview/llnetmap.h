/** 
 * @file llnetmap.h
 * @brief A little map of the world with network information
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

#ifndef LL_LLNETMAP_H
#define LL_LLNETMAP_H

#include "llpanel.h"
#include "llmemberlistener.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v4color.h"
#include "llimage.h"
#include "llimagegl.h"


class LLTextBox;

class LLNetMap : public LLPanel
{
public:
	LLNetMap(const std::string& name);
	virtual ~LLNetMap();

	virtual void	draw();
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	virtual BOOL	handleDoubleClick( S32 x, S32 y, MASK mask );
	virtual BOOL	handleRightMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual BOOL	handleToolTip( S32 x, S32 y, std::string& msg, LLRect* sticky_rect_screen );

	void			renderScaledPointGlobal( const LLVector3d& pos, const LLColor4U &color, F32 radius );

private:

	void			setScale( F32 scale );

	// *TODO: Enable panning of the mini-map
	void			translatePan( F32 delta_x, F32 delta_y );
	void			setPan( F32 x, F32 y )			{ mTargetPanX = x; mTargetPanY = y; }

	void renderPoint(const LLVector3 &pos, const LLColor4U &color, 
					 S32 diameter, S32 relative_height = 0);
	LLVector3		globalPosToView(const LLVector3d& global_pos, BOOL rotated);
	LLVector3d		viewPosToGlobal(S32 x,S32 y, BOOL rotated);

	void			drawTracking( const LLVector3d& pos_global,
							BOOL rotated,
							const LLColor4& color,
							BOOL draw_arrow = TRUE);

	void			setDirectionPos( LLTextBox* text_box, F32 rotation );
	void			updateMinorDirections();
	void			createObjectImage();

	LLHandle<LLView>	mPopupMenuHandle;

	F32				mScale;					// Size of a region in pixels
	F32				mPixelsPerMeter;		// world meters to map pixels
	F32				mObjectMapTPM;			// texels per meter on map
	F32				mObjectMapPixels;		// Width of object map in pixels;
	F32				mTargetPanX;
	F32				mTargetPanY;
	F32				mCurPanX;
	F32				mCurPanY;
	BOOL			mUpdateNow;
	LLVector3d		mObjectImageCenterGlobal;
	LLPointer<LLImageRaw> mObjectRawImagep;
	LLPointer<LLImageGL>	mObjectImagep;

private:
	LLUUID			mClosestAgentToCursor;
	LLUUID			mClosestAgentAtLastRightClick;

	static BOOL		sRotateMap;
	static LLNetMap*	sInstance;
	static BOOL isAgentUnderCursor(void*) { return sInstance && sInstance->mClosestAgentToCursor.notNull(); }
	static void showAgentProfile(void*);
	BOOL isAgentUnderCursor() { return mClosestAgentToCursor.notNull(); }

	class LLScaleMap : public LLMemberListener<LLNetMap>
	{
	public:
		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};

	class LLStopTracking : public LLMemberListener<LLNetMap>
	{
	public:
		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};

	class LLEnableTracking : public LLMemberListener<LLNetMap>
	{
	public:
		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};

	class LLShowAgentProfile : public LLMemberListener<LLNetMap>
	{
	public:
		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};

	class LLEnableProfile : public LLMemberListener<LLNetMap>
	{
	public:
		/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};
};


#endif
