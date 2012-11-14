/** 
 * @file llstatbar.h
 * @brief A little map of the world with network information
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

#ifndef LL_LLSTATBAR_H
#define LL_LLSTATBAR_H

#include "llview.h"
#include "llframetimer.h"
#include "lltracerecording.h"

class LLStatBar : public LLView
{
public:

	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<std::string>	label,
								unit_label;

		Optional<F32>			bar_min,
								bar_max,
								tick_spacing,
								label_spacing,
								update_rate,
								unit_scale;

		Optional<U32>			precision;

		Optional<bool>			show_per_sec,
								show_bar,
								show_history,
								show_mean;

		Optional<std::string>	stat;

		Params()
			: label("label"),
			  unit_label("unit_label"),
			  bar_min("bar_min", 0.0f),
			  bar_max("bar_max", 50.0f),
			  tick_spacing("tick_spacing", 10.0f),
			  label_spacing("label_spacing", 10.0f),
			  precision("precision", 0),
			  update_rate("update_rate", 5.0f),
			  unit_scale("unit_scale", 1.f),
			  show_per_sec("show_per_sec", true),
			  show_bar("show_bar", TRUE),
			  show_history("show_history", false),
			  show_mean("show_mean", true),
			  stat("stat")
		{
			changeDefault(follows.flags, FOLLOWS_TOP | FOLLOWS_LEFT);
		}
	};
	LLStatBar(const Params&);

	virtual void draw();
	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);

	void setStat(const std::string& stat_name);

	void setRange(F32 bar_min, F32 bar_max, F32 tick_spacing, F32 label_spacing);
	void getRange(F32& bar_min, F32& bar_max) { bar_min = mMinBar; bar_max = mMaxBar; }
	
	/*virtual*/ LLRect getRequiredRect();	// Return the height of this object, given the set options.

private:
	F32 mMinBar;
	F32 mMaxBar;
	F32 mTickSpacing;
	F32 mLabelSpacing;
	U32 mPrecision;
	F32 mUpdatesPerSec;
	F32 mUnitScale;
	BOOL mPerSec;				// Use the per sec stats.
	BOOL mDisplayBar;			// Display the bar graph.
	BOOL mDisplayHistory;
	BOOL mDisplayMean;			// If true, display mean, if false, display current value

	LLTrace::TraceType<CountAccumulator<F64> >* mCountFloatp;
	LLTrace::TraceType<CountAccumulator<S64> >* mCountIntp;
	LLTrace::TraceType<MeasurementAccumulator<F64> >* mMeasurementFloatp;
	LLTrace::TraceType<MeasurementAccumulator<S64> >* mMeasurementIntp;

	LLFrameTimer mUpdateTimer;
	LLUIString mLabel;
	std::string mUnitLabel;
	F32 mValue;
};

#endif
