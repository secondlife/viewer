/** 
 * @file llfavoritesbar.cpp
 * @brief LLFavoritesBarCtrl class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llfavoritesbar.h"

#include "llbutton.h"
#include "llfloaterreg.h"
#include "llinventory.h"
#include "lluictrlfactory.h"
#include "llmenugl.h"

#include "llagent.h"
#include "llinventorybridge.h"
#include "llinventorymodel.h"
#include "llsidetray.h"
#include "lltoggleablemenu.h"
#include "llviewerinventory.h"
#include "llviewermenu.h"
#include "llviewermenu.h"

static LLDefaultChildRegistry::Register<LLFavoritesBarCtrl> r("favorites_bar");

// updateButtons's helper
struct LLFavoritesSort
{
	// Sorting by creation date and name
	// TODO - made it customizible using gSavedSettings
	bool operator()(const LLViewerInventoryItem* const& a, const LLViewerInventoryItem* const& b)
	{
		time_t first_create = a->getCreationDate();
		time_t second_create = b->getCreationDate();
		if (first_create == second_create)
		{
			return (LLStringUtil::compareDict(a->getName(), b->getName()) < 0);
		}
		else
		{
			return (first_create > second_create);
		}
	}
};

LLFavoritesBarCtrl::LLFavoritesBarCtrl(const LLFavoritesBarCtrl::Params& p)
:	LLUICtrl(p),
	mFont(p.font.isProvided() ? p.font() : LLFontGL::getFontSansSerifSmall()),
	mPopupMenuHandle(),
	mInventoryItemsPopupMenuHandle()
{
	// Register callback for menus with current registrar (will be parent panel's registrar)
	LLUICtrl::CommitCallbackRegistry::currentRegistrar().add("Favorites.DoToSelected",
		boost::bind(&LLFavoritesBarCtrl::doToSelected, this, _2));

	// Add this if we need to selectively enable items
	//LLUICtrl::EnableCallbackRegistry::currentRegistrar().add("Favorites.EnableSelected",
	//	boost::bind(&LLFavoritesBarCtrl::enableSelected, this, _2));
	
	gInventory.addObserver(this);
}

LLFavoritesBarCtrl::~LLFavoritesBarCtrl()
{
	gInventory.removeObserver(this);

	LLView::deleteViewByHandle(mPopupMenuHandle);
	LLView::deleteViewByHandle(mInventoryItemsPopupMenuHandle);
}

BOOL LLFavoritesBarCtrl::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg)
{
	*accept = ACCEPT_NO;

	switch (cargo_type)
	{

	case DAD_LANDMARK:
		{
			// Copy the item into the favorites folder (if it's not already there).
			LLInventoryItem *item = (LLInventoryItem *)cargo_data;			
			LLUUID favorites_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_FAVORITE);
			if (item->getParentUUID() == favorites_id)
			{
				llwarns << "Attemt to copy a favorite item into the same folder." << llendl;
				break;
			}

			*accept = ACCEPT_YES_COPY_SINGLE;

			if (drop)
			{
				copy_inventory_item(
						gAgent.getID(),
						item->getPermissions().getOwner(),
						item->getUUID(),
						favorites_id,
						std::string(),
						LLPointer<LLInventoryCallback>(NULL));

				llinfos << "Copied inventory item #" << item->getUUID() << " to favorites." << llendl;
			}
			
		}
		break;
	default:
		break;
	}

	return TRUE;
}

//virtual
void LLFavoritesBarCtrl::changed(U32 mask)
{
	if (mFavoriteFolderId.isNull())
	{
		mFavoriteFolderId = gInventory.findCategoryUUIDForType(LLAssetType::AT_FAVORITE);
		
		if (mFavoriteFolderId.notNull())
		{
			gInventory.fetchDescendentsOf(mFavoriteFolderId);
		}
	}	
	else
	{
		updateButtons(getRect().getWidth());
	}
}

//virtual
void LLFavoritesBarCtrl::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	updateButtons(width);

	LLUICtrl::reshape(width, height, called_from_parent);
}

void LLFavoritesBarCtrl::updateButtons(U32 bar_width)
{
	LLInventoryModel::item_array_t items;

	if (!collectFavoriteItems(items))
	{
		return;
	}

	const S32 buttonHPad = LLUI::sSettingGroups["config"]->getS32("ButtonHPad");
	const S32 buttonHGap = 2;
	const S32 buttonVGap = 2;
	static LLButton::Params default_button_params(LLUICtrlFactory::getDefaultParams<LLButton>());
	std::string flat_icon			= "transparent.j2c";
	std::string hover_icon			= default_button_params.image_unselected.name;
	std::string hover_icon_selected	= default_button_params.image_selected.name;
	
	S32 curr_x = buttonHGap;

	S32 count = items.count();

	const S32 chevron_button_width = mFont->getWidth(">>") + buttonHPad * 2;

	S32 buttons_space = bar_width - curr_x;

	S32 first_drop_down_item = count;

	// Calculating, how much buttons can fit in the bar
	S32 buttons_width = 0;
	for (S32 i = 0; i < count; ++i)
	{
		buttons_width += mFont->getWidth(items.get(i)->getName()) + buttonHPad * 2 + buttonHGap;
		if (buttons_width > buttons_space)
		{
			// There is no space for all buttons.
			// Calculating the number of buttons, that are fit with chevron button
			buttons_space -= chevron_button_width + buttonHGap;
			while (i >= 0 && buttons_width > buttons_space)
			{
				buttons_width -= mFont->getWidth(items.get(i)->getName()) + buttonHPad * 2 + buttonHGap;
				i--;
			}
			first_drop_down_item = i + 1; // First item behind visible items
			
			break;
		}
	}

	bool recreate_buttons = true;

	// If inventory items are not changed up to mFirstDropDownItem, no need to recreate them
	if (mFirstDropDownItem == first_drop_down_item && (mItemNamesCache.size() == count || mItemNamesCache.size() == mFirstDropDownItem))
	{
		S32 i;
		for (i = 0; i < mFirstDropDownItem; ++i)
		{
			if (mItemNamesCache.get(i) != items.get(i)->getName())
			{
				break;
			}
		}
		if (i == mFirstDropDownItem)
		{
			recreate_buttons = false;
		}
	}

	if (recreate_buttons)
	{
		mFirstDropDownItem = first_drop_down_item;

		mItemNamesCache.clear();
		for (S32 i = 0; i < mFirstDropDownItem; i++)
		{
			mItemNamesCache.put(items.get(i)->getName());
		}

		// Rebuild the buttons only
		// child_list_t is a linked list, so safe to erase from the middle if we pre-incrament the iterator
		for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); )
		{
			child_list_const_iter_t cur_it = child_it++;
			LLView* viewp = *cur_it;
			LLButton* button = dynamic_cast<LLButton*>(viewp);
			if (button)
			{
				removeChild(button);
				delete button;
			}
		}

		// Adding buttons
		for(S32 i = mFirstDropDownItem -1; i >= 0; i--)
		{

			LLInventoryItem* item = items.get(i);

			S32 buttonWidth = mFont->getWidth(item->getName()) + buttonHPad * 2;

			LLRect rect;
			rect.setOriginAndSize(curr_x, buttonVGap, buttonWidth, getRect().getHeight()-buttonVGap);

			LLButton::Params bparams;
			bparams.image_unselected.name(flat_icon);
			bparams.image_disabled.name(flat_icon);
			bparams.image_selected.name(hover_icon_selected);
			bparams.image_hover_selected.name(hover_icon_selected);
			bparams.image_disabled_selected.name(hover_icon_selected);
			bparams.image_hover_unselected.name(hover_icon);
			bparams.follows.flags (FOLLOWS_LEFT | FOLLOWS_BOTTOM);
			bparams.rect (rect);
			bparams.tab_stop(false);
			bparams.font(mFont);
			bparams.name(item->getName());
			bparams.tool_tip(item->getName());
			bparams.click_callback.function(boost::bind(&LLFavoritesBarCtrl::onButtonClick, this, item->getUUID()));
			bparams.rightclick_callback.function(boost::bind(&LLFavoritesBarCtrl::onButtonRightClick, this, item->getUUID()));

			addChildInBack(LLUICtrlFactory::create<LLButton> (bparams));

			curr_x += buttonWidth + buttonHGap;
		}
	}

	// Chevron button
	if (mFirstDropDownItem != count)
	{
		// Chevron button should stay right aligned
		LLView *chevron_button = getChildView(std::string(">>"), FALSE, FALSE);
		if (chevron_button)
		{
			LLRect rect;
			rect.setOriginAndSize(bar_width - chevron_button_width - buttonHGap, buttonVGap, chevron_button_width, getRect().getHeight()-buttonVGap);
			chevron_button->setRect(rect);
			chevron_button->setVisible(TRUE);
			mChevronRect = rect;
		}
		else
		{
			LLButton::Params bparams;

			LLRect rect;
			rect.setOriginAndSize(bar_width - chevron_button_width - buttonHGap, buttonVGap, chevron_button_width, getRect().getHeight()-buttonVGap);

			bparams.follows.flags (FOLLOWS_LEFT | FOLLOWS_BOTTOM);
			bparams.image_unselected.name(flat_icon);
			bparams.image_disabled.name(flat_icon);
			bparams.image_selected.name(hover_icon_selected);
			bparams.image_hover_selected.name(hover_icon_selected);
			bparams.image_disabled_selected.name(hover_icon_selected);
			bparams.image_hover_unselected.name(hover_icon);
			bparams.rect (rect);
			bparams.tab_stop(false);
			bparams.font(mFont);
			bparams.name(">>");
			bparams.click_callback.function(boost::bind(&LLFavoritesBarCtrl::showDropDownMenu, this));

			addChildInBack(LLUICtrlFactory::create<LLButton> (bparams));

			mChevronRect = rect;
		}
	}
	else
	{
		// Hide chevron button if all items are visible on bar
		LLView *chevron_button = getChildView(std::string(">>"), FALSE, FALSE);
		if (chevron_button)
		{
			chevron_button->setVisible(FALSE);
		}
	}
}

BOOL LLFavoritesBarCtrl::postBuild()
{
	// make the popup menu available
	LLMenuGL* menu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_favorites.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if (!menu)
	{
		menu = LLUICtrlFactory::getDefaultWidget<LLMenuGL>("inventory_menu");
	}
	menu->setBackgroundColor(LLUIColorTable::instance().getColor("MenuPopupBgColor"));
	mInventoryItemsPopupMenuHandle = menu->getHandle();

	return TRUE;
}

BOOL LLFavoritesBarCtrl::collectFavoriteItems(LLInventoryModel::item_array_t &items)
{
	if (mFavoriteFolderId.isNull())
		return FALSE;
	
	LLInventoryModel::cat_array_t cats;

	LLIsType is_type(LLAssetType::AT_LANDMARK);
	gInventory.collectDescendentsIf(mFavoriteFolderId, cats, items, LLInventoryModel::EXCLUDE_TRASH, is_type);

	std::sort(items.begin(), items.end(), LLFavoritesSort());

	return TRUE;
}

void LLFavoritesBarCtrl::showDropDownMenu()
{
	if (mPopupMenuHandle.isDead())
	{
		LLToggleableMenu::Params menu_p;
		menu_p.name("favorites menu");
		menu_p.can_tear_off(false);
		menu_p.visible(false);
		menu_p.scrollable(true);

		LLToggleableMenu* menu = LLUICtrlFactory::create<LLToggleableMenu>(menu_p);

		mPopupMenuHandle = menu->getHandle();
	}

	LLToggleableMenu* menu = (LLToggleableMenu*)mPopupMenuHandle.get();

	if(menu)
	{
		if (menu->getClosedByButtonClick())
		{
			menu->resetClosedByButtonClick();
			return;
		}

		if (menu->getVisible())
		{
			menu->setVisible(FALSE);
			menu->resetClosedByButtonClick();
			return;
		}

		LLInventoryModel::item_array_t items;

		if (!collectFavoriteItems(items))
		{
			return;
		}

		S32 count = items.count();

		// Check it there are changed items, since last call
		if (mItemNamesCache.size() == count)
		{
			S32 i;
			for (i = mFirstDropDownItem; i < count; i++)
			{
				if (mItemNamesCache.get(i) != items.get(i)->getName())
				{
					break;
				}
			}

			// Check passed, just show the menu
			if (i == count)
			{
				menu->buildDrawLabels();
				menu->updateParent(LLMenuGL::sMenuContainer);

				menu->setButtonRect(mChevronRect, this);

				LLMenuGL::showPopup(this, menu, getRect().getWidth() - menu->getRect().getWidth(), 0);
				return;
			}
		}

		// Add menu items to cache, if there is only names of buttons
		if (mItemNamesCache.size() == mFirstDropDownItem)
		{
			for (S32 i = mFirstDropDownItem; i < count; i++)
			{
				mItemNamesCache.put(items.get(i)->getName());
			}
		}

		menu->empty();

		U32 max_width = 0;

		// Menu will not be wider, than bar
		S32 bar_width = getRect().getWidth();

		for(S32 i = mFirstDropDownItem; i < count; i++)
		{
			LLInventoryItem* item = items.get(i);
			const std::string& item_name = item->getName();

			LLMenuItemCallGL::Params item_params;
			item_params.name(item_name);
			item_params.label(item_name);
			
			item_params.on_click.function(boost::bind(&LLFavoritesBarCtrl::onButtonClick, this, item->getUUID()));
			item_params.rightclick_callback.function(boost::bind(&LLFavoritesBarCtrl::onButtonRightClick, this, item->getUUID()));

			LLMenuItemCallGL *menu_item = LLUICtrlFactory::create<LLMenuItemCallGL>(item_params);

			// Check whether item name wider than menu
			if ((S32) menu_item->getNominalWidth() > bar_width)
			{
				S32 chars_total = item_name.length();
				S32 chars_fitted = 1;
				menu_item->setLabel(LLStringExplicit(""));
				S32 label_space = bar_width - menu_item->getFont()->getWidth("...") -
					menu_item->getNominalWidth(); // This returns width of menu item with empty label (pad pixels)

				while (chars_fitted < chars_total && menu_item->getFont()->getWidth(item_name, 0, chars_fitted) < label_space)
				{
					chars_fitted++;
				}
				chars_fitted--; // Rolling back one char, that doesn't fit

				menu_item->setLabel(item_name.substr(0, chars_fitted) + "...");
			}

			max_width = llmax(max_width, menu_item->getNominalWidth());

			menu->addChild(menu_item);
		}

		// Menu will not be wider, than bar
		max_width = llmin((S32)max_width, bar_width);

		menu->buildDrawLabels();
		menu->updateParent(LLMenuGL::sMenuContainer);

		menu->setButtonRect(mChevronRect, this);

		LLMenuGL::showPopup(this, menu, getRect().getWidth() - max_width, 0);
	}
}

void LLFavoritesBarCtrl::onButtonClick(LLUUID item_id)
{
	LLInventoryModel::item_array_t items;

	if (!collectFavoriteItems(items))
	{
		return;
	}

	// We only have one Inventory, gInventory. Some day this should be better abstracted.
	LLInvFVBridgeAction::doAction(item_id,&gInventory);
}

void LLFavoritesBarCtrl::onButtonRightClick(LLUUID item_id)
{
	mSelectedItemID = item_id;
	
	LLMenuGL* menu = (LLMenuGL*)mInventoryItemsPopupMenuHandle.get();
	if (!menu)
	{
		return;
	}
	
	menu->updateParent(LLMenuGL::sMenuContainer);

	S32 x,y;
	LLUI::getCursorPositionLocal(this, &x, &y);
	LLMenuGL::showPopup(this, menu, x, y);
}

void LLFavoritesBarCtrl::doToSelected(const LLSD& userdata)
{
	std::string action = userdata.asString();
	llinfos << "Action = " << action << " Item = " << mSelectedItemID.asString() << llendl;
	
	LLViewerInventoryItem* item = gInventory.getItem(mSelectedItemID);
	if (!item)
		return;
	
	if (action == "open")
	{
		teleport_via_landmark(item->getAssetUUID());
	}
	else if (action == "about")
	{
		LLSD key;
		key["type"] = "landmark";
		key["id"] = mSelectedItemID;

		LLSideTray::getInstance()->showPanel("panel_places", key);
	}
	else if (action == "rename")
	{
		// Would need to re-implement this:
		// folder->startRenamingSelectedItem();
	}
	else if (action == "delete")
	{
		gInventory.removeItem(mSelectedItemID);
	}
}
