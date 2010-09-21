/** 
 * @file llrootview.h
 * @brief Mother of all Views
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
