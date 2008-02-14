/** 
 * @file llurlhistory.cpp
 * @author Sam Kolb
 * @brief Manages a list of recent URLs
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#include "llviewerprecompiledheaders.h"

#include "llurlhistory.h"

#include "lldir.h"
#include "llsdserialize.h"

LLSD LLURLHistory::sHistorySD;

const int MAX_URL_COUNT = 10;

/////////////////////////////////////////////////////////////////////////////

// static
bool LLURLHistory::loadFile(const LLString& filename)
{
	LLSD data;
	{
		LLString temp_str = gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter();

		llifstream file((temp_str + filename).c_str());

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
			return false;
		}
	}
	sHistorySD = data;
	return true;
}

// static
bool LLURLHistory::saveFile(const LLString& filename)
{
	LLString temp_str = gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter();
	llofstream out((temp_str + filename).c_str());
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
	sHistorySD[collection].insert(0, url);
	LLURLHistory::limitSize(collection);
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

