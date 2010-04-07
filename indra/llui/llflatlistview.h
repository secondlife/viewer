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

		/** padding between items */
		Optional<U32> item_pad; 

		/** textbox with info message when list is empty*/
		Optional<LLTextBox::Params> no_items_text;

		Params();
	};
	
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
	virtual bool removeItemByUUID(const LLUUID& uuid);

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
	void setCommitOnSelectionChange(bool b)		{ mCommitOnSelectionChange = b; }

	/** Get number of selected items in the list */
	U32 numSelected() const {return mSelectedItemPairs.size(); }

	/** Get number of items in the list */
	U32 size() const { return mItemPairs.size(); }


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

	virtual bool removeItemPair(item_pair_t* item_pair);

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

#endif
