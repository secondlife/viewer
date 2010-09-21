/** 
 * @file lllistview.h
 * @brief UI widget containing a scrollable, possibly hierarchical list of 
 * folders (LLListViewFolder) and items (LLListViewItem).
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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
#ifndef LLLISTVIEW_H
#define LLLISTVIEW_H

#include "llui.h"		// for LLUIColor, *TODO: use more specific header
#include "lluictrl.h"

class LLTextBox;

class LLListView
:	public LLUICtrl
{
public:
	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<LLUIColor>	bg_color,
							fg_selected_color,
							bg_selected_color;
		Params();
	};
	LLListView(const Params& p);
	virtual ~LLListView();

	// placeholder for setting a property
	void setString(const std::string& s);

private:
	// TODO: scroll container?
	LLTextBox*	mLabel;		// just for testing
	LLUIColor	mBgColor;
	LLUIColor	mFgSelectedColor;
	LLUIColor	mBgSelectedColor;
};

#endif // LLLISTVIEW_H
