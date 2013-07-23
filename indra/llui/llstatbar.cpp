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
		S32 num_whole_digits = llceil(logf(min + possible_tick_value) * OO_LN10);
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

///////////////////////////////////////////////////////////////////////////////////

LLStatBar::LLStatBar(const Params& p)
:	LLView(p),
	mLabel(p.label),
	mUnitLabel(p.unit_label),
	mMinBar(llmin(p.bar_min, p.bar_max)),
	mMaxBar(llmax(p.bar_max, p.bar_min)),
	mCurMaxBar(p.bar_max),
	mDecimalDigits(p.decimal_digits),
	mNumFrames(p.num_frames),
	mMaxHeight(p.max_height),
	mPerSec(p.show_per_sec),
	mDisplayBar(p.show_bar),
	mDisplayHistory(p.show_history),
	mOrientation(p.orientation),
	mAutoScaleMax(!p.bar_max.isProvided()),
	mAutoScaleMin(!p.bar_min.isProvided()),
	mTickValue(p.tick_spacing)
{
	// tick value will be automatically calculated later
	if (!p.tick_spacing.isProvided() && p.bar_min.isProvided() && p.bar_max.isProvided())
	{
		mTickValue = calc_tick_value(mMinBar, mMaxBar);
	}

	setStat(p.stat);
}

BOOL LLStatBar::handleHover(S32 x, S32 y, MASK mask)
{
	if (mCountFloatp)
	{
		LLToolTipMgr::instance().show(LLToolTip::Params().message(mCountFloatp->getDescription()).sticky_rect(calcScreenRect()));
	}
	else if ( mEventFloatp)
	{
		LLToolTipMgr::instance().show(LLToolTip::Params().message(mEventFloatp->getDescription()).sticky_rect(calcScreenRect()));
	}
	else if (mSampleFloatp)
	{
		LLToolTipMgr::instance().show(LLToolTip::Params().message(mSampleFloatp->getDescription()).sticky_rect(calcScreenRect()));
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

void LLStatBar::draw()
{
	F32 current = 0, 
		min     = 0, 
		max     = 0,
		mean    = 0;

    
    S32 num_samples = 0;
    
	LLLocalClipRect _(getLocalRect());
	LLTrace::PeriodicRecording& frame_recording = LLTrace::get_frame_recording();

	std::string unit_label;
	if (mCountFloatp)
	{
		LLTrace::Recording& last_frame_recording = frame_recording.getLastRecording(); 
		unit_label = mUnitLabel.empty() ? mCountFloatp->getUnitLabel() : mUnitLabel;
		if (mPerSec)
		{
			unit_label += "/s";
            num_samples = frame_recording.getSampleCount(*mCountFloatp, mNumFrames);
			current = last_frame_recording.getPerSec(*mCountFloatp);
			min     = frame_recording.getPeriodMinPerSec(*mCountFloatp, mNumFrames);
			max     = frame_recording.getPeriodMaxPerSec(*mCountFloatp, mNumFrames);
			mean    = frame_recording.getPeriodMeanPerSec(*mCountFloatp, mNumFrames);
		}
		else
		{
            num_samples = frame_recording.getSampleCount(*mCountFloatp, mNumFrames);
			current = last_frame_recording.getSum(*mCountFloatp);
			min     = frame_recording.getPeriodMin(*mCountFloatp, mNumFrames);
			max     = frame_recording.getPeriodMax(*mCountFloatp, mNumFrames);
			mean    = frame_recording.getPeriodMean(*mCountFloatp, mNumFrames);
		}
	}
	else if (mEventFloatp)
	{
		LLTrace::Recording& last_frame_recording = frame_recording.getLastRecording();
		unit_label = mUnitLabel.empty() ? mEventFloatp->getUnitLabel() : mUnitLabel;

        num_samples = frame_recording.getSampleCount(*mEventFloatp, mNumFrames);
		current = last_frame_recording.getMean(*mEventFloatp);
		min     = frame_recording.getPeriodMin(*mEventFloatp, mNumFrames);
		max     = frame_recording.getPeriodMax(*mEventFloatp, mNumFrames);
		mean    = frame_recording.getPeriodMean(*mEventFloatp, mNumFrames);
	}
	else if (mSampleFloatp)
	{
		LLTrace::Recording& last_frame_recording = frame_recording.getLastRecording();
		unit_label = mUnitLabel.empty() ? mSampleFloatp->getUnitLabel() : mUnitLabel;

        num_samples = frame_recording.getSampleCount(*mSampleFloatp, mNumFrames);
		current = last_frame_recording.getMean(*mSampleFloatp);
		min     = frame_recording.getPeriodMin(*mSampleFloatp, mNumFrames);
		max     = frame_recording.getPeriodMax(*mSampleFloatp, mNumFrames);
		mean    = frame_recording.getPeriodMean(*mSampleFloatp, mNumFrames);
	}

	S32 bar_top, bar_left, bar_right, bar_bottom;
	if (mOrientation == HORIZONTAL)
	{
		bar_top    = llmax(5, getRect().getHeight() - 15); 
		bar_left   = 0;
		bar_right  = getRect().getWidth() - 40;
		bar_bottom = llmin(bar_top - 5, 0);
	}
	else // VERTICAL
	{
		bar_top    = llmax(5, getRect().getHeight() - 15); 
		bar_left   = 0;
		bar_right  = getRect().getWidth();
		bar_bottom = llmin(bar_top - 5, 20);
	}
	const S32 tick_length = 4;
	const S32 tick_width = 1;

	if ((mAutoScaleMax && max >= mCurMaxBar)|| (mAutoScaleMin && min <= mCurMinBar))
	{
		F32 range_min = mAutoScaleMin ? llmin(mMinBar, min) : mMinBar;
		F32 range_max = mAutoScaleMax ? llmax(mMaxBar, max) : mMaxBar;
		F32 tick_value = 0.f;
		calc_auto_scale_range(range_min, range_max, tick_value);
		if (mAutoScaleMin) { mMinBar = range_min; }
		if (mAutoScaleMax) { mMaxBar = range_max; }
		if (mAutoScaleMin && mAutoScaleMax)
		{
			mTickValue = tick_value;
		}
		else
		{
			mTickValue = calc_tick_value(mMinBar, mMaxBar);
		}
	}

	mCurMaxBar = LLSmoothInterpolation::lerp(mCurMaxBar, mMaxBar, 0.05f);
	mCurMinBar = LLSmoothInterpolation::lerp(mCurMinBar, mMinBar, 0.05f);

	F32 value_scale;
	if (mCurMaxBar == mCurMinBar)
	{
		value_scale = 0.f;
	}
	else
	{
		value_scale = (mOrientation == HORIZONTAL) 
					? (bar_top - bar_bottom)/(mCurMaxBar - mCurMinBar)
					: (bar_right - bar_left)/(mCurMaxBar - mCurMinBar);
	}

	LLFontGL::getFontMonospace()->renderUTF8(mLabel, 0, 0, getRect().getHeight(), LLColor4(1.f, 1.f, 1.f, 1.f),
											 LLFontGL::LEFT, LLFontGL::TOP);
	
	S32 decimal_digits = mDecimalDigits;
	if (is_approx_equal((F32)(S32)mean, mean))
	{
		decimal_digits = 0;
	}
	std::string value_str = num_samples
                            ? llformat("%10.*f %s", decimal_digits, mean, unit_label.c_str())
                            : "n/a";

	// Draw the current value.
	if (mOrientation == HORIZONTAL)
	{
		LLFontGL::getFontMonospace()->renderUTF8(value_str, 0, bar_right, getRect().getHeight(), 
			LLColor4(1.f, 1.f, 1.f, 0.5f),
			LLFontGL::RIGHT, LLFontGL::TOP);
	}
	else
	{
		LLFontGL::getFontMonospace()->renderUTF8(value_str, 0, bar_right, getRect().getHeight(), 
			LLColor4(1.f, 1.f, 1.f, 0.5f),
			LLFontGL::RIGHT, LLFontGL::TOP);
	}

	if (mDisplayBar
        && (mCountFloatp || mEventFloatp || mSampleFloatp))
	{
		// Draw the tick marks.
		LLGLSUIDefault gls_ui;
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		S32 last_tick = 0;
		S32 last_label = 0;
		const S32 MIN_TICK_SPACING  = mOrientation == HORIZONTAL ? 20 : 30;
		const S32 MIN_LABEL_SPACING = mOrientation == HORIZONTAL ? 30 : 60;
		// start counting from actual min, not current, animating min, so that ticks don't float between numbers
		// ensure ticks always hit 0
		if (mTickValue > 0.f)
		{
			F32 start = mCurMinBar < 0.f
				? llceil(-mCurMinBar / mTickValue) * -mTickValue
				: 0.f;
			for (F32 tick_value = start; ;tick_value += mTickValue)
			{
				const S32 begin = llfloor((tick_value - mCurMinBar)*value_scale);
				const S32 end = begin + tick_width;
				if (begin - last_tick < MIN_TICK_SPACING)
				{
					continue;
				}
				last_tick = begin;

				S32 decimal_digits = mDecimalDigits;
				if (is_approx_equal((F32)(S32)tick_value, tick_value))
				{
					decimal_digits = 0;
				}
				std::string tick_string = llformat("%10.*f", decimal_digits, tick_value);

				if (mOrientation == HORIZONTAL)
				{
					if (begin - last_label > MIN_LABEL_SPACING)
					{
						gl_rect_2d(bar_left, end, bar_right - tick_length, begin, LLColor4(1.f, 1.f, 1.f, 0.25f));
						LLFontGL::getFontMonospace()->renderUTF8(tick_string, 0, bar_right, begin,
							LLColor4(1.f, 1.f, 1.f, 0.5f),
							LLFontGL::LEFT, LLFontGL::VCENTER);
						last_label = begin;
					}
					else
					{
						gl_rect_2d(bar_left, end, bar_right - tick_length/2, begin, LLColor4(1.f, 1.f, 1.f, 0.1f));
					}
				}
				else
				{
					if (begin - last_label > MIN_LABEL_SPACING)
					{
						gl_rect_2d(begin, bar_top, end, bar_bottom - tick_length, LLColor4(1.f, 1.f, 1.f, 0.25f));
						LLFontGL::getFontMonospace()->renderUTF8(tick_string, 0, begin - 1, bar_bottom - tick_length,
							LLColor4(1.f, 1.f, 1.f, 0.5f),
							LLFontGL::RIGHT, LLFontGL::TOP);
						last_label = begin;
					}
					else
					{
						gl_rect_2d(begin, bar_top, end, bar_bottom - tick_length/2, LLColor4(1.f, 1.f, 1.f, 0.1f));
					}
				}
				// always draw one tick value past end, so we can see part of the text, if possible
				if (tick_value > mCurMaxBar)
				{
					break;
				}
			}
		}

		// draw background bar.
		gl_rect_2d(bar_left, bar_top, bar_right, bar_bottom, LLColor4(0.f, 0.f, 0.f, 0.25f));

		if (frame_recording.getNumRecordedPeriods() == 0)
		{
			// No data, don't draw anything...
			return;
		}

		// draw min and max
		S32 begin = (S32) ((min - mCurMinBar) * value_scale);

		if (begin < 0)
		{
			begin = 0;
		}

		S32 end = (S32) ((max - mCurMinBar) * value_scale);
		if (mOrientation == HORIZONTAL)
		{
			gl_rect_2d(bar_left, end, bar_right, begin, LLColor4(1.f, 0.f, 0.f, 0.25f));
		}
		else // VERTICAL
		{
			gl_rect_2d(begin, bar_top, end, bar_bottom, LLColor4(1.f, 0.f, 0.f, 0.25f));
		}

        if (num_samples)
        {
            F32 span = (mOrientation == HORIZONTAL)
					? (bar_right - bar_left)
					: (bar_top - bar_bottom);

            if (mDisplayHistory && (mCountFloatp || mEventFloatp || mSampleFloatp))
            {
                const S32 num_values = frame_recording.getNumRecordedPeriods() - 1;
                F32 value = 0;
                S32 i;
                gGL.color4f( 1.f, 0.f, 0.f, 1.f );
                gGL.begin( LLRender::QUADS );
                const S32 max_frame = llmin(mNumFrames, num_values);
                U32 num_samples = 0;
                for (i = 1; i <= max_frame; i++)
                {
                    F32 offset = ((F32)i / (F32)mNumFrames) * span;
                    LLTrace::Recording& recording = frame_recording.getPrevRecording(i);

                    if (mCountFloatp)
                    {
                        value       = mPerSec 
                                        ? recording.getPerSec(*mCountFloatp) 
                                        : recording.getSum(*mCountFloatp);
                        num_samples = recording.getSampleCount(*mCountFloatp);
                    }
                    else if (mEventFloatp)
                    {
                        value       = recording.getMean(*mEventFloatp);
                        num_samples = recording.getSampleCount(*mEventFloatp);
                    }
                    else if (mSampleFloatp)
                    {
                        value       = recording.getMean(*mSampleFloatp);
                        num_samples = recording.getSampleCount(*mSampleFloatp);
                    }
                    
                    if (!num_samples) continue;

                    F32 begin = (value  - mCurMinBar) * value_scale;
                    if (mOrientation == HORIZONTAL)
                    {
                        gGL.vertex2f((F32)bar_right - offset, begin + 1);
                        gGL.vertex2f((F32)bar_right - offset, begin);
                        gGL.vertex2f((F32)bar_right - offset - 1, begin);
                        gGL.vertex2f((F32)bar_right - offset - 1, begin + 1);
                    }
                    else
                    {
                        gGL.vertex2f(begin, (F32)bar_bottom + offset + 1);
                        gGL.vertex2f(begin, (F32)bar_bottom + offset);
                        gGL.vertex2f(begin + 1, (F32)bar_bottom + offset);
                        gGL.vertex2f(begin + 1, (F32)bar_bottom + offset + 1 );
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
                    gl_rect_2d(bar_left, end, bar_right, begin, LLColor4(1.f, 0.f, 0.f, 1.f));
                }
                else
                {
                    gl_rect_2d(begin, bar_top, end, bar_bottom, LLColor4(1.f, 0.f, 0.f, 1.f));
                }
            }

            // draw mean bar
            {
                const S32 begin = (S32) ((mean - mCurMinBar) * value_scale) - 1;
                const S32 end = (S32) ((mean - mCurMinBar) * value_scale) + 1;
                if (mOrientation == HORIZONTAL)
                {
                    gl_rect_2d(bar_left - 2, begin, bar_right + 2, end, LLColor4(0.f, 1.f, 0.f, 1.f));
                }
                else
                {
                    gl_rect_2d(begin, bar_top + 2, end, bar_bottom - 2, LLColor4(0.f, 1.f, 0.f, 1.f));
                }
            }
        }
	}
	
	LLView::draw();
}

void LLStatBar::setStat(const std::string& stat_name)
{
	mCountFloatp	= LLTrace::TraceType<LLTrace::CountAccumulator>::getInstance(stat_name);
	mEventFloatp	= LLTrace::TraceType<LLTrace::EventAccumulator>::getInstance(stat_name);
	mSampleFloatp	= LLTrace::TraceType<LLTrace::SampleAccumulator>::getInstance(stat_name);
}


void LLStatBar::setRange(F32 bar_min, F32 bar_max)
{
	mMinBar		= llmin(bar_min, bar_max);
	mMaxBar		= llmax(bar_min, bar_max);
	mTickValue	= calc_tick_value(mMinBar, mMaxBar);
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

