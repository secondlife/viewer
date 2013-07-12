/** 
 * @file llstatgraph.h
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

#ifndef LL_LLSTATGRAPH_H
#define LL_LLSTATGRAPH_H

#include "llview.h"
#include "llframetimer.h"
#include "v4color.h"

class LLStat;

class LLStatGraph : public LLView
{
public:
	LLStatGraph(const LLView::Params&);

	virtual void draw();

	void setLabel(const std::string& label);
	void setUnits(const std::string& units);
	void setPrecision(const S32 precision);
	void setStat(LLStat *statp);
	void setThreshold(const S32 i, F32 value);
	void setMin(const F32 min);
	void setMax(const F32 max);

	/*virtual*/ void setValue(const LLSD& value);
	
	LLStat *mStatp;
	BOOL mPerSec;
private:
	F32 mValue;

	F32 mMin;
	F32 mMax;
	LLFrameTimer mUpdateTimer;
	std::string mLabel;
	std::string mUnits;
	S32 mPrecision; // Num of digits of precision after dot

	S32 mNumThresholds;
	F32 mThresholds[4];
	LLColor4 mThresholdColors[4];
};

#endif  // LL_LLSTATGRAPH_H
