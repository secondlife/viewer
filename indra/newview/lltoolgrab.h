/** 
 * @file lltoolgrab.h
 * @brief LLToolGrab class header file
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

#ifndef LL_TOOLGRAB_H
#define LL_TOOLGRAB_H

#include "lltool.h"
#include "v3math.h"
#include "llquaternion.h"
#include "llsingleton.h"
#include "lluuid.h"
#include "llviewerwindow.h" // for LLPickInfo

class LLView;
class LLTextBox;
class LLViewerObject;
class LLPickInfo;


// Message utilities
void send_ObjectGrab_message(LLViewerObject* object, const LLPickInfo & pick, const LLVector3 &grab_offset);
void send_ObjectDeGrab_message(LLViewerObject* object, const LLPickInfo & pick);

const MASK DEFAULT_GRAB_MASK = MASK_CONTROL;

/**
 * LLToolGrabBase contains most of the semantics of LLToolGrab. It's just that
 * LLToolGrab is an LLSingleton, but we also explicitly instantiate
 * LLToolGrabBase as part of LLToolCompGun. You can't just make an extra
 * instance of an LLSingleton!
 */
class LLToolGrabBase : public LLTool
{
public:
	LLToolGrabBase(LLToolComposite* composite=NULL);
	~LLToolGrabBase();

	/*virtual*/ bool	handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ bool	handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ bool	handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ bool	handleDoubleClick(S32 x, S32 y, MASK mask);
	/*virtual*/ void	render();		// 3D elements
	/*virtual*/ void	draw();			// 2D elements

	virtual void		handleSelect();
	virtual void		handleDeselect();
	
	virtual LLViewerObject*	getEditingObject();
	virtual LLVector3d		getEditingPointGlobal();
	virtual bool			isEditing();
	virtual void			stopEditing();
	
	virtual void			onMouseCaptureLost();

	bool			hasGrabOffset()  { return true; }	// HACK
	LLVector3		getGrabOffset(S32 x, S32 y);		// HACK

	// Capture the mouse and start grabbing.
	bool			handleObjectHit(const LLPickInfo& info);

	// Certain grabs should not highlight the "Build" toolbar button
	bool getHideBuildHighlight() { return mHideBuildHighlight; }

	void setClickedInMouselook(bool is_clickedInMouselook) {mClickedInMouselook = is_clickedInMouselook;}

	static void		pickCallback(const LLPickInfo& pick_info);
private:
	LLVector3d		getGrabPointGlobal();
	void			startGrab();
	void			stopGrab();

	void			startSpin();
	void			stopSpin();

	void			handleHoverSpin(S32 x, S32 y, MASK mask);
	void			handleHoverActive(S32 x, S32 y, MASK mask);
	void			handleHoverNonPhysical(S32 x, S32 y, MASK mask);
	void			handleHoverInactive(S32 x, S32 y, MASK mask);
	void			handleHoverFailed(S32 x, S32 y, MASK mask);

private:
	enum			EGrabMode { GRAB_INACTIVE, GRAB_ACTIVE_CENTER, GRAB_NONPHYSICAL, GRAB_LOCKED, GRAB_NOOBJECT };

	EGrabMode		mMode;

	bool			mVerticalDragging;

	bool			mHitLand;

	LLTimer			mGrabTimer;						// send simulator time between hover movements

	LLVector3		mGrabOffsetFromCenterInitial;	// meters from CG of object
	LLVector3d		mGrabHiddenOffsetFromCamera;	// in cursor hidden drag, how far is grab offset from camera

	LLVector3d		mDragStartPointGlobal;				// projected into world
	LLVector3d		mDragStartFromCamera;			// drag start relative to camera

	LLPickInfo		mGrabPick;

	S32				mLastMouseX;
	S32				mLastMouseY;
	S32				mAccumDeltaX;	// since cursor hidden, how far have you moved?
	S32				mAccumDeltaY;
	bool			mHasMoved;		// has mouse moved off center at all?
	bool			mOutsideSlop;	// has mouse moved outside center 5 pixels?
	bool			mDeselectedThisClick;
	bool			mValidSelection;

	S32             mLastFace;
	LLVector2       mLastUVCoords;
	LLVector2       mLastSTCoords;
	LLVector3       mLastIntersection;
	LLVector3       mLastNormal;
	LLVector3       mLastBinormal;
	LLVector3       mLastGrabPos;


	bool			mSpinGrabbing;
	LLQuaternion	mSpinRotation;

	bool			mHideBuildHighlight;

	bool			mClickedInMouselook;
};

/// This is the LLSingleton instance of LLToolGrab.
class LLToolGrab : public LLToolGrabBase, public LLSingleton<LLToolGrab>
{
	LLSINGLETON_EMPTY_CTOR(LLToolGrab);
};

extern bool gGrabBtnVertical;
extern bool gGrabBtnSpin;
extern LLTool* gGrabTransientTool;

#endif  // LL_TOOLGRAB_H
