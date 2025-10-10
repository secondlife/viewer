/**
 * @file llfloaterscripting.cpp
 * @brief Asset creation permission preferences.
 * @author Jonathan Yap
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include "llcheckboxctrl.h"
#include "llfloaterscripting.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "lluictrlfactory.h"
#include "llpermissions.h"
#include "llagent.h"
#include "llviewerregion.h"
#include "llnotificationsutil.h"
#include "llsdserialize.h"
#include "llvoavatar.h"
#include "llcorehttputil.h"
#include "lleventfilter.h"
#include "lleventcoro.h"
#include "llviewermenufile.h"
#include "llappviewer.h"

namespace
{
    class LLEditorPicker : public LLFilePickerThread
    {
    public:
        LLEditorPicker(LLFloaterScripting *floater):
            LLFilePickerThread(LLFilePicker::FFLOAD_EXE)
        {
            mHandle = floater->getDerivedHandle<LLFloaterScripting>();
        }
        void notify(const std::vector<std::string>& filenames) override
        {
            if (LLAppViewer::instance()->quitRequested())
            {
                return;
            }

            LLFloaterScripting* floater = mHandle.get();
            if (floater && !filenames.empty())
            {
                floater->pickedEditor(filenames.front());
            }
        }

    private:
        LLHandle<LLFloaterScripting> mHandle;
    };

}

LLFloaterScripting::LLFloaterScripting(const LLSD& seed)
    : LLFloater(seed)
{
    mCommitCallbackRegistrar.add("ScriptingSettings.CLOSE", boost::bind(&LLFloaterScripting::onClickClose, this));
    mCommitCallbackRegistrar.add("ScriptingSettings.BROWSE", boost::bind(&LLFloaterScripting::onClickBrowse, this));
}

bool LLFloaterScripting::postBuild()
{
    refresh();
    return true;
}

void LLFloaterScripting::onClickClose()
{
    closeFloater();
}

void LLFloaterScripting::onClickBrowse()
{
    (new LLEditorPicker(this))->getFile();
}

void LLFloaterScripting::pickedEditor(std::string_view editor)
{
    assert_main_thread();
    std::stringstream cmdline;
    cmdline << "\"" << editor << "\" \"%s\"";

    gSavedSettings.setString("ExternalEditor", cmdline.str());
}
