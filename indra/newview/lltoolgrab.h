/** 
 * @file lltoolgrab.h
 * @brief LLToolGrab class header file
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

#ifndef LL_TOOLGRAB_H
#define LL_TOOLGRAB_H

#include "lltool.h"
#include "v3math.h"
#include "llquaternion.h"
#include "llmemory.h"
#include "lluuid.h"
#include "llviewerwindow.h" // for LLPickInfo

class LLView;
class LLTextBox;
class LLViewerObject;
class LLPickInfo;

class LLToolGrab : public LLTool, public LLSingleton<LLToolGrab>
{
public:
	LLToolGrab( LLToolComposite* composite = NULL );
	~LLToolGrab();

	/*virtual*/ BOOL	handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);
	/*virtual*/ void	render();		// 3D elements
	/*virtual*/ void	draw();			// 2D elements

	virtual void		handleSelect();
	virtual void		handleDeselect();
	
	virtual LLViewerObject*	getEditingObject();
	virtual LLVector3d		getEditingPointGlobal();
	virtual BOOL			isEditing();
	virtual void			stopEditing();
	
	virtual void			onMouseCaptureLost();

	BOOL			hasGrabOffset()  { return TRUE; }	// HACK
	LLVector3		getGrabOffset(S32 x, S32 y);		// HACK

	// Capture the mouse and start grabbing.
	BOOL			handleObjectHit(const LLPickInfo& info);

	// Certain grabs should not highlight the "Build" toolbar button
	BOOL getHideBuildHighlight() { return mHideBuildHighlight; }

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

	BOOL			mVerticalDragging;

	BOOL			mHitLand;

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
	BOOL			mHasMoved;		// has mouse moved off center at all?
	BOOL			mOutsideSlop;	// has mouse moved outside center 5 pixels?
	BOOL			mDeselectedThisClick;

	S32             mLastFace;
	LLVector2       mLastUVCoords;
	LLVector2       mLastSTCoords;
	LLVector3       mLastIntersection;
	LLVector3       mLastNormal;
	LLVector3       mLastBinormal;
	LLVector3       mLastGrabPos;


	BOOL			mSpinGrabbing;
	LLQuaternion	mSpinRotation;

	BOOL			mHideBuildHighlight;
};

extern BOOL gGrabBtnVertical;
extern BOOL gGrabBtnSpin;
extern LLTool* gGrabTransientTool;

#endif  // LL_TOOLGRAB_H


