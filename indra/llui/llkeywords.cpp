/**
 * @file llkeywords.cpp
 * @brief Keyword list for LSL
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "linden_common.h"

#include <iostream>
#include <fstream>

#include "llkeywords.h"
#include "llsdserialize.h"
#include "lltexteditor.h"
#include "llstl.h"

inline bool LLKeywordToken::isHead(const llwchar* s) const
{
	// strncmp is much faster than string compare
	bool res = TRUE;
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

inline bool LLKeywordToken::isTail(const llwchar* s) const
{
	bool res = TRUE;
	const llwchar* t = mDelimiter.c_str();
	S32 len = mDelimiter.size();
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

LLKeywords::LLKeywords() :
	mLoaded(false)
{
}

LLKeywords::~LLKeywords()
{
	std::for_each(mWordTokenMap.begin(), mWordTokenMap.end(), DeletePairedPointer());
	std::for_each(mLineTokenList.begin(), mLineTokenList.end(), DeletePointer());
	std::for_each(mDelimiterTokenList.begin(), mDelimiterTokenList.end(), DeletePointer());
}

void LLKeywords::addColorGroup(const std::string key_in, const LLColor4 color)
{
	WStringMapIndex key ( utf8str_to_wstring(key_in) );
	mColorGroupMap[key] = color;
}

// Add the token as described
void LLKeywords::addToken(LLKeywordToken::TOKEN_TYPE type,
						  const std::string& key_in,
						  const LLColor4& color,
						  const std::string& tool_tip_in,
						  const std::string& delimiter_in)
{
	std::string tip_text = tool_tip_in;
	LLStringUtil::replaceString(tip_text, "\\n", "\n" );
	LLStringUtil::replaceString(tip_text, "\t", " " );
	if (tip_text == "")
	{
		tip_text = "[no info]";
	}
	LLWString tool_tip = utf8str_to_wstring(tip_text);

	LLWString key = utf8str_to_wstring(key_in);
	LLWString delimiter = utf8str_to_wstring(delimiter_in);
	switch(type)
	{
	case LLKeywordToken::TT_CONSTANT:
	case LLKeywordToken::TT_CONTROL:
	case LLKeywordToken::TT_EVENT:
	case LLKeywordToken::TT_FUNCTION:
	case LLKeywordToken::TT_LABEL:
	case LLKeywordToken::TT_SECTION:
	case LLKeywordToken::TT_TYPE:
	case LLKeywordToken::TT_WORD:
		mWordTokenMap[key] = new LLKeywordToken(type, color, key, tool_tip, LLWStringUtil::null);
		break;

	case LLKeywordToken::TT_LINE:
		mLineTokenList.push_front(new LLKeywordToken(type, color, key, tool_tip, LLWStringUtil::null));
		break;

	case LLKeywordToken::TT_TWO_SIDED_DELIMITER:
	case LLKeywordToken::TT_DOUBLE_QUOTATION_MARKS:
	case LLKeywordToken::TT_ONE_SIDED_DELIMITER:
		mDelimiterTokenList.push_front(new LLKeywordToken(type, color, key, tool_tip, delimiter));
		break;

	default:
		llassert(0);
	}
}

void LLKeywords::clear()
{
	clearLoaded();
	mSyntax = LLSD();

	std::for_each(mWordTokenMap.begin(), mWordTokenMap.end(), DeletePairedPointer());
	std::for_each(mLineTokenList.begin(), mLineTokenList.end(), DeletePointer());
	std::for_each(mDelimiterTokenList.begin(), mDelimiterTokenList.end(), DeletePointer());
}

std::string LLKeywords::getArguments(LLSD& arguments)
{
	std::string argString = "";

	if (arguments.isArray())
	{
		U32 argsCount = arguments.size();
		LLSD::array_iterator arrayIt = arguments.beginArray();
		for ( ; arrayIt != arguments.endArray(); ++arrayIt)
		{
			LLSD& args = (*arrayIt);
			if (args.isMap())
			{
				LLSD::map_iterator argsIt = args.beginMap();
				for ( ; argsIt != args.endMap(); ++argsIt)
				{
					argString += argsIt->second.get("type").asString() + " " + argsIt->first;
					if (argsCount-- > 1)
					{
						argString += ", ";
					}
				}
			}
			else
			{
				LL_WARNS("SyntaxLSL")
						<< "Argument array comtains a non-map element!" << LL_ENDL;
			}
		}
	}
	else if (!arguments.isUndefined())
	{
		LL_WARNS("SyntaxLSL") << "Not an array! Invalid arguments LLSD passed to function." << arguments << LL_ENDL;
	}
	return argString == "" ? "" : argString;
}

std::string LLKeywords::getAttribute(const std::string& key)
{
	attribute_iterator_t it = mAttributes.find(key);
	return (it != mAttributes.end()) ? it->second : "";
}

LLColor4 LLKeywords::getColorGroup(const std::string key_in)
{
	std::string ColourGroup = "Black";
	if (key_in == "constants-float")
	{
		ColourGroup = "SyntaxLslConstantFloat";
	}
	else if (key_in == "constants-integer")
	{
		ColourGroup = "SyntaxLslConstantInteger";
	}
	else if (key_in == "constants-key")
	{
		ColourGroup = "SyntaxLslConstantKey";
	}
	else if (key_in == "constants-rotation")
	{
		ColourGroup = "SyntaxLslConstantRotation";
	}
	else if (key_in == "constants-string")
	{
		ColourGroup = "SyntaxLslConstantString";
	}
	else if (key_in == "constants-vector")
	{
		ColourGroup = "SyntaxLslConstantVector";
	}
	else if (key_in == "misc-flow-label")
	{
		ColourGroup = "SyntaxLslControlFlow";
	}
	else if (key_in =="deprecated")
	{
		ColourGroup = "SyntaxLslDeprecated";
	}
	else if (key_in == "events")
	{
		ColourGroup = "SyntaxLslEvent";
	}
	else if (key_in == "functions")
	{
		ColourGroup = "SyntaxLslFunction";
	}
	else if (key_in =="god-mode")
	{
		ColourGroup = "SyntaxLslGodMode";
	}
	else if (key_in == "types")
	{
		ColourGroup = "SyntaxLslDataType";
	}
	else if (key_in == "sections")
	{
		ColourGroup = "SyntaxLslSection";
	}
	else if (key_in == "misc-double_quotation_marks")
	{
		ColourGroup = "SyntaxLslStringLiteral";
	}
	else if (key_in == "misc-comments_1_sided")
	{
		ColourGroup = "SyntaxLslComment1Sided";
	}
	else if (key_in == "misc-comments_2_sided")
	{
		ColourGroup = "SyntaxLslComment2Sided";	
	}
	else
	{
		LL_WARNS("SyntaxLSL") << "Color key '" << key_in << "' not recognized!" << LL_ENDL;
	}

	return LLUIColorTable::instance().getColor(ColourGroup);
}

void LLKeywords::initialise(LLSD SyntaxXML)
{
	mSyntax = SyntaxXML;
	mLoaded = TRUE;
}

void LLKeywords::processTokens()
{
	// Add 'standard' stuff: Quotes, Comments, Strings, Labels, etc. before processing the LLSD
	std::string delimiter;
	addToken(LLKeywordToken::TT_LABEL, "@", getColorGroup("misc-flow-label"), "Label\nTarget for jump statement", delimiter );
	addToken(LLKeywordToken::TT_ONE_SIDED_DELIMITER, "//", getColorGroup("misc-comments_1_sided"), "Comment (single-line)\nNon-functional commentary or disabled code", delimiter );
	addToken(LLKeywordToken::TT_TWO_SIDED_DELIMITER, "/*", getColorGroup("misc-comments_2_sided"), "Comment (multi-line)\nNon-functional commentary or disabled code", "*/" );
	addToken(LLKeywordToken::TT_DOUBLE_QUOTATION_MARKS, "\"", getColorGroup("misc-double_quotation_marks"), "String literal", "\"" );

	LLSD::map_iterator outerIt = mSyntax.beginMap();
	for ( ; outerIt != mSyntax.endMap(); ++outerIt)
	{
		if (outerIt->first == "llsd-lsl-syntax-version")
		{
			LL_INFOS("SyntaxLSL") << "Skipping over version key." << LL_ENDL;
		}
		else
		{
			if (outerIt->second.isMap())
			{
				processTokensGroup(outerIt->second, outerIt->first);
			}
			else
			{
				LL_WARNS("LSL-Tokens-Processing") << "Map for " + outerIt->first + " entries is missing! Ignoring." << LL_ENDL;
			}
		}
	}
	LL_INFOS("SyntaxLSL") << "Finished processing tokens." << LL_ENDL;
}

void LLKeywords::processTokensGroup(LLSD& Tokens, const std::string Group)
{
	LLColor4 Color;
	LLColor4 ColorGroup;
	LLColor4 ColorDeprecated = getColorGroup("deprecated");
	LLColor4 ColorGM = getColorGroup("god-mode");

	LLKeywordToken::TOKEN_TYPE token_type = LLKeywordToken::TT_UNKNOWN;
	// If a new token type is added here, it must also be added to the 'addToken' method
	if (Group == "constants")
	{
		token_type = LLKeywordToken::TT_CONSTANT;
	}
	else if (Group == "controls")
	{
		token_type = LLKeywordToken::TT_CONTROL;
	}
	else if (Group == "events")
	{
		token_type = LLKeywordToken::TT_EVENT;
	}
	else if (Group == "functions")
	{
		token_type = LLKeywordToken::TT_FUNCTION;
	}
	else if (Group == "label")
	{
		token_type = LLKeywordToken::TT_LABEL;
	}
	else if (Group == "types")
	{
		token_type = LLKeywordToken::TT_TYPE;
	}

	ColorGroup = getColorGroup(Group);
	LL_INFOS("Tokens") << "Group: '" << Group << "', using colour: '" << ColorGroup << "'" << LL_ENDL;

	if (Tokens.isMap())
	{
		LLSD::map_iterator outerIt = Tokens.beginMap();
		for ( ; outerIt != Tokens.endMap(); ++outerIt)
		{
			if (outerIt->second.isMap())
			{
				mAttributes.clear();
				LLSD arguments = LLSD ();
				LLSD::map_iterator innerIt = outerIt->second.beginMap();
				for ( ; innerIt != outerIt->second.endMap(); ++innerIt)
				{
					if (innerIt->first == "arguments")
					{ 
						if (innerIt->second.isArray())
						{
							arguments = innerIt->second;
						}
					}
					else if (!innerIt->second.isMap() && !innerIt->second.isArray())
					{
						mAttributes[innerIt->first] = innerIt->second.asString();
					}
					else
					{
						LL_WARNS("SyntaxLSL") << "Not a valid attribute" << LL_ENDL;
					}
				}

				std::string tooltip = "";
				if (token_type == LLKeywordToken::TT_CONSTANT)
				{
					ColorGroup = getColorGroup(Group + "-" + getAttribute("type"));
					tooltip = "Type: " + getAttribute("type") + ", Value: " + getAttribute("value");
				}
				else if (token_type == LLKeywordToken::TT_EVENT)
				{
					tooltip = outerIt->first + "(" + getArguments(arguments) + ")";
				}
				else if (token_type == LLKeywordToken::TT_FUNCTION)
				{
					tooltip = getAttribute("return") + " " + outerIt->first + "(" + getArguments(arguments) + ");";
					tooltip += "\nEnergy: ";
					tooltip += getAttribute("energy") == "" ? "0.0" : getAttribute("energy");
					if (getAttribute("sleep") != "")
					{
						tooltip += ", Sleep: " + getAttribute("sleep");
					}
				}

				if (getAttribute("tooltip") != "")
				{
					if (tooltip != "")
					{
						tooltip += "\n";
					}
					tooltip += getAttribute("tooltip");
				}

				Color = getAttribute("deprecated") == "true" ? ColorDeprecated : ColorGroup;

				if (getAttribute("god-mode") == "true")
				{
					Color = ColorGM;
				}

				addToken(token_type, outerIt->first, Color, tooltip);
			}
		}
	}
	else if (Tokens.isArray())	// Currently nothing should need this, but it's here for completeness
	{
		LL_WARNS("SyntaxLSL") << "Curious, shouldn't be an array here; adding all using color " << Color << LL_ENDL;
		for (int count = 0; count < Tokens.size(); ++count)
		{
			addToken(token_type, Tokens[count], Color, "");
		}
	}
	else
	{
		LL_WARNS("Tokens") << "Invalid map/array passed: '" << Tokens << "'" << LL_ENDL;
	}
}

LLKeywords::WStringMapIndex::WStringMapIndex(const WStringMapIndex& other)
{
	if(other.mOwner)
	{
		copyData(other.mData, other.mLength);
	}
	else
	{
		mOwner = false;
		mLength = other.mLength;
		mData = other.mData;
	}
}

LLKeywords::WStringMapIndex::WStringMapIndex(const LLWString& str)
{
	copyData(str.data(), str.size());
}

LLKeywords::WStringMapIndex::WStringMapIndex(const llwchar *start, size_t length):
mData(start), mLength(length), mOwner(false)
{
}

LLKeywords::WStringMapIndex::~WStringMapIndex()
{
	if(mOwner)
		delete[] mData;
}

void LLKeywords::WStringMapIndex::copyData(const llwchar *start, size_t length)
{
	llwchar *data = new llwchar[length];
	memcpy((void*)data, (const void*)start, length * sizeof(llwchar));

	mOwner = true;
	mLength = length;
	mData = data;
}

bool LLKeywords::WStringMapIndex::operator<(const LLKeywords::WStringMapIndex &other) const
{
	// NOTE: Since this is only used to organize a std::map, it doesn't matter if it uses correct collate order or not.
	// The comparison only needs to strictly order all possible strings, and be stable.

	bool result = false;
	const llwchar* self_iter = mData;
	const llwchar* self_end = mData + mLength;
	const llwchar* other_iter = other.mData;
	const llwchar* other_end = other.mData + other.mLength;

	while(true)
	{
		if(other_iter >= other_end)
		{
			// We've hit the end of other.
			// This covers two cases: other being shorter than self, or the strings being equal.
			// In either case, we want to return false.
			result = false;
			break;
		}
		else if(self_iter >= self_end)
		{
			// self is shorter than other.
			result = true;
			break;
		}
		else if(*self_iter != *other_iter)
		{
			// The current character differs.  The strings are not equal.
			result = *self_iter < *other_iter;
			break;
		}

		self_iter++;
		other_iter++;
	}

	return result;
}

LLColor4 LLKeywords::readColor( const std::string& s )
{
	F32 r, g, b;
	r = g = b = 0.0f;
	S32 values_read = sscanf(s.c_str(), "%f, %f, %f]", &r, &g, &b );
	if( values_read != 3 )
	{
		llinfos << " poorly formed color in keyword file" << llendl;
	}
	return LLColor4( r, g, b, 1.f);
}

LLColor4 LLKeywords::readColor(LLSD& sd)
{
	if (sd.isArray())
	{
		return LLColor4 (sd, 1.f);
	}
	else if (sd.isMap())
	{
		return LLColor4 ( sd.get("x").asReal(), sd.get("y").asReal(), sd.get("z").asReal(), 1.f );
	}
	else
	{
		return LLColor4::black;
	}
}

LLFastTimer::DeclareTimer FTM_SYNTAX_COLORING("Syntax Coloring");

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
	while( *cur )
	{
		if( *cur == '\n' || cur == base )
		{
			if( *cur == '\n' )
			{
				LLTextSegmentPtr text_segment = new LLLineBreakTextSegment(cur-base);
				text_segment->setToken( 0 );
				insertSegment( *seg_list, text_segment, text_len, defaultColor, editor);
				cur++;
				if( !*cur || *cur == '\n' )
				{
					continue;
				}
			}

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

						//create segments from seg_start to seg_end
						insertSegments(wtext, *seg_list,cur_token, text_len, seg_start, seg_end, defaultColor, editor);
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
					cur += cur_delimiter->getLengthHead();

					LLKeywordToken::TOKEN_TYPE type = cur_delimiter->getType();
					if( type == LLKeywordToken::TT_TWO_SIDED_DELIMITER || type == LLKeywordToken::TT_DOUBLE_QUOTATION_MARKS )
					{
						while( *cur && !cur_delimiter->isTail(cur))
						{
							// Check for an escape sequence.
							if (type == LLKeywordToken::TT_DOUBLE_QUOTATION_MARKS && *cur == '\\')
							{
								// Count the number of backslashes.
								S32 num_backslashes = 0;
								while (*cur == '\\')
								{
									num_backslashes++;
									between_delimiters++;
									cur++;
								}
								// If the next character is the end delimiter?
								if (cur_delimiter->isTail(cur))
								{
									// If there was an odd number of backslashes, then this delimiter
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
							cur += cur_delimiter->getLengthHead();
							seg_end = seg_start + between_delimiters + cur_delimiter->getLengthHead() + cur_delimiter->getLengthTail();
						}
						else
						{
							// eof
							seg_end = seg_start + between_delimiters + cur_delimiter->getLengthHead();
						}
					}
					else
					{
						llassert( cur_delimiter->getType() == LLKeywordToken::TT_ONE_SIDED_DELIMITER );
						// Left side is the delimiter.  Right side is eol or eof.
						while( *cur && ('\n' != *cur) )
						{
							between_delimiters++;
							cur++;
						}
						seg_end = seg_start + between_delimiters + cur_delimiter->getLengthHead();
					}

					insertSegments(wtext, *seg_list,cur_delimiter, text_len, seg_start, seg_end, defaultColor, editor);
					/*
					LLTextSegmentPtr text_segment = new LLNormalTextSegment( cur_delimiter->getColor(), seg_start, seg_end, editor );
					text_segment->setToken( cur_delimiter );
					insertSegment( seg_list, text_segment, text_len, defaultColor, editor);
					*/
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
					WStringMapIndex word( cur, seg_len );
					word_token_map_t::iterator map_iter = mWordTokenMap.find(word);
					if( map_iter != mWordTokenMap.end() )
					{
						LLKeywordToken* cur_token = map_iter->second;
						S32 seg_start = cur - base;
						S32 seg_end = seg_start + seg_len;

						// llinfos << "Seg: [" << word.c_str() << "]" << llendl;

						insertSegments(wtext, *seg_list,cur_token, text_len, seg_start, seg_end, defaultColor, editor);
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

void LLKeywords::insertSegments(const LLWString& wtext, std::vector<LLTextSegmentPtr>& seg_list, LLKeywordToken* cur_token, S32 text_len, S32 seg_start, S32 seg_end, const LLColor4 &defaultColor, LLTextEditor& editor )
{
	std::string::size_type pos = wtext.find('\n',seg_start);

	while (pos!=-1 && pos < (std::string::size_type)seg_end)
	{
		if (pos!=seg_start)
		{
			LLTextSegmentPtr text_segment = new LLNormalTextSegment( cur_token->getColor(), seg_start, pos, editor );
			text_segment->setToken( cur_token );
			insertSegment( seg_list, text_segment, text_len, defaultColor, editor);
		}

		LLTextSegmentPtr text_segment = new LLLineBreakTextSegment(pos);
		text_segment->setToken( cur_token );
		insertSegment( seg_list, text_segment, text_len, defaultColor, editor);

		seg_start = pos+1;
		pos = wtext.find('\n',seg_start);
	}

	LLTextSegmentPtr text_segment = new LLNormalTextSegment( cur_token->getColor(), seg_start, seg_end, editor );
	text_segment->setToken( cur_token );
	insertSegment( seg_list, text_segment, text_len, defaultColor, editor);
}

void LLKeywords::insertSegment(std::vector<LLTextSegmentPtr>& seg_list, LLTextSegmentPtr new_segment, S32 text_len, const LLColor4 &defaultColor, LLTextEditor& editor )
{
	LLTextSegmentPtr last = seg_list.back();
	S32 new_seg_end = new_segment->getEnd();

	if( new_segment->getStart() == last->getStart() )
	{
		seg_list.pop_back();
	}
	else
	{
		last->setEnd( new_segment->getStart() );
	}
	seg_list.push_back( new_segment );

	if( new_seg_end < text_len )
	{
		seg_list.push_back( new LLNormalTextSegment( defaultColor, new_seg_end, text_len, editor ) );
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
