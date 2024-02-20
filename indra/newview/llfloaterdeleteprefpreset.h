/** 
 * @file llfloaterdeleteprefpreset.h
 * @brief Floater to delete a graphics / camera preset

 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
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

#ifndef LL_LLFLOATERDELETEPREFPRESET_H
#define LL_LLFLOATERDELETEPREFPRESET_H

#include "llfloater.h"

class LLComboBox;

class LLFloaterDeletePrefPreset : public LLFloater
{

public:
	LLFloaterDeletePrefPreset(const LLSD &key);

	bool postBuild() override;
	void onOpen(const LLSD& key) override;

	void onBtnDelete();
	void onBtnCancel();

private:
	void onPresetsListChange();

	std::string mSubdirectory;
};

#endif // LL_LLFLOATERDELETEPREFPRESET_H
