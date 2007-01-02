/** 
 * @file llkeywords.h
 * @brief Keyword list for LSL
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLKEYWORDS_H
#define LL_LLKEYWORDS_H


#include "llstring.h"
#include "v3color.h"
#include <map>
#include <list>
#include <deque>

class LLTextSegment;


class LLKeywordToken
{
public:
	enum TOKEN_TYPE { WORD, LINE, TWO_SIDED_DELIMITER, ONE_SIDED_DELIMITER };

	LLKeywordToken( TOKEN_TYPE type, const LLColor3& color, const LLWString& token, const LLWString& tool_tip ) 
		:
		mType( type ),
		mToken( token ),
		mColor( color ),
		mToolTip( tool_tip )
	{
	}

	S32					getLength()				{ return mToken.size(); }
	BOOL				isHead(const llwchar* s);
	const LLColor3&		getColor()				{ return mColor; }
	TOKEN_TYPE			getType()				{ return mType; }
	const LLWString&	getToolTip()			{ return mToolTip; }

#ifdef _DEBUG
	void		dump();
#endif

private:
	TOKEN_TYPE	mType;
public:
	LLWString	mToken;
	LLColor3	mColor;
private:
	LLWString	mToolTip;
};

class LLKeywords
{
public:
	LLKeywords();
	~LLKeywords();

	BOOL		loadFromFile(const LLString& filename);
	BOOL		isLoaded()	{ return mLoaded; }

	void		findSegments(std::vector<LLTextSegment *> *seg_list, const LLWString& text );

#ifdef _DEBUG
	void		dump();
#endif

	// Add the token as described
	void addToken(LLKeywordToken::TOKEN_TYPE type,
					const LLString& key,
					const LLColor3& color,
					const LLString& tool_tip = LLString::null);

private:
	LLColor3	readColor(const LLString& s);
	void		insertSegment(std::vector<LLTextSegment *> *seg_list, LLTextSegment* new_segment, S32 text_len);

private:
	BOOL						 mLoaded;
public:
	typedef std::map<LLWString, LLKeywordToken*> word_token_map_t;
	word_token_map_t mWordTokenMap;
private:
	typedef std::deque<LLKeywordToken*> token_list_t;
	token_list_t mLineTokenList;
	token_list_t mDelimiterTokenList;
};

#endif  // LL_LLKEYWORDS_H
