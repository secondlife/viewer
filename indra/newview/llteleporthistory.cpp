/** 
 * @file llteleporthistory.cpp
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

#include "llteleporthistory.h"

#include "llparcel.h"
#include "llsdserialize.h"

#include "llagent.h"
#include "llslurl.h"
#include "llurlsimstring.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llworldmap.h"

//////////////////////////////////////////////////////////////////////////////
// LLTeleportHistoryItem
//////////////////////////////////////////////////////////////////////////////

LLTeleportHistoryItem::LLTeleportHistoryItem(const LLSD& val)
{
	mTitle = val["title"].asString();
	mGlobalPos.setValue(val["global_pos"]);
}

LLSD LLTeleportHistoryItem::toLLSD() const
{
	LLSD val;

	val["title"]		= mTitle;
	val["global_pos"]	= mGlobalPos.getValue();
	
	return val;
}

//////////////////////////////////////////////////////////////////////////////
// LLTeleportHistory
//////////////////////////////////////////////////////////////////////////////

LLTeleportHistory::LLTeleportHistory():
	mCurrentItem(-1),
	mHistoryTeleportInProgress(false),
	mGotInitialUpdate(false),
	mFilename("teleports.txt"),
	mHistoryChangedCallback(NULL)
{
	mTeleportFinishedConn = LLViewerParcelMgr::getInstance()->
		setTeleportFinishedCallback(boost::bind(&LLTeleportHistory::updateCurrentLocation, this));
}

LLTeleportHistory::~LLTeleportHistory()
{
	mTeleportFinishedConn.disconnect();
}

void LLTeleportHistory::goToItem(int idx)

{
	// Validate specified index.
	if (idx < 0 || idx >= (int)mItems.size())
	{
		llwarns << "Invalid teleport history index (" << idx << ") specified" << llendl;
		dump();
		return;
	}
	
	if (idx == mCurrentItem)
	{
		llwarns << "Will not teleport to the same location." << llendl;
		dump();
		return;
	}

	// Make specified item current and teleport to it.
	mCurrentItem = idx;
	mHistoryTeleportInProgress = true;
	onHistoryChanged();
	gAgent.teleportViaLocation(mItems[mCurrentItem].mGlobalPos);
}

void LLTeleportHistory::updateCurrentLocation()
{
	if (mHistoryTeleportInProgress)
	{
		mHistoryTeleportInProgress = false;
	}
	else
	{
		// If we're getting the initial location update
		// while we already have a (loaded) non-empty history,
		// there's no need to purge forward items or add a new item.

		if (mGotInitialUpdate || mItems.size() == 0)
		{
			// Purge forward items (if any).
			if(mItems.size())
				mItems.erase (mItems.begin() + mCurrentItem + 1, mItems.end());
			
			// Append an empty item to the history and make it current.
			mItems.push_back(LLTeleportHistoryItem("", LLVector3d()));
			mCurrentItem++;
		}
	}

	// Update current history item.
	if (mCurrentItem < 0 || mCurrentItem >= (int) mItems.size()) // sanity check
	{
		llwarns << "Invalid current item. (this should not happen)" << llendl;
		return;
	}
	mItems[mCurrentItem].mTitle = getCurrentLocationTitle();
	mItems[mCurrentItem].mGlobalPos	= gAgent.getPositionGlobal();
	dump();
	
	if (!mGotInitialUpdate)
		mGotInitialUpdate = true;

	// Signal the interesting party that we've changed. 
	onHistoryChanged();
}

void LLTeleportHistory::save() const
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
		s_item["is_current"] = (i == mCurrentItem);
		file << LLSDOStreamer<LLSDNotationFormatter>(s_item) << std::endl;
	}

	file.close();
}

// *TODO: clean this up
void LLTeleportHistory::load()
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
	mCurrentItem = -1;
	
	// the parser's destructor is protected so we cannot create in the stack.
	LLPointer<LLSDParser> parser = new LLSDNotationParser();
	std::string line;
	for (int i = 0; std::getline(file, line); i++)
	{
		LLSD s_item;
		std::istringstream iss(line);
		if (parser->parse(iss, s_item, line.length()) == LLSDParser::PARSE_FAILURE)
		{
			llinfos << "Parsing saved teleport history failed" << llendl;
			break;
		}
		
		mItems.push_back(s_item);
		if (s_item["is_current"].asBoolean() == true)
			mCurrentItem = i; 
	}

	file.close();
	onHistoryChanged();
}

void LLTeleportHistory::setHistoryChangedCallback(history_callback_t cb)
{
	mHistoryChangedCallback = cb;
}

void LLTeleportHistory::onHistoryChanged()
{
	if (mHistoryChangedCallback)
	{
		mHistoryChangedCallback();
	}
}

// static
std::string LLTeleportHistory::getCurrentLocationTitle()
{
	std::string location_name;

	if (!gAgent.buildLocationString(location_name, LLAgent::LOCATION_FORMAT_NORMAL))
		location_name = "Unknown";

	return location_name;
}

void LLTeleportHistory::dump() const
{
	llinfos << "Teleport history dump (" << mItems.size() << " items):" << llendl;
	
	for (size_t i=0; i<mItems.size(); i++)
	{
		std::stringstream line;
		line << ((i == mCurrentItem) ? " * " : "   ");
		line << i << ": " << mItems[i].mTitle;
		llinfos << line.str() << llendl;
	}
}
