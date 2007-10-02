/**
 * @file llcommandhandler.cpp
 * @brief Central registry for text-driven "commands", most of
 * which manipulate user interface.  For example, the command
 * "agent (uuid) about" will open the UI for an avatar's profile.
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#include "llviewerprecompiledheaders.h"

#include "llcommandhandler.h"

// system includes
#include <boost/tokenizer.hpp>

//---------------------------------------------------------------------------
// Underlying registry for command handlers, not directly accessible.
//---------------------------------------------------------------------------

class LLCommandHandlerRegistry
{
public:
	static LLCommandHandlerRegistry& instance();
	void add(const char* cmd, LLCommandHandler* handler);
	bool dispatch(const std::string& cmd, const std::vector<std::string>& params);

private:
	std::map<std::string, LLCommandHandler*> mMap;
};

// static 
LLCommandHandlerRegistry& LLCommandHandlerRegistry::instance()
{
	// Force this to be initialized on first call, because we're going
	// to be adding items to the std::map before main() and we can't
	// rely on a global being initialized in the right order.
	static LLCommandHandlerRegistry instance;
	return instance;
}

void LLCommandHandlerRegistry::add(const char* cmd, LLCommandHandler* handler)
{
	mMap[cmd] = handler;
}

bool LLCommandHandlerRegistry::dispatch(const std::string& cmd,
										const std::vector<std::string>& params)
{
	std::map<std::string, LLCommandHandler*>::iterator it = mMap.find(cmd);
	if (it == mMap.end()) return false;
	LLCommandHandler* handler = it->second;
	if (!handler) return false;
	return handler->handle(params);
}

//---------------------------------------------------------------------------
// Automatic registration of commands, runs before main()
//---------------------------------------------------------------------------

LLCommandHandler::LLCommandHandler(const char* cmd)
{
	LLCommandHandlerRegistry::instance().add(cmd, this);
}

LLCommandHandler::~LLCommandHandler()
{
	// Don't care about unregistering these, all the handlers
	// should be static objects.
}

//---------------------------------------------------------------------------
// Public interface
//---------------------------------------------------------------------------

// static
bool LLCommandDispatcher::dispatch(const std::string& cmd, const std::vector<std::string>& params)
{
	return LLCommandHandlerRegistry::instance().dispatch(cmd, params);
}
