/** 
 * @file lllistview.cpp
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
#include "llviewerprecompiledheaders.h"

#include "lllistview.h"

#include "lltextbox.h"
#include "lluictrlfactory.h"	// LLDefaultWidgetRegistry

// linker optimizes this out on Windows until there is a real reference
// to this file
static LLDefaultChildRegistry::Register<LLListView> r("list_view");

LLListView::Params::Params()
:	bg_color("bg_color"),
	fg_selected_color("fg_selected_color"),
	bg_selected_color("bg_selected_color")
{}

LLListView::LLListView(const Params& p)
:	LLUICtrl(p),
	mLabel(NULL),
	mBgColor(p.bg_color()),
	mFgSelectedColor(p.fg_selected_color()),
	mBgSelectedColor(p.bg_selected_color())
{
	LLRect label_rect(0, 20, 300, 0);
	LLTextBox::Params text_box_params;
	text_box_params.rect(label_rect);
	text_box_params.text("This is a list-view");
	mLabel = LLUICtrlFactory::create<LLTextBox>(text_box_params);
	addChild(mLabel);
}

LLListView::~LLListView()
{}


// placeholder for setting a property
void LLListView::setString(const std::string& s)
{
	mLabel->setValue( LLSD(s) );
}
