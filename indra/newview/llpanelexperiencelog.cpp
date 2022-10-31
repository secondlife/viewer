/** 
 * @file llpanelexperiencelog.cpp
 * @brief llpanelexperiencelog
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


#include "llviewerprecompiledheaders.h"
#include "llpanelexperiencelog.h"

#include "llexperiencelog.h"
#include "llexperiencecache.h"
#include "llbutton.h"
#include "llscrolllistctrl.h"
#include "llcombobox.h"
#include "llspinctrl.h"
#include "llcheckboxctrl.h"
#include "llfloaterreg.h"
#include "llfloaterreporter.h"
#include "llinventoryfunctions.h"


#define BTN_PROFILE_XP "btn_profile_xp"
#define BTN_REPORT_XP "btn_report_xp"

static LLPanelInjector<LLPanelExperienceLog> register_experiences_panel("experience_log");


LLPanelExperienceLog::LLPanelExperienceLog(  )
    : mEventList(NULL)
    , mPageSize(25)
    , mCurrentPage(0)
{
    buildFromFile("panel_experience_log.xml");
}

BOOL LLPanelExperienceLog::postBuild( void )
{
    LLExperienceLog* log = LLExperienceLog::getInstance();
    mEventList = getChild<LLScrollListCtrl>("experience_log_list");
    mEventList->setCommitCallback(boost::bind(&LLPanelExperienceLog::onSelectionChanged, this));
    mEventList->setDoubleClickCallback( boost::bind(&LLPanelExperienceLog::onProfileExperience, this));

    getChild<LLButton>("btn_clear")->setCommitCallback(boost::bind(&LLExperienceLog::clear, log));
    getChild<LLButton>("btn_clear")->setCommitCallback(boost::bind(&LLPanelExperienceLog::refresh, this));

    getChild<LLButton>(BTN_PROFILE_XP)->setCommitCallback(boost::bind(&LLPanelExperienceLog::onProfileExperience, this));
    getChild<LLButton>(BTN_REPORT_XP )->setCommitCallback(boost::bind(&LLPanelExperienceLog::onReportExperience, this));
    getChild<LLButton>("btn_notify"  )->setCommitCallback(boost::bind(&LLPanelExperienceLog::onNotify, this));
    getChild<LLButton>("btn_next"    )->setCommitCallback(boost::bind(&LLPanelExperienceLog::onNext, this));
    getChild<LLButton>("btn_prev"    )->setCommitCallback(boost::bind(&LLPanelExperienceLog::onPrev, this));

    LLCheckBoxCtrl* check = getChild<LLCheckBoxCtrl>("notify_all");
    check->set(log->getNotifyNewEvent());
    check->setCommitCallback(boost::bind(&LLPanelExperienceLog::notifyChanged, this));


    LLSpinCtrl* spin = getChild<LLSpinCtrl>("logsizespinner");
    spin->set(log->getMaxDays());
    spin->setCommitCallback(boost::bind(&LLPanelExperienceLog::logSizeChanged, this));

    mPageSize = log->getPageSize();
    refresh();
    mNewEvent = LLExperienceLog::instance().addUpdateSignal(boost::bind(&LLPanelExperienceLog::refresh, this));
    return TRUE;
}

LLPanelExperienceLog* LLPanelExperienceLog::create()
{
    return new LLPanelExperienceLog();
}

void LLPanelExperienceLog::refresh()
{
    S32 selected = mEventList->getFirstSelectedIndex();
    mEventList->deleteAllItems();
    const LLSD events = LLExperienceLog::instance().getEvents();

    if(events.size() == 0)
    {
        mEventList->setCommentText(getString("no_events"));
        return;
    }

    setAllChildrenEnabled(FALSE);

    LLSD item;
    bool waiting = false;
    LLUUID waiting_id;

    int itemsToSkip = mPageSize*mCurrentPage;
    int items = 0;
    bool moreItems = false;
    LLSD events_to_save = events;
    if (!events.emptyMap())
    {
        LLSD::map_const_iterator day = events.endMap();
        do
        {
            --day;
            const LLSD& dayArray = day->second;

            std::string date = day->first;
            if(!LLExperienceLog::instance().isNotExpired(date))
            {
                events_to_save.erase(day->first);
                continue;
            }
            int size = dayArray.size();
            if(itemsToSkip > size)
            {
                itemsToSkip -= size;
                continue;
            }
            if(items >= mPageSize && size > 0)
            {
                moreItems = true;
                break;
            }
            for(int i = dayArray.size() - itemsToSkip - 1; i >= 0; i--)
            {
                if(items >= mPageSize)
                {
                    moreItems = true;
                    break;
                }
                const LLSD event = dayArray[i];
                LLUUID id = event[LLExperienceCache::EXPERIENCE_ID].asUUID();
                const LLSD& experience = LLExperienceCache::instance().get(id);
                if(experience.isUndefined()){
                    waiting = true;
                    waiting_id = id;
                }
                if(!waiting)
                {
                    item["id"] = event;

                    LLSD& columns = item["columns"];
                    columns[0]["column"] = "time";
                    columns[0]["value"] = day->first+event["Time"].asString();
                    columns[1]["column"] = "event";
                    columns[1]["value"] = LLExperienceLog::getPermissionString(event, "ExperiencePermissionShort");
                    columns[2]["column"] = "experience_name";
                    columns[2]["value"] = experience[LLExperienceCache::NAME].asString();
                    columns[3]["column"] = "object_name";
                    columns[3]["value"] = event["ObjectName"].asString();
                    mEventList->addElement(item);
                }
                ++items;
            }
        } while (day != events.beginMap());
    }
    LLExperienceLog::getInstance()->setEventsToSave(events_to_save);
    if(waiting)
    {
        mEventList->deleteAllItems();
        mEventList->setCommentText(getString("loading"));
        LLExperienceCache::instance().get(waiting_id, boost::bind(&LLPanelExperienceLog::refresh, this));
    }
    else
    {
        setAllChildrenEnabled(TRUE);

        mEventList->setEnabled(TRUE);
        getChild<LLButton>("btn_next")->setEnabled(moreItems);
        getChild<LLButton>("btn_prev")->setEnabled(mCurrentPage>0);
        getChild<LLButton>("btn_clear")->setEnabled(mEventList->getItemCount()>0);
        if(selected<0)
        {
            selected = 0;
        }
        mEventList->selectNthItem(selected);
        onSelectionChanged();
    }
}

void LLPanelExperienceLog::onProfileExperience()
{
    LLSD event = getSelectedEvent();
    if(event.isDefined())
    {
        LLFloaterReg::showInstance("experience_profile", event[LLExperienceCache::EXPERIENCE_ID].asUUID(), true);
    }
}

void LLPanelExperienceLog::onReportExperience()
{
    LLSD event = getSelectedEvent();
    if(event.isDefined())
    {
        LLFloaterReporter::showFromExperience(event[LLExperienceCache::EXPERIENCE_ID].asUUID());
    }
}

void LLPanelExperienceLog::onNotify()
{
    LLSD event = getSelectedEvent();
    if(event.isDefined())
    {
        LLExperienceLog::instance().notify(event);
    }
}

void LLPanelExperienceLog::onNext()
{
    mCurrentPage++;
    refresh();
}

void LLPanelExperienceLog::onPrev()
{
    if(mCurrentPage>0)
    {
        mCurrentPage--;
        refresh();
    }
}

void LLPanelExperienceLog::notifyChanged()
{
    LLExperienceLog::instance().setNotifyNewEvent(getChild<LLCheckBoxCtrl>("notify_all")->get());
}

void LLPanelExperienceLog::logSizeChanged()
{
    int value = (int)(getChild<LLSpinCtrl>("logsizespinner")->get());
    LLExperienceLog::instance().setMaxDays(value);
    refresh();
}

void LLPanelExperienceLog::onSelectionChanged()
{
    bool enabled = (1 == mEventList->getNumSelected());
    getChild<LLButton>(BTN_REPORT_XP)->setEnabled(enabled);
    getChild<LLButton>(BTN_PROFILE_XP)->setEnabled(enabled);
    getChild<LLButton>("btn_notify")->setEnabled(enabled);
}

LLSD LLPanelExperienceLog::getSelectedEvent()
{
    LLScrollListItem* item = mEventList->getFirstSelected();
    if(item)
    {
        return item->getValue();
    }
    return LLSD();
}
