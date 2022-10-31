/**
 * @file llfloaterurlentry.h
 * @brief LLFloaterURLEntry class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLFLOATERURLENTRY_H
#define LL_LLFLOATERURLENTRY_H

#include "llfloater.h"
#include "llpanellandmedia.h"
#include "lleventcoro.h"
#include "llcoros.h"

class LLLineEditor;
class LLComboBox;

class LLFloaterURLEntry : public LLFloater
{
public:
    // Can only be shown by LLPanelLandMedia, and pushes data back into
    // that panel via the handle.
    static LLHandle<LLFloater> show(LLHandle<LLPanel> panel_land_media_handle, const std::string media_url);
    /*virtual*/ BOOL    postBuild();
    /*virtual*/ void onClose( bool app_quitting );
    void headerFetchComplete(S32 status, const std::string& mime_type);

    bool addURLToCombobox(const std::string& media_url);

private:
    LLFloaterURLEntry(LLHandle<LLPanel> parent);
    /*virtual*/ ~LLFloaterURLEntry();
    void buildURLHistory();

private:
    LLComboBox*     mMediaURLEdit;
    LLHandle<LLPanel> mPanelLandMediaHandle;

    static void     onBtnOK(void*);
    static void     onBtnCancel(void*);
    static void     onBtnClear(void*);
    bool            callback_clear_url_list(const LLSD& notification, const LLSD& response);

    static void     getMediaTypeCoro(std::string url, LLHandle<LLFloater> parentHandle);

};

#endif  // LL_LLFLOATERURLENTRY_H
