/** 
 * @file lltextutil.cpp
 * @brief Misc text-related auxiliary methods
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "lltextutil.h"

#include "lluicolor.h"
#include "lltextbox.h"
#include "llurlmatch.h"

#include "uriparser/Uri.h"

boost::function<bool(LLUrlMatch*,LLTextBase*)>	LLTextUtil::TextHelpers::iconCallbackCreationFunction = 0;

void LLTextUtil::textboxSetHighlightedVal(LLTextBox *txtbox, const LLStyle::Params& normal_style, const std::string& text, const std::string& hl)
{
	static LLUIColor sFilterTextColor = LLUIColorTable::instance().getColor("FilterTextColor", LLColor4::green);

	std::string text_uc = text;
	LLStringUtil::toUpper(text_uc);

	size_t hl_begin = 0, hl_len = hl.size();

	if (hl_len == 0 || (hl_begin = text_uc.find(hl)) == std::string::npos)
	{
		txtbox->setText(text, normal_style);
		return;
	}

	LLStyle::Params hl_style = normal_style;
	hl_style.color = sFilterTextColor;

	txtbox->setText(LLStringUtil::null); // clear text
	txtbox->appendText(text.substr(0, hl_begin),		false, normal_style);
	txtbox->appendText(text.substr(hl_begin, hl_len),	false, hl_style);
	txtbox->appendText(text.substr(hl_begin + hl_len),	false, normal_style);
}

const std::string& LLTextUtil::formatPhoneNumber(const std::string& phone_str)
{
	static const std::string PHONE_SEPARATOR = LLUI::sSettingGroups["config"]->getString("AvalinePhoneSeparator");
	static const S32 PHONE_PART_LEN = 2;

	static std::string formatted_phone_str;
	formatted_phone_str = phone_str;
	S32 separator_pos = (S32)(formatted_phone_str.size()) - PHONE_PART_LEN;
	for (; separator_pos >= PHONE_PART_LEN; separator_pos -= PHONE_PART_LEN)
	{
		formatted_phone_str.insert(separator_pos, PHONE_SEPARATOR);
	}

	return formatted_phone_str;
}

bool LLTextUtil::processUrlMatch(LLUrlMatch* match,LLTextBase* text_base, bool is_content_trusted)
{
	if (match == 0 || text_base == 0)
		return false;

	if(match->getID() != LLUUID::null && TextHelpers::iconCallbackCreationFunction)
	{
		bool segment_created = TextHelpers::iconCallbackCreationFunction(match,text_base);
		if(segment_created)
			return true;
	}

	// output an optional icon before the Url
	if (is_content_trusted && !match->getIcon().empty() )
	{
		LLUIImagePtr image = LLUI::getUIImage(match->getIcon());
		if (image)
		{
			LLStyle::Params icon;
			icon.image = image;
			// Text will be replaced during rendering with the icon,
			// but string cannot be empty or the segment won't be
			// added (or drawn).
			text_base->appendImageSegment(icon);

			return true;
		}
	}
	
	return false;
}

static void textRangeToString(UriTextRangeA& textRange, std::string& str)
{
	S32 len = textRange.afterLast - textRange.first;
	if (len)
	{
		str = textRange.first;
		str = str.substr(0, len);
	}
}

S32 LLTextUtil::normalizeUri(std::string& uri_string, Uri * urip/* = NULL*/)
{
	UriParserStateA state;
	UriUriA uri;
	state.uri = &uri;

	S32 res = uriParseUriA(&state, uri_string.c_str());

	if (!res)	
	{
		S32 len = uri.scheme.afterLast - uri.scheme.first;

		if (len > 0)
		{
			res = uriNormalizeSyntaxExA(&uri, URI_NORMALIZE_SCHEME | URI_NORMALIZE_HOST);

			if (!res)
			{
				S32 chars_required;
				res = uriToStringCharsRequiredA(&uri, &chars_required);

				if (!res)
				{
					chars_required++;
					std::vector<char> label_buf(chars_required);
					res = uriToStringA(&label_buf[0], &uri, chars_required, NULL);

					if (!res)
					{
						uri_string = &label_buf[0];
					}
				}
			}

			// fill urip if requested
			if (urip)
			{
				textRangeToString(uri.scheme, urip->scheme);
				textRangeToString(uri.hostText, urip->host);
				textRangeToString(uri.portText, urip->port);
				textRangeToString(uri.query, urip->query);
				textRangeToString(uri.fragment, urip->fragment);

				UriPathSegmentA * pathHead = uri.pathHead;
				while (pathHead)
				{
					std::string partOfPath;
					textRangeToString(pathHead->text, partOfPath);

					urip->path += '/';
					urip->path += partOfPath;

					pathHead = pathHead->next;
				}
			}
		}
		else if (uri_string.find_first_of('.') != std::string::npos)
		{
			static bool recursive_call = false;

			// allow only single level recursive call
			if (!recursive_call)
			{
				recursive_call = true;

				// force uri to be with scheme and try to normalize
				std::string uri_with_scheme = "http://";
				uri_with_scheme += uri_string;
				normalizeUri(uri_with_scheme, urip);
				uri_string = uri_with_scheme.substr(7);
				recursive_call = false;
			}		
		}
	}

	uriFreeUriMembersA(&uri);
	return res;
}

// EOF
