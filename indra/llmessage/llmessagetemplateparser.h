/** 
 * @file llmessagetemplateparser.h
 * @brief Classes to parse message template.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_MESSAGETEMPLATEPARSER_H
#define LL_MESSAGETEMPLATEPARSER_H

#include <string>
#include "llmessagetemplate.h"

class LLTemplateTokenizer
{
public:
	LLTemplateTokenizer(const std::string & contents);

	U32 line() const;
	bool atEOF() const;
	std::string next();

	bool want(const std::string & token);
	bool wantEOF();
private:
	void inc();
	void dec();
	std::string get() const;
	void error(std::string message = "generic") const;

	struct positioned_token
	{
		std::string str;
		U32 line;
	};
	
	bool mStarted;
	std::list<positioned_token> mTokens;
	std::list<positioned_token>::const_iterator mCurrent;
};

class LLTemplateParser
{
public:
	typedef std::list<LLMessageTemplate *>::const_iterator message_iterator;
	
	static LLMessageTemplate * parseMessage(LLTemplateTokenizer & tokens);
	static LLMessageBlock * parseBlock(LLTemplateTokenizer & tokens);
	static LLMessageVariable * parseVariable(LLTemplateTokenizer & tokens);

	LLTemplateParser(LLTemplateTokenizer & tokens);
	message_iterator getMessagesBegin() const;
	message_iterator getMessagesEnd() const;
	F32 getVersion() const;
	
private:
	F32 mVersion;
	std::list<LLMessageTemplate *> mMessages;
};

#endif
