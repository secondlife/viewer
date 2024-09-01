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

public:
    // Initialization (deliberately static so they can safely be called in tight loops)
    static bool canEnable();
    static bool isEnabled() { return mIsEnabled; }
    static bool setEnabled(bool enable);

private:
    static bool mIsEnabled;
};

// ============================================================================
