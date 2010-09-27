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
#include "llteleporthistory.h"
#include "llagent.h"

// Max offset for two global positions to consider them as equal
const F64 MAX_GLOBAL_POS_OFFSET = 5.0f;

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

struct LLSortItemsByDate
{
	bool operator()(const LLTeleportHistoryPersistentItem& a, const LLTeleportHistoryPersistentItem& b)
	{
		return a.mDate < b.mDate;
	}
};

LLTeleportHistoryStorage::LLTeleportHistoryStorage() :
	mFilename("teleport_history.txt")
{
	LLTeleportHistory *th = LLTeleportHistory::getInstance();
	if (th)
		th->setHistoryChangedCallback(boost::bind(&LLTeleportHistoryStorage::onTeleportHistoryChange, this));	

	load();
}

LLTeleportHistoryStorage::~LLTeleportHistoryStorage()
{
}

void LLTeleportHistoryStorage::onTeleportHistoryChange()
{
	LLTeleportHistory *th = LLTeleportHistory::getInstance();
	if (!th)
		return;

	// Hacky sanity check. (EXT-6798)
	if (th->getItems().size() == 0)
	{
		llassert(!"Inconsistent teleport history state");
		return;
	}

	const LLTeleportHistoryItem &item = th->getItems()[th->getCurrentItemIndex()];

	addItem(item.mTitle, item.mGlobalPos);
	save();
}

void LLTeleportHistoryStorage::purgeItems()
{
	mItems.clear();
	mHistoryChangedSignal(-1);
}

void LLTeleportHistoryStorage::addItem(const std::string title, const LLVector3d& global_pos)
{
	addItem(title, global_pos, LLDate::now());
}


bool LLTeleportHistoryStorage::compareByTitleAndGlobalPos(const LLTeleportHistoryPersistentItem& a, const LLTeleportHistoryPersistentItem& b)
{
	return a.mTitle == b.mTitle && (a.mGlobalPos - b.mGlobalPos).length() < MAX_GLOBAL_POS_OFFSET;
}

void LLTeleportHistoryStorage::addItem(const std::string title, const LLVector3d& global_pos, const LLDate& date)
{
	LLTeleportHistoryPersistentItem item(title, global_pos, date);

	slurl_list_t::iterator item_iter = std::find_if(mItems.begin(), mItems.end(),
							    boost::bind(&LLTeleportHistoryStorage::compareByTitleAndGlobalPos, this, _1, item));

	// If there is such item already, remove it, since new item is more recent
	S32 removed_index = -1;
	if (item_iter != mItems.end())
	{
		removed_index = item_iter - mItems.begin();
		mItems.erase(item_iter);
	}

	mItems.push_back(item);

	// Check whether sorting is needed
	if (mItems.size() > 1)
	{
		item_iter = mItems.end();

		item_iter--;
		item_iter--;

		// If second to last item is more recent than last, then resort items
		if (item_iter->mDate > item.mDate)
		{
			removed_index = -1;
			std::sort(mItems.begin(), mItems.end(), LLSortItemsByDate());
		}
	}

	mHistoryChangedSignal(removed_index);
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

	std::sort(mItems.begin(), mItems.end(), LLSortItemsByDate());

	mHistoryChangedSignal(-1);
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

boost::signals2::connection LLTeleportHistoryStorage::setHistoryChangedCallback(history_callback_t cb)
{
	return mHistoryChangedSignal.connect(cb);
}

void LLTeleportHistoryStorage::goToItem(S32 idx)
{
	// Validate specified index.
	if (idx < 0 || idx >= (S32)mItems.size())
	{
		llwarns << "Invalid teleport history index (" << idx << ") specified" << llendl;
		dump();
		return;
	}

	// Attempt to teleport to the requested item.
	gAgent.teleportViaLocation(mItems[idx].mGlobalPos);
}

