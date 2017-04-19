/**
 * @file llfloateravatarrendersettings.h
 * @brief Shows the list of avatars with non-default rendering settings
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2017, Linden Research, Inc.
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

#ifndef LL_LLFLOATERAVATARRENDERSETTINGS_H
#define LL_LLFLOATERAVATARRENDERSETTINGS_H

#include "llfloater.h"
#include "lllistcontextmenu.h"
#include "llmutelist.h"

class LLNameListCtrl;

class LLFloaterAvatarRenderSettings : public LLFloater
{
public:

    LLFloaterAvatarRenderSettings(const LLSD& key);
    virtual ~LLFloaterAvatarRenderSettings();

    /*virtual*/ BOOL postBuild();
    /*virtual*/ void onOpen(const LLSD& key);
    /*virtual*/ void draw();

    void onAvatarListRightClick(LLUICtrl* ctrl, S32 x, S32 y);

    void updateList();
    void onFilterEdit(const std::string& search_string);
    void onCustomAction (const LLSD& userdata, const LLUUID& av_id);
    bool isActionChecked(const LLSD& userdata, const LLUUID& av_id);
    void onClickAdd(const LLSD& userdata);

    static void setNeedsUpdate();

private:
    bool isHiddenRow(const std::string& av_name);
    void callbackAvatarPicked(const uuid_vec_t& ids, S32 visual_setting);
    void removePicker();

    bool mNeedsUpdate;
    LLListContextMenu* mContextMenu;
    LLNameListCtrl* mAvatarSettingsList;
    LLHandle<LLFloater> mPicker;

    std::string mNameFilter;
};


#endif //LL_LLFLOATERAVATARRENDERSETTINGS_H
