/** 
 * @file llmemoryview.h
 * @brief LLMemoryView class definition
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

#ifndef LL_LLMEMORYVIEW_H
#define LL_LLMEMORYVIEW_H

#include "llview.h"

class LLAllocator;

class LLMemoryView : public LLView
{
public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Params()
		{
			mouse_opaque = true;
			visible = false;
		}
	};
	LLMemoryView(const LLMemoryView::Params&);
	virtual ~LLMemoryView();

	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL handleHover(S32 x, S32 y, MASK mask);
	virtual void draw();

	void refreshProfile();

private:
    std::vector<LLWString> mLines;
	LLAllocator* mAlloc;
	BOOL mPaused ;

};

#endif
