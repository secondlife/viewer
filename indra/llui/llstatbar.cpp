/** 
 * @file llstatbar.cpp
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

//#include "llviewerprecompiledheaders.h"
#include "linden_common.h"

#include "llstatbar.h"

#include "llmath.h"
#include "llui.h"
#include "llgl.h"
#include "llfontgl.h"

#include "lluictrlfactory.h"
#include "lltracerecording.h"
#include "llcriticaldamp.h"
#include "lltooltip.h"
#include "lllocalcliprect.h"
#include <iostream>
#include "lltrans.h"

// rate at which to update display of value that is rapidly changing
const F32 MEAN_VALUE_UPDATE_TIME = 1.f / 4.f; 
// time between value changes that qualifies as a "rapid change"
const F32Seconds	RAPID_CHANGE_THRESHOLD(0.2f); 
// maximum number of rapid changes in RAPID_CHANGE_WINDOW before switching over to displaying the mean 
// instead of latest value
const S32 MAX_RAPID_CHANGES_PER_SEC = 10;
// period of time over which to measure rapid changes
const F32Seconds RAPID_CHANGE_WINDOW(1.f);

F32 calc_tick_value(F32 min, F32 max)
{
	F32 range = max - min;
	const S32 DIVISORS[] = {6, 8, 10, 4, 5};
	// try storing 
	S32 best_decimal_digit_count = S32_MAX;
	S32 best_divisor = 10;
	for (U32 divisor_idx = 0; divisor_idx < LL_ARRAY_SIZE(DIVISORS); divisor_idx++)
	{
		S32 divisor = DIVISORS[divisor_idx];
		F32 possible_tick_value = range / divisor;
		S32 num_whole_digits = llceil(logf(llabs(min + possible_tick_value)) * OO_LN10);
		for (S32 digit_count = -(num_whole_digits - 1); digit_count < 6; digit_count++)
		{
			F32 test_tick_value = min + (possible_tick_value * pow(10.0, digit_count));

			if (is_approx_equal((F32)(S32)test_tick_value, test_tick_value))
			{
				if (digit_count < best_decimal_digit_count)
				{
					best_decimal_digit_count = digit_count;
					best_divisor = divisor;
				}
				break;
			}
		}
	}

	return is_approx_equal(range, 0.f) ? 0.f : range / best_divisor;
}

void calc_auto_scale_range(F32& min, F32& max, F32& tick)
{
	min = llmin(0.f, min, max);
	max = llmax(0.f, min, max);

	const F32 RANGES[] = {0.f, 1.f,   1.5f, 2.f, 3.f, 5.f, 10.f};
	const F32 TICKS[]  = {0.f, 0.25f, 0.5f, 1.f, 1.f, 1.f, 2.f };

	const S32 num_digits_max = is_approx_equal(llabs(max), 0.f)
							? S32_MIN + 1
							: llceil(logf(llabs(max)) * OO_LN10);
	const S32 num_digits_min = is_approx_equal(llabs(min), 0.f)
							? S32_MIN + 1
							: llceil(logf(llabs(min)) * OO_LN10);

	const S32 num_digits = llmax(num_digits_max, num_digits_min);
	const F32 power_of_10 = pow(10.0, num_digits - 1);
	const F32 starting_max = power_of_10 * ((max < 0.f) ? -1 : 1);
	const F32 starting_min = power_of_10 * ((min < 0.f) ? -1 : 1);

	F32 cur_max = starting_max;
	F32 cur_min = starting_min;
	F32 out_max = max;
	F32 out_min = min;

	F32 cur_tick_min = 0.f;
	F32 cur_tick_max = 0.f;

	for (S32 range_idx = 0; range_idx < LL_ARRAY_SIZE(RANGES); range_idx++)
	{
		cur_max = starting_max * RANGES[range_idx];
		cur_min = starting_min * RANGES[range_idx];

		if (min > 0.f && cur_min <= min)
		{
			out_min = cur_min;
			cur_tick_min = TICKS[range_idx];
		}
		if (max < 0.f && cur_max >= max)
		{
			out_max = cur_max;
			cur_tick_max = TICKS[range_idx];
		}
	}

	cur_max = starting_max;
	cur_min = starting_min;
	for (S32 range_idx = LL_ARRAY_SIZE(RANGES) - 1; range_idx >= 0; range_idx--)
	{
		cur_max = starting_max * RANGES[range_idx];
		cur_min = starting_min * RANGES[range_idx];

		if (min < 0.f && cur_min <= min)
		{
			out_min = cur_min;
			cur_tick_min = TICKS[range_idx];
		}
		if (max > 0.f && cur_max >= max)
		{
			out_max = cur_max;
			cur_tick_max = TICKS[range_idx];
		}	
	}

	tick = power_of_10 * llmax(cur_tick_min, cur_tick_max);
	min = out_min;
	max = out_max;
}

LLStatBar::Params::Params()
:	label("label"),
	unit_label("unit_label"),
	bar_min("bar_min", 0.f),
	bar_max("bar_max", 0.f),
	tick_spacing("tick_spacing", 0.f),
	decimal_digits("decimal_digits", 3),
	show_bar("show_bar", false),
	show_history("show_history", false),
	scale_range("scale_range", true),
	num_frames("num_frames", 200),
	num_frames_short("num_frames_short", 20),
	max_height("max_height", 100),
	stat("stat"),
	orientation("orientation", VERTICAL)
{
	changeDefault(follows.flags, FOLLOWS_TOP | FOLLOWS_LEFT);
}

///////////////////////////////////////////////////////////////////////////////////

LLStatBar::LLStatBar(const Params& p)
:	LLView(p),
	mLabel(p.label),
	mUnitLabel(p.unit_label),
	mTargetMinBar(llmin(p.bar_min, p.bar_max)),
	mTargetMaxBar(llmax(p.bar_max, p.bar_min)),
	mCurMaxBar(p.bar_max),
    mCurMinBar(0),
	mDecimalDigits(p.decimal_digits),
	mNumHistoryFrames(p.num_frames),
	mNumShortHistoryFrames(p.num_frames_short),
	mMaxHeight(p.max_height),
	mDisplayBar(p.show_bar),
	mDisplayHistory(p.show_history),
	mOrientation(p.orientation),
	mAutoScaleMax(!p.bar_max.isProvided()),
	mAutoScaleMin(!p.bar_min.isProvided()),
	mTickSpacing(p.tick_spacing),
	mLastDisplayValue(0.f),
	mStatType(STAT_NONE)
{
	mFloatingTargetMinBar = mTargetMinBar;
	mFloatingTargetMaxBar = mTargetMaxBar;

	mStat.valid = NULL;
	// tick value will be automatically calculated later
	if (!p.tick_spacing.isProvided() && p.bar_min.isProvided() && p.bar_max.isProvided())
	{
		mTickSpacing = calc_tick_value(mTargetMinBar, mTargetMaxBar);
	}

	setStat(p.stat);
}

BOOL LLStatBar::handleHover(S32 x, S32 y, MASK mask)
{
	switch(mStatType)
	{
	case STAT_COUNT:
		LLToolTipMgr::instance().show(LLToolTip::Params().message(mStat.countStatp->getDescription()).sticky_rect(calcScreenRect()));
		break;
	case STAT_EVENT:
		LLToolTipMgr::instance().show(LLToolTip::Params().message(mStat.eventStatp->getDescription()).sticky_rect(calcScreenRect()));
		break;
	case STAT_SAMPLE:
		LLToolTipMgr::instance().show(LLToolTip::Params().message(mStat.sampleStatp->getDescription()).sticky_rect(calcScreenRect()));
		break;
	case STAT_MEM:
		LLToolTipMgr::instance().show(LLToolTip::Params().message(mStat.memStatp->getDescription()).sticky_rect(calcScreenRect()));
		break;
	default:
		break;
	}
	return TRUE;
}

BOOL LLStatBar::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLView::handleMouseDown(x, y, mask);
	if (!handled)
	{
		if (mDisplayBar)
		{
			if (mDisplayHistory || mOrientation == HORIZONTAL)
			{
				mDisplayBar = FALSE;
				mDisplayHistory = FALSE;
			}
			else
			{
				mDisplayHistory = TRUE;
			}
		}
		else
		{
			mDisplayBar = TRUE;
			if (mOrientation == HORIZONTAL)
			{
				mDisplayHistory = TRUE;
			}
		}
		LLView* parent = getParent();
		parent->reshape(parent->getRect().getWidth(), parent->getRect().getHeight(), FALSE);
	}
	return TRUE;
}

template<typename T>
S32 calc_num_rapid_changes(LLTrace::PeriodicRecording& periodic_recording, const T& stat, const F32Seconds time_period)
{
	F32Seconds			elapsed_time,
						time_since_value_changed;
	S32					num_rapid_changes			= 0;
	const F32Seconds	RAPID_CHANGE_THRESHOLD		= F32Seconds(0.3f);
	F64					last_value					= periodic_recording.getPrevRecording(1).getLastValue(stat);

	for (S32 i = 2; i < periodic_recording.getNumRecordedPeriods(); i++)
	{
		LLTrace::Recording& recording = periodic_recording.getPrevRecording(i);
		F64 cur_value = recording.getLastValue(stat);

		if (last_value != cur_value)
		{
			if (time_since_value_changed < RAPID_CHANGE_THRESHOLD) num_rapid_changes++;
			time_since_value_changed = (F32Seconds)0;	
		}
		last_value = cur_value;

		elapsed_time += recording.getDuration();
		if (elapsed_time > time_period) break;
	}

	return num_rapid_changes;
}

void LLStatBar::draw()
{
	LLLocalClipRect _(getLocalRect());

	LLTrace::PeriodicRecording& frame_recording = LLTrace::get_frame_recording();
	LLTrace::Recording& last_frame_recording = frame_recording.getLastRecording(); 

	std::string unit_label;
	F32			current			= 0, 
				min				= 0, 
				max				= 0,
				mean			= 0,
				display_value	= 0;
	S32			num_frames		= mDisplayHistory 
								? mNumHistoryFrames 
								: mNumShortHistoryFrames;
	S32			num_rapid_changes = 0;
	S32			decimal_digits = mDecimalDigits;

	switch(mStatType)
	{
	case STAT_COUNT:
		{
			const LLTrace::StatType<LLTrace::CountAccumulator>& count_stat = *mStat.countStatp;

			unit_label    = std::string(count_stat.getUnitLabel()) + "/s";
			current       = last_frame_recording.getPerSec(count_stat);
			min           = frame_recording.getPeriodMinPerSec(count_stat, num_frames);
			max           = frame_recording.getPeriodMaxPerSec(count_stat, num_frames);
			mean          = frame_recording.getPeriodMeanPerSec(count_stat, num_frames);
			display_value = mean;
		}
		break;
	case STAT_EVENT:
		{
			const LLTrace::StatType<LLTrace::EventAccumulator>& event_stat = *mStat.eventStatp;

			unit_label        = mUnitLabel.empty() ? event_stat.getUnitLabel() : mUnitLabel;
			current           = last_frame_recording.getLastValue(event_stat);
			min               = frame_recording.getPeriodMin(event_stat, num_frames);
			max               = frame_recording.getPeriodMax(event_stat, num_frames);
			mean              = frame_recording.getPeriodMean(event_stat, num_frames);
			display_value     = mean;
		}
		break;
	case STAT_SAMPLE:
		{
			const LLTrace::StatType<LLTrace::SampleAccumulator>& sample_stat = *mStat.sampleStatp;

			unit_label        = mUnitLabel.empty() ? sample_stat.getUnitLabel() : mUnitLabel;
			current           = last_frame_recording.getLastValue(sample_stat);
			min               = frame_recording.getPeriodMin(sample_stat, num_frames);
			max               = frame_recording.getPeriodMax(sample_stat, num_frames);
			mean              = frame_recording.getPeriodMean(sample_stat, num_frames);
			num_rapid_changes = calc_num_rapid_changes(frame_recording, sample_stat, RAPID_CHANGE_WINDOW);

			if (num_rapid_changes / RAPID_CHANGE_WINDOW.value() > MAX_RAPID_CHANGES_PER_SEC)
			{
				display_value = mean;
			}
			else
			{
				display_value = current;
				// always display current value, don't rate limit
				mLastDisplayValue = current;
				if (is_approx_equal((F32)(S32)display_value, display_value))
				{
					decimal_digits = 0;
				}
			}
		}
		break;
	case STAT_MEM:
		{
			const LLTrace::StatType<LLTrace::MemAccumulator>& mem_stat = *mStat.memStatp;

			unit_label        = mUnitLabel.empty() ? mem_stat.getUnitLabel() : mUnitLabel;
			current           = last_frame_recording.getLastValue(mem_stat).value();
			min               = frame_recording.getPeriodMin(mem_stat, num_frames).value();
			max               = frame_recording.getPeriodMax(mem_stat, num_frames).value();
			mean              = frame_recording.getPeriodMean(mem_stat, num_frames).value();
			display_value	  = current;
		}
		break;
	default:
		break;
	}

	LLRect bar_rect;
	if (mOrientation == HORIZONTAL)
	{
		bar_rect.mTop	 = llmax(5, getRect().getHeight() - 15); 
		bar_rect.mLeft   = 0;
		bar_rect.mRight  = getRect().getWidth() - 40;
		bar_rect.mBottom = llmin(bar_rect.mTop - 5, 0);
	}
	else // VERTICAL
	{
		bar_rect.mTop    = llmax(5, getRect().getHeight() - 15); 
		bar_rect.mLeft   = 0;
		bar_rect.mRight  = getRect().getWidth();
		bar_rect.mBottom = llmin(bar_rect.mTop - 5, 20);
	}

	mCurMaxBar = LLSmoothInterpolation::lerp(mCurMaxBar, mTargetMaxBar, 0.05f);
	mCurMinBar = LLSmoothInterpolation::lerp(mCurMinBar, mTargetMinBar, 0.05f);

	// rate limited updates
	if (mLastDisplayValueTimer.getElapsedTimeF32() < MEAN_VALUE_UPDATE_TIME)
	{
		display_value = mLastDisplayValue;
	}
	else
	{
		mLastDisplayValueTimer.reset();
	}
	drawLabelAndValue(display_value, unit_label, bar_rect, decimal_digits);
	mLastDisplayValue = display_value;

	if (mDisplayBar && mStat.valid)
	{
		// Draw the tick marks.
		LLGLSUIDefault gls_ui;
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		F32 value_scale;
		if (mCurMaxBar == mCurMinBar)
		{
			value_scale = 0.f;
		}
		else
		{
			value_scale = (mOrientation == HORIZONTAL) 
				? (bar_rect.getHeight())/(mCurMaxBar - mCurMinBar)
				: (bar_rect.getWidth())/(mCurMaxBar - mCurMinBar);
		}

		drawTicks(min, max, value_scale, bar_rect);

		// draw background bar.
		gl_rect_2d(bar_rect.mLeft, bar_rect.mTop, bar_rect.mRight, bar_rect.mBottom, LLColor4(0.f, 0.f, 0.f, 0.25f));

		// draw values
		if (!llisnan(display_value) && frame_recording.getNumRecordedPeriods() != 0)
		{
			// draw min and max
			S32 begin = (S32) ((min - mCurMinBar) * value_scale);

			if (begin < 0)
			{
				begin = 0;
			}

			S32 end = (S32) ((max - mCurMinBar) * value_scale);
			if (mOrientation == HORIZONTAL)
			{
				gl_rect_2d(bar_rect.mLeft, end, bar_rect.mRight, begin, LLColor4(1.f, 0.f, 0.f, 0.25f));
			}
			else // VERTICAL
			{
				gl_rect_2d(begin, bar_rect.mTop, end, bar_rect.mBottom, LLColor4(1.f, 0.f, 0.f, 0.25f));
			}

			F32 span = (mOrientation == HORIZONTAL)
					? (bar_rect.getWidth())
					: (bar_rect.getHeight());

			if (mDisplayHistory && mStat.valid)
			{
				const S32 num_values = frame_recording.getNumRecordedPeriods() - 1;
				F32 min_value = 0.f,
					max_value = 0.f;

				gGL.color4f(1.f, 0.f, 0.f, 1.f);
				gGL.begin( LLRender::QUADS );
				const S32 max_frame = llmin(num_frames, num_values);
				U32 num_samples = 0;
				for (S32 i = 1; i <= max_frame; i++)
				{
					F32 offset = ((F32)i / (F32)num_frames) * span;
					LLTrace::Recording& recording = frame_recording.getPrevRecording(i);

					switch(mStatType)
					{
						case STAT_COUNT:
							min_value       = recording.getPerSec(*mStat.countStatp);
							max_value		= min_value;
							num_samples		= recording.getSampleCount(*mStat.countStatp);
							break;
						case STAT_EVENT:
							min_value       = recording.getMin(*mStat.eventStatp);
							max_value		= recording.getMax(*mStat.eventStatp);
							num_samples		= recording.getSampleCount(*mStat.eventStatp);
							break;
						case STAT_SAMPLE:
							min_value       = recording.getMin(*mStat.sampleStatp);
							max_value		= recording.getMax(*mStat.sampleStatp);
							num_samples		= recording.getSampleCount(*mStat.sampleStatp);
							break;
						case STAT_MEM:
							min_value       = recording.getMin(*mStat.memStatp).value();
							max_value		= recording.getMax(*mStat.memStatp).value();
							num_samples = 1;
							break;
						default:
							break;
					}

					if (!num_samples) continue;

					F32 min = (min_value  - mCurMinBar) * value_scale;
					F32 max = llmax(min + 1, (max_value - mCurMinBar) * value_scale);
					if (mOrientation == HORIZONTAL)
					{
						gGL.vertex2f((F32)bar_rect.mRight - offset, max);
						gGL.vertex2f((F32)bar_rect.mRight - offset, min);
						gGL.vertex2f((F32)bar_rect.mRight - offset - 1, min);
						gGL.vertex2f((F32)bar_rect.mRight - offset - 1, max);
					}
					else
					{
						gGL.vertex2f(min, (F32)bar_rect.mBottom + offset + 1);
						gGL.vertex2f(min, (F32)bar_rect.mBottom + offset);
						gGL.vertex2f(max, (F32)bar_rect.mBottom + offset);
						gGL.vertex2f(max, (F32)bar_rect.mBottom + offset + 1 );
					}
				}
				gGL.end();
			}
			else
			{
				S32 begin = (S32) ((current - mCurMinBar) * value_scale) - 1;
				S32 end = (S32) ((current - mCurMinBar) * value_scale) + 1;
				// draw current
				if (mOrientation == HORIZONTAL)
				{
					gl_rect_2d(bar_rect.mLeft, end, bar_rect.mRight, begin, LLColor4(1.f, 0.f, 0.f, 1.f));
				}
				else
				{
					gl_rect_2d(begin, bar_rect.mTop, end, bar_rect.mBottom, LLColor4(1.f, 0.f, 0.f, 1.f));
				}
			}

			// draw mean bar
			{
				const S32 begin = (S32) ((mean - mCurMinBar) * value_scale) - 1;
				const S32 end = (S32) ((mean - mCurMinBar) * value_scale) + 1;
				if (mOrientation == HORIZONTAL)
				{
					gl_rect_2d(bar_rect.mLeft - 2, begin, bar_rect.mRight + 2, end, LLColor4(0.f, 1.f, 0.f, 1.f));
				}
				else
				{
					gl_rect_2d(begin, bar_rect.mTop + 2, end, bar_rect.mBottom - 2, LLColor4(0.f, 1.f, 0.f, 1.f));
				}
			}
		}
	}
	
	LLView::draw();
}

void LLStatBar::setStat(const std::string& stat_name)
{
	using namespace LLTrace;
	const StatType<CountAccumulator>*	count_stat;
	const StatType<EventAccumulator>*	event_stat;
	const StatType<SampleAccumulator>*	sample_stat;
	const StatType<MemAccumulator>*		mem_stat;

	if ((count_stat = StatType<CountAccumulator>::getInstance(stat_name)))
	{
		mStat.countStatp = count_stat;
		mStatType = STAT_COUNT;
	}
	else if ((event_stat = StatType<EventAccumulator>::getInstance(stat_name)))
	{
		mStat.eventStatp = event_stat;
		mStatType = STAT_EVENT;
	}
	else if ((sample_stat = StatType<SampleAccumulator>::getInstance(stat_name)))
	{
		mStat.sampleStatp = sample_stat;
		mStatType = STAT_SAMPLE;
	}
	else if ((mem_stat = StatType<MemAccumulator>::getInstance(stat_name)))
	{
		mStat.memStatp = mem_stat;
		mStatType = STAT_MEM;
	}
}


void LLStatBar::setRange(F32 bar_min, F32 bar_max)
{
	mTargetMinBar		= llmin(bar_min, bar_max);
	mTargetMaxBar		= llmax(bar_min, bar_max);
	mFloatingTargetMinBar = mTargetMinBar;
	mFloatingTargetMaxBar = mTargetMaxBar;
	mTickSpacing	= calc_tick_value(mTargetMinBar, mTargetMaxBar);
}

LLRect LLStatBar::getRequiredRect()
{
	LLRect rect;

	if (mDisplayBar)
	{
		if (mDisplayHistory)
		{
			rect.mTop = mMaxHeight;
		}
		else
		{
			rect.mTop = 40;
		}
	}
	else
	{
		rect.mTop = 14;
	}
	return rect;
}

void LLStatBar::drawLabelAndValue( F32 value, std::string &label, LLRect &bar_rect, S32 decimal_digits )
{
	LLFontGL::getFontMonospace()->renderUTF8(mLabel, 0, 0, getRect().getHeight(), LLColor4(1.f, 1.f, 1.f, 1.f),
		LLFontGL::LEFT, LLFontGL::TOP);

	std::string value_str	= !llisnan(value)
							? llformat("%10.*f %s", decimal_digits, value, label.c_str())
							: LLTrans::getString("na");

	// Draw the current value.
	if (mOrientation == HORIZONTAL)
	{
		LLFontGL::getFontMonospace()->renderUTF8(value_str, 0, bar_rect.mRight, getRect().getHeight(), 
			LLColor4(1.f, 1.f, 1.f, 1.f),
			LLFontGL::RIGHT, LLFontGL::TOP);
	}
	else
	{
		LLFontGL::getFontMonospace()->renderUTF8(value_str, 0, bar_rect.mRight, getRect().getHeight(), 
			LLColor4(1.f, 1.f, 1.f, 1.f),
			LLFontGL::RIGHT, LLFontGL::TOP);
	}
}

void LLStatBar::drawTicks( F32 min, F32 max, F32 value_scale, LLRect &bar_rect )
{
	if (!llisnan(min) && (mAutoScaleMax || mAutoScaleMin))
	{
		F32 u = LLSmoothInterpolation::getInterpolant(10.f);
		mFloatingTargetMinBar = llmin(min, lerp(mFloatingTargetMinBar, min, u));
		mFloatingTargetMaxBar = llmax(max, lerp(mFloatingTargetMaxBar, max, u));
		F32 range_min = mAutoScaleMin ? mFloatingTargetMinBar : mTargetMinBar;
		F32 range_max = mAutoScaleMax ? mFloatingTargetMaxBar : mTargetMaxBar;
		F32 tick_value = 0.f;
		calc_auto_scale_range(range_min, range_max, tick_value);
		if (mAutoScaleMin) { mTargetMinBar = range_min; }
		if (mAutoScaleMax) { mTargetMaxBar = range_max; }
		if (mAutoScaleMin && mAutoScaleMax)
		{
			mTickSpacing = tick_value;
		}
		else
		{
			mTickSpacing = calc_tick_value(mTargetMinBar, mTargetMaxBar);
		}
	}

	// start counting from actual min, not current, animating min, so that ticks don't float between numbers
	// ensure ticks always hit 0
	S32 last_tick = S32_MIN;
	S32 last_label = S32_MIN;
	if (mTickSpacing > 0.f && value_scale > 0.f)
	{
		const S32 MIN_TICK_SPACING  = mOrientation == HORIZONTAL ? 20 : 30;
		const S32 MIN_LABEL_SPACING = mOrientation == HORIZONTAL ? 30 : 60;
		const S32 TICK_LENGTH = 4;
		const S32 TICK_WIDTH = 1;

		F32 start = mCurMinBar < 0.f
			? llceil(-mCurMinBar / mTickSpacing) * -mTickSpacing
			: 0.f;
		for (F32 tick_value = start; ;tick_value += mTickSpacing)
		{
			// clamp to S32_MAX / 2 to avoid floating point to integer overflow resulting in S32_MIN
			const S32 tick_begin = llfloor(llmin((F32)(S32_MAX / 2), (tick_value - mCurMinBar)*value_scale));
			const S32 tick_end = tick_begin + TICK_WIDTH;
			if (tick_begin < last_tick + MIN_TICK_SPACING)
			{
				continue;
			}
			last_tick = tick_begin;

			S32 decimal_digits = mDecimalDigits;
			if (is_approx_equal((F32)(S32)tick_value, tick_value))
			{
				decimal_digits = 0;
			}
			std::string tick_label = llformat("%.*f", decimal_digits, tick_value);
			S32 tick_label_width = LLFontGL::getFontMonospace()->getWidth(tick_label);
			if (mOrientation == HORIZONTAL)
			{
				if (tick_begin > last_label + MIN_LABEL_SPACING)
				{
					gl_rect_2d(bar_rect.mLeft, tick_end, bar_rect.mRight - TICK_LENGTH, tick_begin, LLColor4(1.f, 1.f, 1.f, 0.25f));
					LLFontGL::getFontMonospace()->renderUTF8(tick_label, 0, bar_rect.mRight, tick_begin,
						LLColor4(1.f, 1.f, 1.f, 0.5f),
						LLFontGL::LEFT, LLFontGL::VCENTER);
					last_label = tick_begin;
				}
				else
				{
					gl_rect_2d(bar_rect.mLeft, tick_end, bar_rect.mRight - TICK_LENGTH/2, tick_begin, LLColor4(1.f, 1.f, 1.f, 0.1f));
				}
			}
			else
			{
				if (tick_begin > last_label + MIN_LABEL_SPACING)
				{
					gl_rect_2d(tick_begin, bar_rect.mTop, tick_end, bar_rect.mBottom - TICK_LENGTH, LLColor4(1.f, 1.f, 1.f, 0.25f));
					S32 label_pos = tick_begin - ll_round((F32)tick_label_width * ((F32)tick_begin / (F32)bar_rect.getWidth()));
					LLFontGL::getFontMonospace()->renderUTF8(tick_label, 0, label_pos, bar_rect.mBottom - TICK_LENGTH,
						LLColor4(1.f, 1.f, 1.f, 0.5f),
						LLFontGL::LEFT, LLFontGL::TOP);
					last_label = label_pos;
				}
				else
				{
					gl_rect_2d(tick_begin, bar_rect.mTop, tick_end, bar_rect.mBottom - TICK_LENGTH/2, LLColor4(1.f, 1.f, 1.f, 0.1f));
				}
			}
			// always draw one tick value past tick_end, so we can see part of the text, if possible
			if (tick_value > mCurMaxBar)
			{
				break;
			}
		}
	}
}
