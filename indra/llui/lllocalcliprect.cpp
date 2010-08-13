/** 
* @file lllocalcliprect.cpp
*
* $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
