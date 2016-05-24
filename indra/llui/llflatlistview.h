/** 
 * @file llflatlistview.h
 * @brief LLFlatListView base class and extension to support messages for several cases of an empty list.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLFLATLISTVIEW_H
#define LL_LLFLATLISTVIEW_H

#include "llpanel.h"
#include "llscrollcontainer.h"
#include "lltextbox.h"


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
 * Examples of using this control are presented in Picks panel (My Profile and Profile View), where this control is used to 
 * manage the list of pick items.
 *
 * ASSUMPTIONS AND STUFF
 * - NULL pointers and undefined LLSD's are not accepted by any method of this class unless specified otherwise
 * - Order of returned selected items are not guaranteed
 * - The control assumes that all items being added are unique.
 */
class LLFlatListView : public LLScrollContainer, public LLEditMenuHandler
{
	LOG_CLASS(LLFlatListView);
public:

	/**
	 * Abstract comparator for comparing flat list items in a form of LLPanel
	 */
	class ItemComparator
	{
	public:
		ItemComparator() {};
		virtual ~ItemComparator() {};

		/** Returns true if item1 < item2, false otherwise */
		virtual bool compare(const LLPanel* item1, const LLPanel* item2) const = 0;
	};

	/**
	 * Represents reverse comparator which acts as a decorator for a comparator that need to be reversed
	 */
	class ItemReverseComparator : public ItemComparator
	{
	public:
		ItemReverseComparator(const ItemComparator& comparator) : mComparator(comparator) {};
		virtual ~ItemReverseComparator() {};

		virtual bool compare(const LLPanel* item1, const LLPanel* item2) const
		{
			return mComparator.compare(item2, item1);
		}

	private:
		const ItemComparator& mComparator;
	};


	struct Params : public LLInitParam::Block<Params, LLScrollContainer::Params>
	{
		/** turning on/off selection support */
		Optional<bool> allow_select;

		/** turning on/off multiple selection (works while clicking and holding CTRL)*/
		Optional<bool> multi_select;

		/** don't allow to deselect all selected items (for mouse events on items only) */
		Optional<bool> keep_one_selected;

		/** try to keep selection visible after reshape */
		Optional<bool> keep_selection_visible_on_reshape;

		/** padding between items */
		Optional<U32> item_pad; 

		/** textbox with info message when list is empty*/
		Optional<LLTextBox::Params> no_items_text;

		Params();
	};
	
	// disable traversal when finding widget to hand focus off to
	/*virtual*/ BOOL canFocusChildren() const { return FALSE; }

	/**
	 * Connects callback to signal called when Return key is pressed.
	 */
	boost::signals2::connection setReturnCallback( const commit_signal_t::slot_type& cb ) { return mOnReturnSignal.connect(cb); }

	/** Overridden LLPanel's reshape, height is ignored, the list sets its height to accommodate all items */
	virtual void reshape(S32 width, S32 height, BOOL called_from_parent  = TRUE);

	/** Returns full rect of child panel */
	const LLRect& getItemsRect() const;

	LLRect getRequiredRect() { return getItemsRect(); }

	/** Returns distance between items */
	const S32 getItemsPad() { return mItemPad; }

	/**
	 * Adds and item and LLSD value associated with it to the list at specified position
	 * @return true if the item was added, false otherwise 
	 */
	virtual bool addItem(LLPanel * item, const LLSD& value = LLUUID::null, EAddPosition pos = ADD_BOTTOM, bool rearrange = true);

	/**
	 * Insert item_to_add along with associated value to the list right after the after_item.
	 * @return true if the item was successfully added, false otherwise
	 */
	virtual bool insertItemAfter(LLPanel* after_item, LLPanel* item_to_add, const LLSD& value = LLUUID::null);

	/** 
	 * Remove specified item
	 * @return true if the item was removed, false otherwise 
	 */
	virtual bool removeItem(LLPanel* item, bool rearrange = true);

	/** 
	 * Remove an item specified by value
	 * @return true if the item was removed, false otherwise 
	 */
	virtual bool removeItemByValue(const LLSD& value, bool rearrange = true);

	/** 
	 * Remove an item specified by uuid
	 * @return true if the item was removed, false otherwise 
	 */
	virtual bool removeItemByUUID(const LLUUID& uuid, bool rearrange = true);

	/** 
	 * Get an item by value 
	 * @return the item as LLPanel if associated with value, NULL otherwise
	 */
	virtual LLPanel* getItemByValue(const LLSD& value) const;

	template<class T>
	T* getTypedItemByValue(const LLSD& value) const
	{
		return dynamic_cast<T*>(getItemByValue(value));
	}

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
	virtual bool selectItemByUUID(const LLUUID& uuid, bool select = true);

	/**
	 * Get all panels stored in the list.
	 */
	virtual void getItems(std::vector<LLPanel*>& items) const;

	/**
	 * Get all items values.
	 */
	virtual void getValues(std::vector<LLSD>& values) const;
	
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
	virtual void getSelectedUUIDs(uuid_vec_t& selected_uuids) const;

	/** Get the top selected item */
	virtual LLPanel* getSelectedItem() const;

	/** 
	 * Get selected items
	 * @param selected_items An std::vector being populated with pointers to selected items
	 */
	virtual void getSelectedItems(std::vector<LLPanel*>& selected_items) const;


	/**
	 * Resets selection of items.
	 * 
	 * It calls onCommit callback if setCommitOnSelectionChange(bool b) was called with "true"
	 * argument for current Flat List.
	 * @param no_commit_on_deselection - if true onCommit callback will not be called
	 */
	virtual void resetSelection(bool no_commit_on_deselection = false);

	/**
	 * Sets comment text which will be shown in the list is it is empty.
	 *
	 * Textbox to hold passed text is created while this method is called at the first time.
	 *
	 * @param comment_text - string to be shown as a comment.
	 */
	void setNoItemsCommentText( const std::string& comment_text);

	/** Turn on/off multiple selection support */
	void setAllowMultipleSelection(bool allow) { mMultipleSelection = allow; }

	/** Turn on/off selection support */
	void setAllowSelection(bool can_select) { mAllowSelection = can_select; }

	/** Sets flag whether onCommit should be fired if selection was changed */
	// FIXME: this should really be a separate signal, since "Commit" implies explicit user action, and selection changes can happen more indirectly.
	void setCommitOnSelectionChange(bool b)		{ mCommitOnSelectionChange = b; }

	/** Get number of selected items in the list */
	U32 numSelected() const {return mSelectedItemPairs.size(); }

	/** Get number of (visible) items in the list */
	U32 size(const bool only_visible_items = true) const;

	/** Removes all items from the list */
	virtual void clear();

	/**
	 * Removes all items that can be detached from the list but doesn't destroy
	 * them, caller responsible to manage items after they are detached.
	 * Detachable item should accept "detach" action via notify() method,
	 * where it disconnect all callbacks, does other valuable routines and
	 * return 1.
	 */
	void detachItems(std::vector<LLPanel*>& detached_items);

	/**
	 * Set comparator to use for future sorts.
	 * 
	 * This class does NOT manage lifetime of the comparator
	 * but assumes that the comparator is always alive.
	 */
	void setComparator(const ItemComparator* comp) { mItemComparator = comp; }
	void sort();

	bool updateValue(const LLSD& old_value, const LLSD& new_value);

	void scrollToShowFirstSelectedItem();

	void selectFirstItem	();
	void selectLastItem		();

	virtual S32	notify(const LLSD& info) ;

protected:

	/** Pairs LLpanel representing a single item LLPanel and LLSD associated with it */
	typedef std::pair<LLPanel*, LLSD> item_pair_t;

	typedef std::list<item_pair_t*> pairs_list_t;
	typedef pairs_list_t::iterator pairs_iterator_t;
	typedef pairs_list_t::const_iterator pairs_const_iterator_t;

	/** An adapter for a ItemComparator */
	struct ComparatorAdaptor
	{
		ComparatorAdaptor(const ItemComparator& comparator) : mComparator(comparator) {};

		bool operator()(const item_pair_t* item_pair1, const item_pair_t* item_pair2)
		{
			return mComparator.compare(item_pair1->first, item_pair2->first);
		}

		const ItemComparator& mComparator;
	};


	friend class LLUICtrlFactory;
	LLFlatListView(const LLFlatListView::Params& p);

	/** Manage selection on mouse events */
	void onItemMouseClick(item_pair_t* item_pair, MASK mask);

	void onItemRightMouseClick(item_pair_t* item_pair, MASK mask);

	/**
	 *	Updates position of items.
	 *	It does not take into account invisible items.
	 */
	virtual void rearrangeItems();

	virtual item_pair_t* getItemPair(LLPanel* item) const;

	virtual item_pair_t* getItemPair(const LLSD& value) const;

	virtual bool selectItemPair(item_pair_t* item_pair, bool select);

	virtual bool selectNextItemPair(bool is_up_direction, bool reset_selection);

	virtual BOOL canSelectAll() const;
	virtual void selectAll();

	virtual bool isSelected(item_pair_t* item_pair) const;

	virtual bool removeItemPair(item_pair_t* item_pair, bool rearrange);

	/**
	 * Notify parent about changed size of internal controls with "size_changes" action
	 * 
	 * Size includes Items Rect width and either Items Rect height or comment text height.
	 * Comment text height is included if comment text is set and visible.
	 * List border size is also included into notified size.
	 */
	void notifyParentItemsRectChanged();

	virtual BOOL handleKeyHere(KEY key, MASK mask);

	virtual BOOL postBuild();

	virtual void onFocusReceived();

	virtual void onFocusLost();

	virtual void draw();

	LLRect getLastSelectedItemRect();

	void   ensureSelectedVisible();

private:

	void setItemsNoScrollWidth(S32 new_width) {mItemsNoScrollWidth = new_width - 2 * mBorderThickness;}

	void setNoItemsCommentVisible(bool visible) const;

protected:

	/** Comparator to use when sorting the list. */
	const ItemComparator* mItemComparator;


private:

	LLPanel* mItemsPanel;

	S32 mItemsNoScrollWidth;

	S32 mBorderThickness;

	/** Items padding */
	S32 mItemPad;

	/** Selection support flag */
	bool mAllowSelection;

	/** Multiselection support flag, ignored if selection is not supported */
	bool mMultipleSelection;

	/**
	 * Flag specified whether onCommit be called if selection is changed in the list.
	 * 
	 * Can be ignored in the resetSelection() method.
	 * @see resetSelection()
	 */
	bool mCommitOnSelectionChange;

	bool mKeepOneItemSelected;

	bool mIsConsecutiveSelection;

	bool mKeepSelectionVisibleOnReshape;

	/** All pairs of the list */
	pairs_list_t mItemPairs;

	/** Selected pairs for faster access */
	pairs_list_t mSelectedItemPairs;

	/**
	 * Rectangle contained previous size of items parent notified last time.
	 * Is used to reduce amount of parentNotify() calls if size was not changed.
	 */
	LLRect mPrevNotifyParentRect;

	LLTextBox* mNoItemsCommentTextbox;

	LLViewBorder* mSelectedItemsBorder;

	commit_signal_t	mOnReturnSignal;
};

/**
 * Extends LLFlatListView functionality to show different messages when there are no items in the
 * list depend on whether they are filtered or not.
 *
 * Class provides one message per case of empty list.
 * It also provides protected updateNoItemsMessage() method to be called each time when derived list
 * is changed to update base mNoItemsCommentTextbox value.
 *
 * It is implemented to avoid duplication of this functionality in concrete implementations of the
 * lists. It is intended to be used as a base class for lists which should support two different
 * messages for empty state. Can be improved to support more than two messages via state-to-message map.
 */
class LLFlatListViewEx : public LLFlatListView
{
public:
	LOG_CLASS(LLFlatListViewEx);

	struct Params : public LLInitParam::Block<Params, LLFlatListView::Params>
	{
		/**
		 * Contains a message for empty list when it does not contain any items at all.
		 */
		Optional<std::string>	no_items_msg;

		/**
		 * Contains a message for empty list when its items are removed by filtering.
		 */
		Optional<std::string>	no_filtered_items_msg;
		Params();
	};

	// *WORKAROUND: two methods to overload appropriate Params due to localization issue:
	// no_items_msg & no_filtered_items_msg attributes are not defined as translatable in VLT. See EXT-5931
	void setNoItemsMsg(const std::string& msg) { mNoItemsMsg = msg; }
	void setNoFilteredItemsMsg(const std::string& msg) { mNoFilteredItemsMsg = msg; }

	bool getForceShowingUnmatchedItems();

	void setForceShowingUnmatchedItems(bool show);

	/**
	 * Sets up new filter string and filters the list.
	 */
	void setFilterSubString(const std::string& filter_str);
	
	/**
	 * Filters the list, rearranges and notifies parent about shape changes.
	 * Derived classes may want to overload rearrangeItems() to exclude repeated separators after filtration.
	 */
	void filterItems();

	/**
	 * Returns true if last call of filterItems() found at least one matching item
	 */
	bool hasMatchedItems();

protected:
	LLFlatListViewEx(const Params& p);

	/**
	 * Applies a message for empty list depend on passed argument.
	 *
	 * @param filter_string - if is not empty, message for filtered items will be set, otherwise for
	 * completely empty list. Value of filter string will be passed as search_term in SLURL.
	 */
	void updateNoItemsMessage(const std::string& filter_string);

private:
	std::string mNoFilteredItemsMsg;
	std::string mNoItemsMsg;
	std::string	mFilterSubString;
	/**
	 * Show list items that don't match current filter
	 */
	bool mForceShowingUnmatchedItems;
	/**
	 * True if last call of filterItems() found at least one matching item
	 */
	bool mHasMatchedItems;
};

#endif
