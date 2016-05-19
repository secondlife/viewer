/**
 * @file   llviewercontrollistener.h
 * @author Brad Kittenbrink
 * @date   2009-07-09
 * @brief  Event API for subset of LLViewerControl methods
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLVIEWERCONTROLLISTENER_H
#define LL_LLVIEWERCONTROLLISTENER_H

#include "lleventapi.h"

class LLControlGroup;
class LLSD;

class  LLViewerControlListener : public LLEventAPI
{
public:
	LLViewerControlListener();

private:
	static void set(LLSD const & event_data);
	static void toggle(LLSD const & event_data);
	static void get(LLSD const & event_data);
	static void groups(LLSD const & event_data);
	static void vars(LLSD const & event_data);
};

#endif // LL_LLVIEWERCONTROLLISTENER_H
