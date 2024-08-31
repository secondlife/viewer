#include "llviewerprecompiledheaders.h"

#include "lltrans.h"

#include "rlvhelper.h"

using namespace Rlv;

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
        ECmdRet resultBucket;

        // Successful and retained commands are added as-is
        if (isReturnCodeSuccess(eRet))
            resultBucket = ECmdRet::Success;
        else if (ECmdRet::Retained == eRet)
            resultBucket = ECmdRet::Retained;
        else
        {
            // Failed commands get the failure reason appended to help troubleshooting
            resultBucket = ECmdRet::Failed;
            strCmd.append(llformat(" (%s)", getReturnCodeString(eRet).c_str()));
        }

        std::string& strResult = mCommandResults[resultBucket];
        if (!strResult.empty())
            strResult.append(", ");
        strResult.append(strCmd);
    }

    std::string CommandDbgOut::get() const {
        std::ostringstream result;

        if (1 == mCommandResults.size())
        {
            auto it = mCommandResults.begin();
            result << " " << getDebugVerbFromReturnCode(it->first) << ": @" << it->second;;
        }
        else if (mCommandResults.size() > 1)
        {
            auto appendResult = [&](ECmdRet eRet, const std::string& name)
                {
                    auto it = mCommandResults.find(eRet);
                    if (it == mCommandResults.end()) return;
                    result << "\n    - " << LLTrans::getString(name) << ": @" << it->second;
                };
            result << ": @" << mOrigCmd;
            appendResult(ECmdRet::Success, "RlvDebugExecuted");
            appendResult(ECmdRet::Failed, "RlvDebugFailed");
            appendResult(ECmdRet::Retained, "RlvDebugRetained");
        }

        return result.str();
    }

    std::string CommandDbgOut::getDebugVerbFromReturnCode(ECmdRet eRet)
    {
        switch (eRet)
        {
            case ECmdRet::Success:
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
            case ECmdRet::Success:
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
