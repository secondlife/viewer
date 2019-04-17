/**
* @file llfloaterbanduration.cpp
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

#include "llviewerprecompiledheaders.h"
#include "llfloaterbanduration.h"

#include "llfloaterreg.h"
#include "llspinctrl.h"
#include "llradiogroup.h"

LLFloaterBanDuration::LLFloaterBanDuration(const LLSD& target)
    : LLFloater(target)
{
}

BOOL LLFloaterBanDuration::postBuild()
{
    childSetAction("ok_btn", boost::bind(&LLFloaterBanDuration::onClickBan, this));
    childSetAction("cancel_btn", boost::bind(&LLFloaterBanDuration::onClickCancel, this));

    getChild<LLUICtrl>("ban_duration_radio")->setCommitCallback(boost::bind(&LLFloaterBanDuration::onClickRadio, this));
    getChild<LLRadioGroup>("ban_duration_radio")->setSelectedIndex(0);
    getChild<LLUICtrl>("ban_hours")->setEnabled(FALSE);

    return TRUE;
}

LLFloaterBanDuration* LLFloaterBanDuration::show(select_callback_t callback, uuid_vec_t ids)
{
    LLFloaterBanDuration* floater = LLFloaterReg::showTypedInstance<LLFloaterBanDuration>("ban_duration");
    if (!floater)
    {
        LL_WARNS() << "Cannot instantiate ban duration floater" << LL_ENDL;
        return NULL;
    }

    floater->mSelectionCallback = callback;
    floater->mAvatar_ids = ids;

    return floater;
}

void LLFloaterBanDuration::onClickRadio()
{
    getChild<LLUICtrl>("ban_hours")->setEnabled(getChild<LLRadioGroup>("ban_duration_radio")->getSelectedIndex() != 0);
}

void LLFloaterBanDuration::onClickCancel()
{
    closeFloater();
}

void LLFloaterBanDuration::onClickBan()
{
    if (mSelectionCallback)
    {
        S32 time = 0;
        if (getChild<LLRadioGroup>("ban_duration_radio")->getSelectedIndex() != 0)
        {
            LLSpinCtrl* hours_spin = getChild<LLSpinCtrl>("ban_hours");
            if (hours_spin)
            {
                time = LLDate::now().secondsSinceEpoch() + (hours_spin->getValue().asInteger() * 3600);
            }
        }
        mSelectionCallback(mAvatar_ids, time);
    }
    closeFloater();
}

