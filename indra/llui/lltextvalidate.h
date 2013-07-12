/** 
 * @file lltextbase.h
 * @author Martin Reddy
 * @brief The base class of text box/editor, providing Url handling support
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

#ifndef LL_LLTEXTVALIDATE_H
#define LL_LLTEXTVALIDATE_H

#include "llstring.h"
#include "llinitparam.h"
#include <boost/function.hpp>

namespace LLTextValidate
{
	typedef boost::function<BOOL (const LLWString &wstr)> validate_func_t;

	struct ValidateTextNamedFuncs
	:	public LLInitParam::TypeValuesHelper<validate_func_t, ValidateTextNamedFuncs>
	{
		static void declareValues();
	};

	bool	validateFloat(const LLWString &str );
	bool	validateInt(const LLWString &str );
	bool	validatePositiveS32(const LLWString &str);
	bool	validateNonNegativeS32(const LLWString &str);
	bool 	validateNonNegativeS32NoSpace(const LLWString &str);
	bool	validateAlphaNum(const LLWString &str );
	bool	validateAlphaNumSpace(const LLWString &str );
	bool	validateASCIIPrintableNoPipe(const LLWString &str); 
	bool	validateASCIIPrintableNoSpace(const LLWString &str);
	bool	validateASCII(const LLWString &str);
	bool	validateASCIIWithNewLine(const LLWString &str);
}


#endif
