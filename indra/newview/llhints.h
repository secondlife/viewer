/**
 * @file llhints.h
 * @brief Hint popups for displaying context sensitive help in a UI overlay
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLHINTS_H
#define LL_LLHINTS_H

#include "llpanel.h"
#include "llnotifications.h"


class LLHints :  public LLInitClass<LLHints>
{
public:
	static void show(LLNotificationPtr hint);
	static void hide(LLNotificationPtr hint);
	static void registerHintTarget(const std::string& name, LLHandle<LLView> target);
	static LLHandle<LLView> getHintTarget(const std::string& name);
	static void initClass();
private:
	static LLRegistry<std::string, LLHandle<LLView> > sTargetRegistry;
	typedef std::map<LLNotificationPtr, class LLHintPopup*> hint_map_t;
	static hint_map_t sHints;
	static void showHints(const LLSD& show);
};


#endif
