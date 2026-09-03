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
#include "llnotificationptr.h"
#include "lltimer.h"

#include "llurl.h"
#include "lleventcoro.h"
#include "llcoros.h"
#include "llcorehttputil.h"
#include "llembeddedbrowser.h"

#include <vector>

class LLViewerMediaImpl;
class LLUUID;
class LLViewerMediaTexture;
class LLMediaEntry;
class LLVOVolume;
class LLMimeDiscoveryResponder;

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
class LLMediaCtrl;

class LLViewerMedia: public LLSingleton<LLViewerMedia>
{
    LLSINGLETON(LLViewerMedia);
    ~LLViewerMedia();
    void initSingleton() override;
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

    viewer_media_t newMediaImpl(const LLUUID& texture_id,
                                       S32 media_width = 0,
                                       S32 media_height = 0,
                                       U8 media_auto_scale = false,
                                       U8 media_loop = false);

    viewer_media_t updateMediaImpl(LLMediaEntry* media_entry, const std::string& previous_url, bool update_from_self);
    LLViewerMediaImpl* getMediaImplFromTextureID(const LLUUID& texture_id);
    std::string getCurrentUserAgent();
    void updateBrowserUserAgent();
    bool handleSkinCurrentChanged(const LLSD& /*newvalue*/);
    bool textureHasMedia(const LLUUID& texture_id);
    void setVolume(F32 volume);

    // Is any media currently "showing"?  Includes Parcel Media.  Does not include media in the UI.
    bool isAnyMediaShowing();
    // Shows if any media is playing, counts visible non time based media as playing. Does not include media in the UI.
    bool isAnyMediaPlaying();
    // Set all media enabled or disabled, depending on val.   Does not include media in the UI.
    void setAllMediaEnabled(bool val);
    // Set all media paused(stopped for non time based) or playing, depending on val.   Does not include media in the UI.
    void setAllMediaPaused(bool val);

    static void onIdle(void* dummy_arg = NULL); // updateMedia wrapper
    void updateMedia(void* dummy_arg = NULL);

    F32 getVolume();
    void muteListChanged();
    bool isInterestingEnough(const LLVOVolume* object, const F64 &object_interest);

    // Returns the priority-sorted list of all media impls.
    impl_list &getPriorityList();

    // This is the comparitor used to sort the list.
    static bool priorityComparitor(const LLViewerMediaImpl* i1, const LLViewerMediaImpl* i2);

    // One row per active embedded-browser LLViewerMediaImpl, for the media debug floater.
    // Each row: {"slot", "url", "priority", "priority_label", "kind" ("ui"/"prim"/"parcel"), "slurl"}.
    // "slurl" is only ever populated for "prim" rows -- "ui" and "parcel" media have no single
    // object a location can be derived from.
    LLSD getEmbeddedBrowserDebugInfo();

    // The effective MediaMaxInstances cap (already adjusted for low physical memory --
    // see setMaxInstances()), for the media debug floater's "N/max instances" title.
    // Note this cap is shared across both media backends -- it is not embedded-browser-
    // specific -- so on a build where the legacy media plugin is also active, this max
    // is being contested by more than just the rows getEmbeddedBrowserDebugInfo() lists.
    S32 getMaxInstances() const { return mMaxInstances; }

    // These are just helper functions for the convenience of others working with media
    bool hasInWorldMedia();
    std::string getParcelAudioURL();
    bool hasParcelMedia();
    bool hasParcelAudio();
    bool isParcelMediaPlaying();
    bool isParcelAudioPlaying();

    static void authSubmitCallback(const LLSD& notification, const LLSD& response);

    // Clear all cookies for all plugins
    void clearAllCookies();

    // Clear all plugins' caches
    void clearAllCaches();

    // Set the "cookies enabled" flag for all loaded plugins
    void setCookiesEnabled(bool enabled);

    // Set the proxy config for all loaded plugins
    void setProxyConfig(bool enable, const std::string &host, int port);

    void openIDSetup(const std::string &openid_url, const std::string &openid_token);
    void openIDCookieResponse(const std::string& url, const std::string &cookie);

    void proxyWindowOpened(const std::string &target, const std::string &uuid);
    void proxyWindowClosed(const std::string &uuid);

    void createSpareBrowserMediaSource();
    LLPluginClassMedia* getSpareBrowserMediaSource();

    void setOnlyAudibleMediaTextureID(const LLUUID& texture_id);

    LLSD getHeaders();
    LLCore::HttpHeaders::ptr_t getHttpHeaders();
    bool getOpenIDCookie(LLMediaCtrl* media_instance) const;

private:
    void onAuthSubmit(const LLSD& notification, const LLSD& response);
    static bool parseRawCookie(const std::string raw_cookie, std::string& name, std::string& value, std::string& path, bool& httponly, bool& secure);
    void setOpenIDCookie(const std::string& url);
    void onTeleportFinished();

    static void openIDSetupCoro(std::string openidUrl, std::string openidToken);
    static void getOpenIDCookieCoro(std::string url);
    void setMaxInstances(S32 max_instances);

    bool mAnyMediaShowing;
    bool mAnyMediaPlaying;
    S32 mMaxInstances = 12; // overwritten by the constructor's real default anyway (see MAX_MEDIA_INSTANCES_DEFAULT)
    LLURL mOpenIDURL;
    std::string mOpenIDCookie;
    LLPluginClassMedia* mSpareBrowserMediaSource;
    boost::signals2::connection mTeleportFinishConnection;
    boost::signals2::connection mMaxInstancesConnection;
};

// Implementation functions not exported into header file
class LLViewerMediaImpl
    :   public LLMouseHandler, public LLThreadSafeRefCount, public LLPluginClassMediaOwner, public LLViewerMediaEventEmitter, public LLEditMenuHandler
{
    LOG_CLASS(LLViewerMediaImpl);
public:

    friend class LLViewerMedia;

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
    void executeJavaScript(const std::string& code);
    LLPluginClassMedia* getMediaPlugin() { return mMediaSource.get(); }
    void setSize(int width, int height);

    void showNotification(LLNotificationPtr notify);
    void hideNotification();

    void play();
    void stop();
    void pause();
    void start();
    void seek(F32 time);
    void skipBack(F32 step_scale);
    void skipForward(F32 step_scale);
    void setVolume(F32 volume);
    void setMute(bool mute);
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
    void mouseDoubleClick(const LLVector2& texture_coords, MASK mask);
    void mouseDoubleClick(S32 x, S32 y, MASK mask, S32 button = 0);
    void scrollWheel(const LLVector2& texture_coords, S32 scroll_x, S32 scroll_y, MASK mask);
    void scrollWheel(S32 x, S32 y, S32 scroll_x, S32 scroll_y, MASK mask);
    void mouseCapture();

    void navigateBack();
    void navigateForward();
    void navigateReload();
    void navigateHome();
    void unload();
    void navigateTo(const std::string& url, const std::string& mime_type = "", bool rediscover_type = false, bool server_request = false, bool clean_browser = false);
    void navigateInternal(bool should_log = true);
    void navigateStop();
    bool handleKeyHere(KEY key, MASK mask);
    bool handleKeyUpHere(KEY key, MASK mask);
    bool handleUnicodeCharHere(llwchar uni_char);
    bool canNavigateForward();
    bool canNavigateBack();
    std::string getMediaURL() const { return mMediaURL; }
    std::string getCurrentMediaURL();
    std::string getHomeURL() { return mHomeURL; }
    std::string getMediaEntryURL() { return mMediaEntryURL; }
    void setHomeURL(const std::string& home_url, const std::string& mime_type = LLStringUtil::null) { mHomeURL = home_url; mHomeMimeType = mime_type;};
    void clearCache();
    void setPageZoomFactor( double factor );
    double getPageZoomFactor() {return mZoomFactor;}
    std::string getMimeType() { return mMimeType; }
    void scaleMouse(S32 *mouse_x, S32 *mouse_y);
    void scaleTextureCoords(const LLVector2& texture_coords, S32 *x, S32 *y);

    void update();
    bool preMediaTexUpdate(LLViewerMediaTexture*& media_tex, U8*& data, S32& data_width, S32& data_height, S32& x_pos, S32& y_pos, S32& width, S32& height);
    void doMediaTexUpdate(LLViewerMediaTexture* media_tex, U8* data, S32 data_width, S32 data_height, S32 x_pos, S32 y_pos, S32 width, S32 height, bool sync);
    void updateImagesMediaStreams();
    LLUUID getMediaTextureID() const;

    // Backend-agnostic accessors used by consumers (e.g. LLMediaCtrl) that need to draw
    // the media texture directly without caring whether it's backed by the CEF plugin
    // or LLEmbeddedBrowser.
    bool isTextureReady() const;
    S32 getMediaWidth() const;
    S32 getMediaHeight() const;
    S32 getMediaTextureWidth() const;
    S32 getMediaTextureHeight() const;
    bool getMediaTextureCoordsOpenGL() const;
    // Backend-agnostic (see above) -- the plugin path has always let observers pull this
    // straight from LLPluginClassMedia at event time; the embedded-browser path has no such
    // object, so its title is cached here instead (see updateEmbeddedBrowserEvents()).
    std::string getMediaName() const;

    // Embedded-browser-only equivalents of LLPluginClassMedia's own mClick* accessors,
    // populated from updateEmbeddedBrowserEvents() just before MEDIA_EVENT_CLICK_LINK_HREF/
    // MEDIA_EVENT_CLICK_LINK_NOFOLLOW fires (self is null for those, so observers read these
    // instead -- see the handleMediaEvent patches in llmediactrl.cpp).
    std::string getClickURL() const { return mEmbeddedClickURL; }
    std::string getClickTarget() const { return mEmbeddedClickTarget; }
    std::string getClickUUID() const { return mEmbeddedClickUUID; }
    std::string getClickNavType() const { return mEmbeddedClickNavType; }

    // Embedded-browser-only equivalent of LLPluginClassMedia::getStatusText(), populated just
    // before MEDIA_EVENT_STATUS_TEXT_CHANGED fires -- see the handleMediaEvent patches in
    // llmediactrl.cpp/llfloaterwebcontent.cpp/llpaneldirweb.cpp/llpanelprofile.cpp, and
    // LLMediaCtrl::getStatusText(), the widget-level passthrough those actually call.
    std::string getStatusText() const { return mEmbeddedStatusText; }

    // Embedded-browser-only equivalent of LLPluginClassMedia::getFileDownloadFilename(),
    // populated just before MEDIA_EVENT_FILE_DOWNLOAD fires -- see handleMediaEvent in
    // llmediactrl.cpp, which is what LLEmbeddedMediaFilePicker (llviewermenufile.h) is
    // constructed from.
    std::string getFileDownloadFilename() const { return mEmbeddedFileDownloadFilename; }
    // Completes the file dialog request that most recently fired MEDIA_EVENT_PICK_FILE_REQUEST
    // or MEDIA_EVENT_FILE_DOWNLOAD -- pass an empty vector to indicate the user canceled.
    void respondToFileDialog(const std::vector<std::string>& filePaths);

    // Embedded-browser-only equivalent of LLPluginClassMedia::getDebugMessageText(), populated
    // just before MEDIA_EVENT_DEBUG_MESSAGE fires from a page's console.log/warn/error call --
    // see the handleMediaEvent patch in llmediactrl.cpp. Pre-formatted to match that plugin
    // accessor's own text exactly ("Console message: <msg> in file(<source>) at line <n>"), since
    // that's the string LLMediaCtrlListener::getMediaText() searches for its extraction marker in.
    std::string getDebugMessageText() const { return mEmbeddedDebugMessageText; }

    void suspendUpdates(bool suspend) { mSuspendUpdates = suspend; }
    void setVisible(bool visible);
    bool getVisible() const { return mVisible; }
    bool isVisible() const { return mVisible; }

    bool isMediaTimeBased();
    bool isMediaPlaying();
    bool isMediaPaused();
    bool hasMedia() const;
    // Narrow, explicit alternative to hasMedia() for the embedded-browser case -- hasMedia()
    // itself deliberately stays plugin-only (mMediaSource != NULL): it gates real callers
    // elsewhere that immediately follow it with an unchecked getMediaPlugin()->someCall(),
    // which would crash if hasMedia() started reporting true for embedded-browser media.
    // Use `hasMedia() || isUsingEmbeddedBrowser()` at call sites that don't touch the
    // plugin object directly (e.g. forwarding input).
    bool isUsingEmbeddedBrowser() const { return mUseEmbeddedBrowser; }
    // Only meaningful when isUsingEmbeddedBrowser() is true; returns Cef otherwise.
    LLEmbeddedBrowserBackend getEmbeddedBrowserBackend() const { return mEmbeddedBrowserBackend; }
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
    static LLPluginClassMedia* newSourceFromMediaType(std::string media_type, LLPluginClassMediaOwner *owner /* may be NULL */, S32 default_width, S32 default_height, F64 zoom_factor, const std::string target = LLStringUtil::null, bool clean_browser = false);

    // Internally set our desired browser user agent string, including
    // the Second Life version and skin name.  Used because we can
    // switch skins without restarting the app.
    static void updateBrowserUserAgent();

    // Callback for when the SkinCurrent control is changed to
    // switch the user agent string to indicate the new skin.
    static bool handleSkinCurrentChanged(const LLSD& newvalue);

    // need these to handle mouseup...
    /*virtual*/ void    onMouseCaptureLost();
    /*virtual*/ bool    handleMouseUp(S32 x, S32 y, MASK mask);

    // Grr... the only thing I want as an LLMouseHandler are the onMouseCaptureLost and handleMouseUp calls.
    // Sadly, these are all pure virtual, so I have to supply implementations here:
    /*virtual*/ bool    handleMouseDown(S32 x, S32 y, MASK mask) { return false; };
    /*virtual*/ bool    handleHover(S32 x, S32 y, MASK mask) { return false; };
    /*virtual*/ bool    handleScrollWheel(S32 x, S32 y, S32 clicks) { return false; };
    /*virtual*/ bool    handleScrollHWheel(S32 x, S32 y, S32 clicks) { return false; };
    /*virtual*/ bool    handleDoubleClick(S32 x, S32 y, MASK mask) { return false; };
    /*virtual*/ bool    handleRightMouseDown(S32 x, S32 y, MASK mask) { return false; };
    /*virtual*/ bool    handleRightMouseUp(S32 x, S32 y, MASK mask) { return false; };
    /*virtual*/ bool    handleToolTip(S32 x, S32 y, MASK mask) { return false; };
    /*virtual*/ bool    handleMiddleMouseDown(S32 x, S32 y, MASK mask) { return false; };
    /*virtual*/ bool    handleMiddleMouseUp(S32 x, S32 y, MASK mask) {return false; };
    /*virtual*/ const std::string& getName() const;

    /*virtual*/ void    screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const {};
    /*virtual*/ void    localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const {};
    /*virtual*/ bool hasMouseCapture() { return gFocusMgr.getMouseCapture() == this; };

    // Inherited from LLPluginClassMediaOwner
    /*virtual*/ void handleMediaEvent(LLPluginClassMedia* plugin, LLPluginClassMediaOwner::EMediaEvent);

    // LLEditMenuHandler overrides
    /*virtual*/ void    undo();
    /*virtual*/ bool    canUndo() const;

    /*virtual*/ void    redo();
    /*virtual*/ bool    canRedo() const;

    /*virtual*/ void    cut();
    /*virtual*/ bool    canCut() const;

    /*virtual*/ void    copy();
    /*virtual*/ bool    canCopy() const;

    /*virtual*/ void    paste();
    /*virtual*/ bool    canPaste() const;

    /*virtual*/ void    doDelete();
    /*virtual*/ bool    canDoDelete() const;

    /*virtual*/ void    selectAll();
    /*virtual*/ bool    canSelectAll() const;

    void addObject(LLVOVolume* obj) ;
    void removeObject(LLVOVolume* obj) ;
    const std::list< LLVOVolume* >* getObjectList() const ;
    LLVOVolume *getSomeObject();
    void setUpdated(bool updated) ;
    bool isUpdated() ;

    // updates the javascript object in the embedded browser with viewer values
    void updateJavascriptObject();

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

    // Only meaningful when isUsingEmbeddedBrowser() is true; returns false (leaving
    // out_index untouched) if this impl has no connected producer slot right now.
    bool getEmbeddedBrowserSlotIndex(unsigned int& out_index) const;

    typedef enum
    {
        MEDIANAVSTATE_NONE,                                     // State is outside what we need to track for navigation.
        MEDIANAVSTATE_BEGUN,                                    // a MEDIA_EVENT_NAVIGATE_BEGIN has been received which was not server-directed
        MEDIANAVSTATE_FIRST_LOCATION_CHANGED,                   // first LOCATION_CHANGED event after a non-server-directed BEGIN
        MEDIANAVSTATE_FIRST_LOCATION_CHANGED_SPURIOUS,          // Same as above, but the new URL is identical to the previously navigated URL.
        MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED,         // we received a NAVIGATE_COMPLETE event before the first LOCATION_CHANGED
        MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED_SPURIOUS,// Same as above, but the new URL is identical to the previously navigated URL.
        MEDIANAVSTATE_SERVER_SENT,                              // server-directed nav has been requested, but MEDIA_EVENT_NAVIGATE_BEGIN hasn't been received yet
        MEDIANAVSTATE_SERVER_BEGUN,                             // MEDIA_EVENT_NAVIGATE_BEGIN has been received which was server-directed
        MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED,            // first LOCATION_CHANGED event after a server-directed BEGIN
        MEDIANAVSTATE_SERVER_COMPLETE_BEFORE_LOCATION_CHANGED   // we received a NAVIGATE_COMPLETE event before the first LOCATION_CHANGED

    }EMediaNavState;

    // Returns the current nav state of the media.
    // note that this will be updated BEFORE listeners and objects receive media messages
    EMediaNavState getNavState() { return mMediaNavState; }
    void setNavState(EMediaNavState state);

    void setNavigateSuspended(bool suspend);
    bool isNavigateSuspended() { return mNavigateSuspended; };

    void cancelMimeTypeProbe();

    bool isAttachedToHUD() const;

    // Is this media attached to an avatar *not* self
    bool isAttachedToAnotherAvatar() const;

    // Is this media in the agent's parcel?
    bool isInAgentParcel() const;

    // get currently active notification associated with this media instance
    LLNotificationPtr getCurrentNotification() const;

private:
    bool isAutoPlayable() const;
    bool shouldShowBasedOnClass() const;
    bool isObscured() const;
    static bool isObjectAttachedToAnotherAvatar(LLVOVolume *obj);
    static bool isObjectInAgentParcel(LLVOVolume *obj);

private:
    // a single media url with some data and an impl.
    std::shared_ptr<LLPluginClassMedia> mMediaSource;
    // When true, this media impl is backed by LLEmbeddedBrowser instead of
    // mMediaSource/CEF -- see LLViewerMediaImpl::createMediaSource().
    bool mUseEmbeddedBrowser = false;
    unsigned int mEmbeddedBrowserId = 0;
    // Last mute/unmute state actually sent to LLEmbeddedBrowser::setMuted() -- lets
    // updateVolume() only send the opcode when the decision flips, since it's
    // recomputed every frame. See updateVolume()'s own comment for why embedded-
    // browser media gets a mute decision instead of legacy's continuous volume.
    bool mEmbeddedBrowserMuted = false;
    // Which producer-side backend this impl's slot was created with -- decided once,
    // in createMediaSource(), by chooseEmbeddedBrowserBackend() (URL scheme plus
    // mUsedInUI). Remembered rather than recomputed because it is a property of the
    // *slot*, fixed at kRequestSlot time and not changeable afterwards: the producer
    // picks a CEF browser or a LibVLC player when it allocates the slot. Later
    // per-frame calls (see updateVolume()) must route on what the slot actually is,
    // not on what the current URL would ask for today.
    LLEmbeddedBrowserBackend mEmbeddedBrowserBackend = LLEmbeddedBrowserBackend::Cef;
    // Last continuous volume actually sent to LLEmbeddedBrowser::setVolume(), for
    // LibVLC-backed slots only -- lets updateVolume() only send the opcode when the
    // level meaningfully changes, since it's recomputed every frame and
    // mProximityCamera jitters constantly as the camera moves. -1.f is a sentinel that
    // no real (clamped 0..1) volume can equal, so the first updateVolume() after a tab
    // is created always sends, even if the computed level happens to match the
    // producer's own default.
    F32 mEmbeddedBrowserVolume = -1.f;
    // Last render-rate hint actually sent to LLEmbeddedBrowser::setRenderRate() --
    // lets setPriority() only send the opcode when the target fps actually
    // changes, since setPriority() itself is called every frame regardless of
    // whether priority changed. 0 = unthrottled/full rate (always the value for
    // UI/parcel media -- see setPriority()'s own comment on why that's not just
    // whatever the shared priority computation happens to produce).
    unsigned int mEmbeddedBrowserTargetFps = 0;
    // The render-rate tier actually applied/sent so far (0=Normal/High i.e.
    // unthrottled, 1=Low, 2=Slideshow, 3=Hidden -- see kSetRenderRate's own
    // comment in cefshm_protocol.h). A newly computed tier numerically higher
    // than this is a demotion and goes through mEmbeddedBrowserRenderRateDemotion*
    // below; anything else (an improvement, or no change) applies immediately.
    unsigned int mEmbeddedBrowserAppliedTier = 0;
    // Debounces a render-rate DEMOTION only (see EMBEDDED_BROWSER_RENDER_RATE_
    // DEMOTION_GRACE_PERIOD's own comment) -- same true-from-first-frame-until-
    // grace-period-elapses-or-cancelled pattern as mEmbeddedBrowserUnloadPending
    // below, just for a softer consequence (a lower render rate, not a teardown).
    bool mEmbeddedBrowserRenderRateDemotionPending = false;
    LLTimer mEmbeddedBrowserRenderRateDemotionTimer;
    // Debounces the SLIDESHOW/HIDDEN auto-unload in setPriority(): true from
    // the first frame this impl is found unload-worthy until either the
    // grace period in mEmbeddedBrowserUnloadTimer expires (destroy for real)
    // or its priority recovers first (cancelled). Right after a region
    // change, many impls' interest values are still settling and can flicker
    // a freshly-created tab across the SLIDESHOW boundary for a frame or two
    // -- reacting to a single frame's demotion tore the tab down before its
    // page ever finished rendering, forcing it to restart from scratch on
    // every flicker. See setPriority()'s own comment.
    bool mEmbeddedBrowserUnloadPending = false;
    LLTimer mEmbeddedBrowserUnloadTimer;
    // Owns the most recent embedded-browser frame snapshot handed to doMediaTexUpdate().
    // preMediaTexUpdate() replaces this with a fresh copy every frame; update()'s async
    // GL-upload lambda captures a local copy of the shared_ptr (not just the raw data
    // pointer) so the buffer it reads from stays alive for the lambda's lifetime even
    // if this member is reassigned or the underlying LLEmbeddedBrowser tab is resized
    // or destroyed before the worker thread gets around to running it.
    std::shared_ptr<std::vector<U8>> mEmbeddedBrowserFrameSnapshot;
    std::string mEmbeddedBrowserTitle;
    // See getClickURL()/getClickTarget()/getClickUUID()/getClickNavType() above.
    std::string mEmbeddedClickURL;
    std::string mEmbeddedClickTarget;
    std::string mEmbeddedClickUUID;
    std::string mEmbeddedClickNavType;
    // See getFileDownloadFilename()/respondToFileDialog() above.
    std::string mEmbeddedFileDownloadFilename;
    long long mEmbeddedFileDialogId = 0;
    // See getStatusText() above.
    std::string mEmbeddedStatusText;
    // See getDebugMessageText() above.
    std::string mEmbeddedDebugMessageText;

    // Drains LLEmbeddedBrowser::popEvent() for mEmbeddedBrowserId, updating the cached
    // state below and calling emitEvent(nullptr, ...) for each one -- nullptr is a safe,
    // unambiguous signal to observers that this event has no real LLPluginClassMedia
    // behind it (a genuine plugin callback never passes nullptr), see LLMediaCtrl and the
    // other LLViewerMediaObserver overrides patched alongside this for the specific
    // handful of events this can fire. Called once per update().
    void updateEmbeddedBrowserEvents();
    LLCoros::Mutex mLock;
    F64     mZoomFactor;
    LLUUID mTextureId;
    bool  mMovieImageHasMips;
    std::string mMediaURL;          // The last media url set with NavigateTo
    std::string mHomeURL;
    std::string mHomeMimeType;      // forced mime type for home url
    std::string mMimeType;
    std::string mCurrentMediaURL;   // The most current media url from the plugin (via the "location changed" or "navigate complete" events).
    std::string mCurrentMimeType;   // The MIME type that caused the currently loaded plugin to be loaded.
    S32 mLastMouseX;    // save the last mouse coord we get, so when we lose capture we can simulate a mouseup at that point.
    S32 mLastMouseY;

    // Set by mouseDoubleClick() (embedded-browser only), consumed by whichever of mouseUp()/
    // onMouseCaptureLost() actually sends the matching up for that double-click's second press
    // -- both check it since callers vary (LLMediaCtrl fires both in sequence on every click;
    // LLToolPie's prim-media path relies on onMouseCaptureLost() alone). CEF's SendMouseClickEvent()
    // needs the up half of a double-click tagged click_count=2 too, not just the down half, or its
    // internal click-sequence tracking ends up mismatched for the click after this one.
    bool mPendingDoubleClickUp = false;
    S32 mMediaWidth;
    S32 mMediaHeight;
    bool mMediaAutoScale;
    bool mMediaLoop;
    bool mNeedsNewTexture;
    S32 mTextureUsedWidth;
    S32 mTextureUsedHeight;
    bool mSuspendUpdates;
    bool mTextureUpdatePending = false;
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
    // Debounces LLEmbeddedBrowserEventType::ProducerDisconnected -- see
    // EMBEDDED_BROWSER_DISCONNECT_GRACE_PERIOD's own comment in llviewermedia.cpp.
    bool mEmbeddedDisconnectPending = false;
    LLTimer mEmbeddedDisconnectTimer;
    F32 mRequestedVolume;
    F32 mPreviousVolume;
    bool mIsMuted;
    bool mNeedsMuteCheck;
    int mPreviousMediaState;
    F64 mPreviousMediaTime;
    bool mIsDisabled;
    bool mIsParcelMedia;
    S32 mProximity;
    F64 mProximityDistance;
    F64 mProximityCamera;
    bool mMediaAutoPlay;
    std::string mMediaEntryURL;
    bool mInNearbyMediaList;    // used by LLPanelNearbyMedia::refreshList() for performance reasons
    bool mClearCache;
    LLColor4 mBackgroundColor;
    bool mNavigateSuspended;
    bool mNavigateSuspendedDeferred;
    bool mTrustedBrowser;
    std::string mTarget;
    LLNotificationPtr mNotification;
    bool mCleanBrowser;     // force the creation of a clean browsing target with full options enabled
    static std::vector<std::string> sMimeTypesFailed;
    LLPointer<LLImageRaw> mRawImage; //backing buffer for texture updates
private:
    bool mIsUpdated ;
    std::list< LLVOVolume* > mObjectList ;

    void mimeDiscoveryCoro(std::string url);
    LLCoreHttpUtil::HttpCoroutineAdapter::wptr_t mMimeProbe;
    bool mCanceling;

private:
    // embedded_browser_width/height: for the embedded-browser branch only, the exact
    // size to size the GL texture for -- see the caller (preMediaTexUpdate()) for why
    // this must be the SAME snapshot used for the pixel data itself, not a second,
    // independent query of the tab's current size.
    LLViewerMediaTexture *updateMediaImage(S32 embedded_browser_width = -1, S32 embedded_browser_height = -1);
    LL::WorkQueue::weak_t mMainQueue;
    LL::WorkQueue::weak_t mTexUpdateQueue;

};

#endif  // LLVIEWERMEDIA_H
