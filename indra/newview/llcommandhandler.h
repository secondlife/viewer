/**
 * @file llcommandhandler.h
 * @brief Central registry for text-driven "commands", most of
 * which manipulate user interface.  For example, the command
 * "agent (uuid) about" will open the UI for an avatar's profile.
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#ifndef LLCOMMANDHANDLER_H
#define LLCOMMANDHANDLER_H

/* To implement a command "foo" that takes one parameter,
   a UUID, do this:

class LLFooHandler : public LLCommandHandler
{
public:
    // Inform the system you handle commands starting
	// with "foo"
	LLFooHandler() : LLCommandHandler("foo") { }

    // Your code here
	bool handle(const std::vector<std::string>& tokens)
	{
		if (tokens.size() < 1) return false;
		LLUUID id( tokens[0] );
		return doFoo(id);
	}
};

// Creating the object registers with the dispatcher.
LLFooHandler gFooHandler;
*/

class LLCommandHandler
{
public:
	LLCommandHandler(const char* command);
		// Automatically registers object to get called when 
		// command is executed.
		
	virtual ~LLCommandHandler();

	virtual bool handle(const std::vector<std::string>& params) = 0;
		// Execute the command with a provided (possibly empty)
		// list of parameters.
		// Return true if you did something, false if the parameters
		// are invalid or on error.
};


class LLCommandDispatcher
{
public:
	static bool dispatch(const std::string& cmd, const std::vector<std::string>& params);
		// Execute a command registered via the above mechanism,
		// passing string parameters.
		// Returns true if command was found and executed correctly.
};

#endif
