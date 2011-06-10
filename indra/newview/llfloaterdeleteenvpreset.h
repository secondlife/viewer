/** 
 * @file llfloaterdeleteenvpreset.h
 * @brief Floater to delete a water / sky / day cycle preset.
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

#ifndef LL_LLFLOATERDELETEENVPRESET_H
#define LL_LLFLOATERDELETEENVPRESET_H

#include "llfloater.h"

class LLComboBox;

class LLFloaterDeleteEnvPreset : public LLFloater
{
	LOG_CLASS(LLFloaterDeleteEnvPreset);

public:
	LLFloaterDeleteEnvPreset(const LLSD &key);

	/*virtual*/	BOOL	postBuild();
	/*virtual*/ void	onOpen(const LLSD& key);

	void onBtnDelete();
	void onBtnCancel();

private:
	void populatePresetsList();
	void populateWaterPresetsList();
	void populateSkyPresetsList();
	void populateDayCyclesList();

	void postPopulate();

	void onDeleteDayCycleConfirmation();
	void onDeleteSkyPresetConfirmation();
	void onDeleteWaterPresetConfirmation();

	LLComboBox* mPresetCombo;
};

#endif // LL_LLFLOATERDELETEENVPRESET_H
