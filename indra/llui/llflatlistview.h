/** 
 * @file llflatlistview.h
 * @brief LLFlatListView base class
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_LLFLATLISTVIEW_H
#define LL_LLFLATLISTVIEW_H

#include "llscrollcontainer.h"


class LLPanel;

/**
 * LLFlatListView represents a flat list ui control that operates on items in a form of LLPanel's.
 * LLSD can be associated with each added item, it can keep data from an item in digested form.
 * Associated LLSD's can be of any type (singular, a map etc.).
 * Items (LLPanel's subclasses) can be of different height.
 * The list is LLPanel created in itself and grows in height while new items are added. 
 * 
 * The control can manage selection of its items when the flag "allow_select" is set. Also ability to select
 * multiple items (by using CTRL) is enabled through setting the flag "multi_select" - if selection is not allowed that flag 
 * is ignored. The option "keep_one_selected" forces at least one item to be selected at any time (only for mouse events on items)
 * since any item of the list was selected.
 *
 * Examples of using this control are presented in Picks panel (Me Profile and Profile View), where this control is used to 
 * manage the list of pick items.
 *
 * ASSUMPTIONS AND STUFF
 * - NULL pointers and undefined LLSD's are not accepted by any method of this class unless specified otherwise
 * - Order of returned selected items are not guaranteed
 * - The control assumes that all items being added are unique.
 */
class LLFlatListView : public LLScrollContainer
{
public:

	struct Params : public LLInitParam::Block<Params, LLScrollContainer::Params>
	{
		/** turning on/off selection support */
		Optional<bool> allow_select;

		/** turning on/off multiple selection (works while clicking and holding CTRL)*/
		Optional<bool> multi_select;

		/** don't allow to deselect all selected items (for mouse events on items only) */
		Optional<bool> keep_one_selected;

		/** padding between items */
		Optional<U32> item_pad; 

		Params();
	};
	
	virtual ~LLFlatListView() { clear(); };


	/** Overridden LLPanel's reshape, height is ignored, the list sets its height to accommodate all items */
	virtual void reshape(S32 width, S32 height, BOOL called_from_parent  = TRUE);


	/**
	 * Adds and item and LLSD value associated with it to the list at specified position
	 * @return true if the item was added, false otherwise 
	 */
	virtual bool addItem(LLPanel* item, LLSD value = LLUUID::null, EAddPosition pos = ADD_BOTTOM);

	/**
	 * Insert item_to_add along with associated value to the list right after the after_item.
	 * @return true if the item was successfully added, false otherwise
	 */
	virtual bool insertItemAfter(LLPanel* after_item, LLPanel* item_to_add, LLSD value = LLUUID::null);

	/** 
	 * Remove specified item
	 * @return true if the item was removed, false otherwise 
	 */
	virtual bool removeItem(LLPanel* item);

	/** 
	 * Remove an item specified by value
	 * @return true if the item was removed, false otherwise 
	 */
	virtual bool removeItemByValue(const LLSD& value);

	/** 
	 * Remove an item specified by uuid
	 * @return true if the item was removed, false otherwise 
	 */
	virtual bool removeItemByUUID(LLUUID& uuid);

	/** 
	 * Get an item by value 
	 * @return the item as LLPanel if associated with value, NULL otherwise
	 */
	virtual LLPanel* getItemByValue(LLSD& value) const;

	/** 
	 * Select or deselect specified item based on select
	 * @return true if succeed, false otherwise
	 */
	virtual bool selectItem(LLPanel* item, bool select = true);

	/** 
	 * Select or deselect an item by associated value based on select
	 * @return true if succeed, false otherwise
	 */
	virtual bool selectItemByValue(const LLSD& value, bool select = true);

	/** 
	 * Select or deselect an item by associated uuid based on select
	 * @return true if succeed, false otherwise
	 */
	virtual bool selectItemByUUID(LLUUID& uuid, bool select = true);


	
	/**
	 * Get LLSD associated with the first selected item
	 */
	virtual LLSD getSelectedValue() const;

	/**
	 * Get LLSD's associated with selected items.
	 * @param selected_values std::vector being populated with LLSD associated with selected items
 	 */
	virtual void getSelectedValues(std::vector<LLSD>& selected_values) const;


	/** 
	 * Get LLUUID associated with selected item
	 * @return LLUUID if such was associated with selected item 
	 */
	virtual LLUUID getSelectedUUID() const;

	/** 
	 * Get LLUUIDs associated with selected items
	 * @param selected_uuids An std::vector being populated with LLUUIDs associated with selected items
	 */
	virtual void getSelectedUUIDs(std::vector<LLUUID>& selected_uuids) const;

	/** Get the top selected item */
	virtual LLPanel* getSelectedItem() const;

	/** 
	 * Get selected items
	 * @param selected_items An std::vector being populated with pointers to selected items
	 */
	virtual void getSelectedItems(std::vector<LLPanel*>& selected_items) const;


	/** Resets selection of items */
	virtual void resetSelection();


	/** Turn on/off multiple selection support */
	void setAllowMultipleSelection(bool allow) { mMultipleSelection = allow; }

	/** Turn on/off selection support */
	void setAllowSelection(bool can_select) { mAllowSelection = can_select; }


	/** Get number of selected items in the list */
	U32 numSelected() const {return mSelectedItemPairs.size(); }

	/** Get number of items in the list */
	U32 size() const { return mItemPairs.size(); }


	/** Removes all items from the list */
	virtual void clear();


protected:

	/** Pairs LLpanel representing a single item LLPanel and LLSD associated with it */
	typedef std::pair<LLPanel*, LLSD> item_pair_t;

	typedef std::list<item_pair_t*> pairs_list_t;
	typedef pairs_list_t::iterator pairs_iterator_t;
	typedef pairs_list_t::const_iterator pairs_const_iterator_t;


	friend class LLUICtrlFactory;
	LLFlatListView(const LLFlatListView::Params& p);

	/** Manage selection on mouse events */
	void onItemMouseClick(item_pair_t* item_pair, MASK mask);

	/** Updates position of items */
	virtual void rearrangeItems();

	virtual item_pair_t* getItemPair(LLPanel* item) const;

	virtual item_pair_t* getItemPair(const LLSD& value) const;

	virtual bool selectItemPair(item_pair_t* item_pair, bool select);

	virtual bool isSelected(item_pair_t* item_pair) const;

	virtual bool removeItemPair(item_pair_t* item_pair);


private:

	void setItemsNoScrollWidth(S32 new_width) {mItemsNoScrollWidth = new_width - 2 * mBorderThickness;}


private:

	LLPanel* mItemsPanel;

	S32 mItemsNoScrollWidth;

	S32 mBorderThickness;

	/** Items padding */
	U32 mItemPad;

	/** Selection support flag */
	bool mAllowSelection;

	/** Multiselection support flag, ignored if selection is not supported */
	bool mMultipleSelection;

	bool mKeepOneItemSelected;

	/** All pairs of the list */
	pairs_list_t mItemPairs;

	/** Selected pairs for faster access */
	pairs_list_t mSelectedItemPairs;
};

#endif
