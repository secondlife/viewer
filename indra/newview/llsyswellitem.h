/** 
 * @file llsyswellitem.h
 * @brief                                    // TODO
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLSYSWELLITEM_H
#define LL_LLSYSWELLITEM_H

#include "llpanel.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "lliconctrl.h"

#include <string>

class LLSysWellItem : public LLPanel
{
public:
    struct Params : public LLInitParam::Block<Params, LLPanel::Params>
    {
        LLUUID          notification_id;
        std::string     title;
        Params()        {};
    };


    LLSysWellItem(const Params& p);
    virtual ~LLSysWellItem();

    // title
    void setTitle( std::string title );

    // get item's ID
    LLUUID getID() { return mID; }

    // handlers
    virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
    virtual void onMouseEnter(S32 x, S32 y, MASK mask);
    virtual void onMouseLeave(S32 x, S32 y, MASK mask);

    //callbacks
    typedef boost::function<void (LLSysWellItem* item)> syswell_item_callback_t;
    typedef boost::signals2::signal<void (LLSysWellItem* item)> syswell_item_signal_t;
    syswell_item_signal_t mOnItemClose; 
    syswell_item_signal_t mOnItemClick; 
    boost::signals2::connection setOnItemCloseCallback(syswell_item_callback_t cb) { return mOnItemClose.connect(cb); }
    boost::signals2::connection setOnItemClickCallback(syswell_item_callback_t cb) { return mOnItemClick.connect(cb); }

private:

    void onClickCloseBtn();

    LLTextBox*  mTitle;
    LLButton*   mCloseBtn;
    LLUUID      mID;
};

#endif // LL_LLSYSWELLITEM_H


