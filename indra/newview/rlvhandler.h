#pragma once

#include "llchat.h"
#include "llsingleton.h"

#include "rlvhelper.h"

class LLViewerObject;

// ============================================================================
// RlvHandler class
//

class RlvHandler : public LLSingleton<RlvHandler>
{
    template<Rlv::EParamType> friend struct Rlv::CommandHandlerBaseImpl;

    LLSINGLETON_EMPTY_CTOR(RlvHandler);

    /*
     * Command processing
     */
public:
    // Command processing helper functions
    bool         handleSimulatorChat(std::string& message, const LLChat& chat, const LLViewerObject* chatObj);
    Rlv::ECmdRet processCommand(const LLUUID& idObj, const std::string& stCmd, bool fromObj);
protected:
    Rlv::ECmdRet processCommand(std::reference_wrapper<const RlvCommand> rlvCmdRef, bool fromObj);

    /*
     * Helper functions
     */
public:
    // Initialization (deliberately static so they can safely be called in tight loops)
    static bool canEnable();
    static bool isEnabled() { return mIsEnabled; }
    static bool setEnabled(bool enable);

    /*
     * Event handling
     */
public:
    // The command output signal is triggered whenever a command produces channel or debug output
    using command_output_signal_t = boost::signals2::signal<void (const RlvCommand&, S32, const std::string&)>;
    boost::signals2::connection setCommandOutputCallback(const command_output_signal_t::slot_type& cb) { return mOnCommandOutput.connect(cb); }

protected:
    command_output_signal_t mOnCommandOutput;
private:
    static bool mIsEnabled;
};

// ============================================================================
