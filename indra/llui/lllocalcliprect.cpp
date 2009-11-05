/** 
* @file lllocalcliprect.cpp
*
* $LicenseInfo:firstyear=2009&license=viewergpl$
* 
* Copyright (c) 2009, Linden Research, Inc.
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
#include "linden_common.h"

#include "lllocalcliprect.h"

#include "llfontgl.h"
#include "llgl.h"
#include "llui.h"

#include <stack>

//---------------------------------------------------------------------------
// LLScreenClipRect
// implementation class in screen space
//---------------------------------------------------------------------------
class LLScreenClipRect
{
public:
	LLScreenClipRect(const LLRect& rect, BOOL enabled = TRUE);
	virtual ~LLScreenClipRect();

private:
	static void pushClipRect(const LLRect& rect);
	static void popClipRect();
	static void updateScissorRegion();

private:
	LLGLState		mScissorState;
	BOOL			mEnabled;

	static std::stack<LLRect> sClipRectStack;
};

/*static*/ std::stack<LLRect> LLScreenClipRect::sClipRectStack;


LLScreenClipRect::LLScreenClipRect(const LLRect& rect, BOOL enabled)
:	mScissorState(GL_SCISSOR_TEST),
	mEnabled(enabled)
{
	if (mEnabled)
	{
		pushClipRect(rect);
	}
	mScissorState.setEnabled(!sClipRectStack.empty());
	updateScissorRegion();
}

LLScreenClipRect::~LLScreenClipRect()
{
	if (mEnabled)
	{
		popClipRect();
	}
	updateScissorRegion();
}

//static 
void LLScreenClipRect::pushClipRect(const LLRect& rect)
{
	LLRect combined_clip_rect = rect;
	if (!sClipRectStack.empty())
	{
		LLRect top = sClipRectStack.top();
		combined_clip_rect.intersectWith(top);

		if(combined_clip_rect.isEmpty())
		{
			// avoid artifacts where zero area rects show up as lines
			combined_clip_rect = LLRect::null;
		}
	}
	sClipRectStack.push(combined_clip_rect);
}

//static 
void LLScreenClipRect::popClipRect()
{
	sClipRectStack.pop();
}

//static
void LLScreenClipRect::updateScissorRegion()
{
	if (sClipRectStack.empty()) return;

	// finish any deferred calls in the old clipping region
	gGL.flush();

	LLRect rect = sClipRectStack.top();
	stop_glerror();
	S32 x,y,w,h;
	x = llfloor(rect.mLeft * LLUI::sGLScaleFactor.mV[VX]);
	y = llfloor(rect.mBottom * LLUI::sGLScaleFactor.mV[VY]);
	w = llmax(0, llceil(rect.getWidth() * LLUI::sGLScaleFactor.mV[VX])) + 1;
	h = llmax(0, llceil(rect.getHeight() * LLUI::sGLScaleFactor.mV[VY])) + 1;
	glScissor( x,y,w,h );
	stop_glerror();
}

//---------------------------------------------------------------------------
// LLLocalClipRect
//---------------------------------------------------------------------------
LLLocalClipRect::LLLocalClipRect(const LLRect& rect, BOOL enabled /* = TRUE */)
{
	LLRect screen(rect.mLeft + LLFontGL::sCurOrigin.mX, 
		rect.mTop + LLFontGL::sCurOrigin.mY, 
		rect.mRight + LLFontGL::sCurOrigin.mX, 
		rect.mBottom + LLFontGL::sCurOrigin.mY);
	mScreenClipRect = new LLScreenClipRect(screen, enabled);
}

LLLocalClipRect::~LLLocalClipRect()
{
	delete mScreenClipRect;
	mScreenClipRect = NULL;
}
