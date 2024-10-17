/**
 * @file rlvhelper.h
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

#pragma once

#include "rlvdefines.h"

// ============================================================================
// Forward declarations
//

class RlvCommand;

// ============================================================================

namespace Rlv
{
    // ============================================================================
    // BehaviourInfo class - Generic behaviour descriptor (used by restrictions, reply and force commands)
    //

    class BehaviourInfo
    {
    public:
        enum EBehaviourFlags : uint32_t
        {
            // General behaviour flags
            Strict = 0x0001,                // Behaviour has a "_sec" version
            Synonym = 0x0002,               // Behaviour is a synonym of another
            Extended = 0x0004,              // Behaviour is part of the RLVa extended command set
            Experimental = 0x0008,          // Behaviour is part of the RLVa experimental command set
            Blocked = 0x0010,               // Behaviour is blocked
            Deprecated = 0x0020,            // Behaviour is deprecated
            MaskGeneral = 0x0FFF,

            // Force-wear specific flags
            ForceWear_WearReplace = 0x0001 << 16,
            ForceWear_WearAdd = 0x0002 << 16,
            ForceWear_WearRemove = 0x0004 << 16,
            ForceWear_Node = 0x0010 << 16,
            ForceWear_Subtree = 0x0020 << 16,
            ForceWear_ContextNone = 0x0100 << 16,
            ForceWear_ContextObject = 0x0200 << 16,
            MaskForceWear = 0xFFFFu << 16
        };

        BehaviourInfo(const std::string& strBhvr, EBehaviour eBhvr, EParamType maskParamType, std::underlying_type_t<EBehaviourFlags> nBhvrFlags = 0)
            : mBhvr(strBhvr), mBhvrType(eBhvr), mBhvrFlags(nBhvrFlags), mMaskParamType(to_underlying(maskParamType)) {}
        virtual ~BehaviourInfo() {}

        const std::string&    getBehaviour() const { return mBhvr; }
        EBehaviour            getBehaviourType() const { return mBhvrType; }
        std::underlying_type_t<EBehaviourFlags> getBehaviourFlags() const { return mBhvrFlags; }
        std::underlying_type_t<EParamType> getParamTypeMask() const { return mMaskParamType; }
        bool                  hasStrict() const { return mBhvrFlags & Strict; }
        bool                  isBlocked() const { return mBhvrFlags & Blocked; }
        bool                  isExperimental() const { return mBhvrFlags & Experimental; }
        bool                  isExtended() const { return mBhvrFlags & Extended; }
        bool                  isSynonym() const { return mBhvrFlags & Synonym; }
        void                  toggleBehaviourFlag(EBehaviourFlags eBhvrFlag, bool fEnable);

        virtual ECmdRet  processCommand(const RlvCommand& rlvCmd) const { return ECmdRet::FailedNoProcessor; }

    protected:
        std::string mBhvr;
        EBehaviour mBhvrType;
        std::underlying_type_t<EBehaviourFlags> mBhvrFlags;
        std::underlying_type_t<EParamType> mMaskParamType;
    };

    inline void BehaviourInfo::toggleBehaviourFlag(EBehaviourFlags eBhvrFlag, bool fEnable)
    {
        if (fEnable)
            mBhvrFlags |= eBhvrFlag;
        else
            mBhvrFlags &= ~eBhvrFlag;
    }

    // ============================================================================
    // BehaviourDictionary and related classes
    //

    class BehaviourDictionary : public LLSingleton<BehaviourDictionary>
    {
        LLSINGLETON(BehaviourDictionary);
    protected:
        ~BehaviourDictionary() override;
    public:
        void addEntry(const BehaviourInfo* entry_p);

        /*
         * General helper functions
         */
    public:
        EBehaviour           getBehaviourFromString(const std::string& strBhvr, EParamType eParamType, bool* is_strict_p = nullptr) const;
        const BehaviourInfo* getBehaviourInfo(EBehaviour eBhvr, EParamType eParamType) const;
        const BehaviourInfo* getBehaviourInfo(const std::string& strBhvr, EParamType eParamType, bool* is_strict_p = nullptr) const;
        bool                 getCommands(const std::string& strMatch, EParamType eParamType, std::list<std::string>& cmdList) const;
        bool                 getHasStrict(EBehaviour eBhvr) const;
        void                 toggleBehaviourFlag(const std::string& strBhvr, EParamType eParamType, BehaviourInfo::EBehaviourFlags eBvhrFlag, bool fEnable);

        /*
         * Member variables
         */
    protected:
        std::list<const BehaviourInfo*> mBhvrInfoList;
        std::map<std::pair<std::string, EParamType>, const BehaviourInfo*> mString2InfoMap;
        std::multimap<EBehaviour, const BehaviourInfo*> mBhvr2InfoMap;
    };

    // ============================================================================
    // CommandHandler and related classes
    //

    typedef ECmdRet(BhvrHandlerFunc)(const RlvCommand&, bool&);
    typedef void(BhvrToggleHandlerFunc)(EBehaviour, bool);
    typedef ECmdRet(ForceHandlerFunc)(const RlvCommand&);
    typedef ECmdRet(ReplyHandlerFunc)(const RlvCommand&, std::string&);

    //
    // CommandHandlerBaseImpl - Base implementation for each command type (the old process(AddRem|Force|Reply)Command functions)
    //
    template<EParamType paramType> struct CommandHandlerBaseImpl;
    template<> struct CommandHandlerBaseImpl<EParamType::AddRem> { static ECmdRet processCommand(const RlvCommand&, BhvrHandlerFunc*, BhvrToggleHandlerFunc* = nullptr); };
    template<> struct CommandHandlerBaseImpl<EParamType::Force>  { static ECmdRet processCommand(const RlvCommand&, ForceHandlerFunc*); };
    template<> struct CommandHandlerBaseImpl<EParamType::Reply>  { static ECmdRet processCommand(const RlvCommand&, ReplyHandlerFunc*); };

    //
    // CommandHandler - The actual command handler (Note that a handler is more general than a processor; a handler can - for instance - be used by multiple processors)
    //
    #if LL_WINDOWS
        #define RLV_TEMPL_FIX(x) template<x>
    #else
        #define RLV_TEMPL_FIX(x) template<typename Placeholder = int>
    #endif // LL_WINDOWS


    template <EParamType templParamType, EBehaviour templBhvr>
    struct CommandHandler
    {
        RLV_TEMPL_FIX(typename = typename std::enable_if<templParamType == EParamType::AddRem>::type) static ECmdRet onCommand(const RlvCommand&, bool&);
        RLV_TEMPL_FIX(typename = typename std::enable_if<templParamType == EParamType::AddRem>::type) static void onCommandToggle(EBehaviour, bool);
        RLV_TEMPL_FIX(typename = typename std::enable_if<templParamType == EParamType::Force>::type)  static ECmdRet onCommand(const RlvCommand&);
        RLV_TEMPL_FIX(typename = typename std::enable_if<templParamType == EParamType::Reply>::type)  static ECmdRet onCommand(const RlvCommand&, std::string&);
    };

    // Aliases to improve readability in definitions
    template<EBehaviour templBhvr> using BehaviourHandler = CommandHandler<EParamType::AddRem, templBhvr>;
    template<EBehaviour templBhvr> using BehaviourToggleHandler = BehaviourHandler<templBhvr>;
    template<EBehaviour templBhvr> using ForceHandler = CommandHandler<EParamType::Force, templBhvr>;
    template<EBehaviour templBhvr> using ReplyHandler = CommandHandler<EParamType::Reply, templBhvr>;

    // List of shared handlers
    using VersionReplyHandler = ReplyHandler<EBehaviour::Version>;              // Shared between @version and @versionnew

    //
    // CommandProcessor - Templated glue class that brings BehaviourInfo, CommandHandlerBaseImpl and CommandHandler together
    //
    template <EParamType templParamType, EBehaviour templBhvr, typename handlerImpl = CommandHandler<templParamType, templBhvr>, typename baseImpl = CommandHandlerBaseImpl<templParamType>>
    class CommandProcessor : public BehaviourInfo
    {
    public:
        // Default constructor used by behaviour specializations
        RLV_TEMPL_FIX(typename = typename std::enable_if<templBhvr != EBehaviour::Unknown>::type)
        CommandProcessor(const std::string& strBhvr, U32 nBhvrFlags = 0) : BehaviourInfo(strBhvr, templBhvr, templParamType, nBhvrFlags) {}

        // Constructor used when we don't want to specialize on behaviour (see BehaviourGenericProcessor)
        RLV_TEMPL_FIX(typename = typename std::enable_if<templBhvr == EBehaviour::Unknown>::type)
        CommandProcessor(const std::string& strBhvr, EBehaviour eBhvr, U32 nBhvrFlags = 0) : BehaviourInfo(strBhvr, eBhvr, templParamType, nBhvrFlags) {}

        ECmdRet processCommand(const RlvCommand& rlvCmd) const override { return baseImpl::processCommand(rlvCmd, &handlerImpl::onCommand); }
    };

    // Aliases to improve readability in definitions
    template<EBehaviour templBhvr, typename handlerImpl = CommandHandler<EParamType::AddRem, templBhvr>> using BehaviourProcessor = CommandProcessor<EParamType::AddRem, templBhvr, handlerImpl>;
    template<EBehaviour templBhvr, typename handlerImpl = CommandHandler<EParamType::Force, templBhvr>> using ForceProcessor = CommandProcessor<EParamType::Force, templBhvr, handlerImpl>;
    template<EBehaviour templBhvr, typename handlerImpl = CommandHandler<EParamType::Reply, templBhvr>> using ReplyProcessor = CommandProcessor<EParamType::Reply, templBhvr, handlerImpl>;

    // Provides pre-defined generic implementations of basic behaviours (template voodoo - see original commit for something that still made sense)
    template<EBehaviourOptionType templOptionType> struct BehaviourGenericHandler { static ECmdRet onCommand(const RlvCommand& rlvCmd, bool& fRefCount); };
    template<EBehaviourOptionType templOptionType> using BehaviourGenericProcessor = BehaviourProcessor<EBehaviour::Unknown, BehaviourGenericHandler<templOptionType>>;
    template<EBehaviourOptionType templOptionType> struct ForceGenericHandler { static ECmdRet onCommand(const RlvCommand& rlvCmd); };
    template<EBehaviourOptionType templOptionType> using ForceGenericProcessor = ForceProcessor<EBehaviour::Unknown, ForceGenericHandler<templOptionType>>;

    // ============================================================================
    // BehaviourProcessor and related classes - Handles add/rem comamnds aka "restrictions)
    //

    template <EBehaviour eBhvr, typename handlerImpl = BehaviourHandler<eBhvr>, typename toggleHandlerImpl = BehaviourToggleHandler<eBhvr>>
    class BehaviourToggleProcessor : public BehaviourInfo
    {
    public:
        BehaviourToggleProcessor(const std::string& strBhvr, U32 nBhvrFlags = 0) : BehaviourInfo(strBhvr, eBhvr, EParamType::AddRem, nBhvrFlags) {}
        ECmdRet processCommand(const RlvCommand& rlvCmd) const override { return CommandHandlerBaseImpl<EParamType::AddRem>::processCommand(rlvCmd, &handlerImpl::onCommand, &toggleHandlerImpl::onCommandToggle); }
    };
    template <EBehaviour eBhvr, EBehaviourOptionType optionType, typename toggleHandlerImpl = BehaviourToggleHandler<eBhvr>> using RlvBehaviourGenericToggleProcessor = BehaviourToggleProcessor<eBhvr, BehaviourGenericHandler<optionType>, toggleHandlerImpl>;

    // ============================================================================
    // Various helper classes/timers/functors
    //

    struct CommandDbgOut
    {
        CommandDbgOut(const std::string& orig_cmd, bool for_console) : mOrigCmd(orig_cmd), mForConsole(for_console) {}
        void add(std::string strCmd, ECmdRet eRet);
        std::string get() const;
        static std::string getDebugVerbFromReturnCode(ECmdRet eRet);
        static std::string getReturnCodeString(ECmdRet eRet);
    private:
        std::string mOrigCmd;
        std::map<ECmdRet, std::string> mCommandResults;
        bool mForConsole = false;
    };
}

// ============================================================================
// RlvCommand
//

class RlvCommand
{
public:
    explicit RlvCommand(const LLUUID& idObj, const std::string& strCmd);
    RlvCommand(const RlvCommand& rlvCmd, Rlv::EParamType eParamType = Rlv::EParamType::Unknown);

    /*
     * Member functions
     */
public:
    std::string        asString() const;
    const std::string& getBehaviour() const { return mBehaviour; }
    const Rlv::BehaviourInfo* getBehaviourInfo() const { return mBhvrInfo; }
    Rlv::EBehaviour    getBehaviourType() const { return (mBhvrInfo) ? mBhvrInfo->getBehaviourType() : Rlv::EBehaviour::Unknown; }
    U32                getBehaviourFlags() const { return (mBhvrInfo) ? mBhvrInfo->getBehaviourFlags() : 0; }
    const LLUUID&      getObjectID() const { return mObjId; }
    const std::string& getOption() const { return mOption; }
    const std::string& getParam() const { return mParam; }
    Rlv::EParamType    getParamType() const { return mParamType; }
    bool               hasOption() const { return !mOption.empty(); }
    bool               isBlocked() const { return (mBhvrInfo) ? mBhvrInfo->isBlocked() : false; }
    bool               isRefCounted() const { return mIsRefCounted; }
    bool               isStrict() const { return mIsStrict; }
    bool               isValid() const { return mIsValid; }
    Rlv::ECmdRet       processCommand() const { return (mBhvrInfo) ? mBhvrInfo->processCommand(*this) : Rlv::ECmdRet::FailedNoProcessor; }

protected:
    static bool parseCommand(const std::string& strCommand, std::string& strBehaviour, std::string& strOption, std::string& strParam);
    bool               markRefCounted() const { return mIsRefCounted = true; }

    /*
     * Operators
     */
public:
    bool operator ==(const RlvCommand&) const;

    /*
     * Member variables
     */
protected:
    bool                    mIsValid = false;
    LLUUID                  mObjId;
    std::string             mBehaviour;
    const Rlv::BehaviourInfo* mBhvrInfo = nullptr;
    Rlv::EParamType         mParamType = Rlv::EParamType::Unknown;
    bool                    mIsStrict = false;
    std::string             mOption;
    std::string             mParam;
    mutable bool            mIsRefCounted = false;

    friend class RlvHandler;
    friend class RlvObject;
    template<Rlv::EParamType> friend struct Rlv::CommandHandlerBaseImpl;
};

// ============================================================================
