/** 
 * @file llmessagetemplateparser.h
 * @brief Classes to parse message template.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
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
