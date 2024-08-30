#pragma once

#include "llsingleton.h"

#include "rlvdefines.h"

// ============================================================================
// RlvHandler class
//

class RlvHandler : public LLSingleton<RlvHandler>
{
    LLSINGLETON_EMPTY_CTOR(RlvHandler);

public:
    // Initialization (deliberately static so they can safely be called in tight loops)
    static bool canEnable();
    static bool isEnabled() { return mIsEnabled; }
    static bool setEnabled(bool enable);

private:
    static bool mIsEnabled;
};

// ============================================================================
