/** 
 * @file llcommandmanager.h
 * @brief LLCommandManager class to hold commands
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

#ifndef LL_COMMANDMANAGER_H
#define LL_COMMANDMANAGER_H

#include "llinitparam.h"
#include "llsingleton.h"


class LLCommand
{
public:
	struct Params : public LLInitParam::Block<Params>
	{
		Mandatory<std::string>	function;
		Mandatory<std::string>	icon;
		Mandatory<std::string>	label_ref;
		Mandatory<std::string>	name;
		Optional<std::string>	param;
		Mandatory<std::string>	tooltip_ref;

		Params();
	};

	LLCommand(const LLCommand::Params& p);

	const std::string& functionName() const { return mFunction; }
	const std::string& icon() const { return mIcon; }
	const std::string& labelRef() const { return mLabelRef; }
	const std::string& name() const { return mName; }
	const std::string& param() const { return mParam; }
	const std::string& tooltipRef() const { return mTooltipRef; }

private:
	std::string mFunction;
	std::string mIcon;
	std::string mLabelRef;
	std::string	mName;
	std::string mParam;
	std::string mTooltipRef;
};


class LLCommandManager
:	public LLSingleton<LLCommandManager>
{
public:
	struct Params : public LLInitParam::Block<Params>
	{
		Multiple< LLCommand::Params, AtLeast<1> > commands;

		Params()
			:	commands("command")
		{
		}
	};

	LLCommandManager();
	~LLCommandManager();

	U32 count() const;
	LLCommand * getCommand(U32 commandIndex);
	LLCommand * getCommand(const std::string& commandName);

	static bool load();

private:
	typedef std::vector<LLCommand *>	CommandVector;
	CommandVector						mCommands;
};


#endif // LL_COMMANDMANAGER_H
