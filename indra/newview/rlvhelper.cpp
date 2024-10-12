/**
 * @file rlvhelper.cpp
 * @author Kitty Barnett
 * @brief RLVa helper classes for internal use only
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

#include "lltrans.h"
#include "llviewercontrol.h"

#include "rlvhelper.h"

#include <boost/algorithm/string.hpp>

using namespace Rlv;

// ============================================================================
// BehaviourDictionary
//

BehaviourDictionary::BehaviourDictionary()
{
    //
    // Restrictions
    //

    //
    // Reply-only
    //
    addEntry(new ReplyProcessor<EBehaviour::GetCommand>("getcommand"));
    addEntry(new ReplyProcessor<EBehaviour::Version, VersionReplyHandler>("version"));
    addEntry(new ReplyProcessor<EBehaviour::VersionNew, VersionReplyHandler>("versionnew"));
    addEntry(new ReplyProcessor<EBehaviour::VersionNum>("versionnum"));

    // Populate mString2InfoMap (the tuple <behaviour, type> should be unique)
    for (const BehaviourInfo* bhvr_info_p : mBhvrInfoList)
    {
        mString2InfoMap.insert(std::make_pair(std::make_pair(bhvr_info_p->getBehaviour(), static_cast<EParamType>(bhvr_info_p->getParamTypeMask())), bhvr_info_p));
    }

    // Populate m_Bhvr2InfoMap (there can be multiple entries per ERlvBehaviour)
    for (const BehaviourInfo* bhvr_info_p : mBhvrInfoList)
    {
        if ((bhvr_info_p->getParamTypeMask() & to_underlying(EParamType::AddRem)) && !bhvr_info_p->isSynonym())
        {
#ifdef RLV_DEBUG
            for (const auto& itBhvr : boost::make_iterator_range(mBhvr2InfoMap.lower_bound(bhvr_info_p->getBehaviourType()), mBhvr2InfoMap.upper_bound(bhvr_info_p->getBehaviourType())))
            {
                RLV_ASSERT((itBhvr.first != bhvr_info_p->getBehaviourType()) || (itBhvr.second->getBehaviourFlags() != bhvr_info_p->getBehaviourFlags()));
            }
#endif // RLV_DEBUG
            mBhvr2InfoMap.insert(std::pair(bhvr_info_p->getBehaviourType(), bhvr_info_p));
        }
    }
}

BehaviourDictionary::~BehaviourDictionary()
{
    for (const BehaviourInfo* bhvr_info_p : mBhvrInfoList)
    {
        delete bhvr_info_p;
    }
    mBhvrInfoList.clear();
}

void BehaviourDictionary::addEntry(const BehaviourInfo* entry_p)
{
    // Filter experimental commands (if disabled)
    static LLCachedControl<bool> sEnableExperimental(gSavedSettings, Settings::EnableExperimentalCommands);
    if (!entry_p || (!sEnableExperimental && entry_p->isExperimental()))
    {
        return;
    }

    // Sanity check for duplicate entries
#if LL_RELEASE_WITH_DEBUG_INFO || LL_DEBUG
    std::for_each(mBhvrInfoList.begin(), mBhvrInfoList.end(),
        [&entry_p](const BehaviourInfo* bhvr_info_p) {
            RLV_ASSERT_DBG((bhvr_info_p->getBehaviour() != entry_p->getBehaviour()) || ((bhvr_info_p->getParamTypeMask() & entry_p->getParamTypeMask()) == 0));
        });
#endif // LL_RELEASE_WITH_DEBUG_INFO || LL_DEBUG

    mBhvrInfoList.push_back(entry_p);
}

const BehaviourInfo* BehaviourDictionary::getBehaviourInfo(EBehaviour eBhvr, EParamType eParamType) const
{
    const BehaviourInfo* bhvr_info_p = nullptr;
    for (auto itBhvrLower = mBhvr2InfoMap.lower_bound(eBhvr), itBhvrUpper = mBhvr2InfoMap.upper_bound(eBhvr);
        std::find_if(itBhvrLower, itBhvrUpper, [eParamType](const auto& bhvrEntry) { return bhvrEntry.second->getParamTypeMask() == to_underlying(eParamType); }) != itBhvrUpper;
        ++itBhvrLower)
    {
        if (bhvr_info_p)
            return nullptr;
        bhvr_info_p = itBhvrLower->second;
    }
    return bhvr_info_p;
}

const BehaviourInfo* BehaviourDictionary::getBehaviourInfo(const std::string& strBhvr, EParamType eParamType, bool* is_strict_p) const
{
    size_t idxBhvrLastPart = strBhvr.find_last_of('_');
    std::string strBhvrLastPart((std::string::npos != idxBhvrLastPart) && (idxBhvrLastPart < strBhvr.size()) ? strBhvr.substr(idxBhvrLastPart + 1) : LLStringUtil::null);

    bool isStrict = (strBhvrLastPart.compare("sec") == 0);
    if (is_strict_p)
        *is_strict_p = isStrict;

    auto itBhvr = mString2InfoMap.find(std::make_pair((!isStrict) ? strBhvr : strBhvr.substr(0, strBhvr.size() - 4), (has_flag(eParamType, EParamType::AddRem)) ? EParamType::AddRem : eParamType));
    if ((mString2InfoMap.end() == itBhvr) && (!isStrict) && (!strBhvrLastPart.empty()) && (EParamType::Force == eParamType))
    {
        // No match found but it could still be a local scope modifier
        auto itBhvrMod = mString2InfoMap.find(std::make_pair(strBhvr.substr(0, idxBhvrLastPart), EParamType::AddRem));
    }

    return ((itBhvr != mString2InfoMap.end()) && ((!isStrict) || (itBhvr->second->hasStrict()))) ? itBhvr->second : nullptr;
}

EBehaviour BehaviourDictionary::getBehaviourFromString(const std::string& strBhvr, EParamType eParamType, bool* pisStrict) const
{
    const BehaviourInfo* bhvr_info_p = getBehaviourInfo(strBhvr, eParamType, pisStrict);
    // Filter out locally scoped modifier commands since they don't actually have a unique behaviour value of their own
    return bhvr_info_p->getBehaviourType();
}

bool BehaviourDictionary::getCommands(const std::string& strMatch, EParamType eParamType, std::list<std::string>& cmdList) const
{
    cmdList.clear();
    for (const BehaviourInfo* bhvr_info_p : mBhvrInfoList)
    {
        if ((bhvr_info_p->getParamTypeMask() & to_underlying(eParamType)) || (EParamType::Unknown == eParamType))
        {
            std::string strCmd = bhvr_info_p->getBehaviour();
            if ((std::string::npos != strCmd.find(strMatch)) || (strMatch.empty()))
                cmdList.push_back(strCmd);
            if ((bhvr_info_p->hasStrict()) && ((std::string::npos != strCmd.append("_sec").find(strMatch)) || (strMatch.empty())))
                cmdList.push_back(strCmd);
        }
    }
    return !cmdList.empty();
}

bool BehaviourDictionary::getHasStrict(EBehaviour eBhvr) const
{
    for (const auto& itBhvr : boost::make_iterator_range(mBhvr2InfoMap.lower_bound(eBhvr), mBhvr2InfoMap.upper_bound(eBhvr)))
    {
        // Only restrictions can be strict
        if (to_underlying(EParamType::AddRem) != itBhvr.second->getParamTypeMask())
            continue;
        return itBhvr.second->hasStrict();
    }
    RLV_ASSERT(false);
    return false;
}

void BehaviourDictionary::toggleBehaviourFlag(const std::string& strBhvr, EParamType eParamType, BehaviourInfo::EBehaviourFlags eBhvrFlag, bool fEnable)
{
    auto itBhvr = mString2InfoMap.find(std::make_pair(strBhvr, (has_flag(eParamType, EParamType::AddRem)) ? EParamType::AddRem : eParamType));
    if (mString2InfoMap.end() != itBhvr)
    {
        const_cast<BehaviourInfo*>(itBhvr->second)->toggleBehaviourFlag(eBhvrFlag, fEnable);
    }
}

// ============================================================================
// RlvCommmand
//

RlvCommand::RlvCommand(const LLUUID& idObj, const std::string& strCmd)
    : mObjId(idObj)
{
    if (parseCommand(strCmd, mBehaviour, mOption, mParam))
    {
         if ("n" == mParam || "add" == mParam)
             mParamType = EParamType::Add;
         else if ("y" == mParam || "rem" == mParam)
             mParamType = EParamType::Remove;
         else if ("clear" == mBehaviour)                                     // clear is the odd one out so just make it its own type
             mParamType = EParamType::Clear;
         else if ("force" == mParam)
             mParamType = EParamType::Force;
         else if (S32 nTemp; LLStringUtil::convertToS32(mParam, nTemp))  // Assume it's a reply command if we can convert <param> to an S32
            mParamType = EParamType::Reply;
    }

    mIsValid = mParamType != EParamType::Unknown;
    if (!mIsValid)
    {
        mOption.clear();
        mParam.clear();
        return;
    }

    mBhvrInfo = BehaviourDictionary::instance().getBehaviourInfo(mBehaviour, mParamType, &mIsStrict);
}

RlvCommand::RlvCommand(const RlvCommand& rlvCmd, EParamType eParamType)
    : mIsValid(rlvCmd.mIsValid), mObjId(rlvCmd.mObjId), mBehaviour(rlvCmd.mBehaviour), mBhvrInfo(rlvCmd.mBhvrInfo)
    , mParamType( (EParamType::Unknown == eParamType) ? rlvCmd.mParamType : eParamType)
    , mIsStrict(rlvCmd.mIsStrict), mOption(rlvCmd.mOption), mParam(rlvCmd.mParam), mIsRefCounted(rlvCmd.mIsRefCounted)
{
}

bool RlvCommand::parseCommand(const std::string& strCmd, std::string& strBhvr, std::string& strOption, std::string& strParam)
{
    // Format: <behaviour>[:<option>]=<param>
    const size_t idxOption = strCmd.find(':');
    const size_t idxParam  = strCmd.find('=');

    // If <behaviour> is missing it's always an improperly formatted command
    // If there's an option, but it comes after <param> it's also invalid
    if ( (idxOption == 0 || idxParam == 0) ||
         (idxOption != std::string::npos && idxOption >= idxParam) )
    {
        return false;
    }

    strBhvr = strCmd.substr(0, std::string::npos != idxOption ? idxOption : idxParam);
    strOption = strParam = "";

    // If <param> is missing it's an improperly formatted command
    if (idxParam == std::string::npos || idxParam + 1 == strCmd.length())
    {
        // Unless "<behaviour> == "clear" AND (idxOption == 0)"
        // OR <behaviour> == "clear" AND (idxParam != 0)
        if (strBhvr == "clear" && (!idxOption || idxParam))
            return true;
        return false;
    }

    if (idxOption != std::string::npos && idxOption + 1 != idxParam)
        strOption = strCmd.substr(idxOption + 1, idxParam - idxOption - 1);
    strParam = strCmd.substr(idxParam + 1);

    return true;
}

std::string RlvCommand::asString() const
{
    // NOTE: @clear=<param> should be represented as clear:<param>
    return mParamType != EParamType::Clear
        ? getBehaviour() + (!mOption.empty() ? ":" + mOption : "")
        : getBehaviour() + (!mParam.empty() ? ":" + mParam : "");
}

// =========================================================================
// Various helper classes/timers/functors
//

namespace Rlv
{
    // ===========================================================================
    // CommandDbgOut
    //

    void CommandDbgOut::add(std::string strCmd, ECmdRet eRet)
    {
        const std::string strSuffix = getReturnCodeString(eRet);
        if (!strSuffix.empty())
            strCmd.append(llformat(" (%s)", strSuffix.c_str()));
        else if (mForConsole)
            return; // Only show console feedback on successful commands when there's an informational notice

        std::string& strResult = mCommandResults[isReturnCodeSuccess(eRet) ? ECmdRet::Succeeded : (ECmdRet::Retained == eRet ? ECmdRet::Retained : ECmdRet::Failed)];
        if (!strResult.empty())
            strResult.append(", ");
        strResult.append(strCmd);
    }

    std::string CommandDbgOut::get() const {
        std::ostringstream result;

        if (1 == mCommandResults.size() && !mForConsole)
        {
            auto it = mCommandResults.begin();
            result << " " << getDebugVerbFromReturnCode(it->first) << ": @" << it->second;
        }
        else if (!mCommandResults.empty())
        {
            auto appendResult = [&](ECmdRet eRet, const std::string& name)
                {
                    auto it = mCommandResults.find(eRet);
                    if (it == mCommandResults.end()) return;
                    if (!mForConsole) result << "\n    - ";
                    result << LLTrans::getString(name) << ": @" << it->second;
                };
            if (!mForConsole)
                result << ": @" << mOrigCmd;
            appendResult(ECmdRet::Succeeded, !mForConsole ? "RlvDebugExecuted" : "RlvConsoleExecuted");
            appendResult(ECmdRet::Failed, !mForConsole ? "RlvDebugFailed" : "RlvConsoleFailed");
            appendResult(ECmdRet::Retained, !mForConsole ? "RlvDebugRetained" : "RlvConsoleRetained");
        }

        return result.str();
    }

    std::string CommandDbgOut::getDebugVerbFromReturnCode(ECmdRet eRet)
    {
        switch (eRet)
        {
            case ECmdRet::Succeeded:
                return LLTrans::getString("RlvDebugExecuted");
            case ECmdRet::Failed:
                return LLTrans::getString("RlvDebugFailed");
            case ECmdRet::Retained:
                return LLTrans::getString("RlvDebugRetained");
            default:
                RLV_ASSERT(false);
                return LLStringUtil::null;
        }
    }

    std::string CommandDbgOut::getReturnCodeString(ECmdRet eRet)
    {
        switch (eRet)
        {
            case ECmdRet::SuccessUnset:
                return LLTrans::getString("RlvReturnCodeUnset");
            case ECmdRet::SuccessDuplicate:
                return LLTrans::getString("RlvReturnCodeDuplicate");
            case ECmdRet::SuccessDelayed:
                return LLTrans::getString("RlvReturnCodeDelayed");
            case ECmdRet::SuccessDeprecated:
                return LLTrans::getString("RlvReturnCodeDeprecated");
            case ECmdRet::FailedSyntax:
                return LLTrans::getString("RlvReturnCodeSyntax");
            case ECmdRet::FailedOption:
                return LLTrans::getString("RlvReturnCodeOption");
            case ECmdRet::FailedParam:
                return LLTrans::getString("RlvReturnCodeParam");
            case ECmdRet::FailedLock:
                return LLTrans::getString("RlvReturnCodeLock");
            case ECmdRet::FailedDisabled:
                return LLTrans::getString("RlvReturnCodeDisabled");
            case ECmdRet::FailedUnknown:
                return LLTrans::getString("RlvReturnCodeUnknown");
            case ECmdRet::FailedNoSharedRoot:
                return LLTrans::getString("RlvReturnCodeNoSharedRoot");
            case ECmdRet::FailedDeprecated:
                return LLTrans::getString("RlvReturnCodeDeprecatedAndDisabled");
            case ECmdRet::FailedNoBehaviour:
                return LLTrans::getString("RlvReturnCodeNoBehaviour");
            case ECmdRet::FailedUnheldBehaviour:
                return LLTrans::getString("RlvReturnCodeUnheldBehaviour");
            case ECmdRet::FailedBlocked:
                return LLTrans::getString("RlvReturnCodeBlocked");
            case ECmdRet::FailedThrottled:
                return LLTrans::getString("RlvReturnCodeThrottled");
            case ECmdRet::FailedNoProcessor:
                return LLTrans::getString("RlvReturnCodeNoProcessor");
            // The following are identified by the chat verb
            case ECmdRet::Retained:
            case ECmdRet::Succeeded:
            case ECmdRet::Failed:
                return LLStringUtil::null;
            // The following shouldn't occur
            case ECmdRet::Unknown:
            default:
                RLV_ASSERT(false);
                return LLStringUtil::null;
        }
    }

    // ===========================================================================
}

// ============================================================================
