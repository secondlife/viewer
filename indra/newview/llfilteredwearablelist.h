/**
 * @file llfilteredwearablelist.h
 * @brief Functionality for showing filtered wearable flat list
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LLFILTEREDWEARABLELIST_H
#define LL_LLFILTEREDWEARABLELIST_H

#include "llinventoryobserver.h"

class LLInventoryItemsList;

// Class that fills LLInventoryItemsList with filtered data.
class LLFilteredWearableListManager : public LLInventoryObserver
{
	LOG_CLASS(LLFilteredWearableListManager);
public:

	LLFilteredWearableListManager(LLInventoryItemsList* list, U64 filter_mask);
	~LLFilteredWearableListManager();

	/** LLInventoryObserver implementation
	 *
	 */
	/*virtual*/ void changed(U32 mask);

	/**
	 * Sets new filter and applies it immediately
	 */
	void setFilterMask(U64 mask);

	/**
	 * Populates wearable list with filtered data.
	 */
	void populateList();

private:
	LLInventoryItemsList* mWearableList;
	U64 mFilterMask;
};

#endif //LL_LLFILTEREDWEARABLELIST_H

// EOF
