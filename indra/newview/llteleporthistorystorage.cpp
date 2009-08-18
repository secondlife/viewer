/**
 * @file llteleporthistorystorage.cpp
 * @brief Teleport history
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

#include "llviewerprecompiledheaders.h"

#include "llteleporthistorystorage.h"

#include "llsd.h"
#include "llsdserialize.h"
#include "lldir.h"

static LLTeleportHistoryStorage tpstorage;

LLTeleportHistoryPersistentItem::LLTeleportHistoryPersistentItem(const LLSD& val)
{
	mTitle = val["title"].asString();
	mGlobalPos.setValue(val["global_pos"]);
	mDate = val["date"];
}

LLSD LLTeleportHistoryPersistentItem::toLLSD() const
{
	LLSD val;

	val["title"]		= mTitle;
	val["global_pos"]	= mGlobalPos.getValue();
	val["date"]		= mDate;

	return val;
}

LLTeleportHistoryStorage::LLTeleportHistoryStorage() :
	mFilename("teleport_history.txt")
{
}

LLTeleportHistoryStorage::~LLTeleportHistoryStorage()
{
}

void LLTeleportHistoryStorage::purgeItems()
{
	mItems.clear();
}

void LLTeleportHistoryStorage::addItem(const std::string title, const LLVector3d& global_pos)
{
	mItems.push_back(LLTeleportHistoryPersistentItem(title, global_pos));
}

void LLTeleportHistoryStorage::addItem(const std::string title, const LLVector3d& global_pos, const LLDate& date)
{
	mItems.push_back(LLTeleportHistoryPersistentItem(title, global_pos, date));
}

void LLTeleportHistoryStorage::removeItem(S32 idx)
{
	if (idx < 0 || idx >= (S32)mItems.size())
		return;

	mItems.erase (mItems.begin() + idx);
}

void LLTeleportHistoryStorage::save()
{
        // build filename for each user
	std::string resolvedFilename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, mFilename);

	// open the history file for writing
	llofstream file (resolvedFilename);
	if (!file.is_open())
	{
		llwarns << "can't open teleport history file \"" << mFilename << "\" for writing" << llendl;
		return;
	}

	for (size_t i=0; i<mItems.size(); i++)
	{
		LLSD s_item = mItems[i].toLLSD();
		file << LLSDOStreamer<LLSDNotationFormatter>(s_item) << std::endl;
	}

	file.close();
}

void LLTeleportHistoryStorage::load()
{
        // build filename for each user
	std::string resolved_filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, mFilename);

	// open the history file for reading
	llifstream file(resolved_filename);
	if (!file.is_open())
	{
		llwarns << "can't load teleport history from file \"" << mFilename << "\"" << llendl;
		return;
	}

	// remove current entries before we load over them
	mItems.clear();

	// the parser's destructor is protected so we cannot create in the stack.
	LLPointer<LLSDParser> parser = new LLSDNotationParser();
	std::string line;
	while (std::getline(file, line))
	{
		LLSD s_item;
		std::istringstream iss(line);
		if (parser->parse(iss, s_item, line.length()) == LLSDParser::PARSE_FAILURE)
		{
			llinfos << "Parsing saved teleport history failed" << llendl;
			break;
		}

		mItems.push_back(s_item);
	}

	file.close();
}

void LLTeleportHistoryStorage::dump() const
{
	llinfos << "Teleport history storage dump (" << mItems.size() << " items):" << llendl;

	for (size_t i=0; i<mItems.size(); i++)
	{
		std::stringstream line;
		line << i << ": " << mItems[i].mTitle;
		line << " global pos: " << mItems[i].mGlobalPos;
		line << " date: " << mItems[i].mDate;

		llinfos << line.str() << llendl;
	}
}

