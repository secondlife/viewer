/**
* @file lluiuisage.cpp
* @brief Source file for LLUIUsage
*
* $LicenseInfo:firstyear=2021&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2021, Linden Research, Inc.
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
#include "lluiusage.h"
#include <boost/algorithm/string.hpp>

LLUIUsage::LLUIUsage()
{
}

LLUIUsage::~LLUIUsage()
{
}

// static
std::string LLUIUsage::sanitized(const std::string& s)
{
	// ViewerStats db doesn't like "." in keys
	std::string result(s);
	std::replace(result.begin(), result.end(), '.', '_');
	std::replace(result.begin(), result.end(), ' ', '_');
	return result;
}

void LLUIUsage::setLLSDNested(LLSD& sd, const std::string& path, S32 max_elts, S32 val) const
{
	std::vector<std::string> fields;
	boost::split(fields, path, boost::is_any_of("/"));
	auto first_pos = std::max(fields.begin(), fields.end() - max_elts);
	auto end_pos = fields.end();
	std::vector<std::string> last_fields(first_pos,end_pos);

	// Code below is just to accomplish the equivalent of
	//   sd[last_fields[0]][last_fields[1]] = LLSD::Integer(val);
	// for an arbitrary number of fields.
	LLSD* fsd = &sd;
	for (auto it=last_fields.begin(); it!=last_fields.end(); ++it)
	{
		if (it == last_fields.end()-1)
		{
			(*fsd)[*it] = LLSD::Integer(val);
		}
		else
		{
			if (!(*fsd)[*it].isMap())
			{
				(*fsd)[*it] = LLSD::emptyMap();
			}
			fsd = &(*fsd)[*it];
		}
	}
}

void LLUIUsage::logFloater(const std::string& floater)
{
	mFloaterCounts[sanitized(floater)]++;
	LL_DEBUGS("UIUsage") << "floater " << floater << LL_ENDL;
}

void LLUIUsage::logCommand(const std::string& command)
{
	mCommandCounts[sanitized(command)]++;
	LL_DEBUGS("UIUsage") << "command " << command << LL_ENDL;
}

void LLUIUsage::logWidget(const std::string& w)
{
	mWidgetCounts[sanitized(w)]++;
	LL_DEBUGS("UIUsage") << "widget " << w << LL_ENDL;
}

LLSD LLUIUsage::asLLSD() const
{
	LLSD result;
	for (auto const& it : mFloaterCounts)
	{
		result["floaters"][it.first] = LLSD::Integer(it.second);
	}
	for (auto const& it : mCommandCounts)
	{
		result["commands"][it.first] = LLSD::Integer(it.second);
	}
	for (auto const& it : mWidgetCounts)
	{
		setLLSDNested(result["widgets"], it.first, 2, it.second);
	}
	return result;
}

