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

#include "llmath.h"
#include "lluictrl.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v4color.h"
#include "llpointer.h"

class LLColor4U;
class LLCoordGL;
class LLImageRaw;
class LLTextBox;
class LLViewerTexture;

class LLNetMap : public LLUICtrl
{
public:
	struct Params 
	:	public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<LLUIColor>	bg_color;

		Params()
		:	bg_color("bg_color") 
		{}
	};

protected:
	LLNetMap (const Params & p);
	friend class LLUICtrlFactory;

public:
	virtual ~LLNetMap();

	static const F32 MAP_SCALE_MIN;
	static const F32 MAP_SCALE_MID;
	static const F32 MAP_SCALE_MAX;

	/*virtual*/ void	draw();
	/*virtual*/ BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	/*virtual*/ BOOL	handleToolTip( S32 x, S32 y, MASK mask);
	/*virtual*/ void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	
	void			setScale( F32 scale );
	void			setToolTipMsg(const std::string& msg) { mToolTipMsg = msg; }
	void			renderScaledPointGlobal( const LLVector3d& pos, const LLColor4U &color, F32 radius );
	
private:
	void			translatePan( F32 delta_x, F32 delta_y );
	void			setPan( F32 x, F32 y )			{ mTargetPanX = x; mTargetPanY = y; }

	const LLVector3d& getObjectImageCenterGlobal()	{ return mObjectImageCenterGlobal; }
	void 			renderPoint(const LLVector3 &pos, const LLColor4U &color, 
								S32 diameter, S32 relative_height = 0);

	LLVector3		globalPosToView(const LLVector3d& global_pos);
	LLVector3d		viewPosToGlobal(S32 x,S32 y);

	void			drawTracking( const LLVector3d& pos_global, 
								  const LLColor4& color,
								  BOOL draw_arrow = TRUE);
	
	void			createObjectImage();

private:
	LLUIColor		mBackgroundColor;

	F32				mScale;					// Size of a region in pixels
	F32				mPixelsPerMeter;		// world meters to map pixels
	F32				mObjectMapTPM;			// texels per meter on map
	F32				mObjectMapPixels;		// Width of object map in pixels
	F32				mDotRadius;				// Size of avatar markers
	F32				mTargetPanX;
	F32				mTargetPanY;
	F32				mCurPanX;
	F32				mCurPanY;
	BOOL			mUpdateNow;
	LLVector3d		mObjectImageCenterGlobal;
	LLPointer<LLImageRaw> mObjectRawImagep;
	LLPointer<LLViewerTexture>	mObjectImagep;

	LLUUID			mClosestAgentToCursor;
	LLUUID			mClosestAgentAtLastRightClick;

	std::string		mToolTipMsg;
};


#endif
