/**
 * @file llfloaterregionrestartschedule.cpp
 * @author Andrii Kleshchev
 * @brief LLFloaterRegionRestartSchedule class
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

#include "llfloaterregionrestartschedule.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "lllineeditor.h"
#include "llviewercontrol.h"


// match with values used by capability
constexpr char CHECKBOX_PREFIXES[] =
{
    's',
    'm',
    't',
    'w',
    'r',
    'f',
    'a',
};

const std::string CHECKBOX_NAME = "_chk";

LLFloaterRegionRestartSchedule::LLFloaterRegionRestartSchedule(
    LLView* owner)
    : LLFloater(LLSD())
    , mOwnerHandle(owner->getHandle())
{
    buildFromFile("floater_region_restart_schedule.xml");
}

LLFloaterRegionRestartSchedule::~LLFloaterRegionRestartSchedule()
{

}

bool LLFloaterRegionRestartSchedule::postBuild()
{
    mPMAMButton = getChild<LLButton>("am_pm_btn");
    mPMAMButton->setClickedCallback([this](LLUICtrl*, const LLSD&) { onPMAMButtonClicked(); });

    // By default mPMAMButton is supposed to be visible.
    // If localized xml set mPMAMButton to be invisible, assume
    // 24h format and prealligned "UTC" label
    if (mPMAMButton->getVisible())
    {
        bool use_24h_format = gSavedSettings.getBOOL("Use24HourClock");
        if (use_24h_format)
        {
            mPMAMButton->setVisible(false);
            LLUICtrl* lbl = getChild<LLUICtrl>("utc_label");
            lbl->translate(-mPMAMButton->getRect().getWidth(), 0);
        }
    }

    mSaveButton = getChild<LLButton>("save_btn");
    mSaveButton->setClickedCallback([this](LLUICtrl*, const LLSD&) { onSaveButtonClicked(); });

    mCancelButton = getChild<LLButton>("cancel_btn");
    mCancelButton->setClickedCallback([this](LLUICtrl*, const LLSD&) { closeFloater(false); });


    mHoursLineEditor = getChild<LLLineEditor>("hours_edt");
    mHoursLineEditor->setPrevalidate(LLTextValidate::validateNonNegativeS32);
    mHoursLineEditor->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& value) { onCommitHours(value); });

    mMinutesLineEditor = getChild<LLLineEditor>("minutes_edt");
    mMinutesLineEditor->setPrevalidate(LLTextValidate::validateNonNegativeS32);
    mMinutesLineEditor->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& value) { onCommitMinutes(value); });

    for (char c : CHECKBOX_PREFIXES)
    {
        std::string name = c + CHECKBOX_NAME;
        LLCheckBoxCtrl* chk = getChild<LLCheckBoxCtrl>(name);
        chk->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& value) { mSaveButton->setEnabled(true); });
    }

    resetUI(false);

    return true;
}

void LLFloaterRegionRestartSchedule::onOpen(const LLSD& key)
{
    std::string url = gAgent.getRegionCapability("RegionSchedule");
    if (!url.empty())
    {
        LLCoros::instance().launch("LLFloaterRegionRestartSchedule::requestRegionShcheduleCoro",
            boost::bind(&LLFloaterRegionRestartSchedule::requestRegionShcheduleCoro, url, getHandle()));

        mSaveButton->setEnabled(false);
    }
    else
    {
        LL_WARNS("Region") << "Started region schedule floater, but RegionSchedule capability is not available" << LL_ENDL;
    }
}

void LLFloaterRegionRestartSchedule::draw()
{
    LLView* owner = mOwnerHandle.get();
    if (owner)
    {
        static LLCachedControl<F32> max_opacity(gSavedSettings, "PickerContextOpacity", 0.4f);
        drawConeToOwner(mContextConeOpacity, max_opacity, owner);
    }

    LLFloater::draw();
}

void LLFloaterRegionRestartSchedule::onPMAMButtonClicked()
{
    mSaveButton->setEnabled(true);
    mTimeAM = !mTimeAM;
    updateAMPM();
}

void LLFloaterRegionRestartSchedule::onSaveButtonClicked()
{
    std::string url = gAgent.getRegionCapability("RegionSchedule");
    if (!url.empty())
    {
        std::string days;
        for (char c : CHECKBOX_PREFIXES)
        {
            std::string name = c + CHECKBOX_NAME;
            LLCheckBoxCtrl* chk = getChild<LLCheckBoxCtrl>(name);
            if (chk->getValue())
            {
                days += c;
            }
        }
        LLSD restart;
        if (days.size() < 7)
        {
            LLStringUtil::toUpper(days);
            restart["type"] = "W";
            // if days are empty, will reset schedule
            restart["days"] = days;
        }
        else
        {
            restart["type"] = "D";
        }
        S32 hours = mHoursLineEditor->getValue().asInteger();
        if (mPMAMButton->getVisible())
        {
            if (hours == 12)
            {
                hours = 0; // 12:00 AM equals 0:00, while 12:00 PM equals 12:00
            }
            if (!mTimeAM)
            {
                hours += 12;
            }
        }
        restart["time"] = hours * 3600 + mMinutesLineEditor->getValue().asInteger() * 60;

        LLSD body;
        body["restart"] = restart; // event name, at the moment only "restart" is supported
        LLCoros::instance().launch("LLFloaterRegionRestartSchedule::setRegionShcheduleCoro",
            boost::bind(&LLFloaterRegionRestartSchedule::setRegionShcheduleCoro, url, body, getHandle()));

        mSaveButton->setEnabled(false);
    }
    else
    {
        LL_WARNS("Region") << "Saving region schedule, but RegionSchedule capability is not available" << LL_ENDL;
    }
}

void LLFloaterRegionRestartSchedule::onCommitHours(const LLSD& value)
{
    S32 hours = value.asInteger();
    if (mPMAMButton->getVisible())
    {
        // 0:00 equals 12:00 AM 1:00 equals 1:00 AM, 12am < 1am < 2am < 3am...
        if (hours == 0) hours = 12;
        llclamp(hours, 1, 12);
    }
    else
    {
        llclamp(hours, 0, 23);
    }
    mHoursLineEditor->setText(llformat("%02d", hours));
    mSaveButton->setEnabled(true);
}

void LLFloaterRegionRestartSchedule::onCommitMinutes(const LLSD& value)
{
    S32 minutes = value.asInteger();
    llclamp(minutes, 0, 59);
    mMinutesLineEditor->setText(llformat("%02d", minutes));
    mSaveButton->setEnabled(true);
}

void LLFloaterRegionRestartSchedule::resetUI(bool enable_ui)
{
    for (char c : CHECKBOX_PREFIXES)
    {
        std::string name = c + CHECKBOX_NAME;
        LLCheckBoxCtrl* chk = getChild<LLCheckBoxCtrl>(name);
        chk->setValue(false);
        chk->setEnabled(enable_ui);
    }
    if (mPMAMButton->getVisible())
    {
        mHoursLineEditor->setValue("12");
        mPMAMButton->setEnabled(enable_ui);
    }
    else
    {
        mHoursLineEditor->setValue("00");
    }
    mMinutesLineEditor->setValue("00");
    mMinutesLineEditor->setEnabled(enable_ui);
    mHoursLineEditor->setEnabled(enable_ui);
    mTimeAM = true;
    updateAMPM();
}

void LLFloaterRegionRestartSchedule::updateAMPM()
{
    std::string value;
    if (mTimeAM)
    {
        value = getString("am_string");
    }
    else
    {
        value = getString("pm_string");
    }
    mPMAMButton->setLabel(value);
}

bool LLFloaterRegionRestartSchedule::canUse()
{
    std::string url = gAgent.getRegionCapability("RegionSchedule");
    return !url.empty();
}

void LLFloaterRegionRestartSchedule::requestRegionShcheduleCoro(std::string url, LLHandle<LLFloater> handle)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("RegionShcheduleRequest", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url, httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    LLFloaterRegionRestartSchedule* floater = dynamic_cast<LLFloaterRegionRestartSchedule*>(handle.get());
    if (!floater)
    {
        LL_DEBUGS("Region") << "Region Restart Schedule floater is already dead" << LL_ENDL;
    }
    else if (!status)
    {
        LL_WARNS("Region") << "Failed to get region schedule: " << status.toString() << LL_ENDL;
        floater->resetUI(false);
    }
    else if (!result.has("restart"))
    {
        floater->resetUI(true); // no restart schedule yet
    }
    else
    {
        // example: 'restart':{'days':'TR','time':i7200,'type':'W'}
        LLSD &restart = result["restart"];
        std::string type = restart["type"];
        std::string days = restart["days"];
        if (type == "W") // weekly || restart.has("days")
        {
            LLStringUtil::toLower(days);
            for (char c : CHECKBOX_PREFIXES)
            {
                bool enabled = days.find(c) != std::string::npos;
                std::string name = c + CHECKBOX_NAME;
                LLCheckBoxCtrl *chk = floater->getChild<LLCheckBoxCtrl>(name);
                chk->setValue(enabled);
                chk->setEnabled(true);
            }
        }
        else // dayly
        {
            for (char c : CHECKBOX_PREFIXES)
            {
                std::string name = c + CHECKBOX_NAME;
                LLCheckBoxCtrl* chk = floater->getChild<LLCheckBoxCtrl>(name);
                chk->setValue(true);
                chk->setEnabled(true);
            }
        }

        S32 seconds_after_midnight = restart["time"].asInteger();
        S32 hours = seconds_after_midnight / 3600;
        S32 minutes = (seconds_after_midnight % 3600) / 60;

        if (floater->mPMAMButton->getVisible())
        {
            if (hours >= 12)
            {
                hours -= 12;
                floater->mTimeAM = false;
            }
            else
            {
                floater->mTimeAM = true;
            }
            if (hours == 0)
            {
                hours = 12; // 0:00 equals 12:00 AM , 1:00 equals 1:00 AM
            }
            floater->mPMAMButton->setEnabled(true);
        }
        else
        {
            floater->mTimeAM = true;
        }
        floater->updateAMPM();
        floater->mHoursLineEditor->setText(llformat("%02d", hours));
        floater->mHoursLineEditor->setEnabled(true);
        floater->mMinutesLineEditor->setText(llformat("%02d", minutes));
        floater->mMinutesLineEditor->setEnabled(true);

        LL_DEBUGS("Region") << "Region restart schedule type: " << type
            << " Days: " << days
            << " Time:" << hours << ":" << minutes << LL_ENDL;
    }
}

void LLFloaterRegionRestartSchedule::setRegionShcheduleCoro(std::string url, LLSD body, LLHandle<LLFloater> handle)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("RegionShcheduleSetter", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, body, httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    LLFloaterRegionRestartSchedule* floater = dynamic_cast<LLFloaterRegionRestartSchedule*>(handle.get());
    if (floater)
    {
        floater->closeFloater();
    }
}
