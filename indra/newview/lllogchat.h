/** 
 * @file lllogchat.h
 * @brief LLFloaterChat class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */


#ifndef LL_LLLOGCHAT_H
#define LL_LLLOGCHAT_H

class LLLogChat
{
public:
	static LLString timestamp(bool withdate = false);
	static LLString makeLogFileName(LLString(filename));
	static void saveHistory(LLString filename, LLString line);
	static void loadHistory(LLString filename, void (*callback)(LLString,void*),void* userdata);
};

#endif
