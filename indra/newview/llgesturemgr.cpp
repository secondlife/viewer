/** 
 * @file llgesturemgr.cpp
 * @brief Manager for playing gestures on the viewer
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

#include "llgesturemgr.h"

// system
#include <functional>
#include <algorithm>

// library
#include "llaudioengine.h"
#include "lldatapacker.h"
#include "llinventory.h"
#include "llkeyframemotion.h"
#include "llmultigesture.h"
#include "llnotificationsutil.h"
#include "llstl.h"
#include "llstring.h"	// todo: remove
#include "llvfile.h"
#include "message.h"

// newview
#include "llagent.h"
#include "lldelayedgestureerror.h"
#include "llinventorymodel.h"
#include "llviewermessage.h"
#include "llvoavatarself.h"
#include "llviewerstats.h"
#include "llnearbychatbar.h"
#include "llappearancemgr.h"
#include "llgesturelistener.h"

// Longest time, in seconds, to wait for all animations to stop playing
const F32 MAX_WAIT_ANIM_SECS = 30.f;

// If this gesture is a link, get the base gesture that this link points to,
// otherwise just return this id.
static const LLUUID& get_linked_uuid(const LLUUID& item_id);

// Lightweight constructor.
// init() does the heavy lifting.
LLGestureMgr::LLGestureMgr()
:	mValid(FALSE),
	mPlaying(),
	mActive(),
	mLoadingCount(0)
{
	gInventory.addObserver(this);
	mListener.reset(new LLGestureListener());
}


// We own the data for gestures, so clean them up.
LLGestureMgr::~LLGestureMgr()
{
	item_map_t::iterator it;
	for (it = mActive.begin(); it != mActive.end(); ++it)
	{
		LLMultiGesture* gesture = (*it).second;

		delete gesture;
		gesture = NULL;
	}
	gInventory.removeObserver(this);
}


void LLGestureMgr::init()
{
	// TODO
}

void LLGestureMgr::changed(U32 mask) 
{ 
	LLInventoryFetchItemsObserver::changed(mask);

	if (mask & LLInventoryObserver::GESTURE)
	{
		// If there was a gesture label changed, update all the names in the 
		// active gestures and then notify observers
		if (mask & LLInventoryObserver::LABEL)
		{
			for(item_map_t::iterator it = mActive.begin(); it != mActive.end(); ++it)
			{
				if(it->second)
				{
					LLViewerInventoryItem* item = gInventory.getItem(it->first);
					if(item)
					{
						it->second->mName = item->getName();
					}
				}
			}
			notifyObservers();
		}
		// If there was a gesture added or removed notify observers
		// STRUCTURE denotes that the inventory item has been moved
		// In the case of deleting gesture, it is moved to the trash
		else if(mask & LLInventoryObserver::ADD ||
				mask & LLInventoryObserver::REMOVE ||
				mask & LLInventoryObserver::STRUCTURE)
		{
			notifyObservers();
		}
	}
}


// Use this version when you have the item_id but not the asset_id,
// and you KNOW the inventory is loaded.
void LLGestureMgr::activateGesture(const LLUUID& item_id)
{
	LLViewerInventoryItem* item = gInventory.getItem(item_id);
	if (!item) return;
	if (item->getType() != LLAssetType::AT_GESTURE)
		return;

	LLUUID asset_id = item->getAssetUUID();

	mLoadingCount = 1;
	mDeactivateSimilarNames.clear();

	const BOOL inform_server = TRUE;
	const BOOL deactivate_similar = FALSE; 
	activateGestureWithAsset(item_id, asset_id, inform_server, deactivate_similar);
}


void LLGestureMgr::activateGestures(LLViewerInventoryItem::item_array_t& items)
{
	// Load up the assets
	S32 count = 0;
	LLViewerInventoryItem::item_array_t::const_iterator it;
	for (it = items.begin(); it != items.end(); ++it)
	{
		LLViewerInventoryItem* item = *it;

		if (isGestureActive(item->getUUID()))
		{
			continue;
		}
		else 
		{ // Make gesture active and persistent through login sessions.  -spatters 07-12-06
			activateGesture(item->getUUID());
		}

		count++;
	}

	mLoadingCount = count;
	mDeactivateSimilarNames.clear();

	for (it = items.begin(); it != items.end(); ++it)
	{
		LLViewerInventoryItem* item = *it;

		if (isGestureActive(item->getUUID()))
		{
			continue;
		}

		// Don't inform server, we'll do that in bulk
		const BOOL no_inform_server = FALSE;
		const BOOL deactivate_similar = TRUE;
		activateGestureWithAsset(item->getUUID(), item->getAssetUUID(),
								 no_inform_server,
								 deactivate_similar);
	}

	// Inform the database of this change
	LLMessageSystem* msg = gMessageSystem;

	BOOL start_message = TRUE;

	for (it = items.begin(); it != items.end(); ++it)
	{
		LLViewerInventoryItem* item = *it;

		if (isGestureActive(item->getUUID()))
		{
			continue;
		}

		if (start_message)
		{
			msg->newMessage("ActivateGestures");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgent.getID());
			msg->addUUID("SessionID", gAgent.getSessionID());
			msg->addU32("Flags", 0x0);
			start_message = FALSE;
		}
		
		msg->nextBlock("Data");
		msg->addUUID("ItemID", item->getUUID());
		msg->addUUID("AssetID", item->getAssetUUID());
		msg->addU32("GestureFlags", 0x0);

		if (msg->getCurrentSendTotal() > MTUBYTES)
		{
			gAgent.sendReliableMessage();
			start_message = TRUE;
		}
	}

	if (!start_message)
	{
		gAgent.sendReliableMessage();
	}
}


struct LLLoadInfo
{
	LLUUID mItemID;
	BOOL mInformServer;
	BOOL mDeactivateSimilar;
};

// If inform_server is true, will send a message upstream to update
// the user_gesture_active table.
/**
 * It will load a gesture from remote storage
 */
void LLGestureMgr::activateGestureWithAsset(const LLUUID& item_id,
												const LLUUID& asset_id,
												BOOL inform_server,
												BOOL deactivate_similar)
{
	const LLUUID& base_item_id = get_linked_uuid(item_id);

	if( !gAssetStorage )
	{
		llwarns << "LLGestureMgr::activateGestureWithAsset without valid gAssetStorage" << llendl;
		return;
	}
	// If gesture is already active, nothing to do.
	if (isGestureActive(item_id))
	{
		llwarns << "Tried to loadGesture twice " << item_id << llendl;
		return;
	}

//	if (asset_id.isNull())
//	{
//		llwarns << "loadGesture() - gesture has no asset" << llendl;
//		return;
//	}

	// For now, put NULL into the item map.  We'll build a gesture
	// class object when the asset data arrives.
	mActive[base_item_id] = NULL;

	// Copy the UUID
	if (asset_id.notNull())
	{
		LLLoadInfo* info = new LLLoadInfo;
		info->mItemID = base_item_id;
		info->mInformServer = inform_server;
		info->mDeactivateSimilar = deactivate_similar;

		const BOOL high_priority = TRUE;
		gAssetStorage->getAssetData(asset_id,
									LLAssetType::AT_GESTURE,
									onLoadComplete,
									(void*)info,
									high_priority);
	}
	else
	{
		notifyObservers();
	}
}


void LLGestureMgr::deactivateGesture(const LLUUID& item_id)
{
	const LLUUID& base_item_id = get_linked_uuid(item_id);
	item_map_t::iterator it = mActive.find(base_item_id);
	if (it == mActive.end())
	{
		llwarns << "deactivateGesture for inactive gesture " << item_id << llendl;
		return;
	}

	// mActive owns this gesture pointer, so clean up memory.
	LLMultiGesture* gesture = (*it).second;

	// Can be NULL gestures in the map
	if (gesture)
	{
		stopGesture(gesture);

		delete gesture;
		gesture = NULL;
	}

	mActive.erase(it);
	gInventory.addChangedMask(LLInventoryObserver::LABEL, base_item_id);

	// Inform the database of this change
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("DeactivateGestures");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->addU32("Flags", 0x0);
	
	msg->nextBlock("Data");
	msg->addUUID("ItemID", item_id);
	msg->addU32("GestureFlags", 0x0);

	gAgent.sendReliableMessage();

	LLAppearanceMgr::instance().removeCOFItemLinks(base_item_id);

	notifyObservers();
}


void LLGestureMgr::deactivateSimilarGestures(LLMultiGesture* in, const LLUUID& in_item_id)
{
	const LLUUID& base_in_item_id = get_linked_uuid(in_item_id);
	uuid_vec_t gest_item_ids;

	// Deactivate all gestures that match
	item_map_t::iterator it;
	for (it = mActive.begin(); it != mActive.end(); )
	{
		const LLUUID& item_id = (*it).first;
		LLMultiGesture* gest = (*it).second;

		// Don't deactivate the gesture we are looking for duplicates of
		// (for replaceGesture)
		if (!gest || item_id == base_in_item_id) 
		{
			// legal, can have null pointers in list
			++it;
		}
		else if ((!gest->mTrigger.empty() && gest->mTrigger == in->mTrigger)
				 || (gest->mKey != KEY_NONE && gest->mKey == in->mKey && gest->mMask == in->mMask))
		{
			gest_item_ids.push_back(item_id);

			stopGesture(gest);

			delete gest;
			gest = NULL;

			mActive.erase(it++);
			gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);

		}
		else
		{
			++it;
		}
	}

	// Inform database of the change
	LLMessageSystem* msg = gMessageSystem;
	BOOL start_message = TRUE;
	uuid_vec_t::const_iterator vit = gest_item_ids.begin();
	while (vit != gest_item_ids.end())
	{
		if (start_message)
		{
			msg->newMessage("DeactivateGestures");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgent.getID());
			msg->addUUID("SessionID", gAgent.getSessionID());
			msg->addU32("Flags", 0x0);
			start_message = FALSE;
		}
	
		msg->nextBlock("Data");
		msg->addUUID("ItemID", *vit);
		msg->addU32("GestureFlags", 0x0);

		if (msg->getCurrentSendTotal() > MTUBYTES)
		{
			gAgent.sendReliableMessage();
			start_message = TRUE;
		}

		++vit;
	}

	if (!start_message)
	{
		gAgent.sendReliableMessage();
	}

	// Add to the list of names for the user.
	for (vit = gest_item_ids.begin(); vit != gest_item_ids.end(); ++vit)
	{
		LLViewerInventoryItem* item = gInventory.getItem(*vit);
		if (!item) continue;

		mDeactivateSimilarNames.append(item->getName());
		mDeactivateSimilarNames.append("\n");
	}

	notifyObservers();
}


BOOL LLGestureMgr::isGestureActive(const LLUUID& item_id)
{
	const LLUUID& base_item_id = get_linked_uuid(item_id);
	item_map_t::iterator it = mActive.find(base_item_id);
	return (it != mActive.end());
}


BOOL LLGestureMgr::isGesturePlaying(const LLUUID& item_id)
{
	const LLUUID& base_item_id = get_linked_uuid(item_id);

	item_map_t::iterator it = mActive.find(base_item_id);
	if (it == mActive.end()) return FALSE;

	LLMultiGesture* gesture = (*it).second;
	if (!gesture) return FALSE;

	return gesture->mPlaying;
}

BOOL LLGestureMgr::isGesturePlaying(LLMultiGesture* gesture)
{
	if(!gesture)
	{
		return FALSE;
	}

	return gesture->mPlaying;
}

void LLGestureMgr::replaceGesture(const LLUUID& item_id, LLMultiGesture* new_gesture, const LLUUID& asset_id)
{
	const LLUUID& base_item_id = get_linked_uuid(item_id);

	item_map_t::iterator it = mActive.find(base_item_id);
	if (it == mActive.end())
	{
		llwarns << "replaceGesture for inactive gesture " << base_item_id << llendl;
		return;
	}

	LLMultiGesture* old_gesture = (*it).second;
	stopGesture(old_gesture);

	mActive.erase(base_item_id);

	mActive[base_item_id] = new_gesture;

	delete old_gesture;
	old_gesture = NULL;

	if (asset_id.notNull())
	{
		mLoadingCount = 1;
		mDeactivateSimilarNames.clear();

		LLLoadInfo* info = new LLLoadInfo;
		info->mItemID = base_item_id;
		info->mInformServer = TRUE;
		info->mDeactivateSimilar = FALSE;

		const BOOL high_priority = TRUE;
		gAssetStorage->getAssetData(asset_id,
									LLAssetType::AT_GESTURE,
									onLoadComplete,
									(void*)info,
									high_priority);
	}

	notifyObservers();
}

void LLGestureMgr::replaceGesture(const LLUUID& item_id, const LLUUID& new_asset_id)
{
	const LLUUID& base_item_id = get_linked_uuid(item_id);

	item_map_t::iterator it = LLGestureMgr::instance().mActive.find(base_item_id);
	if (it == mActive.end())
	{
		llwarns << "replaceGesture for inactive gesture " << base_item_id << llendl;
		return;
	}

	// mActive owns this gesture pointer, so clean up memory.
	LLMultiGesture* gesture = (*it).second;
	LLGestureMgr::instance().replaceGesture(base_item_id, gesture, new_asset_id);
}

void LLGestureMgr::playGesture(LLMultiGesture* gesture)
{
	if (!gesture) return;

	// Reset gesture to first step
	gesture->mCurrentStep = 0;

	// Add to list of playing
	gesture->mPlaying = TRUE;
	mPlaying.push_back(gesture);

	// Load all needed assets to minimize the delays
	// when gesture is playing.
	for (std::vector<LLGestureStep*>::iterator steps_it = gesture->mSteps.begin();
		 steps_it != gesture->mSteps.end();
		 ++steps_it)
	{
		LLGestureStep* step = *steps_it;
		switch(step->getType())
		{
		case STEP_ANIMATION:
			{
				LLGestureStepAnimation* anim_step = (LLGestureStepAnimation*)step;
				const LLUUID& anim_id = anim_step->mAnimAssetID;

				// Don't request the animation if this step stops it or if it is already in Static VFS
				if (!(anim_id.isNull()
					  || anim_step->mFlags & ANIM_FLAG_STOP
					  || gAssetStorage->hasLocalAsset(anim_id, LLAssetType::AT_ANIMATION)))
				{
					mLoadingAssets.insert(anim_id);

					LLUUID* id = new LLUUID(gAgentID);
					gAssetStorage->getAssetData(anim_id,
									LLAssetType::AT_ANIMATION,
									onAssetLoadComplete,
									(void *)id,
									TRUE);
				}
				break;
			}
		case STEP_SOUND:
			{
				LLGestureStepSound* sound_step = (LLGestureStepSound*)step;
				const LLUUID& sound_id = sound_step->mSoundAssetID;
				if (!(sound_id.isNull()
					  || gAssetStorage->hasLocalAsset(sound_id, LLAssetType::AT_SOUND)))
				{
					mLoadingAssets.insert(sound_id);

					gAssetStorage->getAssetData(sound_id,
									LLAssetType::AT_SOUND,
									onAssetLoadComplete,
									NULL,
									TRUE);
				}
				break;
			}
		case STEP_CHAT:
		case STEP_WAIT:
		case STEP_EOF:
			{
				break;
			}
		default:
			{
				llwarns << "Unknown gesture step type: " << step->getType() << llendl;
			}
		}
	}

	// And get it going
	stepGesture(gesture);

	notifyObservers();
}


// Convenience function that looks up the item_id for you.
void LLGestureMgr::playGesture(const LLUUID& item_id)
{
	const LLUUID& base_item_id = get_linked_uuid(item_id);

	item_map_t::iterator it = mActive.find(base_item_id);
	if (it == mActive.end()) return;

	LLMultiGesture* gesture = (*it).second;
	if (!gesture) return;

	playGesture(gesture);
}


// Iterates through space delimited tokens in string, triggering any gestures found.
// Generates a revised string that has the found tokens replaced by their replacement strings
// and (as a minor side effect) has multiple spaces in a row replaced by single spaces.
BOOL LLGestureMgr::triggerAndReviseString(const std::string &utf8str, std::string* revised_string)
{
	std::string tokenized = utf8str;

	BOOL found_gestures = FALSE;
	BOOL first_token = TRUE;

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" ");
	tokenizer tokens(tokenized, sep);
	tokenizer::iterator token_iter;

	for( token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
	{
		const char* cur_token = token_iter->c_str();
		LLMultiGesture* gesture = NULL;

		// Only pay attention to the first gesture in the string.
		if( !found_gestures )
		{
			// collect gestures that match
			std::vector <LLMultiGesture *> matching;
			item_map_t::iterator it;
			for (it = mActive.begin(); it != mActive.end(); ++it)
			{
				gesture = (*it).second;

				// Gesture asset data might not have arrived yet
				if (!gesture) continue;
				
				if (LLStringUtil::compareInsensitive(gesture->mTrigger, cur_token) == 0)
				{
					matching.push_back(gesture);
				}
				
				gesture = NULL;
			}

			
			if (matching.size() > 0)
			{
				// choose one at random
				{
					S32 random = ll_rand(matching.size());

					gesture = matching[random];
					
					playGesture(gesture);

					if (!gesture->mReplaceText.empty())
					{
						if( !first_token )
						{
							if (revised_string)
								revised_string->append( " " );
						}

						// Don't muck with the user's capitalization if we don't have to.
						if( LLStringUtil::compareInsensitive(cur_token, gesture->mReplaceText) == 0)
						{
							if (revised_string)
								revised_string->append( cur_token );
						}
						else
						{
							if (revised_string)
								revised_string->append( gesture->mReplaceText );
						}
					}
					found_gestures = TRUE;
				}
			}
		}
		
		if(!gesture)
		{
			// This token doesn't match a gesture.  Pass it through to the output.
			if( !first_token )
			{
				if (revised_string)
					revised_string->append( " " );
			}
			if (revised_string)
				revised_string->append( cur_token );
		}

		first_token = FALSE;
		gesture = NULL;
	}
	return found_gestures;
}


BOOL LLGestureMgr::triggerGesture(KEY key, MASK mask)
{
	std::vector <LLMultiGesture *> matching;
	item_map_t::iterator it;

	// collect matching gestures
	for (it = mActive.begin(); it != mActive.end(); ++it)
	{
		LLMultiGesture* gesture = (*it).second;

		// asset data might not have arrived yet
		if (!gesture) continue;

		if (gesture->mKey == key
			&& gesture->mMask == mask)
		{
			matching.push_back(gesture);
		}
	}

	// choose one and play it
	if (matching.size() > 0)
	{
		U32 random = ll_rand(matching.size());
		
		LLMultiGesture* gesture = matching[random];
			
		playGesture(gesture);
		return TRUE;
	}
	return FALSE;
}


S32 LLGestureMgr::getPlayingCount() const
{
	return mPlaying.size();
}


struct IsGesturePlaying : public std::unary_function<LLMultiGesture*, bool>
{
	bool operator()(const LLMultiGesture* gesture) const
	{
		return gesture->mPlaying ? true : false;
	}
};

void LLGestureMgr::update()
{
	S32 i;
	for (i = 0; i < (S32)mPlaying.size(); ++i)
	{
		stepGesture(mPlaying[i]);
	}

	// Clear out gestures that are done, by moving all the
	// ones that are still playing to the front.
	std::vector<LLMultiGesture*>::iterator new_end;
	new_end = std::partition(mPlaying.begin(),
							 mPlaying.end(),
							 IsGesturePlaying());

	// Something finished playing
	if (new_end != mPlaying.end())
	{
		// Delete the completed gestures that want deletion
		std::vector<LLMultiGesture*>::iterator it;
		for (it = new_end; it != mPlaying.end(); ++it)
		{
			LLMultiGesture* gesture = *it;

			if (gesture->mDoneCallback)
			{
				gesture->mDoneCallback(gesture, gesture->mCallbackData);

				// callback might have deleted gesture, can't
				// rely on this pointer any more
				gesture = NULL;
			}
		}

		// And take done gestures out of the playing list
		mPlaying.erase(new_end, mPlaying.end());

		notifyObservers();
	}
}


// Run all steps until you're either done or hit a wait.
void LLGestureMgr::stepGesture(LLMultiGesture* gesture)
{
	if (!gesture)
	{
		return;
	}
	if (!isAgentAvatarValid() || hasLoadingAssets(gesture)) return;

	// Of the ones that started playing, have any stopped?

	std::set<LLUUID>::iterator gest_it;
	for (gest_it = gesture->mPlayingAnimIDs.begin(); 
		 gest_it != gesture->mPlayingAnimIDs.end(); 
		 )
	{
		// look in signaled animations (simulator's view of what is
		// currently playing.
		LLVOAvatar::AnimIterator play_it = gAgentAvatarp->mSignaledAnimations.find(*gest_it);
		if (play_it != gAgentAvatarp->mSignaledAnimations.end())
		{
			++gest_it;
		}
		else
		{
			// not found, so not currently playing or scheduled to play
			// delete from the triggered set
			gesture->mPlayingAnimIDs.erase(gest_it++);
		}
	}

	// Of all the animations that we asked the sim to start for us,
	// pick up the ones that have actually started.
	for (gest_it = gesture->mRequestedAnimIDs.begin();
		 gest_it != gesture->mRequestedAnimIDs.end();
		 )
	{
	 LLVOAvatar::AnimIterator play_it = gAgentAvatarp->mSignaledAnimations.find(*gest_it);
		if (play_it != gAgentAvatarp->mSignaledAnimations.end())
		{
			// Hooray, this animation has started playing!
			// Copy into playing.
			gesture->mPlayingAnimIDs.insert(*gest_it);
			gesture->mRequestedAnimIDs.erase(gest_it++);
		}
		else
		{
			// nope, not playing yet
			++gest_it;
		}
	}

	// Run the current steps
	BOOL waiting = FALSE;
	while (!waiting && gesture->mPlaying)
	{
		// Get the current step, if there is one.
		// Otherwise enter the waiting at end state.
		LLGestureStep* step = NULL;
		if (gesture->mCurrentStep < (S32)gesture->mSteps.size())
		{
			step = gesture->mSteps[gesture->mCurrentStep];
			llassert(step != NULL);
		}
		else
		{
			// step stays null, we're off the end
			gesture->mWaitingAtEnd = TRUE;
		}


		// If we're waiting at the end, wait for all gestures to stop
		// playing.
		// TODO: Wait for all sounds to complete as well.
		if (gesture->mWaitingAtEnd)
		{
			// Neither do we have any pending requests, nor are they
			// still playing.
			if ((gesture->mRequestedAnimIDs.empty()
				&& gesture->mPlayingAnimIDs.empty()))
			{
				// all animations are done playing
				gesture->mWaitingAtEnd = FALSE;
				gesture->mPlaying = FALSE;
			}
			else
			{
				waiting = TRUE;
			}
			continue;
		}

		// If we're waiting on our animations to stop, poll for
		// completion.
		if (gesture->mWaitingAnimations)
		{
			// Neither do we have any pending requests, nor are they
			// still playing.
			if ((gesture->mRequestedAnimIDs.empty()
				&& gesture->mPlayingAnimIDs.empty()))
			{
				// all animations are done playing
				gesture->mWaitingAnimations = FALSE;
				gesture->mCurrentStep++;
			}
			else if (gesture->mWaitTimer.getElapsedTimeF32() > MAX_WAIT_ANIM_SECS)
			{
				// we've waited too long for an animation
				llinfos << "Waited too long for animations to stop, continuing gesture."
					<< llendl;
				gesture->mWaitingAnimations = FALSE;
				gesture->mCurrentStep++;
			}
			else
			{
				waiting = TRUE;
			}
			continue;
		}

		// If we're waiting a fixed amount of time, check for timer
		// expiration.
		if (gesture->mWaitingTimer)
		{
			// We're waiting for a certain amount of time to pass
			LLGestureStepWait* wait_step = (LLGestureStepWait*)step;

			F32 elapsed = gesture->mWaitTimer.getElapsedTimeF32();
			if (elapsed > wait_step->mWaitSeconds)
			{
				// wait is done, continue execution
				gesture->mWaitingTimer = FALSE;
				gesture->mCurrentStep++;
			}
			else
			{
				// we're waiting, so execution is done for now
				waiting = TRUE;
			}
			continue;
		}

		// Not waiting, do normal execution
		runStep(gesture, step);
	}
}


void LLGestureMgr::runStep(LLMultiGesture* gesture, LLGestureStep* step)
{
	switch(step->getType())
	{
	case STEP_ANIMATION:
		{
			LLGestureStepAnimation* anim_step = (LLGestureStepAnimation*)step;
			if (anim_step->mAnimAssetID.isNull())
			{
				gesture->mCurrentStep++;
			}

			if (anim_step->mFlags & ANIM_FLAG_STOP)
			{
				gAgent.sendAnimationRequest(anim_step->mAnimAssetID, ANIM_REQUEST_STOP);
				// remove it from our request set in case we just requested it
				std::set<LLUUID>::iterator set_it = gesture->mRequestedAnimIDs.find(anim_step->mAnimAssetID);
				if (set_it != gesture->mRequestedAnimIDs.end())
				{
					gesture->mRequestedAnimIDs.erase(set_it);
				}
			}
			else
			{
				gAgent.sendAnimationRequest(anim_step->mAnimAssetID, ANIM_REQUEST_START);
				// Indicate that we've requested this animation to play as
				// part of this gesture (but it won't start playing for at
				// least one round-trip to simulator).
				gesture->mRequestedAnimIDs.insert(anim_step->mAnimAssetID);
			}
			gesture->mCurrentStep++;
			break;
		}
	case STEP_SOUND:
		{
			LLGestureStepSound* sound_step = (LLGestureStepSound*)step;
			const LLUUID& sound_id = sound_step->mSoundAssetID;
			const F32 volume = 1.f;
			send_sound_trigger(sound_id, volume);
			gesture->mCurrentStep++;
			break;
		}
	case STEP_CHAT:
		{
			LLGestureStepChat* chat_step = (LLGestureStepChat*)step;
			std::string chat_text = chat_step->mChatText;
			// Don't animate the nodding, as this might not blend with
			// other playing animations.

			const BOOL animate = FALSE;

			LLNearbyChatBar::getInstance()->sendChatFromViewer(chat_text, CHAT_TYPE_NORMAL, animate);

			gesture->mCurrentStep++;
			break;
		}
	case STEP_WAIT:
		{
			LLGestureStepWait* wait_step = (LLGestureStepWait*)step;
			if (wait_step->mFlags & WAIT_FLAG_TIME)
			{
				gesture->mWaitingTimer = TRUE;
				gesture->mWaitTimer.reset();
			}
			else if (wait_step->mFlags & WAIT_FLAG_ALL_ANIM)
			{
				gesture->mWaitingAnimations = TRUE;
				// Use the wait timer as a deadlock breaker for animation
				// waits.
				gesture->mWaitTimer.reset();
			}
			else
			{
				gesture->mCurrentStep++;
			}
			// Don't increment instruction pointer until wait is complete.
			break;
		}
	default:
		{
			break;
		}
	}
}


// static
void LLGestureMgr::onLoadComplete(LLVFS *vfs,
									   const LLUUID& asset_uuid,
									   LLAssetType::EType type,
									   void* user_data, S32 status, LLExtStat ext_status)
{
	LLLoadInfo* info = (LLLoadInfo*)user_data;

	LLUUID item_id = info->mItemID;
	BOOL inform_server = info->mInformServer;
	BOOL deactivate_similar = info->mDeactivateSimilar;

	delete info;
	info = NULL;
	LLGestureMgr& self = LLGestureMgr::instance();
	self.mLoadingCount--;

	if (0 == status)
	{
		LLVFile file(vfs, asset_uuid, type, LLVFile::READ);
		S32 size = file.getSize();

		std::vector<char> buffer(size+1);

		file.read((U8*)&buffer[0], size);
		// ensure there's a trailing NULL so strlen will work.
		buffer[size] = '\0';

		LLMultiGesture* gesture = new LLMultiGesture();

		LLDataPackerAsciiBuffer dp(&buffer[0], size+1);
		BOOL ok = gesture->deserialize(dp);

		if (ok)
		{
			if (deactivate_similar)
			{
				self.deactivateSimilarGestures(gesture, item_id);

				// Display deactivation message if this was the last of the bunch.
				if (self.mLoadingCount == 0
					&& self.mDeactivateSimilarNames.length() > 0)
				{
					// we're done with this set of deactivations
					LLSD args;
					args["NAMES"] = self.mDeactivateSimilarNames;
					LLNotificationsUtil::add("DeactivatedGesturesTrigger", args);
				}
			}

			LLViewerInventoryItem* item = gInventory.getItem(item_id);
			if(item)
			{
				gesture->mName = item->getName();
			}
			else
			{
				// Watch this item and set gesture name when item exists in inventory
				self.setFetchID(item_id);
				self.startFetch();
			}
			self.mActive[item_id] = gesture;

			// Everything has been successful.  Add to the active list.
			gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);

			if (inform_server)
			{
				// Inform the database of this change
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessage("ActivateGestures");
				msg->nextBlock("AgentData");
				msg->addUUID("AgentID", gAgent.getID());
				msg->addUUID("SessionID", gAgent.getSessionID());
				msg->addU32("Flags", 0x0);
				
				msg->nextBlock("Data");
				msg->addUUID("ItemID", item_id);
				msg->addUUID("AssetID", asset_uuid);
				msg->addU32("GestureFlags", 0x0);

				gAgent.sendReliableMessage();
			}
			callback_map_t::iterator i_cb = self.mCallbackMap.find(item_id);
			
			if(i_cb != self.mCallbackMap.end())
			{
				i_cb->second(gesture);
				self.mCallbackMap.erase(i_cb);
			}

			self.notifyObservers();
		}
		else
		{
			llwarns << "Unable to load gesture" << llendl;

			self.mActive.erase(item_id);
			
			delete gesture;
			gesture = NULL;
		}
	}
	else
	{
		LLViewerStats::getInstance()->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );

		if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
			LL_ERR_FILE_EMPTY == status)
		{
			LLDelayedGestureError::gestureMissing( item_id );
		}
		else
		{
			LLDelayedGestureError::gestureFailedToLoad( item_id );
		}

		llwarns << "Problem loading gesture: " << status << llendl;
		
		LLGestureMgr::instance().mActive.erase(item_id);			
	}
}

// static
void LLGestureMgr::onAssetLoadComplete(LLVFS *vfs,
									   const LLUUID& asset_uuid,
									   LLAssetType::EType type,
									   void* user_data, S32 status, LLExtStat ext_status)
{
	LLGestureMgr& self = LLGestureMgr::instance();

	// Complete the asset loading process depending on the type and
	// remove the asset id from pending downloads list.
	switch(type)
	{
	case LLAssetType::AT_ANIMATION:
		{
			LLKeyframeMotion::onLoadComplete(vfs, asset_uuid, type, user_data, status, ext_status);

			self.mLoadingAssets.erase(asset_uuid);

			break;
		}
	case LLAssetType::AT_SOUND:
		{
			LLAudioEngine::assetCallback(vfs, asset_uuid, type, user_data, status, ext_status);

			self.mLoadingAssets.erase(asset_uuid);

			break;
		}
	default:
		{
			llwarns << "Unexpected asset type: " << type << llendl;

			// We don't want to return from this callback without
			// an animation or sound callback being fired
			// and *user_data handled to avoid memory leaks.
			llassert(type == LLAssetType::AT_ANIMATION || type == LLAssetType::AT_SOUND);
		}
	}
}

// static
bool LLGestureMgr::hasLoadingAssets(LLMultiGesture* gesture)
{
	LLGestureMgr& self = LLGestureMgr::instance();

	for (std::vector<LLGestureStep*>::iterator steps_it = gesture->mSteps.begin();
		 steps_it != gesture->mSteps.end();
		 ++steps_it)
	{
		LLGestureStep* step = *steps_it;
		switch(step->getType())
		{
		case STEP_ANIMATION:
			{
				LLGestureStepAnimation* anim_step = (LLGestureStepAnimation*)step;
				const LLUUID& anim_id = anim_step->mAnimAssetID;

				if (!(anim_id.isNull()
					  || anim_step->mFlags & ANIM_FLAG_STOP
					  || self.mLoadingAssets.find(anim_id) == self.mLoadingAssets.end()))
				{
					return true;
				}
				break;
			}
		case STEP_SOUND:
			{
				LLGestureStepSound* sound_step = (LLGestureStepSound*)step;
				const LLUUID& sound_id = sound_step->mSoundAssetID;

				if (!(sound_id.isNull()
					  || self.mLoadingAssets.find(sound_id) == self.mLoadingAssets.end()))
				{
					return true;
				}
				break;
			}
		case STEP_CHAT:
		case STEP_WAIT:
		case STEP_EOF:
			{
				break;
			}
		default:
			{
				llwarns << "Unknown gesture step type: " << step->getType() << llendl;
			}
		}
	}

	return false;
}

void LLGestureMgr::stopGesture(LLMultiGesture* gesture)
{
	if (!gesture) return;

	// Stop any animations that this gesture is currently playing
	std::set<LLUUID>::const_iterator set_it;
	for (set_it = gesture->mRequestedAnimIDs.begin(); set_it != gesture->mRequestedAnimIDs.end(); ++set_it)
	{
		const LLUUID& anim_id = *set_it;
		gAgent.sendAnimationRequest(anim_id, ANIM_REQUEST_STOP);
	}
	for (set_it = gesture->mPlayingAnimIDs.begin(); set_it != gesture->mPlayingAnimIDs.end(); ++set_it)
	{
		const LLUUID& anim_id = *set_it;
		gAgent.sendAnimationRequest(anim_id, ANIM_REQUEST_STOP);
	}

	std::vector<LLMultiGesture*>::iterator it;
	it = std::find(mPlaying.begin(), mPlaying.end(), gesture);
	while (it != mPlaying.end())
	{
		mPlaying.erase(it);
		it = std::find(mPlaying.begin(), mPlaying.end(), gesture);
	}

	gesture->reset();

	if (gesture->mDoneCallback)
	{
		gesture->mDoneCallback(gesture, gesture->mCallbackData);

		// callback might have deleted gesture, can't
		// rely on this pointer any more
		gesture = NULL;
	}

	notifyObservers();
}


void LLGestureMgr::stopGesture(const LLUUID& item_id)
{
	const LLUUID& base_item_id = get_linked_uuid(item_id);

	item_map_t::iterator it = mActive.find(base_item_id);
	if (it == mActive.end()) return;

	LLMultiGesture* gesture = (*it).second;
	if (!gesture) return;

	stopGesture(gesture);
}


void LLGestureMgr::addObserver(LLGestureManagerObserver* observer)
{
	mObservers.push_back(observer);
}

void LLGestureMgr::removeObserver(LLGestureManagerObserver* observer)
{
	std::vector<LLGestureManagerObserver*>::iterator it;
	it = std::find(mObservers.begin(), mObservers.end(), observer);
	if (it != mObservers.end())
	{
		mObservers.erase(it);
	}
}

// Call this method when it's time to update everyone on a new state.
// Copy the list because an observer could respond by removing itself
// from the list.
void LLGestureMgr::notifyObservers()
{
	lldebugs << "LLGestureMgr::notifyObservers" << llendl;

	for(std::vector<LLGestureManagerObserver*>::iterator iter = mObservers.begin(); 
		iter != mObservers.end(); 
		++iter)
	{
		LLGestureManagerObserver* observer = (*iter);
		observer->changed();
	}
}

BOOL LLGestureMgr::matchPrefix(const std::string& in_str, std::string* out_str)
{
	S32 in_len = in_str.length();

	item_map_t::iterator it;
	for (it = mActive.begin(); it != mActive.end(); ++it)
	{
		LLMultiGesture* gesture = (*it).second;
		if (gesture)
		{
			const std::string& trigger = gesture->getTrigger();
			
			if (in_len > (S32)trigger.length())
			{
				// too short, bail out
				continue;
			}

			std::string trigger_trunc = trigger;
			LLStringUtil::truncate(trigger_trunc, in_len);
			if (!LLStringUtil::compareInsensitive(in_str, trigger_trunc))
			{
				*out_str = trigger;
				return TRUE;
			}
		}
	}
	return FALSE;
}


void LLGestureMgr::getItemIDs(uuid_vec_t* ids)
{
	item_map_t::const_iterator it;
	for (it = mActive.begin(); it != mActive.end(); ++it)
	{
		ids->push_back(it->first);
	}
}

void LLGestureMgr::done()
{
	bool notify = false;
	for(item_map_t::iterator it = mActive.begin(); it != mActive.end(); ++it)
	{
		if(it->second && it->second->mName.empty())
		{
			LLViewerInventoryItem* item = gInventory.getItem(it->first);
			if(item)
			{
				it->second->mName = item->getName();
				notify = true;
			}
		}
	}
	if(notify)
	{
		notifyObservers();
	}
}

// static
const LLUUID& get_linked_uuid(const LLUUID &item_id)
{
	LLViewerInventoryItem* item = gInventory.getItem(item_id);
	if (item && item->getIsLinkType())
	{
		return item->getLinkedUUID();
	}
	return item_id;
}

