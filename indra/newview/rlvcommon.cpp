#include "llviewerprecompiledheaders.h"
#include "llagent.h"
#include "llchat.h"
#include "lldbstrings.h"
#include "llversioninfo.h"
#include "llviewerstats.h"
#include "message.h"

#include "rlvdefines.h"
#include "rlvcommon.h"

#include <boost/algorithm/string.hpp>

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
