/** 
 * @file llcofwearables.h
 * @brief LLCOFWearables displayes wearables from the current outfit split into three lists (attachments, clothing and body parts)
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

#ifndef LL_LLCOFWEARABLES_H
#define LL_LLCOFWEARABLES_H

#include "llpanel.h"
#include "llinventorymodel.h"
#include "llappearancemgr.h"

class LLFlatListView;

class LLCOFWearables : public LLPanel
{
public:
	LLCOFWearables();
	virtual ~LLCOFWearables() {};

	/*virtual*/ BOOL postBuild();
	
	LLUUID getSelectedUUID();

	void refresh();
	void clear();

protected:

	void populateAttachmentsAndBodypartsLists(const LLInventoryModel::item_array_t& cof_items);
	void populateClothingList(LLAppearanceMgr::wearables_by_type_t& clothing_by_type);
	
	void addListButtonBar(LLFlatListView* list, std::string xml_filename);
	void addClothingTypesDummies(const LLAppearanceMgr::wearables_by_type_t& clothing_by_type);
	void addWearableTypeSeparator(LLFlatListView* list);
	void onSelectionChange(LLFlatListView* selected_list);

	LLXMLNodePtr getXMLNode(std::string xml_filename);

	LLFlatListView* mAttachments;
	LLFlatListView* mClothing;
	LLFlatListView* mBodyParts;

	LLFlatListView* mLastSelectedList;

};


#endif
