/**
 * @file lloutfitslist.h
 * @brief List of agent's outfits for My Appearance side panel.
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

#ifndef LL_LLOUTFITSLIST_H
#define LL_LLOUTFITSLIST_H

#include "llpanel.h"

// newview
#include "llinventorymodel.h"
#include "llinventoryobserver.h"

class LLAccordionCtrl;
class LLAccordionCtrlTab;
class LLWearableItemsList;

/**
 * @class LLOutfitsList
 *
 * A list of agents's outfits from "My Outfits" inventory category
 * which displays each outfit in an accordion tab with a flat list
 * of items inside it.
 * Uses LLInventoryCategoriesObserver to monitor changes to "My Outfits"
 * inventory category and refresh the outfits listed in it.
 * This class is derived from LLInventoryObserver to know when inventory
 * becomes usable and it is safe to request data from inventory model.
 */
class LLOutfitsList : public LLPanel, public LLInventoryObserver
{
public:
	LLOutfitsList();
	virtual ~LLOutfitsList();

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void changed(U32 mask);

	void refreshList(const LLUUID& category_id);

	// Update tab displaying outfit identified by category_id.
	void updateOutfitTab(const LLUUID& category_id);

	void onTabExpandedCollapsed(LLWearableItemsList* list);

	void setFilterSubString(const std::string& string);

private:
	/**
	 * Reads xml with accordion tab and Flat list from xml file.
	 *
	 * @return LLPointer to XMLNode with accordion tab and flat list.
	 */
	LLXMLNodePtr getAccordionTabXMLNode();

	/**
	 * Wrapper for LLCommonUtils::computeDifference. @see LLCommonUtils::computeDifference
	 */
	void computeDifference(const LLInventoryModel::cat_array_t& vcats, uuid_vec_t& vadded, uuid_vec_t& vremoved);


	LLInventoryCategoriesObserver* 	mCategoriesObserver;

	LLAccordionCtrl*				mAccordion;
	LLPanel*						mListCommands;

	std::string 					mFilterSubString;

	typedef	std::map<LLUUID, LLAccordionCtrlTab*>		outfits_map_t;
	typedef outfits_map_t::value_type					outfits_map_value_t;
	outfits_map_t					mOutfitsMap;
};

#endif //LL_LLOUTFITSLIST_H
