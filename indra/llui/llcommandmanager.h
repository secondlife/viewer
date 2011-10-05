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
		: mName(name)
	{
		mUUID = LLUUID::LLUUID::generateNewID(name);
	}

	LLCommandId(const Params& p)
	:	mName(p.name)
	{
		mUUID = LLUUID::LLUUID::generateNewID(p.name);
	}

	const std::string& name() const { return mName; }
	const LLUUID& uuid() const { return mUUID; }

	bool operator!=(const LLCommandId& command) const
	{
		return (mName != command.mName);
	}

	bool operator==(const LLCommandId& command) const
	{
		return (mName == command.mName);
	}

	bool operator<(const LLCommandId& command) const
	{
		return (mName < command.mName);
	}

	static const LLCommandId null;

private:
	std::string mName;
	LLUUID		mUUID;
};

typedef std::list<LLCommandId> command_id_list_t;

class LLCommand
{
public:
	struct Params : public LLInitParam::Block<Params>
	{
		Mandatory<bool>			available_in_toybox;
		Mandatory<std::string>	function;
		Mandatory<std::string>	icon;
		Mandatory<std::string>	label_ref;
		Mandatory<std::string>	name;
		Optional<LLSD>			parameter;
		Mandatory<std::string>	tooltip_ref;

		Params();
	};

	LLCommand(const LLCommand::Params& p);

	const bool availableInToybox() const { return mAvailableInToybox; }
	const std::string& functionName() const { return mFunction; }
	const std::string& icon() const { return mIcon; }
	const LLCommandId& id() const { return mIdentifier; }
	const std::string& labelRef() const { return mLabelRef; }
	const LLSD& parameter() const { return mParameter; }
	const std::string& tooltipRef() const { return mTooltipRef; }

private:
	LLCommandId mIdentifier;

	bool        mAvailableInToybox;
	std::string mFunction;
	std::string mIcon;
	std::string mLabelRef;
	LLSD        mParameter;
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

	U32 commandCount() const;
	LLCommand * getCommand(U32 commandIndex);
	LLCommand * getCommand(const LLCommandId& commandId);
	LLCommand * getCommand(const LLUUID& commandUUID);

	static bool load();

protected:
	void addCommand(LLCommand * command);

private:
	typedef std::map<LLUUID, U32>	    CommandUUIDMap;
	typedef std::map<LLCommandId, U32>	CommandIndexMap;
	typedef std::vector<LLCommand *>	CommandVector;
	
	CommandVector	mCommands;
	CommandIndexMap	mCommandIndices;
	CommandUUIDMap	mCommandUUIDs;
};


#endif // LL_LLCOMMANDMANAGER_H
