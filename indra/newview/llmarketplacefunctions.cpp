/** 
 * @file llmarketplacefunctions.cpp
 * @brief Implementation of assorted functions related to the marketplace
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

#include "llviewerprecompiledheaders.h"

#include "llmarketplacefunctions.h"

#include "llagent.h"
#include "llviewernetwork.h"


std::string getMarketplaceBaseURL()
{
	std::string url = "https://marketplace.secondlife.com/";

	if (!LLGridManager::getInstance()->isInProductionGrid())
	{
		std::string gridLabel = LLGridManager::getInstance()->getGridLabel();
		url = llformat("https://marketplace.%s.lindenlab.com/", utf8str_tolower(gridLabel).c_str());
	}

	url += "api/1/users/";
	url += gAgent.getID().getString();

	return url;
}

std::string getMarketplaceURL_InventoryImport()
{
	std::string url = getMarketplaceBaseURL();

	url += "/inventory_import";

	return url;
}

std::string getMarketplaceURL_UserStatus()
{
	std::string url = getMarketplaceBaseURL();

	url += "/user_status";

	return url;
}


static bool gMarketplaceSyncEnabled = false;

bool getMarketplaceSyncEnabled()
{
	return gMarketplaceSyncEnabled;
}

void setMarketplaceSyncEnabled(bool syncEnabled)
{
	gMarketplaceSyncEnabled = syncEnabled;
}
