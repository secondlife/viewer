/** 
 * @file lltoolgrab.h
 * @brief LLToolGrab class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_TOOLGRAB_H
#define LL_TOOLGRAB_H

#include "lltool.h"
#include "v3math.h"
#include "llquaternion.h"
#include "llmemory.h"
#include "lluuid.h"

class LLView;
class LLTextBox;
class LLViewerObject;

class LLToolGrab : public LLTool
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
	BOOL			handleObjectHit(LLViewerObject *objectp, S32 x, S32 y, MASK mask);

	// Certain grabs should not highlight the "Build" toolbar button
	BOOL getHideBuildHighlight() { return mHideBuildHighlight; }

	static void		pickCallback(S32 x, S32 y, MASK mask);
private:
	LLVector3d		getGrabPointGlobal();
	void			startGrab(S32 x, S32 y);
	void			stopGrab();

	void			startSpin();
	void			stopSpin();

	void			handleHoverSpin(S32 x, S32 y, MASK mask);
	void			handleHoverActive(S32 x, S32 y, MASK mask);
	void			handleHoverInactive(S32 x, S32 y, MASK mask);
	void			handleHoverFailed(S32 x, S32 y, MASK mask);

private:
	enum			EGrabMode { GRAB_INACTIVE, GRAB_ACTIVE_CENTER, GRAB_NONPHYSICAL, GRAB_LOCKED, GRAB_NOOBJECT };

	EGrabMode		mMode;

	BOOL			mVerticalDragging;

	BOOL			mHitLand;
	LLUUID			mHitObjectID;				// if hit something, its ID

	LLPointer<LLViewerObject>	mGrabObject;					// the object currently being grabbed
	LLTimer			mGrabTimer;						// send simulator time between hover movements

	LLVector3		mGrabOffsetFromCenterInitial;	// meters from CG of object
	LLVector3		mGrabOffset;					// how far cursor currently is from grab start point, meters
	LLVector3d		mGrabHiddenOffsetFromCamera;	// in cursor hidden drag, how far is grab offset from camera

	LLVector3d		mDragStartPointGlobal;				// projected into world
	LLVector3d		mDragStartFromCamera;			// drag start relative to camera

	S32				mLastMouseX;
	S32				mLastMouseY;
	S32				mMouseDownX;
	S32				mMouseDownY;
	MASK			mMouseMask;
	S32				mAccumDeltaX;	// since cursor hidden, how far have you moved?
	S32				mAccumDeltaY;
	BOOL			mHasMoved;		// has mouse moved off center at all?
	BOOL			mOutsideSlop;	// has mouse moved outside center 5 pixels?
	BOOL			mDeselectedThisClick;

	BOOL			mSpinGrabbing;
	LLQuaternion	mSpinRotation;

	BOOL			mHideBuildHighlight;
};

extern LLToolGrab *gToolGrab;

extern BOOL gGrabBtnVertical;
extern BOOL gGrabBtnSpin;
extern LLTool* gGrabTransientTool;

#endif  // LL_TOOLGRAB_H

