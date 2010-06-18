/**
 * @file llinventorylistitem.h
 * @brief Inventory list item panel.
 *
 * Class LLPanelInventoryListItemBase displays inventory item as an element
 * of LLInventoryItemsList.
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

#ifndef LL_LLINVENTORYLISTITEM_H
#define LL_LLINVENTORYLISTITEM_H

// llcommon
#include "llassettype.h"

// llui
#include "llpanel.h"
#include "llstyle.h"

// newview
#include "llwearabletype.h"

class LLIconCtrl;
class LLTextBox;
class LLViewerInventoryItem;

/**
 * @class LLPanelInventoryListItemBase
 *
 * Base class for Inventory flat list item. Panel consists of inventory icon
 * and inventory item name.
 * This class is able to display widgets(buttons) on left(before icon) and right(after text-box) sides 
 * of panel.
 *
 * How to use (see LLPanelClothingListItem for example):
 * - implement init() to build panel from xml
 * - create new xml file, fill it with widgets you want to dynamically show/hide/reshape on left/right sides
 * - redefine postBuild()(call base implementation) and add needed widgets to needed sides,
 *
 */
class LLPanelInventoryListItemBase : public LLPanel
{
public:
	struct Params : public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<LLStyle::Params>	default_style,
									worn_style;
		Params();
	};

	typedef enum e_item_state {
		IS_DEFAULT,
		IS_WORN,
	} EItemState;

	static LLPanelInventoryListItemBase* create(LLViewerInventoryItem* item);

	virtual void draw();

	/**
	 * Let item know it need to be refreshed in next draw()
	 */
	void setNeedsRefresh(bool needs_refresh){ mNeedsRefresh = needs_refresh; }

	bool getNeedsRefresh(){ return mNeedsRefresh; }

	/**
	 * Add widget to left side
	 */
	void addWidgetToLeftSide(const std::string& name, bool show_widget = true);
	void addWidgetToLeftSide(LLUICtrl* ctrl, bool show_widget = true);

	/**
	 * Add widget to right side, widget is supposed to be child of calling panel
	 */
	void addWidgetToRightSide(const std::string& name, bool show_widget = true);
	void addWidgetToRightSide(LLUICtrl* ctrl, bool show_widget = true);

	/**
	 * Mark widgets as visible. Only visible widgets take part in reshaping children
	 */
	void setShowWidget(const std::string& name, bool show);
	void setShowWidget(LLUICtrl* ctrl, bool show);

	/**
	 * Set spacing between widgets during reshape
	 */
	void setWidgetSpacing(S32 spacing) { mWidgetSpacing = spacing; }

	S32 getWidgetSpacing() { return mWidgetSpacing; }

	/**
	 * Inheritors need to call base implementation of postBuild()
	 */
	/*virtual*/ BOOL postBuild();

	/**
	 * Handles item selection
	 */
	/*virtual*/ void setValue(const LLSD& value);

	/**
	 * Handles filter request
	 */
	/*virtual*/ S32  notify(const LLSD& info);

	 /* Highlights item */
	/*virtual*/ void onMouseEnter(S32 x, S32 y, MASK mask);
	/* Removes item highlight */
	/*virtual*/ void onMouseLeave(S32 x, S32 y, MASK mask);

	/** Get the name of a corresponding inventory item */
	const std::string& getItemName() const;

	/** Get the asset type of a corresponding inventory item */
	LLAssetType::EType getType() const;

	/** Get the wearable type of a corresponding inventory item */
	LLWearableType::EType getWearableType() const;

	/** Get the description of a corresponding inventory item */
	const std::string& getDescription() const;

	/** Get the associated inventory item */
	LLViewerInventoryItem* getItem() const { return mItem; }

	virtual ~LLPanelInventoryListItemBase(){}

protected:

	LLPanelInventoryListItemBase(LLViewerInventoryItem* item);

	typedef std::vector<LLUICtrl*> widget_array_t;

	/**
	 * Use it from a factory function to build panel, do not build panel in constructor
	 */
	virtual void init();

	/**
	 * Called after inventory item was updated, update panel widgets to reflect inventory changes.
	 */
	virtual void updateItem(const std::string& name,
							EItemState item_state = IS_DEFAULT);

	/** setter for mIconCtrl */
	void setIconCtrl(LLIconCtrl* icon) { mIconCtrl = icon; }
	/** setter for MTitleCtrl */
	void setTitleCtrl(LLTextBox* tb) { mTitleCtrl = tb; }

	void setLeftWidgetsWidth(S32 width) { mLeftWidgetsWidth = width; }
	void setRightWidgetsWidth(S32 width) { mRightWidgetsWidth = width; }

	/**
	 * Set all widgets from both side visible/invisible. Only enabled widgets
	 * (see setShowWidget()) can become visible
	 */
	virtual void setWidgetsVisible(bool visible);

	/**
	 * Reshape all child widgets - icon, text-box and side widgets
	 */
	virtual void reshapeWidgets();

	/** set wearable type icon image */
	void setIconImage(const LLUIImagePtr& image);

	/** Set item title - inventory item name usually */
	void setTitle(const std::string& title,
				  const std::string& highlit_text,
				  EItemState item_state = IS_DEFAULT);

	/**
	 * Show tool tip if item name text size > panel size
	 */
	virtual BOOL handleToolTip( S32 x, S32 y, MASK mask);

	LLViewerInventoryItem* mItem;

private:

	/** reshape left side widgets
	 * Deprecated for now. Disabled reshape left for now to reserve space for 'delete' 
	 * button in LLPanelClothingListItem according to Neal's comment (https://codereview.productengine.com/secondlife/r/325/)
	 */
	void reshapeLeftWidgets();

	/** reshape right side widgets */
	void reshapeRightWidgets();

	/** reshape remaining widgets */
	void reshapeMiddleWidgets();


	LLIconCtrl*		mIconCtrl;
	LLTextBox*		mTitleCtrl;

	LLUIImagePtr	mIconImage;
	std::string		mHighlightedText;

	widget_array_t	mLeftSideWidgets;
	widget_array_t	mRightSideWidgets;
	S32				mWidgetSpacing;

	S32				mLeftWidgetsWidth;
	S32				mRightWidgetsWidth;
	bool			mNeedsRefresh;
};

#endif //LL_LLINVENTORYLISTITEM_H
