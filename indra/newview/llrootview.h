/** 
 * @file llrootview.h
 * @brief Mother of all Views
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

#ifndef LL_LLROOTVIEW_H
#define LL_LLROOTVIEW_H

#include "llview.h"
#include "lluictrlfactory.h"
#include "lltooltip.h"

class LLRootViewRegistry : public LLChildRegistry<LLRootViewRegistry>
{};

class LLRootView : public LLView
{
public:
	typedef LLRootViewRegistry child_registry_t;

	LLRootView(const Params& p)
	:	LLView(p)
	{}

	// added to provide possibility to handle mouse click event inside all application
	// window without creating any floater
	typedef boost::signals2::signal<void(S32 x, S32 y, MASK mask)>
			mouse_signal_t;

	private:
		mouse_signal_t mMouseDownSignal;

	public:
	/*virtual*/
	BOOL handleMouseDown(S32 x, S32 y, MASK mask)
	{
		mMouseDownSignal(x, y, mask);
		return LLView::handleMouseDown(x, y, mask);
	}

	boost::signals2::connection addMouseDownCallback(
			const mouse_signal_t::slot_type& cb)
	{
		return mMouseDownSignal.connect(cb);
	}
};
#endif //LL_LLROOTVIEW_H
