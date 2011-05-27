/** 
 * @file llfloatereditdaycycle.cpp
 * @brief Floater to create or edit a day cycle
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llfloatereditdaycycle.h"

// libs
#include "llbutton.h"

// newview

LLFloaterEditDayCycle::LLFloaterEditDayCycle(const LLSD &key)
:	LLFloater(key)
{
}

// virtual
BOOL LLFloaterEditDayCycle::postBuild()
{
	getChild<LLButton>("cancel")->setCommitCallback(boost::bind(&LLFloaterEditDayCycle::onBtnCancel, this));

	// Disable some non-functional controls.
	getChildView("day_cycle_combo")->setEnabled(FALSE);
	getChildView("save")->setEnabled(FALSE);

	return TRUE;
}

// virtual
void LLFloaterEditDayCycle::onOpen(const LLSD& key)
{
	std::string param = key.asString();
	std::string floater_title = getString(std::string("title_") + param);
	std::string hint = getString(std::string("hint_" + param));

	// Update floater title.
	setTitle(floater_title);

	// Update the hint at the top.
	getChild<LLUICtrl>("hint")->setValue(hint);

	// Hide the hint to the right of the combo if we're invoked to create a new preset.
	getChildView("note")->setVisible(param == "edit");
}

void LLFloaterEditDayCycle::onBtnCancel()
{
	closeFloater();
}
