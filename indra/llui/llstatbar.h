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
								tick_spacing;

		Optional<bool>			show_bar,
								show_history,
								scale_range;

		Optional<S32>			decimal_digits,
								num_frames,
								num_frames_short,
								max_height;
		Optional<std::string>	stat;
		Optional<EOrientation>	orientation;

		Params();
	};
	LLStatBar(const Params&);

	virtual void draw();
	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleHover(S32 x, S32 y, MASK mask);

	void setStat(const std::string& stat_name);

	void setRange(F32 bar_min, F32 bar_max);
	void getRange(F32& bar_min, F32& bar_max) { bar_min = mTargetMinBar; bar_max = mTargetMaxBar; }
	
	/*virtual*/ LLRect getRequiredRect();	// Return the height of this object, given the set options.

private:
	void drawLabelAndValue( F32 mean, std::string &unit_label, LLRect &bar_rect, S32 decimal_digits );
	void drawTicks( F32 min, F32 max, F32 value_scale, LLRect &bar_rect );

	F32          mTargetMinBar,
				 mTargetMaxBar,
				 mFloatingTargetMinBar,
				 mFloatingTargetMaxBar,
				 mCurMaxBar,
				 mCurMinBar,
				 mTickSpacing;
	S32          mDecimalDigits,
				 mNumHistoryFrames,
				 mNumShortHistoryFrames;
	S32			 mMaxHeight;
	EOrientation mOrientation;
	F32			 mLastDisplayValue;
	LLFrameTimer mLastDisplayValueTimer;

	enum
	{
		STAT_NONE,
		STAT_COUNT,
		STAT_EVENT,
		STAT_SAMPLE,
		STAT_MEM
	} mStatType;

	union
	{
		void*														valid;
		const LLTrace::StatType<LLTrace::CountAccumulator>*		countStatp;
		const LLTrace::StatType<LLTrace::EventAccumulator>*		eventStatp;
		const LLTrace::StatType<LLTrace::SampleAccumulator>*		sampleStatp;
		const LLTrace::StatType<LLTrace::MemAccumulator>*		memStatp;
	} mStat;

	LLUIString   mLabel;
	std::string  mUnitLabel;

	bool         mDisplayBar,			// Display the bar graph.
				 mDisplayHistory,
				 mAutoScaleMax,
				 mAutoScaleMin;
};

#endif
