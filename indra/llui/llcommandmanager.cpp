/** 
 * @file llcommandmanager.cpp
 * @brief LLCommandManager class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

// A control that displays the name of the chosen item, which when
// clicked shows a scrolling box of options.

#include "linden_common.h"

#include "llcommandmanager.h"
#include "lldir.h"
#include "llerror.h"
#include "llxuiparser.h"

#include <boost/foreach.hpp>


//
// LLCommand class
//

LLCommand::Params::Params()
	: function("function")
	, icon("icon")
	, label_ref("label_ref")
	, name("name")
	, param("param")
	, tooltip_ref("tooltip_ref")
{
}

LLCommand::LLCommand(const LLCommand::Params& p)
	: mFunction(p.function)
	, mIcon(p.icon)
	, mLabelRef(p.label_ref)
	, mName(p.name)
	, mParam(p.param)
	, mTooltipRef(p.tooltip_ref)
{
}


//
// LLCommandManager class
//

LLCommandManager::LLCommandManager()
{
}

LLCommandManager::~LLCommandManager()
{
}

U32 LLCommandManager::count() const
{
	return mCommands.size();
}

LLCommand * LLCommandManager::getCommand(U32 commandIndex)
{
	return mCommands[commandIndex];
}

LLCommand * LLCommandManager::getCommand(const std::string& commandName)
{
	LLCommand * command_name_match = NULL;

	for (CommandVector::iterator it = mCommands.begin(); it != mCommands.end(); ++it)
	{
		LLCommand * command = *it;

		if (command->name() == commandName)
		{
			command_name_match = command;
			break;
		}
	}

	return command_name_match;
}

//static
bool LLCommandManager::load()
{
	LLCommandManager& mgr = LLCommandManager::instance();

	std::string commands_file = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "commands.xml");

	LLCommandManager::Params commandsParams;

	LLSimpleXUIParser parser;
	
	if (!parser.readXUI(commands_file, commandsParams))
	{
		llerrs << "Unable to load xml file: " << commands_file << llendl;
		return false;
	}

	if (!commandsParams.validateBlock())
	{
		llerrs << "Unable to validate commands param block from file: " << commands_file << llendl;
		return false;
	}

	BOOST_FOREACH(LLCommand::Params& commandParams, commandsParams.commands)
	{
		LLCommand * command = new LLCommand(commandParams);

		mgr.mCommands.push_back(command);

		llinfos << "Successfully loaded command: " << command->name() << llendl;
	}

	return true;
}
