/** 
 * @file lltextvalidate.cpp
 * @brief Text validation helper functions
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

// Text editor widget to let users enter a single line.

#include "linden_common.h"
 
#include "lltextvalidate.h"
#include "llresmgr.h" // for LLLocale

namespace LLTextValidate
{
	void ValidateTextNamedFuncs::declareValues()
	{
		declare("ascii", validateASCII);
		declare("float", validateFloat);
		declare("int", validateInt);
		declare("positive_s32", validatePositiveS32);
		declare("non_negative_s32", validateNonNegativeS32);
		declare("alpha_num", validateAlphaNum);
		declare("alpha_num_space", validateAlphaNumSpace);
		declare("ascii_printable_no_pipe", validateASCIIPrintableNoPipe);
		declare("ascii_printable_no_space", validateASCIIPrintableNoSpace);
		declare("ascii_with_newline", validateASCIIWithNewLine);
	}

	// Limits what characters can be used to [1234567890.-] with [-] only valid in the first position.
	// Does NOT ensure that the string is a well-formed number--that's the job of post-validation--for
	// the simple reasons that intermediate states may be invalid even if the final result is valid.
	// 
	bool validateFloat(const LLWString &str)
	{
		LLLocale locale(LLLocale::USER_LOCALE);

		bool success = TRUE;
		LLWString trimmed = str;
		LLWStringUtil::trim(trimmed);
		S32 len = trimmed.length();
		if( 0 < len )
		{
			// May be a comma or period, depending on the locale
			llwchar decimal_point = (llwchar)LLResMgr::getInstance()->getDecimalPoint();

			S32 i = 0;

			// First character can be a negative sign
			if( '-' == trimmed[0] )
			{
				i++;
			}

			for( ; i < len; i++ )
			{
				if( (decimal_point != trimmed[i] ) && !LLStringOps::isDigit( trimmed[i] ) )
				{
					success = FALSE;
					break;
				}
			}
		}		

		return success;
	}

	// Limits what characters can be used to [1234567890-] with [-] only valid in the first position.
	// Does NOT ensure that the string is a well-formed number--that's the job of post-validation--for
	// the simple reasons that intermediate states may be invalid even if the final result is valid.
	//
	bool validateInt(const LLWString &str)
	{
		LLLocale locale(LLLocale::USER_LOCALE);

		bool success = TRUE;
		LLWString trimmed = str;
		LLWStringUtil::trim(trimmed);
		S32 len = trimmed.length();
		if( 0 < len )
		{
			S32 i = 0;

			// First character can be a negative sign
			if( '-' == trimmed[0] )
			{
				i++;
			}

			for( ; i < len; i++ )
			{
				if( !LLStringOps::isDigit( trimmed[i] ) )
				{
					success = FALSE;
					break;
				}
			}
		}		

		return success;
	}

	bool validatePositiveS32(const LLWString &str)
	{
		LLLocale locale(LLLocale::USER_LOCALE);

		LLWString trimmed = str;
		LLWStringUtil::trim(trimmed);
		S32 len = trimmed.length();
		bool success = TRUE;
		if(0 < len)
		{
			if(('-' == trimmed[0]) || ('0' == trimmed[0]))
			{
				success = FALSE;
			}
			S32 i = 0;
			while(success && (i < len))
			{
				if(!LLStringOps::isDigit(trimmed[i++]))
				{
					success = FALSE;
				}
			}
		}
		if (success)
		{
			S32 val = strtol(wstring_to_utf8str(trimmed).c_str(), NULL, 10);
			if (val <= 0)
			{
				success = FALSE;
			}
		}
		return success;
	}

	bool validateNonNegativeS32(const LLWString &str)
	{
		LLLocale locale(LLLocale::USER_LOCALE);

		LLWString trimmed = str;
		LLWStringUtil::trim(trimmed);
		S32 len = trimmed.length();
		bool success = TRUE;
		if(0 < len)
		{
			if('-' == trimmed[0])
			{
				success = FALSE;
			}
			S32 i = 0;
			while(success && (i < len))
			{
				if(!LLStringOps::isDigit(trimmed[i++]))
				{
					success = FALSE;
				}
			}
		}
		if (success)
		{
			S32 val = strtol(wstring_to_utf8str(trimmed).c_str(), NULL, 10);
			if (val < 0)
			{
				success = FALSE;
			}
		}
		return success;
	}

	bool validateNonNegativeS32NoSpace(const LLWString &str)
	{
		LLLocale locale(LLLocale::USER_LOCALE);

		LLWString test_str = str;
		S32 len = test_str.length();
		bool success = TRUE;
		if(0 < len)
		{
			if('-' == test_str[0])
			{
				success = FALSE;
			}
			S32 i = 0;
			while(success && (i < len))
			{
				if(!LLStringOps::isDigit(test_str[i]) || LLStringOps::isSpace(test_str[i++]))
				{
					success = FALSE;
				}
			}
		}
		if (success)
		{
			S32 val = strtol(wstring_to_utf8str(test_str).c_str(), NULL, 10);
			if (val < 0)
			{
				success = FALSE;
			}
		}
		return success;
	}

	bool validateAlphaNum(const LLWString &str)
	{
		LLLocale locale(LLLocale::USER_LOCALE);

		bool rv = TRUE;
		S32 len = str.length();
		if(len == 0) return rv;
		while(len--)
		{
			if( !LLStringOps::isAlnum((char)str[len]) )
			{
				rv = FALSE;
				break;
			}
		}
		return rv;
	}

	bool validateAlphaNumSpace(const LLWString &str)
	{
		LLLocale locale(LLLocale::USER_LOCALE);

		bool rv = TRUE;
		S32 len = str.length();
		if(len == 0) return rv;
		while(len--)
		{
			if(!(LLStringOps::isAlnum((char)str[len]) || (' ' == str[len])))
			{
				rv = FALSE;
				break;
			}
		}
		return rv;
	}

	// Used for most names of things stored on the server, due to old file-formats
	// that used the pipe (|) for multiline text storage.  Examples include
	// inventory item names, parcel names, object names, etc.
	bool validateASCIIPrintableNoPipe(const LLWString &str)
	{
		bool rv = TRUE;
		S32 len = str.length();
		if(len == 0) return rv;
		while(len--)
		{
			llwchar wc = str[len];
			if (wc < 0x20
				|| wc > 0x7f
				|| wc == '|')
			{
				rv = FALSE;
				break;
			}
			if(!(wc == ' '
				 || LLStringOps::isAlnum((char)wc)
				 || LLStringOps::isPunct((char)wc) ) )
			{
				rv = FALSE;
				break;
			}
		}
		return rv;
	}


	// Used for avatar names
	bool validateASCIIPrintableNoSpace(const LLWString &str)
	{
		bool rv = TRUE;
		S32 len = str.length();
		if(len == 0) return rv;
		while(len--)
		{
			llwchar wc = str[len];
			if (wc < 0x20
				|| wc > 0x7f
				|| LLStringOps::isSpace(wc))
			{
				rv = FALSE;
				break;
			}
			if( !(LLStringOps::isAlnum((char)str[len]) ||
				  LLStringOps::isPunct((char)str[len]) ) )
			{
				rv = FALSE;
				break;
			}
		}
		return rv;
	}

	bool validateASCII(const LLWString &str)
	{
		bool rv = TRUE;
		S32 len = str.length();
		while(len--)
		{
			if (str[len] < 0x20 || str[len] > 0x7f)
			{
				rv = FALSE;
				break;
			}
		}
		return rv;
	}

	// Used for multiline text stored on the server.
	// Example is landmark description in Places SP.
	bool validateASCIIWithNewLine(const LLWString &str)
	{
		bool rv = TRUE;
		S32 len = str.length();
		while(len--)
		{
			if (str[len] < 0x20 && str[len] != 0xA || str[len] > 0x7f)
			{
				rv = FALSE;
				break;
			}
		}
		return rv;
	}
}
