/**
 * @file llpaneleventinfo.h
 * @brief Info panel for events in the legacy Search
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

#ifndef LL_LLPANELEVENTINFO_H
#define LL_LLPANELEVENTINFO_H

#include "lleventnotifier.h"

class LLTextBox;
class LLTextEditor;
class LLButton;

class LLPanelEventInfo : public LLPanel
{
public:
    LLPanelEventInfo();
    /*virtual*/ ~LLPanelEventInfo();

    /*virtual*/ bool postBuild() override;

    void setEventID(const U32 event_id);
    void sendEventInfoRequest();

    bool processEventInfoReply(LLEventInfo event);

    U32 getEventID() { return mEventID; }

protected:
    void onClickTeleport();
    void onClickMap();
    void onClickNotify();

protected:
    LLTextBox* mTBName;
    LLTextBox* mTBCategory;
    LLTextBox* mTBDate;
    LLTextBox* mTBDuration;
    LLTextEditor* mTBDesc;

    LLTextBox* mTBRunBy;
    LLTextBox* mTBLocation;
    LLTextBox* mTBCover;

    LLButton* mTeleportBtn;
    LLButton* mMapBtn;
    LLButton* mNotifyBtn;

    U32 mEventID;
    LLEventInfo mEventInfo;
    boost::signals2::connection mEventInfoConnection;
};

#endif // LL_LLPANELEVENTINFO_H
