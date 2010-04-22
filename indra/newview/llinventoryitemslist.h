/**
 * @file llinventoryitemslist.h
 * @brief A list of inventory items represented by LLFlatListView.
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

#ifndef LL_LLINVENTORYITEMSLIST_H
#define LL_LLINVENTORYITEMSLIST_H

#include "llpanel.h"

#include "llassettype.h"

#include "llinventorytype.h"

// newview
#include "llflatlistview.h"
#include "llinventorymodel.h"

class LLIconCtrl;
class LLTextBox;

class LLPanelInventoryItem : public LLPanel
{
public:
	LLPanelInventoryItem(LLAssetType::EType asset_type,
						 LLInventoryType::EType inventory_type,
						 U32 wearable_type,
						 const std::string &item_name,
						 const std::string &hl);
	virtual ~LLPanelInventoryItem();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void setValue(const LLSD& value);

	void updateItem();

	void onMouseEnter(S32 x, S32 y, MASK mask);
	void onMouseLeave(S32 x, S32 y, MASK mask);

private:
	LLIconCtrl*		mIcon;
	LLTextBox*		mTitle;

	LLUIImagePtr	mItemIcon;
	std::string		mItemName;
	std::string		mHighlightedText;
};


class LLInventoryItemsList : public LLFlatListView
{
public:
	virtual ~LLInventoryItemsList();

	void refreshList(const LLInventoryModel::item_array_t item_array);

protected:
	friend class LLUICtrlFactory;
	LLInventoryItemsList(const LLFlatListView::Params& p);
};

#endif //LL_LLINVENTORYITEMSLIST_H
