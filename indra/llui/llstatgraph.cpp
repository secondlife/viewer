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
#include "lltracerecording.h"
//#include "llviewercontrol.h"

///////////////////////////////////////////////////////////////////////////////////

LLStatGraph::LLStatGraph(const Params& p)
:	LLView(p),
	mMin(p.min),
	mMax(p.max),
	mPerSec(true),
	mPrecision(p.precision),
	mValue(p.value),
	mStatp(p.stat.legacy_stat),
	mF32Statp(p.stat.rate_stat)
{
	setToolTip(p.name());

	for(LLInitParam::ParamIterator<ThresholdParams>::const_iterator it = p.thresholds.threshold.begin(), end_it = p.thresholds.threshold.end();
		it != end_it;
		++it)
	{
		mThresholds.push_back(Threshold(it->value(), it->color));
	}

	//mThresholdColors[0] = LLColor4(0.f, 1.f, 0.f, 1.f);
	//mThresholdColors[1] = LLColor4(1.f, 1.f, 0.f, 1.f);
	//mThresholdColors[2] = LLColor4(1.f, 0.f, 0.f, 1.f);
	//mThresholdColors[3] = LLColor4(1.f, 0.f, 0.f, 1.f);
	//mThresholds[0] = 50.f;
	//mThresholds[1] = 75.f;
	//mThresholds[2] = 100.f;
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
	else if (mF32Statp)
	{
		LLTrace::Recording* recording = LLTrace::get_thread_recorder()->getPrimaryRecording();

		if (mPerSec)
		{
			mValue = recording->getSum(*mF32Statp) / recording->getSampleTime();
		}
		else
		{
			mValue = recording->getSum(*mF32Statp);
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

	//S32 i;
	//for (i = 0; i < mNumThresholds - 1; i++)
	//{
	//	if (mThresholds[i] > mValue)
	//	{
	//		break;
	//	}
	//}

	threshold_vec_t::iterator it = std::lower_bound(mThresholds.begin(), mThresholds.end(), Threshold(mValue / mMax, LLUIColor()));

	if (it != mThresholds.begin())
	{
		it--;
	}

	color = LLUIColorTable::instance().getColor( "MenuDefaultBgColor" );
	gGL.color4fv(color.mV);
	gl_rect_2d(0, getRect().getHeight(), getRect().getWidth(), 0, TRUE);

	gGL.color4fv(LLColor4::black.mV);
	gl_rect_2d(0, getRect().getHeight(), getRect().getWidth(), 0, FALSE);
	
	color = it->mColor;
	gGL.color4fv(color.mV);
	gl_rect_2d(1, llround(frac*getRect().getHeight()), getRect().getWidth() - 1, 0, TRUE);
}

void LLStatGraph::setMin(const F32 min)
{
	mMin = min;
}

void LLStatGraph::setMax(const F32 max)
{
	mMax = max;
}

