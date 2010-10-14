/** 
 * @file llstatgraph.cpp
 * @brief Simpler compact stat graph with tooltip
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

//#include "llviewerprecompiledheaders.h"
#include "linden_common.h"

#include "llstatgraph.h"
#include "llrender.h"

#include "llmath.h"
#include "llui.h"
#include "llstat.h"
#include "llgl.h"
#include "llglheaders.h"
//#include "llviewercontrol.h"

///////////////////////////////////////////////////////////////////////////////////

LLStatGraph::LLStatGraph(const LLView::Params& p)
:	LLView(p)
{
	mStatp = NULL;
	setToolTip(p.name());
	mNumThresholds = 3;
	mThresholdColors[0] = LLColor4(0.f, 1.f, 0.f, 1.f);
	mThresholdColors[1] = LLColor4(1.f, 1.f, 0.f, 1.f);
	mThresholdColors[2] = LLColor4(1.f, 0.f, 0.f, 1.f);
	mThresholdColors[3] = LLColor4(1.f, 0.f, 0.f, 1.f);
	mThresholds[0] = 50.f;
	mThresholds[1] = 75.f;
	mThresholds[2] = 100.f;
	mMin = 0.f;
	mMax = 125.f;
	mPerSec = TRUE;
	mValue = 0.f;
	mPrecision = 0;
}

void LLStatGraph::draw()
{
	F32 range, frac;
	range = mMax - mMin;
	if (mStatp)
	{
		if (mPerSec)
		{
			mValue = mStatp->getMeanPerSec();
		}
		else
		{
			mValue = mStatp->getMean();
		}
	}
	frac = (mValue - mMin) / range;
	frac = llmax(0.f, frac);
	frac = llmin(1.f, frac);

	if (mUpdateTimer.getElapsedTimeF32() > 0.5f)
	{
		std::string format_str;
		std::string tmp_str;
		format_str = llformat("%%s%%.%df%%s", mPrecision);
		tmp_str = llformat(format_str.c_str(), mLabel.c_str(), mValue, mUnits.c_str());
		setToolTip(tmp_str);

		mUpdateTimer.reset();
	}

	LLColor4 color;

	S32 i;
	for (i = 0; i < mNumThresholds - 1; i++)
	{
		if (mThresholds[i] > mValue)
		{
			break;
		}
	}

	//gl_drop_shadow(0,  getRect().getHeight(), getRect().getWidth(), 0,
	//				LLUIColorTable::instance().getColor("ColorDropShadow"), 
	//				(S32) gSavedSettings.getF32("DropShadowFloater") );

	color = LLUIColorTable::instance().getColor( "MenuDefaultBgColor" );
	gGL.color4fv(color.mV);
	gl_rect_2d(0, getRect().getHeight(), getRect().getWidth(), 0, TRUE);

	gGL.color4fv(LLColor4::black.mV);
	gl_rect_2d(0, getRect().getHeight(), getRect().getWidth(), 0, FALSE);
	
	color = mThresholdColors[i];
	gGL.color4fv(color.mV);
	gl_rect_2d(1, llround(frac*getRect().getHeight()), getRect().getWidth() - 1, 0, TRUE);
}

void LLStatGraph::setValue(const LLSD& value)
{
	mValue = (F32)value.asReal();
}

void LLStatGraph::setMin(const F32 min)
{
	mMin = min;
}

void LLStatGraph::setMax(const F32 max)
{
	mMax = max;
}

void LLStatGraph::setStat(LLStat *statp)
{
	mStatp = statp;
}

void LLStatGraph::setLabel(const std::string& label)
{
	mLabel = label;
}

void LLStatGraph::setUnits(const std::string& units)
{
	mUnits = units;
}

void LLStatGraph::setPrecision(const S32 precision)
{
	mPrecision = precision;
}

void LLStatGraph::setThreshold(const S32 i, F32 value)
{
	mThresholds[i] = value;
}
