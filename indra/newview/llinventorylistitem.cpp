/**
 * @file llinventorylistitem.cpp
 * @brief Inventory list item panel.
 *
 * Class LLPanelInventoryListItemBase displays inventory item as an element
 * of LLInventoryItemsList.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llinventorylistitem.h"

// llui
#include "lliconctrl.h"
#include "lltextbox.h"
#include "lltextutil.h"

// newview
#include "llinventoryicon.h"
#include "llinventorymodel.h"
#include "llviewerinventory.h"

static LLWidgetNameRegistry::StaticRegistrar sRegisterPanelInventoryListItemBaseParams(&typeid(LLPanelInventoryListItemBase::Params), "inventory_list_item");

static const S32 WIDGET_SPACING = 3;

LLPanelInventoryListItemBase::Params::Params()
:	default_style("default_style"),
	worn_style("worn_style"),
	hover_image("hover_image"),
	selected_image("selected_image"),
	separator_image("separator_image"),
	item_icon("item_icon"),
	item_name("item_name")
{};

LLPanelInventoryListItemBase* LLPanelInventoryListItemBase::create(LLViewerInventoryItem* item)
{
	LLPanelInventoryListItemBase* list_item = NULL;
	if (item)
	{
		const LLPanelInventoryListItemBase::Params& params = LLUICtrlFactory::getDefaultParams<LLPanelInventoryListItemBase>();
		list_item = new LLPanelInventoryListItemBase(item, params);
		list_item->initFromParams(params);
		list_item->postBuild();
	}
	return list_item;
}

void LLPanelInventoryListItemBase::draw()
{
	if (getNeedsRefresh())
	{
		LLViewerInventoryItem* inv_item = getItem();
		if (inv_item)
		{
			updateItem(inv_item->getName());
		}
		setNeedsRefresh(false);
	}

	if (mHovered && mHoverImage)
	{
		mHoverImage->draw(getLocalRect());
	}

	if (mSelected && mSelectedImage)
	{
		mSelectedImage->draw(getLocalRect());
	}

	if (mSeparatorVisible && mSeparatorImage)
	{
		// place under bottom of listitem, using image height
		// item_pad in list using the item should be >= image height
		// to avoid cropping of top of the next item.
		LLRect separator_rect = getLocalRect();
		separator_rect.mTop = separator_rect.mBottom;
		separator_rect.mBottom -= mSeparatorImage->getHeight();
		F32 alpha = getCurrentTransparency();
		mSeparatorImage->draw(separator_rect, UI_VERTEX_COLOR % alpha);
	}
	
	LLPanel::draw();
}

// virtual
void LLPanelInventoryListItemBase::updateItem(const std::string& name,
											  EItemState item_state)
{
	setIconImage(mIconImage);
	setTitle(name, mHighlightedText, item_state);
}

void LLPanelInventoryListItemBase::addWidgetToLeftSide(const std::string& name, bool show_widget/* = true*/)
{
	LLUICtrl* ctrl = findChild<LLUICtrl>(name);
	if(ctrl)
	{
		addWidgetToLeftSide(ctrl, show_widget);
	}
}

void LLPanelInventoryListItemBase::addWidgetToLeftSide(LLUICtrl* ctrl, bool show_widget/* = true*/)
{
	mLeftSideWidgets.push_back(ctrl);
	setShowWidget(ctrl, show_widget);
}

void LLPanelInventoryListItemBase::addWidgetToRightSide(const std::string& name, bool show_widget/* = true*/)
{
	LLUICtrl* ctrl = findChild<LLUICtrl>(name);
	if(ctrl)
	{
		addWidgetToRightSide(ctrl, show_widget);
	}
}

void LLPanelInventoryListItemBase::addWidgetToRightSide(LLUICtrl* ctrl, bool show_widget/* = true*/)
{
	mRightSideWidgets.push_back(ctrl);
	setShowWidget(ctrl, show_widget);
}

void LLPanelInventoryListItemBase::setShowWidget(const std::string& name, bool show)
{
	LLUICtrl* widget = findChild<LLUICtrl>(name);
	if(widget)
	{
		setShowWidget(widget, show);
	}
}

void LLPanelInventoryListItemBase::setShowWidget(LLUICtrl* ctrl, bool show)
{
	// Enable state determines whether widget may become visible in setWidgetsVisible()
	ctrl->setEnabled(show);
}

BOOL LLPanelInventoryListItemBase::postBuild()
{
	LLViewerInventoryItem* inv_item = getItem();
	if (inv_item)
	{
		mIconImage = LLInventoryIcon::getIcon(inv_item->getType(), inv_item->getInventoryType(), inv_item->getFlags(), FALSE);
		updateItem(inv_item->getName());
	}

	setNeedsRefresh(true);

	setWidgetsVisible(false);
	reshapeWidgets();

	return TRUE;
}

void LLPanelInventoryListItemBase::setValue(const LLSD& value)
{
	if (!value.isMap()) return;
	if (!value.has("selected")) return;
	mSelected = value["selected"];
}

void LLPanelInventoryListItemBase::onMouseEnter(S32 x, S32 y, MASK mask)
{
	mHovered = true;
	LLPanel::onMouseEnter(x, y, mask);
}

void LLPanelInventoryListItemBase::onMouseLeave(S32 x, S32 y, MASK mask)
{
	mHovered = false;
	LLPanel::onMouseLeave(x, y, mask);
}

const std::string& LLPanelInventoryListItemBase::getItemName() const
{
	LLViewerInventoryItem* inv_item = getItem();
	if (NULL == inv_item)
	{
		return LLStringUtil::null;
	}
	return inv_item->getName();
}

LLAssetType::EType LLPanelInventoryListItemBase::getType() const
{
	LLViewerInventoryItem* inv_item = getItem();
	if (NULL == inv_item)
	{
		return LLAssetType::AT_NONE;
	}
	return inv_item->getType();
}

LLWearableType::EType LLPanelInventoryListItemBase::getWearableType() const
{
	LLViewerInventoryItem* inv_item = getItem();
	if (NULL == inv_item)
	{
		return LLWearableType::WT_NONE;
	}
	return inv_item->getWearableType();
}

const std::string& LLPanelInventoryListItemBase::getDescription() const
{
	LLViewerInventoryItem* inv_item = getItem();
	if (NULL == inv_item)
	{
		return LLStringUtil::null;
	}
	return inv_item->getDescription();
}

time_t LLPanelInventoryListItemBase::getCreationDate() const
{
	LLViewerInventoryItem* inv_item = getItem();
	if (NULL == inv_item)
	{
		return 0;
	}

	return inv_item->getCreationDate();
}

LLViewerInventoryItem* LLPanelInventoryListItemBase::getItem() const
{
	return gInventory.getItem(mInventoryItemUUID);
}

S32 LLPanelInventoryListItemBase::notify(const LLSD& info)
{
	S32 rv = 0;
	if(info.has("match_filter"))
	{
		mHighlightedText = info["match_filter"].asString();

		std::string test(mTitleCtrl->getText());
		LLStringUtil::toUpper(test);

		if(mHighlightedText.empty() || std::string::npos != test.find(mHighlightedText))
		{
			rv = 0; // substring is found
		}
		else
		{
			rv = -1;
		}

		setNeedsRefresh(true);
	}
	else
	{
		rv = LLPanel::notify(info);
	}
	return rv;
}

LLPanelInventoryListItemBase::LLPanelInventoryListItemBase(LLViewerInventoryItem* item, const LLPanelInventoryListItemBase::Params& params)
:	LLPanel(params),
	mInventoryItemUUID(item ? item->getUUID() : LLUUID::null),
	mIconCtrl(NULL),
	mTitleCtrl(NULL),
	mWidgetSpacing(WIDGET_SPACING),
	mLeftWidgetsWidth(0),
	mRightWidgetsWidth(0),
	mNeedsRefresh(false),
	mHovered(false),
	mSelected(false),
	mSeparatorVisible(false),
	mHoverImage(params.hover_image),
	mSelectedImage(params.selected_image),
	mSeparatorImage(params.separator_image)
{
	LLIconCtrl::Params icon_params(params.item_icon);
	applyXUILayout(icon_params, this);

	mIconCtrl = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
	if (mIconCtrl)
	{
		addChild(mIconCtrl);
	}
	else
	{
		LLIconCtrl::Params icon_params;
		icon_params.name = "item_icon";
		mIconCtrl = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
	}

	LLTextBox::Params text_params(params.item_name);
	applyXUILayout(text_params, this);

	mTitleCtrl = LLUICtrlFactory::create<LLTextBox>(text_params);
	if (mTitleCtrl)
	{
		addChild(mTitleCtrl);
	}
	else
	{
		LLTextBox::Params text_aprams;
		text_params.name = "item_title";
		mTitleCtrl = LLUICtrlFactory::create<LLTextBox>(text_params);
	}
}

class WidgetVisibilityChanger
{
public:
	WidgetVisibilityChanger(bool visible) : mVisible(visible){}
	void operator()(LLUICtrl* widget)
	{
		// Disabled widgets never become visible. see LLPanelInventoryListItemBase::setShowWidget()
		widget->setVisible(mVisible && widget->getEnabled());
	}
private:
	bool mVisible;
};

void LLPanelInventoryListItemBase::setWidgetsVisible(bool visible)
{
	std::for_each(mLeftSideWidgets.begin(), mLeftSideWidgets.end(), WidgetVisibilityChanger(visible));
	std::for_each(mRightSideWidgets.begin(), mRightSideWidgets.end(), WidgetVisibilityChanger(visible));
}

void LLPanelInventoryListItemBase::reshapeWidgets()
{
	// disabled reshape left for now to reserve space for 'delete' button in LLPanelClothingListItem
	/*reshapeLeftWidgets();*/
	reshapeRightWidgets();
	reshapeMiddleWidgets();
}

void LLPanelInventoryListItemBase::setIconImage(const LLUIImagePtr& image)
{
	if(image)
	{
		mIconImage = image; 
		mIconCtrl->setImage(mIconImage);
	}
}

void LLPanelInventoryListItemBase::setTitle(const std::string& title,
											const std::string& highlit_text,
											EItemState item_state)
{
	mTitleCtrl->setToolTip(title);

	LLStyle::Params style_params;

	const LLPanelInventoryListItemBase::Params& params = LLUICtrlFactory::getDefaultParams<LLPanelInventoryListItemBase>();

	switch(item_state)
	{
	case IS_DEFAULT:
		style_params = params.default_style();
		break;
	case IS_WORN:
		style_params = params.worn_style();
		break;
	default:;
	}

	LLTextUtil::textboxSetHighlightedVal(
		mTitleCtrl,
		style_params,
		title,
		highlit_text);
}

BOOL LLPanelInventoryListItemBase::handleToolTip( S32 x, S32 y, MASK mask)
{
	LLRect text_box_rect = mTitleCtrl->getRect();
	if (text_box_rect.pointInRect(x, y) &&
		mTitleCtrl->getTextPixelWidth() <= text_box_rect.getWidth())
	{
		return FALSE;
	}
	return LLPanel::handleToolTip(x, y, mask);
}

void LLPanelInventoryListItemBase::reshapeLeftWidgets()
{
	S32 widget_left = 0;
	mLeftWidgetsWidth = 0;

	widget_array_t::const_iterator it = mLeftSideWidgets.begin();
	const widget_array_t::const_iterator it_end = mLeftSideWidgets.end();
	for( ; it_end != it; ++it)
	{
		LLUICtrl* widget = *it;
		if(!widget->getVisible())
		{
			continue;
		}
		LLRect widget_rect(widget->getRect());
		widget_rect.setLeftTopAndSize(widget_left, widget_rect.mTop, widget_rect.getWidth(), widget_rect.getHeight());
		widget->setShape(widget_rect);

		widget_left += widget_rect.getWidth() + getWidgetSpacing();
		mLeftWidgetsWidth = widget_rect.mRight;
	}
}

void LLPanelInventoryListItemBase::reshapeRightWidgets()
{
	S32 widget_right = getLocalRect().getWidth();
	S32 widget_left = widget_right;

	widget_array_t::const_reverse_iterator it = mRightSideWidgets.rbegin();
	const widget_array_t::const_reverse_iterator it_end = mRightSideWidgets.rend();
	for( ; it_end != it; ++it)
	{
		LLUICtrl* widget = *it;
		if(!widget->getVisible())
		{
			continue;
		}
		LLRect widget_rect(widget->getRect());
		widget_left = widget_right - widget_rect.getWidth();
		widget_rect.setLeftTopAndSize(widget_left, widget_rect.mTop, widget_rect.getWidth(), widget_rect.getHeight());
		widget->setShape(widget_rect);

		widget_right = widget_left - getWidgetSpacing();
	}
	mRightWidgetsWidth = getLocalRect().getWidth() - widget_left;
}

void LLPanelInventoryListItemBase::reshapeMiddleWidgets()
{
	LLRect icon_rect(mIconCtrl->getRect());
	icon_rect.setLeftTopAndSize(mLeftWidgetsWidth + getWidgetSpacing(), icon_rect.mTop, 
		icon_rect.getWidth(), icon_rect.getHeight());
	mIconCtrl->setShape(icon_rect);

	S32 name_left = icon_rect.mRight + getWidgetSpacing();
	S32 name_right = getLocalRect().getWidth() - mRightWidgetsWidth - getWidgetSpacing();
	LLRect name_rect(mTitleCtrl->getRect());
	name_rect.set(name_left, name_rect.mTop, name_right, name_rect.mBottom);
	mTitleCtrl->setShape(name_rect);
}

// EOF
