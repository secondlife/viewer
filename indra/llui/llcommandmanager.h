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

#ifndef LL_LLCOMMANDMANAGER_H
#define LL_LLCOMMANDMANAGER_H

#include "llinitparam.h"
#include "llsingleton.h"


class LLCommand;
class LLCommandManager;


class LLCommandId
{
public:
	friend class LLCommand;
	friend class LLCommandManager;

	struct Params : public LLInitParam::Block<Params>
	{
		Mandatory<std::string> name;

		Params()
		:	name("name")
		{}
	};

	LLCommandId(const std::string& name)
	{
		mUUID = LLUUID::generateNewID(name);
	}

	LLCommandId(const Params& p)
	{
		mUUID = LLUUID::generateNewID(p.name);
	}

	LLCommandId(const LLUUID& uuid)
	:	mUUID(uuid)
	{}
	
	const LLUUID& uuid() const { return mUUID; }

	bool operator!=(const LLCommandId& command) const
	{
		return (mUUID != command.mUUID);
	}

	bool operator==(const LLCommandId& command) const
	{
		return (mUUID == command.mUUID);
	}

	static const LLCommandId null;

private:
	LLUUID		mUUID;
};

typedef std::list<LLCommandId> command_id_list_t;


class LLCommand
{
public:
	struct Params : public LLInitParam::Block<Params>
	{
		Mandatory<bool>			available_in_toybox;
		Mandatory<std::string>	icon;
		Mandatory<std::string>	label_ref;
		Mandatory<std::string>	name;
		Mandatory<std::string>	tooltip_ref;

		Mandatory<std::string>	execute_function;
		Optional<LLSD>			execute_parameters;

		Optional<std::string>	execute_stop_function;
		Optional<LLSD>			execute_stop_parameters;
		
		Optional<std::string>	is_enabled_function;
		Optional<LLSD>			is_enabled_parameters;

		Optional<std::string>	is_running_function;
		Optional<LLSD>			is_running_parameters;

		Optional<std::string>	is_starting_function;
		Optional<LLSD>			is_starting_parameters;

		Params();
	};

	LLCommand(const LLCommand::Params& p);

	const bool availableInToybox() const { return mAvailableInToybox; }
	const std::string& icon() const { return mIcon; }
	const LLCommandId& id() const { return mIdentifier; }
	const std::string& labelRef() const { return mLabelRef; }
	const std::string& name() const { return mName; }
	const std::string& tooltipRef() const { return mTooltipRef; }

	const std::string& executeFunctionName() const { return mExecuteFunction; }
	const LLSD& executeParameters() const { return mExecuteParameters; }

	const std::string& executeStopFunctionName() const { return mExecuteStopFunction; }
	const LLSD& executeStopParameters() const { return mExecuteStopParameters; }
	
	const std::string& isEnabledFunctionName() const { return mIsEnabledFunction; }
	const LLSD& isEnabledParameters() const { return mIsEnabledParameters; }

	const std::string& isRunningFunctionName() const { return mIsRunningFunction; }
	const LLSD& isRunningParameters() const { return mIsRunningParameters; }

	const std::string& isStartingFunctionName() const { return mIsStartingFunction; }
	const LLSD& isStartingParameters() const { return mIsStartingParameters; }

private:
	LLCommandId mIdentifier;

	bool        mAvailableInToybox;
	std::string mIcon;
	std::string mLabelRef;
	std::string mName;
	std::string mTooltipRef;

	std::string mExecuteFunction;
	LLSD        mExecuteParameters;

	std::string mExecuteStopFunction;
	LLSD        mExecuteStopParameters;
	
	std::string mIsEnabledFunction;
	LLSD        mIsEnabledParameters;

	std::string mIsRunningFunction;
	LLSD        mIsRunningParameters;

	std::string mIsStartingFunction;
	LLSD        mIsStartingParameters;
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

	U32 commandCount() const;
	LLCommand * getCommand(U32 commandIndex);
	LLCommand * getCommand(const LLCommandId& commandId);

	static bool load();

protected:
	void addCommand(LLCommand * command);

private:
	typedef std::map<LLUUID, U32>		CommandIndexMap;
	typedef std::vector<LLCommand *>	CommandVector;
	
	CommandVector	mCommands;
	CommandIndexMap	mCommandIndices;
};


#endif // LL_LLCOMMANDMANAGER_H
