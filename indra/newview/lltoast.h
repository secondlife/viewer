/** 
 * @file lltoast.h
 * @brief This class implements a placeholder for any notification panel.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLTOAST_H
#define LL_LLTOAST_H


#include "llpanel.h"
#include "llmodaldialog.h"
#include "lleventtimer.h"
#include "llnotificationptr.h"

#include "llviewercontrol.h"
#include "lltexteditor.h"

#define MOUSE_LEAVE false
#define MOUSE_ENTER true

namespace LLNotificationsUI
{

class LLToast;
/**
 * Timer for toasts.
 */
class LLToastLifeTimer: public LLEventTimer
{
public:
	LLToastLifeTimer(LLToast* toast, F32 period);

	/*virtual*/
	BOOL tick();
	void stop();
	void start();
	void restart();
	BOOL getStarted();
	void setPeriod(F32 period);
	F32 getRemainingTimeF32();

	LLTimer&  getEventTimer() { return mEventTimer;}
private :
	LLToast* mToast;
};

/**
 * Represents toast pop-up.
 * This is a parent view for all toast panels.
 */
class LLToast : public LLModalDialog
{
	friend class LLToastLifeTimer;
public:
	typedef boost::function<void (LLToast* toast)> toast_callback_t;
	typedef boost::signals2::signal<void (LLToast* toast)> toast_signal_t;

	struct Params : public LLInitParam::Block<Params>
	{
		Mandatory<LLPanel*>				panel;
		Optional<LLUUID>				notif_id,	 //notification ID
										session_id;	 //im session ID
		Optional<LLNotificationPtr>		notification;

		//NOTE: Life time of a toast (i.e. period of time from the moment toast was shown
		//till the moment when toast was hidden) is the sum of lifetime_secs and fading_time_secs.

		Optional<F32>					lifetime_secs, // Number of seconds while a toast is non-transparent
										fading_time_secs; // Number of seconds while a toast is transparent


		Optional<toast_callback_t>		on_delete_toast,
										on_mouse_enter;
		Optional<bool>					can_fade,
										can_be_stored,
										enable_hide_btn,
										is_modal,
										is_tip,
										force_show,
										force_store;


		Params();
	};

	LLToast(const LLToast::Params& p);
	virtual ~LLToast();
	BOOL postBuild();

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	// Toast handlers
	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);

	//Fading

	/** Stop fading timer */
	virtual void stopFading();

	/** Start fading timer */
	virtual void startFading();

	bool isHovered();

	// Operating with toasts
	// insert a panel to a toast
	void insertPanel(LLPanel* panel);

	void reshapeToPanel();

	// get toast's panel
	LLPanel* getPanel() { return mPanel; }
	// enable/disable Toast's Hide button
	void setHideButtonEnabled(bool enabled);
	//
	F32 getTimeLeftToLive();
	//
	LLToastLifeTimer* getTimer() { return mTimer.get();}
	//
	virtual void draw();
	//
	virtual void setVisible(BOOL show);

	/*virtual*/ void setBackgroundOpaque(BOOL b);
	//
	virtual void hide();

	/*virtual*/ void onFocusLost();

	/*virtual*/ void onFocusReceived();

	void setLifetime(S32 seconds);

	void setFadingTime(S32 seconds);

	/**
	 * Returns padding between floater top and wrapper_panel top.
	 * This padding should be taken into account when positioning or reshaping toasts
	 */
	S32 getTopPad();

	S32 getRightPad();

	// get/set Toast's flags or states
	// get information whether the notification corresponding to the toast is valid or not
	bool isNotificationValid();
	// get toast's Notification ID
	const LLUUID getNotificationID() { return mNotificationID;}
	// get toast's Session ID
	const LLUUID getSessionID() { return mSessionID;}
	//
	void setCanFade(bool can_fade);
	//
	void setCanBeStored(bool can_be_stored) { mCanBeStored = can_be_stored; }
	//
	bool getCanBeStored() { return mCanBeStored; }
	// set whether this toast considered as hidden or not
	void setIsHidden( bool is_toast_hidden ) { mIsHidden = is_toast_hidden; }

	const LLNotificationPtr& getNotification() { return mNotification;}

	// Registers signals/callbacks for events
	toast_signal_t mOnFadeSignal;
	toast_signal_t mOnMouseEnterSignal;
	toast_signal_t mOnDeleteToastSignal;
	toast_signal_t mOnToastDestroyedSignal;
	boost::signals2::connection setOnFadeCallback(toast_callback_t cb) { return mOnFadeSignal.connect(cb); }
	boost::signals2::connection setOnToastDestroyedCallback(toast_callback_t cb) { return mOnToastDestroyedSignal.connect(cb); }

	typedef boost::function<void (LLToast* toast, bool mouse_enter)> toast_hover_check_callback_t;
	typedef boost::signals2::signal<void (LLToast* toast, bool mouse_enter)> toast_hover_check_signal_t;
	toast_hover_check_signal_t mOnToastHoverSignal;	
	boost::signals2::connection setOnToastHoverCallback(toast_hover_check_callback_t cb) { return mOnToastHoverSignal.connect(cb); }

	boost::signals2::connection setMouseEnterCallback( const commit_signal_t::slot_type& cb ) { return mToastMouseEnterSignal.connect(cb); };
	boost::signals2::connection setMouseLeaveCallback( const commit_signal_t::slot_type& cb ) { return mToastMouseLeaveSignal.connect(cb); };

	virtual S32	notifyParent(const LLSD& info);

	LLHandle<LLToast> getHandle() { mHandle.bind(this); return mHandle; }

private:

	void onToastMouseEnter();

	void onToastMouseLeave();

	void expire();

	void setTransparentState(bool transparent);

	LLUUID				mNotificationID;
	LLUUID				mSessionID;
	LLNotificationPtr	mNotification;

	LLRootHandle<LLToast>	mHandle;
		
	LLPanel* mWrapperPanel;

	// timer counts a lifetime of a toast
	std::auto_ptr<LLToastLifeTimer> mTimer;

	F32			mToastLifetime; // in seconds
	F32			mToastFadingTime; // in seconds

	LLPanel*		mPanel;
	LLButton*		mHideBtn;

	LLColor4	mBgColor;
	bool		mCanFade;
	bool		mCanBeStored;
	bool		mHideBtnEnabled;
	bool		mHideBtnPressed;
	bool		mIsHidden;  // this flag is TRUE when a toast has faded or was hidden with (x) button (EXT-1849)
	bool		mIsTip;
	bool		mIsTransparent;

	commit_signal_t mToastMouseEnterSignal;
	commit_signal_t mToastMouseLeaveSignal;
};

}
#endif
