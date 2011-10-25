/** 
 * @file llteleporthistory.cpp
 * @brief Teleport history
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

#include "llviewerprecompiledheaders.h"

#include "llteleporthistory.h"

#include "llparcel.h"
#include "llsdserialize.h"

#include "llagent.h"
#include "llvoavatarself.h"
#include "llslurl.h"
#include "llviewercontrol.h"        // for gSavedSettings
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llworldmap.h"
#include "llagentui.h"

//////////////////////////////////////////////////////////////////////////////
// LLTeleportHistoryItem
//////////////////////////////////////////////////////////////////////////////

const std::string& LLTeleportHistoryItem::getTitle() const
{
	return gSavedSettings.getBOOL("NavBarShowCoordinates") ? mFullTitle : mTitle;
}

//////////////////////////////////////////////////////////////////////////////
// LLTeleportHistory
//////////////////////////////////////////////////////////////////////////////

LLTeleportHistory::LLTeleportHistory():
	mCurrentItem(-1),
	mRequestedItem(-1),
	mGotInitialUpdate(false),
	mTeleportHistoryStorage(NULL)
{
	mTeleportFinishedConn = LLViewerParcelMgr::getInstance()->
		setTeleportFinishedCallback(boost::bind(&LLTeleportHistory::updateCurrentLocation, this, _1));
	mTeleportFailedConn = LLViewerParcelMgr::getInstance()->
		setTeleportFailedCallback(boost::bind(&LLTeleportHistory::onTeleportFailed, this));
}

LLTeleportHistory::~LLTeleportHistory()
{
	mTeleportFinishedConn.disconnect();
	mTeleportFailedConn.disconnect();
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

	// Attempt to teleport to the requested item.
	gAgent.teleportViaLocation(mItems[idx].mGlobalPos);
	mRequestedItem = idx;
}

void LLTeleportHistory::onTeleportFailed()
{
	// Are we trying to teleport within the history?
	if (mRequestedItem != -1)
	{
		// Not anymore.
		mRequestedItem = -1;
	}
}

void LLTeleportHistory::handleLoginComplete()
{
	if( mGotInitialUpdate )
	{
		return;
	}
	updateCurrentLocation(gAgent.getPositionGlobal());
}


void LLTeleportHistory::updateCurrentLocation(const LLVector3d& new_pos)
{
	if (!mTeleportHistoryStorage)
	{
		mTeleportHistoryStorage = LLTeleportHistoryStorage::getInstance();
	}
	if (mRequestedItem != -1) // teleport within the history in progress?
	{
		mCurrentItem = mRequestedItem;
		mRequestedItem = -1;
	}
	else
	{
		//EXT-7034
		//skip initial update if agent avatar is no valid yet
		//this may happen when updateCurrentLocation called while login process
		//sometimes isAgentAvatarValid return false and in this case new_pos
		//(which actually is gAgent.getPositionGlobal() ) is invalid
		//if this position will be saved then teleport back will teleport user to wrong position
		if ( !mGotInitialUpdate && !isAgentAvatarValid() )
		{
			return ;
		}

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

		// Update current history item.
		if (mCurrentItem < 0 || mCurrentItem >= (int) mItems.size()) // sanity check
		{
			llwarns << "Invalid current item. (this should not happen)" << llendl;
			llassert(!"Invalid current teleport history item");
			return;
		}
		LLVector3 new_pos_local = gAgent.getPosAgentFromGlobal(new_pos);
		mItems[mCurrentItem].mFullTitle = getCurrentLocationTitle(true, new_pos_local);
		mItems[mCurrentItem].mTitle = getCurrentLocationTitle(false, new_pos_local);
		mItems[mCurrentItem].mGlobalPos	= new_pos;
		mItems[mCurrentItem].mRegionID = gAgent.getRegion()->getRegionID();
	}

	dump();
	
	if (!mGotInitialUpdate)
		mGotInitialUpdate = true;

	// Signal the interesting party that we've changed. 
	onHistoryChanged();
}

boost::signals2::connection LLTeleportHistory::setHistoryChangedCallback(history_callback_t cb)
{
	return mHistoryChangedSignal.connect(cb);
}

void LLTeleportHistory::onHistoryChanged()
{
	mHistoryChangedSignal();
}

void LLTeleportHistory::purgeItems()
{
	if (mItems.size() == 0) // no entries yet (we're called before login)
	{
		// If we don't return here the history will get into inconsistent state, hence:
		// 1) updateCurrentLocation() will malfunction,
		//    so further teleports will not properly update the history;
		// 2) mHistoryChangedSignal subscribers will be notified
		//    of such an invalid change. (EXT-6798)
		// Both should not happen.
		return;
	}

	if (mItems.size() > 0)
	{
		mItems.erase(mItems.begin(), mItems.end()-1);
	}
	// reset the count
	mRequestedItem = -1;
	mCurrentItem = 0;

	onHistoryChanged();
}

// static
std::string LLTeleportHistory::getCurrentLocationTitle(bool full, const LLVector3& local_pos_override)
{
	std::string location_name;
	LLAgentUI::ELocationFormat fmt = full ? LLAgentUI::LOCATION_FORMAT_NO_MATURITY : LLAgentUI::LOCATION_FORMAT_NORMAL;

	if (!LLAgentUI::buildLocationString(location_name, fmt, local_pos_override)) location_name = "Unknown";
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
		line << " REGION_ID: " << mItems[i].mRegionID;
		line << ", pos: " << mItems[i].mGlobalPos;
		llinfos << line.str() << llendl;
	}
}
