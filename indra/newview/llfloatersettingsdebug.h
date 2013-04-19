/** 
 * @file llfloatersettingsdebug.h
 * @brief floater for debugging internal viewer settings
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#ifndef LLFLOATERDEBUGSETTINGS_H
#define LLFLOATERDEBUGSETTINGS_H

#include "llcontrol.h"
#include "llfloater.h"

class LLFloaterSettingsDebug 
:	public LLFloater
{
	friend class LLFloaterReg;

public:

	virtual BOOL postBuild();
	virtual void draw();

	void updateControl(LLControlVariable* control);

	void onSettingSelect(LLUICtrl* ctrl);
	void onCommitSettings();
	void onClickDefault();

private:
	// key - selects which settings to show, one of:
	// "all", "base", "account", "skin"
	LLFloaterSettingsDebug(const LLSD& key);
	virtual ~LLFloaterSettingsDebug();
	
protected:
	class LLTextEditor* mComment;
};

#endif //LLFLOATERDEBUGSETTINGS_H

