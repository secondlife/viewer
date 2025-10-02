/**
 * @file llfloatermoderation.cpp
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llavataractions.h"
#include "lldateutil.h"
#include "llsdutil.h"
#include "llscrolllistctrl.h"
#include "lluictrlfactory.h"
#include "lluuid.h"
#include "llview.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "llwindow.h"
#include "llworld.h"

#include <random>

#include "llfloatermoderation.h"

LLFloaterModeration::LLFloaterModeration(const LLSD& key) : LLFloater(key)
{
}

LLFloaterModeration::~LLFloaterModeration()
{
}

void LLFloaterModeration::refresh()
{
    onRefreshList();
}

bool LLFloaterModeration::postBuild()
{
    mRefreshListBtn = getChild<LLUICtrl>("refresh_list_btn");
    mRefreshListBtn->setEnabled(true);
    mRefreshListBtn->setCommitCallback([this](LLUICtrl*, void*)
    {
        onRefreshList();
    }, nullptr);

    mSelectAllBtn = getChild<LLUICtrl>("select_all_btn");
    mSelectAllBtn->setEnabled(true);
    mSelectAllBtn->setCommitCallback([this](LLUICtrl*, void*)
    {
        mResidentListScroller->selectAll();
    }, nullptr);

    mSelectNoneBtn = getChild<LLUICtrl>("select_none_btn");
    mSelectNoneBtn->setEnabled(true);
    mSelectNoneBtn->setCommitCallback([this](LLUICtrl*, void*)
    {
        // Thgere really ought to be a ::selectNone()...
        mResidentListScroller->deselect();
    }, nullptr);

    mShowProfileBtn = getChild<LLUICtrl>("show_resident_profile_btn");
    mShowProfileBtn->setEnabled(true);
    mShowProfileBtn->setCommitCallback([this](LLUICtrl*, void*)
    {
        openSelectedProfile();
    }, nullptr);

    mTrackResidentBtn = getChild<LLUICtrl>("track_resident_btn");
    mTrackResidentBtn ->setEnabled(true);
    mTrackResidentBtn->setCommitCallback([this](LLUICtrl*, void*)
    {
        trackResidentPosition();
    }, nullptr);

    mMuteResidentsBtn = getChild<LLUICtrl>("mute_residents_btn");
    mMuteResidentsBtn->setEnabled(true);
    mMuteResidentsBtn->setCommitCallback([this](LLUICtrl*, void*)
    {
        muteResidents();
    }, nullptr);

    mUnmuteResidentsBtn = getChild<LLUICtrl>("unmute_residents_btn");
    mUnmuteResidentsBtn->setEnabled(true);
    mUnmuteResidentsBtn->setCommitCallback([this](LLUICtrl*, void*)
    {
        unmuteResidents();
    }, nullptr);

    mCloseBtn = getChild<LLButton>("close_btn");
    mCloseBtn->setEnabled(true);
    mCloseBtn->setCommitCallback([this](LLUICtrl*, void*)
    {
        closeFloater();
    }, nullptr);

    // Let users double click on an entry on the list to open the relevant profile floater
    mResidentListScroller = getChild<LLScrollListCtrl>("moderation_resident_info_list");
    mResidentListScroller->setDoubleClickCallback(boost::bind(&LLFloaterModeration::onDoubleClickListItem, this));

    // Start by refreshing the list of residents around us
    onRefreshList();

    return true;
}

void LLFloaterModeration::draw()
{
    LLFloater::draw();

    // Enable/disable some buttons if 0 or > 1 resident is selected
    const bool single_select = mResidentListScroller->getNumSelected() == 1 ? true : false;
    mTrackResidentBtn->setEnabled(single_select);
    mShowProfileBtn->setEnabled(single_select);

    // Enable/disable some buttons if > 0 resident(s) are selected
    const bool something_selected = mResidentListScroller->getNumSelected() > 0 ? true : false;
    mMuteResidentsBtn->setEnabled(something_selected);
    mUnmuteResidentsBtn->setEnabled(something_selected);
}

void LLFloaterModeration::onRefreshList()
{
    refreshList();
    refreshUI();
}

void LLFloaterModeration::sortListByLoudness()
{
    sort(mResidentList.begin(), mResidentList.end(),
         [ = ](list_elem_t*& a, list_elem_t*& b)
    {
        return a->recent_loudness > b->recent_loudness;
    });
}

void LLFloaterModeration::trimList(size_t final_size)
{
    while (mResidentList.size() > final_size)
    {
        list_elem_t* elem = mResidentList.back();
        delete elem;
        mResidentList.pop_back();
    }

    if (final_size == 0)
    {
        mResidentList.clear();
    }
}

void LLFloaterModeration::refreshList()
{
    LLVector3d my_pos = gAgent.getPositionGlobal();

    std::vector<LLVector3d> positions;
    uuid_vec_t avatar_ids;
    const auto range = 1024; // TODO: fix this for region or parcel
    LLWorld::getInstance()->getAvatars(&avatar_ids, &positions, my_pos, range);

    trimList(0);

    for (auto i = 0; i < avatar_ids.size(); i++)
    {
        list_element* elem = new list_element;
        elem->id = avatar_ids[i];

        // Actual value for resident name (various formats depending on what
        // we have cached, but should be sufficient until it gets populated
        // asynchronously with the AvatarInformation callback
        LLAvatarName av_name;

        if (! LLAvatarNameCache::get(avatar_ids[i], &av_name))
        {
            elem->name = STRINGIZE(gCacheName->getDefaultName());
        }
        else
        {
            elem->name = STRINGIZE(av_name.getDisplayName() << " (" << av_name.getUserName() << ")");
        }

        // placeholder value - actual value comes in asynchronously via a callback
        elem->born_on = LLDate::now();

        // actual value since we have all the information we need to calculate it
        elem->distance = dist_vec(positions[i], my_pos);

        // Check if this user is a Linden
        elem->is_linden = isLinden(avatar_ids[i]);

        // Collect how loud they have been recently
        elem->recent_loudness = getRecentLoudness(avatar_ids[i]);

        // check if this user has their voice muted
        elem->is_voice_muted = false;
        LLVOAvatar* avatar = getAvatarFromId(avatar_ids[i]);
        if (avatar)
        {
            elem->is_voice_muted = (avatar->getNearbyVoiceMuteSettings() == LLVOAvatar::AV_NEARBY_VOICE_MUTED);
        }

        mResidentList.push_back(elem);

        // Observe and issue a request for additional details about this resident
        // e.g. name if not in cache, age and more.
        LLAvatarPropertiesProcessor::getInstance()->addObserver(avatar_ids[i], this);
        LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesRequest(avatar_ids[i]);
    }

    addDummyResident("Snowshoe Cringifoot");
    addDummyResident("Applepie Kitterbul");
    addDummyResident("Wigglepod Bundersauce");
    addDummyResident("Hufflesnuff Potterwhag");
    addDummyResident("Joly Lotbinière");

    // Initial state if sorted by loudness since this is likely whom you're looking to moderate
    sortListByLoudness();
}

void LLFloaterModeration::addDummyResident(const std::string name)
{
    list_element* elem = new list_element;
    elem->id = LLUUID::null;
    elem->name = name;
    elem->born_on = LLDate::now();
    elem->distance = 5.0;
    elem->is_linden = false;
    elem->is_voice_muted = false;
    elem->recent_loudness = getRecentLoudness(LLUUID::null);
    mResidentList.push_back(elem);
}

/**
 *  Virtual override for LLAvatarPropertiesObserver - used to collect avatar
 *  data asynchronously like account age and name updates (and other things later)
 */
void LLFloaterModeration::processProperties(void* data, EAvatarProcessorType type)
{
    LLAvatarData* avatar_data = static_cast<LLAvatarData*>(data);

    if (avatar_data)
    {
        if (data && type == APT_PROPERTIES)
        {
            for (struct list_element* entry : mResidentList)
            {
                if (entry->id == avatar_data->avatar_id)
                {
                    entry->born_on = avatar_data->born_on;
                }
            }

            refreshUI();
        }

        if (LLAvatarPropertiesProcessor::instanceExists())
        {
            LLAvatarPropertiesProcessor::getInstance()->removeObserver(avatar_data->avatar_id, this);
        }
    }
}

void LLFloaterModeration::refreshUI()
{
    mResidentListScroller->deleteAllItems();

    for (auto iter = mResidentList.begin(); iter != mResidentList.end(); ++iter)
    {
        LLUUID av_uuid = (*iter)->id;
        std::string row_num_str(STRINGIZE(1 + std::distance(std::begin(mResidentList), iter)));
        std::string agent_name = (*iter)->name;
        std::string is_linden_icon = (*iter)->is_linden ? "Profile_Badge_Linden" : "";
        std::string is_voice_muted_icon = (*iter)->is_voice_muted ? "VoiceMute_Off" : "VoicePTT_Lvl2";
        std::string account_age_str = LLDateUtil::ageFromDate((*iter)->born_on, LLDate::now());
        std::string recent_loudness_str = STRINGIZE((*iter)->recent_loudness);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << (*iter)->distance << "m";
        std::string dist_str(ss.str());

        // Disambiguate Lindens
        std::string font_style = "NORMAL";
        if (av_uuid == gAgent.getID())
        {
            font_style = "BOLD|ITALIC";
        }

        LLSD row;
        // ID is hidden - used to retrieve other info later on
        row["columns"][EListColumnNum::ID]["column"] = "id_column";
        row["columns"][EListColumnNum::ID]["type"] = "text";
        row["columns"][EListColumnNum::ID]["value"] = STRINGIZE(av_uuid);
        row["columns"][EListColumnNum::ID]["font"]["name"] = mScrollListFontFace;
        row["columns"][EListColumnNum::ID]["font"]["style"] = font_style;

        // Useful to have a visual count of the numnber of residents (maybe)
        row["columns"][EListColumnNum::ROWNUM]["column"] = "number_column";
        row["columns"][EListColumnNum::ROWNUM]["type"] = "text";
        row["columns"][EListColumnNum::ROWNUM]["value"] = row_num_str;
        row["columns"][EListColumnNum::ROWNUM]["font"]["name"] = mScrollListFontFace;
        row["columns"][EListColumnNum::ROWNUM]["font"]["style"] = font_style;

        // The name of the resident
        row["columns"][EListColumnNum::NAME]["column"] = "name_column";
        row["columns"][EListColumnNum::NAME]["type"] = "text";
        row["columns"][EListColumnNum::NAME]["value"] = agent_name;
        row["columns"][EListColumnNum::NAME]["font"]["name"] = mScrollListFontFace;
        row["columns"][EListColumnNum::NAME]["font"]["style"] = font_style;

        // The age of the resident
        row["columns"][EListColumnNum::ACCOUNT_AGE]["column"] = "account_age_column";
        row["columns"][EListColumnNum::ACCOUNT_AGE]["type"] = "text";
        row["columns"][EListColumnNum::ACCOUNT_AGE]["value"] = account_age_str;
        row["columns"][EListColumnNum::ACCOUNT_AGE]["font"]["name"] = mScrollListFontFace;
        row["columns"][EListColumnNum::ACCOUNT_AGE]["font"]["style"] = font_style;

        // The distance of the resident from the person using the tool
        row["columns"][EListColumnNum::DISTANCE]["column"] = "distance_column";
        row["columns"][EListColumnNum::DISTANCE]["type"] = "text";
        row["columns"][EListColumnNum::DISTANCE]["value"] = dist_str;
        row["columns"][EListColumnNum::DISTANCE]["font"]["name"] = mScrollListFontFace;
        row["columns"][EListColumnNum::DISTANCE]["font"]["style"] = font_style;

        // Whether or not the resident is a Linden (in case that determines the mute or not descision :) )
        row["columns"][EListColumnNum::LINDEN]["column"] = "linden_column";
        row["columns"][EListColumnNum::LINDEN]["type"] = "icon";
        row["columns"][EListColumnNum::LINDEN]["value"] = is_linden_icon;
        row["columns"][EListColumnNum::LINDEN]["halign"] = LLFontGL::HCENTER;
        row["columns"][EListColumnNum::LINDEN]["font"]["name"] = mScrollListFontFace;

        // Whether or not the resident voice muted
        row["columns"][EListColumnNum::VOICE_MUTED]["column"] = "voice_muted_column";
        row["columns"][EListColumnNum::VOICE_MUTED]["type"] = "icon";
        row["columns"][EListColumnNum::VOICE_MUTED]["value"] = is_voice_muted_icon;
        row["columns"][EListColumnNum::VOICE_MUTED]["halign"] = LLFontGL::HCENTER;
        row["columns"][EListColumnNum::VOICE_MUTED]["font"]["name"] = mScrollListFontFace;

        // How "loud" the resident has been recently
        row["columns"][EListColumnNum::RECENT_LOUDNESS]["column"] = "recent_loudness_column";
        row["columns"][EListColumnNum::RECENT_LOUDNESS]["type"] = "text";
        row["columns"][EListColumnNum::RECENT_LOUDNESS]["value"] = recent_loudness_str;
        row["columns"][EListColumnNum::RECENT_LOUDNESS]["halign"] = LLFontGL::HCENTER;
        row["columns"][EListColumnNum::RECENT_LOUDNESS]["font"]["name"] = mScrollListFontFace;
        row["columns"][EListColumnNum::RECENT_LOUDNESS]["font"]["style"] = font_style;

        LLScrollListItem* item = mResidentListScroller->addElement(row);
    }
}

void LLFloaterModeration::onDoubleClickListItem(void* data)
{
    LLFloaterModeration* self = (LLFloaterModeration*)data;

    self->openSelectedProfile();
}

LLUUID LLFloaterModeration::getSelectedAvatarId()
{
    LLScrollListItem* first_selected = mResidentListScroller->getFirstSelected();
    if (! first_selected)
    {
        return LLUUID::null;
    }

    const LLScrollListCell* id_cell = first_selected->getColumn(EListColumnNum::ID);
    if (! id_cell)
    {
        return LLUUID::null;
    }

    return LLUUID(id_cell->getValue().asString());
}

void LLFloaterModeration::openSelectedProfile()
{
    LLUUID selectded_id = getSelectedAvatarId();
    if (selectded_id != LLUUID::null)
    {
        LLAvatarActions::showProfile(selectded_id);
    }
}

bool LLFloaterModeration::isLinden(const LLUUID& av_id)
{
    std::string first_name, last_name;
    LLAvatarName av_name;

    if (LLAvatarNameCache::get(av_id, &av_name))
    {
        std::istringstream full_name(av_name.getUserName());
        full_name >> first_name >> last_name;
    }
    else
    {
        gCacheName->getFirstLastName(av_id, first_name, last_name);
    }

    const std::string LL_LINDEN = "Linden";
    const std::string LL_MOLE = "Mole";
    const std::string LL_PRODUCTENGINE = "ProductEngine";
    const std::string LL_SCOUT = "Scout";
    const std::string LL_TESTER = "Tester";

    return (last_name == LL_LINDEN ||
            last_name == LL_MOLE ||
            last_name == LL_PRODUCTENGINE ||
            last_name == LL_SCOUT ||
            last_name == LL_TESTER);
}

int LLFloaterModeration::getRecentLoudness(const LLUUID& av_id)
{
    // random value 0 (church mouse) to 100 (death metal) for now
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, 100);

    int loudness = distr(gen);

    return loudness;
}

void LLFloaterModeration::trackResidentPosition()
{
    LLUUID selectded_id = getSelectedAvatarId();
    if (selectded_id != LLUUID::null)
    {
        LLAvatarActions::showOnMap(selectded_id);
    }
}

LLVOAvatar* LLFloaterModeration::getAvatarFromId(const LLUUID& id)
{
    LLViewerObject *obj = gObjectList.findObject(id);
    while (obj && obj->isAttachment())
    {
        obj = (LLViewerObject*)obj->getParent();
    }

    if (obj && obj->isAvatar())
    {
        return (LLVOAvatar*)obj;
    }
    else
    {
        return NULL;
    }
}

void LLFloaterModeration::applyActionSelectedResidents(EResidentAction action)
{
    std::vector<LLScrollListItem*> selected = mResidentListScroller->getAllSelected();
    for (const LLScrollListItem* s : selected)
    {
        const LLScrollListCell* id_cell = s->getColumn(EListColumnNum::ID);
        const LLScrollListCell* name_cell = s->getColumn(EListColumnNum::NAME);

        if (id_cell && name_cell)
        {
            LLVOAvatar* avatar = getAvatarFromId(id_cell->getValue().asUUID());
            if (avatar)
            {
                if (action == EResidentAction::MUTE)
                {
                    LL_INFOS() << "    "
                               << name_cell->getValue().asString()
                               << " (" << id_cell->getValue().asString() << ")"
                               << LL_ENDL;

                    avatar->setNearbyVoiceMuteSettings(LLVOAvatar::AV_NEARBY_VOICE_MUTED);
                }
                else if (action == EResidentAction::UNMUTE)
                {
                    LL_INFOS() << "    "
                               << name_cell->getValue().asString()
                               << " (" << id_cell->getValue().asString() << ")"
                               << LL_ENDL;

                    avatar->setNearbyVoiceMuteSettings(LLVOAvatar::AV_NEARBY_VOICE_UNMUTED);
                }
            }
        }
    }

    // Update internal storage and then UI to reflect any modifications that we made
    // TODO: only refresh if something changed but the overhead is so small that it may not be worth it.
    onRefreshList();
}

void LLFloaterModeration::muteResidents()
{
    LL_INFOS() << "Muting " << mResidentListScroller->getNumSelected() << " selected residents:" << LL_ENDL;
    applyActionSelectedResidents(EResidentAction::MUTE);
}

void LLFloaterModeration::unmuteResidents()
{
    LL_INFOS() << "Unmuting " << mResidentListScroller->getNumSelected() << " selected residents" << LL_ENDL;
    applyActionSelectedResidents(EResidentAction::UNMUTE);
}

void LLNearbyVoiceMuteHelper::requestMuteChange(LLVOAvatar* avatar, bool mute)
{
    if (avatar)
    {
        LLViewerRegion* region = avatar->getRegion();
        if (! region || ! region->capabilitiesReceived())
        {
            // retry ?
            LL_INFOS() << "Region or region capabilities unavailable" << LL_ENDL;
            return;
        }
        LL_INFOS() << "Region name is " << region->getName() << LL_ENDL;

        std::string url = region->getCapability("SpatialVoiceModerationRequest");
        if (url.empty())
        {
            // retry ?
            LL_INFOS() << "Capability URL is empty" << LL_ENDL;
            return;
        }
        LL_INFOS() << "Capability URL is " << url << LL_ENDL;

        const std::string agent_name = avatar->getFullname();
        const LLUUID agent_id = avatar->getID();

        const std::string operand = mute ? "mute" : "unmute";

        LLSD body;
        body["operand"] = operand;
        body["agent_id"] = agent_id;
        body["moderator_id"] = gAgent.getID(); // consider sending moderator ID too ??

        LL_INFOS() << "Capability body is " << body << LL_ENDL;

        LL_INFOS() <<
        "Resident " <<
                   agent_name <<
        " (" <<
                   agent_id <<
        ")" <<
        " applying " <<
                   operand <<
                   LL_ENDL;

        std::string success_msg =
            STRINGIZE("Resident " <<
                      agent_name <<
        " (" <<
                      agent_id <<
        ")" <<
        " nearby voice was set to " <<
                      operand);

        std::string failure_msg =
            STRINGIZE("Unable to change voice muting for resident " <<
                      agent_name <<
        " (" <<
                      agent_id <<
        ")");

        LLCoreHttpUtil::HttpCoroutineAdapter::messageHttpPost(url, body,
                success_msg,
                failure_msg);
    }
}

