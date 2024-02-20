/** 
 * @file llscreenchannel.h
 * @brief Class implements a channel on a screen in which appropriate toasts may appear.
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
	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Mandatory<LLUUID>			id;
		Optional<bool>				display_toasts_always;
		Optional<EToastAlignment>	toast_align;
		Optional<EChannelAlignment>	channel_align;

		Params()
		:	id("id", LLUUID("")), 
			display_toasts_always("display_toasts_always", false), 
			toast_align("toast_align", NA_BOTTOM), 
			channel_align("channel_align", CA_LEFT)
		{}
	};

	LLScreenChannelBase(const Params&);
	
	bool postBuild();

	void reshape(S32 width, S32 height, bool called_from_parent = true);

	// Channel's outfit-functions
	// update channel's size and position in the World View
	virtual void		updatePositionAndSize(LLRect rect);

	// initialization of channel's shape and position
	virtual void		init(S32 channel_left, S32 channel_right);

	// kill or modify a toast by its ID
	virtual void		killToastByNotificationID(LLUUID id) {};
	virtual void		modifyToastNotificationByID(LLUUID id, LLSD data) {};
	virtual void		removeToastByNotificationID(LLUUID id){};
	
	// hide all toasts from screen, but not remove them from a channel
	virtual void		hideToastsFromScreen() {};
	// removes all toasts from a channel
	virtual void		removeToastsFromChannel() {};
	
	// show all toasts in a channel
	virtual void		redrawToasts() {};

	
	// Channel's behavior-functions
	// set whether a channel will control hovering inside itself or not
	virtual void setControlHovering(bool control) { mControlHovering = control; }
	

	bool isHovering();

	void setCanStoreToasts(bool store) { mCanStoreToasts = store; }

	bool getDisplayToastsAlways() { return mDisplayToastsAlways; }

	// get number of hidden notifications from a channel
	S32	 getNumberOfHiddenToasts() { return mHiddenToastsNum;}

	
	void setShowToasts(bool show) { mShowToasts = show; }
	bool getShowToasts() { return mShowToasts; }

	// get toast allignment preset for a channel
	e_notification_toast_alignment getToastAlignment() {return mToastAlignment;}
	
	// get ID of a channel
	LLUUID	getChannelID() { return mID; }
	LLHandle<LLScreenChannelBase> getHandle() { return getDerivedHandle<LLScreenChannelBase>(); }

protected:
	void	updateRect();
	LLRect	getChannelRect();

	// Channel's flags
	bool		mControlHovering;
	LLToast*	mHoveredToast;
	bool		mCanStoreToasts;
	bool		mDisplayToastsAlways;
	// controls whether a channel shows toasts or not
	bool		mShowToasts;
	// 
	EToastAlignment		mToastAlignment;
	EChannelAlignment	mChannelAlignment;

	S32			mHiddenToastsNum;

	// channel's ID
	LLUUID	mID;
	
	LLView*	mFloaterSnapRegion;
	LLView* mChicletRegion;
};


/**
 * Screen channel manages toasts visibility and positioning on the screen.
 */
class LLScreenChannel : public LLScreenChannelBase
{
	friend class LLChannelManager;
public:
	LLScreenChannel(const Params&);
	virtual ~LLScreenChannel();

	class Matcher
	{
	public:
		Matcher(){}
		virtual ~Matcher() {}
		virtual bool matches(const LLNotificationPtr) const = 0;
	};

	std::list<const LLToast*> findToasts(const Matcher& matcher);

	// Channel's outfit-functions
	// update channel's size and position in the World View
	void		updatePositionAndSize(LLRect new_rect);
	// initialization of channel's shape and position
	void		init(S32 channel_left, S32 channel_right);
	
	// Operating with toasts
	// add a toast to a channel
	void		addToast(const LLToast::Params& p);
	// kill or modify a toast by its ID
	void		killToastByNotificationID(LLUUID id);
	void		removeToastByNotificationID(LLUUID id);
	void		killMatchedToasts(const Matcher& matcher);
	void		modifyToastByNotificationID(LLUUID id, LLPanel* panel);
	// hide all toasts from screen, but not remove them from a channel
	void		hideToastsFromScreen();
	// hide toast by notification id
	void		hideToast(const LLUUID& notification_id);

	/**
	 * Closes hidden matched toasts from channel.
	 */
	void closeHiddenToasts(const Matcher& matcher);

	// removes all toasts from a channel
	void		removeToastsFromChannel();
	// show all toasts in a channel
	void		redrawToasts();
	//
	void		loadStoredToastsToChannel();
	// finds a toast among stored by its Notification ID and throws it on a screen to a channel
	void		loadStoredToastByNotificationIDToChannel(LLUUID id);
	// removes from channel all toasts that belongs to the certain IM session 
	void		removeToastsBySessionID(LLUUID id);
	// remove all storable toasts from screen and store them
	void		removeAndStoreAllStorableToasts();
	// close the StartUp Toast
	void		closeStartUpToast();


	/** Stop fading given toast */
	virtual void stopToastTimer(LLToast* toast);

	/** Start fading given toast */
	virtual void startToastTimer(LLToast* toast);

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
	typedef boost::signals2::signal<void (LLPanel* info_panel, const LLUUID id)> store_toast_signal_t;
	boost::signals2::connection addOnStoreToastCallback(store_toast_signal_t::slot_type cb) { return mOnStoreToast.connect(cb); }

private:
	store_toast_signal_t mOnStoreToast;	

	class ToastElem
	{
	public:
		ToastElem(const LLHandle<LLToast>& toast) : mToast(toast)
		{
		}

		ToastElem(const ToastElem& toast_elem) : mToast(toast_elem.mToast)
		{
		}

		LLToast* getToast() const
		{
			return mToast.get();
		}

		LLUUID getID() const
		{
			return mToast.isDead() ? LLUUID() : mToast.get()->getNotificationID();
		}

		bool operator == (const LLUUID &id_op) const
		{
			return (getID() == id_op);
		}

		bool operator == (LLPanel* panel_op) const
		{
			return (mToast.get() == panel_op);
		}

	private:
		LLHandle<LLToast>	mToast;
	};

	// Channel's handlers
	void	onToastHover(LLToast* toast, bool mouse_enter);
	void	onToastFade(LLToast* toast);
	void	onToastDestroyed(LLToast* toast);
	void	onStartUpToastHide();

	//
	void	storeToast(ToastElem& toast_elem);
	// send signal to observers about destroying of a toast, update channel's Hovering state, close the toast
	void	deleteToast(LLToast* toast);
	
	// show-functions depending on allignment of toasts
	void	showToastsBottom();
	void	showToastsCentre();
	void	showToastsTop();
	
	// create the StartUp Toast
	void	createStartUpToast(S32 notif_num, F32 timer);

	/**
	 * Notification channel and World View ratio(0.0 - always show 1 notification, 1.0 - max ratio).
	 */
	static F32 getHeightRatio();

	// Channel's flags
	static bool	mWasStartUpToastShown;

	// attributes for the StartUp Toast	
	LLToast* mStartUpToastPanel;


	std::vector<ToastElem>		mToastList;
	std::vector<ToastElem>		mStoredToastList;
};

}
#endif
