/** 
 * @file llnetmap.h
 * @brief A little map of the world with network information
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

#ifndef LL_LLNETMAP_H
#define LL_LLNETMAP_H

#include "llmath.h"
#include "lluictrl.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v4color.h"
#include "llpointer.h"
#include "llcoord.h"

class LLColor4U;
class LLImageRaw;
class LLViewerTexture;
class LLFloaterMap;
class LLMenuGL;

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
	friend class LLFloaterMap;

public:
	virtual ~LLNetMap();

    static const F32 MAP_SCALE_MIN;
    static const F32 MAP_SCALE_FAR;
    static const F32 MAP_SCALE_MEDIUM;
    static const F32 MAP_SCALE_CLOSE;
    static const F32 MAP_SCALE_VERY_CLOSE;
    static const F32 MAP_SCALE_MAX;

	/*virtual*/ void	draw();
	/*virtual*/ BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	/*virtual*/ BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleHover( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL	handleToolTip( S32 x, S32 y, MASK mask);
	/*virtual*/ void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	/*virtual*/ BOOL 	postBuild();
	/*virtual*/ BOOL	handleRightMouseDown( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL	handleClick(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleDoubleClick( S32 x, S32 y, MASK mask );

    void            setScale(F32 scale);

    void            setToolTipMsg(const std::string& msg) { mToolTipMsg = msg; }
    void            setParcelNameMsg(const std::string& msg) { mParcelNameMsg = msg; }
    void            setParcelSalePriceMsg(const std::string& msg) { mParcelSalePriceMsg = msg; }
    void            setParcelSaleAreaMsg(const std::string& msg) { mParcelSaleAreaMsg = msg; }
    void            setParcelOwnerMsg(const std::string& msg) { mParcelOwnerMsg = msg; }
    void            setRegionNameMsg(const std::string& msg) { mRegionNameMsg = msg; }
    void            setToolTipHintMsg(const std::string& msg) { mToolTipHintMsg = msg; }
    void            setAltToolTipHintMsg(const std::string& msg) { mAltToolTipHintMsg = msg; }

	void			renderScaledPointGlobal( const LLVector3d& pos, const LLColor4U &color, F32 radius );

private:
	const LLVector3d& getObjectImageCenterGlobal()	{ return mObjectImageCenterGlobal; }
	void 			renderPoint(const LLVector3 &pos, const LLColor4U &color, 
								S32 diameter, S32 relative_height = 0);

	LLVector3		globalPosToView(const LLVector3d& global_pos);
	LLVector3d		viewPosToGlobal(S32 x,S32 y);

	void			drawTracking( const LLVector3d& pos_global, 
								  const LLColor4& color,
								  BOOL draw_arrow = TRUE);
    bool            isMouseOnPopupMenu();
    void            updateAboutLandPopupButton();
	BOOL			handleToolTipAgent(const LLUUID& avatar_id);
	static void		showAvatarInspector(const LLUUID& avatar_id);

	void			createObjectImage();

    F32             getScaleForName(std::string scale_name);
	static bool		outsideSlop(S32 x, S32 y, S32 start_x, S32 start_y, S32 slop);

private:
	bool			mUpdateNow;

	LLUIColor		mBackgroundColor;

	F32				mScale;					// Size of a region in pixels
	F32				mPixelsPerMeter;		// world meters to map pixels
	F32				mObjectMapTPM;			// texels per meter on map
	F32				mObjectMapPixels;		// Width of object map in pixels
	F32				mDotRadius;				// Size of avatar markers

    bool            mPanning; // map is being dragged
    bool            mCentering; // map is being re-centered around the agent
    LLVector2       mCurPan;
    LLVector2       mStartPan; // pan offset at start of drag
    LLVector3d      mPopupWorldPos; // world position picked under mouse when context menu is opened
    LLCoordGL       mMouseDown; // pointer position at start of drag

	LLVector3d		mObjectImageCenterGlobal;
	LLPointer<LLImageRaw> mObjectRawImagep;
	LLPointer<LLViewerTexture>	mObjectImagep;

	LLUUID			mClosestAgentToCursor;
	LLUUID			mClosestAgentAtLastRightClick;

    std::string     mToolTipMsg;
    std::string     mParcelNameMsg;
    std::string     mParcelSalePriceMsg;
    std::string     mParcelSaleAreaMsg;
    std::string     mParcelOwnerMsg;
    std::string     mRegionNameMsg;
    std::string     mToolTipHintMsg;
    std::string     mAltToolTipHintMsg;

public:
	void			setSelected(uuid_vec_t uuids) { gmSelected=uuids; };

private:
    bool isZoomChecked(const LLSD& userdata);
    void setZoom(const LLSD& userdata);
    void handleStopTracking(const LLSD& userdata);
    void activateCenterMap(const LLSD& userdata);
    bool isMapOrientationChecked(const LLSD& userdata);
    void setMapOrientation(const LLSD& userdata);
    void popupShowAboutLand(const LLSD& userdata);

    LLHandle<LLView> mPopupMenuHandle;
	uuid_vec_t		gmSelected;
};


#endif
