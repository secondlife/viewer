/** 
 * @file llfloatertoybox.cpp
 * @brief The toybox for flexibilizing the UI.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llfloatertoybox.h"

#include "llbutton.h"
#include "llcommandmanager.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llpanel.h"
#include "lltoolbar.h"
#include "lltoolbarview.h"
#include "lltrans.h"

LLFloaterToybox::LLFloaterToybox(const LLSD& key)
	: LLFloater(key)
	, mToolBar(NULL)
{
	mCommitCallbackRegistrar.add("Toybox.RestoreDefaults", boost::bind(&LLFloaterToybox::onBtnRestoreDefaults, this));
	mCommitCallbackRegistrar.add("Toybox.ClearAll", boost::bind(&LLFloaterToybox::onBtnClearAll, this));
}

LLFloaterToybox::~LLFloaterToybox()
{
}

bool compare_localized_command_labels(LLCommand * cmd1, LLCommand * cmd2)
{
	std::string lab1 = LLTrans::getString(cmd1->labelRef());
	std::string lab2 = LLTrans::getString(cmd2->labelRef());

	return (lab1 < lab2);
}

BOOL LLFloaterToybox::postBuild()
{	
	mToolBar = getChild<LLToolBar>("toybox_toolbar");

	mToolBar->setStartDragCallback(boost::bind(LLToolBarView::startDragTool,_1,_2,_3));
	mToolBar->setHandleDragCallback(boost::bind(LLToolBarView::handleDragTool,_1,_2,_3,_4));
	mToolBar->setHandleDropCallback(boost::bind(LLToolBarView::handleDropTool,_1,_2,_3,_4));
	mToolBar->setButtonEnterCallback(boost::bind(&LLFloaterToybox::onToolBarButtonEnter,this,_1));
	
	//
	// Sort commands by localized labels so they will appear alphabetized in all languages
	//

	std::list<LLCommand *> alphabetized_commands;

	LLCommandManager& cmdMgr = LLCommandManager::instance();
	for (U32 i = 0; i < cmdMgr.commandCount(); i++)
	{
		LLCommand * command = cmdMgr.getCommand(i);

		if (command->availableInToybox())
		{
			alphabetized_commands.push_back(command);
		}
	}

	alphabetized_commands.sort(compare_localized_command_labels);

	//
	// Create Buttons
	//

	for (std::list<LLCommand *>::iterator it = alphabetized_commands.begin(); it != alphabetized_commands.end(); ++it)
	{
		mToolBar->addCommand((*it)->id());
	}

	return TRUE;
}

void LLFloaterToybox::draw()
{
	llassert(gToolBarView != NULL);

	const command_id_list_t& command_list = mToolBar->getCommandsList();

	for (command_id_list_t::const_iterator it = command_list.begin(); it != command_list.end(); ++it)
	{
		const LLCommandId& id = *it;

		const bool command_not_present = (gToolBarView->hasCommand(id) == LLToolBarView::TOOLBAR_NONE);
		mToolBar->enableCommand(id, command_not_present);
	}

	LLFloater::draw();
}

static bool finish_restore_toybox(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	if (option == 0)
	{
		LLToolBarView::loadDefaultToolbars();
	}

	return false;
}

static bool finish_clear_all_toybox(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	if (option == 0)
	{
		LLToolBarView::clearAllToolbars();
	}

	return false;
}

static LLNotificationFunctorRegistration finish_restore_toybox_reg("ConfirmRestoreToybox", finish_restore_toybox);
static LLNotificationFunctorRegistration finish_clear_all_toybox_reg("ConfirmClearAllToybox", finish_clear_all_toybox);

void LLFloaterToybox::onBtnRestoreDefaults()
{
	LLNotificationsUtil::add("ConfirmRestoreToybox");
}

void LLFloaterToybox::onBtnClearAll()
{
	LLNotificationsUtil::add("ConfirmClearAllToybox");
}

BOOL LLFloaterToybox::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
	EDragAndDropType cargo_type,
	void* cargo_data,
	EAcceptance* accept,
	std::string& tooltip_msg)
{
	S32 local_x = x - mToolBar->getRect().mLeft;
	S32 local_y = y - mToolBar->getRect().mBottom;
	return mToolBar->handleDragAndDrop(local_x, local_y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
}

void LLFloaterToybox::onToolBarButtonEnter(LLView* button)
{
	std::string suffix = "";

	LLCommandId commandId(button->getName());
	LLCommand* command = LLCommandManager::instance().getCommand(commandId);

	if (command)
	{
		S32 command_loc = gToolBarView->hasCommand(commandId);

		switch(command_loc)
		{
		case LLToolBarView::TOOLBAR_BOTTOM:	suffix = LLTrans::getString("Toolbar_Bottom_Tooltip");	break;
		case LLToolBarView::TOOLBAR_LEFT:	suffix = LLTrans::getString("Toolbar_Left_Tooltip");	break;
		case LLToolBarView::TOOLBAR_RIGHT:	suffix = LLTrans::getString("Toolbar_Right_Tooltip");	break;

		default:
			break;
		}
	}

	mToolBar->setTooltipButtonSuffix(suffix);
}


// eof
