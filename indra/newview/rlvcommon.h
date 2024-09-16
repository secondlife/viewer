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
