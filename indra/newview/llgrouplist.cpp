/**
 * @file llgrouplist.cpp
 * @brief List of the groups the agent belongs to.
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

#include "llviewerprecompiledheaders.h"

#include "llgrouplist.h"

// libs
#include "llbutton.h"
#include "llgroupiconctrl.h"
#include "llmenugl.h"
#include "lltextbox.h"
#include "lltextutil.h"
#include "lltrans.h"

// newview
#include "llagent.h"
#include "llgroupactions.h"
#include "llfloaterreg.h"
#include "llviewercontrol.h"    // for gSavedSettings
#include "llviewermenu.h"       // for gMenuHolder
#include "llvoiceclient.h"

static LLDefaultChildRegistry::Register<LLGroupList> r("group_list");

class LLGroupComparator : public LLFlatListView::ItemComparator
{
public:
    LLGroupComparator() {};

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

class LLSharedGroupComparator : public LLFlatListView::ItemComparator
{
public:
    LLSharedGroupComparator() {};

    /*virtual*/ bool compare(const LLPanel* item1, const LLPanel* item2) const
    {
        const LLGroupListItem* group_item1 = static_cast<const LLGroupListItem*>(item1);
        std::string name1 = group_item1->getGroupName();
        bool item1_shared = gAgent.isInGroup(group_item1->getGroupID(), true);

        const LLGroupListItem* group_item2 = static_cast<const LLGroupListItem*>(item2);
        std::string name2 = group_item2->getGroupName();
        bool item2_shared = gAgent.isInGroup(group_item2->getGroupID(), true);

        if (item2_shared != item1_shared)
        {
            return item1_shared;
        }

        LLStringUtil::toUpper(name1);
        LLStringUtil::toUpper(name2);

        return name1 < name2;
    }
};

static LLGroupComparator GROUP_COMPARATOR;
static LLSharedGroupComparator SHARED_GROUP_COMPARATOR;

LLGroupList::Params::Params()
: for_agent("for_agent", true)
{
}

LLGroupList::LLGroupList(const Params& p)
:   LLFlatListViewEx(p)
    , mForAgent(p.for_agent)
    , mDirty(true) // to force initial update
    , mShowIcons(false)
    , mShowNone(true)
{
    setCommitOnSelectionChange(true);

    // Set default sort order.
    if (mForAgent)
    {
        setComparator(&GROUP_COMPARATOR);
    }
    else
    {
        // shared groups first
        setComparator(&SHARED_GROUP_COMPARATOR);
    }

    if (mForAgent)
    {
        enableForAgent(true);
    }
}

LLGroupList::~LLGroupList()
{
    if (mForAgent) gAgent.removeListener(this);
    if (mContextMenuHandle.get()) mContextMenuHandle.get()->die();
}

void LLGroupList::enableForAgent(bool show_icons)
{
    mForAgent = true;

    mShowIcons = mForAgent && gSavedSettings.getBOOL("GroupListShowIcons") && show_icons;

    // Listen for agent group changes.
    gAgent.addListener(this, "new group");

    // Set up context menu.
    ScopedRegistrarHelper registrar;
    LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

    registrar.add("People.Groups.Action",           boost::bind(&LLGroupList::onContextMenuItemClick,   this, _2));
    enable_registrar.add("People.Groups.Enable",    boost::bind(&LLGroupList::onContextMenuItemEnable,  this, _2));

    LLToggleableMenu* context_menu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_people_groups.xml",
            gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
    if(context_menu)
        mContextMenuHandle = context_menu->getHandle();
}

// virtual
void LLGroupList::draw()
{
    if (mDirty)
        refresh();

    LLFlatListView::draw();
}

// virtual
bool LLGroupList::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    bool handled = LLUICtrl::handleRightMouseDown(x, y, mask);

    if (mForAgent)
    {
        LLToggleableMenu* context_menu = mContextMenuHandle.get();
        if (context_menu && size() > 0)
        {
            context_menu->buildDrawLabels();
            context_menu->updateParent(LLMenuGL::sMenuContainer);
            LLMenuGL::showPopup(this, context_menu, x, y);
        }
    }

    return handled;
}

// virtual
bool LLGroupList::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    bool handled = LLView::handleDoubleClick(x, y, mask);
    // Handle double click only for the selected item in the list, skip clicks on empty space.
    if (handled)
    {
        if (mDoubleClickSignal && getItemsRect().pointInRect(x, y))
        {
            (*mDoubleClickSignal)(this, x, y, mask);
        }
    }

    return handled;
}

void LLGroupList::setNameFilter(const std::string& filter)
{
    std::string filter_upper = filter;
    LLStringUtil::toUpper(filter_upper);
    if (mNameFilter != filter_upper)
    {
        mNameFilter = filter_upper;

        // set no items message depend on filter state
        updateNoItemsMessage(filter);

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
    if (mForAgent)
    {
        const LLUUID&       highlight_id    = gAgent.getGroupID();
        S32                 count           = static_cast<S32>(gAgent.mGroups.size());
        LLUUID              id;
        bool                have_filter     = !mNameFilter.empty();

        clear();

        for(S32 i = 0; i < count; ++i)
        {
            id = gAgent.mGroups.at(i).mID;
            const LLGroupData& group_data = gAgent.mGroups.at(i);
            if (have_filter && !findInsensitive(group_data.mName, mNameFilter))
                continue;
            addNewItem(id, group_data.mName, group_data.mInsigniaID, ADD_BOTTOM, group_data.mListInProfile);
        }

        // Sort the list.
        sort();

        // Add "none" to list at top if filter not set (what's the point of filtering "none"?).
        // but only if some real groups exists. EXT-4838
        if (!have_filter && count > 0 && mShowNone)
        {
            std::string loc_none = LLTrans::getString("GroupsNone");
            addNewItem(LLUUID::null, loc_none, LLUUID::null, ADD_TOP);
        }

        selectItemByUUID(highlight_id);
    }
    else
    {
        clear();

        for (group_map_t::iterator it = mGroups.begin(); it != mGroups.end(); ++it)
        {
            addNewItem(it->second, it->first, LLUUID::null, ADD_BOTTOM);
        }

        // Sort the list.
        sort();
    }

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

void LLGroupList::setGroups(const std::map< std::string,LLUUID> group_list)
{
    mGroups = group_list;
    setDirty(true);
}

//////////////////////////////////////////////////////////////////////////
// PRIVATE Section
//////////////////////////////////////////////////////////////////////////

void LLGroupList::addNewItem(const LLUUID& id, const std::string& name, const LLUUID& icon_id, EAddPosition pos, bool visible_in_profile)
{
    LLGroupListItem* item = new LLGroupListItem(mForAgent, mShowIcons);

    item->setGroupID(id);
    item->setName(name, mNameFilter);
    item->setGroupIconID(icon_id);

    item->getChildView("info_btn")->setVisible( false);
    item->getChildView("profile_btn")->setVisible( false);
    item->getChildView("notices_btn")->setVisible(false);
    item->setGroupIconVisible(mShowIcons);
    if (!mShowIcons)
    {
        item->setVisibleInProfile(visible_in_profile);
    }
    addItem(item, id, pos);

//  setCommentVisible(false);
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

    if (event->desc() == "value_changed")
    {
        LLSD data = event->getValue();
        if (data.has("group_id") && data.has("visible"))
        {
            LLUUID group_id = data["group_id"].asUUID();
            bool visible = data["visible"].asBoolean();

            std::vector<LLPanel*> items;
            getItems(items);
            for (std::vector<LLPanel*>::iterator it = items.begin(); it != items.end(); ++it)
            {
                LLGroupListItem* item = dynamic_cast<LLGroupListItem*>(*it);
                if (item && item->getGroupID() == group_id)
                {
                    item->setVisibleInProfile(visible);
                    break;
                }
            }
        }
        return true;
    }

    return false;
}

bool LLGroupList::onContextMenuItemClick(const LLSD& userdata)
{
    std::string action = userdata.asString();
    LLUUID selected_group = getSelectedUUID();

    if (action == "view_info")
    {
        LLGroupActions::show(selected_group);
    }
    else if (action == "chat")
    {
        LLGroupActions::startIM(selected_group);
    }
    else if (action == "call")
    {
        LLGroupActions::startCall(selected_group);
    }
    else if (action == "activate")
    {
        LLGroupActions::activate(selected_group);
    }
    else if (action == "leave")
    {
        LLGroupActions::leave(selected_group);
    }

    return true;
}

bool LLGroupList::onContextMenuItemEnable(const LLSD& userdata)
{
    LLUUID selected_group_id = getSelectedUUID();
    bool real_group_selected = selected_group_id.notNull(); // a "real" (not "none") group is selected

    // each group including "none" can be activated
    if (userdata.asString() == "activate")
        return gAgent.getGroupID() != selected_group_id;

    if (userdata.asString() == "call")
      return real_group_selected && LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking();

    return real_group_selected;
}

/************************************************************************/
/*          LLGroupListItem implementation                              */
/************************************************************************/

LLGroupListItem::LLGroupListItem(bool for_agent, bool show_icons)
:   LLPanel(),
mGroupIcon(NULL),
mGroupNameBox(NULL),
mInfoBtn(NULL),
mProfileBtn(NULL),
mNoticesBtn(NULL),
mVisibilityHideBtn(NULL),
mVisibilityShowBtn(NULL),
mGroupID(LLUUID::null),
mForAgent(for_agent)
{
    if (show_icons)
    {
        buildFromFile( "panel_group_list_item.xml");
    }
    else
    {
        buildFromFile( "panel_group_list_item_short.xml");
    }
}

LLGroupListItem::~LLGroupListItem()
{
    LLGroupMgr::getInstance()->removeObserver(this);
}

//virtual
bool  LLGroupListItem::postBuild()
{
    mGroupIcon = getChild<LLGroupIconCtrl>("group_icon");
    mGroupNameBox = getChild<LLTextBox>("group_name");

    mInfoBtn = getChild<LLButton>("info_btn");
    mInfoBtn->setClickedCallback(boost::bind(&LLGroupListItem::onInfoBtnClick, this));

    mProfileBtn = getChild<LLButton>("profile_btn");
    mProfileBtn->setClickedCallback([this](LLUICtrl *, const LLSD &) { onProfileBtnClick(); });

    mNoticesBtn = getChild<LLButton>("notices_btn");
    mNoticesBtn->setClickedCallback([this](LLUICtrl *, const LLSD &) { onNoticesBtnClick(); });

    mVisibilityHideBtn = findChild<LLButton>("visibility_hide_btn");
    if (mVisibilityHideBtn)
    {
        mVisibilityHideBtn->setClickedCallback([this](LLUICtrl *, const LLSD &) { onVisibilityBtnClick(false); });
    }
    mVisibilityShowBtn = findChild<LLButton>("visibility_show_btn");
    if (mVisibilityShowBtn)
    {
        mVisibilityShowBtn->setClickedCallback([this](LLUICtrl *, const LLSD &) { onVisibilityBtnClick(true); });
    }

    // Remember group icon width including its padding from the name text box,
    // so that we can hide and show the icon again later.
    // Also note that panel_group_list_item and panel_group_list_item_short
    // have icons of different sizes so we need to figure it per file.
    mIconWidth = mGroupNameBox->getRect().mLeft - mGroupIcon->getRect().mLeft;

    return true;
}

//virtual
void LLGroupListItem::setValue( const LLSD& value )
{
    if (!value.isMap()) return;
    if (!value.has("selected")) return;
    getChildView("selected_icon")->setVisible( value["selected"]);
}

void LLGroupListItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
    getChildView("hovered_icon")->setVisible( true);
    if (mGroupID.notNull()) // don't show the info button for the "none" group
    {
        mInfoBtn->setVisible(true);
        mProfileBtn->setVisible(true);
        if (mForAgent)
        {
            LLGroupData agent_gdatap;
            if (gAgent.getGroupData(mGroupID, agent_gdatap))
            {
                if (mVisibilityHideBtn)
                {
                mVisibilityHideBtn->setVisible(agent_gdatap.mListInProfile);
                mVisibilityShowBtn->setVisible(!agent_gdatap.mListInProfile);
            }
                mNoticesBtn->setVisible(true);
            }
        }
    }

    LLPanel::onMouseEnter(x, y, mask);
}

void LLGroupListItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
    getChildView("hovered_icon")->setVisible( false);
    mInfoBtn->setVisible(false);
    mProfileBtn->setVisible(false);
    mNoticesBtn->setVisible(false);
    if (mVisibilityHideBtn)
    {
        mVisibilityHideBtn->setVisible(false);
        mVisibilityShowBtn->setVisible(false);
    }

    LLPanel::onMouseLeave(x, y, mask);
}

void LLGroupListItem::setName(const std::string& name, const std::string& highlight)
{
    mGroupName = name;
    LLTextUtil::textboxSetHighlightedVal(mGroupNameBox, mGroupNameStyle, name, highlight);
    mGroupNameBox->setToolTip(name);
}

void LLGroupListItem::setGroupID(const LLUUID& group_id)
{
    LLGroupMgr::getInstance()->removeObserver(this);

    mID = group_id;
    mGroupID = group_id;

    if (mForAgent)
    {
        // Active group should be bold.
        setBold(group_id == gAgent.getGroupID());
    }
    else
    {
        // Groups shared with the agent should be bold
        setBold(gAgent.isInGroup(group_id, true));
    }

    LLGroupMgr::getInstance()->addObserver(this);
}

void LLGroupListItem::setGroupIconID(const LLUUID& group_icon_id)
{
    mGroupIcon->setIconId(group_icon_id);
}

void LLGroupListItem::setGroupIconVisible(bool visible)
{
    // Already done? Then do nothing.
    if (mGroupIcon->getVisible() == (bool)visible)
        return;

    // Show/hide the group icon.
    mGroupIcon->setVisible(visible);

    // Move the group name horizontally by icon size + its distance from the group name.
    LLRect name_rect = mGroupNameBox->getRect();
    name_rect.mLeft += visible ? mIconWidth : -mIconWidth;
    mGroupNameBox->setRect(name_rect);
}

void LLGroupListItem::setVisibleInProfile(bool visible)
{
    mGroupNameBox->setColor(LLUIColorTable::instance().getColor((visible ? "GroupVisibleInProfile" : "GroupHiddenInProfile"), LLColor4::red).get());
}

//////////////////////////////////////////////////////////////////////////
// Private Section
//////////////////////////////////////////////////////////////////////////
void LLGroupListItem::setBold(bool bold)
{
    // *BUG: setName() overrides the style params.

    LLFontDescriptor new_desc(mGroupNameBox->getFont()->getFontDesc());

    // *NOTE dzaporozhan
    // On Windows LLFontGL::NORMAL will not remove LLFontGL::BOLD if font
    // is predefined as bold (SansSerifSmallBold, for example)
    new_desc.setStyle(bold ? LLFontGL::BOLD : LLFontGL::NORMAL);
    LLFontGL* new_font = LLFontGL::getFont(new_desc);
    mGroupNameStyle.font = new_font;

    // *NOTE: You cannot set the style on a text box anymore, you must
    // rebuild the text.  This will cause problems if the text contains
    // hyperlinks, as their styles will be wrong.
    mGroupNameBox->setText(mGroupName, mGroupNameStyle);
}

void LLGroupListItem::onInfoBtnClick()
{
    LLFloaterReg::showInstance("inspect_group", LLSD().with("group_id", mGroupID));
}

void LLGroupListItem::onProfileBtnClick()
{
    LLGroupActions::show(mGroupID);
}

void LLGroupListItem::onNoticesBtnClick()
{
    LLGroupActions::show(mGroupID, true);
}

void LLGroupListItem::onVisibilityBtnClick(bool new_visibility)
{
    LLGroupData agent_gdatap;
    if (gAgent.getGroupData(mGroupID, agent_gdatap))
    {
        gAgent.setUserGroupFlags(mGroupID, agent_gdatap.mAcceptNotices, new_visibility);
        setVisibleInProfile(new_visibility);
        mVisibilityHideBtn->setVisible(new_visibility);
        mVisibilityShowBtn->setVisible(!new_visibility);
    }
}

void LLGroupListItem::changed(LLGroupChange gc)
{
    LLGroupMgrGroupData* group_data = LLGroupMgr::getInstance()->getGroupData(mID);
    if ((gc == GC_ALL || gc == GC_PROPERTIES) && group_data)
    {
        setGroupIconID(group_data->mInsigniaID);
    }
}

//EOF
