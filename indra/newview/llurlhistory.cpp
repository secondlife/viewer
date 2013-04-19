/**
 * @file llurlhistory.cpp
 * @brief Manages a list of recent URLs
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llurlhistory.h"

#include "lldir.h"
#include "llsdserialize.h"

LLSD LLURLHistory::sHistorySD;

const int MAX_URL_COUNT = 10;

/////////////////////////////////////////////////////////////////////////////

// static
bool LLURLHistory::loadFile(const std::string& filename)
{
	LLSD data;
	{
		std::string temp_str = gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter();

		llifstream file((temp_str + filename));

		if (file.is_open())
		{
			llinfos << "Loading history.xml file at " << filename << llendl;
			LLSDSerialize::fromXML(data, file);
		}

		if (data.isUndefined())
		{
			llinfos << "file missing, ill-formed, "
				"or simply undefined; not changing the"
				" file" << llendl;
			sHistorySD = LLSD();
			return false;
		}
	}
	sHistorySD = data;
	return true;
}

// static
bool LLURLHistory::saveFile(const std::string& filename)
{	
	std::string temp_str = gDirUtilp->getLindenUserDir();
	if( temp_str.empty() )
	{
		llinfos << "Can't save URL history - no user directory set yet." << llendl;
		return false;
	}

	temp_str += gDirUtilp->getDirDelimiter() + filename;
	llofstream out(temp_str);
	if (!out.good())
	{
		llwarns << "Unable to open " << filename << " for output." << llendl;
		return false;
	}

	LLSDSerialize::toXML(sHistorySD, out);

	out.close();
	return true;
}
// static
// This function returns a portion of the history llsd that contains the collected
// url history
LLSD LLURLHistory::getURLHistory(const std::string& collection)
{
	if(sHistorySD.has(collection))
	{
		return sHistorySD[collection];
	}
	return LLSD();
}

// static
void LLURLHistory::addURL(const std::string& collection, const std::string& url)
{
	if(! url.empty())
	{
		sHistorySD[collection].insert(0, url);
		LLURLHistory::limitSize(collection);
	}
}
// static
void LLURLHistory::removeURL(const std::string& collection, const std::string& url)
{
	for(int index = 0; index < sHistorySD[collection].size(); index++)
	{
		if(sHistorySD[collection].get(index).asString() == url)
		{
			sHistorySD[collection].erase(index);
		}
	}
}

// static
void LLURLHistory::clear(const std::string& collection)
{
	sHistorySD[ collection ] = LLSD();
}

void LLURLHistory::limitSize(const std::string& collection)
{
	while(sHistorySD[collection].size() > MAX_URL_COUNT)
	{
		sHistorySD[collection].erase(MAX_URL_COUNT);
	}
}

