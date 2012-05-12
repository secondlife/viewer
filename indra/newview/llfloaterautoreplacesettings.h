/** 
 * @file llfloaterautoreplacesettings.h
 * @brief Auto Replace List floater
 * @copyright Copyright (c) 2011 LordGregGreg Back
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * $/LicenseInfo$
 */

#ifndef LLFLOATERAUTOREPLACESETTINGS_H
#define LLFLOATERAUTOREPLACESETTINGS_H

#include "llfloater.h"
#include "llmediactrl.h"
#include "llscrolllistctrl.h"
#include "lllineeditor.h"

#include "llviewerinventory.h"
#include <boost/bind.hpp>

class LLFloaterAutoReplaceSettings : 
public LLFloater
{
public:
	LLFloaterAutoReplaceSettings(const LLSD& key);

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onClose(bool app_quitting);

	void setData(void * data);
	void updateEnabledStuff();
	void updateNamesList();
	void updateListControlsEnabled(BOOL selected);
	void updateItemsList();

	LLScrollListCtrl *namesList;
	LLScrollListCtrl *entryList;
	LLLineEditor* mOldText;
	LLLineEditor* mNewText;

private:

	static void onBoxCommitEnabled(LLUICtrl* caller, void* user_data);
	static void onEntrySettingChange(LLUICtrl* caller, void* user_data);
	static void onSelectName(LLUICtrl* caller, void* user_data);

	static void deleteEntry(void* data);
	static void addEntry(void* data);
	static void exportList(void* data);
	static void removeList(void* data);
	static void loadList(void* data);
};

#endif  // LLFLOATERAUTOREPLACESETTINGS_H
