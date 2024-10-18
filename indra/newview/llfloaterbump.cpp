/**
 * @file llfloaterbump.cpp
 * @brief Floater showing recent bumps, hits with objects, pushes, etc.
 * @author Cory Ondrejka, James Cook
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "llsd.h"
#include "mean_collision_data.h"

#include "llavataractions.h"
#include "llfloaterbump.h"
#include "llfloaterreg.h"
#include "llfloaterreporter.h"
#include "llmutelist.h"
#include "llpanelblockedlist.h"
#include "llscrolllistctrl.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llviewermessage.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"

///----------------------------------------------------------------------------
/// Class LLFloaterBump
///----------------------------------------------------------------------------

// Default constructor
LLFloaterBump::LLFloaterBump(const LLSD& key)
:   LLFloater(key)
{
    mCommitCallbackRegistrar.add("Avatar.SendIM", { boost::bind(&LLFloaterBump::startIM, this), cb_info::UNTRUSTED_THROTTLE });
    mCommitCallbackRegistrar.add("Avatar.ReportAbuse", { boost::bind(&LLFloaterBump::reportAbuse, this), cb_info::UNTRUSTED_THROTTLE });
    mCommitCallbackRegistrar.add("ShowAgentProfile", { boost::bind(&LLFloaterBump::showProfile, this), cb_info::UNTRUSTED_THROTTLE });
    mCommitCallbackRegistrar.add("Avatar.InviteToGroup", { boost::bind(&LLFloaterBump::inviteToGroup, this), cb_info::UNTRUSTED_THROTTLE });
    mCommitCallbackRegistrar.add("Avatar.Call", { boost::bind(&LLFloaterBump::startCall, this), cb_info::UNTRUSTED_BLOCK });
    mEnableCallbackRegistrar.add("Avatar.EnableCall", { boost::bind(&LLFloaterBump::canCall, this), cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("Avatar.AddFriend", { boost::bind(&LLFloaterBump::addFriend, this), cb_info::UNTRUSTED_THROTTLE });
    mEnableCallbackRegistrar.add("Avatar.EnableAddFriend", boost::bind(&LLFloaterBump::enableAddFriend, this));
    mCommitCallbackRegistrar.add("Avatar.Mute", { boost::bind(&LLFloaterBump::muteAvatar, this), cb_info::UNTRUSTED_BLOCK });
    mEnableCallbackRegistrar.add("Avatar.EnableMute", boost::bind(&LLFloaterBump::enableMute, this));
    mCommitCallbackRegistrar.add("PayObject", { boost::bind(&LLFloaterBump::payAvatar, this), cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("Tools.LookAtSelection", { boost::bind(&LLFloaterBump::zoomInAvatar, this) });
}


// Destroys the object
LLFloaterBump::~LLFloaterBump()
{
    auto menu = mPopupMenuHandle.get();
    if (menu)
    {
        menu->die();
        mPopupMenuHandle.markDead();
    }
}

bool LLFloaterBump::postBuild()
{
    mList = getChild<LLScrollListCtrl>("bump_list");
    mList->setAllowMultipleSelection(false);
    mList->setRightMouseDownCallback(boost::bind(&LLFloaterBump::onScrollListRightClicked, this, _1, _2, _3));

    LLContextMenu* menu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>("menu_avatar_other.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
    if (menu)
    {
        mPopupMenuHandle = menu->getHandle();
        menu->setItemVisible(std::string("Normal"), false);
        menu->setItemVisible(std::string("Always use impostor"), false);
        menu->setItemVisible(std::string("Never use impostor"), false);
        menu->setItemVisible(std::string("Impostor seperator"), false);
    }

    return true;
}
// virtual
void LLFloaterBump::onOpen(const LLSD& key)
{
    if (gMeanCollisionList.empty())
    {
        mNames.clear();
        mList->deleteAllItems();

        std::string none_detected = getString("none_detected");
        LLSD row;
        row["columns"][0]["value"] = none_detected;
        row["columns"][0]["font"] = "SansSerifBold";
        mList->addElement(row);
    }
    else
    {
        populateCollisionList();
    }
}

void LLFloaterBump::populateCollisionList()
{
    mNames.clear();
    mList->deleteAllItems();

    for (mean_collision_list_t::iterator iter = gMeanCollisionList.begin();
                 iter != gMeanCollisionList.end(); ++iter)
    {
        LLMeanCollisionData *mcd = *iter;
        add(mList, mcd);
    }
}

void LLFloaterBump::add(LLScrollListCtrl* list, LLMeanCollisionData* mcd)
{
    if (mcd->mFullName.empty() || list->getItemCount() >= 20)
    {
        return;
    }

    std::string timeStr = getString ("timeStr");
    LLSD substitution;

    substitution["datetime"] = (S32) mcd->mTime;
    LLStringUtil::format (timeStr, substitution);

    std::string action;
    switch(mcd->mType)
    {
    case MEAN_BUMP:
        action = "bump";
        break;
    case MEAN_LLPUSHOBJECT:
        action = "llpushobject";
        break;
    case MEAN_SELECTED_OBJECT_COLLIDE:
        action = "selected_object_collide";
        break;
    case MEAN_SCRIPTED_OBJECT_COLLIDE:
        action = "scripted_object_collide";
        break;
    case MEAN_PHYSICAL_OBJECT_COLLIDE:
        action = "physical_object_collide";
        break;
    default:
        LL_INFOS() << "LLFloaterBump::add unknown mean collision type "
            << mcd->mType << LL_ENDL;
        return;
    }

    // All above action strings are in XML file
    LLUIString text = getString(action);
    text.setArg("[TIME]", timeStr);
    text.setArg("[NAME]", mcd->mFullName);

    LLSD row;
    row["id"] = mcd->mPerp;
    row["columns"][0]["value"] = text;
    row["columns"][0]["font"] = "SansSerifBold";
    list->addElement(row);


    mNames[mcd->mPerp] = mcd->mFullName;
}


void LLFloaterBump::onScrollListRightClicked(LLUICtrl* ctrl, S32 x, S32 y)
{
    if (!gMeanCollisionList.empty())
    {
        LLScrollListItem* item = mList->hitItem(x, y);
        auto menu = mPopupMenuHandle.get();
        if (item && menu)
        {
            mItemUUID = item->getUUID();
            menu->buildDrawLabels();
            menu->updateParent(LLMenuGL::sMenuContainer);

            std::string mute_msg = (LLMuteList::getInstance()->isMuted(mItemUUID, mNames[mItemUUID])) ? "UnmuteAvatar" : "MuteAvatar";
            menu->getChild<LLUICtrl>("Avatar Mute")->setValue(LLTrans::getString(mute_msg));
            menu->setItemEnabled(std::string("Zoom In"), bool(gObjectList.findObject(mItemUUID)));

            menu->show(x, y);
            LLMenuGL::showPopup(ctrl, menu, x, y);
        }
    }
}


void LLFloaterBump::startIM()
{
    LLAvatarActions::startIM(mItemUUID);
}

void LLFloaterBump::startCall()
{
    LLAvatarActions::startCall(mItemUUID);
}

bool LLFloaterBump::canCall()
{
    return LLAvatarActions::canCallTo(mItemUUID);
}

void LLFloaterBump::reportAbuse()
{
    LLFloaterReporter::showFromAvatar(mItemUUID, "av_name");
}

void LLFloaterBump::showProfile()
{
    LLAvatarActions::showProfile(mItemUUID);
}

void LLFloaterBump::addFriend()
{
    LLAvatarActions::requestFriendshipDialog(mItemUUID);
}

bool LLFloaterBump::enableAddFriend()
{
    return !LLAvatarActions::isFriend(mItemUUID);
}

void LLFloaterBump::muteAvatar()
{
    LLMute mute(mItemUUID, mNames[mItemUUID], LLMute::AGENT);
    if (LLMuteList::getInstance()->isMuted(mute.mID))
    {
        LLMuteList::getInstance()->remove(mute);
    }
    else
    {
        LLMuteList::getInstance()->add(mute);
        LLPanelBlockedList::showPanelAndSelect(mute.mID);
    }
}

void LLFloaterBump::payAvatar()
{
    LLAvatarActions::pay(mItemUUID);
}

void LLFloaterBump::zoomInAvatar()
{
    handle_zoom_to_object(mItemUUID);
}

bool LLFloaterBump::enableMute()
{
    return LLAvatarActions::canBlock(mItemUUID);
}

void LLFloaterBump::inviteToGroup()
{
    LLAvatarActions::inviteToGroup(mItemUUID);
}

LLFloaterBump* LLFloaterBump::getInstance()
{
    return LLFloaterReg::getTypedInstance<LLFloaterBump>("bumps");
}
