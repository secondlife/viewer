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
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llviewerinventory.h"
#include "llinventory.h"
#include "llinventorymodel.h"
#include "llbutton.h"
#include "llinventorybridge.h"

static LLRegisterWidget<LLFavoritesBarCtrl> r("favorites_bar");

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
	mFont(p.font.isProvided() ? p.font() : LLFontGL::getFontSansSerifSmall())
{
	gInventory.addObserver(this);
}

LLFavoritesBarCtrl::~LLFavoritesBarCtrl()
{
	gInventory.removeObserver(this);
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

		// IAN BUG: did the spec ask for calling cards here?
	case DAD_LANDMARK:
	case DAD_CALLINGCARD:
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
	LLInventoryModel::item_array_t items;

	if (!collectFavoriteItems(items))
	{
		return;
	}

	S32 count = items.count();
	if (getChildCount() == count)
	{
		// Check whether buttons are reflecting state of favorite inventory folder
		const LLView::child_list_t *buttons_list = getChildList();
		S32 i = 0;
		for(LLView::child_list_const_iter_t iter = buttons_list->begin(); iter != buttons_list->end(); iter++)
		{
			LLButton *button = (LLButton *)*iter;
			LLInventoryItem* item = items.get(i);
			// tooltip contains full name of item, label can be cutted
			if (button->getToolTip() != item->getName())
			{
				break;
			}


			i++;
		}

		// Check passed, nothing is changed
		if (i < items.count())
		{
			return;
		}
	}

	//FIXME: optimize this
	deleteAllChildren();

	LLButton::Params bparams;

	const S32 buttonWidth = LLUI::sSettingGroups["config"]->getS32("ButtonHPad") * 2;
	LLRect rect;

	for(S32 i = 0; i < count; ++i)
	{
		rect.setOriginAndSize(0, 0, buttonWidth, llround(mFont->getLineHeight()));

		bparams.follows.flags (FOLLOWS_LEFT | FOLLOWS_BOTTOM);
		bparams.rect (rect);
		bparams.tab_stop(false);
		bparams.font(mFont);
		bparams.click_callback.function(boost::bind(&LLFavoritesBarCtrl::onButtonClick, this, 0));

		LLButton *button = LLUICtrlFactory::create<LLButton> (bparams);

		addChildInBack(button);
	}

	updateButtons(getRect().getWidth(), items);
}

//virtual
void LLFavoritesBarCtrl::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLInventoryModel::item_array_t items;
	if (collectFavoriteItems(items))
	{
		updateButtons(width, items);
	}

	LLUICtrl::reshape(width, height, called_from_parent);
}

void LLFavoritesBarCtrl::updateButtons(U32 barWidth, LLInventoryModel::item_array_t items)
{
	std::sort(items.begin(), items.end(), LLFavoritesSort());

	const S32 buttonHPad = LLUI::sSettingGroups["config"]->getS32("ButtonHPad");
	const S32 buttonHGap = 2;

	S32 curr_x = buttonHGap;

	S32 count = items.count();
	S32 labelsWidth = 0;

	for (S32 i = 0; i < count; ++i)
	{
		labelsWidth += mFont->getWidth(items.get(i)->getName());
	}

	const S32 ellipsisWIdth = mFont->getWidth("...");

	F32 shrinkFactor = 1.0f;

	S32 labelsSpace = barWidth - buttonHGap * (count + 1) - buttonHPad * 2 * count; // There is leading buttonHGap in front of first button,
												    // one buttonHGap at the end of each button and 2 buttonHPad's
	if (labelsWidth >= labelsSpace)
	{
		shrinkFactor = (float)labelsSpace / labelsWidth;
	}

	const LLView::child_list_t *buttons_list = getChildList();

	S32 i = 0;
	for(LLView::child_list_const_iter_t iter = buttons_list->begin(); iter != buttons_list->end(); iter++)
	{
		LLButton *button = (LLButton *)*iter;

		LLInventoryItem* item = items.get(i);

		LLRect rect;

		S32 labelWidth = mFont->getWidth(item->getName());

		if (shrinkFactor < 1.0f)
		{
			labelWidth = (S32) ((float)labelWidth * shrinkFactor);

			if (labelWidth > ellipsisWIdth)
			{
				labelWidth -= ellipsisWIdth;

				S32 charsTotal = item->getName().length();
				S32 charsFitted = 1;
				while (charsFitted < charsTotal && mFont->getWidth(item->getName(), 0, charsFitted) < labelWidth)
				{
					charsFitted++;
				}

				charsFitted--;
				labelWidth += ellipsisWIdth;

				button->setLabel(item->getName().substr(0, charsFitted) + "...");
			}
			else
			{
				button->setLabel((LLStringExplicit)"");
				labelWidth = 0;
			}
		}
		else
		{
			button->setLabel(item->getName());
		}

		S32 buttonWidth = labelWidth + buttonHPad * 2;
		rect.setOriginAndSize(curr_x, 2, buttonWidth, llround(mFont->getLineHeight()));
		button->setRect(rect);

		button->setToolTip(item->getName());

		curr_x += buttonWidth + buttonHGap;

		i++;
	}
}

BOOL LLFavoritesBarCtrl::collectFavoriteItems(LLInventoryModel::item_array_t &items)
{
	LLUUID favorite_folder_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_FAVORITE);

	if (!favorite_folder_id.notNull())
		return FALSE;
	
	LLInventoryModel::cat_array_t cats;

	gInventory.collectDescendents(favorite_folder_id, cats, items, LLInventoryModel::EXCLUDE_TRASH);

	return TRUE;
}

void LLFavoritesBarCtrl::onButtonClick(int idx)
{
	LLInventoryModel::item_array_t items;

	if (!collectFavoriteItems(items))
	{
		return;
	}

	S32 count = items.count();

	if (idx < 0 || idx >= count)
	{
		llwarns << "Invalid favorites bar index" << llendl;
		return;
	}

	LLInventoryItem* item = items.get(idx);
	if(item)
	{
		LLUUID item_id = item->getUUID();
		LLAssetType::EType item_type = item->getType();

		//TODO - donno but may be we must use InventoryModel from InventoryView
		//think for now there is only one model...but still
		LLInvFVBridgeAction::doAction(item_type,item_id,&gInventory);
	}
}

