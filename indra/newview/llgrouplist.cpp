/** 
 * @file llgrouplist.cpp
 * @brief List of the groups the agent belongs to.
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

#include "llgrouplist.h"

// libs
#include "llbutton.h"
#include "lliconctrl.h"
#include "lltextbox.h"
#include "lltrans.h"

// newview
#include "llagent.h"
#include "llgroupactions.h"
#include "llviewercontrol.h"	// for gSavedSettings

static LLDefaultChildRegistry::Register<LLGroupList> r("group_list");
S32 LLGroupListItem::sIconWidth = 0;

class LLGroupComparator : public LLFlatListView::ItemComparator
{
public:
	/** Returns true if item1 < item2, false otherwise */
	/*virtual*/ bool compare(const LLPanel* item1, const LLPanel* item2) const
	{
		std::string name1 = static_cast<const LLGroupListItem*>(item1)->getGroupName();
		std::string name2 = static_cast<const LLGroupListItem*>(item2)->getGroupName();

		LLStringUtil::toUpper(name1);
		LLStringUtil::toUpper(name2);

		return name1 < name2;
	}
};

static const LLGroupComparator GROUP_COMPARATOR;

LLGroupList::Params::Params()
{
	
}

LLGroupList::LLGroupList(const Params& p)
:	LLFlatListView(p)
	, mDirty(true) // to force initial update
{
	// Listen for agent group changes.
	gAgent.addListener(this, "new group");

	mShowIcons = gSavedSettings.getBOOL("GroupListShowIcons");
	setCommitOnSelectionChange(true);
	// TODO: implement context menu
	// display a context menu appropriate for a list of group names
//	setContextMenu(LLScrollListCtrl::MENU_GROUP);

	// Set default sort order.
	setComparator(&GROUP_COMPARATOR);
}

LLGroupList::~LLGroupList()
{
	gAgent.removeListener(this);
}

// virtual
void LLGroupList::draw()
{
	if (mDirty)
		refresh();

	LLFlatListView::draw();
}

void LLGroupList::setNameFilter(const std::string& filter)
{
	if (mNameFilter != filter)
	{
		mNameFilter = filter;
		setDirty();
	}
}

static bool findInsensitive(std::string haystack, const std::string& needle_upper)
{
    LLStringUtil::toUpper(haystack);
    return haystack.find(needle_upper) != std::string::npos;
}

void LLGroupList::refresh()
{
	const LLUUID& 		highlight_id	= gAgent.getGroupID();
	S32					count			= gAgent.mGroups.count();
	LLUUID				id;
	bool				have_filter		= !mNameFilter.empty();

	clear();

	for(S32 i = 0; i < count; ++i)
	{
		id = gAgent.mGroups.get(i).mID;
		const LLGroupData& group_data = gAgent.mGroups.get(i);
		if (have_filter && !findInsensitive(group_data.mName, mNameFilter))
			continue;
		addNewItem(id, group_data.mName, group_data.mInsigniaID, highlight_id == id, ADD_BOTTOM);
	}

	// Sort the list.
	sort();

	// add "none" to list at top
	{
		std::string loc_none = LLTrans::getString("GroupsNone");
		if (have_filter || findInsensitive(loc_none, mNameFilter))
			addNewItem(LLUUID::null, loc_none, LLUUID::null, highlight_id.isNull(), ADD_TOP);
	}

	selectItemByUUID(highlight_id);

	setDirty(false);
	onCommit();
}

void LLGroupList::toggleIcons()
{
	// Save the new value for new items to use.
	mShowIcons = !mShowIcons;
	gSavedSettings.setBOOL("GroupListShowIcons", mShowIcons);

	// Show/hide icons for all existing items.
	std::vector<LLPanel*> items;
	getItems(items);
	for( std::vector<LLPanel*>::const_iterator it = items.begin(); it != items.end(); it++)
	{
		static_cast<LLGroupListItem*>(*it)->setGroupIconVisible(mShowIcons);
	}
}

//////////////////////////////////////////////////////////////////////////
// PRIVATE Section
//////////////////////////////////////////////////////////////////////////

void LLGroupList::addNewItem(const LLUUID& id, const std::string& name, const LLUUID& icon_id, BOOL is_bold, EAddPosition pos)
{
	LLGroupListItem* item = new LLGroupListItem();

	item->setName(name);
	item->setGroupID(id);
	item->setGroupIconID(icon_id);
//	item->setContextMenu(mContextMenu);

	item->childSetVisible("info_btn", false);
	item->setGroupIconVisible(mShowIcons);

	addItem(item, id, pos);

//	setCommentVisible(false);
}

// virtual
bool LLGroupList::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	// Why is "new group" sufficient?
	if (event->desc() == "new group")
	{
		setDirty();
		return true;
	}

	return false;
}

/************************************************************************/
/*          LLGroupListItem implementation                              */
/************************************************************************/

LLGroupListItem::LLGroupListItem()
:	LLPanel(),
mGroupIcon(NULL),
mGroupNameBox(NULL),
mInfoBtn(NULL),
//mContextMenu(NULL), //TODO:
mGroupID(LLUUID::null)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_group_list_item.xml");

	// Remember group icon width including its padding from the name text box,
	// so that we can hide and show the icon again later.
	if (!sIconWidth)
	{
		sIconWidth = mGroupNameBox->getRect().mLeft - mGroupIcon->getRect().mLeft;
	}
}

//virtual
BOOL  LLGroupListItem::postBuild()
{
	mGroupIcon = getChild<LLIconCtrl>("group_icon");
	mGroupNameBox = getChild<LLTextBox>("group_name");

	mInfoBtn = getChild<LLButton>("info_btn");
	mInfoBtn->setClickedCallback(boost::bind(&LLGroupListItem::onInfoBtnClick, this));

	return TRUE;
}

//virtual
void LLGroupListItem::setValue( const LLSD& value )
{
	if (!value.isMap()) return;
	if (!value.has("selected")) return;
	childSetVisible("selected_icon", value["selected"]);
}

void LLGroupListItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	childSetVisible("hovered_icon", true);
	if (mGroupID.notNull()) // don't show the info button for the "none" group
		mInfoBtn->setVisible(true);

	LLPanel::onMouseEnter(x, y, mask);
}

void LLGroupListItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	childSetVisible("hovered_icon", false);
	mInfoBtn->setVisible(false);

	LLPanel::onMouseLeave(x, y, mask);
}

void LLGroupListItem::setName(const std::string& name)
{
	mGroupName = name;
	mGroupNameBox->setValue(name);
	mGroupNameBox->setToolTip(name);
}

void LLGroupListItem::setGroupID(const LLUUID& group_id)
{
	mGroupID = group_id;
	setActive(group_id == gAgent.getGroupID());
}

void LLGroupListItem::setGroupIconID(const LLUUID& group_icon_id)
{
	if (group_icon_id.notNull())
	{
		mGroupIcon->setValue(group_icon_id);
	}
}

void LLGroupListItem::setGroupIconVisible(bool visible)
{
	// Already done? Then do nothing.
	if (mGroupIcon->getVisible() == (BOOL)visible)
		return;

	// Show/hide the group icon.
	mGroupIcon->setVisible(visible);

	// Move the group name horizontally by icon size + its distance from the group name.
	LLRect name_rect = mGroupNameBox->getRect();
	name_rect.mLeft += visible ? sIconWidth : -sIconWidth;
	mGroupNameBox->setRect(name_rect);
}

//////////////////////////////////////////////////////////////////////////
// Private Section
//////////////////////////////////////////////////////////////////////////
void LLGroupListItem::setActive(bool active)
{
	// Active group should be bold.
	LLFontDescriptor new_desc(mGroupNameBox->getDefaultFont()->getFontDesc());

	// *NOTE dzaporozhan
	// On Windows LLFontGL::NORMAL will not remove LLFontGL::BOLD if font 
	// is predefined as bold (SansSerifSmallBold, for example)
	new_desc.setStyle(active ? LLFontGL::BOLD : LLFontGL::NORMAL);
	LLFontGL* new_font = LLFontGL::getFont(new_desc);
	LLStyle::Params style_params;
	style_params.font = new_font;

	// *NOTE: You cannot set the style on a text box anymore, you must
	// rebuild the text.  This will cause problems if the text contains
	// hyperlinks, as their styles will be wrong.
	std::string text = mGroupNameBox->getText();
	mGroupNameBox->clear();
	mGroupNameBox->appendText(text, false, style_params);
}

void LLGroupListItem::onInfoBtnClick()
{
	LLGroupActions::show(mGroupID);
}
//EOF
