/**
 * @file llurlhistory.cpp
 * @brief Manages a list of recent URLs
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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
	LLSD::array_iterator iter = sHistorySD[collection].beginArray();
	LLSD::array_iterator end = sHistorySD[collection].endArray();
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

