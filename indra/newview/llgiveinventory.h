/**
 * @file llgiveinventory.cpp
 * @brief LLGiveInventory class declaration
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

#ifndef LL_LLGIVEINVENTORY_H
#define LL_LLGIVEINVENTORY_H

class LLInventoryItem;
class LLInventoryCategory;

/**
 * Class represented give inventory related actions.
 *
 * It has only static methods and is not intended to be instantiated for now.
 */
class LLGiveInventory
{
public:
	/**
	 * Checks if inventory item you are attempting to transfer to a resident can be given.
	 *
	 * @return true if you can give, otherwise false.
	 */
	static bool isInventoryGiveAcceptable(const LLInventoryItem* item);

	/**
	 * Checks if inventory item you are attempting to transfer to a group can be given.
	 *
	 * @return true if you can give, otherwise false.
	 */
	static bool isInventoryGroupGiveAcceptable(const LLInventoryItem* item);

	/**
	 * Gives passed inventory item to specified avatar in specified session.
	 */
	static bool doGiveInventoryItem(const LLUUID& to_agent,
									const LLInventoryItem* item,
									const LLUUID& im_session_id = LLUUID::null);

	/**
	 * Gives passed inventory category to specified avatar in specified session.
	 */
	static bool doGiveInventoryCategory(const LLUUID& to_agent,
									const LLInventoryCategory* item,
									const LLUUID &session_id = LLUUID::null,
									const std::string& notification = std::string());

	// give inventory item functionality
	static bool handleCopyProtectedItem(const LLSD& notification, const LLSD& response);

private:
	// this class is not intended to be instantiated.
	LLGiveInventory();

	/**
	 * logs "Inventory item offered" to IM
	 */
	static void logInventoryOffer(const LLUUID& to_agent,
									const LLUUID &im_session_id = LLUUID::null);

	static void commitGiveInventoryItem(const LLUUID& to_agent,
									const LLInventoryItem* item,
									const LLUUID &im_session_id = LLUUID::null);

	// give inventory category functionality
	static bool handleCopyProtectedCategory(const LLSD& notification, const LLSD& response);
	static bool commitGiveInventoryCategory(const LLUUID& to_agent,
									const LLInventoryCategory* cat,
									const LLUUID &im_session_id = LLUUID::null);

};

#endif // LL_LLGIVEINVENTORY_H
