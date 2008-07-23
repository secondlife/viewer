/** 
 * @file lltoolselectrect.h
 * @brief A tool to select multiple objects with a screen-space rectangle.
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

#ifndef LL_LLTOOLSELECTRECT_H
#define LL_LLTOOLSELECTRECT_H

#include "lltool.h"
#include "lltoolselect.h"

class LLToolSelectRect
:	public LLToolSelect
{
public:
	LLToolSelectRect( LLToolComposite* composite );

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual void	draw();							// draw the select rectangle

	void handlePick(const LLPickInfo& pick);

protected:
	void			handleRectangleSelection(S32 x, S32 y, MASK mask);	// true if you selected one
	BOOL			outsideSlop(S32 x, S32 y, S32 start_x, S32 start_y);

protected:
	S32				mDragStartX;					// screen coords, from left
	S32				mDragStartY;					// screen coords, from bottom

	S32				mDragEndX;
	S32				mDragEndY;

	S32				mDragLastWidth;
	S32				mDragLastHeight;

	BOOL			mMouseOutsideSlop;		// has mouse ever gone outside slop region?
};


#endif
