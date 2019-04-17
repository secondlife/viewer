/**
* @file llfloaterbanduration.h
*
* $LicenseInfo:firstyear=2004&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2018, Linden Research, Inc.
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


#ifndef LL_FLOATERBANDURATION_H
#define LL_FLOATERBANDURATION_H

#include "llfloater.h"

class LLFloaterBanDuration : public LLFloater
{
    typedef boost::function<void(const uuid_vec_t&, const S32 duration)> select_callback_t;

public:
    LLFloaterBanDuration(const LLSD& target);
    BOOL postBuild();
    static LLFloaterBanDuration* show(select_callback_t callback, uuid_vec_t id);

private:
    ~LLFloaterBanDuration() {};
    void onClickBan();
    void onClickCancel();
    void onClickRadio();

    uuid_vec_t mAvatar_ids;
    select_callback_t mSelectionCallback;
};

#endif // LL_FLOATERBANDURATION_H

