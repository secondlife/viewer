/** 
 * @file llstatview.h
 * @brief Container for all statistics info.
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

#ifndef LL_LLSTATVIEW_H
#define LL_LLSTATVIEW_H

#include "llstatbar.h"
#include "llcontainerview.h"
#include <vector>

class LLStatBar;

// widget registrars
struct StatViewRegistry : public LLChildRegistry<StatViewRegistry>
{};

class LLStatView : public LLContainerView
{
public:
	struct Params : public LLInitParam::Block<Params, LLContainerView::Params>
	{
		Optional<std::string> setting;
		Params() 
		:	setting("setting")
		{
			changeDefault(follows.flags, FOLLOWS_TOP | FOLLOWS_LEFT);
		}
	};

	// my valid children are stored in this registry
	typedef StatViewRegistry child_registry_t;

	~LLStatView();

protected:
	LLStatView(const Params&);
	friend class LLUICtrlFactory;

protected:
	std::string mSetting;

};
#endif // LL_STATVIEW_
