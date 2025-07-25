/**
 * @file llfloaterslapptest.cpp
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
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

#include "llfloaterslapptest.h"
#include "lluictrlfactory.h"

#include "lllineeditor.h"
#include "lltextbox.h"

LLFloaterSLappTest::LLFloaterSLappTest(const LLSD& key)
    :   LLFloater("floater_test_slapp")
{
}

LLFloaterSLappTest::~LLFloaterSLappTest()
{}

bool LLFloaterSLappTest::postBuild()
{
    getChild<LLLineEditor>("remove_folder_id")->setKeystrokeCallback([this](LLLineEditor* editor, void*)
        {
            std::string slapp(getString("remove_folder_slapp"));
            getChild<LLTextBox>("remove_folder_txt")->setValue(slapp + editor->getValue().asString());
        }, NULL);

    return true;
}
