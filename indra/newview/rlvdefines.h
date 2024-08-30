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
