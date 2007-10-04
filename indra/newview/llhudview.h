/** 
 * @file llhudview.h
 * @brief 2D HUD overlay
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
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

#ifndef LL_LLHUDVIEW_H
#define LL_LLHUDVIEW_H

#include "llview.h"
#include "v4color.h"

class LLVector3d;

class LLHUDView
: public LLView
{
public:
	LLHUDView(const std::string& name, const LLRect& rect);
	virtual ~LLHUDView();

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	virtual void draw();

	const LLColor4& colorFromType(S32 type);

protected:
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);
};

extern LLHUDView *gHUDView;

#endif
