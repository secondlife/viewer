/** 
 * @file llkeywords.cpp
 * @brief Keyword list for LSL
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

#include "linden_common.h"

#include <iostream>
#include <fstream>

#include "llkeywords.h"
#include "lltexteditor.h"
#include "llstl.h"
#include <boost/tokenizer.hpp>

const U32 KEYWORD_FILE_CURRENT_VERSION = 2;

inline BOOL LLKeywordToken::isHead(const llwchar* s) const
{
	// strncmp is much faster than string compare
	BOOL res = TRUE;
	const llwchar* t = mToken.c_str();
	S32 len = mToken.size();
	for (S32 i=0; i<len; i++)
	{
		if (s[i] != t[i])
		{
			res = FALSE;
			break;
		}
	}
	return res;
}

LLKeywords::LLKeywords() : mLoaded(FALSE)
{
}

LLKeywords::~LLKeywords()
{
	std::for_each(mWordTokenMap.begin(), mWordTokenMap.end(), DeletePairedPointer());
	std::for_each(mLineTokenList.begin(), mLineTokenList.end(), DeletePointer());
	std::for_each(mDelimiterTokenList.begin(), mDelimiterTokenList.end(), DeletePointer());
}

BOOL LLKeywords::loadFromFile( const std::string& filename )
{
	mLoaded = FALSE;

	////////////////////////////////////////////////////////////
	// File header

	const S32 BUFFER_SIZE = 1024;
	char	buffer[BUFFER_SIZE];	/* Flawfinder: ignore */

	llifstream file;
	file.open(filename);	/* Flawfinder: ignore */
	if( file.fail() )
	{
		llinfos << "LLKeywords::loadFromFile()  Unable to open file: " << filename << llendl;
		return mLoaded;
	}

	// Identifying string
	file >> buffer;
	if( strcmp( buffer, "llkeywords" ) )
	{
		llinfos << filename << " does not appear to be a keyword file" << llendl;
		return mLoaded;
	}

	// Check file version
	file >> buffer;
	U32	version_num;
	file >> version_num;
	if( strcmp(buffer, "version") || version_num != (U32)KEYWORD_FILE_CURRENT_VERSION )
	{
		llinfos << filename << " does not appear to be a version " << KEYWORD_FILE_CURRENT_VERSION << " keyword file" << llendl;
		return mLoaded;
	}

	// start of line (SOL)
	std::string SOL_COMMENT("#");
	std::string SOL_WORD("[word ");
	std::string SOL_LINE("[line ");
	std::string SOL_ONE_SIDED_DELIMITER("[one_sided_delimiter ");
	std::string SOL_TWO_SIDED_DELIMITER("[two_sided_delimiter ");

	LLColor3 cur_color( 1, 0, 0 );
	LLKeywordToken::TOKEN_TYPE cur_type = LLKeywordToken::WORD;

	while (!file.eof())
	{
		buffer[0] = 0;
		file.getline( buffer, BUFFER_SIZE );
		std::string line(buffer);
		if( line.find(SOL_COMMENT) == 0 )
		{
			continue;
		}
		else if( line.find(SOL_WORD) == 0 )
		{
			cur_color = readColor( line.substr(SOL_WORD.size()) );
			cur_type = LLKeywordToken::WORD;
			continue;
		}
		else if( line.find(SOL_LINE) == 0 )
		{
			cur_color = readColor( line.substr(SOL_LINE.size()) );
			cur_type = LLKeywordToken::LINE;
			continue;
		}
		else if( line.find(SOL_TWO_SIDED_DELIMITER) == 0 )
		{
			cur_color = readColor( line.substr(SOL_TWO_SIDED_DELIMITER.size()) );
			cur_type = LLKeywordToken::TWO_SIDED_DELIMITER;
			continue;
		}
		else if( line.find(SOL_ONE_SIDED_DELIMITER) == 0 )	
		{
			cur_color = readColor( line.substr(SOL_ONE_SIDED_DELIMITER.size()) );
			cur_type = LLKeywordToken::ONE_SIDED_DELIMITER;
			continue;
		}

		std::string token_buffer( line );
		LLStringUtil::trim(token_buffer);
		
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep_word("", " \t");
		tokenizer word_tokens(token_buffer, sep_word);
		tokenizer::iterator token_word_iter = word_tokens.begin();

		if( !token_buffer.empty() && token_word_iter != word_tokens.end() )
		{
			// first word is keyword
			std::string keyword = (*token_word_iter);
			LLStringUtil::trim(keyword);

			// following words are tooltip
			std::string tool_tip;
			while (++token_word_iter != word_tokens.end())
			{
				tool_tip += (*token_word_iter);
			}
			LLStringUtil::trim(tool_tip);
			
			if( !tool_tip.empty() )
			{
				// Replace : with \n for multi-line tool tips.
				LLStringUtil::replaceChar( tool_tip, ':', '\n' );
				addToken(cur_type, keyword, cur_color, tool_tip );
			}
			else
			{
				addToken(cur_type, keyword, cur_color, LLStringUtil::null );
			}
		}
	}

	file.close();

	mLoaded = TRUE;
	return mLoaded;
}

// Add the token as described
void LLKeywords::addToken(LLKeywordToken::TOKEN_TYPE type,
						  const std::string& key_in,
						  const LLColor3& color,
						  const std::string& tool_tip_in )
{
	LLWString key = utf8str_to_wstring(key_in);
	LLWString tool_tip = utf8str_to_wstring(tool_tip_in);
	switch(type)
	{
	case LLKeywordToken::WORD:
		mWordTokenMap[key] = new LLKeywordToken(type, color, key, tool_tip);
		break;

	case LLKeywordToken::LINE:
		mLineTokenList.push_front(new LLKeywordToken(type, color, key, tool_tip));
		break;

	case LLKeywordToken::TWO_SIDED_DELIMITER:
	case LLKeywordToken::ONE_SIDED_DELIMITER:
		mDelimiterTokenList.push_front(new LLKeywordToken(type, color, key, tool_tip));
		break;

	default:
		llassert(0);
	}
}

LLColor3 LLKeywords::readColor( const std::string& s )
{
	F32 r, g, b;
	r = g = b = 0.0f;
	S32 values_read = sscanf(s.c_str(), "%f, %f, %f]", &r, &g, &b );
	if( values_read != 3 )
	{
		llinfos << " poorly formed color in keyword file" << llendl;
	}
	return LLColor3( r, g, b );
}

LLFastTimerUtil::DeclareTimer FTM_SYNTAX_COLORING("Syntax Coloring");

// Walk through a string, applying the rules specified by the keyword token list and
// create a list of color segments.
void LLKeywords::findSegments(std::vector<LLTextSegmentPtr>* seg_list, const LLWString& wtext, const LLColor4 &defaultColor, LLTextEditor& editor)
{
	LLFastTimer ft(FTM_SYNTAX_COLORING);
	seg_list->clear();

	if( wtext.empty() )
	{
		return;
	}
	
	S32 text_len = wtext.size() + 1;

	seg_list->push_back( new LLNormalTextSegment( defaultColor, 0, text_len, editor ) ); 

	const llwchar* base = wtext.c_str();
	const llwchar* cur = base;
	const llwchar* line = NULL;

	while( *cur )
	{
		if( *cur == '\n' || cur == base )
		{
			if( *cur == '\n' )
			{
				cur++;
				if( !*cur || *cur == '\n' )
				{
					continue;
				}
			}

			// Start of a new line
			line = cur;

			// Skip white space
			while( *cur && isspace(*cur) && (*cur != '\n')  )
			{
				cur++;
			}
			if( !*cur || *cur == '\n' )
			{
				continue;
			}

			// cur is now at the first non-whitespace character of a new line	
		
			// Line start tokens
			{
				BOOL line_done = FALSE;
				for (token_list_t::iterator iter = mLineTokenList.begin();
					 iter != mLineTokenList.end(); ++iter)
				{
					LLKeywordToken* cur_token = *iter;
					if( cur_token->isHead( cur ) )
					{
						S32 seg_start = cur - base;
						while( *cur && *cur != '\n' )
						{
							// skip the rest of the line
							cur++;
						}
						S32 seg_end = cur - base;
						
						LLTextSegmentPtr text_segment = new LLNormalTextSegment( cur_token->getColor(), seg_start, seg_end, editor );
						text_segment->setToken( cur_token );
						insertSegment( seg_list, text_segment, text_len, defaultColor, editor);
						line_done = TRUE; // to break out of second loop.
						break;
					}
				}

				if( line_done )
				{
					continue;
				}
			}
		}

		// Skip white space
		while( *cur && isspace(*cur) && (*cur != '\n')  )
		{
			cur++;
		}

		while( *cur && *cur != '\n' )
		{
			// Check against delimiters
			{
				S32 seg_start = 0;
				LLKeywordToken* cur_delimiter = NULL;
				for (token_list_t::iterator iter = mDelimiterTokenList.begin();
					 iter != mDelimiterTokenList.end(); ++iter)
				{
					LLKeywordToken* delimiter = *iter;
					if( delimiter->isHead( cur ) )
					{
						cur_delimiter = delimiter;
						break;
					}
				}

				if( cur_delimiter )
				{
					S32 between_delimiters = 0;
					S32 seg_end = 0;

					seg_start = cur - base;
					cur += cur_delimiter->getLength();
					
					if( cur_delimiter->getType() == LLKeywordToken::TWO_SIDED_DELIMITER )
					{
						while( *cur && !cur_delimiter->isHead(cur))
						{
							// Check for an escape sequence.
							if (*cur == '\\')
							{
								// Count the number of backslashes.
								S32 num_backslashes = 0;
								while (*cur == '\\')
								{
									num_backslashes++;
									between_delimiters++;
									cur++;
								}
								// Is the next character the end delimiter?
								if (cur_delimiter->isHead(cur))
								{
									// Is there was an odd number of backslashes, then this delimiter
									// does not end the sequence.
									if (num_backslashes % 2 == 1)
									{
										between_delimiters++;
										cur++;
									}
									else
									{
										// This is an end delimiter.
										break;
									}
								}
							}
							else
							{
								between_delimiters++;
								cur++;
							}
						}

						if( *cur )
						{
							cur += cur_delimiter->getLength();
							seg_end = seg_start + between_delimiters + 2 * cur_delimiter->getLength();
						}
						else
						{
							// eof
							seg_end = seg_start + between_delimiters + cur_delimiter->getLength();
						}
					}
					else
					{
						llassert( cur_delimiter->getType() == LLKeywordToken::ONE_SIDED_DELIMITER );
						// Left side is the delimiter.  Right side is eol or eof.
						while( *cur && ('\n' != *cur) )
						{
							between_delimiters++;
							cur++;
						}
						seg_end = seg_start + between_delimiters + cur_delimiter->getLength();
					}


					LLTextSegmentPtr text_segment = new LLNormalTextSegment( cur_delimiter->getColor(), seg_start, seg_end, editor );
					text_segment->setToken( cur_delimiter );
					insertSegment( seg_list, text_segment, text_len, defaultColor, editor);

					// Note: we don't increment cur, since the end of one delimited seg may be immediately
					// followed by the start of another one.
					continue;
				}
			}

			// check against words
			llwchar prev = cur > base ? *(cur-1) : 0;
			if( !isalnum( prev ) && (prev != '_') )
			{
				const llwchar* p = cur;
				while( isalnum( *p ) || (*p == '_') )
				{
					p++;
				}
				S32 seg_len = p - cur;
				if( seg_len > 0 )
				{
					LLWString word( cur, 0, seg_len );
					word_token_map_t::iterator map_iter = mWordTokenMap.find(word);
					if( map_iter != mWordTokenMap.end() )
					{
						LLKeywordToken* cur_token = map_iter->second;
						S32 seg_start = cur - base;
						S32 seg_end = seg_start + seg_len;

						// llinfos << "Seg: [" << word.c_str() << "]" << llendl;


						LLTextSegmentPtr text_segment = new LLNormalTextSegment( cur_token->getColor(), seg_start, seg_end, editor );
						text_segment->setToken( cur_token );
						insertSegment( seg_list, text_segment, text_len, defaultColor, editor);
					}
					cur += seg_len; 
					continue;
				}
			}

			if( *cur && *cur != '\n' )
			{
				cur++;
			}
		}
	}
}

void LLKeywords::insertSegment(std::vector<LLTextSegmentPtr>* seg_list, LLTextSegmentPtr new_segment, S32 text_len, const LLColor4 &defaultColor, LLTextEditor& editor )
{
	LLTextSegmentPtr last = seg_list->back();
	S32 new_seg_end = new_segment->getEnd();

	if( new_segment->getStart() == last->getStart() )
	{
		seg_list->pop_back();
	}
	else
	{
		last->setEnd( new_segment->getStart() );
	}
	seg_list->push_back( new_segment );

	if( new_seg_end < text_len )
	{
		seg_list->push_back( new LLNormalTextSegment( defaultColor, new_seg_end, text_len, editor ) );
	}
}

#ifdef _DEBUG
void LLKeywords::dump()
{
	llinfos << "LLKeywords" << llendl;


	llinfos << "LLKeywords::sWordTokenMap" << llendl;
	word_token_map_t::iterator word_token_iter = mWordTokenMap.begin();
	while( word_token_iter != mWordTokenMap.end() )
	{
		LLKeywordToken* word_token = word_token_iter->second;
		word_token->dump();
		++word_token_iter;
	}

	llinfos << "LLKeywords::sLineTokenList" << llendl;
	for (token_list_t::iterator iter = mLineTokenList.begin();
		 iter != mLineTokenList.end(); ++iter)
	{
		LLKeywordToken* line_token = *iter;
		line_token->dump();
	}


	llinfos << "LLKeywords::sDelimiterTokenList" << llendl;
	for (token_list_t::iterator iter = mDelimiterTokenList.begin();
		 iter != mDelimiterTokenList.end(); ++iter)
	{
		LLKeywordToken* delimiter_token = *iter;
		delimiter_token->dump();
	}
}

void LLKeywordToken::dump()
{
	llinfos << "[" << 
		mColor.mV[VX] << ", " <<
		mColor.mV[VY] << ", " <<
		mColor.mV[VZ] << "] [" <<
		wstring_to_utf8str(mToken) << "]" <<
		llendl;
}

#endif  // DEBUG
