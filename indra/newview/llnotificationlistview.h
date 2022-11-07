/** 
 * @file llnotificationlistview.h
 * @brief LLNotificationListView class to support notifications list contained in enclosing floater.
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2015, Linden Research, Inc.
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

#ifndef LL_LLNOTIFICATIONLISTVIEW_H
#define LL_LLNOTIFICATIONLISTVIEW_H

#include "llflatlistview.h"
#include "llnotificationlistitem.h"

/**
 * Notification list
 */
class LLNotificationListView : public LLFlatListView
{
    LOG_CLASS(LLNotificationListView);
public:
    struct Params : public LLInitParam::Block<Params, LLFlatListView::Params> {};

    LLNotificationListView(const Params& p);
    ~LLNotificationListView();
    friend class LLUICtrlFactory;

    virtual bool addNotification(LLNotificationListItem * item);
};

#endif
