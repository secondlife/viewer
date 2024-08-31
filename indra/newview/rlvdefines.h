#pragma once

// ============================================================================
// Defines
//

namespace Rlv
{
    // Version of the specification we report
    struct SpecVersion {
        static constexpr S32 Major = 4;
        static constexpr S32 Minor = 0;
        static constexpr S32 Patch = 0;
        static constexpr S32 Build = 0;
    };

    // RLVa implementation version
    struct ImplVersion {
        static constexpr S32 Major = 3;
        static constexpr S32 Minor = 0;
        static constexpr S32 Patch = 0;
        static constexpr S32 ImplId = 2; /* Official viewer */
    };
}

// Defining these makes it easier if we ever need to change our tag
#define RLV_WARNS		LL_WARNS("RLV")
#define RLV_INFOS		LL_INFOS("RLV")
#define RLV_DEBUGS		LL_DEBUGS("RLV")
#define RLV_ENDL		LL_ENDL

#define RLV_RELEASE
#if LL_RELEASE_WITH_DEBUG_INFO || LL_DEBUG
    // Make sure we halt execution on errors
    #define RLV_ERRS				LL_ERRS("RLV")
    // Keep our asserts separate from LL's
    #define RLV_ASSERT(f)			if (!(f)) { RLV_ERRS << "ASSERT (" << #f << ")" << RLV_ENDL; }
    #define RLV_ASSERT_DBG(f)		RLV_ASSERT(f)
#else
    // We don't want to check assertions in release builds
    #ifndef RLV_RELEASE
        #define RLV_ASSERT(f)		RLV_VERIFY(f)
        #define RLV_ASSERT_DBG(f)
    #else
        #define RLV_ASSERT(f)
        #define RLV_ASSERT_DBG(f)
    #endif // RLV_RELEASE
#endif // LL_RELEASE_WITH_DEBUG_INFO || LL_DEBUG

namespace Rlv
{
    namespace Constants
    {
        constexpr char CmdPrefix = '@';
    }
}

// ============================================================================
// Enumeration declarations
//

namespace Rlv
{
    enum class ECmdRet{
        Unknown           = 0x0000,            // Unknown error (should only be used internally)
        Retained,                              // Command was retained
        Success           = 0x0100,            // Command executed successfully
        SuccessUnset,                          // Command executed successfully (RLV_TYPE_REMOVE for an unrestricted behaviour)
        SuccessDuplicate,                      // Command executed successfully (RLV_TYPE_ADD for an already restricted behaviour)
        SuccessDeprecated,                     // Command executed successfully but has been marked as deprecated
        SuccessDelayed,                        // Command parsed valid but will execute at a later time
        Failed            = 0x0200,            // Command failed (general failure)
        FailedSyntax,                          // Command failed (syntax error)
        FailedOption,                          // Command failed (invalid option)
        FailedParam,                           // Command failed (invalid param)
        FailedLock,                            // Command failed (command is locked by another object)
        FailedDisabled,                        // Command failed (command disabled by user)
        FailedUnknown,                         // Command failed (unknown command)
        FailedNoSharedRoot,                    // Command failed (missing #RLV)
        FailedDeprecated,                      // Command failed (deprecated and no longer supported)
        FailedNoBehaviour,                     // Command failed (force modifier on an object with no active restrictions)
        FailedUnheldBehaviour,                 // Command failed (local modifier on an object that doesn't hold the base behaviour)
        FailedBlocked,                         // Command failed (object is blocked)
        FailedThrottled,                       // Command failed (throttled)
        FailedNoProcessor                      // Command doesn't have a template processor define (legacy code)
    };

    constexpr bool isReturnCodeSuccess(ECmdRet eRet)
    {
        return (static_cast<uint16_t>(eRet) & static_cast<uint16_t>(ECmdRet::Success)) == static_cast<uint16_t>(ECmdRet::Success);
    }
}

// ============================================================================
// Settings
//

namespace Rlv
{
    namespace Settings
    {
        constexpr char Main[] = "RestrainedLove";
        constexpr char Debug[] = "RestrainedLoveDebug";

        constexpr char DebugHideUnsetDup[] = "RLVaDebugHideUnsetDuplicate";
        constexpr char EnableIMQuery[] = "RLVaEnableIMQuery";
        constexpr char EnableTempAttach[] = "RLVaEnableTemporaryAttachments";
        constexpr char TopLevelMenu[] = "RLVaTopLevelMenu";
    };

};

// ============================================================================
