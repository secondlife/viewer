/** 
 * @file llimfloatercontainer.cpp
 * @brief Multifloater containing active IM sessions in separate tab container tabs
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

#include "llimfloater.h"
#include "llimfloatercontainer.h"

#include "llfloaterreg.h"
#include "lllayoutstack.h"
#include "llnearbychat.h"

#include "llagent.h"
#include "llavataractions.h"
#include "llavatariconctrl.h"
#include "llgroupiconctrl.h"
#include "llfloateravatarpicker.h"
#include "llimview.h"
#include "lltransientfloatermgr.h"
#include "llviewercontrol.h"

//
// LLIMFloaterContainer
//
LLIMFloaterContainer::LLIMFloaterContainer(const LLSD& seed)
:	LLMultiFloater(seed)
	,mExpandCollapseBtn(NULL)
{
	mAutoResize = FALSE;
	LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::IM, this);
}

LLIMFloaterContainer::~LLIMFloaterContainer()
{
	mNewMessageConnection.disconnect();
	LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::IM, this);

	gSavedPerAccountSettings.setBOOL("ConversationsListPaneCollapsed", mConversationsPane->isCollapsed());
	gSavedPerAccountSettings.setBOOL("ConversationsMessagePaneCollapsed", mMessagesPane->isCollapsed());
}

BOOL LLIMFloaterContainer::postBuild()
{
	mNewMessageConnection = LLIMModel::instance().mNewMsgSignal.connect(boost::bind(&LLIMFloaterContainer::onNewMessageReceived, this, _1));
	// Do not call base postBuild to not connect to mCloseSignal to not close all floaters via Close button
	// mTabContainer will be initialized in LLMultiFloater::addChild()
	
	setTabContainer(getChild<LLTabContainer>("im_box_tab_container"));

	mConversationsStack = getChild<LLLayoutStack>("conversations_stack");
	mConversationsPane = getChild<LLLayoutPanel>("conversations_layout_panel");
	mMessagesPane = getChild<LLLayoutPanel>("messages_layout_panel");
	
	mConversationsListPanel = getChild<LLPanel>("conversations_list_panel");

	mExpandCollapseBtn = getChild<LLButton>("expand_collapse_btn");
	mExpandCollapseBtn->setClickedCallback(boost::bind(&LLIMFloaterContainer::onExpandCollapseButtonClicked, this));

	childSetAction("add_btn", boost::bind(&LLIMFloaterContainer::onAddButtonClicked, this));

	collapseMessagesPane(gSavedPerAccountSettings.getBOOL("ConversationsMessagePaneCollapsed"));
	collapseConversationsPane(gSavedPerAccountSettings.getBOOL("ConversationsListPaneCollapsed"));

	return TRUE;
}

void LLIMFloaterContainer::onOpen(const LLSD& key)
{
	LLMultiFloater::onOpen(key);
	if (getFloaterCount() == 0)
	{
		// If there's *no* conversation open so far, we force the opening of the nearby chat conversation
		// *TODO: find a way to move this to XML as a default panel or something like that
		LLSD name("chat_bar");
		LLSD key("");
		LLFloaterReg::toggleInstanceOrBringToFront(name,key);
	}
	/*
	if (key.isDefined())
	{
		LLIMFloater* im_floater = LLIMFloater::findInstance(key.asUUID());
		if (im_floater)
		{
			im_floater->openFloater();
		}
	}
	 */
}

// virtual
void LLIMFloaterContainer::addFloater(LLFloater* floaterp, 
									BOOL select_added_floater, 
									LLTabContainer::eInsertionPoint insertion_point)
{
	if(!floaterp) return;

	// already here
	if (floaterp->getHost() == this)
	{
		openFloater(floaterp->getKey());
		return;
	}

	LLMultiFloater::addFloater(floaterp, select_added_floater, insertion_point);

	LLUUID session_id = floaterp->getKey();

	// CHUI-137 : Temporary implementation of conversations list
	// Create a conversation item
	LLConversationItem* item = new LLConversationItem(floaterp->getTitle(),session_id, floaterp, this);
	mConversationsItems[session_id] = item;
	// Create a widget from it
	LLFolderViewItem* widget = createConversationItemWidget(item);
	mConversationsWidgets[session_id] = widget;
	// Add it to the UI
	widget->setVisible(TRUE);
	mConversationsListPanel->addChild(widget);
	LLRect panel_rect = mConversationsListPanel->getRect();
	S32 item_height = 16;
	S32 index = mConversationsWidgets.size() - 1;
	widget->setRect(LLRect(0,
						   panel_rect.getHeight() - item_height*index,
						   panel_rect.getWidth(),
						   panel_rect.getHeight() - item_height*(index+1)));
	// CHUI-137 : end
	
	LLView* floater_contents = floaterp->getChild<LLView>("contents_view");

	// we don't show the header when the floater is hosted,
	// so reshape floater contents to occupy the header space
	floater_contents->setShape(floaterp->getRect());

	LLIconCtrl* icon = 0;

	if(gAgent.isInGroup(session_id, TRUE))
	{
		LLGroupIconCtrl::Params icon_params;
		icon_params.group_id = session_id;
		icon = LLUICtrlFactory::instance().create<LLGroupIconCtrl>(icon_params);

		mSessions[session_id] = floaterp;
		floaterp->mCloseSignal.connect(boost::bind(&LLIMFloaterContainer::onCloseFloater, this, session_id));
	}
	else
	{
		LLUUID avatar_id = LLIMModel::getInstance()->getOtherParticipantID(session_id);

		LLAvatarIconCtrl::Params icon_params;
		icon_params.avatar_id = avatar_id;
		icon = LLUICtrlFactory::instance().create<LLAvatarIconCtrl>(icon_params);

		mSessions[session_id] = floaterp;
		floaterp->mCloseSignal.connect(boost::bind(&LLIMFloaterContainer::onCloseFloater, this, session_id));
	}
	mTabContainer->setTabImage(floaterp, icon);
}

// virtual
void LLIMFloaterContainer::removeFloater(LLFloater* floaterp)
{
	LLMultiFloater::removeFloater(floaterp);

    // CHUI-137 : Temporary implementation of conversations list
	// Clean up the conversations list
 	LLUUID session_id = floaterp->getKey();
    // Delete the widget and the associated conversation item
    // Note : since the mConversationsItems is also the listener to the widget, deleting 
    // the widget will also delete its listener
	conversations_widgets_map::iterator widget_it = mConversationsWidgets.find(session_id);
	if (widget_it != mConversationsWidgets.end())
    {
        LLFolderViewItem* widget = widget_it->second;
        delete widget;
    }
    // Suppress the conversation items and widgets from their respective maps
	mConversationsItems.erase(session_id);
	mConversationsWidgets.erase(session_id);
    // Reposition the leftover conversation items
	LLRect panel_rect = mConversationsListPanel->getRect();
	S32 item_height = 16;
    int index = 0;
    for (widget_it = mConversationsWidgets.begin(); widget_it != mConversationsWidgets.end(); ++widget_it, ++index)
    {
        LLFolderViewItem* widget = widget_it->second;
        widget->setRect(LLRect(0,
                               panel_rect.getHeight() - item_height*index,
                               panel_rect.getWidth(),
                               panel_rect.getHeight() - item_height*(index+1)));
    }
    // CHUI-137 : end
   
	LLRect contents_rect = floaterp->getRect();

	// reduce the floater contents height by header height
	contents_rect.mTop -= floaterp->getHeaderHeight();

	LLView* floater_contents = floaterp->getChild<LLView>("contents_view");
	floater_contents->setShape(contents_rect);
}

void LLIMFloaterContainer::onCloseFloater(LLUUID& id)
{
	mSessions.erase(id);
}

// virtual
void LLIMFloaterContainer::computeResizeLimits(S32& new_min_width, S32& new_min_height)
{
	bool is_left_pane_expanded = !mConversationsPane->isCollapsed();
	bool is_right_pane_expanded = !mMessagesPane->isCollapsed();

	S32 conversations_pane_min_dim = mConversationsPane->getMinDim();

	if (is_right_pane_expanded)
	{
		S32 conversations_pane_width =
				(is_left_pane_expanded ? gSavedPerAccountSettings.getS32("ConversationsListPaneWidth") : conversations_pane_min_dim);

		// possibly increase minimum size constraint due to children's minimums.
		for (S32 tab_idx = 0; tab_idx < mTabContainer->getTabCount(); ++tab_idx)
		{
			LLFloater* floaterp = dynamic_cast<LLFloater*>(mTabContainer->getPanelByIndex(tab_idx));
			if (floaterp)
			{
				new_min_width = llmax(new_min_width,
						floaterp->getMinWidth() + conversations_pane_width + LLPANEL_BORDER_WIDTH * 2);
				new_min_height = llmax(new_min_height, floaterp->getMinHeight());
			}
		}
	}
	else
	{
		new_min_width = conversations_pane_min_dim;
	}
}

void LLIMFloaterContainer::onNewMessageReceived(const LLSD& data)
{
	LLUUID session_id = data["session_id"].asUUID();
	LLFloater* floaterp = get_ptr_in_map(mSessions, session_id);
	LLFloater* current_floater = LLMultiFloater::getActiveFloater();

	if(floaterp && current_floater && floaterp != current_floater)
	{
		if(LLMultiFloater::isFloaterFlashing(floaterp))
			LLMultiFloater::setFloaterFlashing(floaterp, FALSE);
		LLMultiFloater::setFloaterFlashing(floaterp, TRUE);
	}
}

void LLIMFloaterContainer::onExpandCollapseButtonClicked()
{
	if (mConversationsPane->isCollapsed() && mMessagesPane->isCollapsed()
			&& gSavedPerAccountSettings.getBOOL("ConversationsExpandMessagePaneFirst"))
	{
		// Expand the messages pane from ultra minimized state
		// if it was collapsed last in order.
		collapseMessagesPane(false);
	}
	else
	{
		collapseConversationsPane(!mConversationsPane->isCollapsed());
	}
}

LLIMFloaterContainer* LLIMFloaterContainer::findInstance()
{
	return LLFloaterReg::findTypedInstance<LLIMFloaterContainer>("im_container");
}

LLIMFloaterContainer* LLIMFloaterContainer::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLIMFloaterContainer>("im_container");
}

void LLIMFloaterContainer::setMinimized(BOOL b)
{
	if (isMinimized() == b) return;
	
	LLMultiFloater::setMinimized(b);
	// Hide minimized floater (see EXT-5315)
	setVisible(!b);

	if (isMinimized()) return;

	if (getActiveFloater())
	{
		getActiveFloater()->setVisible(TRUE);
	}
}

void LLIMFloaterContainer::collapseMessagesPane(bool collapse)
{
	if (mMessagesPane->isCollapsed() == collapse)
	{
		return;
	}

	if (collapse)
	{
		// Save the messages pane width before collapsing it.
		gSavedPerAccountSettings.setS32("ConversationsMessagePaneWidth", mMessagesPane->getRect().getWidth());

		// Save the order in which the panels are closed to reverse user's last action.
		gSavedPerAccountSettings.setBOOL("ConversationsExpandMessagePaneFirst", mConversationsPane->isCollapsed());
	}

	// Show/hide the messages pane.
	mConversationsStack->collapsePanel(mMessagesPane, collapse);

	updateState(collapse, gSavedPerAccountSettings.getS32("ConversationsMessagePaneWidth"));
}

void LLIMFloaterContainer::collapseConversationsPane(bool collapse)
{
	if (mConversationsPane->isCollapsed() == collapse)
	{
		return;
	}

	LLView* button_panel = getChild<LLView>("conversations_pane_buttons_expanded");
	button_panel->setVisible(!collapse);
	mExpandCollapseBtn->setImageOverlay(getString(collapse ? "expand_icon" : "collapse_icon"));

	if (collapse)
	{
		// Save the conversations pane width before collapsing it.
		gSavedPerAccountSettings.setS32("ConversationsListPaneWidth", mConversationsPane->getRect().getWidth());

		// Save the order in which the panels are closed to reverse user's last action.
		gSavedPerAccountSettings.setBOOL("ConversationsExpandMessagePaneFirst", !mMessagesPane->isCollapsed());
	}

	mConversationsStack->collapsePanel(mConversationsPane, collapse);

	S32 collapsed_width = mConversationsPane->getMinDim();
	updateState(collapse, gSavedPerAccountSettings.getS32("ConversationsListPaneWidth") - collapsed_width);
}

void LLIMFloaterContainer::updateState(bool collapse, S32 delta_width)
{
	LLRect floater_rect = getRect();
	floater_rect.mRight += ((collapse ? -1 : 1) * delta_width);
	setShape(floater_rect);

	updateResizeLimits();

	bool is_left_pane_expanded = !mConversationsPane->isCollapsed();
	bool is_right_pane_expanded = !mMessagesPane->isCollapsed();

	setCanResize(is_left_pane_expanded || is_right_pane_expanded);
	setCanMinimize(is_left_pane_expanded || is_right_pane_expanded);
}

void LLIMFloaterContainer::onAddButtonClicked()
{
    LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(boost::bind(&LLIMFloaterContainer::onAvatarPicked, this, _1), TRUE, TRUE);
    LLFloater* root_floater = gFloaterView->getParentFloater(this);
    if (picker && root_floater)
    {
        root_floater->addDependentFloater(picker);
    }
}

void LLIMFloaterContainer::onAvatarPicked(const uuid_vec_t& ids)
{
    if (ids.size() == 1)
    {
        LLAvatarActions::startIM(ids.back());
    }
    else
    {
        LLAvatarActions::startConference(ids);
    }
}

// CHUI-137 : Temporary implementation of conversations list
LLFolderViewItem* LLIMFloaterContainer::createConversationItemWidget(LLConversationItem* item)
{
	LLFolderViewItem::Params params;
	
	params.name = item->getDisplayName();
	//params.icon = bridge->getIcon();
	//params.icon_open = bridge->getOpenIcon();
	//params.creation_date = bridge->getCreationDate();
	//params.root = mFolderRoot;
	params.listener = item;
	params.rect = LLRect (0, 0, 0, 0);
	params.tool_tip = params.name;
	
	return LLUICtrlFactory::create<LLFolderViewItem>(params);
}

// Conversation items
LLConversationItem::LLConversationItem(std::string name, const LLUUID& uuid, LLFloater* floaterp, LLIMFloaterContainer* containerp) :
	mName(name),
	mUUID(uuid),
    mFloater(floaterp),
    mContainer(containerp)
{
    // Hack: the nearby chat has no name so we catch that case and impose one
	// Of course, we won't be doing this in the final code
	if (name == "")
		mName = "Nearby Chat";
}

// Virtual action callbacks
void LLConversationItem::selectItem(void)
{
    // Always expand the message pane in that case
    mContainer->collapseMessagesPane(false);
    // Switch to the conversation floater that is being selected
    mContainer->selectFloater(mFloater);
}

void LLConversationItem::performAction(LLInventoryModel* model, std::string action)
{
}

void LLConversationItem::openItem( void )
{
}

void LLConversationItem::closeItem( void )
{
}

void LLConversationItem::previewItem( void )
{
}

void LLConversationItem::showProperties(void)
{
}

// EOF
