/** 
 * @file lltextutil.cpp
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

#include "lltextutil.h"

#include "lluicolor.h"
#include "lltextbox.h"


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

// EOF
