/** 
 * @file llfloaterforgetuser.h
 * @brief LLFloaterForgetUser class declaration.
 *
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2019, Linden Research, Inc.
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

#ifndef LL_LLFLOATERFORGETUSER_H
#define LL_LLFLOATERFORGETUSER_H

#include "llfloater.h"

class LLScrollListCtrl;

class LLFloaterForgetUser : public LLFloater
{
public:
    LLFloaterForgetUser(const LLSD &key);
    ~LLFloaterForgetUser();

    bool postBuild() override;
    void onForgetClicked();

private:
    bool onConfirmForget(const LLSD& notification, const LLSD& response);
    static bool onConfirmLogout(const LLSD& notification, const LLSD& response, const std::string &favorites_id, const std::string &grid);
    void processForgetUser();
    static void forgetUser(const std::string &userid, const std::string &fav_id, const std::string &grid, bool delete_data);
    void loadGridToList(const std::string &grid, bool show_grid_name);

    LLScrollListCtrl *mScrollList;

    bool mLoginPanelDirty;
    std::map<std::string, S32> mUserGridsCount;
};

#endif
