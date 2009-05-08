/** 
 * @file llstatbar.h
 * @brief A little map of the world with network information
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLSTATBAR_H
#define LL_LLSTATBAR_H

#include "llview.h"
#include "llframetimer.h"

class LLStat;

class LLStatBar : public LLView
{
public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<std::string> label;
		Optional<std::string> unit_label;
		Optional<F32> bar_min;
		Optional<F32> bar_max;
		Optional<F32> tick_spacing;
		Optional<F32> label_spacing;
		Optional<U32> precision;
		Optional<F32> update_rate;
		Optional<bool> show_per_sec;
		Optional<bool> show_bar;
		Optional<bool> show_history;
		Optional<bool> show_mean;
		Optional<std::string> stat;
		Params()
			: label("label"),
			  unit_label("unit_label"),
			  bar_min("bar_min", 0.0f),
			  bar_max("bar_max", 50.0f),
			  tick_spacing("tick_spacing", 10.0f),
			  label_spacing("label_spacing", 10.0f),
			  precision("precision", 0),
			  update_rate("update_rate", 5.0f),
			  show_per_sec("show_per_sec", TRUE),
			  show_bar("show_bar", TRUE),
			  show_history("show_history", FALSE),
			  show_mean("show_mean", TRUE),
			  stat("stat")
		{
			follows.flags(FOLLOWS_TOP | FOLLOWS_LEFT);
		}
	};
	LLStatBar(const Params&);

	virtual void draw();
	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);

	void setStat(LLStat* stat) { mStatp = stat; }
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
	BOOL mPerSec;				// Use the per sec stats.
	BOOL mDisplayBar;			// Display the bar graph.
	BOOL mDisplayHistory;
	BOOL mDisplayMean;			// If true, display mean, if false, display current value

	LLStat* mStatp;

	LLFrameTimer mUpdateTimer;
	LLUIString mLabel;
	std::string mUnitLabel;
	F32 mValue;
};

#endif
