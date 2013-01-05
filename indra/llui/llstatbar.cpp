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

///////////////////////////////////////////////////////////////////////////////////

LLStatBar::LLStatBar(const Params& p)
	: LLView(p),
	  mLabel(p.label),
	  mUnitLabel(p.unit_label),
	  mMinBar(p.bar_min),
	  mMaxBar(p.bar_max),
	  mCountFloatp(LLTrace::Count<>::getInstance(p.stat)),
	  mCountIntp(LLTrace::Count<S64>::getInstance(p.stat)),
	  mMeasurementFloatp(LLTrace::Measurement<>::getInstance(p.stat)),
	  mMeasurementIntp(LLTrace::Measurement<S64>::getInstance(p.stat)),
	  mTickSpacing(p.tick_spacing),
	  mLabelSpacing(p.label_spacing),
	  mPrecision(p.precision),
	  mUpdatesPerSec(p.update_rate),
	  mUnitScale(p.unit_scale),
	  mPerSec(p.show_per_sec),
	  mDisplayBar(p.show_bar),
	  mDisplayHistory(p.show_history),
	  mDisplayMean(p.show_mean)
{}

BOOL LLStatBar::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (mDisplayBar)
	{
		if (mDisplayHistory)
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
	}

	LLView* parent = getParent();
	parent->reshape(parent->getRect().getWidth(), parent->getRect().getHeight(), FALSE);

	return FALSE;
}

void LLStatBar::draw()
{
	F32 current = 0.f, 
		min = 0.f, 
		max = 0.f,
		mean = 0.f;

	LLTrace::PeriodicRecording& frame_recording = LLTrace::get_frame_recording();

	if (mCountFloatp)
	{
		LLTrace::Recording& last_frame_recording = frame_recording.getLastRecordingPeriod(); 

		if (mPerSec)
		{
			current = last_frame_recording.getPerSec(*mCountFloatp);
			min = frame_recording.getPeriodMinPerSec(*mCountFloatp);
			max = frame_recording.getPeriodMaxPerSec(*mCountFloatp);
			mean = frame_recording.getPeriodMeanPerSec(*mCountFloatp);
		}
		else
		{
			current = last_frame_recording.getSum(*mCountFloatp);
			min = frame_recording.getPeriodMin(*mCountFloatp);
			max = frame_recording.getPeriodMax(*mCountFloatp);
			mean = frame_recording.getPeriodMean(*mCountFloatp);
		}
	}
	else if (mCountIntp)
	{
		LLTrace::Recording& last_frame_recording = frame_recording.getLastRecordingPeriod(); 

		if (mPerSec)
		{
			current = last_frame_recording.getPerSec(*mCountIntp);
			min = frame_recording.getPeriodMinPerSec(*mCountIntp);
			max = frame_recording.getPeriodMaxPerSec(*mCountIntp);
			mean = frame_recording.getPeriodMeanPerSec(*mCountIntp);
		}
		else
		{
			current = last_frame_recording.getSum(*mCountIntp);
			min = frame_recording.getPeriodMin(*mCountIntp);
			max = frame_recording.getPeriodMax(*mCountIntp);
			mean = frame_recording.getPeriodMean(*mCountIntp);
		}
	}
	else if (mMeasurementFloatp)
	{
		LLTrace::Recording& recording = frame_recording.getTotalRecording();
		current = recording.getLastValue(*mMeasurementFloatp);
		min = recording.getMin(*mMeasurementFloatp);
		max = recording.getMax(*mMeasurementFloatp);
		mean = recording.getMean(*mMeasurementFloatp);
	}
	else if (mMeasurementIntp)
	{
		LLTrace::Recording& recording = frame_recording.getTotalRecording();
		current = recording.getLastValue(*mMeasurementIntp);
		min = recording.getMin(*mMeasurementIntp);
		max = recording.getMax(*mMeasurementIntp);
		mean = recording.getMean(*mMeasurementIntp);
	}

	current *= mUnitScale;
	min *= mUnitScale;
	max *= mUnitScale;
	mean *= mUnitScale;

	if ((mUpdatesPerSec == 0.f) || (mUpdateTimer.getElapsedTimeF32() > 1.f/mUpdatesPerSec) || (mValue == 0.f))
	{
		if (mDisplayMean)
		{
			mValue = mean;
		}
		else
		{
			mValue = current;
		}
		mUpdateTimer.reset();
	}

	S32 width = getRect().getWidth() - 40;
	S32 max_width = width;
	S32 bar_top = getRect().getHeight() - 15; // 16 pixels from top.
	S32 bar_height = bar_top - 20;
	S32 tick_height = 4;
	S32 tick_width = 1;
	S32 left, top, right, bottom;

	F32 value_scale = max_width/(mMaxBar - mMinBar);

	LLFontGL::getFontMonospace()->renderUTF8(mLabel, 0, 0, getRect().getHeight(), LLColor4(1.f, 1.f, 1.f, 1.f),
											 LLFontGL::LEFT, LLFontGL::TOP);

	std::string value_format;
	std::string value_str;
	if (!mUnitLabel.empty())
	{
		value_format = llformat( "%%.%df%%s", mPrecision);
		value_str = llformat( value_format.c_str(), mValue, mUnitLabel.c_str());
	}
	else
	{
		value_format = llformat( "%%.%df", mPrecision);
		value_str = llformat( value_format.c_str(), mValue);
	}

	// Draw the value.
	LLFontGL::getFontMonospace()->renderUTF8(value_str, 0, width, getRect().getHeight(), 
											 LLColor4(1.f, 1.f, 1.f, 0.5f),
											 LLFontGL::RIGHT, LLFontGL::TOP);

	value_format = llformat( "%%.%df", mPrecision);
	if (mDisplayBar && (mCountFloatp || mCountIntp || mMeasurementFloatp || mMeasurementIntp))
	{
		std::string tick_label;

		// Draw the tick marks.
		F32 tick_value;
		top = bar_top;
		bottom = bar_top - bar_height - tick_height/2;

		LLGLSUIDefault gls_ui;
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		for (tick_value = mMinBar; tick_value <= mMaxBar; tick_value += mTickSpacing)
		{
			left = llfloor((tick_value - mMinBar)*value_scale);
			right = left + tick_width;
			gl_rect_2d(left, top, right, bottom, LLColor4(1.f, 1.f, 1.f, 0.1f));
		}

		// Draw the tick labels (and big ticks).
		bottom = bar_top - bar_height - tick_height;
		for (tick_value = mMinBar; tick_value <= mMaxBar; tick_value += mLabelSpacing)
		{
			left = llfloor((tick_value - mMinBar)*value_scale);
			right = left + tick_width;
			gl_rect_2d(left, top, right, bottom, LLColor4(1.f, 1.f, 1.f, 0.25f));

			tick_label = llformat( value_format.c_str(), tick_value);
			// draw labels for the tick marks
			LLFontGL::getFontMonospace()->renderUTF8(tick_label, 0, left - 1, bar_top - bar_height - tick_height,
													 LLColor4(1.f, 1.f, 1.f, 0.5f),
													 LLFontGL::LEFT, LLFontGL::TOP);
		}

		// Now, draw the bars
		top = bar_top;
		bottom = bar_top - bar_height;

		// draw background bar.
		left = 0;
		right = width;
		gl_rect_2d(left, top, right, bottom, LLColor4(0.f, 0.f, 0.f, 0.25f));

		if (frame_recording.getNumPeriods() == 0)
		{
			// No data, don't draw anything...
			return;
		}
		// draw min and max
		left = (S32) ((min - mMinBar) * value_scale);

		if (left < 0)
		{
			left = 0;
			llwarns << "Min:" << min << llendl;
		}

		right = (S32) ((max - mMinBar) * value_scale);
		gl_rect_2d(left, top, right, bottom, LLColor4(1.f, 0.f, 0.f, 0.25f));

		if (mDisplayHistory && (mCountFloatp || mCountIntp || mMeasurementFloatp || mMeasurementIntp))
		{
			S32 num_values = frame_recording.getNumPeriods() - 1;
			S32 i;
			for (i = 1; i <= num_values; i++)
			{
				if (mPerSec)
				{
					if (mCountFloatp)
					{
						left = (S32)((frame_recording.getPrevRecordingPeriod(i).getPerSec(*mCountFloatp)  - mMinBar) * value_scale);
						right = (S32)((frame_recording.getPrevRecordingPeriod(i).getPerSec(*mCountFloatp)  - mMinBar) * value_scale) + 1;
					}
					else if (mCountIntp)
					{
						left = (S32)((frame_recording.getPrevRecordingPeriod(i).getPerSec(*mCountIntp)  - mMinBar) * value_scale);
						right = (S32)((frame_recording.getPrevRecordingPeriod(i).getPerSec(*mCountIntp)  - mMinBar) * value_scale) + 1;
					}
					else if (mMeasurementFloatp)
					{
						left = (S32)((frame_recording.getPrevRecordingPeriod(i).getPerSec(*mMeasurementFloatp)  - mMinBar) * value_scale);
						right = (S32)((frame_recording.getPrevRecordingPeriod(i).getPerSec(*mMeasurementFloatp)  - mMinBar) * value_scale) + 1;
					}
					else if (mMeasurementIntp)
					{
						left = (S32)((frame_recording.getPrevRecordingPeriod(i).getPerSec(*mMeasurementIntp)  - mMinBar) * value_scale);
						right = (S32)((frame_recording.getPrevRecordingPeriod(i).getPerSec(*mMeasurementIntp)  - mMinBar) * value_scale) + 1;
					}
					gl_rect_2d(left, bottom+i+1, right, bottom+i, LLColor4(1.f, 0.f, 0.f, 1.f));
				}
				else
				{
					if (mCountFloatp)
					{
						left = (S32)((frame_recording.getPrevRecordingPeriod(i).getSum(*mCountFloatp)  - mMinBar) * value_scale);
						right = (S32)((frame_recording.getPrevRecordingPeriod(i).getSum(*mCountFloatp)  - mMinBar) * value_scale) + 1;
					}
					else if (mCountIntp)
					{
						left = (S32)((frame_recording.getPrevRecordingPeriod(i).getSum(*mCountIntp)  - mMinBar) * value_scale);
						right = (S32)((frame_recording.getPrevRecordingPeriod(i).getSum(*mCountIntp)  - mMinBar) * value_scale) + 1;
					}
					else if (mMeasurementFloatp)
					{
						left = (S32)((frame_recording.getPrevRecordingPeriod(i).getSum(*mMeasurementFloatp)  - mMinBar) * value_scale);
						right = (S32)((frame_recording.getPrevRecordingPeriod(i).getSum(*mMeasurementFloatp)  - mMinBar) * value_scale) + 1;
					}
					else if (mMeasurementIntp)
					{
						left = (S32)((frame_recording.getPrevRecordingPeriod(i).getSum(*mMeasurementIntp)  - mMinBar) * value_scale);
						right = (S32)((frame_recording.getPrevRecordingPeriod(i).getSum(*mMeasurementIntp)  - mMinBar) * value_scale) + 1;
					}
					gl_rect_2d(left, bottom+i+1, right, bottom+i, LLColor4(1.f, 0.f, 0.f, 1.f));
				}
			}
		}
		else
		{
			// draw current
			left = (S32) ((current - mMinBar) * value_scale) - 1;
			right = (S32) ((current - mMinBar) * value_scale) + 1;
			gl_rect_2d(left, top, right, bottom, LLColor4(1.f, 0.f, 0.f, 1.f));
		}

		// draw mean bar
		top = bar_top + 2;
		bottom = bar_top - bar_height - 2;
		left = (S32) ((mean - mMinBar) * value_scale) - 1;
		right = (S32) ((mean - mMinBar) * value_scale) + 1;
		gl_rect_2d(left, top, right, bottom, LLColor4(0.f, 1.f, 0.f, 1.f));
	}
	
	LLView::draw();
}

void LLStatBar::setStat(const std::string& stat_name)
{
	mCountFloatp = LLTrace::Count<>::getInstance(stat_name);
	mCountIntp = LLTrace::Count<S64>::getInstance(stat_name);
	mMeasurementFloatp = LLTrace::Measurement<>::getInstance(stat_name);
	mMeasurementIntp = LLTrace::Measurement<S64>::getInstance(stat_name);
}


void LLStatBar::setRange(F32 bar_min, F32 bar_max, F32 tick_spacing, F32 label_spacing)
{
	mMinBar = bar_min;
	mMaxBar = bar_max;
	mTickSpacing = tick_spacing;
	mLabelSpacing = label_spacing;
}

LLRect LLStatBar::getRequiredRect()
{
	LLRect rect;

	if (mDisplayBar)
	{
		if (mDisplayHistory)
		{
			rect.mTop = 35 + LLTrace::get_frame_recording().getNumPeriods();
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
