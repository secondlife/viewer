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
#include "llpanel.h"
#include "lltoolbar.h"
#include "lltoolbarview.h"
#include "lltrans.h"


LLFloaterToybox::LLFloaterToybox(const LLSD& key)
	: LLFloater(key)
	, mBtnRestoreDefaults(NULL)
	, mToolBar(NULL)
{
	mCommitCallbackRegistrar.add("Toybox.RestoreDefaults", boost::bind(&LLFloaterToybox::onBtnRestoreDefaults, this));
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
	center();

	mBtnRestoreDefaults = getChild<LLButton>("btn_restore_defaults");
	mToolBar = getChild<LLToolBar>("toybox_toolbar");

	LLCommandManager& cmdMgr = LLCommandManager::instance();

	//
	// Sort commands by localized labels so they will appear alphabetized in all languages
	//

	std::list<LLCommand *> alphabetized_commands;

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

void LLFloaterToybox::onOpen(const LLSD& key)
{

}

BOOL LLFloaterToybox::canClose()
{
	return TRUE;
}

void LLFloaterToybox::onClose(bool app_quitting)
{

}

void LLFloaterToybox::draw()
{
	llassert(gToolBarView != NULL);

	LLCommandManager& cmdMgr = LLCommandManager::instance();

	for (U32 i = 0; i < cmdMgr.commandCount(); i++)
	{
		LLCommand * command = cmdMgr.getCommand(i);

		if (command->availableInToybox())
		{
			mToolBar->enableCommand(command->id(), !gToolBarView->hasCommand(command->id()));
		}
	}

	LLFloater::draw();
}

void LLFloaterToybox::onFocusReceived()
{
}

void LLFloaterToybox::onBtnRestoreDefaults()
{
}


// eof
