/** 
 * @file llscreenchannel.h
 * @brief Class implements a channel on a screen in which appropriate toasts may appear.
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

#ifndef LL_LLSCREENCHANNEL_H
#define LL_LLSCREENCHANNEL_H

#include "lltoast.h"

#include <map>
#include <boost/shared_ptr.hpp>

namespace LLNotificationsUI
{

typedef enum e_notification_toast_alignment
{
	NA_TOP, 
	NA_CENTRE,
	NA_BOTTOM,
} EToastAlignment;

typedef enum e_channel_alignment
{
	CA_LEFT, 
	CA_CENTRE,
	CA_RIGHT,
} EChannelAlignment;

class LLScreenChannelBase : public LLUICtrl
{
	friend class LLChannelManager;
public:
	LLScreenChannelBase(const LLUUID& id);
	~LLScreenChannelBase();

	// Channel's outfit-functions
	// update channel's size and position in the World View
	virtual void		updatePositionAndSize(LLRect old_world_rect, LLRect new_world_rect);
	// initialization of channel's shape and position
	virtual void		init(S32 channel_left, S32 channel_right);


	virtual void		setToastAlignment(EToastAlignment align) {mToastAlignment = align;}
	
	virtual void		setChannelAlignment(EChannelAlignment align) {mChannelAlignment = align;}
	virtual void		setOverflowFormatString ( const std::string& str)  { mOverflowFormatString = str; }
	
	// kill or modify a toast by its ID
	virtual void		killToastByNotificationID(LLUUID id) {};
	virtual void		modifyToastNotificationByID(LLUUID id, LLSD data) {};
	
	// hide all toasts from screen, but not remove them from a channel
	virtual void		hideToastsFromScreen() {};
	// removes all toasts from a channel
	virtual void		removeToastsFromChannel() {};
	
	// show all toasts in a channel
	virtual void		redrawToasts() {};

	virtual void 		closeOverflowToastPanel() {};
	virtual void 		hideOverflowToastPanel() {};

	
	// Channel's behavior-functions
	// set whether a channel will control hovering inside itself or not
	virtual void setControlHovering(bool control) { mControlHovering = control; }
	

	bool isHovering();

	void setCanStoreToasts(bool store) { mCanStoreToasts = store; }

	void setDisplayToastsAlways(bool display_toasts) { mDisplayToastsAlways = display_toasts; }
	bool getDisplayToastsAlways() { return mDisplayToastsAlways; }

	// get number of hidden notifications from a channel
	S32	 getNumberOfHiddenToasts() { return mHiddenToastsNum;}

	
	void setShowToasts(bool show) { mShowToasts = show; }
	bool getShowToasts() { return mShowToasts; }

	// get toast allignment preset for a channel
	e_notification_toast_alignment getToastAlignment() {return mToastAlignment;}
	
	// get ID of a channel
	LLUUID	getChannelID() { return mID; }

protected:
	// Channel's flags
	bool		mControlHovering;
	LLToast*		mHoveredToast;
	bool		mCanStoreToasts;
	bool		mDisplayToastsAlways;
	bool		mOverflowToastHidden;
	// controls whether a channel shows toasts or not
	bool		mShowToasts;
	// 
	EToastAlignment		mToastAlignment;
	EChannelAlignment	mChannelAlignment;

	// attributes for the Overflow Toast
	S32			mHiddenToastsNum;
	LLToast*	mOverflowToastPanel;	
	std::string mOverflowFormatString;

	// channel's ID
	LLUUID	mID;

	// store a connection to prevent futher crash that is caused by sending a signal to a destroyed channel
	boost::signals2::connection mWorldViewRectConnection;
};


/**
 * Screen channel manages toasts visibility and positioning on the screen.
 */
class LLScreenChannel : public LLScreenChannelBase
{
	friend class LLChannelManager;
public:
	LLScreenChannel(LLUUID& id);
	virtual ~LLScreenChannel();

	class Matcher
	{
	public:
		Matcher(){}
		virtual ~Matcher() {}
		virtual bool matches(const LLNotificationPtr) const = 0;
	};

	std::list<LLToast*> findToasts(const Matcher& matcher);

	// Channel's outfit-functions
	// update channel's size and position in the World View
	void		updatePositionAndSize(LLRect old_world_rect, LLRect new_world_rect);
	// initialization of channel's shape and position
	void		init(S32 channel_left, S32 channel_right);
	
	// Operating with toasts
	// add a toast to a channel
	void		addToast(const LLToast::Params& p);
	// kill or modify a toast by its ID
	void		killToastByNotificationID(LLUUID id);
	void		killMatchedToasts(const Matcher& matcher);
	void		modifyToastByNotificationID(LLUUID id, LLPanel* panel);
	// hide all toasts from screen, but not remove them from a channel
	void		hideToastsFromScreen();
	// hide toast by notification id
	void		hideToast(const LLUUID& notification_id);
	// removes all toasts from a channel
	void		removeToastsFromChannel();
	// show all toasts in a channel
	void		redrawToasts();
	//
	void		loadStoredToastsToChannel();
	// finds a toast among stored by its Notification ID and throws it on a screen to a channel
	void		loadStoredToastByNotificationIDToChannel(LLUUID id);
	// removes a toast from stored finding it by its Notification ID 
	void		removeStoredToastByNotificationID(LLUUID id);
	// removes from channel all toasts that belongs to the certain IM session 
	void		removeToastsBySessionID(LLUUID id);
	// remove all storable toasts from screen and store them
	void		removeAndStoreAllStorableToasts();
	// close the Overflow Toast
	void 		closeOverflowToastPanel();
	// close the StartUp Toast
	void		closeStartUpToast();


	/** Stop fading all toasts */
	virtual void stopFadingToasts();

	/** Start fading all toasts */
	virtual void startFadingToasts();

	// get StartUp Toast's state
	static bool	getStartUpToastShown() { return mWasStartUpToastShown; }
	// tell all channels that the StartUp toast was shown and allow them showing of toasts
	static void	setStartUpToastShown() { mWasStartUpToastShown = true; }
	// let a channel update its ShowToast flag
	void updateShowToastsState();


	// Channel's other interface functions functions
	// update number of notifications in the StartUp Toast
	void	updateStartUpString(S32 num);

	LLToast* getToastByNotificationID(LLUUID id);

	// Channel's signals
	// signal on storing of faded toasts event
	typedef boost::function<void (LLPanel* info_panel, const LLUUID id)> store_tost_callback_t;
	typedef boost::signals2::signal<void (LLPanel* info_panel, const LLUUID id)> store_tost_signal_t;
	store_tost_signal_t mOnStoreToast;	
	boost::signals2::connection setOnStoreToastCallback(store_tost_callback_t cb) { return mOnStoreToast.connect(cb); }
	// signal on rejecting of a toast event
	typedef boost::function<void (LLUUID id)> reject_tost_callback_t;
	typedef boost::signals2::signal<void (LLUUID id)> reject_tost_signal_t;
	reject_tost_signal_t mRejectToastSignal; boost::signals2::connection setOnRejectToastCallback(reject_tost_callback_t cb) { return mRejectToastSignal.connect(cb); }

private:
	struct ToastElem
	{
		LLUUID		id;
		LLToast*	toast;

		ToastElem(LLToast::Params p) : id(p.notif_id)
		{
			toast = new LLToast(p);
		}

		ToastElem(const ToastElem& toast_elem)
		{
			id = toast_elem.id;
			toast = toast_elem.toast;
		}

		bool operator == (const LLUUID &id_op) const
		{
			return (id == id_op);
		}

		bool operator == (LLPanel* panel_op) const
		{
			return (toast == panel_op);
		}
	};

	// Channel's handlers
	void	onToastHover(LLToast* toast, bool mouse_enter);
	void	onToastFade(LLToast* toast);
	void	onToastDestroyed(LLToast* toast);
	void	onOverflowToastHide();
	void	onStartUpToastHide();

	//
	void	storeToast(ToastElem& toast_elem);
	// send signal to observers about destroying of a toast, update channel's Hovering state, close the toast
	void	deleteToast(LLToast* toast);
	
	// show-functions depending on allignment of toasts
	void	showToastsBottom();
	void	showToastsCentre();
	void	showToastsTop();
	
	// create the Overflow Toast
	void	createOverflowToast(S32 bottom, F32 timer);

	// create the StartUp Toast
	void	createStartUpToast(S32 notif_num, F32 timer);

	/**
	 * Notification channel and World View ratio(0.0 - always show 1 notification, 1.0 - max ratio).
	 */
	static F32 getHeightRatio();

	S32 getOverflowToastHeight();

	// Channel's flags
	static bool	mWasStartUpToastShown;

	// attributes for the StartUp Toast	
	LLToast* mStartUpToastPanel;


	std::vector<ToastElem>		mToastList;
	std::vector<ToastElem>		mStoredToastList;
};

}
#endif
