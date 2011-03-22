/**
 * @file llmediactrl.h
 * @brief Web browser UI control
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLMediaCtrl_H
#define LL_LLMediaCtrl_H

#include "llviewermedia.h"

#include "lluictrl.h"
#include "llframetimer.h"

class LLViewBorder;
class LLUICtrlFactory;

////////////////////////////////////////////////////////////////////////////////
//
class LLMediaCtrl :
	public LLPanel,
	public LLViewerMediaObserver,
	public LLViewerMediaEventEmitter,
	public LLInstanceTracker<LLMediaCtrl, LLUUID>
{
	LOG_CLASS(LLMediaCtrl);
public:
	struct Params : public LLInitParam::Block<Params, LLPanel::Params> 
	{
		Optional<std::string>	start_url;
		
		Optional<bool>			border_visible,
								ignore_ui_scale,
								hide_loading,
								decouple_texture_size,
								trusted_content,
								focus_on_click;
								
		Optional<S32>			texture_width,
								texture_height;
		
		Optional<LLUIColor>		caret_color;

		Optional<std::string>	initial_mime_type;
		Optional<std::string>	media_id;
		Optional<std::string>	error_page_url;
		
		Params();
	};
	
protected:
	LLMediaCtrl(const Params&);
	friend class LLUICtrlFactory;

public:
		virtual ~LLMediaCtrl();

		void setBorderVisible( BOOL border_visible );

		// For the tutorial window, we don't want to take focus on clicks,
		// as the examples include how to move around with the arrow
		// keys.  Thus we keep focus on the app by setting this false.
		// Defaults to true.
		void setTakeFocusOnClick( bool take_focus );

		// handle mouse related methods
		virtual BOOL handleHover( S32 x, S32 y, MASK mask );
		virtual BOOL handleMouseUp( S32 x, S32 y, MASK mask );
		virtual BOOL handleMouseDown( S32 x, S32 y, MASK mask );
		virtual BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
		virtual BOOL handleRightMouseUp(S32 x, S32 y, MASK mask);
		virtual BOOL handleDoubleClick( S32 x, S32 y, MASK mask );
		virtual BOOL handleScrollWheel( S32 x, S32 y, S32 clicks );
		virtual BOOL handleToolTip(S32 x, S32 y, MASK mask);

		// navigation
		void navigateTo( std::string url_in, std::string mime_type = "");
		void navigateBack();
		void navigateHome();
		void navigateForward();	
		void navigateToLocalPage( const std::string& subdir, const std::string& filename_in );
		bool canNavigateBack();
		bool canNavigateForward();
		std::string getCurrentNavUrl();

		// By default, we do not handle "secondlife:///app/" SLURLs, because
		// those can cause teleports, open windows, etc.  We cannot be sure
		// that each "click" is actually due to a user action, versus 
		// Javascript or some other mechanism.  However, we need the search
		// floater and login page to handle these URLs.  Those are safe
		// because we control the page content.  See DEV-9530.  JC.
		void setHomePageUrl( const std::string& urlIn, const std::string& mime_type = LLStringUtil::null );
		std::string getHomePageUrl();

		void setTarget(const std::string& target);

		void setErrorPageURL(const std::string& url);
		const std::string& getErrorPageURL();

		// Clear the browser cache when the instance gets loaded
		void clearCache();

		// accessor/mutator for flag that indicates if frequent updates to texture happen
		bool getFrequentUpdates() { return mFrequentUpdates; };
		void setFrequentUpdates( bool frequentUpdatesIn ) {  mFrequentUpdates = frequentUpdatesIn; };

		void setIgnoreUIScale(bool ignore) { mIgnoreUIScale = ignore; }
		bool getIgnoreUIScale() { return mIgnoreUIScale; }

		void setAlwaysRefresh(bool refresh) { mAlwaysRefresh = refresh; }
		bool getAlwaysRefresh() { return mAlwaysRefresh; }
		
		void setForceUpdate(bool force_update) { mForceUpdate = force_update; }
		bool getForceUpdate() { return mForceUpdate; }

		bool ensureMediaSourceExists();
		void unloadMediaSource();
		
		LLPluginClassMedia* getMediaPlugin();

		bool setCaretColor( unsigned int red, unsigned int green, unsigned int blue );
		
		void setDecoupleTextureSize(bool decouple) { mDecoupleTextureSize = decouple; }
		bool getDecoupleTextureSize() { return mDecoupleTextureSize; }

		void setTextureSize(S32 width, S32 height);

		void showNotification(boost::shared_ptr<class LLNotification> notify);
		void hideNotification();

		// over-rides
		virtual BOOL handleKeyHere( KEY key, MASK mask);
		virtual void handleVisibilityChange ( BOOL new_visibility );
		virtual BOOL handleUnicodeCharHere(llwchar uni_char);
		virtual void reshape( S32 width, S32 height, BOOL called_from_parent = TRUE);
		virtual void draw();
		virtual BOOL postBuild();

		// focus overrides
		void onFocusLost();
		void onFocusReceived();
		
		// Incoming media event dispatcher
		virtual void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);

	protected:
		void convertInputCoords(S32& x, S32& y);

	private:
		void onVisibilityChange ( const LLSD& new_visibility );
		void onPopup(const LLSD& notification, const LLSD& response);

		const S32 mTextureDepthBytes;
		LLUUID mMediaTextureID;
		LLViewBorder* mBorder;
		bool mFrequentUpdates;
		bool mForceUpdate;
		const bool mTrusted;
		std::string mHomePageUrl;
		std::string mHomePageMimeType;
		std::string mCurrentNavUrl;
		std::string mErrorPageURL;
		std::string mTarget;
		bool mIgnoreUIScale;
		bool mAlwaysRefresh;
		viewer_media_t mMediaSource;
		bool mTakeFocusOnClick;
		bool mStretchToFill;
		bool mMaintainAspectRatio;
		bool mHideLoading;
		bool mHidingInitialLoad;
		bool mDecoupleTextureSize;
		S32 mTextureWidth;
		S32 mTextureHeight;
		bool mClearCache;
		class LLWindowShade* mWindowShade;
		bool mHoverTextChanged;
};

#endif // LL_LLMediaCtrl_H
