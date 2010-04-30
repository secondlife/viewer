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
#include "llwearableitemslist.h"

class LLFlatListView;

/**
 * Adaptor between LLAccordionCtrlTab and LLFlatListView to facilitate communication between them 
 * (notify, notifyParent) regarding size changes of a list and selection changes across accordion tabs.
 * Besides that it acts as a container for the LLFlatListView and a button bar on top of it.
 */
class LLCOFAccordionListAdaptor : public LLPanel
{
public:
	LLCOFAccordionListAdaptor() : LLPanel() {};
	~LLCOFAccordionListAdaptor() {};

	S32 notifyParent(const LLSD& info)
	{
		LLView* parent = getParent();
		if (!parent) return -1;
		
		if (!info.has("action") || "size_changes" != info["action"])
		{
			return parent->notifyParent(info);
		}

		LLRect rc;
		childGetRect("button_bar", rc);

		LLSD params;
		params["action"] = "size_changes";
		params["width"] = info["width"];
		params["height"] = info["height"].asInteger() + rc.getHeight();

		return parent->notifyParent(params);
	}


	S32 notify(const LLSD& info)
	{
		for (child_list_const_iter_t iter = beginChild(); iter != endChild(); iter++)
		{
			if (dynamic_cast<LLFlatListView*>(*iter))
			{
				return (*iter)->notify(info);
			}
		}
		return LLPanel::notify(info);
	};
};


class LLCOFWearables : public LLPanel
{
public:

	/**
	 * Represents a collection of callbacks assigned to inventory panel item's buttons
	 */
	class LLCOFCallbacks
	{
	public:
		LLCOFCallbacks() {};
		virtual ~LLCOFCallbacks() {};
		
		typedef boost::function<void (void*)> cof_callback_t;

		cof_callback_t mMoveWearableCloser;
		cof_callback_t mMoveWearableFurther;
		cof_callback_t mEditWearable;
		cof_callback_t mDeleteWearable;
	};



	LLCOFWearables();
	virtual ~LLCOFWearables() {};

	/*virtual*/ BOOL postBuild();
	
	LLUUID getSelectedUUID();

	void refresh();
	void clear();

	LLCOFCallbacks& getCOFCallbacks() { return mCOFCallbacks; }

protected:

	void populateAttachmentsAndBodypartsLists(const LLInventoryModel::item_array_t& cof_items);
	void populateClothingList(LLAppearanceMgr::wearables_by_type_t& clothing_by_type);
	
	void addClothingTypesDummies(const LLAppearanceMgr::wearables_by_type_t& clothing_by_type);
	void onSelectionChange(LLFlatListView* selected_list);

	LLPanelClothingListItem* buildClothingListItem(LLViewerInventoryItem* item, bool first, bool last);
	LLPanelBodyPartsListItem* buildBodypartListItem(LLViewerInventoryItem* item);

	LLFlatListView* mAttachments;
	LLFlatListView* mClothing;
	LLFlatListView* mBodyParts;

	LLFlatListView* mLastSelectedList;

	LLCOFCallbacks mCOFCallbacks;

};


#endif
