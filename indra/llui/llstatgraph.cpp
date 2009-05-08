/** 
 * @file llstatgraph.cpp
 * @brief Simpler compact stat graph with tooltip
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
	//				gSavedSkinSettings.getColor("ColorDropShadow"), 
	//				(S32) gSavedSettings.getF32("DropShadowFloater") );

	color = LLUI::sSettingGroups["color"]->getColor( "MenuDefaultBgColor" );
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
