/** 
 * @file lllistview.h
 * @brief UI widget containing a scrollable, possibly hierarchical list of 
 * folders (LLListViewFolder) and items (LLListViewItem).
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
#ifndef LLLISTVIEW_H
#define LLLISTVIEW_H

#include "llui.h"       // for LLUIColor, *TODO: use more specific header
#include "lluictrl.h"

class LLTextBox;

class LLListView
:   public LLUICtrl
{
public:
    struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
    {
        Optional<LLUIColor> bg_color,
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
    LLTextBox*  mLabel;     // just for testing
    LLUIColor   mBgColor;
    LLUIColor   mFgSelectedColor;
    LLUIColor   mBgSelectedColor;
};

#endif // LLLISTVIEW_H
