#pragma once

#include "llfloater.h"

#include "rlvdefines.h"

class LLChatEntry;
class LLLayoutPanel;
class LLTextEditor;
class RlvCommand;
class RlvHandler;

namespace Rlv
{
    // ============================================================================
    // FloaterConsole - debug console to allow command execution without the need for a script
    //

    class FloaterConsole : public LLFloater
    {
        friend class LLFloaterReg;
        FloaterConsole(const LLSD& sdKey) : LLFloater(sdKey) {}

    public:
        bool postBuild() override;
        void onClose(bool fQuitting) override;
    protected:
        void onInput();
        void reshapeLayoutPanel();

    private:
        boost::signals2::scoped_connection mCommandOutputConn;
        int mInputEditPad = 0;
        LLLayoutPanel* mInputPanel = nullptr;
        LLChatEntry* mInputEdit = nullptr;
        LLTextEditor* mOutputText = nullptr;
    };

    // ============================================================================
};
