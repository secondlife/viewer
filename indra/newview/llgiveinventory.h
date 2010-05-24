/**
 * @file llgiveinventory.cpp
 * @brief LLGiveInventory class declaration
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
	static void doGiveInventoryItem(const LLUUID& to_agent,
									const LLInventoryItem* item,
									const LLUUID& im_session_id = LLUUID::null);

	/**
	 * Gives passed inventory category to specified avatar in specified session.
	 */
	static void doGiveInventoryCategory(const LLUUID& to_agent,
									const LLInventoryCategory* item,
									const LLUUID &session_id = LLUUID::null);

private:
	// this class is not intended to be instantiated.
	LLGiveInventory();

	/**
	 * logs "Inventory item offered" to IM
	 */
	static void logInventoryOffer(const LLUUID& to_agent,
									const LLUUID &im_session_id = LLUUID::null);

	// give inventory item functionality
	static bool handleCopyProtectedItem(const LLSD& notification, const LLSD& response);
	static void commitGiveInventoryItem(const LLUUID& to_agent,
									const LLInventoryItem* item,
									const LLUUID &im_session_id = LLUUID::null);

	// give inventory category functionality
	static bool handleCopyProtectedCategory(const LLSD& notification, const LLSD& response);
	static void commitGiveInventoryCategory(const LLUUID& to_agent,
									const LLInventoryCategory* cat,
									const LLUUID &im_session_id = LLUUID::null);

};

#endif // LL_LLGIVEINVENTORY_H
