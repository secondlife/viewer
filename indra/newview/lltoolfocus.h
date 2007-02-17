/** 
 * @file lltoolfocus.h
 * @brief A tool to set the build focus point.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTOOLFOCUS_H
#define LL_LLTOOLFOCUS_H

#include "lltool.h"

class LLToolCamera
:	public LLTool
{
public:
	LLToolCamera();
	virtual ~LLToolCamera();

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);

	virtual void	onMouseCaptureLost();

	virtual void	handleSelect();
	virtual void	handleDeselect();

	virtual LLTool*	getOverrideTool(MASK mask) { return NULL; }

	static void pickCallback(S32 x, S32 y, MASK mask);
	BOOL mouseSteerMode() { return mMouseSteering; }

protected:
	// called from handleMouseUp and onMouseCaptureLost to "let go"
	// of the mouse and make it visible JC
	void releaseMouse();

protected:
	S32		mAccumX;
	S32		mAccumY;
	S32		mMouseDownX;
	S32		mMouseDownY;
	BOOL	mOutsideSlopX;
	BOOL	mOutsideSlopY;
	BOOL	mValidClickPoint;
	BOOL	mMouseSteering;
	S32		mMouseUpX;	// needed for releaseMouse()
	S32		mMouseUpY;
	MASK	mMouseUpMask;
};


extern LLToolCamera *gToolCamera;

extern BOOL gCameraBtnOrbit;
extern BOOL gCameraBtnPan;

#endif
