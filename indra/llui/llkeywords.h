/** 
 * @file llkeywords.h
 * @brief Keyword list for LSL
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#ifndef LL_LLKEYWORDS_H
#define LL_LLKEYWORDS_H


#include "llstring.h"
#include "v3color.h"
#include <map>
#include <list>
#include <deque>
#include "llpointer.h"

class LLTextSegment;
typedef LLPointer<LLTextSegment> LLTextSegmentPtr;

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

	S32					getLength() const		{ return mToken.size(); }
	BOOL				isHead(const llwchar* s) const;
	const LLWString&	getToken() const		{ return mToken; }
	const LLColor3&		getColor() const		{ return mColor; }
	TOKEN_TYPE			getType()  const		{ return mType; }
	const LLWString&	getToolTip() const		{ return mToolTip; }

#ifdef _DEBUG
	void		dump();
#endif

private:
	TOKEN_TYPE	mType;
	LLWString	mToken;
	LLColor3	mColor;
	LLWString	mToolTip;
};

class LLKeywords
{
public:
	LLKeywords();
	~LLKeywords();

	BOOL		loadFromFile(const std::string& filename);
	BOOL		isLoaded() const	{ return mLoaded; }

	void		findSegments(std::vector<LLTextSegmentPtr> *seg_list, const LLWString& text, const LLColor4 &defaultColor, class LLTextEditor& editor );

	// Add the token as described
	void addToken(LLKeywordToken::TOKEN_TYPE type,
					const std::string& key,
					const LLColor3& color,
					const std::string& tool_tip = LLStringUtil::null);
	
	// This class is here as a performance optimization.
	// The word token map used to be defined as std::map<LLWString, LLKeywordToken*>.
	// This worked, but caused a performance bottleneck due to memory allocation and string copies
	//  because it's not possible to search such a map without creating an LLWString.
	// Using this class as the map index instead allows us to search using segments of an existing
	//  text run without copying them first, which greatly reduces overhead in LLKeywords::findSegments().
	class WStringMapIndex
	{
	public:
		// copy constructor
		WStringMapIndex(const WStringMapIndex& other);
		// constructor from a string (copies the string's data into the new object)
		WStringMapIndex(const LLWString& str);
		// constructor from pointer and length
		// NOTE: does NOT copy data, caller must ensure that the lifetime of the pointer exceeds that of the new object!
		WStringMapIndex(const llwchar *start, size_t length);
		~WStringMapIndex();
		bool operator<(const WStringMapIndex &other) const;
	private:
		void copyData(const llwchar *start, size_t length);
		const llwchar *mData;
		size_t mLength;
		bool mOwner;
	};

	typedef std::map<WStringMapIndex, LLKeywordToken*> word_token_map_t;
	typedef word_token_map_t::const_iterator keyword_iterator_t;
	keyword_iterator_t begin() const { return mWordTokenMap.begin(); }
	keyword_iterator_t end() const { return mWordTokenMap.end(); }

#ifdef _DEBUG
	void		dump();
#endif

private:
	LLColor3	readColor(const std::string& s);
	void		insertSegment(std::vector<LLTextSegmentPtr>& seg_list, LLTextSegmentPtr new_segment, S32 text_len, const LLColor4 &defaultColor, class LLTextEditor& editor);
	void		insertSegments(const LLWString& wtext, std::vector<LLTextSegmentPtr>& seg_list, LLKeywordToken* token, S32 text_len, S32 seg_start, S32 seg_end, const LLColor4 &defaultColor, LLTextEditor& editor);

	BOOL		mLoaded;
	word_token_map_t mWordTokenMap;
	typedef std::deque<LLKeywordToken*> token_list_t;
	token_list_t mLineTokenList;
	token_list_t mDelimiterTokenList;
};

#endif  // LL_LLKEYWORDS_H
