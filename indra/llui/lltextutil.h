/** 
 * @file lltextutil.h
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

#ifndef LL_LLTEXTUTIL_H
#define LL_LLTEXTUTIL_H

#include "llstyle.h"

class LLTextBox;
class LLUrlMatch;
class LLTextBase;

namespace LLTextUtil
{

	/**
	 * Set value for text box, highlighting substring hl_uc.
	 * 
	 * Used to highlight filter matches.
	 * 
	 * @param txtbox        Text box to set value for
	 * @param normal_style  Style to use for non-highlighted text
	 * @param text          Text to set
	 * @param hl            Upper-cased string to highlight
	 */
	void textboxSetHighlightedVal(
		LLTextBox *txtbox,
		const LLStyle::Params& normal_style,
		const std::string& text,
		const std::string& hl);

	/**
	 * Formats passed phone number to be more human readable.
	 *
	 * It just divides the number on parts by two digits from right to left. The first left part
	 * can have 2 or 3 digits, i.e. +44-33-33-44-55-66 or 12-34-56-78-90. Separator is set in
	 * application settings (AvalinePhoneSeparator)
	 *
	 * @param[in] phone_str string with original phone number
	 * @return reference to string with formatted phone number
	 */
	const std::string& formatPhoneNumber(const std::string& phone_str);

	bool processUrlMatch(LLUrlMatch* match,LLTextBase* text_base);

	class TextHelpers
	{

		//we need this special callback since we need to create LLAvataIconCtrls while parsing
		//avatar/group url but can't create LLAvataIconCtrl from LLUI
		public:
			static boost::function<bool(LLUrlMatch*,LLTextBase*)> iconCallbackCreationFunction;
	};

	
}

#endif // LL_LLTEXTUTIL_H
