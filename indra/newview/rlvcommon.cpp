/**
 * @file rlvcommon.h
 * @author Kitty Barnett
 * @brief RLVa helper functions and constants used throughout the viewer
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

#include "llagent.h"
#include "llchat.h"
#include "lldbstrings.h"
#include "llversioninfo.h"
#include "llviewermenu.h"
#include "llviewerstats.h"
#include "message.h"
#include <boost/algorithm/string.hpp>

#include "rlvcommon.h"

#include "llviewercontrol.h"
#include "rlvhandler.h"

using namespace Rlv;

// ============================================================================
// RlvStrings
//

std::string Strings::getVersion(bool wants_legacy)
{
    return llformat("%s viewer v%d.%d.%d (RLVa %d.%d.%d)",
        !wants_legacy ? "RestrainedLove" : "RestrainedLife",
        SpecVersion::Major, SpecVersion::Minor, SpecVersion::Patch,
        ImplVersion::Major, ImplVersion::Minor, ImplVersion::Patch);
}

std::string Strings::getVersionAbout()
{
    return llformat("RLV v%d.%d.%d / RLVa v%d.%d.%d.%d",
        SpecVersion::Major, SpecVersion::Minor, SpecVersion::Patch,
        ImplVersion::Major, ImplVersion::Minor, ImplVersion::Patch, LLVersionInfo::instance().getBuild());
}

std::string Strings::getVersionNum()
{
    return llformat("%d%02d%02d%02d",
        SpecVersion::Major, SpecVersion::Minor, SpecVersion::Patch, SpecVersion::Build);
}

std::string Strings::getVersionImplNum()
{
    return llformat("%d%02d%02d%02d",
        ImplVersion::Major, ImplVersion::Minor, ImplVersion::Patch, ImplVersion::ImplId);
}

// ============================================================================
// RlvUtil
//

void Util::menuToggleVisible()
{
    bool isTopLevel = gSavedSettings.getBOOL(Settings::TopLevelMenu);
    bool isRlvEnabled = RlvHandler::isEnabled();

    LLMenuGL* menuRLVaMain = gMenuBarView->findChildMenuByName("RLVa Main", false);
    LLMenuGL* menuAdvanced = gMenuBarView->findChildMenuByName("Advanced", false);
    LLMenuGL* menuRLVaEmbed= menuAdvanced->findChildMenuByName("RLVa Embedded", false);

    gMenuBarView->setItemVisible("RLVa Main", isRlvEnabled && isTopLevel);
    menuAdvanced->setItemVisible("RLVa Embedded", isRlvEnabled && !isTopLevel);

    if ( isRlvEnabled && menuRLVaMain && menuRLVaEmbed &&
         ( (isTopLevel && 1 == menuRLVaMain->getItemCount()) || (!isTopLevel && 1 == menuRLVaEmbed->getItemCount())) )
    {
        LLMenuGL* menuFrom = isTopLevel ? menuRLVaEmbed : menuRLVaMain;
        LLMenuGL* menuTo = isTopLevel ? menuRLVaMain : menuRLVaEmbed;
        while (LLMenuItemGL* pItem = menuFrom->getItem(1))
        {
            menuFrom->removeChild(pItem);
            menuTo->addChild(pItem);
            pItem->updateBranchParent(menuTo);
        }
    }
}

bool Util::parseStringList(const std::string& strInput, std::vector<std::string>& optionList, std::string_view strSeparator)
{
    if (!strInput.empty())
        boost::split(optionList, strInput, boost::is_any_of(strSeparator));
    return !optionList.empty();
}

bool Util::sendChatReply(S32 nChannel, const std::string& strUTF8Text)
{
    if (!isValidReplyChannel(nChannel))
        return false;

    // Copy/paste from send_chat_from_viewer()
    gMessageSystem->newMessageFast(_PREHASH_ChatFromViewer);
    gMessageSystem->nextBlockFast(_PREHASH_AgentData);
    gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    gMessageSystem->nextBlockFast(_PREHASH_ChatData);
    gMessageSystem->addStringFast(_PREHASH_Message, utf8str_truncate(strUTF8Text, MAX_MSG_STR_LEN));
    gMessageSystem->addU8Fast(_PREHASH_Type, CHAT_TYPE_SHOUT);
    gMessageSystem->addS32("Channel", nChannel);
    gAgent.sendReliableMessage();
    add(LLStatViewer::CHAT_COUNT, 1);

    return true;
}

// ============================================================================
