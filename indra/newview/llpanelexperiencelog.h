/** 
 * @file llpanelexperiencelog.h
 * @brief llpanelexperiencelog and related class definitions
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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


#ifndef LL_LLPANELEXPERIENCELOG_H
#define LL_LLPANELEXPERIENCELOG_H

#include "llpanel.h"
class LLScrollListCtrl;

class LLPanelExperienceLog
    : public LLPanel 
{
public:

    LLPanelExperienceLog();

    static LLPanelExperienceLog* create();

    /*virtual*/ BOOL postBuild(void);

    void refresh();
protected:
    void logSizeChanged();
    void notifyChanged();
    void onNext();
    void onNotify();
    void onPrev();
    void onProfileExperience();
    void onReportExperience();
    void onSelectionChanged();

    LLSD getSelectedEvent();
private:
    LLScrollListCtrl* mEventList;
    U32 mPageSize;
    U32 mCurrentPage;
    boost::signals2::scoped_connection mNewEvent;
};

#endif // LL_LLPANELEXPERIENCELOG_H
