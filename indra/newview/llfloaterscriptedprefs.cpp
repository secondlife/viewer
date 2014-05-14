/**
 * @file llfloaterscriptedprefs.cpp
 * @brief Color controls for the script editor
 * @author Cinder Roxley
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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
#include "llfloaterscriptedprefs.h"

#include "llcolorswatch.h"


LLFloaterScriptEdPrefs::LLFloaterScriptEdPrefs(const LLSD& key)
:	LLFloater(key)
{
	mCommitCallbackRegistrar.add("ScriptPref.applyUIColor",	boost::bind(&LLFloaterScriptEdPrefs::applyUIColor, this ,_1, _2));
	mCommitCallbackRegistrar.add("ScriptPref.getUIColor",	boost::bind(&LLFloaterScriptEdPrefs::getUIColor, this ,_1, _2));
}

BOOL LLFloaterScriptEdPrefs::postBuild()
{
	return TRUE;
}

void LLFloaterScriptEdPrefs::applyUIColor(LLUICtrl* ctrl, const LLSD& param)
{
	LLUIColorTable::instance().setColor(param.asString(), LLColor4(ctrl->getValue()));
	// *TODO: Signal all active script editors to change colors on the fly.
}

void LLFloaterScriptEdPrefs::getUIColor(LLUICtrl* ctrl, const LLSD& param)
{
	LLColorSwatchCtrl* color_swatch = dynamic_cast<LLColorSwatchCtrl*>(ctrl);
	color_swatch->setOriginal(LLUIColorTable::instance().getColor(param.asString()));
}
