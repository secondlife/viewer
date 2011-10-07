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
// LLCommandId class
//

const LLCommandId LLCommandId::null("null command");

//
// LLCommand class
//

LLCommand::Params::Params()
	: function("function")
	, available_in_toybox("available_in_toybox", false)
	, icon("icon")
	, label_ref("label_ref")
	, name("name")
	, parameter("parameter")
	, tooltip_ref("tooltip_ref")
{
}

LLCommand::LLCommand(const LLCommand::Params& p)
	: mFunction(p.function)
	, mAvailableInToybox(p.available_in_toybox)
	, mIcon(p.icon)
	, mIdentifier(p.name)
	, mLabelRef(p.label_ref)
	, mParameter(p.parameter)
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
	for (CommandVector::iterator cmdIt = mCommands.begin(); cmdIt != mCommands.end(); ++cmdIt)
	{
		LLCommand * command = *cmdIt;

		delete command;
	}
}

U32 LLCommandManager::commandCount() const
{
	return mCommands.size();
}

LLCommand * LLCommandManager::getCommand(U32 commandIndex)
{
	return mCommands[commandIndex];
}

LLCommand * LLCommandManager::getCommand(const LLCommandId& commandId)
{
	LLCommand * command_match = NULL;

	CommandIndexMap::const_iterator found = mCommandIndices.find(commandId);
	
	if (found != mCommandIndices.end())
	{
		command_match = mCommands[found->second];
	}

	return command_match;
}

void LLCommandManager::addCommand(LLCommand * command)
{
	mCommandIndices[command->id()] = mCommands.size();
	mCommands.push_back(command);

	lldebugs << "Successfully added command: " << command->id().name() << llendl;
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
		llerrs << "Invalid commands file: " << commands_file << llendl;
		return false;
	}

	BOOST_FOREACH(LLCommand::Params& commandParams, commandsParams.commands)
	{
		LLCommand * command = new LLCommand(commandParams);

		mgr.addCommand(command);
	}

	return true;
}
