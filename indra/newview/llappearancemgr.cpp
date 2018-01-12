/** 
 * @file llappearancemgr.cpp
 * @brief Manager for initiating appearance changes on the viewer
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include "llaccordionctrltab.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llattachmentsmgr.h"
#include "llcommandhandler.h"
#include "lleventtimer.h"
#include "llfloatersidepanelcontainer.h"
#include "llgesturemgr.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventoryobserver.h"
#include "llnotificationsutil.h"
#include "lloutfitobserver.h"
#include "lloutfitslist.h"
#include "llselectmgr.h"
#include "llsidepanelappearance.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llviewerregion.h"
#include "llwearablelist.h"
#include "llsdutil.h"
#include "llsdserialize.h"
#include "llhttpretrypolicy.h"
#include "llaisapi.h"
#include "llhttpsdhandler.h"
#include "llcorehttputil.h"
#include "llappviewer.h"
#include "llcoros.h"
#include "lleventcoro.h"

#include "llavatarpropertiesprocessor.h"

#if LL_MSVC
// disable boost::lexical_cast warning
#pragma warning (disable:4702)
#endif

namespace 
{
    const S32   BAKE_RETRY_MAX_COUNT = 5;
    const F32   BAKE_RETRY_TIMEOUT = 2.0F;
}

// *TODO$: LLInventoryCallback should be deprecated to conform to the new boost::bind/coroutine model.
// temp code in transition
void doAppearanceCb(LLPointer<LLInventoryCallback> cb, LLUUID id)
{
    if (cb.notNull())
        cb->fire(id);
}

std::string self_av_string()
{
	// On logout gAgentAvatarp can already be invalid
	return isAgentAvatarValid() ? gAgentAvatarp->avString() : "";
}

// RAII thingy to guarantee that a variable gets reset when the Setter
// goes out of scope.  More general utility would be handy - TODO:
// check boost.
class BoolSetter
{
public:
	BoolSetter(bool& var):
		mVar(var)
	{
		mVar = true;
	}
	~BoolSetter()
	{
		mVar = false; 
	}
private:
	bool& mVar;
};

char ORDER_NUMBER_SEPARATOR('@');

class LLOutfitUnLockTimer: public LLEventTimer
{
public:
	LLOutfitUnLockTimer(F32 period) : LLEventTimer(period)
	{
		// restart timer on BOF changed event
		LLOutfitObserver::instance().addBOFChangedCallback(boost::bind(
				&LLOutfitUnLockTimer::reset, this));
		stop();
	}

	/*virtual*/
	BOOL tick()
	{
		if(mEventTimer.hasExpired())
		{
			LLAppearanceMgr::instance().setOutfitLocked(false);
		}
		return FALSE;
	}
	void stop() { mEventTimer.stop(); }
	void start() { mEventTimer.start(); }
	void reset() { mEventTimer.reset(); }
	BOOL getStarted() { return mEventTimer.getStarted(); }

	LLTimer&  getEventTimer() { return mEventTimer;}
};

// support for secondlife:///app/appearance SLapps
class LLAppearanceHandler : public LLCommandHandler
{
public:
	// requests will be throttled from a non-trusted browser
	LLAppearanceHandler() : LLCommandHandler("appearance", UNTRUSTED_THROTTLE) {}

	bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
	{
		// support secondlife:///app/appearance/show, but for now we just
		// make all secondlife:///app/appearance SLapps behave this way
		if (!LLUI::sSettingGroups["config"]->getBOOL("EnableAppearance"))
		{
			LLNotificationsUtil::add("NoAppearance", LLSD(), LLSD(), std::string("SwitchToStandardSkinAndQuit"));
			return true;
		}

		LLFloaterSidePanelContainer::showPanel("appearance", LLSD());
		return true;
	}
};

LLAppearanceHandler gAppearanceHandler;


LLUUID findDescendentCategoryIDByName(const LLUUID& parent_id, const std::string& name)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	LLNameCategoryCollector has_name(name);
	gInventory.collectDescendentsIf(parent_id,
									cat_array,
									item_array,
									LLInventoryModel::EXCLUDE_TRASH,
									has_name);
	if (0 == cat_array.size())
		return LLUUID();
	else
	{
		LLViewerInventoryCategory *cat = cat_array.at(0);
		if (cat)
			return cat->getUUID();
		else
		{
			LL_WARNS() << "null cat" << LL_ENDL;
			return LLUUID();
		}
	}
}

// We want this to be much lower (e.g. 15.0 is usually fine), bumping
// up for now until we can diagnose some cases of very slow response
// to requests.
const F32 DEFAULT_RETRY_AFTER_INTERVAL = 300.0;

// Given the current back-end problems, retrying is causing too many
// duplicate items. Bump this back to 2 once they are resolved (or can
// leave at 0 if the operations become actually reliable).
const S32 DEFAULT_MAX_RETRIES = 0;

class LLCallAfterInventoryBatchMgr: public LLEventTimer 
{
public:
	LLCallAfterInventoryBatchMgr(const LLUUID& dst_cat_id,
								 const std::string& phase_name,
								 nullary_func_t on_completion_func,
								 nullary_func_t on_failure_func = no_op,
								 F32 retry_after = DEFAULT_RETRY_AFTER_INTERVAL,
								 S32 max_retries = DEFAULT_MAX_RETRIES
		):
		mDstCatID(dst_cat_id),
		mTrackingPhase(phase_name),
		mOnCompletionFunc(on_completion_func),
		mOnFailureFunc(on_failure_func),
		mRetryAfter(retry_after),
		mMaxRetries(max_retries),
		mPendingRequests(0),
		mFailCount(0),
		mCompletionOrFailureCalled(false),
		mRetryCount(0),
		LLEventTimer(5.0)
	{
		if (!mTrackingPhase.empty())
		{
			selfStartPhase(mTrackingPhase);
		}
	}

	void addItems(LLInventoryModel::item_array_t& src_items)
	{
		for (LLInventoryModel::item_array_t::const_iterator it = src_items.begin();
			 it != src_items.end();
			 ++it)
		{
			LLViewerInventoryItem* item = *it;
			llassert(item);
			addItem(item->getUUID());
		}
	}

	// Request or re-request operation for specified item.
	void addItem(const LLUUID& item_id)
	{
		LL_DEBUGS("Avatar") << "item_id " << item_id << LL_ENDL;
		if (!requestOperation(item_id))
		{
			LL_DEBUGS("Avatar") << "item_id " << item_id << " requestOperation false, skipping" << LL_ENDL;
			return;
		}

		mPendingRequests++;
		// On a re-request, this will reset the timer.
		mWaitTimes[item_id] = LLTimer();
		if (mRetryCounts.find(item_id) == mRetryCounts.end())
		{
			mRetryCounts[item_id] = 0;
		}
		else
		{
			mRetryCounts[item_id]++;
		}
	}

	virtual bool requestOperation(const LLUUID& item_id) = 0;

	void onOp(const LLUUID& src_id, const LLUUID& dst_id, LLTimer timestamp)
	{
		if (ll_frand() < gSavedSettings.getF32("InventoryDebugSimulateLateOpRate"))
		{
			LL_WARNS() << "Simulating late operation by punting handling to later" << LL_ENDL;
			doAfterInterval(boost::bind(&LLCallAfterInventoryBatchMgr::onOp,this,src_id,dst_id,timestamp),
							mRetryAfter);
			return;
		}
		mPendingRequests--;
		F32 elapsed = timestamp.getElapsedTimeF32();
		LL_DEBUGS("Avatar") << "op done, src_id " << src_id << " dst_id " << dst_id << " after " << elapsed << " seconds" << LL_ENDL;
		if (mWaitTimes.find(src_id) == mWaitTimes.end())
		{
			// No longer waiting for this item - either serviced
			// already or gave up after too many retries.
			LL_WARNS() << "duplicate or late operation, src_id " << src_id << "dst_id " << dst_id
					<< " elapsed " << elapsed << " after end " << (S32) mCompletionOrFailureCalled << LL_ENDL;
		}
		mTimeStats.push(elapsed);
		mWaitTimes.erase(src_id);
		if (mWaitTimes.empty() && !mCompletionOrFailureCalled)
		{
			onCompletionOrFailure();
		}
	}

	void onCompletionOrFailure()
	{
		assert (!mCompletionOrFailureCalled);
		mCompletionOrFailureCalled = true;
		
		// Will never call onCompletion() if any item has been flagged as
		// a failure - otherwise could wind up with corrupted
		// outfit, involuntary nudity, etc.
		reportStats();
		if (!mTrackingPhase.empty())
		{
			selfStopPhase(mTrackingPhase);
		}
		if (!mFailCount)
		{
			onCompletion();
		}
		else
		{
			onFailure();
		}
	}

	void onFailure()
	{
		LL_INFOS() << "failed" << LL_ENDL;
		mOnFailureFunc();
	}

	void onCompletion()
	{
		LL_INFOS() << "done" << LL_ENDL;
		mOnCompletionFunc();
	}
	
	// virtual
	// Will be deleted after returning true - only safe to do this if all callbacks have fired.
	BOOL tick()
	{
		// mPendingRequests will be zero if all requests have been
		// responded to.  mWaitTimes.empty() will be true if we have
		// received at least one reply for each UUID.  If requests
		// have been dropped and retried, these will not necessarily
		// be the same.  Only safe to return true if all requests have
		// been serviced, since it will result in this object being
		// deleted.
		bool all_done = (mPendingRequests==0);

		if (!mWaitTimes.empty())
		{
			LL_WARNS() << "still waiting on " << mWaitTimes.size() << " items" << LL_ENDL;
			for (std::map<LLUUID,LLTimer>::iterator it = mWaitTimes.begin();
				 it != mWaitTimes.end();)
			{
				// Use a copy of iterator because it may be erased/invalidated.
				std::map<LLUUID,LLTimer>::iterator curr_it = it;
				++it;
				
				F32 time_waited = curr_it->second.getElapsedTimeF32();
				S32 retries = mRetryCounts[curr_it->first];
				if (time_waited > mRetryAfter)
				{
					if (retries < mMaxRetries)
					{
						LL_DEBUGS("Avatar") << "Waited " << time_waited <<
							" for " << curr_it->first << ", retrying" << LL_ENDL;
						mRetryCount++;
						addItem(curr_it->first);
					}
					else
					{
						LL_WARNS() << "Giving up on " << curr_it->first << " after too many retries" << LL_ENDL;
						mWaitTimes.erase(curr_it);
						mFailCount++;
					}
				}
				if (mWaitTimes.empty())
				{
					onCompletionOrFailure();
				}

			}
		}
		return all_done;
	}

	void reportStats()
	{
		LL_DEBUGS("Avatar") << "Phase: " << mTrackingPhase << LL_ENDL;
		LL_DEBUGS("Avatar") << "mFailCount: " << mFailCount << LL_ENDL;
		LL_DEBUGS("Avatar") << "mRetryCount: " << mRetryCount << LL_ENDL;
		LL_DEBUGS("Avatar") << "Times: n " << mTimeStats.getCount() << " min " << mTimeStats.getMinValue() << " max " << mTimeStats.getMaxValue() << LL_ENDL;
		LL_DEBUGS("Avatar") << "Mean " << mTimeStats.getMean() << " stddev " << mTimeStats.getStdDev() << LL_ENDL;
	}
	
	virtual ~LLCallAfterInventoryBatchMgr()
	{
		LL_DEBUGS("Avatar") << "deleting" << LL_ENDL;
	}

protected:
	std::string mTrackingPhase;
	std::map<LLUUID,LLTimer> mWaitTimes;
	std::map<LLUUID,S32> mRetryCounts;
	LLUUID mDstCatID;
	nullary_func_t mOnCompletionFunc;
	nullary_func_t mOnFailureFunc;
	F32 mRetryAfter;
	S32 mMaxRetries;
	S32 mPendingRequests;
	S32 mFailCount;
	S32 mRetryCount;
	bool mCompletionOrFailureCalled;
	LLViewerStats::StatsAccumulator mTimeStats;
};

class LLCallAfterInventoryCopyMgr: public LLCallAfterInventoryBatchMgr
{
public:
	LLCallAfterInventoryCopyMgr(LLInventoryModel::item_array_t& src_items,
								const LLUUID& dst_cat_id,
								const std::string& phase_name,
								nullary_func_t on_completion_func,
								nullary_func_t on_failure_func = no_op,
								 F32 retry_after = DEFAULT_RETRY_AFTER_INTERVAL,
								 S32 max_retries = DEFAULT_MAX_RETRIES
		):
		LLCallAfterInventoryBatchMgr(dst_cat_id, phase_name, on_completion_func, on_failure_func, retry_after, max_retries)
	{
		addItems(src_items);
		sInstanceCount++;
	}

	~LLCallAfterInventoryCopyMgr()
	{
		sInstanceCount--;
	}
	
	virtual bool requestOperation(const LLUUID& item_id)
	{
		LLViewerInventoryItem *item = gInventory.getItem(item_id);
		llassert(item);
		LL_DEBUGS("Avatar") << "copying item " << item_id << LL_ENDL;
		if (ll_frand() < gSavedSettings.getF32("InventoryDebugSimulateOpFailureRate"))
		{
			LL_DEBUGS("Avatar") << "simulating failure by not sending request for item " << item_id << LL_ENDL;
			return true;
		}
		copy_inventory_item(
			gAgent.getID(),
			item->getPermissions().getOwner(),
			item->getUUID(),
			mDstCatID,
			std::string(),
			new LLBoostFuncInventoryCallback(boost::bind(&LLCallAfterInventoryBatchMgr::onOp,this,item_id,_1,LLTimer()))
			);
		return true;
	}

	static S32 getInstanceCount() { return sInstanceCount; }
	
private:
	static S32 sInstanceCount;
};

S32 LLCallAfterInventoryCopyMgr::sInstanceCount = 0;

class LLWearCategoryAfterCopy: public LLInventoryCallback
{
public:
	LLWearCategoryAfterCopy(bool append):
		mAppend(append)
	{}

	// virtual
	void fire(const LLUUID& id)
	{
		// Wear the inventory category.
		LLInventoryCategory* cat = gInventory.getCategory(id);
		LLAppearanceMgr::instance().wearInventoryCategoryOnAvatar(cat, mAppend);
	}

private:
	bool mAppend;
};

class LLTrackPhaseWrapper : public LLInventoryCallback
{
public:
	LLTrackPhaseWrapper(const std::string& phase_name, LLPointer<LLInventoryCallback> cb = NULL):
		mTrackingPhase(phase_name),
		mCB(cb)
	{
		selfStartPhase(mTrackingPhase);
	}

	// virtual
	void fire(const LLUUID& id)
	{
		if (mCB)
		{
			mCB->fire(id);
		}
	}

	// virtual
	~LLTrackPhaseWrapper()
	{
		selfStopPhase(mTrackingPhase);
	}

protected:
	std::string mTrackingPhase;
	LLPointer<LLInventoryCallback> mCB;
};

LLUpdateAppearanceOnDestroy::LLUpdateAppearanceOnDestroy(bool enforce_item_restrictions,
														 bool enforce_ordering,
														 nullary_func_t post_update_func 
	):
	mFireCount(0),
	mEnforceItemRestrictions(enforce_item_restrictions),
	mEnforceOrdering(enforce_ordering),
	mPostUpdateFunc(post_update_func)
{
	selfStartPhase("update_appearance_on_destroy");
}

void LLUpdateAppearanceOnDestroy::fire(const LLUUID& inv_item)
{
	LLViewerInventoryItem* item = (LLViewerInventoryItem*)gInventory.getItem(inv_item);
	const std::string item_name = item ? item->getName() : "ITEM NOT FOUND";
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LL_DEBUGS("Avatar") << self_av_string() << "callback fired [ name:" << item_name << " UUID:" << inv_item << " count:" << mFireCount << " ] " << LL_ENDL;
#endif
	mFireCount++;
}

LLUpdateAppearanceOnDestroy::~LLUpdateAppearanceOnDestroy()
{
	if (!LLApp::isExiting())
	{
		// speculative fix for MAINT-1150
		LL_INFOS("Avatar") << self_av_string() << "done update appearance on destroy" << LL_ENDL;

		selfStopPhase("update_appearance_on_destroy");

		LLAppearanceMgr::instance().updateAppearanceFromCOF(mEnforceItemRestrictions,
															mEnforceOrdering,
															mPostUpdateFunc);
	}
}

LLUpdateAppearanceAndEditWearableOnDestroy::LLUpdateAppearanceAndEditWearableOnDestroy(const LLUUID& item_id):
	mItemID(item_id)
{
}

LLRequestServerAppearanceUpdateOnDestroy::~LLRequestServerAppearanceUpdateOnDestroy()
{
	LL_DEBUGS("Avatar") << "ATT requesting server appearance update" << LL_ENDL;
    if (!LLApp::isExiting())
    {
        LLAppearanceMgr::instance().requestServerAppearanceUpdate();
    }
}

void edit_wearable_and_customize_avatar(LLUUID item_id)
{
	// Start editing the item if previously requested.
	gAgentWearables.editWearableIfRequested(item_id);
	
	// TODO: camera mode may not be changed if a debug setting is tweaked
	if( gAgentCamera.cameraCustomizeAvatar() )
	{
		// If we're in appearance editing mode, the current tab may need to be refreshed
		LLSidepanelAppearance *panel = dynamic_cast<LLSidepanelAppearance*>(
			LLFloaterSidePanelContainer::getPanel("appearance"));
		if (panel)
		{
			panel->showDefaultSubpart();
		}
	}
}

LLUpdateAppearanceAndEditWearableOnDestroy::~LLUpdateAppearanceAndEditWearableOnDestroy()
{
	if (!LLApp::isExiting())
	{
		LLAppearanceMgr::instance().updateAppearanceFromCOF(
			true,true,
			boost::bind(edit_wearable_and_customize_avatar, mItemID));
	}
}


struct LLFoundData
{
	LLFoundData() :
		mAssetType(LLAssetType::AT_NONE),
		mWearableType(LLWearableType::WT_INVALID),
		mWearable(NULL) {}

	LLFoundData(const LLUUID& item_id,
				const LLUUID& asset_id,
				const std::string& name,
				const LLAssetType::EType& asset_type,
				const LLWearableType::EType& wearable_type,
				const bool is_replacement = false
		) :
		mItemID(item_id),
		mAssetID(asset_id),
		mName(name),
		mAssetType(asset_type),
		mWearableType(wearable_type),
		mIsReplacement(is_replacement),
		mWearable( NULL ) {}
	
	LLUUID mItemID;
	LLUUID mAssetID;
	std::string mName;
	LLAssetType::EType mAssetType;
	LLWearableType::EType mWearableType;
	LLViewerWearable* mWearable;
	bool mIsReplacement;
};

	
class LLWearableHoldingPattern
{
	LOG_CLASS(LLWearableHoldingPattern);

public:
	LLWearableHoldingPattern();
	~LLWearableHoldingPattern();

	bool pollFetchCompletion();
	void onFetchCompletion();
	bool isFetchCompleted();
	bool isTimedOut();

	void checkMissingWearables();
	bool pollMissingWearables();
	bool isMissingCompleted();
	void recoverMissingWearable(LLWearableType::EType type);
	void clearCOFLinksForMissingWearables();
	
	void onWearableAssetFetch(LLViewerWearable *wearable);
	void onAllComplete();

	typedef std::list<LLFoundData> found_list_t;
	found_list_t& getFoundList();
	void eraseTypeToLink(LLWearableType::EType type);
	void eraseTypeToRecover(LLWearableType::EType type);
	void setObjItems(const LLInventoryModel::item_array_t& items);
	void setGestItems(const LLInventoryModel::item_array_t& items);
	bool isMostRecent();
	void handleLateArrivals();
	void resetTime(F32 timeout);
	static S32 countActive() { return sActiveHoldingPatterns.size(); }
	S32 index() { return mIndex; }
	
private:
	found_list_t mFoundList;
	LLInventoryModel::item_array_t mObjItems;
	LLInventoryModel::item_array_t mGestItems;
	typedef std::set<S32> type_set_t;
	type_set_t mTypesToRecover;
	type_set_t mTypesToLink;
	S32 mResolved;
	LLTimer mWaitTime;
	bool mFired;
	typedef std::set<LLWearableHoldingPattern*> type_set_hp;
	static type_set_hp sActiveHoldingPatterns;
	static S32 sNextIndex;
	S32 mIndex;
	bool mIsMostRecent;
	std::set<LLViewerWearable*> mLateArrivals;
	bool mIsAllComplete;
};

LLWearableHoldingPattern::type_set_hp LLWearableHoldingPattern::sActiveHoldingPatterns;
S32 LLWearableHoldingPattern::sNextIndex = 0;

LLWearableHoldingPattern::LLWearableHoldingPattern():
	mResolved(0),
	mFired(false),
	mIsMostRecent(true),
	mIsAllComplete(false)
{
	if (countActive()>0)
	{
		LL_INFOS() << "Creating LLWearableHoldingPattern when "
				   << countActive()
				   << " other attempts are active."
				   << " Flagging others as invalid."
				   << LL_ENDL;
		for (type_set_hp::iterator it = sActiveHoldingPatterns.begin();
			 it != sActiveHoldingPatterns.end();
			 ++it)
		{
			(*it)->mIsMostRecent = false;
		}
			 
	}
	mIndex = sNextIndex++;
	sActiveHoldingPatterns.insert(this);
	LL_DEBUGS("Avatar") << "HP " << index() << " created" << LL_ENDL;
	selfStartPhase("holding_pattern");
}

LLWearableHoldingPattern::~LLWearableHoldingPattern()
{
	sActiveHoldingPatterns.erase(this);
	if (isMostRecent())
	{
		selfStopPhase("holding_pattern");
	}
	LL_DEBUGS("Avatar") << "HP " << index() << " deleted" << LL_ENDL;
}

bool LLWearableHoldingPattern::isMostRecent()
{
	return mIsMostRecent;
}

LLWearableHoldingPattern::found_list_t& LLWearableHoldingPattern::getFoundList()
{
	return mFoundList;
}

void LLWearableHoldingPattern::eraseTypeToLink(LLWearableType::EType type)
{
	mTypesToLink.erase(type);
}

void LLWearableHoldingPattern::eraseTypeToRecover(LLWearableType::EType type)
{
	mTypesToRecover.erase(type);
}

void LLWearableHoldingPattern::setObjItems(const LLInventoryModel::item_array_t& items)
{
	mObjItems = items;
}

void LLWearableHoldingPattern::setGestItems(const LLInventoryModel::item_array_t& items)
{
	mGestItems = items;
}

bool LLWearableHoldingPattern::isFetchCompleted()
{
	return (mResolved >= (S32)getFoundList().size()); // have everything we were waiting for?
}

bool LLWearableHoldingPattern::isTimedOut()
{
	return mWaitTime.hasExpired();
}

void LLWearableHoldingPattern::checkMissingWearables()
{
	if (!isMostRecent())
	{
		// runway why don't we actually skip here?
		LL_WARNS() << self_av_string() << "skipping because LLWearableHolding pattern is invalid (superceded by later outfit request)" << LL_ENDL;
	}

	std::vector<S32> found_by_type(LLWearableType::WT_COUNT,0);
	std::vector<S32> requested_by_type(LLWearableType::WT_COUNT,0);
	for (found_list_t::iterator it = getFoundList().begin(); it != getFoundList().end(); ++it)
	{
		LLFoundData &data = *it;
		if (data.mWearableType < LLWearableType::WT_COUNT)
			requested_by_type[data.mWearableType]++;
		if (data.mWearable)
			found_by_type[data.mWearableType]++;
	}

	for (S32 type = 0; type < LLWearableType::WT_COUNT; ++type)
	{
		if (requested_by_type[type] > found_by_type[type])
		{
			LL_WARNS() << self_av_string() << "got fewer wearables than requested, type " << type << ": requested " << requested_by_type[type] << ", found " << found_by_type[type] << LL_ENDL;
		}
		if (found_by_type[type] > 0)
			continue;
		if (
			// If at least one wearable of certain types (pants/shirt/skirt)
			// was requested but none was found, create a default asset as a replacement.
			// In all other cases, don't do anything.
			// For critical types (shape/hair/skin/eyes), this will keep the avatar as a cloud 
			// due to logic in LLVOAvatarSelf::getIsCloud().
			// For non-critical types (tatoo, socks, etc.) the wearable will just be missing.
			(requested_by_type[type] > 0) &&  
			((type == LLWearableType::WT_PANTS) || (type == LLWearableType::WT_SHIRT) || (type == LLWearableType::WT_SKIRT)))
		{
			mTypesToRecover.insert(type);
			mTypesToLink.insert(type);
			recoverMissingWearable((LLWearableType::EType)type);
			LL_WARNS() << self_av_string() << "need to replace " << type << LL_ENDL; 
		}
	}

	resetTime(60.0F);

	if (isMostRecent())
	{
		selfStartPhase("get_missing_wearables_2");
	}
	if (!pollMissingWearables())
	{
		doOnIdleRepeating(boost::bind(&LLWearableHoldingPattern::pollMissingWearables,this));
	}
}

void LLWearableHoldingPattern::onAllComplete()
{
	if (isAgentAvatarValid())
	{
		gAgentAvatarp->outputRezTiming("Agent wearables fetch complete");
	}

	if (!isMostRecent())
	{
		// runway need to skip here?
		LL_WARNS() << self_av_string() << "skipping because LLWearableHolding pattern is invalid (superceded by later outfit request)" << LL_ENDL;
	}

	// Activate all gestures in this folder
	if (mGestItems.size() > 0)
	{
		LL_DEBUGS("Avatar") << self_av_string() << "Activating " << mGestItems.size() << " gestures" << LL_ENDL;
		
		LLGestureMgr::instance().activateGestures(mGestItems);
		
		// Update the inventory item labels to reflect the fact
		// they are active.
		LLViewerInventoryCategory* catp =
			gInventory.getCategory(LLAppearanceMgr::instance().getCOF());
		
		if (catp)
		{
			gInventory.updateCategory(catp);
			gInventory.notifyObservers();
		}
	}

	if (isAgentAvatarValid())
	{
		LL_DEBUGS("Avatar") << self_av_string() << "Updating " << mObjItems.size() << " attachments" << LL_ENDL;
		LLAgentWearables::llvo_vec_t objects_to_remove;
		LLAgentWearables::llvo_vec_t objects_to_retain;
		LLInventoryModel::item_array_t items_to_add;

		LLAgentWearables::findAttachmentsAddRemoveInfo(mObjItems,
													   objects_to_remove,
													   objects_to_retain,
													   items_to_add);

		LL_DEBUGS("Avatar") << self_av_string() << "Removing " << objects_to_remove.size()
							<< " attachments" << LL_ENDL;

		// Here we remove the attachment pos overrides for *all*
		// attachments, even those that are not being removed. This is
		// needed to get joint positions all slammed down to their
		// pre-attachment states.
		gAgentAvatarp->clearAttachmentOverrides();

		if (objects_to_remove.size() || items_to_add.size())
		{
			LL_DEBUGS("Avatar") << "ATT will remove " << objects_to_remove.size()
								<< " and add " << items_to_add.size() << " items" << LL_ENDL;
		}

		// Take off the attachments that will no longer be in the outfit.
		LLAgentWearables::userRemoveMultipleAttachments(objects_to_remove);
		
		// Update wearables.
		LL_INFOS("Avatar") << self_av_string() << "HP " << index() << " updating agent wearables with "
						   << mResolved << " wearable items " << LL_ENDL;
		LLAppearanceMgr::instance().updateAgentWearables(this);
		
		// Restore attachment pos overrides for the attachments that
		// are remaining in the outfit.
		for (LLAgentWearables::llvo_vec_t::iterator it = objects_to_retain.begin();
			 it != objects_to_retain.end();
			 ++it)
		{
			LLViewerObject *objectp = *it;
			gAgentAvatarp->addAttachmentOverridesForObject(objectp);
		}
		
		// Add new attachments to match those requested.
		LL_DEBUGS("Avatar") << self_av_string() << "Adding " << items_to_add.size() << " attachments" << LL_ENDL;
		LLAgentWearables::userAttachMultipleAttachments(items_to_add);
	}

	if (isFetchCompleted() && isMissingCompleted())
	{
		// Only safe to delete if all wearable callbacks and all missing wearables completed.
		delete this;
	}
	else
	{
		mIsAllComplete = true;
		handleLateArrivals();
	}
}

void LLWearableHoldingPattern::onFetchCompletion()
{
	if (isMostRecent())
	{
		selfStopPhase("get_wearables_2");
	}
		
	if (!isMostRecent())
	{
		// runway skip here?
		LL_WARNS() << self_av_string() << "skipping because LLWearableHolding pattern is invalid (superceded by later outfit request)" << LL_ENDL;
	}

	checkMissingWearables();
}

// Runs as an idle callback until all wearables are fetched (or we time out).
bool LLWearableHoldingPattern::pollFetchCompletion()
{
	if (!isMostRecent())
	{
		// runway skip here?
		LL_WARNS() << self_av_string() << "skipping because LLWearableHolding pattern is invalid (superceded by later outfit request)" << LL_ENDL;
	}

	bool completed = isFetchCompleted();
	bool timed_out = isTimedOut();
	bool done = completed || timed_out;

	if (done)
	{
		LL_INFOS("Avatar") << self_av_string() << "HP " << index() << " polling, done status: " << completed << " timed out " << timed_out
				<< " elapsed " << mWaitTime.getElapsedTimeF32() << LL_ENDL;

		mFired = true;
		
		if (timed_out)
		{
			LL_WARNS() << self_av_string() << "Exceeded max wait time for wearables, updating appearance based on what has arrived" << LL_ENDL;
		}

		onFetchCompletion();
	}
	return done;
}

void recovered_item_link_cb(const LLUUID& item_id, LLWearableType::EType type, LLViewerWearable *wearable, LLWearableHoldingPattern* holder)
{
	if (!holder->isMostRecent())
	{
		LL_WARNS() << "HP " << holder->index() << " skipping because LLWearableHolding pattern is invalid (superceded by later outfit request)" << LL_ENDL;
		// runway skip here?
	}

	LL_INFOS() << "HP " << holder->index() << " recovered item link for type " << type << LL_ENDL;
	holder->eraseTypeToLink(type);
	// Add wearable to FoundData for actual wearing
	LLViewerInventoryItem *item = gInventory.getItem(item_id);
	LLViewerInventoryItem *linked_item = item ? item->getLinkedItem() : NULL;

	if (linked_item)
	{
		gInventory.addChangedMask(LLInventoryObserver::LABEL, linked_item->getUUID());
			
		if (item)
		{
			LLFoundData found(linked_item->getUUID(),
							  linked_item->getAssetUUID(),
							  linked_item->getName(),
							  linked_item->getType(),
							  linked_item->isWearableType() ? linked_item->getWearableType() : LLWearableType::WT_INVALID,
							  true // is replacement
				);
			found.mWearable = wearable;
			holder->getFoundList().push_front(found);
		}
		else
		{
			LL_WARNS() << self_av_string() << "inventory link not found for recovered wearable" << LL_ENDL;
		}
	}
	else
	{
		LL_WARNS() << self_av_string() << "HP " << holder->index() << " inventory link not found for recovered wearable" << LL_ENDL;
	}
}

void recovered_item_cb(const LLUUID& item_id, LLWearableType::EType type, LLViewerWearable *wearable, LLWearableHoldingPattern* holder)
{
	if (!holder->isMostRecent())
	{
		// runway skip here?
		LL_WARNS() << self_av_string() << "skipping because LLWearableHolding pattern is invalid (superceded by later outfit request)" << LL_ENDL;
	}

	LL_DEBUGS("Avatar") << self_av_string() << "Recovered item for type " << type << LL_ENDL;
	LLConstPointer<LLInventoryObject> itemp = gInventory.getItem(item_id);
	wearable->setItemID(item_id);
	holder->eraseTypeToRecover(type);
	llassert(itemp);
	if (itemp)
	{
		LLPointer<LLInventoryCallback> cb = new LLBoostFuncInventoryCallback(boost::bind(recovered_item_link_cb,_1,type,wearable,holder));

		link_inventory_object(LLAppearanceMgr::instance().getCOF(), itemp, cb);
	}
}

void LLWearableHoldingPattern::recoverMissingWearable(LLWearableType::EType type)
{
	if (!isMostRecent())
	{
		// runway skip here?
		LL_WARNS() << self_av_string() << "skipping because LLWearableHolding pattern is invalid (superceded by later outfit request)" << LL_ENDL;
	}
	
		// Try to recover by replacing missing wearable with a new one.
	LLNotificationsUtil::add("ReplacedMissingWearable");
	LL_DEBUGS() << "Wearable " << LLWearableType::getTypeLabel(type)
				<< " could not be downloaded.  Replaced inventory item with default wearable." << LL_ENDL;
	LLViewerWearable* wearable = LLWearableList::instance().createNewWearable(type, gAgentAvatarp);

	// Add a new one in the lost and found folder.
	const LLUUID lost_and_found_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
	LLPointer<LLInventoryCallback> cb = new LLBoostFuncInventoryCallback(boost::bind(recovered_item_cb,_1,type,wearable,this));

	create_inventory_item(gAgent.getID(),
						  gAgent.getSessionID(),
						  lost_and_found_id,
						  wearable->getTransactionID(),
						  wearable->getName(),
						  wearable->getDescription(),
						  wearable->getAssetType(),
						  LLInventoryType::IT_WEARABLE,
						  wearable->getType(),
						  wearable->getPermissions().getMaskNextOwner(),
						  cb);
}

bool LLWearableHoldingPattern::isMissingCompleted()
{
	return mTypesToLink.size()==0 && mTypesToRecover.size()==0;
}

void LLWearableHoldingPattern::clearCOFLinksForMissingWearables()
{
	for (found_list_t::iterator it = getFoundList().begin(); it != getFoundList().end(); ++it)
	{
		LLFoundData &data = *it;
		if ((data.mWearableType < LLWearableType::WT_COUNT) && (!data.mWearable))
		{
			// Wearable link that was never resolved; remove links to it from COF
			LL_INFOS("Avatar") << self_av_string() << "HP " << index() << " removing link for unresolved item " << data.mItemID.asString() << LL_ENDL;
			LLAppearanceMgr::instance().removeCOFItemLinks(data.mItemID);
		}
	}
}

bool LLWearableHoldingPattern::pollMissingWearables()
{
	if (!isMostRecent())
	{
		// runway skip here?
		LL_WARNS() << self_av_string() << "skipping because LLWearableHolding pattern is invalid (superceded by later outfit request)" << LL_ENDL;
	}
	
	bool timed_out = isTimedOut();
	bool missing_completed = isMissingCompleted();
	bool done = timed_out || missing_completed;

	if (!done)
	{
		LL_INFOS("Avatar") << self_av_string() << "HP " << index() << " polling missing wearables, waiting for items " << mTypesToRecover.size()
				<< " links " << mTypesToLink.size()
				<< " wearables, timed out " << timed_out
				<< " elapsed " << mWaitTime.getElapsedTimeF32()
				<< " done " << done << LL_ENDL;
	}

	if (done)
	{
		if (isMostRecent())
		{
			selfStopPhase("get_missing_wearables_2");
		}

		gAgentAvatarp->debugWearablesLoaded();

		// BAP - if we don't call clearCOFLinksForMissingWearables()
		// here, we won't have to add the link back in later if the
		// wearable arrives late.  This is to avoid corruption of
		// wearable ordering info.  Also has the effect of making
		// unworn item links visible in the COF under some
		// circumstances.

		//clearCOFLinksForMissingWearables();
		onAllComplete();
	}
	return done;
}

// Handle wearables that arrived after the timeout period expired.
void LLWearableHoldingPattern::handleLateArrivals()
{
	// Only safe to run if we have previously finished the missing
	// wearables and other processing - otherwise we could be in some
	// intermediate state - but have not been superceded by a later
	// outfit change request.
	if (mLateArrivals.size() == 0)
	{
		// Nothing to process.
		return;
	}
	if (!isMostRecent())
	{
		LL_WARNS() << self_av_string() << "Late arrivals not handled - outfit change no longer valid" << LL_ENDL;
	}
	if (!mIsAllComplete)
	{
		LL_WARNS() << self_av_string() << "Late arrivals not handled - in middle of missing wearables processing" << LL_ENDL;
	}

	LL_INFOS("Avatar") << self_av_string() << "HP " << index() << " need to handle " << mLateArrivals.size() << " late arriving wearables" << LL_ENDL;

	// Update mFoundList using late-arriving wearables.
	std::set<LLWearableType::EType> replaced_types;
	for (LLWearableHoldingPattern::found_list_t::iterator iter = getFoundList().begin();
		 iter != getFoundList().end(); ++iter)
	{
		LLFoundData& data = *iter;
		for (std::set<LLViewerWearable*>::iterator wear_it = mLateArrivals.begin();
			 wear_it != mLateArrivals.end();
			 ++wear_it)
		{
			LLViewerWearable *wearable = *wear_it;

			if(wearable->getAssetID() == data.mAssetID)
			{
				data.mWearable = wearable;

				replaced_types.insert(data.mWearableType);

				// BAP - if we didn't call
				// clearCOFLinksForMissingWearables() earlier, we
				// don't need to restore the link here.  Fixes
				// wearable ordering problems.

				// LLAppearanceMgr::instance().addCOFItemLink(data.mItemID,false);

				// BAP failing this means inventory or asset server
				// are corrupted in a way we don't handle.
				llassert((data.mWearableType < LLWearableType::WT_COUNT) && (wearable->getType() == data.mWearableType));
				break;
			}
		}
	}

	// Remove COF links for any default wearables previously used to replace the late arrivals.
	// All this pussyfooting around with a while loop and explicit
	// iterator incrementing is to allow removing items from the list
	// without clobbering the iterator we're using to navigate.
	LLWearableHoldingPattern::found_list_t::iterator iter = getFoundList().begin();
	while (iter != getFoundList().end())
	{
		LLFoundData& data = *iter;

		// If an item of this type has recently shown up, removed the corresponding replacement wearable from COF.
		if (data.mWearable && data.mIsReplacement &&
			replaced_types.find(data.mWearableType) != replaced_types.end())
		{
			LLAppearanceMgr::instance().removeCOFItemLinks(data.mItemID);
			std::list<LLFoundData>::iterator clobber_ator = iter;
			++iter;
			getFoundList().erase(clobber_ator);
		}
		else
		{
			++iter;
		}
	}

	// Clear contents of late arrivals.
	mLateArrivals.clear();

	// Update appearance based on mFoundList
	LLAppearanceMgr::instance().updateAgentWearables(this);
}

void LLWearableHoldingPattern::resetTime(F32 timeout)
{
	mWaitTime.reset();
	mWaitTime.setTimerExpirySec(timeout);
}

void LLWearableHoldingPattern::onWearableAssetFetch(LLViewerWearable *wearable)
{
	if (!isMostRecent())
	{
		LL_WARNS() << self_av_string() << "skipping because LLWearableHolding pattern is invalid (superceded by later outfit request)" << LL_ENDL;
	}
	
	mResolved += 1;  // just counting callbacks, not successes.
	LL_DEBUGS("Avatar") << self_av_string() << "HP " << index() << " resolved " << mResolved << "/" << getFoundList().size() << LL_ENDL;
	if (!wearable)
	{
		LL_WARNS() << self_av_string() << "no wearable found" << LL_ENDL;
	}

	if (mFired)
	{
		LL_WARNS() << self_av_string() << "called after holder fired" << LL_ENDL;
		if (wearable)
		{
			mLateArrivals.insert(wearable);
			if (mIsAllComplete)
			{
				handleLateArrivals();
			}
		}
		return;
	}

	if (!wearable)
	{
		return;
	}

	U32 use_count = 0;
	for (LLWearableHoldingPattern::found_list_t::iterator iter = getFoundList().begin();
		iter != getFoundList().end(); ++iter)
	{
		LLFoundData& data = *iter;
		if (wearable->getAssetID() == data.mAssetID)
		{
			// Failing this means inventory or asset server are corrupted in a way we don't handle.
			if ((data.mWearableType >= LLWearableType::WT_COUNT) || (wearable->getType() != data.mWearableType))
			{
				LL_WARNS() << self_av_string() << "recovered wearable but type invalid. inventory wearable type: " << data.mWearableType << " asset wearable type: " << wearable->getType() << LL_ENDL;
				break;
			}

			if (use_count == 0)
			{
				data.mWearable = wearable;
				use_count++;
			}
			else
			{
				LLViewerInventoryItem* wearable_item = gInventory.getItem(data.mItemID);
				if (wearable_item && wearable_item->isFinished() && wearable_item->getPermissions().allowModifyBy(gAgentID))
				{
					// We can't edit and do some other interactions with same asset twice, copy it
					// Note: can't update incomplete items. Usually attached from previous viewer build, but
					// consider adding fetch and completion callback
					LLViewerWearable* new_wearable = LLWearableList::instance().createCopy(wearable, wearable->getName());
					data.mWearable = new_wearable;
					data.mAssetID = new_wearable->getAssetID();

					// Update existing inventory item
					wearable_item->setAssetUUID(new_wearable->getAssetID());
					wearable_item->setTransactionID(new_wearable->getTransactionID());
					gInventory.updateItem(wearable_item, LLInventoryObserver::INTERNAL);
					wearable_item->updateServer(FALSE);

					use_count++;
				}
				else
				{
					// Note: technically a bug, LLViewerWearable can identify only one item id at a time,
					// yet we are tying it to multiple items here.
					// LLViewerWearable need to support more then one item.
					LL_WARNS() << "Same LLViewerWearable is used by multiple items! " << wearable->getAssetID() << LL_ENDL;
					data.mWearable = wearable;
				}
			}
		}
	}

	if (use_count > 1)
	{
		LL_WARNS() << "Copying wearable, multiple asset id uses! " << wearable->getAssetID() << LL_ENDL;
		gInventory.notifyObservers();
	}
}

static void onWearableAssetFetch(LLViewerWearable* wearable, void* data)
{
	LLWearableHoldingPattern* holder = (LLWearableHoldingPattern*)data;
	holder->onWearableAssetFetch(wearable);
}


static void removeDuplicateItems(LLInventoryModel::item_array_t& items)
{
	LLInventoryModel::item_array_t new_items;
	std::set<LLUUID> items_seen;
	std::deque<LLViewerInventoryItem*> tmp_list;
	// Traverse from the front and keep the first of each item
	// encountered, so we actually keep the *last* of each duplicate
	// item.  This is needed to give the right priority when adding
	// duplicate items to an existing outfit.
	for (S32 i=items.size()-1; i>=0; i--)
	{
		LLViewerInventoryItem *item = items.at(i);
		LLUUID item_id = item->getLinkedUUID();
		if (items_seen.find(item_id)!=items_seen.end())
			continue;
		items_seen.insert(item_id);
		tmp_list.push_front(item);
	}
	for (std::deque<LLViewerInventoryItem*>::iterator it = tmp_list.begin();
		 it != tmp_list.end();
		 ++it)
	{
		new_items.push_back(*it);
	}
	items = new_items;
}

//=========================================================================

const std::string LLAppearanceMgr::sExpectedTextureName = "OutfitPreview";

const LLUUID LLAppearanceMgr::getCOF() const
{
	return gInventory.findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
}

S32 LLAppearanceMgr::getCOFVersion() const
{
	LLViewerInventoryCategory *cof = gInventory.getCategory(getCOF());
	if (cof)
	{
		return cof->getVersion();
	}
	else
	{
		return LLViewerInventoryCategory::VERSION_UNKNOWN;
	}
}

const LLViewerInventoryItem* LLAppearanceMgr::getBaseOutfitLink()
{
	const LLUUID& current_outfit_cat = getCOF();
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	// Can't search on FT_OUTFIT since links to categories return FT_CATEGORY for type since they don't
	// return preferred type.
	LLIsType is_category( LLAssetType::AT_CATEGORY ); 
	gInventory.collectDescendentsIf(current_outfit_cat,
									cat_array,
									item_array,
									false,
									is_category);
	for (LLInventoryModel::item_array_t::const_iterator iter = item_array.begin();
		 iter != item_array.end();
		 iter++)
	{
		const LLViewerInventoryItem *item = (*iter);
		const LLViewerInventoryCategory *cat = item->getLinkedCategory();
		if (cat && cat->getPreferredType() == LLFolderType::FT_OUTFIT)
		{
			const LLUUID parent_id = cat->getParentUUID();
			LLViewerInventoryCategory*  parent_cat =  gInventory.getCategory(parent_id);
			// if base outfit moved to trash it means that we don't have base outfit
			if (parent_cat != NULL && parent_cat->getPreferredType() == LLFolderType::FT_TRASH)
			{
				return NULL;
			}
			return item;
		}
	}
	return NULL;
}

bool LLAppearanceMgr::getBaseOutfitName(std::string& name)
{
	const LLViewerInventoryItem* outfit_link = getBaseOutfitLink();
	if(outfit_link)
	{
		const LLViewerInventoryCategory *cat = outfit_link->getLinkedCategory();
		if (cat)
		{
			name = cat->getName();
			return true;
		}
	}
	return false;
}

const LLUUID LLAppearanceMgr::getBaseOutfitUUID()
{
	const LLViewerInventoryItem* outfit_link = getBaseOutfitLink();
	if (!outfit_link || !outfit_link->getIsLinkType()) return LLUUID::null;

	const LLViewerInventoryCategory* outfit_cat = outfit_link->getLinkedCategory();
	if (!outfit_cat) return LLUUID::null;

	if (outfit_cat->getPreferredType() != LLFolderType::FT_OUTFIT)
	{
		LL_WARNS() << "Expected outfit type:" << LLFolderType::FT_OUTFIT << " but got type:" << outfit_cat->getType() << " for folder name:" << outfit_cat->getName() << LL_ENDL;
		return LLUUID::null;
	}

	return outfit_cat->getUUID();
}

void wear_on_avatar_cb(const LLUUID& inv_item, bool do_replace = false)
{
	if (inv_item.isNull())
		return;
	
	LLViewerInventoryItem *item = gInventory.getItem(inv_item);
	if (item)
	{
		LLAppearanceMgr::instance().wearItemOnAvatar(inv_item, true, do_replace);
	}
}

void LLAppearanceMgr::wearItemsOnAvatar(const uuid_vec_t& item_ids_to_wear,
                                        bool do_update,
                                        bool replace,
                                        LLPointer<LLInventoryCallback> cb)
{
    bool first = true;

    LLInventoryObject::const_object_list_t items_to_link;

    for (uuid_vec_t::const_iterator it = item_ids_to_wear.begin();
         it != item_ids_to_wear.end();
         ++it)
    {
        replace = first && replace;
        first = false;

        const LLUUID& item_id_to_wear = *it;

        if (item_id_to_wear.isNull())
        {
            LL_DEBUGS("Avatar") << "null id " << item_id_to_wear << LL_ENDL;
            continue;
        }

        LLViewerInventoryItem* item_to_wear = gInventory.getItem(item_id_to_wear);
        if (!item_to_wear)
        {
            LL_DEBUGS("Avatar") << "inventory item not found for id " << item_id_to_wear << LL_ENDL;
            continue;
        }

        if (gInventory.isObjectDescendentOf(item_to_wear->getUUID(), gInventory.getLibraryRootFolderID()))
        {
            LL_DEBUGS("Avatar") << "inventory item in library, will copy and wear "
                                << item_to_wear->getName() << " id " << item_id_to_wear << LL_ENDL;
            LLPointer<LLInventoryCallback> cb = new LLBoostFuncInventoryCallback(boost::bind(wear_on_avatar_cb,_1,replace));
            copy_inventory_item(gAgent.getID(), item_to_wear->getPermissions().getOwner(),
                                item_to_wear->getUUID(), LLUUID::null, std::string(), cb);
            continue;
        } 
        else if (!gInventory.isObjectDescendentOf(item_to_wear->getUUID(), gInventory.getRootFolderID()))
        {
			// not in library and not in agent's inventory
            LL_DEBUGS("Avatar") << "inventory item not in user inventory or library, skipping "
                                << item_to_wear->getName() << " id " << item_id_to_wear << LL_ENDL;
            continue; 
        }
        else if (gInventory.isObjectDescendentOf(item_to_wear->getUUID(), gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH)))
        {
            LLNotificationsUtil::add("CannotWearTrash");
            LL_DEBUGS("Avatar") << "inventory item is in trash, skipping "
                                << item_to_wear->getName() << " id " << item_id_to_wear << LL_ENDL;
            continue;
        }
        else if (isLinkedInCOF(item_to_wear->getUUID())) // EXT-84911
        {
            LL_DEBUGS("Avatar") << "inventory item is already in COF, skipping "
                                << item_to_wear->getName() << " id " << item_id_to_wear << LL_ENDL;
            continue;
        }

        switch (item_to_wear->getType())
        {
            case LLAssetType::AT_CLOTHING:
            {
                if (gAgentWearables.areWearablesLoaded())
                {
                    if (!cb && do_update)
                    {
                        cb = new LLUpdateAppearanceAndEditWearableOnDestroy(item_id_to_wear);
                    }
                    LLWearableType::EType type = item_to_wear->getWearableType();
                    S32 wearable_count = gAgentWearables.getWearableCount(type);
                    if ((replace && wearable_count != 0) || !gAgentWearables.canAddWearable(type))
                    {
                        LLUUID item_id = gAgentWearables.getWearableItemID(item_to_wear->getWearableType(),
                                                                           wearable_count-1);
                        removeCOFItemLinks(item_id, cb);
                    }
                    
                    items_to_link.push_back(item_to_wear);
                } 
            }
            break;

            case LLAssetType::AT_BODYPART:
            {
                // TODO: investigate wearables may not be loaded at this point EXT-8231
                
                // Remove the existing wearables of the same type.
                // Remove existing body parts anyway because we must not be able to wear e.g. two skins.
                removeCOFLinksOfType(item_to_wear->getWearableType());
                if (!cb && do_update)
                {
                    cb = new LLUpdateAppearanceAndEditWearableOnDestroy(item_id_to_wear);
                }
                items_to_link.push_back(item_to_wear);
            }
            break;
                
            case LLAssetType::AT_OBJECT:
            {
                rez_attachment(item_to_wear, NULL, replace);
            }
            break;

            default: continue;
        }
    }

    // Batch up COF link creation - more efficient if using AIS.
    if (items_to_link.size())
    {
        link_inventory_array(getCOF(), items_to_link, cb); 
    }
}

void LLAppearanceMgr::wearItemOnAvatar(const LLUUID& item_id_to_wear,
									   bool do_update,
									   bool replace,
									   LLPointer<LLInventoryCallback> cb)
{
    uuid_vec_t ids;
    ids.push_back(item_id_to_wear);
    wearItemsOnAvatar(ids, do_update, replace, cb);
}

// Update appearance from outfit folder.
void LLAppearanceMgr::changeOutfit(bool proceed, const LLUUID& category, bool append)
{
	if (!proceed)
		return;
	LLAppearanceMgr::instance().updateCOF(category,append);
}

void LLAppearanceMgr::replaceCurrentOutfit(const LLUUID& new_outfit)
{
	LLViewerInventoryCategory* cat = gInventory.getCategory(new_outfit);
	wearInventoryCategory(cat, false, false);
}

// Remove existing photo link from outfit folder.
void LLAppearanceMgr::removeOutfitPhoto(const LLUUID& outfit_id)
{
    LLInventoryModel::cat_array_t sub_cat_array;
    LLInventoryModel::item_array_t outfit_item_array;
    gInventory.collectDescendents(
        outfit_id,
        sub_cat_array,
        outfit_item_array,
        LLInventoryModel::EXCLUDE_TRASH);
    BOOST_FOREACH(LLViewerInventoryItem* outfit_item, outfit_item_array)
    {
        LLViewerInventoryItem* linked_item = outfit_item->getLinkedItem();
        if (linked_item != NULL)
        {
            if (linked_item->getActualType() == LLAssetType::AT_TEXTURE)
            {
                gInventory.removeItem(outfit_item->getUUID());
            }
        }
        else if (outfit_item->getActualType() == LLAssetType::AT_TEXTURE)
        {
            gInventory.removeItem(outfit_item->getUUID());
        }
    }
}

// Open outfit renaming dialog.
void LLAppearanceMgr::renameOutfit(const LLUUID& outfit_id)
{
	LLViewerInventoryCategory* cat = gInventory.getCategory(outfit_id);
	if (!cat)
	{
		return;
	}

	LLSD args;
	args["NAME"] = cat->getName();

	LLSD payload;
	payload["cat_id"] = outfit_id;

	LLNotificationsUtil::add("RenameOutfit", args, payload, boost::bind(onOutfitRename, _1, _2));
}

// User typed new outfit name.
// static
void LLAppearanceMgr::onOutfitRename(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option != 0) return; // canceled

	std::string outfit_name = response["new_name"].asString();
	LLStringUtil::trim(outfit_name);
	if (!outfit_name.empty())
	{
		LLUUID cat_id = notification["payload"]["cat_id"].asUUID();
		rename_category(&gInventory, cat_id, outfit_name);
	}
}

void LLAppearanceMgr::setOutfitLocked(bool locked)
{
	if (mOutfitLocked == locked)
	{
		return;
	}

	mOutfitLocked = locked;
	if (locked)
	{
		mUnlockOutfitTimer->reset();
		mUnlockOutfitTimer->start();
	}
	else
	{
		mUnlockOutfitTimer->stop();
	}

	LLOutfitObserver::instance().notifyOutfitLockChanged();
}

void LLAppearanceMgr::addCategoryToCurrentOutfit(const LLUUID& cat_id)
{
	LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
	wearInventoryCategory(cat, false, true);
}

void LLAppearanceMgr::takeOffOutfit(const LLUUID& cat_id)
{
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLFindWearablesEx collector(/*is_worn=*/ true, /*include_body_parts=*/ false);

	gInventory.collectDescendentsIf(cat_id, cats, items, FALSE, collector);

	LLInventoryModel::item_array_t::const_iterator it = items.begin();
	const LLInventoryModel::item_array_t::const_iterator it_end = items.end();
	uuid_vec_t uuids_to_remove;
	for( ; it_end != it; ++it)
	{
		LLViewerInventoryItem* item = *it;
		uuids_to_remove.push_back(item->getUUID());
	}
	removeItemsFromAvatar(uuids_to_remove);

	// deactivate all gestures in the outfit folder
	LLInventoryModel::item_array_t gest_items;
	getDescendentsOfAssetType(cat_id, gest_items, LLAssetType::AT_GESTURE);
	for(S32 i = 0; i  < gest_items.size(); ++i)
	{
		LLViewerInventoryItem *gest_item = gest_items[i];
		if ( LLGestureMgr::instance().isGestureActive( gest_item->getLinkedUUID()) )
		{
			LLGestureMgr::instance().deactivateGesture( gest_item->getLinkedUUID() );
		}
	}
}

// Create a copy of src_id + contents as a subfolder of dst_id.
void LLAppearanceMgr::shallowCopyCategory(const LLUUID& src_id, const LLUUID& dst_id,
											  LLPointer<LLInventoryCallback> cb)
{
	LLInventoryCategory *src_cat = gInventory.getCategory(src_id);
	if (!src_cat)
	{
		LL_WARNS() << "folder not found for src " << src_id.asString() << LL_ENDL;
		return;
	}
	LL_INFOS() << "starting, src_id " << src_id << " name " << src_cat->getName() << " dst_id " << dst_id << LL_ENDL;
	LLUUID parent_id = dst_id;
	if(parent_id.isNull())
	{
		parent_id = gInventory.getRootFolderID();
	}
	LLUUID subfolder_id = gInventory.createNewCategory( parent_id,
														LLFolderType::FT_NONE,
														src_cat->getName());
	shallowCopyCategoryContents(src_id, subfolder_id, cb);

	gInventory.notifyObservers();
}

void LLAppearanceMgr::slamCategoryLinks(const LLUUID& src_id, const LLUUID& dst_id,
										bool include_folder_links, LLPointer<LLInventoryCallback> cb)
{
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	LLSD contents = LLSD::emptyArray();
	gInventory.getDirectDescendentsOf(src_id, cats, items);
	LL_INFOS() << "copying " << items->size() << " items" << LL_ENDL;
	for (LLInventoryModel::item_array_t::const_iterator iter = items->begin();
		 iter != items->end();
		 ++iter)
	{
		const LLViewerInventoryItem* item = (*iter);
		switch (item->getActualType())
		{
			case LLAssetType::AT_LINK:
			{
				LL_DEBUGS("Avatar") << "linking inventory item " << item->getName() << LL_ENDL;
				//getActualDescription() is used for a new description 
				//to propagate ordering information saved in descriptions of links
				LLSD item_contents;
				item_contents["name"] = item->getName();
				item_contents["desc"] = item->getActualDescription();
				item_contents["linked_id"] = item->getLinkedUUID();
				item_contents["type"] = LLAssetType::AT_LINK; 
				contents.append(item_contents);
				break;
			}
			case LLAssetType::AT_LINK_FOLDER:
			{
				LLViewerInventoryCategory *catp = item->getLinkedCategory();
				if (catp && include_folder_links)
				{
					LL_DEBUGS("Avatar") << "linking inventory folder " << item->getName() << LL_ENDL;
					LLSD base_contents;
					base_contents["name"] = catp->getName();
					base_contents["desc"] = ""; // categories don't have descriptions.
					base_contents["linked_id"] = catp->getLinkedUUID();
					base_contents["type"] = LLAssetType::AT_LINK_FOLDER; 
					contents.append(base_contents);
				}
				break;
			}
			default:
			{
				// Linux refuses to compile unless all possible enums are handled. Really, Linux?
				break;
			}
		}
	}
	slam_inventory_folder(dst_id, contents, cb);
}
// Copy contents of src_id to dst_id.
void LLAppearanceMgr::shallowCopyCategoryContents(const LLUUID& src_id, const LLUUID& dst_id,
													  LLPointer<LLInventoryCallback> cb)
{
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf(src_id, cats, items);
	LL_INFOS() << "copying " << items->size() << " items" << LL_ENDL;
	LLInventoryObject::const_object_list_t link_array;
	for (LLInventoryModel::item_array_t::const_iterator iter = items->begin();
		 iter != items->end();
		 ++iter)
	{
		const LLViewerInventoryItem* item = (*iter);
		switch (item->getActualType())
		{
			case LLAssetType::AT_LINK:
			{
				LL_DEBUGS("Avatar") << "linking inventory item " << item->getName() << LL_ENDL;
				link_array.push_back(LLConstPointer<LLInventoryObject>(item));
				break;
			}
			case LLAssetType::AT_LINK_FOLDER:
			{
				LLViewerInventoryCategory *catp = item->getLinkedCategory();
				// Skip copying outfit links.
				if (catp && catp->getPreferredType() != LLFolderType::FT_OUTFIT)
				{
					LL_DEBUGS("Avatar") << "linking inventory folder " << item->getName() << LL_ENDL;
					link_array.push_back(LLConstPointer<LLInventoryObject>(item));
				}
				break;
			}
			case LLAssetType::AT_CLOTHING:
			case LLAssetType::AT_OBJECT:
			case LLAssetType::AT_BODYPART:
			case LLAssetType::AT_GESTURE:
			{
				LL_DEBUGS("Avatar") << "copying inventory item " << item->getName() << LL_ENDL;
				copy_inventory_item(gAgent.getID(),
									item->getPermissions().getOwner(),
									item->getUUID(),
									dst_id,
									item->getName(),
									cb);
				break;
			}
			default:
				// Ignore non-outfit asset types
				break;
		}
	}
	if (!link_array.empty())
	{
		link_inventory_array(dst_id, link_array, cb);
	}
}

BOOL LLAppearanceMgr::getCanMakeFolderIntoOutfit(const LLUUID& folder_id)
{
	// These are the wearable items that are required for considering this
	// folder as containing a complete outfit.
	U32 required_wearables = 0;
	required_wearables |= 1LL << LLWearableType::WT_SHAPE;
	required_wearables |= 1LL << LLWearableType::WT_SKIN;
	required_wearables |= 1LL << LLWearableType::WT_HAIR;
	required_wearables |= 1LL << LLWearableType::WT_EYES;

	// These are the wearables that the folder actually contains.
	U32 folder_wearables = 0;
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf(folder_id, cats, items);
	for (LLInventoryModel::item_array_t::const_iterator iter = items->begin();
		 iter != items->end();
		 ++iter)
	{
		const LLViewerInventoryItem* item = (*iter);
		if (item->isWearableType())
		{
			const LLWearableType::EType wearable_type = item->getWearableType();
			folder_wearables |= 1LL << wearable_type;
		}
	}

	// If the folder contains the required wearables, return TRUE.
	return ((required_wearables & folder_wearables) == required_wearables);
}

bool LLAppearanceMgr::getCanRemoveOutfit(const LLUUID& outfit_cat_id)
{
	// Disallow removing the base outfit.
	if (outfit_cat_id == getBaseOutfitUUID())
	{
		return false;
	}

	// Check if the outfit folder itself is removable.
	if (!get_is_category_removable(&gInventory, outfit_cat_id))
	{
		return false;
	}

	// Check for the folder's non-removable descendants.
	LLFindNonRemovableObjects filter_non_removable;
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLInventoryModel::item_array_t::const_iterator it;
	gInventory.collectDescendentsIf(outfit_cat_id, cats, items, false, filter_non_removable);
	if (!cats.empty() || !items.empty())
	{
		return false;
	}

	return true;
}

// static
bool LLAppearanceMgr::getCanRemoveFromCOF(const LLUUID& outfit_cat_id)
{
	if (gAgentWearables.isCOFChangeInProgress())
	{
		return false;
	}
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLFindWearablesEx is_worn(/*is_worn=*/ true, /*include_body_parts=*/ false);
	gInventory.collectDescendentsIf(outfit_cat_id,
		cats,
		items,
		LLInventoryModel::EXCLUDE_TRASH,
		is_worn);
	return items.size() > 0;
}

// static
bool LLAppearanceMgr::getCanAddToCOF(const LLUUID& outfit_cat_id)
{
	if (gAgentWearables.isCOFChangeInProgress())
	{
		return false;
	}

	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLFindWearablesEx not_worn(/*is_worn=*/ false, /*include_body_parts=*/ false);
	gInventory.collectDescendentsIf(outfit_cat_id,
		cats,
		items,
		LLInventoryModel::EXCLUDE_TRASH,
		not_worn);

	return items.size() > 0;
}

bool LLAppearanceMgr::getCanReplaceCOF(const LLUUID& outfit_cat_id)
{
	// Don't allow wearing anything while we're changing appearance.
	if (gAgentWearables.isCOFChangeInProgress())
	{
		return false;
	}

	// Check whether it's the base outfit.
	if (outfit_cat_id.isNull() || outfit_cat_id == getBaseOutfitUUID())
	{
		return false;
	}

	// Check whether the outfit contains any wearables
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLFindWearables is_wearable;
	gInventory.collectDescendentsIf(outfit_cat_id,
		cats,
		items,
		LLInventoryModel::EXCLUDE_TRASH,
		is_wearable);

	return items.size() > 0;
}

// Moved from LLWearableList::ContextMenu for wider utility.
bool LLAppearanceMgr::canAddWearables(const uuid_vec_t& item_ids)
{
	// TODO: investigate wearables may not be loaded at this point EXT-8231

	U32 n_objects = 0;
	U32 n_clothes = 0;

	// Count given clothes (by wearable type) and objects.
	for (uuid_vec_t::const_iterator it = item_ids.begin(); it != item_ids.end(); ++it)
	{
		LLViewerInventoryItem* item = gInventory.getItem(*it);
		if (!item)
		{
			return false;
		}

		if (item->getType() == LLAssetType::AT_OBJECT)
		{
			++n_objects;
		}
		else if (item->getType() == LLAssetType::AT_CLOTHING)
		{
			++n_clothes;
		}
		else if (item->getType() == LLAssetType::AT_BODYPART || item->getType() == LLAssetType::AT_GESTURE)
		{
			return isAgentAvatarValid();
		}
		else
		{
			LL_WARNS() << "Unexpected wearable type" << LL_ENDL;
			return false;
		}
	}

	// Check whether we can add all the objects.
	if (!isAgentAvatarValid() || !gAgentAvatarp->canAttachMoreObjects(n_objects))
	{
		return false;
	}

	// Check whether we can add all the clothes.
    U32 sum_clothes = n_clothes + gAgentWearables.getClothingLayerCount();
    return sum_clothes <= LLAgentWearables::MAX_CLOTHING_LAYERS;
}

void LLAppearanceMgr::purgeBaseOutfitLink(const LLUUID& category, LLPointer<LLInventoryCallback> cb)
{
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	gInventory.collectDescendents(category, cats, items,
								  LLInventoryModel::EXCLUDE_TRASH);
	for (S32 i = 0; i < items.size(); ++i)
	{
		LLViewerInventoryItem *item = items.at(i);
		if (item->getActualType() != LLAssetType::AT_LINK_FOLDER)
			continue;
		LLViewerInventoryCategory* catp = item->getLinkedCategory();
		if(catp && catp->getPreferredType() == LLFolderType::FT_OUTFIT)
		{
			remove_inventory_item(item->getUUID(), cb);
		}
	}
}

// Keep the last N wearables of each type.  For viewer 2.0, N is 1 for
// both body parts and clothing items.
void LLAppearanceMgr::filterWearableItems(
	LLInventoryModel::item_array_t& items, S32 max_per_type, S32 max_total)
{
    // Restrict by max total items first.
    if ((max_total > 0) && (items.size() > max_total))
    {
        LLInventoryModel::item_array_t items_to_keep;
        for (S32 i=0; i<max_total; i++)
        {
            items_to_keep.push_back(items[i]);
        }
        items = items_to_keep;
    }

    if (max_per_type > 0)
    {
        // Divvy items into arrays by wearable type.
        std::vector<LLInventoryModel::item_array_t> items_by_type(LLWearableType::WT_COUNT);
        divvyWearablesByType(items, items_by_type);

        // rebuild items list, retaining the last max_per_type of each array
        items.clear();
        for (S32 i=0; i<LLWearableType::WT_COUNT; i++)
        {
            S32 size = items_by_type[i].size();
            if (size <= 0)
                continue;
            S32 start_index = llmax(0,size-max_per_type);
            for (S32 j = start_index; j<size; j++)
            {
                items.push_back(items_by_type[i][j]);
            }
        }
    }
}

void LLAppearanceMgr::updateCOF(const LLUUID& category, bool append)
{
	LLViewerInventoryCategory *pcat = gInventory.getCategory(category);
	if (!pcat)
	{
		LL_WARNS() << "no category found for id " << category << LL_ENDL;
		return;
	}
	LL_INFOS("Avatar") << self_av_string() << "starting, cat '" << (pcat ? pcat->getName() : "[UNKNOWN]") << "'" << LL_ENDL;

	const LLUUID cof = getCOF();

	// Deactivate currently active gestures in the COF, if replacing outfit
	if (!append)
	{
		LLInventoryModel::item_array_t gest_items;
		getDescendentsOfAssetType(cof, gest_items, LLAssetType::AT_GESTURE);
		for(S32 i = 0; i  < gest_items.size(); ++i)
		{
			LLViewerInventoryItem *gest_item = gest_items.at(i);
			if ( LLGestureMgr::instance().isGestureActive( gest_item->getLinkedUUID()) )
			{
				LLGestureMgr::instance().deactivateGesture( gest_item->getLinkedUUID() );
			}
		}
	}
	
	// Collect and filter descendents to determine new COF contents.

	// - Body parts: always include COF contents as a fallback in case any
	// required parts are missing.
	// Preserve body parts from COF if appending.
	LLInventoryModel::item_array_t body_items;
	getDescendentsOfAssetType(cof, body_items, LLAssetType::AT_BODYPART);
	getDescendentsOfAssetType(category, body_items, LLAssetType::AT_BODYPART);
	if (append)
		reverse(body_items.begin(), body_items.end());
	// Reduce body items to max of one per type.
	removeDuplicateItems(body_items);
	filterWearableItems(body_items, 1, 0);

	// - Wearables: include COF contents only if appending.
	LLInventoryModel::item_array_t wear_items;
	if (append)
		getDescendentsOfAssetType(cof, wear_items, LLAssetType::AT_CLOTHING);
	getDescendentsOfAssetType(category, wear_items, LLAssetType::AT_CLOTHING);
	// Reduce wearables to max of one per type.
	removeDuplicateItems(wear_items);
	filterWearableItems(wear_items, 0, LLAgentWearables::MAX_CLOTHING_LAYERS);

	// - Attachments: include COF contents only if appending.
	LLInventoryModel::item_array_t obj_items;
	if (append)
		getDescendentsOfAssetType(cof, obj_items, LLAssetType::AT_OBJECT);
	getDescendentsOfAssetType(category, obj_items, LLAssetType::AT_OBJECT);
	removeDuplicateItems(obj_items);

	// - Gestures: include COF contents only if appending.
	LLInventoryModel::item_array_t gest_items;
	if (append)
		getDescendentsOfAssetType(cof, gest_items, LLAssetType::AT_GESTURE);
	getDescendentsOfAssetType(category, gest_items, LLAssetType::AT_GESTURE);
	removeDuplicateItems(gest_items);
	
	// Create links to new COF contents.
	LLInventoryModel::item_array_t all_items;
	std::copy(body_items.begin(), body_items.end(), std::back_inserter(all_items));
	std::copy(wear_items.begin(), wear_items.end(), std::back_inserter(all_items));
	std::copy(obj_items.begin(), obj_items.end(), std::back_inserter(all_items));
	std::copy(gest_items.begin(), gest_items.end(), std::back_inserter(all_items));

	// Find any wearables that need description set to enforce ordering.
	desc_map_t desc_map;
	getWearableOrderingDescUpdates(wear_items, desc_map);

	// Will link all the above items.
	// link_waiter enforce flags are false because we've already fixed everything up in updateCOF().
	LLPointer<LLInventoryCallback> link_waiter = new LLUpdateAppearanceOnDestroy(false,false);
	LLSD contents = LLSD::emptyArray();

	for (LLInventoryModel::item_array_t::const_iterator it = all_items.begin();
		 it != all_items.end(); ++it)
	{
		LLSD item_contents;
		LLInventoryItem *item = *it;

		std::string desc;
		desc_map_t::const_iterator desc_iter = desc_map.find(item->getUUID());
		if (desc_iter != desc_map.end())
		{
			desc = desc_iter->second;
			LL_DEBUGS("Avatar") << item->getName() << " overriding desc to: " << desc
								<< " (was: " << item->getActualDescription() << ")" << LL_ENDL;
		}
		else
		{
			desc = item->getActualDescription();
		}

		item_contents["name"] = item->getName();
		item_contents["desc"] = desc;
		item_contents["linked_id"] = item->getLinkedUUID();
		item_contents["type"] = LLAssetType::AT_LINK; 
		contents.append(item_contents);
	}
	const LLUUID& base_id = append ? getBaseOutfitUUID() : category;
	LLViewerInventoryCategory *base_cat = gInventory.getCategory(base_id);
	if (base_cat && (base_cat->getPreferredType() == LLFolderType::FT_OUTFIT))
	{
		LLSD base_contents;
		base_contents["name"] = base_cat->getName();
		base_contents["desc"] = "";
		base_contents["linked_id"] = base_cat->getLinkedUUID();
		base_contents["type"] = LLAssetType::AT_LINK_FOLDER; 
		contents.append(base_contents);
	}
	if (gSavedSettings.getBOOL("DebugAvatarAppearanceMessage"))
	{
		dump_sequential_xml(gAgentAvatarp->getFullname() + "_slam_request", contents);
	}
	slam_inventory_folder(getCOF(), contents, link_waiter);

	LL_DEBUGS("Avatar") << self_av_string() << "waiting for LLUpdateAppearanceOnDestroy" << LL_ENDL;
}

void LLAppearanceMgr::updatePanelOutfitName(const std::string& name)
{
	LLSidepanelAppearance* panel_appearance =
		dynamic_cast<LLSidepanelAppearance *>(LLFloaterSidePanelContainer::getPanel("appearance"));
	if (panel_appearance)
	{
		panel_appearance->refreshCurrentOutfitName(name);
	}
}

void LLAppearanceMgr::createBaseOutfitLink(const LLUUID& category, LLPointer<LLInventoryCallback> link_waiter)
{
	const LLUUID cof = getCOF();
	LLViewerInventoryCategory* catp = gInventory.getCategory(category);
	std::string new_outfit_name = "";

	purgeBaseOutfitLink(cof, link_waiter);

	if (catp && catp->getPreferredType() == LLFolderType::FT_OUTFIT)
	{
		link_inventory_object(cof, catp, link_waiter);
		new_outfit_name = catp->getName();
	}
	
	updatePanelOutfitName(new_outfit_name);
}

void LLAppearanceMgr::updateAgentWearables(LLWearableHoldingPattern* holder)
{
	LL_DEBUGS("Avatar") << "updateAgentWearables()" << LL_ENDL;
	LLInventoryItem::item_array_t items;
	std::vector< LLViewerWearable* > wearables;
	wearables.reserve(32);

	// For each wearable type, find the wearables of that type.
	for( S32 i = 0; i < LLWearableType::WT_COUNT; i++ )
	{
		for (LLWearableHoldingPattern::found_list_t::iterator iter = holder->getFoundList().begin();
			 iter != holder->getFoundList().end(); ++iter)
		{
			LLFoundData& data = *iter;
			LLViewerWearable* wearable = data.mWearable;
			if( wearable && ((S32)wearable->getType() == i) )
			{
				LLViewerInventoryItem* item = (LLViewerInventoryItem*)gInventory.getItem(data.mItemID);
				if( item && (item->getAssetUUID() == wearable->getAssetID()) )
				{
					items.push_back(item);
					wearables.push_back(wearable);
				}
			}
		}
	}

	if(wearables.size() > 0)
	{
		gAgentWearables.setWearableOutfit(items, wearables);
	}
}

S32 LLAppearanceMgr::countActiveHoldingPatterns()
{
	return LLWearableHoldingPattern::countActive();
}

static void remove_non_link_items(LLInventoryModel::item_array_t &items)
{
	LLInventoryModel::item_array_t pruned_items;
	for (LLInventoryModel::item_array_t::const_iterator iter = items.begin();
		 iter != items.end();
		 ++iter)
	{
 		const LLViewerInventoryItem *item = (*iter);
		if (item && item->getIsLinkType())
		{
			pruned_items.push_back((*iter));
		}
	}
	items = pruned_items;
}

//a predicate for sorting inventory items by actual descriptions
bool sort_by_actual_description(const LLInventoryItem* item1, const LLInventoryItem* item2)
{
	if (!item1 || !item2) 
	{
		LL_WARNS() << "either item1 or item2 is NULL" << LL_ENDL;
		return true;
	}

	return item1->getActualDescription() < item2->getActualDescription();
}

void item_array_diff(LLInventoryModel::item_array_t& full_list,
					 LLInventoryModel::item_array_t& keep_list,
					 LLInventoryModel::item_array_t& kill_list)
	
{
	for (LLInventoryModel::item_array_t::iterator it = full_list.begin();
		 it != full_list.end();
		 ++it)
	{
		LLViewerInventoryItem *item = *it;
		if (std::find(keep_list.begin(), keep_list.end(), item) == keep_list.end())
		{
			kill_list.push_back(item);
		}
	}
}

S32 LLAppearanceMgr::findExcessOrDuplicateItems(const LLUUID& cat_id,
												 LLAssetType::EType type,
												 S32 max_items_per_type,
												 S32 max_items_total,
												 LLInventoryObject::object_list_t& items_to_kill)
{
	S32 to_kill_count = 0;

	LLInventoryModel::item_array_t items;
	getDescendentsOfAssetType(cat_id, items, type);
	LLInventoryModel::item_array_t curr_items = items;
	removeDuplicateItems(items);
	if (max_items_per_type > 0 || max_items_total > 0)
	{
		filterWearableItems(items, max_items_per_type, max_items_total);
	}
	LLInventoryModel::item_array_t kill_items;
	item_array_diff(curr_items,items,kill_items);
	for (LLInventoryModel::item_array_t::iterator it = kill_items.begin();
		 it != kill_items.end();
		 ++it)
	{
		items_to_kill.push_back(LLPointer<LLInventoryObject>(*it));
		to_kill_count++;
	}
	return to_kill_count;
}
	

void LLAppearanceMgr::findAllExcessOrDuplicateItems(const LLUUID& cat_id,
													LLInventoryObject::object_list_t& items_to_kill)
{
	findExcessOrDuplicateItems(cat_id,LLAssetType::AT_BODYPART,
							   1, 0, items_to_kill);
	findExcessOrDuplicateItems(cat_id,LLAssetType::AT_CLOTHING,
							   0, LLAgentWearables::MAX_CLOTHING_LAYERS, items_to_kill);
	findExcessOrDuplicateItems(cat_id,LLAssetType::AT_OBJECT,
							   0, 0, items_to_kill);
}

void LLAppearanceMgr::enforceCOFItemRestrictions(LLPointer<LLInventoryCallback> cb)
{
	LLInventoryObject::object_list_t items_to_kill;
	findAllExcessOrDuplicateItems(getCOF(), items_to_kill);
	if (items_to_kill.size()>0)
	{
		// Remove duplicate or excess wearables. Should normally be enforced at the UI level, but
		// this should catch anything that gets through.
		remove_inventory_items(items_to_kill, cb);
	}
}

void LLAppearanceMgr::updateAppearanceFromCOF(bool enforce_item_restrictions,
											  bool enforce_ordering,
											  nullary_func_t post_update_func)
{
	if (mIsInUpdateAppearanceFromCOF)
	{
		LL_WARNS() << "Called updateAppearanceFromCOF inside updateAppearanceFromCOF, skipping" << LL_ENDL;
		return;
	}

	LL_DEBUGS("Avatar") << self_av_string() << "starting" << LL_ENDL;

	if (enforce_item_restrictions)
	{
		// The point here is just to call
		// updateAppearanceFromCOF() again after excess items
		// have been removed. That time we will set
		// enforce_item_restrictions to false so we don't get
		// caught in a perpetual loop.
		LLPointer<LLInventoryCallback> cb(
			new LLUpdateAppearanceOnDestroy(false, enforce_ordering, post_update_func));
		enforceCOFItemRestrictions(cb);
		return;
	}

	if (enforce_ordering)
	{
		//checking integrity of the COF in terms of ordering of wearables, 
		//checking and updating links' descriptions of wearables in the COF (before analyzed for "dirty" state)

		// As with enforce_item_restrictions handling above, we want
		// to wait for the update callbacks, then (finally!) call
		// updateAppearanceFromCOF() with no additional COF munging needed.
		LLPointer<LLInventoryCallback> cb(
			new LLUpdateAppearanceOnDestroy(false, false, post_update_func));
		updateClothingOrderingInfo(LLUUID::null, cb);
		return;
	}

	if (!validateClothingOrderingInfo())
	{
		LL_WARNS() << "Clothing ordering error" << LL_ENDL;
	}

	BoolSetter setIsInUpdateAppearanceFromCOF(mIsInUpdateAppearanceFromCOF);
	selfStartPhase("update_appearance_from_cof");

	// update dirty flag to see if the state of the COF matches
	// the saved outfit stored as a folder link
	updateIsDirty();

	// Send server request for appearance update
	if (gAgent.getRegion() && gAgent.getRegion()->getCentralBakeVersion())
	{
		requestServerAppearanceUpdate();
	}

	LLUUID current_outfit_id = getCOF();

	// Find all the wearables that are in the COF's subtree.
	LL_DEBUGS() << "LLAppearanceMgr::updateFromCOF()" << LL_ENDL;
	LLInventoryModel::item_array_t wear_items;
	LLInventoryModel::item_array_t obj_items;
	LLInventoryModel::item_array_t gest_items;
	getUserDescendents(current_outfit_id, wear_items, obj_items, gest_items);
	// Get rid of non-links in case somehow the COF was corrupted.
	remove_non_link_items(wear_items);
	remove_non_link_items(obj_items);
	remove_non_link_items(gest_items);

	dumpItemArray(wear_items,"asset_dump: wear_item");
	dumpItemArray(obj_items,"asset_dump: obj_item");

	LLViewerInventoryCategory *cof = gInventory.getCategory(current_outfit_id);
	if (!gInventory.isCategoryComplete(current_outfit_id))
	{
		LL_WARNS() << "COF info is not complete. Version " << cof->getVersion()
				<< " descendent_count " << cof->getDescendentCount()
				<< " viewer desc count " << cof->getViewerDescendentCount() << LL_ENDL;
	}
	if(!wear_items.size())
	{
		LLNotificationsUtil::add("CouldNotPutOnOutfit");
		return;
	}

	//preparing the list of wearables in the correct order for LLAgentWearables
	sortItemsByActualDescription(wear_items);


	LL_DEBUGS("Avatar") << "HP block starts" << LL_ENDL;
	LLTimer hp_block_timer;
	LLWearableHoldingPattern* holder = new LLWearableHoldingPattern;

	holder->setObjItems(obj_items);
	holder->setGestItems(gest_items);
		
	// Note: can't do normal iteration, because if all the
	// wearables can be resolved immediately, then the
	// callback will be called (and this object deleted)
	// before the final getNextData().

	for(S32 i = 0; i  < wear_items.size(); ++i)
	{
		LLViewerInventoryItem *item = wear_items.at(i);
		LLViewerInventoryItem *linked_item = item ? item->getLinkedItem() : NULL;

		// Fault injection: use debug setting to test asset 
		// fetch failures (should be replaced by new defaults in
		// lost&found).
		U32 skip_type = gSavedSettings.getU32("ForceAssetFail");

		if (item && item->getIsLinkType() && linked_item)
		{
			LLFoundData found(linked_item->getUUID(),
							  linked_item->getAssetUUID(),
							  linked_item->getName(),
							  linked_item->getType(),
							  linked_item->isWearableType() ? linked_item->getWearableType() : LLWearableType::WT_INVALID
				);

			if (skip_type != LLWearableType::WT_INVALID && skip_type == found.mWearableType)
			{
				found.mAssetID.generate(); // Replace with new UUID, guaranteed not to exist in DB
			}
			//pushing back, not front, to preserve order of wearables for LLAgentWearables
			holder->getFoundList().push_back(found);
		}
		else
		{
			if (!item)
			{
				LL_WARNS() << "Attempt to wear a null item " << LL_ENDL;
			}
			else if (!linked_item)
			{
				LL_WARNS() << "Attempt to wear a broken link [ name:" << item->getName() << " ] " << LL_ENDL;
			}
		}
	}

	selfStartPhase("get_wearables_2");

	for (LLWearableHoldingPattern::found_list_t::iterator it = holder->getFoundList().begin();
		 it != holder->getFoundList().end(); ++it)
	{
		LLFoundData& found = *it;

		LL_DEBUGS() << self_av_string() << "waiting for onWearableAssetFetch callback, asset " << found.mAssetID.asString() << LL_ENDL;

		// Fetch the wearables about to be worn.
		LLWearableList::instance().getAsset(found.mAssetID,
											found.mName,
											gAgentAvatarp,
											found.mAssetType,
											onWearableAssetFetch,
											(void*)holder);

	}

	holder->resetTime(gSavedSettings.getF32("MaxWearableWaitTime"));
	if (!holder->pollFetchCompletion())
	{
		doOnIdleRepeating(boost::bind(&LLWearableHoldingPattern::pollFetchCompletion,holder));
	}
	post_update_func();

	LL_DEBUGS("Avatar") << "HP block ends, elapsed " << hp_block_timer.getElapsedTimeF32() << LL_ENDL;
}

void LLAppearanceMgr::getDescendentsOfAssetType(const LLUUID& category,
													LLInventoryModel::item_array_t& items,
													LLAssetType::EType type)
{
	LLInventoryModel::cat_array_t cats;
	LLIsType is_of_type(type);
	gInventory.collectDescendentsIf(category,
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_of_type);
}

void LLAppearanceMgr::getUserDescendents(const LLUUID& category, 
											 LLInventoryModel::item_array_t& wear_items,
											 LLInventoryModel::item_array_t& obj_items,
											 LLInventoryModel::item_array_t& gest_items)
{
	LLInventoryModel::cat_array_t wear_cats;
	LLFindWearables is_wearable;
	gInventory.collectDescendentsIf(category,
									wear_cats,
									wear_items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_wearable);

	LLInventoryModel::cat_array_t obj_cats;
	LLIsType is_object( LLAssetType::AT_OBJECT );
	gInventory.collectDescendentsIf(category,
									obj_cats,
									obj_items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_object);

	// Find all gestures in this folder
	LLInventoryModel::cat_array_t gest_cats;
	LLIsType is_gesture( LLAssetType::AT_GESTURE );
	gInventory.collectDescendentsIf(category,
									gest_cats,
									gest_items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_gesture);
}

void LLAppearanceMgr::wearInventoryCategory(LLInventoryCategory* category, bool copy, bool append)
{
	if(!category) return;

	selfClearPhases();
	selfStartPhase("wear_inventory_category");

	gAgentWearables.notifyLoadingStarted();

	LL_INFOS("Avatar") << self_av_string() << "wearInventoryCategory( " << category->getName()
			 << " )" << LL_ENDL;

	// If we are copying from library, attempt to use AIS to copy the category.
    if (copy && AISAPI::isAvailable())
	{
		LLUUID parent_id;
		parent_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_CLOTHING);
		if (parent_id.isNull())
		{
			parent_id = gInventory.getRootFolderID();
		}

		LLPointer<LLInventoryCallback> copy_cb = new LLWearCategoryAfterCopy(append);
		LLPointer<LLInventoryCallback> track_cb = new LLTrackPhaseWrapper(
													std::string("wear_inventory_category_callback"), copy_cb);

        AISAPI::completion_t cr = boost::bind(&doAppearanceCb, track_cb, _1);
        AISAPI::CopyLibraryCategory(category->getUUID(), parent_id, false, cr);
	}
    else
	{
		selfStartPhase("wear_inventory_category_fetch");
		callAfterCategoryFetch(category->getUUID(),boost::bind(&LLAppearanceMgr::wearCategoryFinal,
															   &LLAppearanceMgr::instance(),
															   category->getUUID(), copy, append));
	}
}

S32 LLAppearanceMgr::getActiveCopyOperations() const
{
	return LLCallAfterInventoryCopyMgr::getInstanceCount(); 
}

void LLAppearanceMgr::wearCategoryFinal(LLUUID& cat_id, bool copy_items, bool append)
{
	LL_INFOS("Avatar") << self_av_string() << "starting" << LL_ENDL;

	selfStopPhase("wear_inventory_category_fetch");
	
	// We now have an outfit ready to be copied to agent inventory. Do
	// it, and wear that outfit normally.
	LLInventoryCategory* cat = gInventory.getCategory(cat_id);
	if(copy_items)
	{
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf(cat_id, cats, items);
		std::string name;
		if(!cat)
		{
			// should never happen.
			name = "New Outfit";
		}
		else
		{
			name = cat->getName();
		}
		LLViewerInventoryItem* item = NULL;
		LLInventoryModel::item_array_t::const_iterator it = items->begin();
		LLInventoryModel::item_array_t::const_iterator end = items->end();
		LLUUID pid;
		for(; it < end; ++it)
		{
			item = *it;
			if(item)
			{
				if(LLInventoryType::IT_GESTURE == item->getInventoryType())
				{
					pid = gInventory.findCategoryUUIDForType(LLFolderType::FT_GESTURE);
				}
				else
				{
					pid = gInventory.findCategoryUUIDForType(LLFolderType::FT_CLOTHING);
				}
				break;
			}
		}
		if(pid.isNull())
		{
			pid = gInventory.getRootFolderID();
		}
		
		LLUUID new_cat_id = gInventory.createNewCategory(
			pid,
			LLFolderType::FT_NONE,
			name);

		// Create a CopyMgr that will copy items, manage its own destruction
		new LLCallAfterInventoryCopyMgr(
			*items, new_cat_id, std::string("wear_inventory_category_callback"),
			boost::bind(&LLAppearanceMgr::wearInventoryCategoryOnAvatar,
						LLAppearanceMgr::getInstance(),
						gInventory.getCategory(new_cat_id),
						append));

		// BAP fixes a lag in display of created dir.
		gInventory.notifyObservers();
	}
	else
	{
		// Wear the inventory category.
		LLAppearanceMgr::instance().wearInventoryCategoryOnAvatar(cat, append);
	}
}

// *NOTE: hack to get from avatar inventory to avatar
void LLAppearanceMgr::wearInventoryCategoryOnAvatar( LLInventoryCategory* category, bool append )
{
	// Avoid unintentionally overwriting old wearables.  We have to do
	// this up front to avoid having to deal with the case of multiple
	// wearables being dirty.
	if (!category) return;

	if ( !LLInventoryCallbackManager::is_instantiated() )
	{
		// shutting down, ignore.
		return;
	}

	LL_INFOS("Avatar") << self_av_string() << "wearInventoryCategoryOnAvatar '" << category->getName()
			 << "'" << LL_ENDL;
			 	
	if (gAgentCamera.cameraCustomizeAvatar())
	{
		// switching to outfit editor should automagically save any currently edited wearable
		LLFloaterSidePanelContainer::showPanel("appearance", LLSD().with("type", "edit_outfit"));
	}

	LLAppearanceMgr::changeOutfit(TRUE, category->getUUID(), append);
}

// FIXME do we really want to search entire inventory for matching name?
void LLAppearanceMgr::wearOutfitByName(const std::string& name)
{
	LL_INFOS("Avatar") << self_av_string() << "Wearing category " << name << LL_ENDL;

	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	LLNameCategoryCollector has_name(name);
	gInventory.collectDescendentsIf(gInventory.getRootFolderID(),
									cat_array,
									item_array,
									LLInventoryModel::EXCLUDE_TRASH,
									has_name);
	bool copy_items = false;
	LLInventoryCategory* cat = NULL;
	if (cat_array.size() > 0)
	{
		// Just wear the first one that matches
		cat = cat_array.at(0);
	}
	else
	{
		gInventory.collectDescendentsIf(LLUUID::null,
										cat_array,
										item_array,
										LLInventoryModel::EXCLUDE_TRASH,
										has_name);
		if(cat_array.size() > 0)
		{
			cat = cat_array.at(0);
			copy_items = true;
		}
	}

	if(cat)
	{
		LLAppearanceMgr::wearInventoryCategory(cat, copy_items, false);
	}
	else
	{
		LL_WARNS() << "Couldn't find outfit " <<name<< " in wearOutfitByName()"
				<< LL_ENDL;
	}
}

bool areMatchingWearables(const LLViewerInventoryItem *a, const LLViewerInventoryItem *b)
{
	return (a->isWearableType() && b->isWearableType() &&
			(a->getWearableType() == b->getWearableType()));
}

class LLDeferredCOFLinkObserver: public LLInventoryObserver
{
public:
	LLDeferredCOFLinkObserver(const LLUUID& item_id, LLPointer<LLInventoryCallback> cb, const std::string& description):
		mItemID(item_id),
		mCallback(cb),
		mDescription(description)
	{
	}

	~LLDeferredCOFLinkObserver()
	{
	}
	
	/* virtual */ void changed(U32 mask)
	{
		const LLInventoryItem *item = gInventory.getItem(mItemID);
		if (item)
		{
			gInventory.removeObserver(this);
			LLAppearanceMgr::instance().addCOFItemLink(item, mCallback, mDescription);
			delete this;
		}
	}

private:
	const LLUUID mItemID;
	std::string mDescription;
	LLPointer<LLInventoryCallback> mCallback;
};


// BAP - note that this runs asynchronously if the item is not already loaded from inventory.
// Dangerous if caller assumes link will exist after calling the function.
void LLAppearanceMgr::addCOFItemLink(const LLUUID &item_id,
									 LLPointer<LLInventoryCallback> cb,
									 const std::string description)
{
	const LLInventoryItem *item = gInventory.getItem(item_id);
	if (!item)
	{
		LLDeferredCOFLinkObserver *observer = new LLDeferredCOFLinkObserver(item_id, cb, description);
		gInventory.addObserver(observer);
	}
	else
	{
		addCOFItemLink(item, cb, description);
	}
}

void LLAppearanceMgr::addCOFItemLink(const LLInventoryItem *item,
									 LLPointer<LLInventoryCallback> cb,
									 const std::string description)
{
	const LLViewerInventoryItem *vitem = dynamic_cast<const LLViewerInventoryItem*>(item);
	if (!vitem)
	{
		LL_WARNS() << "not an llviewerinventoryitem, failed" << LL_ENDL;
		return;
	}

	gInventory.addChangedMask(LLInventoryObserver::LABEL, vitem->getLinkedUUID());

	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	gInventory.collectDescendents(LLAppearanceMgr::getCOF(),
								  cat_array,
								  item_array,
								  LLInventoryModel::EXCLUDE_TRASH);
	bool linked_already = false;
	for (S32 i=0; i<item_array.size(); i++)
	{
		// Are these links to the same object?
		const LLViewerInventoryItem* inv_item = item_array.at(i).get();
		const LLWearableType::EType wearable_type = inv_item->getWearableType();

		const bool is_body_part =    (wearable_type == LLWearableType::WT_SHAPE) 
								  || (wearable_type == LLWearableType::WT_HAIR) 
								  || (wearable_type == LLWearableType::WT_EYES)
								  || (wearable_type == LLWearableType::WT_SKIN);

		if (inv_item->getLinkedUUID() == vitem->getLinkedUUID())
		{
			linked_already = true;
		}
		// Are these links to different items of the same body part
		// type? If so, new item will replace old.
		else if ((vitem->isWearableType()) && (vitem->getWearableType() == wearable_type))
		{
			if (is_body_part && inv_item->getIsLinkType())
			{
				remove_inventory_item(inv_item->getUUID(), cb);
			}
			else if (!gAgentWearables.canAddWearable(wearable_type))
			{
				// MULTI-WEARABLES: make sure we don't go over clothing limits
				remove_inventory_item(inv_item->getUUID(), cb);
			}
		}
	}

	if (!linked_already)
	{
		LLViewerInventoryItem *copy_item = new LLViewerInventoryItem;
		copy_item->copyViewerItem(vitem);
		copy_item->setDescription(description);
		link_inventory_object(getCOF(), copy_item, cb);
	}
}

LLInventoryModel::item_array_t LLAppearanceMgr::findCOFItemLinks(const LLUUID& item_id)
{
	LLInventoryModel::item_array_t result;

    LLUUID linked_id = gInventory.getLinkedItemID(item_id);
    LLInventoryModel::cat_array_t cat_array;
    LLInventoryModel::item_array_t item_array;
    gInventory.collectDescendents(LLAppearanceMgr::getCOF(),
                                  cat_array,
                                  item_array,
                                  LLInventoryModel::EXCLUDE_TRASH);
    for (S32 i=0; i<item_array.size(); i++)
    {
        const LLViewerInventoryItem* inv_item = item_array.at(i).get();
        if (inv_item->getLinkedUUID() == linked_id)
        {
            result.push_back(item_array.at(i));
        }
    }
	return result;
}

bool LLAppearanceMgr::isLinkedInCOF(const LLUUID& item_id)
{
	LLInventoryModel::item_array_t links = LLAppearanceMgr::instance().findCOFItemLinks(item_id);
	return links.size() > 0;
}

void LLAppearanceMgr::removeAllClothesFromAvatar()
{
	// Fetch worn clothes (i.e. the ones in COF).
	LLInventoryModel::item_array_t clothing_items;
	LLInventoryModel::cat_array_t dummy;
	LLIsType is_clothing(LLAssetType::AT_CLOTHING);
	gInventory.collectDescendentsIf(getCOF(),
									dummy,
									clothing_items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_clothing);
	uuid_vec_t item_ids;
	for (LLInventoryModel::item_array_t::iterator it = clothing_items.begin();
		it != clothing_items.end(); ++it)
	{
		item_ids.push_back((*it).get()->getLinkedUUID());
	}

	// Take them off by removing from COF.
	removeItemsFromAvatar(item_ids);
}

void LLAppearanceMgr::removeAllAttachmentsFromAvatar()
{
	if (!isAgentAvatarValid()) return;

	LLAgentWearables::llvo_vec_t objects_to_remove;
	
	for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
		 iter != gAgentAvatarp->mAttachmentPoints.end();)
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
			 attachment_iter != attachment->mAttachedObjects.end();
			 ++attachment_iter)
		{
			LLViewerObject *attached_object = (*attachment_iter);
			if (attached_object)
			{
				objects_to_remove.push_back(attached_object);
			}
		}
	}
	uuid_vec_t ids_to_remove;
	for (LLAgentWearables::llvo_vec_t::iterator it = objects_to_remove.begin();
		 it != objects_to_remove.end();
		 ++it)
	{
		ids_to_remove.push_back((*it)->getAttachmentItemID());
	}
	removeItemsFromAvatar(ids_to_remove);
}

class LLUpdateOnCOFLinkRemove : public LLInventoryCallback
{
public:
	LLUpdateOnCOFLinkRemove(const LLUUID& remove_item_id, LLPointer<LLInventoryCallback> cb = NULL):
		mItemID(remove_item_id),
		mCB(cb)
	{
	}

	/* virtual */ void fire(const LLUUID& item_id)
	{
		// just removed cof link, "(wear)" suffix depends on presence of link, so update label
		gInventory.addChangedMask(LLInventoryObserver::LABEL, mItemID);
		if (mCB.notNull())
		{
			mCB->fire(item_id);
		}
	}

private:
	LLUUID mItemID;
	LLPointer<LLInventoryCallback> mCB;
};

void LLAppearanceMgr::removeCOFItemLinks(const LLUUID& item_id, LLPointer<LLInventoryCallback> cb)
{	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	gInventory.collectDescendents(LLAppearanceMgr::getCOF(),
								  cat_array,
								  item_array,
								  LLInventoryModel::EXCLUDE_TRASH);
	for (S32 i=0; i<item_array.size(); i++)
	{
		const LLInventoryItem* item = item_array.at(i).get();
		if (item->getIsLinkType() && item->getLinkedUUID() == item_id)
		{
			if (item->getType() == LLAssetType::AT_OBJECT)
			{
				// Immediate delete
				remove_inventory_item(item->getUUID(), cb, true);
				gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);
			}
			else
			{
				// Delayed delete
				// Pointless to update item_id label here since link still exists and first notifyObservers
				// call will restore (wear) suffix, mark for update after deletion
				LLPointer<LLUpdateOnCOFLinkRemove> cb_label = new LLUpdateOnCOFLinkRemove(item_id, cb);
				remove_inventory_item(item->getUUID(), cb_label, false);
			}
		}
	}
}

void LLAppearanceMgr::removeCOFLinksOfType(LLWearableType::EType type, LLPointer<LLInventoryCallback> cb)
{
	LLFindWearablesOfType filter_wearables_of_type(type);
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLInventoryModel::item_array_t::const_iterator it;

	gInventory.collectDescendentsIf(getCOF(), cats, items, true, filter_wearables_of_type);
	for (it = items.begin(); it != items.end(); ++it)
	{
		const LLViewerInventoryItem* item = *it;
		if (item->getIsLinkType()) // we must operate on links only
		{
			remove_inventory_item(item->getUUID(), cb);
		}
	}
}

bool sort_by_linked_uuid(const LLViewerInventoryItem* item1, const LLViewerInventoryItem* item2)
{
	if (!item1 || !item2)
	{
		LL_WARNS() << "item1, item2 cannot be null, something is very wrong" << LL_ENDL;
		return true;
	}

	return item1->getLinkedUUID() < item2->getLinkedUUID();
}

void LLAppearanceMgr::updateIsDirty()
{
	LLUUID cof = getCOF();
	LLUUID base_outfit;

	// find base outfit link 
	const LLViewerInventoryItem* base_outfit_item = getBaseOutfitLink();
	LLViewerInventoryCategory* catp = NULL;
	if (base_outfit_item && base_outfit_item->getIsLinkType())
	{
		catp = base_outfit_item->getLinkedCategory();
	}
	if(catp && catp->getPreferredType() == LLFolderType::FT_OUTFIT)
	{
		base_outfit = catp->getUUID();
	}

	// Set dirty to "false" if no base outfit found to disable "Save"
	// and leave only "Save As" enabled in My Outfits.
	mOutfitIsDirty = false;

	if (base_outfit.notNull())
	{
		LLIsValidItemLink collector;

		LLInventoryModel::cat_array_t cof_cats;
		LLInventoryModel::item_array_t cof_items;
		gInventory.collectDescendentsIf(cof, cof_cats, cof_items,
									  LLInventoryModel::EXCLUDE_TRASH, collector);

		LLInventoryModel::cat_array_t outfit_cats;
		LLInventoryModel::item_array_t outfit_items;
		gInventory.collectDescendentsIf(base_outfit, outfit_cats, outfit_items,
									  LLInventoryModel::EXCLUDE_TRASH, collector);

		for (U32 i = 0; i < outfit_items.size(); ++i)
		{
			LLViewerInventoryItem* linked_item = outfit_items.at(i)->getLinkedItem();
			if (linked_item != NULL && linked_item->getActualType() == LLAssetType::AT_TEXTURE)
			{
				outfit_items.erase(outfit_items.begin() + i);
				break;
			}
		}

		if(outfit_items.size() != cof_items.size())
		{
			LL_DEBUGS("Avatar") << "item count different - base " << outfit_items.size() << " cof " << cof_items.size() << LL_ENDL;
			// Current outfit folder should have one more item than the outfit folder.
			// this one item is the link back to the outfit folder itself.
			mOutfitIsDirty = true;
			return;
		}

		//"dirty" - also means a difference in linked UUIDs and/or a difference in wearables order (links' descriptions)
		std::sort(cof_items.begin(), cof_items.end(), sort_by_linked_uuid);
		std::sort(outfit_items.begin(), outfit_items.end(), sort_by_linked_uuid);

		for (U32 i = 0; i < cof_items.size(); ++i)
		{
			LLViewerInventoryItem *item1 = cof_items.at(i);
			LLViewerInventoryItem *item2 = outfit_items.at(i);

			if (item1->getLinkedUUID() != item2->getLinkedUUID() || 
				item1->getName() != item2->getName() ||
				item1->getActualDescription() != item2->getActualDescription())
			{
				if (item1->getLinkedUUID() != item2->getLinkedUUID())
				{
					LL_DEBUGS("Avatar") << "link id different " << LL_ENDL;
				}
				else
				{
					if (item1->getName() != item2->getName())
					{
						LL_DEBUGS("Avatar") << "name different " << item1->getName() << " " << item2->getName() << LL_ENDL;
					}
					if (item1->getActualDescription() != item2->getActualDescription())
					{
						LL_DEBUGS("Avatar") << "desc different " << item1->getActualDescription()
											<< " " << item2->getActualDescription() 
											<< " names " << item1->getName() << " " << item2->getName() << LL_ENDL;
					}
				}
				mOutfitIsDirty = true;
				return;
			}
		}
	}
	llassert(!mOutfitIsDirty);
	LL_DEBUGS("Avatar") << "clean" << LL_ENDL;
}

// *HACK: Must match name in Library or agent inventory
const std::string ROOT_GESTURES_FOLDER = "Gestures";
const std::string COMMON_GESTURES_FOLDER = "Common Gestures";
const std::string MALE_GESTURES_FOLDER = "Male Gestures";
const std::string FEMALE_GESTURES_FOLDER = "Female Gestures";
const std::string SPEECH_GESTURES_FOLDER = "Speech Gestures";
const std::string OTHER_GESTURES_FOLDER = "Other Gestures";

void LLAppearanceMgr::copyLibraryGestures()
{
	LL_INFOS("Avatar") << self_av_string() << "Copying library gestures" << LL_ENDL;

	// Copy gestures
	LLUUID lib_gesture_cat_id =
		gInventory.findLibraryCategoryUUIDForType(LLFolderType::FT_GESTURE,false);
	if (lib_gesture_cat_id.isNull())
	{
		LL_WARNS() << "Unable to copy gestures, source category not found" << LL_ENDL;
	}
	LLUUID dst_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_GESTURE);

	std::vector<std::string> gesture_folders_to_copy;
	gesture_folders_to_copy.push_back(MALE_GESTURES_FOLDER);
	gesture_folders_to_copy.push_back(FEMALE_GESTURES_FOLDER);
	gesture_folders_to_copy.push_back(COMMON_GESTURES_FOLDER);
	gesture_folders_to_copy.push_back(SPEECH_GESTURES_FOLDER);
	gesture_folders_to_copy.push_back(OTHER_GESTURES_FOLDER);

	for(std::vector<std::string>::iterator it = gesture_folders_to_copy.begin();
		it != gesture_folders_to_copy.end();
		++it)
	{
		std::string& folder_name = *it;

		LLPointer<LLInventoryCallback> cb(NULL);

		// After copying gestures, activate Common, Other, plus
		// Male and/or Female, depending upon the initial outfit gender.
		ESex gender = gAgentAvatarp->getSex();

		std::string activate_male_gestures;
		std::string activate_female_gestures;
		switch (gender) {
			case SEX_MALE:
				activate_male_gestures = MALE_GESTURES_FOLDER;
				break;
			case SEX_FEMALE:
				activate_female_gestures = FEMALE_GESTURES_FOLDER;
				break;
			case SEX_BOTH:
				activate_male_gestures = MALE_GESTURES_FOLDER;
				activate_female_gestures = FEMALE_GESTURES_FOLDER;
				break;
		}

		if (folder_name == activate_male_gestures ||
			folder_name == activate_female_gestures ||
			folder_name == COMMON_GESTURES_FOLDER ||
			folder_name == OTHER_GESTURES_FOLDER)
		{
			cb = new LLBoostFuncInventoryCallback(activate_gesture_cb);
		}

		LLUUID cat_id = findDescendentCategoryIDByName(lib_gesture_cat_id,folder_name);
		if (cat_id.isNull())
		{
			LL_WARNS() << self_av_string() << "failed to find gesture folder for " << folder_name << LL_ENDL;
		}
		else
		{
			LL_DEBUGS("Avatar") << self_av_string() << "initiating fetch and copy for " << folder_name << " cat_id " << cat_id << LL_ENDL;
			callAfterCategoryFetch(cat_id,
								   boost::bind(&LLAppearanceMgr::shallowCopyCategory,
											   &LLAppearanceMgr::instance(),
											   cat_id, dst_id, cb));
		}
	}
}

// Handler for anything that's deferred until avatar de-clouds.
void LLAppearanceMgr::onFirstFullyVisible()
{
	gAgentAvatarp->outputRezTiming("Avatar fully loaded");
	gAgentAvatarp->reportAvatarRezTime();
	gAgentAvatarp->debugAvatarVisible();

	// If this is the first time we've ever logged in,
	// then copy default gestures from the library.
	if (gAgent.isFirstLogin()) {
		copyLibraryGestures();
	}
}

// update "dirty" state - defined outside class to allow for calling
// after appearance mgr instance has been destroyed.
void appearance_mgr_update_dirty_state()
{
	if (LLAppearanceMgr::instanceExists())
	{
		LLAppearanceMgr& app_mgr = LLAppearanceMgr::instance();
		LLUUID image_id = app_mgr.getOutfitImage();
		if(image_id.notNull())
		{
			LLPointer<LLInventoryCallback> cb = NULL;
			link_inventory_object(app_mgr.getBaseOutfitUUID(), image_id, cb);
		}

		LLAppearanceMgr::getInstance()->updateIsDirty();
		LLAppearanceMgr::getInstance()->setOutfitLocked(false);
		gAgentWearables.notifyLoadingFinished();
	}
}

void update_base_outfit_after_ordering()
{
	LLAppearanceMgr& app_mgr = LLAppearanceMgr::instance();
	app_mgr.setOutfitImage(LLUUID());
	LLInventoryModel::cat_array_t sub_cat_array;
	LLInventoryModel::item_array_t outfit_item_array;
	gInventory.collectDescendents(app_mgr.getBaseOutfitUUID(),
								sub_cat_array,
								outfit_item_array,
								LLInventoryModel::EXCLUDE_TRASH);
	BOOST_FOREACH(LLViewerInventoryItem* outfit_item, outfit_item_array)
	{
		LLViewerInventoryItem* linked_item = outfit_item->getLinkedItem();
		if (linked_item != NULL)
		{
			if (linked_item->getActualType() == LLAssetType::AT_TEXTURE)
			{
				app_mgr.setOutfitImage(linked_item->getLinkedUUID());
				if (linked_item->getName() == LLAppearanceMgr::sExpectedTextureName)
				{
					// Images with "appropriate" name take priority
					break;
				}
			}
		}
		else if (outfit_item->getActualType() == LLAssetType::AT_TEXTURE)
		{
			app_mgr.setOutfitImage(outfit_item->getUUID());
			if (outfit_item->getName() == LLAppearanceMgr::sExpectedTextureName)
			{
				// Images with "appropriate" name take priority
				break;
			}
		}
	}

	LLPointer<LLInventoryCallback> dirty_state_updater =
		new LLBoostFuncInventoryCallback(no_op_inventory_func, appearance_mgr_update_dirty_state);

	//COF contains only links so we copy to the Base Outfit only links
	const LLUUID base_outfit_id = app_mgr.getBaseOutfitUUID();
	bool copy_folder_links = false;
	app_mgr.slamCategoryLinks(app_mgr.getCOF(), base_outfit_id, copy_folder_links, dirty_state_updater);

}

// Save COF changes - update the contents of the current base outfit
// to match the current COF. Fails if no current base outfit is set.
bool LLAppearanceMgr::updateBaseOutfit()
{
	if (isOutfitLocked())
	{
		// don't allow modify locked outfit
		llassert(!isOutfitLocked());
		return false;
	}

	setOutfitLocked(true);

	gAgentWearables.notifyLoadingStarted();

	const LLUUID base_outfit_id = getBaseOutfitUUID();
	if (base_outfit_id.isNull()) return false;
	LL_DEBUGS("Avatar") << "saving cof to base outfit " << base_outfit_id << LL_ENDL;

	LLPointer<LLInventoryCallback> cb =
		new LLBoostFuncInventoryCallback(no_op_inventory_func, update_base_outfit_after_ordering);
	// Really shouldn't be needed unless there's a race condition -
	// updateAppearanceFromCOF() already calls updateClothingOrderingInfo.
	updateClothingOrderingInfo(LLUUID::null, cb);

	return true;
}

void LLAppearanceMgr::divvyWearablesByType(const LLInventoryModel::item_array_t& items, wearables_by_type_t& items_by_type)
{
	items_by_type.resize(LLWearableType::WT_COUNT);
	if (items.empty()) return;

	for (S32 i=0; i<items.size(); i++)
	{
		LLViewerInventoryItem *item = items.at(i);
		if (!item)
		{
			LL_WARNS("Appearance") << "NULL item found" << LL_ENDL;
			continue;
		}
		// Ignore non-wearables.
		if (!item->isWearableType())
			continue;
		LLWearableType::EType type = item->getWearableType();
		if(type < 0 || type >= LLWearableType::WT_COUNT)
		{
			LL_WARNS("Appearance") << "Invalid wearable type. Inventory type does not match wearable flag bitfield." << LL_ENDL;
			continue;
		}
		items_by_type[type].push_back(item);
	}
}

std::string build_order_string(LLWearableType::EType type, U32 i)
{
		std::ostringstream order_num;
		order_num << ORDER_NUMBER_SEPARATOR << type * 100 + i;
		return order_num.str();
}

struct WearablesOrderComparator
{
	LOG_CLASS(WearablesOrderComparator);
	WearablesOrderComparator(const LLWearableType::EType type)
	{
		mControlSize = build_order_string(type, 0).size();
	};

	bool operator()(const LLInventoryItem* item1, const LLInventoryItem* item2)
	{
		const std::string& desc1 = item1->getActualDescription();
		const std::string& desc2 = item2->getActualDescription();
		
		bool item1_valid = (desc1.size() == mControlSize) && (ORDER_NUMBER_SEPARATOR == desc1[0]);
		bool item2_valid = (desc2.size() == mControlSize) && (ORDER_NUMBER_SEPARATOR == desc2[0]);

		if (item1_valid && item2_valid)
			return desc1 < desc2;

		//we need to sink down invalid items: items with empty descriptions, items with "Broken link" descriptions,
		//items with ordering information but not for the associated wearables type
		if (!item1_valid && item2_valid) 
			return false;
		else if (item1_valid && !item2_valid)
			return true;

		return item1->getName() < item2->getName();
	}

	U32 mControlSize;
};

void LLAppearanceMgr::getWearableOrderingDescUpdates(LLInventoryModel::item_array_t& wear_items,
													 desc_map_t& desc_map)
{
	wearables_by_type_t items_by_type(LLWearableType::WT_COUNT);
	divvyWearablesByType(wear_items, items_by_type);

	for (U32 type = LLWearableType::WT_SHIRT; type < LLWearableType::WT_COUNT; type++)
	{
		U32 size = items_by_type[type].size();
		if (!size) continue;
		
		//sinking down invalid items which need reordering
		std::sort(items_by_type[type].begin(), items_by_type[type].end(), WearablesOrderComparator((LLWearableType::EType) type));
		
		//requesting updates only for those links which don't have "valid" descriptions
		for (U32 i = 0; i < size; i++)
		{
			LLViewerInventoryItem* item = items_by_type[type][i];
			if (!item) continue;
			
			std::string new_order_str = build_order_string((LLWearableType::EType)type, i);
			if (new_order_str == item->getActualDescription()) continue;
			
			desc_map[item->getUUID()] = new_order_str;
		}
	}
}

bool LLAppearanceMgr::validateClothingOrderingInfo(LLUUID cat_id)
{
	// COF is processed if cat_id is not specified
	if (cat_id.isNull())
	{
		cat_id = getCOF();
	}

	LLInventoryModel::item_array_t wear_items;
	getDescendentsOfAssetType(cat_id, wear_items, LLAssetType::AT_CLOTHING);

	// Identify items for which desc needs to change.
	desc_map_t desc_map;
	getWearableOrderingDescUpdates(wear_items, desc_map);

	for (desc_map_t::const_iterator it = desc_map.begin();
		 it != desc_map.end(); ++it)
	{
		const LLUUID& item_id = it->first;
		const std::string& new_order_str = it->second;
		LLViewerInventoryItem *item = gInventory.getItem(item_id);
		LL_WARNS() << "Order validation fails: " << item->getName()
				<< " needs to update desc to: " << new_order_str
				<< " (from: " << item->getActualDescription() << ")" << LL_ENDL;
	}
	
	return desc_map.size() == 0;
}

void LLAppearanceMgr::updateClothingOrderingInfo(LLUUID cat_id,
												 LLPointer<LLInventoryCallback> cb)
{
	// COF is processed if cat_id is not specified
	if (cat_id.isNull())
	{
		cat_id = getCOF();
	}

	LLInventoryModel::item_array_t wear_items;
	getDescendentsOfAssetType(cat_id, wear_items, LLAssetType::AT_CLOTHING);

	// Identify items for which desc needs to change.
	desc_map_t desc_map;
	getWearableOrderingDescUpdates(wear_items, desc_map);

	for (desc_map_t::const_iterator it = desc_map.begin();
		 it != desc_map.end(); ++it)
	{
		LLSD updates;
		const LLUUID& item_id = it->first;
		const std::string& new_order_str = it->second;
		LLViewerInventoryItem *item = gInventory.getItem(item_id);
		LL_DEBUGS("Avatar") << item->getName() << " updating desc to: " << new_order_str
			<< " (was: " << item->getActualDescription() << ")" << LL_ENDL;
		updates["desc"] = new_order_str;
		update_inventory_item(item_id,updates,cb);
	}
		
}


LLSD LLAppearanceMgr::dumpCOF() const
{
	LLSD links = LLSD::emptyArray();
	LLMD5 md5;
	
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	gInventory.collectDescendents(getCOF(),cat_array,item_array,LLInventoryModel::EXCLUDE_TRASH);
	for (S32 i=0; i<item_array.size(); i++)
	{
		const LLViewerInventoryItem* inv_item = item_array.at(i).get();
		LLSD item;
		LLUUID item_id(inv_item->getUUID());
		md5.update((unsigned char*)item_id.mData, 16);
		item["description"] = inv_item->getActualDescription();
		md5.update(inv_item->getActualDescription());
		item["asset_type"] = inv_item->getActualType();
		LLUUID linked_id(inv_item->getLinkedUUID());
		item["linked_id"] = linked_id;
		md5.update((unsigned char*)linked_id.mData, 16);

		if (LLAssetType::AT_LINK == inv_item->getActualType())
		{
			const LLViewerInventoryItem* linked_item = inv_item->getLinkedItem();
			if (NULL == linked_item)
			{
				LL_WARNS() << "Broken link for item '" << inv_item->getName()
						<< "' (" << inv_item->getUUID()
						<< ") during requestServerAppearanceUpdate" << LL_ENDL;
				continue;
			}
			// Some assets may be 'hidden' and show up as null in the viewer.
			//if (linked_item->getAssetUUID().isNull())
			//{
			//	LL_WARNS() << "Broken link (null asset) for item '" << inv_item->getName()
			//			<< "' (" << inv_item->getUUID()
			//			<< ") during requestServerAppearanceUpdate" << LL_ENDL;
			//	continue;
			//}
			LLUUID linked_asset_id(linked_item->getAssetUUID());
			md5.update((unsigned char*)linked_asset_id.mData, 16);
			U32 flags = linked_item->getFlags();
			md5.update(boost::lexical_cast<std::string>(flags));
		}
		else if (LLAssetType::AT_LINK_FOLDER != inv_item->getActualType())
		{
			LL_WARNS() << "Non-link item '" << inv_item->getName()
					<< "' (" << inv_item->getUUID()
					<< ") type " << (S32) inv_item->getActualType()
					<< " during requestServerAppearanceUpdate" << LL_ENDL;
			continue;
		}
		links.append(item);
	}
	LLSD result = LLSD::emptyMap();
	result["cof_contents"] = links;
	char cof_md5sum[MD5HEX_STR_SIZE];
	md5.finalize();
	md5.hex_digest(cof_md5sum);
	result["cof_md5sum"] = std::string(cof_md5sum);
	return result;
}

// static
void LLAppearanceMgr::onIdle(void *)
{
    LLAppearanceMgr* mgr = LLAppearanceMgr::getInstance();
    if (mgr->mRerequestAppearanceBake)
    {
        mgr->requestServerAppearanceUpdate();
    }
}

void LLAppearanceMgr::requestServerAppearanceUpdate()
{
    // Workaround: we shouldn't request update from server prior to uploading all attachments, but it is
    // complicated to check for pending attachment uploads, so we are just waiting for uploads to complete
    if (!mOutstandingAppearanceBakeRequest && gAssetStorage->getNumPendingUploads() == 0)
    {
        mRerequestAppearanceBake = false;
        LLCoprocedureManager::CoProcedure_t proc = boost::bind(&LLAppearanceMgr::serverAppearanceUpdateCoro, this, _1);
        LLCoprocedureManager::instance().enqueueCoprocedure("AIS", "LLAppearanceMgr::serverAppearanceUpdateCoro", proc);
    }
    else
    {
        // Shedule update
        mRerequestAppearanceBake = true;
    }
}

void LLAppearanceMgr::serverAppearanceUpdateCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &httpAdapter)
{
    BoolSetter outstanding(mOutstandingAppearanceBakeRequest);
    if (!gAgent.getRegion())
    {
        LL_WARNS("Avatar") << "Region not set, cannot request server appearance update" << LL_ENDL;
        return;
    }
    if (gAgent.getRegion()->getCentralBakeVersion() == 0)
    {
        LL_WARNS("Avatar") << "Region does not support baking" << LL_ENDL;
        return;
    }

    std::string url = gAgent.getRegion()->getCapability("UpdateAvatarAppearance");
    if (url.empty())
    {
        LL_WARNS("Agent") << "Could not retrieve region capability \"UpdateAvatarAppearance\"" << LL_ENDL;
        return;
    }

    //----------------
    if (gAgentAvatarp->isEditingAppearance())
    {
        LL_WARNS("Avatar") << "Avatar editing appearance, not sending request." << LL_ENDL;
        // don't send out appearance updates if in appearance editing mode
        return;
    }

    llcoro::suspend();
    S32 retryCount(0);
    bool bRetry;
    do
    {
        // If we have already received an update for this or higher cof version, 
        // put a warning in the log and cancel the request.
        S32 cofVersion = getCOFVersion();
        S32 lastRcv = gAgentAvatarp->mLastUpdateReceivedCOFVersion;
        S32 lastReq = gAgentAvatarp->mLastUpdateRequestCOFVersion;

        LL_INFOS("Avatar") << "Requesting COF version " << cofVersion <<
            " (Last Received:" << lastRcv << ")" <<
            " (Last Requested:" << lastReq << ")" << LL_ENDL;

        if (cofVersion == LLViewerInventoryCategory::VERSION_UNKNOWN)
        {
            LL_WARNS("AVatar") << "COF version is unknown... not requesting until COF version is known." << LL_ENDL;
            return;
        }
        else
        {
            if (cofVersion <= lastRcv)
            {
                LL_WARNS("Avatar") << "Have already received update for cof version " << lastRcv
                    << " but requesting for " << cofVersion << LL_ENDL;
                return;
            }
            if (lastReq >= cofVersion)
            {
                LL_WARNS("Avatar") << "Request already in flight for cof version " << lastReq
                    << " but requesting for " << cofVersion << LL_ENDL;
                return;
            }
        }

        // Actually send the request.
        LL_DEBUGS("Avatar") << "Will send request for cof_version " << cofVersion << LL_ENDL;

        bRetry = false;
        LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest());

        if (gSavedSettings.getBOOL("DebugForceAppearanceRequestFailure"))
        {
            cofVersion += 999;
            LL_WARNS("Avatar") << "Forcing version failure on COF Baking" << LL_ENDL;
        }

        LL_INFOS("Avatar") << "Requesting bake for COF version " << cofVersion << LL_ENDL;

        LLSD postData;
        if (gSavedSettings.getBOOL("DebugAvatarExperimentalServerAppearanceUpdate"))
        {
            postData = dumpCOF();
        }
        else
        {
            postData["cof_version"] = cofVersion;
        }

        gAgentAvatarp->mLastUpdateRequestCOFVersion = cofVersion;

        LLSD result = httpAdapter->postAndSuspend(httpRequest, url, postData);

        LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
        LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

        if (!status || !result["success"].asBoolean())
        {
            std::string message = (result.has("error")) ? result["error"].asString() : status.toString();
            LL_WARNS("Avatar") << "Appearance Failure. server responded with \"" << message << "\"" << LL_ENDL;

            // We may have requested a bake for a stale COF (especially if the inventory 
            // is still updating.  If that is the case re send the request with the 
            // corrected COF version.  (This may also be the case if the viewer is running 
            // on multiple machines.
            if (result.has("expected"))
            {
                S32 expectedCofVersion = result["expected"].asInteger();
                LL_WARNS("Avatar") << "Server expected " << expectedCofVersion << " as COF version" << LL_ENDL;

                // Force an update texture request for ourself.  The message will return
                // through the UDP and be handled in LLVOAvatar::processAvatarAppearance
                // this should ensure that we receive a new canonical COF from the sim
                // host. Hopefully it will return before the timeout.
                LLAvatarPropertiesProcessor::getInstance()->sendAvatarTexturesRequest(gAgent.getID());

                bRetry = true;
                // Wait for a 1/2 second before trying again.  Just to keep from asking too quickly.
                if (++retryCount > BAKE_RETRY_MAX_COUNT)
                {
                    LL_WARNS("Avatar") << "Bake retry count exceeded!" << LL_ENDL;
                    break;
                }
                F32 timeout = pow(BAKE_RETRY_TIMEOUT, static_cast<float>(retryCount)) - 1.0;

                LL_WARNS("Avatar") << "Bake retry #" << retryCount << " in " << timeout << " seconds." << LL_ENDL;

                llcoro::suspendUntilTimeout(timeout); 
                bRetry = true;
                continue;
            }
            else
            {
                LL_WARNS("Avatar") << "No retry attempted." << LL_ENDL;
                break;
            }
        }

        LL_DEBUGS("Avatar") << "succeeded" << LL_ENDL;
        if (gSavedSettings.getBOOL("DebugAvatarAppearanceMessage"))
        {
            dump_sequential_xml(gAgentAvatarp->getFullname() + "_appearance_request_ok", result);
        }

    } while (bRetry);
}

/*static*/
void LLAppearanceMgr::debugAppearanceUpdateCOF(const LLSD& content)
{
    dump_sequential_xml(gAgentAvatarp->getFullname() + "_appearance_request_error", content);

    LL_INFOS("Avatar") << "AIS COF, version received: " << content["expected"].asInteger()
        << " ================================= " << LL_ENDL;
    std::set<LLUUID> ais_items, local_items;
    const LLSD& cof_raw = content["cof_raw"];
    for (LLSD::array_const_iterator it = cof_raw.beginArray();
        it != cof_raw.endArray(); ++it)
    {
        const LLSD& item = *it;
        if (item["parent_id"].asUUID() == LLAppearanceMgr::instance().getCOF())
        {
            ais_items.insert(item["item_id"].asUUID());
            if (item["type"].asInteger() == 24) // link
            {
                LL_INFOS("Avatar") << "AIS Link: item_id: " << item["item_id"].asUUID()
                    << " linked_item_id: " << item["asset_id"].asUUID()
                    << " name: " << item["name"].asString()
                    << LL_ENDL;
            }
            else if (item["type"].asInteger() == 25) // folder link
            {
                LL_INFOS("Avatar") << "AIS Folder link: item_id: " << item["item_id"].asUUID()
                    << " linked_item_id: " << item["asset_id"].asUUID()
                    << " name: " << item["name"].asString()
                    << LL_ENDL;
            }
            else
            {
                LL_INFOS("Avatar") << "AIS Other: item_id: " << item["item_id"].asUUID()
                    << " linked_item_id: " << item["asset_id"].asUUID()
                    << " name: " << item["name"].asString()
                    << " type: " << item["type"].asInteger()
                    << LL_ENDL;
            }
        }
    }
    LL_INFOS("Avatar") << LL_ENDL;
    LL_INFOS("Avatar") << "Local COF, version requested: " << content["observed"].asInteger()
        << " ================================= " << LL_ENDL;
    LLInventoryModel::cat_array_t cat_array;
    LLInventoryModel::item_array_t item_array;
    gInventory.collectDescendents(LLAppearanceMgr::instance().getCOF(),
        cat_array, item_array, LLInventoryModel::EXCLUDE_TRASH);
    for (S32 i = 0; i < item_array.size(); i++)
    {
        const LLViewerInventoryItem* inv_item = item_array.at(i).get();
        local_items.insert(inv_item->getUUID());
        LL_INFOS("Avatar") << "LOCAL: item_id: " << inv_item->getUUID()
            << " linked_item_id: " << inv_item->getLinkedUUID()
            << " name: " << inv_item->getName()
            << " parent: " << inv_item->getParentUUID()
            << LL_ENDL;
    }
    LL_INFOS("Avatar") << " ================================= " << LL_ENDL;
    S32 local_only = 0, ais_only = 0;
    for (std::set<LLUUID>::iterator it = local_items.begin(); it != local_items.end(); ++it)
    {
        if (ais_items.find(*it) == ais_items.end())
        {
            LL_INFOS("Avatar") << "LOCAL ONLY: " << *it << LL_ENDL;
            local_only++;
        }
    }
    for (std::set<LLUUID>::iterator it = ais_items.begin(); it != ais_items.end(); ++it)
    {
        if (local_items.find(*it) == local_items.end())
        {
            LL_INFOS("Avatar") << "AIS ONLY: " << *it << LL_ENDL;
            ais_only++;
        }
    }
    if (local_only == 0 && ais_only == 0)
    {
        LL_INFOS("Avatar") << "COF contents identical, only version numbers differ (req "
            << content["observed"].asInteger()
            << " rcv " << content["expected"].asInteger()
            << ")" << LL_ENDL;
    }
}


std::string LLAppearanceMgr::getAppearanceServiceURL() const
{
	if (gSavedSettings.getString("DebugAvatarAppearanceServiceURLOverride").empty())
	{
		return mAppearanceServiceURL;
	}
	return gSavedSettings.getString("DebugAvatarAppearanceServiceURLOverride");
}

void show_created_outfit(LLUUID& folder_id, bool show_panel = true)
{
	if (!LLApp::isRunning())
	{
		LL_WARNS() << "called during shutdown, skipping" << LL_ENDL;
		return;
	}
	
	LL_DEBUGS("Avatar") << "called" << LL_ENDL;
	LLSD key;
	
	//EXT-7727. For new accounts inventory callback is created during login process
	// and may be processed after login process is finished
	if (show_panel)
	{
		LL_DEBUGS("Avatar") << "showing panel" << LL_ENDL;
		LLFloaterSidePanelContainer::showPanel("appearance", "panel_outfits_inventory", key);
		
	}
	LLOutfitsList *outfits_list =
		dynamic_cast<LLOutfitsList*>(LLFloaterSidePanelContainer::getPanel("appearance", "outfitslist_tab"));
	if (outfits_list)
	{
		outfits_list->setSelectedOutfitByUUID(folder_id);
	}
	
	LLAppearanceMgr::getInstance()->updateIsDirty();
	gAgentWearables.notifyLoadingFinished(); // New outfit is saved.
	LLAppearanceMgr::getInstance()->updatePanelOutfitName("");

	// For SSB, need to update appearance after we add a base outfit
	// link, since, the COF version has changed. There is a race
	// condition in initial outfit setup which can lead to rez
	// failures - SH-3860.
	LL_DEBUGS("Avatar") << "requesting appearance update after createBaseOutfitLink" << LL_ENDL;
	LLPointer<LLInventoryCallback> cb = new LLUpdateAppearanceOnDestroy;
	LLAppearanceMgr::getInstance()->createBaseOutfitLink(folder_id, cb);
}

void LLAppearanceMgr::onOutfitFolderCreated(const LLUUID& folder_id, bool show_panel)
{
	LLPointer<LLInventoryCallback> cb =
		new LLBoostFuncInventoryCallback(no_op_inventory_func,
										 boost::bind(&LLAppearanceMgr::onOutfitFolderCreatedAndClothingOrdered,this,folder_id,show_panel));
	updateClothingOrderingInfo(LLUUID::null, cb);
}

void LLAppearanceMgr::onOutfitFolderCreatedAndClothingOrdered(const LLUUID& folder_id, bool show_panel)
{
	LLPointer<LLInventoryCallback> cb =
		new LLBoostFuncInventoryCallback(no_op_inventory_func,
										 boost::bind(show_created_outfit,folder_id,show_panel));
	bool copy_folder_links = false;
	slamCategoryLinks(getCOF(), folder_id, copy_folder_links, cb);
}

void LLAppearanceMgr::makeNewOutfitLinks(const std::string& new_folder_name, bool show_panel)
{
	if (!isAgentAvatarValid()) return;

	LL_DEBUGS("Avatar") << "creating new outfit" << LL_ENDL;

	gAgentWearables.notifyLoadingStarted();

	// First, make a folder in the My Outfits directory.
	const LLUUID parent_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);
    if (AISAPI::isAvailable())
	{
		// cap-based category creation was buggy until recently. use
		// existence of AIS as an indicator the fix is present. Does
		// not actually use AIS to create the category.
		inventory_func_type func = boost::bind(&LLAppearanceMgr::onOutfitFolderCreated,this,_1,show_panel);
		LLUUID folder_id = gInventory.createNewCategory(
			parent_id,
			LLFolderType::FT_OUTFIT,
			new_folder_name,
			func);
	}
	else
	{		
		LLUUID folder_id = gInventory.createNewCategory(
			parent_id,
			LLFolderType::FT_OUTFIT,
			new_folder_name);
		onOutfitFolderCreated(folder_id, show_panel);
	}
}

void LLAppearanceMgr::wearBaseOutfit()
{
	const LLUUID& base_outfit_id = getBaseOutfitUUID();
	if (base_outfit_id.isNull()) return;
	
	updateCOF(base_outfit_id);
}

void LLAppearanceMgr::removeItemsFromAvatar(const uuid_vec_t& ids_to_remove)
{
	if (ids_to_remove.empty())
	{
		LL_WARNS() << "called with empty list, nothing to do" << LL_ENDL;
		return;
	}
	LLPointer<LLInventoryCallback> cb = new LLUpdateAppearanceOnDestroy;
	for (uuid_vec_t::const_iterator it = ids_to_remove.begin(); it != ids_to_remove.end(); ++it)
	{
		const LLUUID& id_to_remove = *it;
		const LLUUID& linked_item_id = gInventory.getLinkedItemID(id_to_remove);
		LLViewerInventoryItem *item = gInventory.getItem(linked_item_id);
		if (item && item->getType() == LLAssetType::AT_OBJECT)
		{
			LL_DEBUGS("Avatar") << "ATT removing attachment " << item->getName() << " id " << item->getUUID() << LL_ENDL;
		}
		if (item && item->getType() == LLAssetType::AT_BODYPART)
		{
		    continue;
		}
		removeCOFItemLinks(linked_item_id, cb);
		addDoomedTempAttachment(linked_item_id);
	}
}

void LLAppearanceMgr::removeItemFromAvatar(const LLUUID& id_to_remove)
{
	uuid_vec_t ids_to_remove;
	ids_to_remove.push_back(id_to_remove);
	removeItemsFromAvatar(ids_to_remove);
}


// Adds the given item ID to mDoomedTempAttachmentIDs iff it's a temp attachment
void LLAppearanceMgr::addDoomedTempAttachment(const LLUUID& id_to_remove)
{
	LLViewerObject * attachmentp = gAgentAvatarp->findAttachmentByID(id_to_remove);
	if (attachmentp &&
		attachmentp->isTempAttachment())
	{	// If this is a temp attachment and we want to remove it, record the ID 
		// so it will be deleted when attachments are synced up with COF
		mDoomedTempAttachmentIDs.insert(id_to_remove);
		//LL_INFOS() << "Will remove temp attachment id " << id_to_remove << LL_ENDL;
	}
}

// Find AND REMOVES the given UUID from mDoomedTempAttachmentIDs
bool LLAppearanceMgr::shouldRemoveTempAttachment(const LLUUID& item_id)
{
	doomed_temp_attachments_t::iterator iter = mDoomedTempAttachmentIDs.find(item_id);
	if (iter != mDoomedTempAttachmentIDs.end())
	{
		mDoomedTempAttachmentIDs.erase(iter);
		return true;
	}
	return false;
}


bool LLAppearanceMgr::moveWearable(LLViewerInventoryItem* item, bool closer_to_body)
{
	if (!item || !item->isWearableType()) return false;
	if (item->getType() != LLAssetType::AT_CLOTHING) return false;
	if (!gInventory.isObjectDescendentOf(item->getUUID(), getCOF())) return false;

	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLFindWearablesOfType filter_wearables_of_type(item->getWearableType());
	gInventory.collectDescendentsIf(getCOF(), cats, items, true, filter_wearables_of_type);
	if (items.empty()) return false;

	// We assume that the items have valid descriptions.
	std::sort(items.begin(), items.end(), WearablesOrderComparator(item->getWearableType()));

	if (closer_to_body && items.front() == item) return false;
	if (!closer_to_body && items.back() == item) return false;
	
	LLInventoryModel::item_array_t::iterator it = std::find(items.begin(), items.end(), item);
	if (items.end() == it) return false;


	//swapping descriptions
	closer_to_body ? --it : ++it;
	LLViewerInventoryItem* swap_item = *it;
	if (!swap_item) return false;
	std::string tmp = swap_item->getActualDescription();
	swap_item->setDescription(item->getActualDescription());
	item->setDescription(tmp);

	// LL_DEBUGS("Inventory") << "swap, item "
	// 					   << ll_pretty_print_sd(item->asLLSD())
	// 					   << " swap_item "
	// 					   << ll_pretty_print_sd(swap_item->asLLSD()) << LL_ENDL;

	// FIXME switch to use AISv3 where supported.
	//items need to be updated on a dataserver
	item->setComplete(TRUE);
	item->updateServer(FALSE);
	gInventory.updateItem(item);

	swap_item->setComplete(TRUE);
	swap_item->updateServer(FALSE);
	gInventory.updateItem(swap_item);

	//to cause appearance of the agent to be updated
	bool result = false;
	if ((result = gAgentWearables.moveWearable(item, closer_to_body)))
	{
		gAgentAvatarp->wearableUpdated(item->getWearableType());
	}

	setOutfitDirty(true);

	//*TODO do we need to notify observers here in such a way?
	gInventory.notifyObservers();

	return result;
}

//static
void LLAppearanceMgr::sortItemsByActualDescription(LLInventoryModel::item_array_t& items)
{
	if (items.size() < 2) return;

	std::sort(items.begin(), items.end(), sort_by_actual_description);
}

//#define DUMP_CAT_VERBOSE

void LLAppearanceMgr::dumpCat(const LLUUID& cat_id, const std::string& msg)
{
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	gInventory.collectDescendents(cat_id, cats, items, LLInventoryModel::EXCLUDE_TRASH);

#ifdef DUMP_CAT_VERBOSE
	LL_INFOS() << LL_ENDL;
	LL_INFOS() << str << LL_ENDL;
	S32 hitcount = 0;
	for(S32 i=0; i<items.size(); i++)
	{
		LLViewerInventoryItem *item = items.get(i);
		if (item)
			hitcount++;
		LL_INFOS() << i <<" "<< item->getName() <<LL_ENDL;
	}
#endif
	LL_INFOS() << msg << " count " << items.size() << LL_ENDL;
}

void LLAppearanceMgr::dumpItemArray(const LLInventoryModel::item_array_t& items,
									const std::string& msg)
{
	for (S32 i=0; i<items.size(); i++)
	{
		LLViewerInventoryItem *item = items.at(i);
		LLViewerInventoryItem *linked_item = item ? item->getLinkedItem() : NULL;
		LLUUID asset_id;
		if (linked_item)
		{
			asset_id = linked_item->getAssetUUID();
		}
		LL_DEBUGS("Avatar") << self_av_string() << msg << " " << i <<" " << (item ? item->getName() : "(nullitem)") << " " << asset_id.asString() << LL_ENDL;
	}
}

bool LLAppearanceMgr::mActive = true;

LLAppearanceMgr::LLAppearanceMgr():
	mAttachmentInvLinkEnabled(false),
	mOutfitIsDirty(false),
	mOutfitLocked(false),
	mInFlightTimer(),
	mIsInUpdateAppearanceFromCOF(false),
    mOutstandingAppearanceBakeRequest(false),
    mRerequestAppearanceBake(false)
{
	LLOutfitObserver& outfit_observer = LLOutfitObserver::instance();
	// unlock outfit on save operation completed
	outfit_observer.addCOFSavedCallback(boost::bind(
			&LLAppearanceMgr::setOutfitLocked, this, false));

	mUnlockOutfitTimer.reset(new LLOutfitUnLockTimer(gSavedSettings.getS32(
			"OutfitOperationsTimeout")));

	gIdleCallbacks.addFunction(&LLAttachmentsMgr::onIdle, NULL);
	gIdleCallbacks.addFunction(&LLAppearanceMgr::onIdle, NULL); //sheduling appearance update requests
}

LLAppearanceMgr::~LLAppearanceMgr()
{
	mActive = false;
}

void LLAppearanceMgr::setAttachmentInvLinkEnable(bool val)
{
	LL_DEBUGS("Avatar") << "setAttachmentInvLinkEnable => " << (int) val << LL_ENDL;
	mAttachmentInvLinkEnabled = val;
}
boost::signals2::connection LLAppearanceMgr::setAttachmentsChangedCallback(attachments_changed_callback_t cb)
{
	return mAttachmentsChangeSignal.connect(cb);
}

void dumpAttachmentSet(const std::set<LLUUID>& atts, const std::string& msg)
{
       LL_INFOS() << msg << LL_ENDL;
       for (std::set<LLUUID>::const_iterator it = atts.begin();
               it != atts.end();
               ++it)
       {
               LLUUID item_id = *it;
               LLViewerInventoryItem *item = gInventory.getItem(item_id);
               if (item)
                       LL_INFOS() << "atts " << item->getName() << LL_ENDL;
               else
                       LL_INFOS() << "atts " << "UNKNOWN[" << item_id.asString() << "]" << LL_ENDL;
       }
       LL_INFOS() << LL_ENDL;
}

void LLAppearanceMgr::registerAttachment(const LLUUID& item_id)
{
	LLViewerInventoryItem *item = gInventory.getItem(item_id);
	LL_DEBUGS("Avatar") << "ATT registering attachment "
						<< (item ? item->getName() : "UNKNOWN") << " " << item_id << LL_ENDL;
	gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);

	LLAttachmentsMgr::instance().onAttachmentArrived(item_id);

	mAttachmentsChangeSignal();
}

void LLAppearanceMgr::unregisterAttachment(const LLUUID& item_id)
{
	LLViewerInventoryItem *item = gInventory.getItem(item_id);
	LL_DEBUGS("Avatar") << "ATT unregistering attachment "
						<< (item ? item->getName() : "UNKNOWN") << " " << item_id << LL_ENDL;
	gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);

    LLAttachmentsMgr::instance().onDetachCompleted(item_id);
	if (mAttachmentInvLinkEnabled && isLinkedInCOF(item_id))
	{
		LL_DEBUGS("Avatar") << "ATT removing COF link for attachment "
							<< (item ? item->getName() : "UNKNOWN") << " " << item_id << LL_ENDL;
		LLAppearanceMgr::removeCOFItemLinks(item_id);
	}
	else
	{
		//LL_INFOS() << "no link changes, inv link not enabled" << LL_ENDL;
	}

	mAttachmentsChangeSignal();
}

BOOL LLAppearanceMgr::getIsInCOF(const LLUUID& obj_id) const
{
	const LLUUID& cof = getCOF();
	if (obj_id == cof)
		return TRUE;
	const LLInventoryObject* obj = gInventory.getObject(obj_id);
	if (obj && obj->getParentUUID() == cof)
		return TRUE;
	return FALSE;
}

BOOL LLAppearanceMgr::getIsProtectedCOFItem(const LLUUID& obj_id) const
{
	if (!getIsInCOF(obj_id)) return FALSE;

	// If a non-link somehow ended up in COF, allow deletion.
	const LLInventoryObject *obj = gInventory.getObject(obj_id);
	if (obj && !obj->getIsLinkType())
	{
		return FALSE;
	}

	// For now, don't allow direct deletion from the COF.  Instead, force users
	// to choose "Detach" or "Take Off".
	return TRUE;
}

class CallAfterCategoryFetchStage2: public LLInventoryFetchItemsObserver
{
public:
	CallAfterCategoryFetchStage2(const uuid_vec_t& ids,
								 nullary_func_t callable) :
		LLInventoryFetchItemsObserver(ids),
		mCallable(callable)
	{
	}
	~CallAfterCategoryFetchStage2()
	{
	}
	virtual void done()
	{
		LL_INFOS() << this << " done with incomplete " << mIncomplete.size()
				<< " complete " << mComplete.size() <<  " calling callable" << LL_ENDL;

		gInventory.removeObserver(this);
		doOnIdleOneTime(mCallable);
		delete this;
	}
protected:
	nullary_func_t mCallable;
};

class CallAfterCategoryFetchStage1: public LLInventoryFetchDescendentsObserver
{
public:
	CallAfterCategoryFetchStage1(const LLUUID& cat_id, nullary_func_t callable) :
		LLInventoryFetchDescendentsObserver(cat_id),
		mCallable(callable)
	{
	}
	~CallAfterCategoryFetchStage1()
	{
	}
	virtual void done()
	{
		// What we do here is get the complete information on the
		// items in the requested category, and set up an observer
		// that will wait for that to happen.
		LLInventoryModel::cat_array_t cat_array;
		LLInventoryModel::item_array_t item_array;
		gInventory.collectDescendents(mComplete.front(),
									  cat_array,
									  item_array,
									  LLInventoryModel::EXCLUDE_TRASH);
		S32 count = item_array.size();
		if(!count)
		{
			LL_WARNS() << "Nothing fetched in category " << mComplete.front()
					<< LL_ENDL;
			gInventory.removeObserver(this);
			doOnIdleOneTime(mCallable);

			delete this;
			return;
		}

		LL_INFOS() << "stage1 got " << item_array.size() << " items, passing to stage2 " << LL_ENDL;
		uuid_vec_t ids;
		for(S32 i = 0; i < count; ++i)
		{
			ids.push_back(item_array.at(i)->getUUID());
		}
		
		gInventory.removeObserver(this);
		
		// do the fetch
		CallAfterCategoryFetchStage2 *stage2 = new CallAfterCategoryFetchStage2(ids, mCallable);
		stage2->startFetch();
		if(stage2->isFinished())
		{
			// everything is already here - call done.
			stage2->done();
		}
		else
		{
			// it's all on it's way - add an observer, and the inventory
			// will call done for us when everything is here.
			gInventory.addObserver(stage2);
		}
		delete this;
	}
protected:
	nullary_func_t mCallable;
};

void callAfterCategoryFetch(const LLUUID& cat_id, nullary_func_t cb)
{
	CallAfterCategoryFetchStage1 *stage1 = new CallAfterCategoryFetchStage1(cat_id, cb);
	stage1->startFetch();
	if (stage1->isFinished())
	{
		stage1->done();
	}
	else
	{
		gInventory.addObserver(stage1);
	}
}

void add_wearable_type_counts(const uuid_vec_t& ids,
                              S32& clothing_count,
                              S32& bodypart_count,
                              S32& object_count,
                              S32& other_count)
{
    for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
    {
        const LLUUID& item_id_to_wear = *it;
        LLViewerInventoryItem* item_to_wear = gInventory.getItem(item_id_to_wear);
        if (item_to_wear)
        {
            if (item_to_wear->getType() == LLAssetType::AT_CLOTHING)
            {
                clothing_count++;
            }
            else if (item_to_wear->getType() == LLAssetType::AT_BODYPART)
            {
                bodypart_count++;
            }
            else if (item_to_wear->getType() == LLAssetType::AT_OBJECT)
            {
                object_count++;
            }
            else
            {
                other_count++;
            }
        }
        else
        {
            other_count++;
        }
    }
}

void wear_multiple(const uuid_vec_t& ids, bool replace)
{
    S32 clothing_count = 0;
    S32 bodypart_count = 0;
    S32 object_count = 0;
    S32 other_count = 0;
    add_wearable_type_counts(ids, clothing_count, bodypart_count, object_count, other_count);

    LLPointer<LLInventoryCallback> cb = NULL;
    if (clothing_count > 0 || bodypart_count > 0)
    {
        cb = new LLUpdateAppearanceOnDestroy;
    }
	LLAppearanceMgr::instance().wearItemsOnAvatar(ids, true, replace, cb);
}

// SLapp for easy-wearing of a stock (library) avatar
//
class LLWearFolderHandler : public LLCommandHandler
{
public:
	// not allowed from outside the app
	LLWearFolderHandler() : LLCommandHandler("wear_folder", UNTRUSTED_BLOCK) { }

	bool handle(const LLSD& tokens, const LLSD& query_map,
				LLMediaCtrl* web)
	{
		LLSD::UUID folder_uuid;

		if (folder_uuid.isNull() && query_map.has("folder_name"))
		{
			std::string outfit_folder_name = query_map["folder_name"];
			folder_uuid = findDescendentCategoryIDByName(
				gInventory.getLibraryRootFolderID(),
				outfit_folder_name);	
		}
		if (folder_uuid.isNull() && query_map.has("folder_id"))
		{
			folder_uuid = query_map["folder_id"].asUUID();
		}
		
		if (folder_uuid.notNull())
		{
			LLPointer<LLInventoryCategory> category = new LLInventoryCategory(folder_uuid,
																			  LLUUID::null,
																			  LLFolderType::FT_CLOTHING,
																			  "Quick Appearance");
			if ( gInventory.getCategory( folder_uuid ) != NULL )
			{
				LLAppearanceMgr::getInstance()->wearInventoryCategory(category, true, false);
				
				// *TODOw: This may not be necessary if initial outfit is chosen already -- josh
				gAgent.setOutfitChosen(TRUE);
			}
		}

		// release avatar picker keyboard focus
		gFocusMgr.setKeyboardFocus( NULL );

		return true;
	}
};

LLWearFolderHandler gWearFolderHandler;
