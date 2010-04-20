/** 
 * @file lltoast.h
 * @brief This class implements a placeholder for any notification panel.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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
	LLToastLifeTimer(LLToast* toast, F32 period) : mToast(toast), LLEventTimer(period){}

	/*virtual*/
	BOOL tick();
	void stop() { mEventTimer.stop(); }
	void start() { mEventTimer.start(); }
	void restart() {mEventTimer.reset(); }
	BOOL getStarted() { return mEventTimer.getStarted(); }

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
		Optional<F32>					lifetime_secs,
										fading_time_secs; // Number of seconds while a toast is fading
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
	void resetTimer() { mTimer->start(); }
	//
	void stopTimer() { mTimer->stop(); }
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

private:

	void onToastMouseEnter();

	void onToastMouseLeave();

	void handleTipToastClick(S32 x, S32 y, MASK mask);

	void	expire();

	LLUUID				mNotificationID;
	LLUUID				mSessionID;
	LLNotificationPtr	mNotification;

	LLPanel* mWrapperPanel;

	// timer counts a lifetime of a toast
	std::auto_ptr<LLToastLifeTimer> mTimer;

	F32			mToastFadingTime; // in seconds

	LLPanel*		mPanel;
	LLButton*		mHideBtn;
	LLTextEditor*	mTextEditor;

	LLColor4	mBgColor;
	bool		mCanFade;
	bool		mCanBeStored;
	bool		mHideBtnEnabled;
	bool		mHideBtnPressed;
	bool		mIsHidden;  // this flag is TRUE when a toast has faded or was hidden with (x) button (EXT-1849)
	bool		mIsTip;

	commit_signal_t mToastMouseEnterSignal;
	commit_signal_t mToastMouseLeaveSignal;
};

}
#endif
