/** 
 * @file llfloaterbuycurrency.h
 * @brief LLFloaterBuyCurrency class definition
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#ifndef LL_LLFLOATERBUYCURRENCY_H
#define LL_LLFLOATERBUYCURRENCY_H

#include "stdtypes.h"

class LLFloater;

class LLFloaterBuyCurrency
{
public:
	static void buyCurrency();
	static void buyCurrency(const std::string& name, S32 price);
		/* name should be a noun phrase of the object or service being bought:
				"That object costs"
				"Trying to give"
				"Uploading costs"
			a space and the price will be appended
		*/
	
	static LLFloater* buildFloater(const LLSD& key);
};


#endif
