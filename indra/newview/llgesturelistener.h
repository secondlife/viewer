/**
 * @file   llgesturelistener.h
 * @author Dave Simmons
 * @date   2011-03-15
 * @brief  Class definition for LLGestureListener.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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


#ifndef LL_LLGESTURELISTENER_H
#define LL_LLGESTURELISTENER_H

#include "lleventapi.h"

class LLSD;

class LLGestureListener : public LLEventAPI
{
public:
	LLGestureListener();

private:
    void getActiveGestures(LLSD const & gesture_data) const;
	void isGesturePlaying(LLSD const & gesture_data) const;
    void startGesture(LLSD const & gesture_data) const;
    void stopGesture(LLSD const & gesture_data) const;

	void startOrStopGesture(LLSD const & event_data, bool start) const;
};

#endif // LL_LLGESTURELISTENER_H

