/**
 * @file llviewermedia.cpp
 * @brief Client interface to the media engine
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

#include "llviewermedia.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llappviewer.h"
#include "llaudioengine.h"  // for gAudiop
#include "llcallbacklist.h"
#include "lldir.h"
#include "lldiriterator.h"
#include "llevent.h"		// LLSimpleListener
#include "llfilepicker.h"
#include "llfloaterwebcontent.h"	// for handling window close requests and geometry change requests in media browser windows.
#include "llfocusmgr.h"
#include "llimagegl.h"
#include "llkeyboard.h"
#include "lllogininstance.h"
#include "llmarketplacefunctions.h"
#include "llmediaentry.h"
#include "llmimetypes.h"
#include "llmutelist.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llavataractions.h"
#include "llparcel.h"
#include "llpluginclassmedia.h"
#include "llurldispatcher.h"
#include "lluuid.h"
#include "llversioninfo.h"
#include "llviewermediafocus.h"
#include "llviewercontrol.h"
#include "llviewermenufile.h" // LLFilePickerThread
#include "llviewernetwork.h"
#include "llviewerparcelaskplay.h"
#include "llviewerparcelmedia.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llvovolume.h"
#include "llfloaterreg.h"
#include "llwebprofile.h"
#include "llwindow.h"
#include "llvieweraudio.h"
#include "llcorehttputil.h"

#include "llfloaterwebcontent.h"	// for handling window close requests and geometry change requests in media browser windows.

#include <boost/bind.hpp>	// for SkinFolder listener
#include <boost/signals2.hpp>

extern bool gCubeSnapshot;

// *TODO: Consider enabling mipmaps (they have been disabled for a long time). Likely has a significant performance impact for tiled/high texture repeat media. Mip generation in a shader may also be an option if necessary.
constexpr bool USE_MIPMAPS = false;

void init_threaded_picker_load_dialog(LLPluginClassMedia* plugin, LLFilePicker::ELoadFilter filter, bool get_multiple)
{
    (new LLMediaFilePicker(plugin, filter, get_multiple))->getFile(); // will delete itself
}

///////////////////////////////////////////////////////////////////////////////

// Move this to its own file.

LLViewerMediaEventEmitter::~LLViewerMediaEventEmitter()
{
	observerListType::iterator iter = mObservers.begin();

	while( iter != mObservers.end() )
	{
		LLViewerMediaObserver *self = *iter;
		iter++;
		remObserver(self);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaEventEmitter::addObserver( LLViewerMediaObserver* observer )
{
	if ( ! observer )
		return false;

	if ( std::find( mObservers.begin(), mObservers.end(), observer ) != mObservers.end() )
		return false;

	mObservers.push_back( observer );
	observer->mEmitters.push_back( this );

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaEventEmitter::remObserver( LLViewerMediaObserver* observer )
{
	if ( ! observer )
		return false;

	mObservers.remove( observer );
	observer->mEmitters.remove(this);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
void LLViewerMediaEventEmitter::emitEvent( LLPluginClassMedia* media, LLViewerMediaObserver::EMediaEvent event )
{
	// Broadcast the event to any observers.
	observerListType::iterator iter = mObservers.begin();
	while( iter != mObservers.end() )
	{
		LLViewerMediaObserver *self = *iter;
		++iter;
		self->handleMediaEvent( media, event );
	}
}

// Move this to its own file.
LLViewerMediaObserver::~LLViewerMediaObserver()
{
	std::list<LLViewerMediaEventEmitter *>::iterator iter = mEmitters.begin();

	while( iter != mEmitters.end() )
	{
		LLViewerMediaEventEmitter *self = *iter;
		iter++;
		self->remObserver( this );
	}
}


static LLViewerMedia::impl_list sViewerMediaImplList;
static LLViewerMedia::impl_id_map sViewerMediaTextureIDMap;
static LLTimer sMediaCreateTimer;
static const F32 LLVIEWERMEDIA_CREATE_DELAY = 1.0f;
static F32 sGlobalVolume = 1.0f;
static bool sForceUpdate = false;
static LLUUID sOnlyAudibleTextureID = LLUUID::null;
static F64 sLowestLoadableImplInterest = 0.0f;

//////////////////////////////////////////////////////////////////////////////////////////
static void add_media_impl(LLViewerMediaImpl* media)
{
	sViewerMediaImplList.push_back(media);
}

//////////////////////////////////////////////////////////////////////////////////////////
static void remove_media_impl(LLViewerMediaImpl* media)
{
	LLViewerMedia::impl_list::iterator iter = sViewerMediaImplList.begin();
	LLViewerMedia::impl_list::iterator end = sViewerMediaImplList.end();

	for(; iter != end; iter++)
	{
		if(media == *iter)
		{
			sViewerMediaImplList.erase(iter);
			return;
		}
	}
}

class LLViewerMediaMuteListObserver : public LLMuteListObserver
{
	/* virtual */ void onChange()  { LLViewerMedia::getInstance()->muteListChanged();}
};

static LLViewerMediaMuteListObserver sViewerMediaMuteListObserver;
static bool sViewerMediaMuteListObserverInitialized = false;


//////////////////////////////////////////////////////////////////////////////////////////
// LLViewerMedia
//////////////////////////////////////////////////////////////////////////////////////////

/*static*/ const char* LLViewerMedia::AUTO_PLAY_MEDIA_SETTING = "ParcelMediaAutoPlayEnable";
/*static*/ const char* LLViewerMedia::SHOW_MEDIA_ON_OTHERS_SETTING = "MediaShowOnOthers";
/*static*/ const char* LLViewerMedia::SHOW_MEDIA_WITHIN_PARCEL_SETTING = "MediaShowWithinParcel";
/*static*/ const char* LLViewerMedia::SHOW_MEDIA_OUTSIDE_PARCEL_SETTING = "MediaShowOutsideParcel";

LLViewerMedia::LLViewerMedia():
mAnyMediaShowing(false),
mAnyMediaPlaying(false),
mSpareBrowserMediaSource(NULL)
{
}

LLViewerMedia::~LLViewerMedia()
{
    gIdleCallbacks.deleteFunction(LLViewerMedia::onIdle, NULL);
    mTeleportFinishConnection.disconnect();
    if (mSpareBrowserMediaSource != NULL)
    {
        delete mSpareBrowserMediaSource;
        mSpareBrowserMediaSource = NULL;
    }
}

// static
void LLViewerMedia::initSingleton()
{
    gIdleCallbacks.addFunction(LLViewerMedia::onIdle, NULL);
    mTeleportFinishConnection = LLViewerParcelMgr::getInstance()->
        setTeleportFinishedCallback(boost::bind(&LLViewerMedia::onTeleportFinished, this));
}

//////////////////////////////////////////////////////////////////////////////////////////
viewer_media_t LLViewerMedia::newMediaImpl(
											 const LLUUID& texture_id,
											 S32 media_width,
											 S32 media_height,
											 U8 media_auto_scale,
											 U8 media_loop)
{
	LLViewerMediaImpl* media_impl = getMediaImplFromTextureID(texture_id);
	if(media_impl == NULL || texture_id.isNull())
	{
		// Create the media impl
		media_impl = new LLViewerMediaImpl(texture_id, media_width, media_height, media_auto_scale, media_loop);
	}
	else
	{
		media_impl->unload();
		media_impl->setTextureID(texture_id);
		media_impl->mMediaWidth = media_width;
		media_impl->mMediaHeight = media_height;
		media_impl->mMediaAutoScale = media_auto_scale;
		media_impl->mMediaLoop = media_loop;
	}

	return media_impl;
}

viewer_media_t LLViewerMedia::updateMediaImpl(LLMediaEntry* media_entry, const std::string& previous_url, bool update_from_self)
{
    llassert(!gCubeSnapshot);
	// Try to find media with the same media ID
	viewer_media_t media_impl = getMediaImplFromTextureID(media_entry->getMediaID());

	LL_DEBUGS() << "called, current URL is \"" << media_entry->getCurrentURL()
			<< "\", previous URL is \"" << previous_url
			<< "\", update_from_self is " << (update_from_self?"true":"false")
			<< LL_ENDL;

	bool was_loaded = false;
	bool needs_navigate = false;

	if(media_impl)
	{
		was_loaded = media_impl->hasMedia();

		media_impl->setHomeURL(media_entry->getHomeURL());

		media_impl->mMediaAutoScale = media_entry->getAutoScale();
		media_impl->mMediaLoop = media_entry->getAutoLoop();
		media_impl->mMediaWidth = media_entry->getWidthPixels();
		media_impl->mMediaHeight = media_entry->getHeightPixels();
		media_impl->mMediaAutoPlay = media_entry->getAutoPlay();
		media_impl->mMediaEntryURL = media_entry->getCurrentURL();
		if (media_impl->mMediaSource)
		{
			media_impl->mMediaSource->setAutoScale(media_impl->mMediaAutoScale);
			media_impl->mMediaSource->setLoop(media_impl->mMediaLoop);
			media_impl->mMediaSource->setSize(media_entry->getWidthPixels(), media_entry->getHeightPixels());
		}

		bool url_changed = (media_impl->mMediaEntryURL != previous_url);
		if(media_impl->mMediaEntryURL.empty())
		{
			if(url_changed)
			{
				// The current media URL is now empty.  Unload the media source.
				media_impl->unload();

				LL_DEBUGS() << "Unloading media instance (new current URL is empty)." << LL_ENDL;
			}
		}
		else
		{
			// The current media URL is not empty.
			// If (the media was already loaded OR the media was set to autoplay) AND this update didn't come from this agent,
			// do a navigate.
			bool auto_play = media_impl->isAutoPlayable();
			if((was_loaded || auto_play) && !update_from_self)
			{
				needs_navigate = url_changed;
			}

			LL_DEBUGS() << "was_loaded is " << (was_loaded?"true":"false")
					<< ", auto_play is " << (auto_play?"true":"false")
					<< ", needs_navigate is " << (needs_navigate?"true":"false") << LL_ENDL;
		}
	}
	else
	{
		media_impl = newMediaImpl(
			media_entry->getMediaID(),
			media_entry->getWidthPixels(),
			media_entry->getHeightPixels(),
			media_entry->getAutoScale(),
			media_entry->getAutoLoop());

		media_impl->setHomeURL(media_entry->getHomeURL());
		media_impl->mMediaAutoPlay = media_entry->getAutoPlay();
		media_impl->mMediaEntryURL = media_entry->getCurrentURL();
		if(media_impl->isAutoPlayable())
		{
			needs_navigate = true;
		}
	}

	if(media_impl)
	{
		if(needs_navigate)
		{
			media_impl->navigateTo(media_impl->mMediaEntryURL, "", true, true);
			LL_DEBUGS() << "navigating to URL " << media_impl->mMediaEntryURL << LL_ENDL;
		}
		else if(!media_impl->mMediaURL.empty() && (media_impl->mMediaURL != media_impl->mMediaEntryURL))
		{
			// If we already have a non-empty media URL set and we aren't doing a navigate, update the media URL to match the media entry.
			media_impl->mMediaURL = media_impl->mMediaEntryURL;

			// If this causes a navigate at some point (such as after a reload), it should be considered server-driven so it isn't broadcast.
			media_impl->mNavigateServerRequest = true;

			LL_DEBUGS() << "updating URL in the media impl to " << media_impl->mMediaEntryURL << LL_ENDL;
		}
	}

	return media_impl;
}

//////////////////////////////////////////////////////////////////////////////////////////
LLViewerMediaImpl* LLViewerMedia::getMediaImplFromTextureID(const LLUUID& texture_id)
{
	LLViewerMediaImpl* result = NULL;

	// Look up the texture ID in the texture id->impl map.
	impl_id_map::iterator iter = sViewerMediaTextureIDMap.find(texture_id);
	if(iter != sViewerMediaTextureIDMap.end())
	{
		result = iter->second;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
std::string LLViewerMedia::getCurrentUserAgent()
{
	// Don't use user-visible string to avoid
	// punctuation and strange characters.
	std::string skin_name = gSavedSettings.getString("SkinCurrent");

	// Just in case we need to check browser differences in A/B test
	// builds.
	std::string channel = LLVersionInfo::instance().getChannel();

	// append our magic version number string to the browser user agent id
	// See the HTTP 1.0 and 1.1 specifications for allowed formats:
	// http://www.ietf.org/rfc/rfc1945.txt section 10.15
	// http://www.ietf.org/rfc/rfc2068.txt section 3.8
	// This was also helpful:
	// http://www.mozilla.org/build/revised-user-agent-strings.html
	std::ostringstream codec;
	codec << "SecondLife/";
	codec << LLVersionInfo::instance().getVersion();
	codec << " (" << channel << "; " << skin_name << " skin)";
	LL_INFOS() << codec.str() << LL_ENDL;

	return codec.str();
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::updateBrowserUserAgent()
{
	std::string user_agent = getCurrentUserAgent();

	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if(pimpl->mMediaSource && pimpl->mMediaSource->pluginSupportsMediaBrowser())
		{
			pimpl->mMediaSource->setBrowserUserAgent(user_agent);
		}
	}

}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMedia::handleSkinCurrentChanged(const LLSD& /*newvalue*/)
{
	// gSavedSettings is already updated when this function is called.
	updateBrowserUserAgent();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMedia::textureHasMedia(const LLUUID& texture_id)
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if(pimpl->getMediaTextureID() == texture_id)
		{
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::setVolume(F32 volume)
{
	if(volume != sGlobalVolume || sForceUpdate)
	{
		sGlobalVolume = volume;
		impl_list::iterator iter = sViewerMediaImplList.begin();
		impl_list::iterator end = sViewerMediaImplList.end();

		for(; iter != end; iter++)
		{
			LLViewerMediaImpl* pimpl = *iter;
			pimpl->updateVolume();
		}

		sForceUpdate = false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
F32 LLViewerMedia::getVolume()
{
	return sGlobalVolume;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::muteListChanged()
{
	// When the mute list changes, we need to check mute status on all impls.
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		pimpl->mNeedsMuteCheck = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMedia::isInterestingEnough(const LLVOVolume *object, const F64 &object_interest)
{
	bool result = false;

	if (NULL == object)
	{
		result = false;
	}
	// Focused?  Then it is interesting!
	else if (LLViewerMediaFocus::getInstance()->getFocusedObjectID() == object->getID())
	{
		result = true;
	}
	// Selected?  Then it is interesting!
	// XXX Sadly, 'contains()' doesn't take a const :(
	else if (LLSelectMgr::getInstance()->getSelection()->contains(const_cast<LLVOVolume*>(object)))
	{
		result = true;
	}
	else
	{
		LL_DEBUGS() << "object interest = " << object_interest << ", lowest loadable = " << sLowestLoadableImplInterest << LL_ENDL;
		if(object_interest >= sLowestLoadableImplInterest)
			result = true;
	}

	return result;
}

LLViewerMedia::impl_list &LLViewerMedia::getPriorityList()
{
	return sViewerMediaImplList;
}

// static
// This is the predicate function used to sort sViewerMediaImplList by priority.
bool LLViewerMedia::priorityComparitor(const LLViewerMediaImpl* i1, const LLViewerMediaImpl* i2)
{
	if(i1->isForcedUnloaded() && !i2->isForcedUnloaded())
	{
		// Muted or failed items always go to the end of the list, period.
		return false;
	}
	else if(i2->isForcedUnloaded() && !i1->isForcedUnloaded())
	{
		// Muted or failed items always go to the end of the list, period.
		return true;
	}
	else if(i1->hasFocus())
	{
		// The item with user focus always comes to the front of the list, period.
		return true;
	}
	else if(i2->hasFocus())
	{
		// The item with user focus always comes to the front of the list, period.
		return false;
	}
	else if(i1->isParcelMedia())
	{
		// The parcel media impl sorts above all other inworld media, unless one has focus.
		return true;
	}
	else if(i2->isParcelMedia())
	{
		// The parcel media impl sorts above all other inworld media, unless one has focus.
		return false;
	}
	else if(i1->getUsedInUI() && !i2->getUsedInUI())
	{
		// i1 is a UI element, i2 is not.  This makes i1 "less than" i2, so it sorts earlier in our list.
		return true;
	}
	else if(i2->getUsedInUI() && !i1->getUsedInUI())
	{
		// i2 is a UI element, i1 is not.  This makes i2 "less than" i1, so it sorts earlier in our list.
		return false;
	}
	else if(i1->isPlayable() && !i2->isPlayable())
	{
		// Playable items sort above ones that wouldn't play even if they got high enough priority
		return true;
	}
	else if(!i1->isPlayable() && i2->isPlayable())
	{
		// Playable items sort above ones that wouldn't play even if they got high enough priority
		return false;
	}
	else if(i1->getInterest() == i2->getInterest())
	{
		// Generally this will mean both objects have zero interest.  In this case, sort on distance.
		return (i1->getProximityDistance() < i2->getProximityDistance());
	}
	else
	{
		// The object with the larger interest value should be earlier in the list, so we reverse the sense of the comparison here.
		return (i1->getInterest() > i2->getInterest());
	}
}

static bool proximity_comparitor(const LLViewerMediaImpl* i1, const LLViewerMediaImpl* i2)
{
	if(i1->getProximityDistance() < i2->getProximityDistance())
	{
		return true;
	}
	else if(i1->getProximityDistance() > i2->getProximityDistance())
	{
		return false;
	}
	else
	{
		// Both objects have the same distance.  This most likely means they're two faces of the same object.
		// They may also be faces on different objects with exactly the same distance (like HUD objects).
		// We don't actually care what the sort order is for this case, as long as it's stable and doesn't change when you enable/disable media.
		// Comparing the impl pointers gives a completely arbitrary ordering, but it will be stable.
		return (i1 < i2);
	}
}

static LLTrace::BlockTimerStatHandle FTM_MEDIA_UPDATE("Update Media");
static LLTrace::BlockTimerStatHandle FTM_MEDIA_SPARE_IDLE("Spare Idle");
static LLTrace::BlockTimerStatHandle FTM_MEDIA_UPDATE_INTEREST("Update/Interest");
static LLTrace::BlockTimerStatHandle FTM_MEDIA_UPDATE_VOLUME("Update/Volume");
static LLTrace::BlockTimerStatHandle FTM_MEDIA_SORT("Media Sort");
static LLTrace::BlockTimerStatHandle FTM_MEDIA_SORT2("Media Sort 2");
static LLTrace::BlockTimerStatHandle FTM_MEDIA_MISC("Misc");


//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::onIdle(void *dummy_arg)
{
    LLViewerMedia::getInstance()->updateMedia(dummy_arg);
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::updateMedia(void *dummy_arg)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_MEDIA; //LL_RECORD_BLOCK_TIME(FTM_MEDIA_UPDATE);

    llassert(!gCubeSnapshot);

	// Enable/disable the plugin read thread
	LLPluginProcessParent::setUseReadThread(gSavedSettings.getBOOL("PluginUseReadThread"));

    // SL-16418 We can't call LLViewerMediaImpl->update() if we are in the state of shutting down.
    if(LLApp::isExiting())
    {
        setAllMediaEnabled(false);
        return;
    }

	// HACK: we always try to keep a spare running webkit plugin around to improve launch times.
	// 2017-04-19 Removed CP - this doesn't appear to buy us much and consumes a lot of resources so
	// removing it for now.
	//createSpareBrowserMediaSource();

	mAnyMediaShowing = false;
	mAnyMediaPlaying = false;

	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	{
        LL_PROFILE_ZONE_NAMED_CATEGORY_MEDIA("media update interest"); //LL_RECORD_BLOCK_TIME(FTM_MEDIA_UPDATE_INTEREST);
		for(; iter != end;)
		{
			LLViewerMediaImpl* pimpl = *iter++;
			pimpl->update();
			pimpl->calculateInterest();
		}
	}

	// Let the spare media source actually launch
	if(mSpareBrowserMediaSource)
	{
        LL_PROFILE_ZONE_NAMED_CATEGORY_MEDIA("media spare idle"); //LL_RECORD_BLOCK_TIME(FTM_MEDIA_SPARE_IDLE);
		mSpareBrowserMediaSource->idle();
	}

	{
        LL_PROFILE_ZONE_NAMED_CATEGORY_MEDIA("media sort"); //LL_RECORD_BLOCK_TIME(FTM_MEDIA_SORT);
		// Sort the static instance list using our interest criteria
		sViewerMediaImplList.sort(priorityComparitor);
	}

	// Go through the list again and adjust according to priority.
	iter = sViewerMediaImplList.begin();
	end = sViewerMediaImplList.end();

	F64 total_cpu = 0.0f;
	int impl_count_total = 0;
	int impl_count_interest_low = 0;
	int impl_count_interest_normal = 0;

	std::vector<LLViewerMediaImpl*> proximity_order;

	static LLCachedControl<bool> inworld_media_enabled(gSavedSettings, "AudioStreamingMedia", true);
	static LLCachedControl<bool> inworld_audio_enabled(gSavedSettings, "AudioStreamingMusic", true);
	U32 max_instances = gSavedSettings.getU32("PluginInstancesTotal");
	U32 max_normal = gSavedSettings.getU32("PluginInstancesNormal");
	U32 max_low = gSavedSettings.getU32("PluginInstancesLow");
	F32 max_cpu = gSavedSettings.getF32("PluginInstancesCPULimit");
	// Setting max_cpu to 0.0 disables CPU usage checking.
	bool check_cpu_usage = (max_cpu != 0.0f);

	LLViewerMediaImpl* lowest_interest_loadable = NULL;

	// Notes on tweakable params:
	// max_instances must be set high enough to allow the various instances used in the UI (for the help browser, search, etc.) to be loaded.
	// If max_normal + max_low is less than max_instances, things will tend to get unloaded instead of being set to slideshow.

	{
        LL_PROFILE_ZONE_NAMED_CATEGORY_MEDIA("media misc"); //LL_RECORD_BLOCK_TIME(FTM_MEDIA_MISC);
		for(; iter != end; iter++)
		{
			LLViewerMediaImpl* pimpl = *iter;

			LLPluginClassMedia::EPriority new_priority = LLPluginClassMedia::PRIORITY_NORMAL;

			if(pimpl->isForcedUnloaded() || (impl_count_total >= (int)max_instances))
			{
				// Never load muted or failed impls.
				// Hard limit on the number of instances that will be loaded at one time
				new_priority = LLPluginClassMedia::PRIORITY_UNLOADED;
			}
			else if(!pimpl->getVisible())
			{
				new_priority = LLPluginClassMedia::PRIORITY_HIDDEN;
			}
			else if(pimpl->hasFocus())
			{
				new_priority = LLPluginClassMedia::PRIORITY_HIGH;
				impl_count_interest_normal++;	// count this against the count of "normal" instances for priority purposes
			}
			else if(pimpl->getUsedInUI())
			{
				new_priority = LLPluginClassMedia::PRIORITY_NORMAL;
				impl_count_interest_normal++;
			}
			else if(pimpl->isParcelMedia())
			{
				new_priority = LLPluginClassMedia::PRIORITY_NORMAL;
				impl_count_interest_normal++;
			}
			else
			{
				// Look at interest and CPU usage for instances that aren't in any of the above states.

				// Heuristic -- if the media texture's approximate screen area is less than 1/4 of the native area of the texture,
				// turn it down to low instead of normal.  This may downsample for plugins that support it.
				bool media_is_small = false;
				F64 approximate_interest = pimpl->getApproximateTextureInterest();
				if(approximate_interest == 0.0f)
				{
					// this media has no current size, which probably means it's not loaded.
					media_is_small = true;
				}
				else if(pimpl->getInterest() < (approximate_interest / 4))
				{
					media_is_small = true;
				}

				if(pimpl->getInterest() == 0.0f)
				{
					// This media is completely invisible, due to being outside the view frustrum or out of range.
					new_priority = LLPluginClassMedia::PRIORITY_HIDDEN;
				}
				else if(check_cpu_usage && (total_cpu > max_cpu))
				{
					// Higher priority plugins have already used up the CPU budget.  Set remaining ones to slideshow priority.
					new_priority = LLPluginClassMedia::PRIORITY_SLIDESHOW;
				}
				else if((impl_count_interest_normal < (int)max_normal) && !media_is_small)
				{
					// Up to max_normal inworld get normal priority
					new_priority = LLPluginClassMedia::PRIORITY_NORMAL;
					impl_count_interest_normal++;
				}
				else if (impl_count_interest_low + impl_count_interest_normal < (int)max_low + (int)max_normal)
				{
					// The next max_low inworld get turned down
					new_priority = LLPluginClassMedia::PRIORITY_LOW;
					impl_count_interest_low++;

					// Set the low priority size for downsampling to approximately the size the texture is displayed at.
					{
						F32 approximate_interest_dimension = (F32) sqrt(pimpl->getInterest());

						pimpl->setLowPrioritySizeLimit(ll_round(approximate_interest_dimension));
					}
				}
				else
				{
					// Any additional impls (up to max_instances) get very infrequent time
					new_priority = LLPluginClassMedia::PRIORITY_SLIDESHOW;
				}
			}

			if(!pimpl->getUsedInUI() && (new_priority != LLPluginClassMedia::PRIORITY_UNLOADED))
			{
				// This is a loadable inworld impl -- the last one in the list in this class defines the lowest loadable interest.
				lowest_interest_loadable = pimpl;

				impl_count_total++;
			}

			// Overrides if the window is minimized or we lost focus (taking care
			// not to accidentally "raise" the priority either)
			if (!gViewerWindow->getActive() /* viewer window minimized? */
				&& new_priority > LLPluginClassMedia::PRIORITY_HIDDEN)
			{
				new_priority = LLPluginClassMedia::PRIORITY_HIDDEN;
			}
			else if (!gFocusMgr.getAppHasFocus() /* viewer window lost focus? */
					 && new_priority > LLPluginClassMedia::PRIORITY_LOW)
			{
				new_priority = LLPluginClassMedia::PRIORITY_LOW;
			}

			if(!inworld_media_enabled)
			{
				// If inworld media is locked out, force all inworld media to stay unloaded.
				if(!pimpl->getUsedInUI())
				{
					new_priority = LLPluginClassMedia::PRIORITY_UNLOADED;
				}
			}
			// update the audio stream here as well
			static bool restore_parcel_audio = false;
			if( !inworld_audio_enabled)
			{
				if(LLViewerMedia::isParcelAudioPlaying() && gAudiop && LLViewerMedia::hasParcelAudio())
				{
					LLViewerAudio::getInstance()->stopInternetStreamWithAutoFade();
					restore_parcel_audio = true;
				}
			}
            else
            {
                if(gAudiop && LLViewerMedia::hasParcelAudio() && restore_parcel_audio && gSavedSettings.getBOOL("MediaTentativeAutoPlay"))
                {
                    LLViewerAudio::getInstance()->startInternetStreamWithAutoFade(LLViewerMedia::getParcelAudioURL());
                    restore_parcel_audio = false;
                }
            }

			pimpl->setPriority(new_priority);

			if(pimpl->getUsedInUI())
			{
				// Any impls used in the UI should not be in the proximity list.
				pimpl->mProximity = -1;
			}
			else
			{
				proximity_order.push_back(pimpl);
			}

			total_cpu += pimpl->getCPUUsage();

			if (!pimpl->getUsedInUI() && pimpl->hasMedia())
			{
				mAnyMediaShowing = true;
			}

			if (!pimpl->getUsedInUI() && pimpl->hasMedia() && (pimpl->isMediaPlaying() || !pimpl->isMediaTimeBased()))
			{
				// consider visible non-timebased media as playing
				mAnyMediaPlaying = true;
			}

		}
	}

	// Re-calculate this every time.
	sLowestLoadableImplInterest	= 0.0f;

	// Only do this calculation if we've hit the impl count limit -- up until that point we always need to load media data.
	if(lowest_interest_loadable && (impl_count_total >= (int)max_instances))
	{
		// Get the interest value of this impl's object for use by isInterestingEnough
		LLVOVolume *object = lowest_interest_loadable->getSomeObject();
		if(object)
		{
			// NOTE: Don't use getMediaInterest() here.  We want the pixel area, not the total media interest,
			// 		so that we match up with the calculation done in LLMediaDataClient.
			sLowestLoadableImplInterest = object->getPixelArea();
		}
	}

	if(gSavedSettings.getBOOL("MediaPerformanceManagerDebug"))
	{
		// Give impls the same ordering as the priority list
		// they're already in the right order for this.
	}
	else
	{
        LL_PROFILE_ZONE_NAMED_CATEGORY_MEDIA("media sort2"); // LL_RECORD_BLOCK_TIME(FTM_MEDIA_SORT2);
		// Use a distance-based sort for proximity values.
		std::stable_sort(proximity_order.begin(), proximity_order.end(), proximity_comparitor);
	}

	// Transfer the proximity order to the proximity fields in the objects.
	for(int i = 0; i < (int)proximity_order.size(); i++)
	{
		proximity_order[i]->mProximity = i;
	}

	LL_DEBUGS("PluginPriority") << "Total reported CPU usage is " << total_cpu << LL_ENDL;

}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMedia::isAnyMediaShowing()
{
	return mAnyMediaShowing;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMedia::isAnyMediaPlaying()
{
    return mAnyMediaPlaying;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::setAllMediaEnabled(bool val)
{
	// Set "tentative" autoplay first.  We need to do this here or else
	// re-enabling won't start up the media below.
	gSavedSettings.setBOOL("MediaTentativeAutoPlay", val);

	// Then
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for(; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if (!pimpl->getUsedInUI())
		{
			pimpl->setDisabled(!val);
		}
	}

	// Also do Parcel Media and Parcel Audio
	if (val)
	{
		if (!LLViewerMedia::isParcelMediaPlaying() && LLViewerMedia::hasParcelMedia())
		{
			LLViewerParcelMedia::getInstance()->play(LLViewerParcelMgr::getInstance()->getAgentParcel());
		}

		static LLCachedControl<bool> audio_streaming_music(gSavedSettings, "AudioStreamingMusic", true);
		if (audio_streaming_music &&
			!LLViewerMedia::isParcelAudioPlaying() &&
			gAudiop &&
			LLViewerMedia::hasParcelAudio())
		{
			if (LLAudioEngine::AUDIO_PAUSED == gAudiop->isInternetStreamPlaying())
			{
				// 'false' means unpause
				gAudiop->pauseInternetStream(false);
			}
			else
			{
				LLViewerAudio::getInstance()->startInternetStreamWithAutoFade(LLViewerMedia::getParcelAudioURL());
			}
		}
	}
	else {
		// This actually unloads the impl, as opposed to "stop"ping the media
		LLViewerParcelMedia::getInstance()->stop();
		if (gAudiop)
		{
			LLViewerAudio::getInstance()->stopInternetStreamWithAutoFade();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::setAllMediaPaused(bool val)
{
    // Set "tentative" autoplay first.  We need to do this here or else
    // re-enabling won't start up the media below.
    gSavedSettings.setBOOL("MediaTentativeAutoPlay", !val);

    // Then
    impl_list::iterator iter = sViewerMediaImplList.begin();
    impl_list::iterator end = sViewerMediaImplList.end();

    for (; iter != end; iter++)
    {
        LLViewerMediaImpl* pimpl = *iter;
        if (!pimpl->getUsedInUI())
        {
            // upause/pause time based media, enable/disable any other
            if (!val)
            {
                pimpl->setDisabled(val);
                if (pimpl->isMediaTimeBased() && pimpl->isMediaPaused())
                {
                    pimpl->play();
                }
            }
            else if (pimpl->isMediaTimeBased() && pimpl->mMediaSource && (pimpl->isMediaPlaying() || pimpl->isMediaPaused()))
            {
                pimpl->pause();
            }
            else
            {
                pimpl->setDisabled(val);
            }
        }
    }

    LLParcel *agent_parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();

    // Also do Parcel Media and Parcel Audio
    if (!val)
    {
        if (!LLViewerMedia::isParcelMediaPlaying() && LLViewerMedia::hasParcelMedia())
        {
            LLViewerParcelMedia::getInstance()->play(agent_parcel);
        }

        static LLCachedControl<bool> audio_streaming_music(gSavedSettings, "AudioStreamingMusic", true);
        if (audio_streaming_music &&
            !LLViewerMedia::isParcelAudioPlaying() &&
            gAudiop &&
            LLViewerMedia::hasParcelAudio())
        {
            if (LLAudioEngine::AUDIO_PAUSED == gAudiop->isInternetStreamPlaying())
            {
                // 'false' means unpause
                gAudiop->pauseInternetStream(false);
            }
            else
            {
                LLViewerAudio::getInstance()->startInternetStreamWithAutoFade(LLViewerMedia::getParcelAudioURL());
            }
        }
    }
    else {
        // This actually unloads the impl, as opposed to "stop"ping the media
        LLViewerParcelMedia::getInstance()->stop();
        if (gAudiop)
        {
            LLViewerAudio::getInstance()->stopInternetStreamWithAutoFade();
        }
    }

    // remove play choice for current parcel
    if (agent_parcel && gAgent.getRegion())
    {
        LLViewerParcelAskPlay::getInstance()->resetSetting(gAgent.getRegion()->getRegionID(), agent_parcel->getLocalID());
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMedia::isParcelMediaPlaying()
{
    viewer_media_t media = LLViewerParcelMedia::getInstance()->getParcelMedia();
    return (LLViewerMedia::hasParcelMedia() && media && media->hasMedia());
}

/////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMedia::isParcelAudioPlaying()
{
	return (LLViewerMedia::hasParcelAudio() && gAudiop && LLAudioEngine::AUDIO_PLAYING == gAudiop->isInternetStreamPlaying());
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::authSubmitCallback(const LLSD& notification, const LLSD& response)
{
    LLViewerMedia::getInstance()->onAuthSubmit(notification, response);
}

void LLViewerMedia::onAuthSubmit(const LLSD& notification, const LLSD& response)
{
	LLViewerMediaImpl *impl = LLViewerMedia::getMediaImplFromTextureID(notification["payload"]["media_id"]);
	if(impl)
	{
		LLPluginClassMedia* media = impl->getMediaPlugin();
		if(media)
		{
			if (response["ok"])
			{
				media->sendAuthResponse(true, response["username"], response["password"]);
			}
			else
			{
				media->sendAuthResponse(false, "", "");
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::clearAllCookies()
{
	// Clear all cookies for all plugins
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for (; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if(pimpl->mMediaSource)
		{
			pimpl->mMediaSource->clear_cookies();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::clearAllCaches()
{
	// Clear all plugins' caches
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for (; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		pimpl->clearCache();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::setCookiesEnabled(bool enabled)
{
	// Set the "cookies enabled" flag for all loaded plugins
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for (; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if(pimpl->mMediaSource)
		{
			pimpl->mMediaSource->cookies_enabled(enabled);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::setProxyConfig(bool enable, const std::string &host, int port)
{
	// Set the proxy config for all loaded plugins
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for (; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if(pimpl->mMediaSource)
		{
			pimpl->mMediaSource->proxy_setup(enable, host, port);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
LLSD LLViewerMedia::getHeaders()
{
	LLSD headers = LLSD::emptyMap();
	headers[HTTP_OUT_HEADER_ACCEPT] = "*/*";
	// *TODO: Should this be 'application/llsd+xml' ?
	// *TODO: Should this even be set at all?   This header is only not overridden in 'GET' methods.
	headers[HTTP_OUT_HEADER_CONTENT_TYPE] = HTTP_CONTENT_XML;
	headers[HTTP_OUT_HEADER_COOKIE] = mOpenIDCookie;
	headers[HTTP_OUT_HEADER_USER_AGENT] = getCurrentUserAgent();

	return headers;
}

 /////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMedia::parseRawCookie(const std::string raw_cookie, std::string& name, std::string& value, std::string& path, bool& httponly, bool& secure)
{
	std::size_t name_pos = raw_cookie.find_first_of("=");
	if (name_pos != std::string::npos)
	{
		name = raw_cookie.substr(0, name_pos);
		std::size_t value_pos = raw_cookie.find_first_of(";", name_pos);
		if (value_pos != std::string::npos)
		{
			value = raw_cookie.substr(name_pos + 1, value_pos - name_pos - 1);
			path = "/";	// assume root path for now

			httponly = true;	// hard coded for now
			secure = true;

			return true;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////
LLCore::HttpHeaders::ptr_t LLViewerMedia::getHttpHeaders()
{
    LLCore::HttpHeaders::ptr_t headers(new LLCore::HttpHeaders);

    headers->append(HTTP_OUT_HEADER_ACCEPT, "*/*");
    headers->append(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_XML);
    headers->append(HTTP_OUT_HEADER_COOKIE, mOpenIDCookie);
    headers->append(HTTP_OUT_HEADER_USER_AGENT, getCurrentUserAgent());

    return headers;
}


/////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::setOpenIDCookie(const std::string& url)
{
	if(!gNonInteractive && !mOpenIDCookie.empty())
	{
        std::string profileUrl = getProfileURL("");

        LLCoros::instance().launch("LLViewerMedia::getOpenIDCookieCoro",
            boost::bind(&LLViewerMedia::getOpenIDCookieCoro, profileUrl));
	}
}

//static
void LLViewerMedia::getOpenIDCookieCoro(std::string url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("getOpenIDCookieCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    LLCore::HttpHeaders::ptr_t httpHeaders(new LLCore::HttpHeaders);
    
    httpOpts->setFollowRedirects(true);
    httpOpts->setWantHeaders(true);
    httpOpts->setSSLVerifyPeer(false); // viewer's cert bundle doesn't appear to agree with web certs from "https://my.secondlife.com/"

    LLURL hostUrl(url.c_str());
    std::string hostAuth = hostUrl.getAuthority();

    // *TODO: Expand LLURL to split and extract this information better. 
    // The structure of a URL is well defined and needing to retrieve parts of it are common.
    // original comment:
    // The LLURL can give me the 'authority', which is of the form: [username[:password]@]hostname[:port]
    // We want just the hostname for the cookie code, but LLURL doesn't seem to have a way to extract that.
    // We therefore do it here.
    std::string authority = getInstance()->mOpenIDURL.mAuthority;
    std::string::size_type hostStart = authority.find('@');
    if (hostStart == std::string::npos)
    {   // no username/password
        hostStart = 0;
    }
    else
    {   // Hostname starts after the @. 
			// Hostname starts after the @. 
        // (If the hostname part is empty, this may put host_start at the end of the string.  In that case, it will end up passing through an empty hostname, which is correct.)
        ++hostStart;
    }
    std::string::size_type hostEnd = authority.rfind(':');
    if ((hostEnd == std::string::npos) || (hostEnd < hostStart))
    {   // no port
        hostEnd = authority.size();
    }
    
	LLViewerMedia* inst = getInstance();
	if (url.length())
	{
		LLMediaCtrl* media_instance = LLFloaterReg::getInstance("destinations")->getChild<LLMediaCtrl>("destination_guide_contents");
		if (media_instance)
		{
			std::string cookie_host = authority.substr(hostStart, hostEnd - hostStart);
			std::string cookie_name = "";
			std::string cookie_value = "";
			std::string cookie_path = "";
			bool httponly = true;
			bool secure = true;
			if (inst->parseRawCookie(inst->mOpenIDCookie, cookie_name, cookie_value, cookie_path, httponly, secure) &&
				media_instance->getMediaPlugin())
			{
				// MAINT-5711 - inexplicably, the CEF setCookie function will no longer set the cookie if the 
				// url and domain are not the same. This used to be my.sl.com and id.sl.com respectively and worked.
				// For now, we use the URL for the OpenID POST request since it will have the same authority
				// as the domain field.
				// (Feels like there must be a less dirty way to construct a URL from component LLURL parts)
				// MAINT-6392 - Rider: Do not change, however, the original URI requested, since it is used further
				// down.
				std::string cefUrl(std::string(inst->mOpenIDURL.mURI) + "://" + std::string(inst->mOpenIDURL.mAuthority));

				media_instance->getMediaPlugin()->setCookie(cefUrl, cookie_name, cookie_value, cookie_host, 
                    cookie_path, httponly, secure);

                // Now that we have parsed the raw cookie, we must store it so that each new media instance
                // can also get a copy and faciliate logging into internal SL sites.
				media_instance->getMediaPlugin()->storeOpenIDCookie(cefUrl, cookie_name, cookie_value, 
                    cookie_host, cookie_path, httponly, secure);
			}
		}
	}

    // Note: Rider: MAINT-6392 - Some viewer code requires access to the my.sl.com openid cookie for such 
    // actions as posting snapshots to the feed.  This is handled through HTTPCore rather than CEF and so 
    // we must learn to SHARE the cookies.

	// Do a web profile get so we can store the cookie 
    httpHeaders->append(HTTP_OUT_HEADER_ACCEPT, "*/*");
    httpHeaders->append(HTTP_OUT_HEADER_COOKIE, inst->mOpenIDCookie);
    httpHeaders->append(HTTP_OUT_HEADER_USER_AGENT, inst->getCurrentUserAgent());

    LL_DEBUGS("MediaAuth") << "Requesting " << url << LL_ENDL;
    LL_DEBUGS("MediaAuth") << "sOpenIDCookie = [" << inst->mOpenIDCookie << "]" << LL_ENDL;
    
    LLSD result = httpAdapter->getRawAndSuspend(httpRequest, url, httpOpts, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS("MediaAuth") << "Error getting web profile." << LL_ENDL;
        return;
    }

    LLSD resultHeaders = httpResults[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS];
    if (!resultHeaders.has(HTTP_IN_HEADER_SET_COOKIE))
    {
        LL_WARNS("MediaAuth") << "No cookie in response." << LL_ENDL;
        return;
    }

    const std::string& cookie = resultHeaders[HTTP_IN_HEADER_SET_COOKIE].asStringRef();
    LL_DEBUGS("MediaAuth") << "cookie = " << cookie << LL_ENDL;

    // Set cookie for snapshot publishing.
    std::string authCookie = cookie.substr(0, cookie.find(";")); // strip path
    LLWebProfile::setAuthCookie(authCookie);
}

/////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::openIDSetup(const std::string &openidUrl, const std::string &openidToken)
{
	LL_DEBUGS("MediaAuth") << "url = \"" << openidUrl << "\", token = \"" << openidToken << "\"" << LL_ENDL;

    LLCoros::instance().launch("LLViewerMedia::openIDSetupCoro",
        boost::bind(&LLViewerMedia::openIDSetupCoro, openidUrl, openidToken));
}

void LLViewerMedia::openIDSetupCoro(std::string openidUrl, std::string openidToken)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("openIDSetupCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    LLCore::HttpHeaders::ptr_t httpHeaders(new LLCore::HttpHeaders);

    httpOpts->setWantHeaders(true);

	// post the token to the url 
    // the responder will need to extract the cookie(s).
    // Save the OpenID URL for later -- we may need the host when adding the cookie.
    getInstance()->mOpenIDURL.init(openidUrl.c_str());
	
    // We shouldn't ever do this twice, but just in case this code gets repurposed later, clear existing cookies.
    getInstance()->mOpenIDCookie.clear();

    httpHeaders->append(HTTP_OUT_HEADER_ACCEPT, "*/*");
    httpHeaders->append(HTTP_OUT_HEADER_CONTENT_TYPE, "application/x-www-form-urlencoded");

    LLCore::BufferArray::ptr_t rawbody(new LLCore::BufferArray);
    LLCore::BufferArrayStream bas(rawbody.get());

    bas << std::noskipws << openidToken;

    LLSD result = httpAdapter->postRawAndSuspend(httpRequest, openidUrl, rawbody, httpOpts, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS("MediaAuth") << "Error getting Open ID cookie" << LL_ENDL;
        return;
    }

    LLSD resultHeaders = httpResults[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS];
    if (!resultHeaders.has(HTTP_IN_HEADER_SET_COOKIE))
    {
        LL_WARNS("MediaAuth") << "No cookie in response." << LL_ENDL;
        return;
    }

    // We don't care about the content of the response, only the Set-Cookie header.
    const std::string& cookie = resultHeaders[HTTP_IN_HEADER_SET_COOKIE].asString();

	// *TODO: What about bad status codes?  Does this destroy previous cookies?
    LLViewerMedia::getInstance()->openIDCookieResponse(openidUrl, cookie);
    LL_DEBUGS("MediaAuth") << "OpenID cookie set." << LL_ENDL;
			
}

/////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::openIDCookieResponse(const std::string& url, const std::string &cookie)
{
	LL_DEBUGS("MediaAuth") << "Cookie received: \"" << cookie << "\"" << LL_ENDL;

	mOpenIDCookie += cookie;

	setOpenIDCookie(url);
}

/////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::proxyWindowOpened(const std::string &target, const std::string &uuid)
{
	if(uuid.empty())
		return;

	for (impl_list::iterator iter = sViewerMediaImplList.begin(); iter != sViewerMediaImplList.end(); iter++)
	{
		if((*iter)->mMediaSource && (*iter)->mMediaSource->pluginSupportsMediaBrowser())
		{
			(*iter)->mMediaSource->proxyWindowOpened(target, uuid);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::proxyWindowClosed(const std::string &uuid)
{
	if(uuid.empty())
		return;

	for (impl_list::iterator iter = sViewerMediaImplList.begin(); iter != sViewerMediaImplList.end(); iter++)
	{
		if((*iter)->mMediaSource && (*iter)->mMediaSource->pluginSupportsMediaBrowser())
		{
			(*iter)->mMediaSource->proxyWindowClosed(uuid);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::createSpareBrowserMediaSource()
{
	// If we don't have a spare browser media source, create one.
	// However, if PluginAttachDebuggerToPlugins is set then don't spawn a spare
	// SLPlugin process in order to not be confused by an unrelated gdb terminal
	// popping up at the moment we start a media plugin.
	if (!mSpareBrowserMediaSource && !gSavedSettings.getBOOL("PluginAttachDebuggerToPlugins"))
	{
		// The null owner will keep the browser plugin from fully initializing
		// (specifically, it keeps LLPluginClassMedia from negotiating a size change,
		// which keeps MediaPluginWebkit::initBrowserWindow from doing anything until we have some necessary data, like the background color)
		mSpareBrowserMediaSource = LLViewerMediaImpl::newSourceFromMediaType(HTTP_CONTENT_TEXT_HTML, NULL, 0, 0, 1.0);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
LLPluginClassMedia* LLViewerMedia::getSpareBrowserMediaSource()
{
	LLPluginClassMedia* result = mSpareBrowserMediaSource;
	mSpareBrowserMediaSource = NULL;
	return result;
};

bool LLViewerMedia::hasInWorldMedia()
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	// This should be quick, because there should be very few non-in-world-media impls
	for (; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if (!pimpl->getUsedInUI() && !pimpl->isParcelMedia())
		{
			// Found an in-world media impl
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMedia::hasParcelMedia()
{
	return !LLViewerParcelMedia::getInstance()->getURL().empty();
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMedia::hasParcelAudio()
{
	return !LLViewerMedia::getParcelAudioURL().empty();
}

//////////////////////////////////////////////////////////////////////////////////////////
std::string LLViewerMedia::getParcelAudioURL()
{
	return LLViewerParcelMgr::getInstance()->getAgentParcel()->getMusicURL();
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::onTeleportFinished()
{
	// On teleport, clear this setting (i.e. set it to true)
	gSavedSettings.setBOOL("MediaTentativeAutoPlay", true);

	LLViewerMediaImpl::sMimeTypesFailed.clear();
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMedia::setOnlyAudibleMediaTextureID(const LLUUID& texture_id)
{
	sOnlyAudibleTextureID = texture_id;
	sForceUpdate = true;
}

std::vector<std::string> LLViewerMediaImpl::sMimeTypesFailed;
//////////////////////////////////////////////////////////////////////////////////////////
// LLViewerMediaImpl
//////////////////////////////////////////////////////////////////////////////////////////
LLViewerMediaImpl::LLViewerMediaImpl(	  const LLUUID& texture_id,
										  S32 media_width,
										  S32 media_height,
										  U8 media_auto_scale,
										  U8 media_loop)
:
	mMediaSource( NULL ),
	mMovieImageHasMips(false),
	mMediaWidth(media_width),
	mMediaHeight(media_height),
	mMediaAutoScale(media_auto_scale),
	mMediaLoop(media_loop),
	mNeedsNewTexture(true),
	mTextureUsedWidth(0),
	mTextureUsedHeight(0),
	mSuspendUpdates(false),
	mVisible(true),
	mLastSetCursor( UI_CURSOR_ARROW ),
	mMediaNavState( MEDIANAVSTATE_NONE ),
	mInterest(0.0f),
	mUsedInUI(false),
	mHasFocus(false),
	mPriority(LLPluginClassMedia::PRIORITY_UNLOADED),
	mNavigateRediscoverType(false),
	mNavigateServerRequest(false),
	mMediaSourceFailed(false),
	mRequestedVolume(1.0f),
	mPreviousVolume(1.0f),
	mIsMuted(false),
	mNeedsMuteCheck(false),
	mPreviousMediaState(MEDIA_NONE),
	mPreviousMediaTime(0.0f),
	mIsDisabled(false),
	mIsParcelMedia(false),
	mProximity(-1),
	mProximityDistance(0.0f),
	mMediaAutoPlay(false),
	mInNearbyMediaList(false),
	mClearCache(false),
	mBackgroundColor(LLColor4::white),
	mNavigateSuspended(false),
	mNavigateSuspendedDeferred(false),
	mIsUpdated(false),
	mTrustedBrowser(false),
	mZoomFactor(1.0),
    mCleanBrowser(false),
    mMimeProbe(),
    mCanceling(false)
{

	// Set up the mute list observer if it hasn't been set up already.
	if(!sViewerMediaMuteListObserverInitialized)
	{
		LLMuteList::getInstance()->addObserver(&sViewerMediaMuteListObserver);
		sViewerMediaMuteListObserverInitialized = true;
	}

	add_media_impl(this);

	setTextureID(texture_id);

	// connect this media_impl to the media texture, creating it if it doesn't exist.0
	// This is necessary because we need to be able to use getMaxVirtualSize() even if the media plugin is not loaded.
    // *TODO: Consider enabling mipmaps (they have been disabled for a long time). Likely has a significant performance impact for tiled/high texture repeat media. Mip generation in a shader may also be an option if necessary.
	LLViewerMediaTexture* media_tex = LLViewerTextureManager::getMediaTexture(mTextureId, USE_MIPMAPS);
	if(media_tex)
	{
		media_tex->setMediaImpl();
	}

    mMainQueue = LL::WorkQueue::getInstance("mainloop");
    mTexUpdateQueue = LL::WorkQueue::getInstance("LLImageGL"); // Share work queue with tex loader.
}

//////////////////////////////////////////////////////////////////////////////////////////
LLViewerMediaImpl::~LLViewerMediaImpl()
{
	destroyMediaSource();

	LLViewerMediaTexture::removeMediaImplFromTexture(mTextureId) ;

	setTextureID();
	remove_media_impl(this);
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::emitEvent(LLPluginClassMedia* plugin, LLViewerMediaObserver::EMediaEvent event)
{
	// Broadcast to observers using the superclass version
	LLViewerMediaEventEmitter::emitEvent(plugin, event);

	// If this media is on one or more LLVOVolume objects, tell them about the event as well.
	std::list< LLVOVolume* >::iterator iter = mObjectList.begin() ;
	while(iter != mObjectList.end())
	{
		LLVOVolume *self = *iter;
		++iter;
		self->mediaEvent(this, plugin, event);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::initializeMedia(const std::string& mime_type)
{
	bool mimeTypeChanged = (mMimeType != mime_type);
	bool pluginChanged = (LLMIMETypes::implType(mCurrentMimeType) != LLMIMETypes::implType(mime_type));

	if(!mMediaSource || pluginChanged)
	{
		// We don't have a plugin at all, or the new mime type is handled by a different plugin than the old mime type.
		(void)initializePlugin(mime_type);
	}
	else if(mimeTypeChanged)
	{
		// The same plugin should be able to handle the new media -- just update the stored mime type.
		mMimeType = mime_type;
	}

	return (mMediaSource != NULL);
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::createMediaSource()
{
	if(mPriority == LLPluginClassMedia::PRIORITY_UNLOADED)
	{
		// This media shouldn't be created yet.
		return;
	}

	if(! mMediaURL.empty())
	{
		navigateInternal();
	}
	else if(! mMimeType.empty())
	{
		if (!initializeMedia(mMimeType))
		{
			LL_WARNS("Media") << "Failed to initialize media for mime type " << mMimeType << LL_ENDL;
		}
	}

}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::destroyMediaSource()
{
	mNeedsNewTexture = true;

	// Tell the viewer media texture it's no longer active
	LLViewerMediaTexture* oldImage = LLViewerTextureManager::findMediaTexture( mTextureId );
	if (oldImage)
	{
		oldImage->setPlaying(false) ;
	}

	cancelMimeTypeProbe();

    {
        LLMutexLock lock(&mLock); // Delay tear-down while bg thread is updating
        if(mMediaSource)
        {
            mMediaSource->setDeleteOK(true) ;
            mMediaSource = NULL; // shared pointer
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::setMediaType(const std::string& media_type)
{
	mMimeType = media_type;
}

//////////////////////////////////////////////////////////////////////////////////////////
/*static*/
LLPluginClassMedia* LLViewerMediaImpl::newSourceFromMediaType(std::string media_type, LLPluginClassMediaOwner *owner /* may be NULL */, S32 default_width, S32 default_height, F64 zoom_factor, const std::string target, bool clean_browser)
{
	if (gNonInteractive)
    {
        return NULL;
    }

	std::string plugin_basename = LLMIMETypes::implType(media_type);
	LLPluginClassMedia* media_source = NULL;

	// HACK: we always try to keep a spare running webkit plugin around to improve launch times.
	// If a spare was already created before PluginAttachDebuggerToPlugins was set, don't use it.
    // Do not use a spare if launching with full viewer control (e.g. Twitter and few others)
	if ((plugin_basename == "media_plugin_cef") &&
        !gSavedSettings.getBOOL("PluginAttachDebuggerToPlugins") && !clean_browser)
	{
		media_source = LLViewerMedia::getInstance()->getSpareBrowserMediaSource();
		if(media_source)
		{
			media_source->setOwner(owner);
			media_source->setTarget(target);
			media_source->setSize(default_width, default_height);
			media_source->setZoomFactor(zoom_factor);

			return media_source;
		}
	}
	if(plugin_basename.empty())
	{
		LL_WARNS_ONCE("Media") << "Couldn't find plugin for media type " << media_type << LL_ENDL;
	}
	else
	{
		std::string launcher_name = gDirUtilp->getLLPluginLauncher();
		std::string plugin_name = gDirUtilp->getLLPluginFilename(plugin_basename);

		std::string user_data_path_cache = gDirUtilp->getCacheDir(false);
		user_data_path_cache += gDirUtilp->getDirDelimiter();

		std::string user_data_path_cef_log = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "cef_log.txt");

		// See if the plugin executable exists
		llstat s;
		if(LLFile::stat(launcher_name, &s))
		{
			LL_WARNS_ONCE("Media") << "Couldn't find launcher at " << launcher_name << LL_ENDL;
		}
		else if(LLFile::stat(plugin_name, &s))
		{
#if !LL_LINUX
			LL_WARNS_ONCE("Media") << "Couldn't find plugin at " << plugin_name << LL_ENDL;
#endif
		}
		else
		{
			media_source = new LLPluginClassMedia(owner);
			media_source->setSize(default_width, default_height);
			media_source->setUserDataPath(user_data_path_cache, gDirUtilp->getUserName(), user_data_path_cef_log);
			media_source->setLanguageCode(LLUI::getLanguage());
			media_source->setZoomFactor(zoom_factor);

			// collect 'cookies enabled' setting from prefs and send to embedded browser
			bool cookies_enabled = gSavedSettings.getBOOL( "CookiesEnabled" );
			media_source->cookies_enabled( cookies_enabled || clean_browser);

			// collect 'javascript enabled' setting from prefs and send to embedded browser
			bool javascript_enabled = gSavedSettings.getBOOL("BrowserJavascriptEnabled");
			media_source->setJavascriptEnabled(javascript_enabled || clean_browser);

			// collect 'web security disabled' (see Chrome --web-security-disabled) setting from prefs and send to embedded browser
			bool web_security_disabled = gSavedSettings.getBOOL("BrowserWebSecurityDisabled");
			media_source->setWebSecurityDisabled(web_security_disabled || clean_browser);

			// collect setting indicates if local file access from file URLs is allowed from prefs and send to embedded browser
			bool file_access_from_file_urls = gSavedSettings.getBOOL("BrowserFileAccessFromFileUrls");
			media_source->setFileAccessFromFileUrlsEnabled(file_access_from_file_urls || clean_browser);

			// As of SL-15559 PDF files do not load in CEF v91 we enable plugins
			// but explicitly disable Flash (PDF support in CEF is now treated as a plugin)
			media_source->setPluginsEnabled(true);

			bool media_plugin_debugging_enabled = gSavedSettings.getBOOL("MediaPluginDebugging");
			media_source->enableMediaPluginDebugging( media_plugin_debugging_enabled  || clean_browser);

			// need to set agent string here before instance created
			media_source->setBrowserUserAgent(LLViewerMedia::getInstance()->getCurrentUserAgent());

            // configure and pass proxy setup based on debug settings that are 
            // configured by UI in prefs -> setup
            media_source->proxy_setup(gSavedSettings.getBOOL("BrowserProxyEnabled"), gSavedSettings.getString("BrowserProxyAddress"), gSavedSettings.getS32("BrowserProxyPort"));

			media_source->setTarget(target);

			const std::string plugin_dir = gDirUtilp->getLLPluginDir();
			if (media_source->init(launcher_name, plugin_dir, plugin_name, gSavedSettings.getBOOL("PluginAttachDebuggerToPlugins")))
			{
				return media_source;
			}
			else
			{
				LL_WARNS("Media") << "Failed to init plugin.  Destroying." << LL_ENDL;
				delete media_source;
			}
		}
	}
#if !LL_LINUX
	LL_WARNS_ONCE("Plugin") << "plugin initialization failed for mime type: " << media_type << LL_ENDL;
#endif

	if(gAgent.isInitialized())
	{
	    if (std::find(sMimeTypesFailed.begin(), sMimeTypesFailed.end(), media_type) == sMimeTypesFailed.end())
	    {
	        LLSD args;
	        args["MIME_TYPE"] = media_type;
	        LLNotificationsUtil::add("NoPlugin", args);
	        sMimeTypesFailed.push_back(media_type);
	    }
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::initializePlugin(const std::string& media_type)
{
	if(mMediaSource)
	{
		// Save the previous media source's last set size before destroying it.
		mMediaWidth = mMediaSource->getSetWidth();
		mMediaHeight = mMediaSource->getSetHeight();
		mZoomFactor = mMediaSource->getZoomFactor();
	}

	// Always delete the old media impl first.
	destroyMediaSource();

	// and unconditionally set the mime type
	mMimeType = media_type;

	if(mPriority == LLPluginClassMedia::PRIORITY_UNLOADED)
	{
		// This impl should not be loaded at this time.
		LL_DEBUGS("PluginPriority") << this << "Not loading (PRIORITY_UNLOADED)" << LL_ENDL;

		return false;
	}

	// If we got here, we want to ignore previous init failures.
	mMediaSourceFailed = false;

	// Save the MIME type that really caused the plugin to load
	mCurrentMimeType = mMimeType;

	LLPluginClassMedia* media_source = newSourceFromMediaType(mMimeType, this, mMediaWidth, mMediaHeight, mZoomFactor, mTarget, mCleanBrowser);

	if (media_source)
	{
		media_source->injectOpenIDCookie();
		media_source->setDisableTimeout(gSavedSettings.getBOOL("DebugPluginDisableTimeout"));
		media_source->setLoop(mMediaLoop);
		media_source->setAutoScale(mMediaAutoScale);
		media_source->setBrowserUserAgent(LLViewerMedia::getInstance()->getCurrentUserAgent());
		media_source->focus(mHasFocus);
		media_source->setBackgroundColor(mBackgroundColor);

		if(gSavedSettings.getBOOL("BrowserIgnoreSSLCertErrors"))
		{
			media_source->ignore_ssl_cert_errors(true);
		}

		// the correct way to deal with certs it to load ours from ca-bundle.crt and append them to the ones
		// Qt/WebKit loads from your system location.
		std::string ca_path = gDirUtilp->getCAFile();
		media_source->addCertificateFilePath( ca_path );

		if(mClearCache)
		{
			mClearCache = false;
			media_source->clear_cache();
		}

		mMediaSource.reset(media_source);
		mMediaSource->setDeleteOK(false) ;
		updateVolume();

		return true;
	}

	// Make sure the timer doesn't try re-initing this plugin repeatedly until something else changes.
	mMediaSourceFailed = true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::loadURI()
{
	if(mMediaSource)
	{
		// trim whitespace from front and back of URL - fixes EXT-5363
		LLStringUtil::trim( mMediaURL );

		// URI often comes unescaped
		std::string uri = LLURI::escapePathAndData(mMediaURL);
        {
            // Do not log the query parts
            LLURI u(uri);
            std::string sanitized_uri = (u.query().empty() ? uri : u.scheme() + "://" + u.authority() + u.path());
            LL_INFOS() << "Asking media source to load URI: " << sanitized_uri << LL_ENDL;
        }

		mMediaSource->loadURI( uri );

		// A non-zero mPreviousMediaTime means that either this media was previously unloaded by the priority code while playing/paused,
		// or a seek happened before the media loaded.  In either case, seek to the saved time.
		if(mPreviousMediaTime != 0.0f)
		{
			seek(mPreviousMediaTime);
		}

		if(mPreviousMediaState == MEDIA_PLAYING)
		{
			// This media was playing before this instance was unloaded.
			start();
		}
		else if(mPreviousMediaState == MEDIA_PAUSED)
		{
			// This media was paused before this instance was unloaded.
			pause();
		}
		else
		{
			// No relevant previous media play state -- if we're loading the URL, we want to start playing.
			start();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::executeJavaScript(const std::string& code)
{
	if (mMediaSource)
	{
		mMediaSource->executeJavaScript(code);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::setSize(int width, int height)
{
	mMediaWidth = width;
	mMediaHeight = height;
	if(mMediaSource)
	{
		mMediaSource->setSize(width, height);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::showNotification(LLNotificationPtr notify)
{
	mNotification = notify;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::hideNotification()
{
	mNotification.reset();
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::play()
{
	// If the media source isn't there, try to initialize it and load an URL.
	if(mMediaSource == NULL)
	{
	 	if(!initializeMedia(mMimeType))
		{
			// This may be the case where the plugin's priority is PRIORITY_UNLOADED
			return;
		}

		// Only do this if the media source was just loaded.
		loadURI();
	}

	// always start the media
	start();
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::stop()
{
	if(mMediaSource)
	{
		mMediaSource->stop();
		// destroyMediaSource();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::pause()
{
	if(mMediaSource)
	{
		mMediaSource->pause();
	}
	else
	{
		mPreviousMediaState = MEDIA_PAUSED;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::start()
{
	if(mMediaSource)
	{
		mMediaSource->start();
	}
	else
	{
		mPreviousMediaState = MEDIA_PLAYING;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::seek(F32 time)
{
	if(mMediaSource)
	{
		mMediaSource->seek(time);
	}
	else
	{
		// Save the seek time to be set when the media is loaded.
		mPreviousMediaTime = time;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::skipBack(F32 step_scale)
{
	if(mMediaSource)
	{
		if(mMediaSource->pluginSupportsMediaTime())
		{
			F64 back_step = mMediaSource->getCurrentTime() - (mMediaSource->getDuration()*step_scale);
			if(back_step < 0.0)
			{
				back_step = 0.0;
			}
			mMediaSource->seek(back_step);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::skipForward(F32 step_scale)
{
	if(mMediaSource)
	{
		if(mMediaSource->pluginSupportsMediaTime())
		{
			F64 forward_step = mMediaSource->getCurrentTime() + (mMediaSource->getDuration()*step_scale);
			if(forward_step > mMediaSource->getDuration())
			{
				forward_step = mMediaSource->getDuration();
			}
			mMediaSource->seek(forward_step);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::setVolume(F32 volume)
{
	mRequestedVolume = volume;
	updateVolume();
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::setMute(bool mute)
{
	if (mute)
	{
		mPreviousVolume = mRequestedVolume;
		setVolume(0.0);
	}
	else
	{
		setVolume(mPreviousVolume);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::updateVolume()
{
    LL_RECORD_BLOCK_TIME(FTM_MEDIA_UPDATE_VOLUME);
	if(mMediaSource)
	{
		// always scale the volume by the global media volume
		F32 volume = mRequestedVolume * LLViewerMedia::getInstance()->getVolume();

		if (mProximityCamera > 0)
		{
			if (mProximityCamera > gSavedSettings.getF32("MediaRollOffMax"))
			{
				volume = 0;
			}
			else if (mProximityCamera > gSavedSettings.getF32("MediaRollOffMin"))
			{
				// attenuated_volume = 1 / (roll_off_rate * (d - min))^2
				// the +1 is there so that for distance 0 the volume stays the same
				F64 adjusted_distance = mProximityCamera - gSavedSettings.getF32("MediaRollOffMin");
				F64 attenuation = 1.0 + (gSavedSettings.getF32("MediaRollOffRate") * adjusted_distance);
				attenuation = 1.0 / (attenuation * attenuation);
				// the attenuation multiplier should never be more than one since that would increase volume
				volume = volume * llmin(1.0, attenuation);
			}
		}

		if (sOnlyAudibleTextureID == LLUUID::null || sOnlyAudibleTextureID == mTextureId)
		{
			mMediaSource->setVolume(volume);
		}
		else
		{
			mMediaSource->setVolume(0.0f);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
F32 LLViewerMediaImpl::getVolume()
{
	return mRequestedVolume;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::focus(bool focus)
{
	mHasFocus = focus;

	if (mMediaSource)
	{
		// call focus just for the hell of it, even though this apopears to be a nop
		mMediaSource->focus(focus);
		if (focus)
		{
			// spoof a mouse click to *actually* pass focus
			// Don't do this anymore -- it actually clicks through now.
//			mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_DOWN, 1, 1, 0);
//			mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_UP, 1, 1, 0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::hasFocus() const
{
	// FIXME: This might be able to be a bit smarter by hooking into LLViewerMediaFocus, etc.
	return mHasFocus;
}

std::string LLViewerMediaImpl::getCurrentMediaURL()
{
	if(!mCurrentMediaURL.empty())
	{
		return mCurrentMediaURL;
	}

	return mMediaURL;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::clearCache()
{
	if(mMediaSource)
	{
		mMediaSource->clear_cache();
	}
	else
	{
		mClearCache = true;
	}
}


//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::setPageZoomFactor( double factor )
{
	if(mMediaSource && factor != mZoomFactor)
	{
		mZoomFactor = factor;
		mMediaSource->set_page_zoom_factor( factor );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseDown(S32 x, S32 y, MASK mask, S32 button)
{
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
//	LL_INFOS() << "mouse down (" << x << ", " << y << ")" << LL_ENDL;
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_DOWN, button, x, y, mask);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseUp(S32 x, S32 y, MASK mask, S32 button)
{
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
//	LL_INFOS() << "mouse up (" << x << ", " << y << ")" << LL_ENDL;
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_UP, button, x, y, mask);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseMove(S32 x, S32 y, MASK mask)
{
    scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
//	LL_INFOS() << "mouse move (" << x << ", " << y << ")" << LL_ENDL;
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_MOVE, 0, x, y, mask);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//static
void LLViewerMediaImpl::scaleTextureCoords(const LLVector2& texture_coords, S32 *x, S32 *y)
{
	F32 texture_x = texture_coords.mV[VX];
	F32 texture_y = texture_coords.mV[VY];

	// Deal with repeating textures by wrapping the coordinates into the range [0, 1.0)
	texture_x = fmodf(texture_x, 1.0f);
	if(texture_x < 0.0f)
		texture_x = 1.0 + texture_x;

	texture_y = fmodf(texture_y, 1.0f);
	if(texture_y < 0.0f)
		texture_y = 1.0 + texture_y;

	// scale x and y to texel units.
	*x = ll_round(texture_x * mMediaSource->getTextureWidth());
	*y = ll_round((1.0f - texture_y) * mMediaSource->getTextureHeight());

	// Adjust for the difference between the actual texture height and the amount of the texture in use.
	*y -= (mMediaSource->getTextureHeight() - mMediaSource->getHeight());
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseDown(const LLVector2& texture_coords, MASK mask, S32 button)
{
	if(mMediaSource)
	{
		S32 x, y;
		scaleTextureCoords(texture_coords, &x, &y);

		mouseDown(x, y, mask, button);
	}
}

void LLViewerMediaImpl::mouseUp(const LLVector2& texture_coords, MASK mask, S32 button)
{
	if(mMediaSource)
	{
		S32 x, y;
		scaleTextureCoords(texture_coords, &x, &y);

		mouseUp(x, y, mask, button);
	}
}

void LLViewerMediaImpl::mouseMove(const LLVector2& texture_coords, MASK mask)
{
	if(mMediaSource)
	{
		S32 x, y;
		scaleTextureCoords(texture_coords, &x, &y);

		mouseMove(x, y, mask);
	}
}

void LLViewerMediaImpl::mouseDoubleClick(const LLVector2& texture_coords, MASK mask)
{
    if (mMediaSource)
    {
        S32 x, y;
        scaleTextureCoords(texture_coords, &x, &y);

        mouseDoubleClick(x, y, mask);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseDoubleClick(S32 x, S32 y, MASK mask, S32 button)
{
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_DOUBLE_CLICK, button, x, y, mask);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::scrollWheel(const LLVector2& texture_coords, S32 scroll_x, S32 scroll_y, MASK mask)
{
    if (mMediaSource)
    {
        S32 x, y;
        scaleTextureCoords(texture_coords, &x, &y);

        scrollWheel(x, y, scroll_x, scroll_y, mask);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::scrollWheel(S32 x, S32 y, S32 scroll_x, S32 scroll_y, MASK mask)
{
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (mMediaSource)
	{
		mMediaSource->scrollEvent(x, y, scroll_x, scroll_y, mask);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::onMouseCaptureLost()
{
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_UP, 0, mLastMouseX, mLastMouseY, 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// NOTE: this is called when the mouse is released when we have capture.
	// Due to the way mouse coordinates are mapped to the object, we can't use the x and y coordinates that come in with the event.

	if(hasMouseCapture())
	{
		// Release the mouse -- this will also send a mouseup to the media
		gFocusMgr.setMouseCapture( nullptr );
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::updateJavascriptObject()
{
	static LLFrameTimer timer ;

	if ( mMediaSource )
	{
		// flag to expose this information to internal browser or not.
		bool enable = gSavedSettings.getBOOL("BrowserEnableJSObject");

		if(!enable)
		{
			return ; //no need to go further.
		}

		if(timer.getElapsedTimeF32() < 1.0f)
		{
			return ; //do not update more than once per second.
		}
		timer.reset() ;

		mMediaSource->jsEnableObject( enable );

		// these values are only menaingful after login so don't set them before
		bool logged_in = LLLoginInstance::getInstance()->authSuccess();
		if ( logged_in )
		{
			// current location within a region
			LLVector3 agent_pos = gAgent.getPositionAgent();
			double x = agent_pos.mV[ VX ];
			double y = agent_pos.mV[ VY ];
			double z = agent_pos.mV[ VZ ];
			mMediaSource->jsAgentLocationEvent( x, y, z );

			// current location within the grid
			LLVector3d agent_pos_global = gAgent.getLastPositionGlobal();
			double global_x = agent_pos_global.mdV[ VX ];
			double global_y = agent_pos_global.mdV[ VY ];
			double global_z = agent_pos_global.mdV[ VZ ];
			mMediaSource->jsAgentGlobalLocationEvent( global_x, global_y, global_z );

			// current agent orientation
			double rotation = atan2( gAgent.getAtAxis().mV[VX], gAgent.getAtAxis().mV[VY] );
			double angle = rotation * RAD_TO_DEG;
			if ( angle < 0.0f ) angle = 360.0f + angle;	// TODO: has to be a better way to get orientation!
			mMediaSource->jsAgentOrientationEvent( angle );

			// current region agent is in
			std::string region_name("");
			LLViewerRegion* region = gAgent.getRegion();
			if ( region )
			{
				region_name = region->getName();
			};
			mMediaSource->jsAgentRegionEvent( region_name );
		}

		// language code the viewer is set to
		mMediaSource->jsAgentLanguageEvent( LLUI::getLanguage() );

		// maturity setting the agent has selected
		if ( gAgent.prefersAdult() )
			mMediaSource->jsAgentMaturityEvent( "GMA" );	// Adult means see adult, mature and general content
		else
		if ( gAgent.prefersMature() )
			mMediaSource->jsAgentMaturityEvent( "GM" );	// Mature means see mature and general content
		else
		if ( gAgent.prefersPG() )
			mMediaSource->jsAgentMaturityEvent( "G" );	// PG means only see General content
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
const std::string& LLViewerMediaImpl::getName() const
{
	if (mMediaSource)
	{
		return mMediaSource->getMediaName();
	}

	return LLStringUtil::null;
};

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::navigateBack()
{
	if (mMediaSource)
	{
		mMediaSource->browse_back();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::navigateForward()
{
	if (mMediaSource)
	{
		mMediaSource->browse_forward();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::navigateReload()
{
	navigateTo(getCurrentMediaURL(), "", true, false);
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::navigateHome()
{
	bool rediscover_mimetype = mHomeMimeType.empty();
	navigateTo(mHomeURL, mHomeMimeType, rediscover_mimetype, false);
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::unload()
{
	// Unload the media impl and clear its state.
	destroyMediaSource();
	resetPreviousMediaState();
	mMediaURL.clear();
	mMimeType.clear();
	mCurrentMediaURL.clear();
	mCurrentMimeType.clear();
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::navigateTo(const std::string& url, const std::string& mime_type,  bool rediscover_type, bool server_request, bool clean_browser)
{
	cancelMimeTypeProbe();

	if(mMediaURL != url)
	{
		// Don't carry media play state across distinct URLs.
		resetPreviousMediaState();
	}

	// Always set the current URL and MIME type.
	mMediaURL = url;
	mMimeType = mime_type;
    mCleanBrowser = clean_browser;

	// Clear the current media URL, since it will no longer be correct.
	mCurrentMediaURL.clear();

	// if mime type discovery was requested, we'll need to do it when the media loads
	mNavigateRediscoverType = rediscover_type;

	// and if this was a server request, the navigate on load will also need to be one.
	mNavigateServerRequest = server_request;

	// An explicit navigate resets the "failed" flag.
	mMediaSourceFailed = false;

	if(mPriority == LLPluginClassMedia::PRIORITY_UNLOADED)
	{
		// Helpful to have media urls in log file. Shouldn't be spammy.
        {
            // Do not log the query parts
            LLURI u(url);
            std::string sanitized_url = (u.query().empty() ? url : u.scheme() + "://" + u.authority() + u.path());
            LL_INFOS() << "NOT LOADING media id= " << mTextureId << " url=" << sanitized_url << ", mime_type=" << mime_type << LL_ENDL;
        }

		// This impl should not be loaded at this time.
		LL_DEBUGS("PluginPriority") << this << "Not loading (PRIORITY_UNLOADED)" << LL_ENDL;

		return;
	}

	navigateInternal();
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::navigateInternal()
{
	// Helpful to have media urls in log file. Shouldn't be spammy.
    {
        // Do not log the query parts
        LLURI u(mMediaURL);
        std::string sanitized_url = (u.query().empty() ? mMediaURL : u.scheme() + "://" + u.authority() + u.path());
        LL_INFOS() << "media id= " << mTextureId << " url=" << sanitized_url << ", mime_type=" << mMimeType << LL_ENDL;
    }

	if(mNavigateSuspended)
	{
		LL_WARNS() << "Deferring navigate." << LL_ENDL;
		mNavigateSuspendedDeferred = true;
		return;
	}
	
    
    if (!mMimeProbe.expired())
	{
		LL_WARNS() << "MIME type probe already in progress -- bailing out." << LL_ENDL;
		return;
	}

	if(mNavigateServerRequest)
	{
		setNavState(MEDIANAVSTATE_SERVER_SENT);
	}
	else
	{
		setNavState(MEDIANAVSTATE_NONE);
	}

	// If the caller has specified a non-empty MIME type, look that up in our MIME types list.
	// If we have a plugin for that MIME type, use that instead of attempting auto-discovery.
	// This helps in supporting legacy media content where the server the media resides on returns a bogus MIME type
	// but the parcel owner has correctly set the MIME type in the parcel media settings.

	if(!mMimeType.empty() && (mMimeType != LLMIMETypes::getDefaultMimeType()))
	{
		std::string plugin_basename = LLMIMETypes::implType(mMimeType);
		if(!plugin_basename.empty())
		{
			// We have a plugin for this mime type
			mNavigateRediscoverType = false;
		}
	}

	if(mNavigateRediscoverType)
	{

		LLURI uri(mMediaURL);
		std::string scheme = uri.scheme();

		if(scheme.empty() || "http" == scheme || "https" == scheme)
		{
            LLCoros::instance().launch("LLViewerMediaImpl::mimeDiscoveryCoro",
                boost::bind(&LLViewerMediaImpl::mimeDiscoveryCoro, this, mMediaURL));
		}
		else if("data" == scheme || "file" == scheme || "about" == scheme)
		{
			// FIXME: figure out how to really discover the type for these schemes
			// We use "data" internally for a text/html url for loading the login screen
			if(initializeMedia(HTTP_CONTENT_TEXT_HTML))
			{
				loadURI();
			}
		}
		else
		{
			// This catches 'rtsp://' urls
			if(initializeMedia(scheme))
			{
				loadURI();
			}
		}
	}
	else if(initializeMedia(mMimeType))
	{
		loadURI();
	}
	else
	{
		LL_WARNS("Media") << "Couldn't navigate to: " << mMediaURL << " as there is no media type for: " << mMimeType << LL_ENDL;
	}
}

void LLViewerMediaImpl::mimeDiscoveryCoro(std::string url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("mimeDiscoveryCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    LLCore::HttpHeaders::ptr_t httpHeaders(new LLCore::HttpHeaders);

    // Increment our refcount so that we do not go away while the coroutine is active.
    this->ref();

    mMimeProbe = httpAdapter;

    httpOpts->setFollowRedirects(true);
    httpOpts->setHeadersOnly(true);

    httpHeaders->append(HTTP_OUT_HEADER_ACCEPT, "*/*");
    httpHeaders->append(HTTP_OUT_HEADER_COOKIE, "");

    LLSD result = httpAdapter->getRawAndSuspend(httpRequest, url, httpOpts, httpHeaders);

    mMimeProbe.reset();

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS() << "Error retrieving media headers." << LL_ENDL;
    }

    if (this->getNumRefs() > 1)
    {   // if there is only a single ref count outstanding it will be the one we took out above...
        // we can skip the rest of this routine

        LLSD resultHeaders = httpResults[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS];

        const std::string& mediaType = resultHeaders[HTTP_IN_HEADER_CONTENT_TYPE].asStringRef();

        std::string::size_type idx1 = mediaType.find_first_of(";");
        std::string mimeType = mediaType.substr(0, idx1);

        // We now no longer need to check the error code returned from the probe.
        // If we have a mime type, use it.  If not, default to the web plugin and let it handle error reporting.
        // The probe was successful.
        if (mimeType.empty())
        {
            // Some sites don't return any content-type header at all.
            // Treat an empty mime type as text/html.
            mimeType = HTTP_CONTENT_TEXT_HTML;
        }

        LL_DEBUGS() << "Media type \"" << mediaType << "\", mime type is \"" << mimeType << "\"" << LL_ENDL;

        // the call to initializeMedia may disconnect the responder, which will clear mMediaImpl.
        // Make a local copy so we can call loadURI() afterwards.

        if (!mimeType.empty())
        {
            if (initializeMedia(mimeType))
            {
                loadURI();
            }
        }

    }
    else
    {
        LL_WARNS() << "LLViewerMediaImpl to be released." << LL_ENDL;
    }

    this->unref();
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::navigateStop()
{
	if(mMediaSource)
	{
		mMediaSource->browse_stop();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::handleKeyHere(KEY key, MASK mask)
{
	bool result = false;

	if (mMediaSource)
	{
		// FIXME: THIS IS SO WRONG.
		// Menu keys should be handled by the menu system and not passed to UI elements, but this is how LLTextEditor and LLLineEditor do it...
		if (MASK_CONTROL & mask && key != KEY_LEFT && key != KEY_RIGHT && key != KEY_HOME && key != KEY_END)
		{
			result = true;
		}

		if (!result)
		{
            LLSD native_key_data = gViewerWindow->getWindow()->getNativeKeyData();
			result = mMediaSource->keyEvent(LLPluginClassMedia::KEY_EVENT_DOWN, key, mask, native_key_data);
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::handleKeyUpHere(KEY key, MASK mask)
{
	bool result = false;

	if (mMediaSource)
	{
		// FIXME: THIS IS SO WRONG.
		// Menu keys should be handled by the menu system and not passed to UI elements, but this is how LLTextEditor and LLLineEditor do it...
		if (MASK_CONTROL & mask && key != KEY_LEFT && key != KEY_RIGHT && key != KEY_HOME && key != KEY_END)
		{
			result = true;
		}

		if (!result)
		{
			LLSD native_key_data = gViewerWindow->getWindow()->getNativeKeyData();
			result = mMediaSource->keyEvent(LLPluginClassMedia::KEY_EVENT_UP, key, mask, native_key_data);
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::handleUnicodeCharHere(llwchar uni_char)
{
	bool result = false;

	if (mMediaSource)
	{
		// only accept 'printable' characters, sigh...
		if (uni_char >= 32 // discard 'control' characters
			&& uni_char != 127) // SDL thinks this is 'delete' - yuck.
		{
			LLSD native_key_data = gViewerWindow->getWindow()->getNativeKeyData();

			mMediaSource->textInput(wstring_to_utf8str(LLWString(1, uni_char)), gKeyboard->currentMask(false), native_key_data);
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::canNavigateForward()
{
	bool result = false;
	if (mMediaSource)
	{
		result = mMediaSource->getHistoryForwardAvailable();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::canNavigateBack()
{
	bool result = false;
	if (mMediaSource)
	{
		result = mMediaSource->getHistoryBackAvailable();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
static LLTrace::BlockTimerStatHandle FTM_MEDIA_DO_UPDATE("Do Update");
static LLTrace::BlockTimerStatHandle FTM_MEDIA_GET_DATA("Get Data");
static LLTrace::BlockTimerStatHandle FTM_MEDIA_SET_SUBIMAGE("Set Subimage");


void LLViewerMediaImpl::update()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_MEDIA; //LL_RECORD_BLOCK_TIME(FTM_MEDIA_DO_UPDATE);
    if(mMediaSource == NULL)
    {
        if(mPriority == LLPluginClassMedia::PRIORITY_UNLOADED)
        {
            // This media source should not be loaded.
        }
        else if(mPriority <= LLPluginClassMedia::PRIORITY_SLIDESHOW)
        {
            // Don't load new instances that are at PRIORITY_SLIDESHOW or below.  They're just kept around to preserve state.
        }
        else if (!mMimeProbe.expired())
        {
            // this media source is doing a MIME type probe -- don't try loading it again.
        }
        else
        {
            // This media may need to be loaded.
            if(sMediaCreateTimer.hasExpired())
            {
                LL_DEBUGS("PluginPriority") << this << ": creating media based on timer expiration" << LL_ENDL;
                createMediaSource();
                sMediaCreateTimer.setTimerExpirySec(LLVIEWERMEDIA_CREATE_DELAY);
            }
            else
            {
                LL_DEBUGS("PluginPriority") << this << ": NOT creating media (waiting on timer)" << LL_ENDL;
            }
        }
    }
    else
    {
        updateVolume();

        // TODO: this is updated every frame - is this bad?
        // Removing this as part of the post viewer64 media update
        // Removed as not implemented in CEF embedded browser
        // See MAINT-8194 for a more fuller description
        // updateJavascriptObject();
    }


    if(mMediaSource == NULL)
    {
        return;
    }

    // Make sure a navigate doesn't happen during the idle -- it can cause mMediaSource to get destroyed, which can cause a crash.
    setNavigateSuspended(true);

    mMediaSource->idle();

    setNavigateSuspended(false);

    if(mMediaSource == NULL)
    {
        return;
    }

    if(mMediaSource->isPluginExited())
    {
        resetPreviousMediaState();
        destroyMediaSource();
        return;
    }

    if(!mMediaSource->textureValid())
    {
        return;
    }

    if(mSuspendUpdates || !mVisible)
    {
        return;
    }

    
    LLViewerMediaTexture* media_tex;
    U8* data;
    S32 data_width;
    S32 data_height;
    S32 x_pos;
    S32 y_pos;
    S32 width;
    S32 height;

    if (preMediaTexUpdate(media_tex, data, data_width, data_height, x_pos, y_pos, width, height))
    {
        // Push update to worker thread
        auto main_queue = LLImageGLThread::sEnabledMedia ? mMainQueue.lock() : nullptr;
        if (main_queue)
        {
            mTextureUpdatePending = true;
            ref();  // protect texture from deletion while active on bg queue
            media_tex->ref();
            main_queue->postTo(
                mTexUpdateQueue, // Worker thread queue
                [=]() // work done on update worker thread
                {
#if LL_IMAGEGL_THREAD_CHECK
                    media_tex->getGLTexture()->mActiveThread = LLThread::currentID();
#endif
                    doMediaTexUpdate(media_tex, data, data_width, data_height, x_pos, y_pos, width, height, true);
                },
                [=]() // callback to main thread
                {
#if LL_IMAGEGL_THREAD_CHECK
                    media_tex->getGLTexture()->mActiveThread = LLThread::currentID();
#endif
                    mTextureUpdatePending = false;
                    media_tex->unref();
                    unref();
                });
        }
        else
        {
            doMediaTexUpdate(media_tex, data, data_width, data_height, x_pos, y_pos, width, height, false); // otherwise, update on main thread
        }
    }
}

bool LLViewerMediaImpl::preMediaTexUpdate(LLViewerMediaTexture*& media_tex, U8*& data, S32& data_width, S32& data_height, S32& x_pos, S32& y_pos, S32& width, S32& height)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_MEDIA;

    bool retval = false;

    if (!mTextureUpdatePending)
    {
        media_tex = updateMediaImage();

        if (media_tex && mMediaSource)
        {
            LLRect dirty_rect;
            S32 media_width = mMediaSource->getTextureWidth();
            S32 media_height = mMediaSource->getTextureHeight();
            //S32 media_depth = mMediaSource->getTextureDepth();

            // Since we're updating this texture, we know it's playing.  Tell the texture to do its replacement magic so it gets rendered.
            media_tex->setPlaying(true);

            if (mMediaSource->getDirty(&dirty_rect))
            {
                // Constrain the dirty rect to be inside the texture
                x_pos = llmax(dirty_rect.mLeft, 0);
                y_pos = llmax(dirty_rect.mBottom, 0);
                width = llmin(dirty_rect.mRight, media_width) - x_pos;
                height = llmin(dirty_rect.mTop, media_height) - y_pos;

                if (width > 0 && height > 0)
                {
                    data = mMediaSource->getBitsData();
                    data_width = mMediaSource->getWidth();
                    data_height = mMediaSource->getHeight();

                    if (data != NULL)
                    {
                        // data is ready to be copied to GL
                        retval = true;
                    }
                }

                mMediaSource->resetDirty();
            }
        }
    }

    return retval;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::doMediaTexUpdate(LLViewerMediaTexture* media_tex, U8* data, S32 data_width, S32 data_height, S32 x_pos, S32 y_pos, S32 width, S32 height, bool sync)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_MEDIA;
    LLMutexLock lock(&mLock); // don't allow media source tear-down during update

    // wrap "data" in an LLImageRaw but do NOT make a copy
    LLPointer<LLImageRaw> raw = new LLImageRaw(data, media_tex->getWidth(), media_tex->getHeight(), media_tex->getComponents(), true);

    // *NOTE: Recreating the GL texture each media update may seem wasteful
    // (note the texture creation in preMediaTexUpdate), however, it apparently
    // prevents GL calls from blocking, due to poor bookkeeping of state of
    // updated textures by the OpenGL implementation. (Windows 10/Nvidia)
    // -Cosmic,2023-04-04
    // Allocate GL texture based on LLImageRaw but do NOT copy to GL
    LLGLuint tex_name = 0;
    media_tex->createGLTexture(0, raw, 0, true, LLGLTexture::OTHER, true, &tex_name);

    // copy just the subimage covered by the image raw to GL
    media_tex->setSubImage(data, data_width, data_height, x_pos, y_pos, width, height, tex_name);
    
    if (sync)
    {
        media_tex->getGLTexture()->syncToMainThread(tex_name);
    }
    else
    {
        media_tex->getGLTexture()->syncTexName(tex_name);
    }
    
    // release the data pointer before freeing raw so LLImageRaw destructor doesn't
    // free memory at data pointer
    raw->releaseData();
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::updateImagesMediaStreams()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
LLViewerMediaTexture* LLViewerMediaImpl::updateMediaImage()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_MEDIA;
    llassert(!gCubeSnapshot);
    if (!mMediaSource)
    {
        return nullptr; // not ready for updating
    }

    //llassert(!mTextureId.isNull());
    // *TODO: Consider enabling mipmaps (they have been disabled for a long time). Likely has a significant performance impact for tiled/high texture repeat media. Mip generation in a shader may also be an option if necessary.
    LLViewerMediaTexture* media_tex = LLViewerTextureManager::getMediaTexture( mTextureId, USE_MIPMAPS );
 
    if ( mNeedsNewTexture
        || (media_tex->getWidth() != mMediaSource->getTextureWidth())
        || (media_tex->getHeight() != mMediaSource->getTextureHeight())
        || (mTextureUsedWidth != mMediaSource->getWidth())
        || (mTextureUsedHeight != mMediaSource->getHeight())
        )
    {
        LL_DEBUGS("Media") << "initializing media placeholder" << LL_ENDL;
        LL_DEBUGS("Media") << "movie image id " << mTextureId << LL_ENDL;

        int texture_width = mMediaSource->getTextureWidth();
        int texture_height = mMediaSource->getTextureHeight();
        int texture_depth = mMediaSource->getTextureDepth();

        // MEDIAOPT: check to see if size actually changed before doing work
        media_tex->destroyGLTexture();

        // MEDIAOPT: seems insane that we actually have to make an imageraw then
        // immediately discard it
        LLPointer<LLImageRaw> raw = new LLImageRaw(texture_width, texture_height, texture_depth);
        // Clear the texture to the background color, ignoring alpha.
        // convert background color channels from [0.0, 1.0] to [0, 255];
        raw->clear(int(mBackgroundColor.mV[VX] * 255.0f), int(mBackgroundColor.mV[VY] * 255.0f), int(mBackgroundColor.mV[VZ] * 255.0f), 0xff);

        // ask media source for correct GL image format constants
        media_tex->setExplicitFormat(mMediaSource->getTextureFormatInternal(),
            mMediaSource->getTextureFormatPrimary(),
            mMediaSource->getTextureFormatType(),
            mMediaSource->getTextureFormatSwapBytes());

        int discard_level = 0;
        media_tex->createGLTexture(discard_level, raw);

        // MEDIAOPT: set this dynamically on play/stop
        // FIXME
//		media_tex->mIsMediaTexture = true;
        mNeedsNewTexture = false;

        // If the amount of the texture being drawn by the media goes down in either width or height,
        // recreate the texture to avoid leaving parts of the old image behind.
        mTextureUsedWidth = mMediaSource->getWidth();
        mTextureUsedHeight = mMediaSource->getHeight();
    }
    return media_tex;
}


//////////////////////////////////////////////////////////////////////////////////////////
LLUUID LLViewerMediaImpl::getMediaTextureID() const
{
	return mTextureId;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::setVisible(bool visible)
{
	mVisible = visible;

	if(mVisible)
	{
		if(mMediaSource && mMediaSource->isPluginExited())
		{
			destroyMediaSource();
		}

		if(!mMediaSource)
		{
			createMediaSource();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseCapture()
{
	gFocusMgr.setMouseCapture(this);
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::scaleMouse(S32 *mouse_x, S32 *mouse_y)
{
#if 0
	S32 media_width, media_height;
	S32 texture_width, texture_height;
	getMediaSize( &media_width, &media_height );
	getTextureSize( &texture_width, &texture_height );
	S32 y_delta = texture_height - media_height;

	*mouse_y -= y_delta;
#endif
}



//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::isMediaTimeBased()
{
	bool result = false;

	if(mMediaSource)
	{
		result = mMediaSource->pluginSupportsMediaTime();
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::isMediaPlaying()
{
	bool result = false;

	if(mMediaSource)
	{
		EMediaStatus status = mMediaSource->getStatus();
		if(status == MEDIA_PLAYING || status == MEDIA_LOADING)
			result = true;
	}

	return result;
}
//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::isMediaPaused()
{
	bool result = false;

	if(mMediaSource)
	{
		if(mMediaSource->getStatus() == MEDIA_PAUSED)
			result = true;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaImpl::hasMedia() const
{
	return mMediaSource != NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
void LLViewerMediaImpl::resetPreviousMediaState()
{
	mPreviousMediaState = MEDIA_NONE;
	mPreviousMediaTime = 0.0f;
}


//////////////////////////////////////////////////////////////////////////////////////////
//
void LLViewerMediaImpl::setDisabled(bool disabled, bool forcePlayOnEnable)
{
	if(mIsDisabled != disabled)
	{
		// Only do this on actual state transitions.
		mIsDisabled = disabled;

		if(mIsDisabled)
		{
			// We just disabled this media.  Clear all state.
			unload();
		}
		else
		{
			// We just (re)enabled this media.  Do a navigate if auto-play is in order.
			if(isAutoPlayable() || forcePlayOnEnable)
			{
				navigateTo(mMediaEntryURL, "", true, true);
			}
		}

	}
};

//////////////////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaImpl::isForcedUnloaded() const
{
	if(mIsMuted || mMediaSourceFailed || mIsDisabled)
	{
		return true;
	}

	// If this media's class is not supposed to be shown, unload
	if (!shouldShowBasedOnClass() || isObscured())
	{
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaImpl::isPlayable() const
{
	if(isForcedUnloaded())
	{
		// All of the forced-unloaded criteria also imply not playable.
		return false;
	}

	if(hasMedia())
	{
		// Anything that's already playing is, by definition, playable.
		return true;
	}

	if(!mMediaURL.empty())
	{
		// If something has navigated the instance, it's ready to be played.
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::handleMediaEvent(LLPluginClassMedia* plugin, LLPluginClassMediaOwner::EMediaEvent event)
{
	bool pass_through = true;
	switch(event)
	{
		case MEDIA_EVENT_CLICK_LINK_NOFOLLOW:
		{
			LL_DEBUGS("Media") << "MEDIA_EVENT_CLICK_LINK_NOFOLLOW, uri is: " << plugin->getClickURL() << LL_ENDL;
			std::string url = plugin->getClickURL();
			std::string nav_type = plugin->getClickNavType();
			LLURLDispatcher::dispatch(url, nav_type, NULL, mTrustedBrowser);
		}
		break;
		case MEDIA_EVENT_CLICK_LINK_HREF:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CLICK_LINK_HREF, target is \"" << plugin->getClickTarget() << "\", uri is " << plugin->getClickURL() << LL_ENDL;
		};
		break;
		case MEDIA_EVENT_PLUGIN_FAILED_LAUNCH:
		{
			// The plugin failed to load properly.  Make sure the timer doesn't retry.
			// TODO: maybe mark this plugin as not loadable somehow?
			mMediaSourceFailed = true;

			// Reset the last known state of the media to defaults.
			resetPreviousMediaState();

			// TODO: may want a different message for this case?
			LLSD args;
			args["PLUGIN"] = LLMIMETypes::implType(mCurrentMimeType);
			LLNotificationsUtil::add("MediaPluginFailed", args);
		}
		break;

		case MEDIA_EVENT_PLUGIN_FAILED:
		{
			// The plugin crashed.
			mMediaSourceFailed = true;

			// Reset the last known state of the media to defaults.
			resetPreviousMediaState();

			LLSD args;
			args["PLUGIN"] = LLMIMETypes::implType(mCurrentMimeType);
			// SJB: This is getting called every frame if the plugin fails to load, continuously respawining the alert!
			//LLNotificationsUtil::add("MediaPluginFailed", args);
		}
		break;

		case MEDIA_EVENT_CURSOR_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CURSOR_CHANGED, new cursor is " << plugin->getCursorName() << LL_ENDL;

			std::string cursor = plugin->getCursorName();
			mLastSetCursor = getCursorFromString(cursor);
		}
		break;

		case LLViewerMediaObserver::MEDIA_EVENT_FILE_DOWNLOAD:
		{
			LL_DEBUGS("Media") << "Media event - file download requested - filename is " << plugin->getFileDownloadFilename() << LL_ENDL;
		}
		break;

		case LLViewerMediaObserver::MEDIA_EVENT_NAVIGATE_BEGIN:
		{
			LL_DEBUGS("Media") << "MEDIA_EVENT_NAVIGATE_BEGIN, uri is: " << plugin->getNavigateURI() << LL_ENDL;
			hideNotification();

			if(getNavState() == MEDIANAVSTATE_SERVER_SENT)
			{
				setNavState(MEDIANAVSTATE_SERVER_BEGUN);
			}
			else
			{
				setNavState(MEDIANAVSTATE_BEGUN);
			}
		}
		break;

		case LLViewerMediaObserver::MEDIA_EVENT_NAVIGATE_COMPLETE:
		{
			LL_DEBUGS("Media") << "MEDIA_EVENT_NAVIGATE_COMPLETE, uri is: " << plugin->getNavigateURI() << LL_ENDL;

			std::string url = plugin->getNavigateURI();
			if(getNavState() == MEDIANAVSTATE_BEGUN)
			{
				if(mCurrentMediaURL == url)
				{
					// This is a navigate that takes us to the same url as the previous navigate.
					setNavState(MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED_SPURIOUS);
				}
				else
				{
					mCurrentMediaURL = url;
					setNavState(MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED);
				}
			}
			else if(getNavState() == MEDIANAVSTATE_SERVER_BEGUN)
			{
				mCurrentMediaURL = url;
				setNavState(MEDIANAVSTATE_SERVER_COMPLETE_BEFORE_LOCATION_CHANGED);
			}
			else
			{
				// all other cases need to leave the state alone.
			}
		}
		break;

		case LLViewerMediaObserver::MEDIA_EVENT_LOCATION_CHANGED:
		{
			LL_DEBUGS("Media") << "MEDIA_EVENT_LOCATION_CHANGED, uri is: " << plugin->getLocation() << LL_ENDL;

			std::string url = plugin->getLocation();

			if(getNavState() == MEDIANAVSTATE_BEGUN)
			{
				if(mCurrentMediaURL == url)
				{
					// This is a navigate that takes us to the same url as the previous navigate.
					setNavState(MEDIANAVSTATE_FIRST_LOCATION_CHANGED_SPURIOUS);
				}
				else
				{
					mCurrentMediaURL = url;
					setNavState(MEDIANAVSTATE_FIRST_LOCATION_CHANGED);
				}
			}
			else if(getNavState() == MEDIANAVSTATE_SERVER_BEGUN)
			{
				mCurrentMediaURL = url;
				setNavState(MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED);
			}
			else
			{
				bool internal_nav = false;
				if (url != mCurrentMediaURL)
				{
					// Check if it is internal navigation
					// Note: Not sure if we should detect internal navigations as 'address change',
					// but they are not redirects and do not cause NAVIGATE_BEGIN (also see SL-1005)
					size_t pos = url.find("#");
					if (pos != std::string::npos)
					{
						// assume that new link always have '#', so this is either
						// transfer from 'link#1' to 'link#2' or from link to 'link#2'
						// filter out cases like 'redirect?link'
						std::string base_url = url.substr(0, pos);
						pos = mCurrentMediaURL.find(base_url);
						if (pos == 0)
						{
							// base link hasn't changed
							internal_nav = true;
						}
					}
				}

				if (internal_nav)
				{
					// Internal navigation by '#'
					mCurrentMediaURL = url;
					setNavState(MEDIANAVSTATE_FIRST_LOCATION_CHANGED);
				}
				else
				{
					// Don't track redirects.
					setNavState(MEDIANAVSTATE_NONE);
				}
			}
		}
		break;

		case LLViewerMediaObserver::MEDIA_EVENT_PICK_FILE_REQUEST:
		{
			LL_DEBUGS("Media") << "Media event - file pick requested." <<  LL_ENDL;

			init_threaded_picker_load_dialog(plugin, LLFilePicker::FFLOAD_ALL, plugin->getIsMultipleFilePick());
		}
		break;

		case LLViewerMediaObserver::MEDIA_EVENT_AUTH_REQUEST:
		{
			LLNotification::Params auth_request_params;
			auth_request_params.name = "AuthRequest";

			// pass in host name and realm for site (may be zero length but will always exist)
			LLSD args;
			LLURL raw_url( plugin->getAuthURL().c_str() );
			args["HOST_NAME"] = raw_url.getAuthority();
			args["REALM"] = plugin->getAuthRealm();
			auth_request_params.substitutions = args;

			auth_request_params.payload = LLSD().with("media_id", mTextureId);
			auth_request_params.functor.function = boost::bind(&LLViewerMedia::authSubmitCallback, _1, _2);
			LLNotifications::instance().add(auth_request_params);
		};
		break;

		case LLViewerMediaObserver::MEDIA_EVENT_CLOSE_REQUEST:
		{
			std::string uuid = plugin->getClickUUID();

			LL_INFOS() << "MEDIA_EVENT_CLOSE_REQUEST for uuid " << uuid << LL_ENDL;

			if(uuid.empty())
			{
				// This close request is directed at this instance, let it fall through.
			}
			else
			{
				// This close request is directed at another instance
				pass_through = false;
				LLFloaterWebContent::closeRequest(uuid);
			}
		}
		break;

		case LLViewerMediaObserver::MEDIA_EVENT_GEOMETRY_CHANGE:
		{
			std::string uuid = plugin->getClickUUID();

			LL_INFOS() << "MEDIA_EVENT_GEOMETRY_CHANGE for uuid " << uuid << LL_ENDL;

			if(uuid.empty())
			{
				// This geometry change request is directed at this instance, let it fall through.
			}
			else
			{
				// This request is directed at another instance
				pass_through = false;
				LLFloaterWebContent::geometryChanged(uuid, plugin->getGeometryX(), plugin->getGeometryY(), plugin->getGeometryWidth(), plugin->getGeometryHeight());
			}
		}
		break;

		default:
		break;
	}

	if(pass_through)
	{
		// Just chain the event to observers.
		emitEvent(plugin, event);
	}
}

////////////////////////////////////////////////////////////////////////////////
// virtual
void
LLViewerMediaImpl::cut()
{
	if (mMediaSource)
		mMediaSource->cut();
}

////////////////////////////////////////////////////////////////////////////////
// virtual
bool
LLViewerMediaImpl::canCut() const
{
	if (mMediaSource)
		return mMediaSource->canCut();
	else
		return false;
}

////////////////////////////////////////////////////////////////////////////////
// virtual
void
LLViewerMediaImpl::copy()
{
	if (mMediaSource)
		mMediaSource->copy();
}

////////////////////////////////////////////////////////////////////////////////
// virtual
bool
LLViewerMediaImpl::canCopy() const
{
	if (mMediaSource)
		return mMediaSource->canCopy();
	else
		return false;
}

////////////////////////////////////////////////////////////////////////////////
// virtual
void
LLViewerMediaImpl::paste()
{
	if (mMediaSource)
		mMediaSource->paste();
}

////////////////////////////////////////////////////////////////////////////////
// virtual
bool
LLViewerMediaImpl::canPaste() const
{
	if (mMediaSource)
		return mMediaSource->canPaste();
	else
		return false;
}

void LLViewerMediaImpl::setUpdated(bool updated)
{
	mIsUpdated = updated ;
}

bool LLViewerMediaImpl::isUpdated()
{
	return mIsUpdated ;
}

static LLTrace::BlockTimerStatHandle FTM_MEDIA_CALCULATE_INTEREST("Calculate Interest");

void LLViewerMediaImpl::calculateInterest()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_MEDIA; //LL_RECORD_BLOCK_TIME(FTM_MEDIA_CALCULATE_INTEREST);
	LLViewerMediaTexture* texture = LLViewerTextureManager::findMediaTexture( mTextureId );

    llassert(!gCubeSnapshot);

	if(texture != NULL)
	{
		mInterest = texture->getMaxVirtualSize();
	}
	else
	{
		// This will be a relatively common case now, since it will always be true for unloaded media.
		mInterest = 0.0f;
	}

	// Calculate distance from the avatar, for use in the proximity calculation.
	mProximityDistance = 0.0f;
	mProximityCamera = 0.0f;
	if(!mObjectList.empty())
	{
		// Just use the first object in the list.  We could go through the list and find the closest object, but this should work well enough.
		std::list< LLVOVolume* >::iterator iter = mObjectList.begin() ;
		LLVOVolume* objp = *iter ;
		llassert_always(objp != NULL) ;

		// The distance calculation is invalid for HUD attachments -- leave both mProximityDistance and mProximityCamera at 0 for them.
		if(!objp->isHUDAttachment())
		{
			LLVector3d obj_global = objp->getPositionGlobal() ;
			LLVector3d agent_global = gAgent.getPositionGlobal() ;
			LLVector3d global_delta = agent_global - obj_global ;
			mProximityDistance = global_delta.magVecSquared();  // use distance-squared because it's cheaper and sorts the same.

            static LLUICachedControl<S32> mEarLocation("MediaSoundsEarLocation", 0);
            LLVector3d ear_position;
            switch(mEarLocation)
            {
            case 0:
            default:
                ear_position = gAgentCamera.getCameraPositionGlobal();
                break;

            case 1:
                ear_position = agent_global;
                break;
            }
            LLVector3d camera_delta = ear_position - obj_global;
			mProximityCamera = camera_delta.magVec();
		}
	}

	if(mNeedsMuteCheck)
	{
		// Check all objects this instance is associated with, and those objects' owners, against the mute list
		mIsMuted = false;

		std::list< LLVOVolume* >::iterator iter = mObjectList.begin() ;
		for(; iter != mObjectList.end() ; ++iter)
		{
			LLVOVolume *obj = *iter;
			llassert(obj);
			if (!obj) continue;
			if(LLMuteList::getInstance() &&
			   LLMuteList::getInstance()->isMuted(obj->getID()))
			{
				mIsMuted = true;
			}
			else
			{
				// We won't have full permissions data for all objects.  Attempt to mute objects when we can tell their owners are muted.
				if (LLSelectMgr::getInstance())
				{
					LLPermissions* obj_perm = LLSelectMgr::getInstance()->findObjectPermissions(obj);
					if(obj_perm)
					{
						if(LLMuteList::getInstance() &&
						   LLMuteList::getInstance()->isMuted(obj_perm->getOwner()))
							mIsMuted = true;
					}
				}
			}
		}

		mNeedsMuteCheck = false;
	}
}

F64 LLViewerMediaImpl::getApproximateTextureInterest()
{
	F64 result = 0.0f;

	if(mMediaSource)
	{
		result = mMediaSource->getFullWidth();
		result *= mMediaSource->getFullHeight();
	}
	else
	{
		// No media source is loaded -- all we have to go on is the texture size that has been set on the impl, if any.
		result = mMediaWidth;
		result *= mMediaHeight;
	}

	return result;
}

void LLViewerMediaImpl::setUsedInUI(bool used_in_ui)
{
	mUsedInUI = used_in_ui;

	// HACK: Force elements used in UI to load right away.
	// This fixes some issues where UI code that uses the browser instance doesn't expect it to be unloaded.
	if(mUsedInUI && (mPriority == LLPluginClassMedia::PRIORITY_UNLOADED))
	{
		if(getVisible())
		{
			setPriority(LLPluginClassMedia::PRIORITY_NORMAL);
		}
		else
		{
			setPriority(LLPluginClassMedia::PRIORITY_HIDDEN);
		}

		createMediaSource();
	}
};

void LLViewerMediaImpl::setBackgroundColor(LLColor4 color)
{
	mBackgroundColor = color;

	if(mMediaSource)
	{
		mMediaSource->setBackgroundColor(mBackgroundColor);
	}
};

F64 LLViewerMediaImpl::getCPUUsage() const
{
	F64 result = 0.0f;

	if(mMediaSource)
	{
		result = mMediaSource->getCPUUsage();
	}

	return result;
}

void LLViewerMediaImpl::setPriority(LLPluginClassMedia::EPriority priority)
{
	if(mPriority != priority)
	{
		LL_DEBUGS("PluginPriority")
			<< "changing priority of media id " << mTextureId
			<< " from " << LLPluginClassMedia::priorityToString(mPriority)
			<< " to " << LLPluginClassMedia::priorityToString(priority)
			<< LL_ENDL;
	}

	mPriority = priority;

	if(priority == LLPluginClassMedia::PRIORITY_UNLOADED)
	{
		if(mMediaSource)
		{
			// Need to unload the media source

			// First, save off previous media state
			mPreviousMediaState = mMediaSource->getStatus();
			mPreviousMediaTime = mMediaSource->getCurrentTime();

			destroyMediaSource();
		}
	}

	if(mMediaSource)
	{
		mMediaSource->setPriority(mPriority);
	}

	// NOTE: loading (or reloading) media sources whose priority has risen above PRIORITY_UNLOADED is done in update().
}

void LLViewerMediaImpl::setLowPrioritySizeLimit(int size)
{
	if(mMediaSource)
	{
		mMediaSource->setLowPrioritySizeLimit(size);
	}
}

void LLViewerMediaImpl::setNavState(EMediaNavState state)
{
	mMediaNavState = state;

	switch (state)
	{
		case MEDIANAVSTATE_NONE: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_NONE" << LL_ENDL; break;
		case MEDIANAVSTATE_BEGUN: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_BEGUN" << LL_ENDL; break;
		case MEDIANAVSTATE_FIRST_LOCATION_CHANGED: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_FIRST_LOCATION_CHANGED" << LL_ENDL; break;
		case MEDIANAVSTATE_FIRST_LOCATION_CHANGED_SPURIOUS: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_FIRST_LOCATION_CHANGED_SPURIOUS" << LL_ENDL; break;
		case MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED" << LL_ENDL; break;
		case MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED_SPURIOUS: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED_SPURIOUS" << LL_ENDL; break;
		case MEDIANAVSTATE_SERVER_SENT: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_SERVER_SENT" << LL_ENDL; break;
		case MEDIANAVSTATE_SERVER_BEGUN: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_SERVER_BEGUN" << LL_ENDL; break;
		case MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED" << LL_ENDL; break;
		case MEDIANAVSTATE_SERVER_COMPLETE_BEFORE_LOCATION_CHANGED: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_SERVER_COMPLETE_BEFORE_LOCATION_CHANGED" << LL_ENDL; break;
	}
}

void LLViewerMediaImpl::setNavigateSuspended(bool suspend)
{
	if(mNavigateSuspended != suspend)
	{
		mNavigateSuspended = suspend;
		if(!suspend)
		{
			// We're coming out of suspend.  If someone tried to do a navigate while suspended, do one now instead.
			if(mNavigateSuspendedDeferred)
			{
				mNavigateSuspendedDeferred = false;
				navigateInternal();
			}
		}
	}
}

void LLViewerMediaImpl::cancelMimeTypeProbe()
{
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t probeAdapter = mMimeProbe.lock();

    if (probeAdapter)
        probeAdapter->cancelSuspendedOperation();
		
}

void LLViewerMediaImpl::addObject(LLVOVolume* obj)
{
	std::list< LLVOVolume* >::iterator iter = mObjectList.begin() ;
	for(; iter != mObjectList.end() ; ++iter)
	{
		if(*iter == obj)
		{
			return ; //already in the list.
		}
	}

	mObjectList.push_back(obj) ;
	mNeedsMuteCheck = true;
}

void LLViewerMediaImpl::removeObject(LLVOVolume* obj)
{
	mObjectList.remove(obj) ;
	mNeedsMuteCheck = true;
}

const std::list< LLVOVolume* >* LLViewerMediaImpl::getObjectList() const
{
	return &mObjectList ;
}

LLVOVolume *LLViewerMediaImpl::getSomeObject()
{
	LLVOVolume *result = NULL;

	std::list< LLVOVolume* >::iterator iter = mObjectList.begin() ;
	if(iter != mObjectList.end())
	{
		result = *iter;
	}

	return result;
}

void LLViewerMediaImpl::setTextureID(LLUUID id)
{
	if(id != mTextureId)
	{
		if(mTextureId.notNull())
		{
			// Remove this item's entry from the map
			sViewerMediaTextureIDMap.erase(mTextureId);
		}

		if(id.notNull())
		{
			sViewerMediaTextureIDMap.insert(LLViewerMedia::impl_id_map::value_type(id, this));
		}

		mTextureId = id;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaImpl::isAutoPlayable() const
{
	return (mMediaAutoPlay &&
			gSavedSettings.getS32("ParcelMediaAutoPlayEnable") != 0 &&
			gSavedSettings.getBOOL("MediaTentativeAutoPlay"));
}

//////////////////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaImpl::shouldShowBasedOnClass() const
{
	// If this is parcel media or in the UI, return true always
	if (getUsedInUI() || isParcelMedia()) return true;

	bool attached_to_another_avatar = isAttachedToAnotherAvatar();
	bool inside_parcel = isInAgentParcel();

	//	LL_INFOS() << " hasFocus = " << hasFocus() <<
	//	" others = " << (attached_to_another_avatar && gSavedSettings.getBOOL(LLViewerMedia::SHOW_MEDIA_ON_OTHERS_SETTING)) <<
	//	" within = " << (inside_parcel && gSavedSettings.getBOOL(LLViewerMedia::SHOW_MEDIA_WITHIN_PARCEL_SETTING)) <<
	//	" outside = " << (!inside_parcel && gSavedSettings.getBOOL(LLViewerMedia::SHOW_MEDIA_OUTSIDE_PARCEL_SETTING)) << LL_ENDL;

	// If it has focus, we should show it
	// This is incorrect, and causes EXT-6750 (disabled attachment media still plays)
//	if (hasFocus())
//		return true;

	// If it is attached to an avatar and the pref is off, we shouldn't show it
	if (attached_to_another_avatar)
	{
		static LLCachedControl<bool> show_media_on_others(gSavedSettings, LLViewerMedia::SHOW_MEDIA_ON_OTHERS_SETTING, false);
		return show_media_on_others;
	}
	if (inside_parcel)
	{
		static LLCachedControl<bool> show_media_within_parcel(gSavedSettings, LLViewerMedia::SHOW_MEDIA_WITHIN_PARCEL_SETTING, true);

		return show_media_within_parcel;
	}
	else
	{
		static LLCachedControl<bool> show_media_outside_parcel(gSavedSettings, LLViewerMedia::SHOW_MEDIA_OUTSIDE_PARCEL_SETTING, true);

		return show_media_outside_parcel;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaImpl::isObscured() const
{
    if (getUsedInUI() || isParcelMedia() || isAttachedToHUD()) return false;

    LLParcel* agent_parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    if (!agent_parcel)
    {
        return false;
    }
    
    if (agent_parcel->getObscureMOAP() && !isInAgentParcel())
    {
        return true;
    }

    return false;
}

bool LLViewerMediaImpl::isAttachedToHUD() const
{
    std::list< LLVOVolume* >::const_iterator iter = mObjectList.begin();
    std::list< LLVOVolume* >::const_iterator end = mObjectList.end();
    for ( ; iter != end; iter++)
    {
        if ((*iter)->isHUDAttachment())
        {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaImpl::isAttachedToAnotherAvatar() const
{
	bool result = false;

	std::list< LLVOVolume* >::const_iterator iter = mObjectList.begin();
	std::list< LLVOVolume* >::const_iterator end = mObjectList.end();
	for ( ; iter != end; iter++)
	{
		if (isObjectAttachedToAnotherAvatar(*iter))
		{
			result = true;
			break;
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
//static
bool LLViewerMediaImpl::isObjectAttachedToAnotherAvatar(LLVOVolume *obj)
{
	bool result = false;
	LLXform *xform = obj;
	// Walk up parent chain
	while (NULL != xform)
	{
		LLViewerObject *object = dynamic_cast<LLViewerObject*> (xform);
		if (NULL != object)
		{
			LLVOAvatar *avatar = object->asAvatar();
			if ((NULL != avatar) && (avatar != gAgentAvatarp))
			{
				result = true;
				break;
			}
		}
		xform = xform->getParent();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaImpl::isInAgentParcel() const
{
	bool result = false;

	std::list< LLVOVolume* >::const_iterator iter = mObjectList.begin();
	std::list< LLVOVolume* >::const_iterator end = mObjectList.end();
	for ( ; iter != end; iter++)
	{
		LLVOVolume *object = *iter;
		if (LLViewerMediaImpl::isObjectInAgentParcel(object))
		{
			result = true;
			break;
		}
	}
	return result;
}

LLNotificationPtr LLViewerMediaImpl::getCurrentNotification() const
{
	return mNotification;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// static
bool LLViewerMediaImpl::isObjectInAgentParcel(LLVOVolume *obj)
{
	return (LLViewerParcelMgr::getInstance()->inAgentParcel(obj->getPositionGlobal()));
}
