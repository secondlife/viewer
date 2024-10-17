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

#pragma once

#include "rlvdefines.h"

namespace Rlv
{
    // ============================================================================
    // RlvStrings
    //

    class Strings
    {
    public:
        static std::string        getVersion(bool wants_legacy);
        static std::string        getVersionAbout();
        static std::string        getVersionImplNum();
        static std::string        getVersionNum();
    };

    // ============================================================================
    // RlvUtil
    //

    namespace Util
    {
        bool isValidReplyChannel(S32 nChannel, bool isLoopback = false);
        void menuToggleVisible();
        bool parseStringList(const std::string& strInput, std::vector<std::string>& optionList, std::string_view strSeparator = Constants::OptionSeparator);
        bool sendChatReply(S32 nChannel, const std::string& strUTF8Text);
        bool sendChatReply(const std::string& strChannel, const std::string& strUTF8Text);
    };

    inline bool Util::isValidReplyChannel(S32 nChannel, bool isLoopback)
    {
        return (nChannel > (!isLoopback ? 0 : -1)) && (CHAT_CHANNEL_DEBUG != nChannel);
    }

    inline bool Util::sendChatReply(const std::string& strChannel, const std::string& strUTF8Text)
    {
        S32 nChannel;
        return LLStringUtil::convertToS32(strChannel, nChannel) ? sendChatReply(nChannel, strUTF8Text) : false;
    }

    // ============================================================================
}
