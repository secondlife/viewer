/**
 * @file llviewermedia.h
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

#ifndef LLVIEWERMEDIA_H
#define LLVIEWERMEDIA_H

#include "llfocusmgr.h"
#include "lleditmenuhandler.h"

#include "llpanel.h"
#include "llpluginclassmediaowner.h"

#include "llviewermediaobserver.h"

#include "llpluginclassmedia.h"
#include "v4color.h"

#include "llurl.h"

class LLViewerMediaImpl;
class LLUUID;
class LLViewerMediaTexture;
class LLMediaEntry;
class LLVOVolume;
class LLMimeDiscoveryResponder;
class LLPluginCookieStore;

typedef LLPointer<LLViewerMediaImpl> viewer_media_t;
///////////////////////////////////////////////////////////////////////////////
//
class LLViewerMediaEventEmitter
{
public:
	virtual ~LLViewerMediaEventEmitter();

	bool addObserver( LLViewerMediaObserver* subject );
	bool remObserver( LLViewerMediaObserver* subject );
	virtual void emitEvent(LLPluginClassMedia* self, LLViewerMediaObserver::EMediaEvent event);

private:
	typedef std::list< LLViewerMediaObserver* > observerListType;
	observerListType mObservers;
};

class LLViewerMediaImpl;

class LLViewerMedia
{
	LOG_CLASS(LLViewerMedia);
public:
	
	// String to get/set media autoplay in gSavedSettings
	static const char* AUTO_PLAY_MEDIA_SETTING;
	static const char* SHOW_MEDIA_ON_OTHERS_SETTING;
	static const char* SHOW_MEDIA_WITHIN_PARCEL_SETTING;
	static const char* SHOW_MEDIA_OUTSIDE_PARCEL_SETTING;
	
	typedef std::list<LLViewerMediaImpl*> impl_list;
	
	typedef std::map<LLUUID, LLViewerMediaImpl*> impl_id_map;
	
	// Special case early init for just web browser component
	// so we can show login screen.  See .cpp file for details. JC
	
	static viewer_media_t newMediaImpl(const LLUUID& texture_id,
									   S32 media_width = 0, 
									   S32 media_height = 0, 
									   U8 media_auto_scale = false,
									   U8 media_loop = false);
	
	static viewer_media_t updateMediaImpl(LLMediaEntry* media_entry, const std::string& previous_url, bool update_from_self);
	static LLViewerMediaImpl* getMediaImplFromTextureID(const LLUUID& texture_id);
	static std::string getCurrentUserAgent();
	static void updateBrowserUserAgent();
	static bool handleSkinCurrentChanged(const LLSD& /*newvalue*/);
	static bool textureHasMedia(const LLUUID& texture_id);
	static void setVolume(F32 volume);
	
	// Is any media currently "showing"?  Includes Parcel Media.  Does not include media in the UI.
	static bool isAnyMediaShowing();
	// Set all media enabled or disabled, depending on val.   Does not include media in the UI.
	static void setAllMediaEnabled(bool val);
	
	static void updateMedia(void* dummy_arg = NULL);
	
	static void initClass();
	static void cleanupClass();
	
	static F32 getVolume();	
	static void muteListChanged();
	static void setInWorldMediaDisabled(bool disabled);
	static bool getInWorldMediaDisabled();
	
	static bool isInterestingEnough(const LLVOVolume* object, const F64 &object_interest);
	
	// Returns the priority-sorted list of all media impls.
	static impl_list &getPriorityList();
	
	// This is the comparitor used to sort the list.
	static bool priorityComparitor(const LLViewerMediaImpl* i1, const LLViewerMediaImpl* i2);
	
	// These are just helper functions for the convenience of others working with media
	static bool hasInWorldMedia();
	static std::string getParcelAudioURL();
	static bool hasParcelMedia();
	static bool hasParcelAudio();
	static bool isParcelMediaPlaying();
	static bool isParcelAudioPlaying();
	
	// Clear all cookies for all plugins
	static void clearAllCookies();
	
	// Clear all plugins' caches
	static void clearAllCaches();
	
	// Set the "cookies enabled" flag for all loaded plugins
	static void setCookiesEnabled(bool enabled);
	
	// Set the proxy config for all loaded plugins
	static void setProxyConfig(bool enable, const std::string &host, int port);
	
	static LLPluginCookieStore *getCookieStore();
	static void loadCookieFile();
	static void saveCookieFile();
	static void addCookie(const std::string &name, const std::string &value, const std::string &domain, const LLDate &expires, const std::string &path = std::string("/"), bool secure = false );
	static void addSessionCookie(const std::string &name, const std::string &value, const std::string &domain, const std::string &path = std::string("/"), bool secure = false );
	static void removeCookie(const std::string &name, const std::string &domain, const std::string &path = std::string("/") );

	static void openIDSetup(const std::string &openid_url, const std::string &openid_token);
	static void openIDCookieResponse(const std::string &cookie);
	
	static void proxyWindowOpened(const std::string &target, const std::string &uuid);
	static void proxyWindowClosed(const std::string &uuid);
	
	static void createSpareBrowserMediaSource();
	static LLPluginClassMedia* getSpareBrowserMediaSource();
	
private:
	static void setOpenIDCookie();
	static void onTeleportFinished();
	
	static LLPluginCookieStore *sCookieStore;
	static LLURL sOpenIDURL;
	static std::string sOpenIDCookie;
	static LLPluginClassMedia* sSpareBrowserMediaSource;
};

// Implementation functions not exported into header file
class LLViewerMediaImpl
	:	public LLMouseHandler, public LLRefCount, public LLPluginClassMediaOwner, public LLViewerMediaEventEmitter, public LLEditMenuHandler
{
	LOG_CLASS(LLViewerMediaImpl);
public:
	
	friend class LLViewerMedia;
	friend class LLMimeDiscoveryResponder;
	
	LLViewerMediaImpl(
		const LLUUID& texture_id,
		S32 media_width, 
		S32 media_height, 
		U8 media_auto_scale,
		U8 media_loop);

	~LLViewerMediaImpl();
	
	// Override inherited version from LLViewerMediaEventEmitter 
	virtual void emitEvent(LLPluginClassMedia* self, LLViewerMediaObserver::EMediaEvent event);

	void createMediaSource();
	void destroyMediaSource();
	void setMediaType(const std::string& media_type);
	bool initializeMedia(const std::string& mime_type);
	bool initializePlugin(const std::string& media_type);
	void loadURI();
	LLPluginClassMedia* getMediaPlugin() { return mMediaSource; }
	void setSize(int width, int height);

	void play();
	void stop();
	void pause();
	void start();
	void seek(F32 time);
	void skipBack(F32 step_scale);
	void skipForward(F32 step_scale);
	void setVolume(F32 volume);
	void updateVolume();
	F32 getVolume();
	void focus(bool focus);
	// True if the impl has user focus.
	bool hasFocus() const;
	void mouseDown(S32 x, S32 y, MASK mask, S32 button = 0);
	void mouseUp(S32 x, S32 y, MASK mask, S32 button = 0);
	void mouseMove(S32 x, S32 y, MASK mask);
	void mouseDown(const LLVector2& texture_coords, MASK mask, S32 button = 0);
	void mouseUp(const LLVector2& texture_coords, MASK mask, S32 button = 0);
	void mouseMove(const LLVector2& texture_coords, MASK mask);
	void mouseDoubleClick(S32 x,S32 y, MASK mask, S32 button = 0);
	void scrollWheel(S32 x, S32 y, MASK mask);
	void mouseCapture();
	
	void navigateBack();
	void navigateForward();
	void navigateReload();
	void navigateHome();
	void unload();
	void navigateTo(const std::string& url, const std::string& mime_type = "", bool rediscover_type = false, bool server_request = false);
	void navigateInternal();
	void navigateStop();
	bool handleKeyHere(KEY key, MASK mask);
	bool handleUnicodeCharHere(llwchar uni_char);
	bool canNavigateForward();
	bool canNavigateBack();
	std::string getMediaURL() const { return mMediaURL; }
	std::string getCurrentMediaURL();
	std::string getHomeURL() { return mHomeURL; }
	std::string getMediaEntryURL() { return mMediaEntryURL; }
	void setHomeURL(const std::string& home_url, const std::string& mime_type = LLStringUtil::null) { mHomeURL = home_url; mHomeMimeType = mime_type;};
	void clearCache();
	std::string getMimeType() { return mMimeType; }
	void scaleMouse(S32 *mouse_x, S32 *mouse_y);
	void scaleTextureCoords(const LLVector2& texture_coords, S32 *x, S32 *y);

	void update();
	void updateImagesMediaStreams();
	LLUUID getMediaTextureID() const;
	
	void suspendUpdates(bool suspend) { mSuspendUpdates = suspend; }
	void setVisible(bool visible);
	bool getVisible() const { return mVisible; }
	bool isVisible() const { return mVisible; }

	bool isMediaTimeBased();
	bool isMediaPlaying();
	bool isMediaPaused();
	bool hasMedia() const;
	bool isMediaFailed() const { return mMediaSourceFailed; }
	void setMediaFailed(bool val) { mMediaSourceFailed = val; }
	void resetPreviousMediaState();
	
	void setDisabled(bool disabled, bool forcePlayOnEnable = false);
	bool isMediaDisabled() const { return mIsDisabled; };
	
	void setInNearbyMediaList(bool in_list) { mInNearbyMediaList = in_list; }
	bool getInNearbyMediaList() { return mInNearbyMediaList; }
	
	// returns true if this instance should not be loaded (disabled, muted object, crashed, etc.)
	bool isForcedUnloaded() const;
	
	// returns true if this instance could be playable based on autoplay setting, current load state, etc.
	bool isPlayable() const;
	
	void setIsParcelMedia(bool is_parcel_media) { mIsParcelMedia = is_parcel_media; }
	bool isParcelMedia() const { return mIsParcelMedia; }

	ECursorType getLastSetCursor() { return mLastSetCursor; }
	
	void setTarget(const std::string& target) { mTarget = target; }
	
	// utility function to create a ready-to-use media instance from a desired media type.
	static LLPluginClassMedia* newSourceFromMediaType(std::string media_type, LLPluginClassMediaOwner *owner /* may be NULL */, S32 default_width, S32 default_height, const std::string target = LLStringUtil::null);

	// Internally set our desired browser user agent string, including
	// the Second Life version and skin name.  Used because we can
	// switch skins without restarting the app.
	static void updateBrowserUserAgent();

	// Callback for when the SkinCurrent control is changed to
	// switch the user agent string to indicate the new skin.
	static bool handleSkinCurrentChanged(const LLSD& newvalue);

	// need these to handle mouseup...
	/*virtual*/ void	onMouseCaptureLost();
	/*virtual*/ BOOL	handleMouseUp(S32 x, S32 y, MASK mask);

	// Grr... the only thing I want as an LLMouseHandler are the onMouseCaptureLost and handleMouseUp calls.
	// Sadly, these are all pure virtual, so I have to supply implementations here:
	/*virtual*/ BOOL	handleMouseDown(S32 x, S32 y, MASK mask) { return FALSE; };
	/*virtual*/ BOOL	handleHover(S32 x, S32 y, MASK mask) { return FALSE; };
	/*virtual*/ BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks) { return FALSE; };
	/*virtual*/ BOOL	handleDoubleClick(S32 x, S32 y, MASK mask) { return FALSE; };
	/*virtual*/ BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask) { return FALSE; };
	/*virtual*/ BOOL	handleRightMouseUp(S32 x, S32 y, MASK mask) { return FALSE; };
	/*virtual*/ BOOL	handleToolTip(S32 x, S32 y, MASK mask) { return FALSE; };
	/*virtual*/ BOOL	handleMiddleMouseDown(S32 x, S32 y, MASK mask) { return FALSE; };
	/*virtual*/ BOOL	handleMiddleMouseUp(S32 x, S32 y, MASK mask) {return FALSE; };
	/*virtual*/ std::string getName() const;

	/*virtual*/ void	screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const {};
	/*virtual*/ void	localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const {};
	/*virtual*/ BOOL hasMouseCapture() { return gFocusMgr.getMouseCapture() == this; };

	// Inherited from LLPluginClassMediaOwner
	/*virtual*/ void handleMediaEvent(LLPluginClassMedia* plugin, LLPluginClassMediaOwner::EMediaEvent);
	/*virtual*/ void handleCookieSet(LLPluginClassMedia* self, const std::string &cookie);

	// LLEditMenuHandler overrides
	/*virtual*/ void	cut();
	/*virtual*/ BOOL	canCut() const;

	/*virtual*/ void	copy();
	/*virtual*/ BOOL	canCopy() const;

	/*virtual*/ void	paste();
	/*virtual*/ BOOL	canPaste() const;
	
	void addObject(LLVOVolume* obj) ;
	void removeObject(LLVOVolume* obj) ;
	const std::list< LLVOVolume* >* getObjectList() const ;
	LLVOVolume *getSomeObject();
	void setUpdated(BOOL updated) ;
	BOOL isUpdated() ;
	
	// Updates the "interest" value in this object
	void calculateInterest();
	F64 getInterest() const { return mInterest; };
	F64 getApproximateTextureInterest();
	S32 getProximity() const { return mProximity; };
	F64 getProximityDistance() const { return mProximityDistance; };
	
	// Mark this object as being used in a UI panel instead of on a prim
	// This will be used as part of the interest sorting algorithm.
	void setUsedInUI(bool used_in_ui);
	bool getUsedInUI() const { return mUsedInUI; };

	void setBackgroundColor(LLColor4 color);
	
	F64 getCPUUsage() const;
	
	void setPriority(LLPluginClassMedia::EPriority priority);
	LLPluginClassMedia::EPriority getPriority() { return mPriority; };

	void setLowPrioritySizeLimit(int size);

	void setTextureID(LLUUID id = LLUUID::null);

	bool isTrustedBrowser() { return mTrustedBrowser; }
	void setTrustedBrowser(bool trusted) { mTrustedBrowser = trusted; }
	
	typedef enum 
	{
		MEDIANAVSTATE_NONE,										// State is outside what we need to track for navigation.
		MEDIANAVSTATE_BEGUN,									// a MEDIA_EVENT_NAVIGATE_BEGIN has been received which was not server-directed
		MEDIANAVSTATE_FIRST_LOCATION_CHANGED,					// first LOCATION_CHANGED event after a non-server-directed BEGIN
		MEDIANAVSTATE_FIRST_LOCATION_CHANGED_SPURIOUS,			// Same as above, but the new URL is identical to the previously navigated URL.
		MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED,			// we received a NAVIGATE_COMPLETE event before the first LOCATION_CHANGED
		MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED_SPURIOUS,// Same as above, but the new URL is identical to the previously navigated URL.
		MEDIANAVSTATE_SERVER_SENT,								// server-directed nav has been requested, but MEDIA_EVENT_NAVIGATE_BEGIN hasn't been received yet
		MEDIANAVSTATE_SERVER_BEGUN,								// MEDIA_EVENT_NAVIGATE_BEGIN has been received which was server-directed
		MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED,			// first LOCATION_CHANGED event after a server-directed BEGIN
		MEDIANAVSTATE_SERVER_COMPLETE_BEFORE_LOCATION_CHANGED	// we received a NAVIGATE_COMPLETE event before the first LOCATION_CHANGED
		
	}EMediaNavState;
    
	// Returns the current nav state of the media.
	// note that this will be updated BEFORE listeners and objects receive media messages 
	EMediaNavState getNavState() { return mMediaNavState; }
	void setNavState(EMediaNavState state);
	
	void setNavigateSuspended(bool suspend);
	bool isNavigateSuspended() { return mNavigateSuspended; };
	
	void cancelMimeTypeProbe();
	
	// Is this media attached to an avatar *not* self
	bool isAttachedToAnotherAvatar() const;
	
	// Is this media in the agent's parcel?
	bool isInAgentParcel() const;

private:
	bool isAutoPlayable() const;
	bool shouldShowBasedOnClass() const;
	static bool isObjectAttachedToAnotherAvatar(LLVOVolume *obj);
	static bool isObjectInAgentParcel(LLVOVolume *obj);
	
private:
	// a single media url with some data and an impl.
	LLPluginClassMedia* mMediaSource;
	LLUUID mTextureId;
	bool  mMovieImageHasMips;
	std::string mMediaURL;			// The last media url set with NavigateTo
	std::string mHomeURL;
	std::string mHomeMimeType;		// forced mime type for home url
	std::string mMimeType;
	std::string mCurrentMediaURL;	// The most current media url from the plugin (via the "location changed" or "navigate complete" events).
	std::string mCurrentMimeType;	// The MIME type that caused the currently loaded plugin to be loaded.
	S32 mLastMouseX;	// save the last mouse coord we get, so when we lose capture we can simulate a mouseup at that point.
	S32 mLastMouseY;
	S32 mMediaWidth;
	S32 mMediaHeight;
	bool mMediaAutoScale;
	bool mMediaLoop;
	bool mNeedsNewTexture;
	S32 mTextureUsedWidth;
	S32 mTextureUsedHeight;
	bool mSuspendUpdates;
	bool mVisible;
	ECursorType mLastSetCursor;
	EMediaNavState mMediaNavState;
	F64 mInterest;
	bool mUsedInUI;
	bool mHasFocus;
	LLPluginClassMedia::EPriority mPriority;
	bool mNavigateRediscoverType;
	bool mNavigateServerRequest;
	bool mMediaSourceFailed;
	F32 mRequestedVolume;
	bool mIsMuted;
	bool mNeedsMuteCheck;
	int mPreviousMediaState;
	F64 mPreviousMediaTime;
	bool mIsDisabled;
	bool mIsParcelMedia;
	S32 mProximity;
	F64 mProximityDistance;
	F64 mProximityCamera;
	LLMimeDiscoveryResponder *mMimeTypeProbe;
	bool mMediaAutoPlay;
	std::string mMediaEntryURL;
	bool mInNearbyMediaList;	// used by LLPanelNearbyMedia::refreshList() for performance reasons
	bool mClearCache;
	LLColor4 mBackgroundColor;
	bool mNavigateSuspended;
	bool mNavigateSuspendedDeferred;
	bool mTrustedBrowser;
	std::string mTarget;
	
private:
	BOOL mIsUpdated ;
	std::list< LLVOVolume* > mObjectList ;

private:
	LLViewerMediaTexture *updatePlaceholderImage();
};

#endif	// LLVIEWERMEDIA_H
