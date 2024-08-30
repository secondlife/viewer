#include "llviewerprecompiledheaders.h"
#include "llappviewer.h"
#include "llstartup.h"

#include "rlvdefines.h"
#include "rlvcommon.h"
#include "rlvhandler.h"

using namespace Rlv;

// ============================================================================
// Static variable initialization
//

bool RlvHandler::mIsEnabled = false;

// ============================================================================
// Initialization helper functions
//

bool RlvHandler::canEnable()
{
    return LLStartUp::getStartupState() <= STATE_LOGIN_CLEANUP;
}

bool RlvHandler::setEnabled(bool enable)
{
    if (mIsEnabled == enable)
        return enable;

    if (enable && canEnable())
    {
        RLV_INFOS << "Enabling Restrained Love API support - " << RlvStrings::getVersionAbout() << RLV_ENDL;
        mIsEnabled = true;
    }

    return mIsEnabled;
}

// ============================================================================
