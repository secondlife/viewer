/**
 * @file rlvdefines.h
 * @author Kitty Barnett
 * @brief RLVa common defines, constants and enums
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

// ============================================================================
// Defines
//

// Defining these makes it easier if we ever need to change our tag
#define RLV_WARNS       LL_WARNS("RLV")
#define RLV_INFOS       LL_INFOS("RLV")
#define RLV_DEBUGS      LL_DEBUGS("RLV")
#define RLV_ENDL        LL_ENDL
#define RLV_VERIFY(f)   (f)

#if LL_RELEASE_WITH_DEBUG_INFO || LL_DEBUG
    // Make sure we halt execution on errors
    #define RLV_ERRS                LL_ERRS("RLV")
    // Keep our asserts separate from LL's
    #define RLV_ASSERT(f)           if (!(f)) { RLV_ERRS << "ASSERT (" << #f << ")" << RLV_ENDL; }
    #define RLV_ASSERT_DBG(f)       RLV_ASSERT(f)
#else
    // Don't halt execution on errors in release
    #define RLV_ERRS                LL_WARNS("RLV")
    // We don't want to check assertions in release builds
    #ifdef RLV_DEBUG
        #define RLV_ASSERT(f)       RLV_VERIFY(f)
        #define RLV_ASSERT_DBG(f)
    #else
        #define RLV_ASSERT(f)
        #define RLV_ASSERT_DBG(f)
    #endif // RLV_DEBUG
#endif // LL_RELEASE_WITH_DEBUG_INFO || LL_DEBUG

namespace Rlv
{
    // Version of the specification we report
    namespace SpecVersion {
        constexpr S32 Major = 4;
        constexpr S32 Minor = 0;
        constexpr S32 Patch = 0;
        constexpr S32 Build = 0;
    };

    // RLVa implementation version
    namespace ImplVersion {
        constexpr S32 Major = 3;
        constexpr S32 Minor = 0;
        constexpr S32 Patch = 0;
        constexpr S32 ImplId = 13;
    };

    namespace Constants
    {
        constexpr char CmdPrefix = '@';
        constexpr char ConsolePrompt[] = "> ";
        constexpr std::string_view OptionSeparator = ";";
    }
}

// ============================================================================
// Enumeration declarations
//

namespace Rlv
{
    enum class EBehaviour {
        Version = 0,
        VersionNew,
        VersionNum,
        GetCommand,

        Count,
        Unknown,
    };

    enum class EBehaviourOptionType
    {
        EmptyOrException,                   // Behaviour takes no parameters
        Exception,                          // Behaviour requires an exception as a parameter
        NoneOrException,                    // Behaviour takes either no parameters or an exception
    };

    enum class EParamType {
        Unknown = 0x00,
        Add     = 0x01,                 // <param> == "n"|"add"
        Remove  = 0x02,                 // <param> == "y"|"rem"
        Force   = 0x04,                 // <param> == "force"
        Reply   = 0x08,                 // <param> == <number>
        Clear   = 0x10,
        AddRem  = Add | Remove
    };

    enum class ECmdRet {
        Unknown = 0x0000,               // Unknown error (should only be used internally)
        Retained,                       // Command was retained
        Succeeded = 0x0100,             // Command executed successfully
        SuccessUnset,                   // Command executed successfully (RLV_TYPE_REMOVE for an unrestricted behaviour)
        SuccessDuplicate,               // Command executed successfully (RLV_TYPE_ADD for an already restricted behaviour)
        SuccessDeprecated,              // Command executed successfully but has been marked as deprecated
        SuccessDelayed,                 // Command parsed valid but will execute at a later time
        Failed = 0x0200,                // Command failed (general failure)
        FailedSyntax,                   // Command failed (syntax error)
        FailedOption,                   // Command failed (invalid option)
        FailedParam,                    // Command failed (invalid param)
        FailedLock,                     // Command failed (command is locked by another object)
        FailedDisabled,                 // Command failed (command disabled by user)
        FailedUnknown,                  // Command failed (unknown command)
        FailedNoSharedRoot,             // Command failed (missing #RLV)
        FailedDeprecated,               // Command failed (deprecated and no longer supported)
        FailedNoBehaviour,              // Command failed (force modifier on an object with no active restrictions)
        FailedUnheldBehaviour,          // Command failed (local modifier on an object that doesn't hold the base behaviour)
        FailedBlocked,                  // Command failed (object is blocked)
        FailedThrottled,                // Command failed (throttled)
        FailedNoProcessor               // Command doesn't have a template processor define (legacy code)
    };

    enum class EExceptionCheck
    {
        Permissive,                     // Exception can be set by any object
        Strict,                         // Exception must be set by all objects holding the restriction
        Default,                        // Permissive or strict will be determined by currently enforced restrictions
    };

    // Replace&remove in c++23
    template <typename E>
    constexpr std::enable_if_t<std::is_enum_v<E> && !std::is_convertible_v<E, int>, std::underlying_type_t<E>> to_underlying(E e) noexcept
    {
        return static_cast<std::underlying_type_t<E>>(e);
    }

    template <typename E>
    constexpr std::enable_if_t<std::is_enum_v<E> && !std::is_convertible_v<E, int>, bool> has_flag(E value, E flag) noexcept
    {
        return (to_underlying(value) & to_underlying(flag)) != 0;
    }

    constexpr bool isReturnCodeSuccess(ECmdRet eRet)
    {
        return (to_underlying(eRet) & to_underlying(ECmdRet::Succeeded)) == to_underlying(ECmdRet::Succeeded);
    }

    constexpr bool isReturnCodeFailed(ECmdRet eRet)
    {
        return (to_underlying(eRet) & to_underlying(ECmdRet::Failed)) == to_underlying(ECmdRet::Failed);
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
        constexpr char EnableExperimentalCommands[] = "RLVaExperimentalCommands";
        constexpr char EnableIMQuery[] = "RLVaEnableIMQuery";
        constexpr char EnableTempAttach[] = "RLVaEnableTemporaryAttachments";
        constexpr char TopLevelMenu[] = "RLVaTopLevelMenu";
    };

};

// ============================================================================
