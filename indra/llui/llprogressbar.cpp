/** 
 * @file llprogressbar.cpp
 * @brief LLProgressBar class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llprogressbar.h"

#include "indra_constants.h"
#include "llmath.h"
#include "llgl.h"
#include "llui.h"
#include "llfontgl.h"
#include "lltimer.h"
#include "llglheaders.h"

#include "llfocusmgr.h"
#include "lluictrlfactory.h"

static LLDefaultChildRegistry::Register<LLProgressBar> r("progress_bar");

LLProgressBar::Params::Params()
:	image_bar("image_bar"),
	image_fill("image_fill"),
	color_bar("color_bar"),
	color_bg("color_bg")
{}


LLProgressBar::LLProgressBar(const LLProgressBar::Params& p) 
:	LLUICtrl(p),
	mImageBar(p.image_bar),
	mImageFill(p.image_fill),
	mColorBackground(p.color_bg()),
	mColorBar(p.color_bar()),
	mPercentDone(0.f)
{}

LLProgressBar::~LLProgressBar()
{
	gFocusMgr.releaseFocusIfNeeded( this );
}

void LLProgressBar::draw()
{
	static LLTimer timer;
	F32 alpha = getDrawContext().mAlpha;
	
	LLColor4 image_bar_color = mColorBackground.get();
	image_bar_color.setAlpha(alpha);
	mImageBar->draw(getLocalRect(), image_bar_color);

	alpha *= 0.5f + 0.5f*0.5f*(1.f + (F32)sin(3.f*timer.getElapsedTimeF32()));
	LLColor4 bar_color = mColorBar.get();
	bar_color.mV[VALPHA] *= alpha; // modulate alpha
	LLRect progress_rect = getLocalRect();
	progress_rect.mRight = llround(getRect().getWidth() * (mPercentDone / 100.f));
	mImageFill->draw(progress_rect, bar_color);
}

void LLProgressBar::setValue(const LLSD& value)
{
	mPercentDone = llclamp((F32)value.asReal(), 0.f, 100.f);
}
