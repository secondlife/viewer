/** 
 * @file llpanelappearancetab.h
 * @brief Tabs interface for Side Bar "My Appearance" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLPANELAPPEARANCETAB_H
#define LL_LLPANELAPPEARANCETAB_H

#include "llpanel.h"

class LLPanelAppearanceTab : public LLPanel
{
public:
	LLPanelAppearanceTab() : LLPanel() {}
	virtual ~LLPanelAppearanceTab() {}

	virtual void setFilterSubString(const std::string& string) = 0;

	virtual bool isActionEnabled(const LLSD& userdata) = 0;

	virtual void getSelectedItemsUUIDs(uuid_vec_t& selected_uuids) const {}

	static const std::string& getFilterSubString() { return sFilterSubString; }

protected:

	/**
	 * Returns true if there are any items that can be taken off among currently selected, otherwise false.
	 */
	bool canTakeOffSelected();

	static std::string		sFilterSubString;
};

#endif //LL_LLPANELAPPEARANCETAB_H
