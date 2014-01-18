/*
 *  LLCalcParser.cpp
 *  Copyright 2008 Aimee Walton.
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2008, Linden Research, Inc.
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
 *
 */

#include "linden_common.h"

#include "llcalcparser.h"
using namespace boost::spirit::classic;

F32 LLCalcParser::lookup(const std::string::iterator& start, const std::string::iterator& end) const
{
	LLCalc::calc_map_t::iterator iter;

	std::string name(start, end);
	
	if (mConstants)
	{
		iter = mConstants->find(name);
		if (iter != mConstants->end())
		{
			return (*iter).second;
		}
	}
	else
	{
		// This should never happen!
		throw_(end, std::string("Missing constants table"));
	}
	
	if (mVariables)
	{
		iter = mVariables->find(name);
		if (iter != mVariables->end())
		{
			return (*iter).second;
		}
	}
	
	throw_(end, std::string("Unknown symbol " + name));
	return 0.f;
}
