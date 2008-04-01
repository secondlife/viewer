/** 
 * @file lltoolfocus.h
 * @brief A tool to set the build focus point.
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

#ifndef LL_LLTOOLFOCUS_H
#define LL_LLTOOLFOCUS_H

#include "lltool.h"

class LLToolCamera
:	public LLTool, public LLSingleton<LLToolCamera>
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


extern BOOL gCameraBtnOrbit;
extern BOOL gCameraBtnPan;

#endif
