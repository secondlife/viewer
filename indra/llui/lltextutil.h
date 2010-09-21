/** 
 * @file lltextutil.h
 * @brief Misc text-related auxiliary methods
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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
