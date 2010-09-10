/**
 * @file llpanelappearancetab.h
 * @brief Tabs interface for Side Bar "My Appearance" panel
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llpanelappearancetab.h"


#include "llinventoryfunctions.h"
#include "llinventorymodel.h"

//virtual
bool LLPanelAppearanceTab::canTakeOffSelected()
{
	uuid_vec_t selected_uuids;
	getSelectedItemsUUIDs(selected_uuids);

	LLFindWearablesEx is_worn(/*is_worn=*/ true, /*include_body_parts=*/ false);

	for (uuid_vec_t::const_iterator it=selected_uuids.begin(); it != selected_uuids.end(); ++it)
	{
		LLViewerInventoryItem* item = gInventory.getItem(*it);
		if (!item) continue;

		if (is_worn(NULL, item)) return true;
	}
	return false;
}
