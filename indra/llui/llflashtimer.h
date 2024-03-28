/**
 * @file llflashtimer.h
 * @brief LLFlashTimer class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LL_FLASHTIMER_H
#define LL_FLASHTIMER_H

#include "lleventtimer.h"
#include "boost/function.hpp"

class LLFlashTimer : public LLEventTimer
{
public:

	typedef boost::function<void (bool)> callback_t;

	/**
	 * Constructor.
	 *
	 * @param count - how many times callback should be called (twice to not change original state)
	 * @param period - how frequently callback should be called
	 * @param cb - callback to be called each tick
	 */
	LLFlashTimer(callback_t cb = NULL, S32 count = 0, F32 period = 0.0);
	~LLFlashTimer() {};

	/*virtual*/ bool tick();

	void startFlashing();
	void stopFlashing();

	bool isFlashingInProgress();
	bool isCurrentlyHighlighted();
	/*
	 * Use this instead of deleting this object.
	 * The next call to tick() will return true and that will destroy this object.
	 */
	void unset();

private:
	callback_t		mCallback;
	/**
	 * How many times parent will blink.
	 */
	S32 mFlashCount;
	S32 mCurrentTickCount;
	bool mIsCurrentlyHighlighted;
	bool mIsFlashingInProgress;
	bool mUnset;
};

#endif /* LL_FLASHTIMER_H */
