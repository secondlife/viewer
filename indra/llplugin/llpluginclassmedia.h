/** 
 * @file llpluginclassmedia.h
 * @brief LLPluginClassMedia handles interaction with a plugin which knows about the "media" message class.
 *
 * @cond
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
 * @endcond
 */

#ifndef LL_LLPLUGINCLASSMEDIA_H
#define LL_LLPLUGINCLASSMEDIA_H

#include "llgltypes.h"
#include "llpluginprocessparent.h"
#include "llrect.h"
#include "llpluginclassmediaowner.h"
#include <queue>
#include "v4color.h"

class LLPluginClassMedia : public LLPluginProcessParentOwner
{
	LOG_CLASS(LLPluginClassMedia);
public:
	LLPluginClassMedia(LLPluginClassMediaOwner *owner);
	virtual ~LLPluginClassMedia();

	// local initialization, called by the media manager when creating a source
	virtual bool init(const std::string &launcher_filename, 
					  const std::string &plugin_filename, 
					  bool debug);

	// undoes everything init() didm called by the media manager when destroying a source
	virtual void reset();
	
	void idle(void);
	
	// All of these may return 0 or an actual valid value.
	// Callers need to check the return for 0, and not use the values in that case.
	int getWidth() const { return (mMediaWidth > 0) ? mMediaWidth : 0; };
	int getHeight() const { return (mMediaHeight > 0) ? mMediaHeight : 0; };
	int getNaturalWidth() const { return mNaturalMediaWidth; };
	int getNaturalHeight() const { return mNaturalMediaHeight; };
	int getSetWidth() const { return mSetMediaWidth; };
	int getSetHeight() const { return mSetMediaHeight; };
	int getBitsWidth() const { return (mTextureWidth > 0) ? mTextureWidth : 0; };
	int getBitsHeight() const { return (mTextureHeight > 0) ? mTextureHeight : 0; };
	int getTextureWidth() const;
	int getTextureHeight() const;
	int getFullWidth() const { return mFullMediaWidth; };
	int getFullHeight() const { return mFullMediaHeight; };
	
	// This may return NULL.  Callers need to check for and handle this case.
	unsigned char* getBitsData();

	// gets the format details of the texture data
	// These may return 0 if they haven't been set up yet.  The caller needs to detect this case.
	int getTextureDepth() const { return mRequestedTextureDepth; };
	int getTextureFormatInternal() const { return mRequestedTextureInternalFormat; };
	int getTextureFormatPrimary() const { return mRequestedTextureFormat; };
	int getTextureFormatType() const { return mRequestedTextureType; };
	bool getTextureFormatSwapBytes() const { return mRequestedTextureSwapBytes; };
	bool getTextureCoordsOpenGL() const { return mRequestedTextureCoordsOpenGL; };

	void setSize(int width, int height);
	void setAutoScale(bool auto_scale);
	
	void setBackgroundColor(LLColor4 color) { mBackgroundColor = color; };
	
	// Returns true if all of the texture parameters (depth, format, size, and texture size) are set up and consistent.
	// This will initially be false, and will also be false for some time after setSize while the resize is processed.
	// Note that if this returns true, it is safe to use all the get() functions above without checking for invalid return values
	// until you call idle() again.
	bool textureValid(void);
	
	bool getDirty(LLRect *dirty_rect = NULL);
	void resetDirty(void);
	
	typedef enum 
	{
		MOUSE_EVENT_DOWN,
		MOUSE_EVENT_UP,
		MOUSE_EVENT_MOVE,
		MOUSE_EVENT_DOUBLE_CLICK
	}EMouseEventType;
	
	void mouseEvent(EMouseEventType type, int button, int x, int y, MASK modifiers);

	typedef enum 
	{
		KEY_EVENT_DOWN,
		KEY_EVENT_UP,
		KEY_EVENT_REPEAT
	}EKeyEventType;
	
	bool keyEvent(EKeyEventType type, int key_code, MASK modifiers, LLSD native_key_data);

	void scrollEvent(int x, int y, MASK modifiers);
	
	// Text may be unicode (utf8 encoded)
	bool textInput(const std::string &text, MASK modifiers, LLSD native_key_data);
	
	void loadURI(const std::string &uri);
	
	// "Loading" means uninitialized or any state prior to fully running (processing commands)
	bool isPluginLoading(void) { return mPlugin?mPlugin->isLoading():false; };

	// "Running" means the steady state -- i.e. processing messages
	bool isPluginRunning(void) { return mPlugin?mPlugin->isRunning():false; };
	
	// "Exited" means any regular or error state after "Running" (plugin may have crashed or exited normally)
	bool isPluginExited(void) { return mPlugin?mPlugin->isDone():false; };

	std::string getPluginVersion() { return mPlugin?mPlugin->getPluginVersion():std::string(""); };

	bool getDisableTimeout() { return mPlugin?mPlugin->getDisableTimeout():false; };
	void setDisableTimeout(bool disable) { if(mPlugin) mPlugin->setDisableTimeout(disable); };
	
	// Inherited from LLPluginProcessParentOwner
	/* virtual */ void receivePluginMessage(const LLPluginMessage &message);
	/* virtual */ void pluginLaunchFailed();
	/* virtual */ void pluginDied();
	
	
	typedef enum 
	{
		PRIORITY_UNLOADED,	// media plugin isn't even loaded.
		PRIORITY_STOPPED,	// media is not playing, shouldn't need to update at all.
		PRIORITY_HIDDEN,	// media is not being displayed or is out of view, don't need to do graphic updates, but may still update audio, playhead, etc.
		PRIORITY_SLIDESHOW,	// media is in the far distance, updates very infrequently
		PRIORITY_LOW,		// media is in the distance, may be rendered at reduced size
		PRIORITY_NORMAL,	// normal (default) priority
		PRIORITY_HIGH		// media has user focus and/or is taking up most of the screen
	}EPriority;

	static const char* priorityToString(EPriority priority);
	void setPriority(EPriority priority);
	void setLowPrioritySizeLimit(int size);
	
	F64 getCPUUsage();
	
	void sendPickFileResponse(const std::string &file);

	void sendAuthResponse(bool ok, const std::string &username, const std::string &password);

	// Valid after a MEDIA_EVENT_CURSOR_CHANGED event
	std::string getCursorName() const { return mCursorName; };

	LLPluginClassMediaOwner::EMediaStatus getStatus() const { return mStatus; }

	void	cut();
	bool	canCut() const { return mCanCut; };

	void	copy();
	bool	canCopy() const { return mCanCopy; };

	void	paste();
	bool	canPaste() const { return mCanPaste; };
	
	// These can be called before init(), and they will be queued and sent before the media init message.
	void	setUserDataPath(const std::string &user_data_path);
	void	setLanguageCode(const std::string &language_code);
	void	setPluginsEnabled(const bool enabled);
	void	setJavascriptEnabled(const bool enabled);
	void	setTarget(const std::string &target);
	
	///////////////////////////////////
	// media browser class functions
	bool pluginSupportsMediaBrowser(void);
	
	void focus(bool focused);
	void clear_cache();
	void clear_cookies();
	void set_cookies(const std::string &cookies);
	void enable_cookies(bool enable);
	void proxy_setup(bool enable, const std::string &host = LLStringUtil::null, int port = 0);
	void browse_stop();
	void browse_reload(bool ignore_cache = false);
	void browse_forward();
	void browse_back();
	void set_status_redirect(int code, const std::string &url);
	void setBrowserUserAgent(const std::string& user_agent);
	void proxyWindowOpened(const std::string &target, const std::string &uuid);
	void proxyWindowClosed(const std::string &uuid);
	void ignore_ssl_cert_errors(bool ignore);
	
	// This is valid after MEDIA_EVENT_NAVIGATE_BEGIN or MEDIA_EVENT_NAVIGATE_COMPLETE
	std::string	getNavigateURI() const { return mNavigateURI; };

	// These are valid after MEDIA_EVENT_NAVIGATE_COMPLETE
	S32			getNavigateResultCode() const { return mNavigateResultCode; };
	std::string getNavigateResultString() const { return mNavigateResultString; };
	bool		getHistoryBackAvailable() const { return mHistoryBackAvailable; };
	bool		getHistoryForwardAvailable() const { return mHistoryForwardAvailable; };

	// This is valid after MEDIA_EVENT_PROGRESS_UPDATED
	int			getProgressPercent() const { return mProgressPercent; };
	
	// This is valid after MEDIA_EVENT_STATUS_TEXT_CHANGED
	std::string getStatusText() const { return mStatusText; };
	
	// This is valid after MEDIA_EVENT_LOCATION_CHANGED
	std::string getLocation() const { return mLocation; };
	
	// This is valid after MEDIA_EVENT_CLICK_LINK_HREF or MEDIA_EVENT_CLICK_LINK_NOFOLLOW
	std::string getClickURL() const { return mClickURL; };

	// This is valid after MEDIA_EVENT_CLICK_LINK_HREF
	std::string getClickTarget() const { return mClickTarget; };

	// This is valid during MEDIA_EVENT_CLICK_LINK_HREF and MEDIA_EVENT_GEOMETRY_CHANGE
	std::string getClickUUID() const { return mClickUUID; };
	
	// These are valid during MEDIA_EVENT_GEOMETRY_CHANGE
	S32 getGeometryX() const { return mGeometryX; };
	S32 getGeometryY() const { return mGeometryY; };
	S32 getGeometryWidth() const { return mGeometryWidth; };
	S32 getGeometryHeight() const { return mGeometryHeight; };
	
	// These are valid during MEDIA_EVENT_AUTH_REQUEST
	std::string	getAuthURL() const { return mAuthURL; };
	std::string	getAuthRealm() const { return mAuthRealm; };

	std::string getMediaName() const { return mMediaName; };
	std::string getMediaDescription() const { return mMediaDescription; };

	// Crash the plugin.  If you use this outside of a testbed, you will be punished.
	void		crashPlugin();
	
	// Hang the plugin.  If you use this outside of a testbed, you will be punished.
	void		hangPlugin();

	///////////////////////////////////
	// media time class functions
	bool pluginSupportsMediaTime(void);
	void stop();
	void start(float rate = 0.0f);
	void pause();
	void seek(float time);
	void setLoop(bool loop);
	void setVolume(float volume);
	float getVolume();
	
	F64 getCurrentTime(void) const { return mCurrentTime; };
	F64 getDuration(void) const { return mDuration; };
	F64 getCurrentPlayRate(void) { return mCurrentRate; };
	F64 getLoadedDuration(void) const { return mLoadedDuration; };
	
	// Initialize the URL history of the plugin by sending
	// "init_history" message 
	void initializeUrlHistory(const LLSD& url_history);

protected:

	LLPluginClassMediaOwner *mOwner;

	// Notify this object's owner that an event has occurred.
	void mediaEvent(LLPluginClassMediaOwner::EMediaEvent event);
		
	void sendMessage(const LLPluginMessage &message);  // Send message internally, either queueing or sending directly.
	std::queue<LLPluginMessage> mSendQueue;		// Used to queue messages while the plugin initializes.
	
	void setSizeInternal(void);

	bool		mTextureParamsReceived;		// the mRequestedTexture* fields are only valid when this is true
	S32 		mRequestedTextureDepth;
	LLGLenum	mRequestedTextureInternalFormat;
	LLGLenum	mRequestedTextureFormat;
	LLGLenum	mRequestedTextureType;
	bool		mRequestedTextureSwapBytes;
	bool		mRequestedTextureCoordsOpenGL;
	
	std::string mTextureSharedMemoryName;
	size_t		mTextureSharedMemorySize;
	
	// True to scale requested media up to the full size of the texture (i.e. next power of two)
	bool		mAutoScaleMedia;

	// default media size for the plugin, from the texture_params message.
	int			mDefaultMediaWidth;
	int			mDefaultMediaHeight;

	// Size that has been requested by the plugin itself
	int			mNaturalMediaWidth;
	int			mNaturalMediaHeight;

	// Size that has been requested with setSize()
	int			mSetMediaWidth;
	int			mSetMediaHeight;
	
	// Full calculated media size (before auto-scale and downsample calculations)
	int			mFullMediaWidth;
	int			mFullMediaHeight;

	// Actual media size being set (after auto-scale)
	int			mRequestedMediaWidth;
	int			mRequestedMediaHeight;
	
	// Texture size calculated from actual media size
	int			mRequestedTextureWidth;
	int			mRequestedTextureHeight;
	
	// Size that the plugin has acknowledged
	int			mTextureWidth;
	int			mTextureHeight;
	int			mMediaWidth;
	int			mMediaHeight;
	
	float		mRequestedVolume;
	
	// Priority of this media stream
	EPriority	mPriority;
	int			mLowPrioritySizeLimit;
	
	bool		mAllowDownsample;
	int			mPadding;
	
	
	LLPluginProcessParent *mPlugin;
	
	LLRect mDirtyRect;
	
	std::string translateModifiers(MASK modifiers);
	
	std::string mCursorName;
	int			mLastMouseX;
	int			mLastMouseY;

	LLPluginClassMediaOwner::EMediaStatus mStatus;
	
	F64				mSleepTime;

	bool			mCanCut;
	bool			mCanCopy;
	bool			mCanPaste;
	
	std::string		mMediaName;
	std::string		mMediaDescription;
	
	LLColor4		mBackgroundColor;
	
	std::string		mTarget;
	
	/////////////////////////////////////////
	// media_browser class
	std::string		mNavigateURI;
	S32				mNavigateResultCode;
	std::string		mNavigateResultString;
	bool			mHistoryBackAvailable;
	bool			mHistoryForwardAvailable;
	std::string		mStatusText;
	int				mProgressPercent;
	std::string		mLocation;
	std::string		mClickURL;
	std::string		mClickTarget;
	std::string		mClickUUID;
	S32				mGeometryX;
	S32				mGeometryY;
	S32				mGeometryWidth;
	S32				mGeometryHeight;
	std::string		mAuthURL;
	std::string		mAuthRealm;
	
	/////////////////////////////////////////
	// media_time class
	F64				mCurrentTime;
	F64				mDuration;
	F64				mCurrentRate;
	F64				mLoadedDuration;
	
//--------------------------------------
	//debug use only
	//
private:
	bool  mDeleteOK ;
public:
	void setDeleteOK(bool flag) { mDeleteOK = flag ;}
//--------------------------------------
};

#endif // LL_LLPLUGINCLASSMEDIA_H
