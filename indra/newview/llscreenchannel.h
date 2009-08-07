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


/**
 * Screen channel manages toasts visibility and positioning on the screen.
 */
class LLScreenChannel : public LLUICtrl
{
	friend class LLChannelManager;
public:
	LLScreenChannel();
	virtual ~LLScreenChannel();

	// Channel's outfit-functions
	// classic reshape
	void		reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	// initialization of channel's shape and position
	void		init(S32 channel_left, S32 channel_right);
	// set allignment of toasts inside a channel
	void		setToastAlignment(e_notification_toast_alignment align) {mToastAlignment = align;}
	// set a template for a string in the OverflowToast
	void		setOverflowFormatString ( std::string str)  { mOverflowFormatString = str; }
	
	// Operating with toasts
	// add a toast to a channel
	LLToast*	addToast(LLUUID id, LLPanel* panel, bool is_not_tip = true);
	// kill or modify a toast by its ID
	void		killToastByNotificationID(LLUUID id);
	void		modifyToastByNotificationID(LLUUID id, LLPanel* panel);
	// hide all toasts from screen, but not remove them from a channel
	void		hideToastsFromScreen();
	// removes all toasts from a channel
	void		removeToastsFromChannel();
	// show all toasts in a channel
	void		showToasts();
	//
	void		loadStoredToastsToChannel();
	// 
	void 		closeUnreadToastsPanel();

	// Channel's behavior-functions
	// set whether a channel will control hovering inside itself or not
	void		setControlHovering(bool control) { mControlHovering = control; }
	// set Hovering flag for a channel
	void		setHovering(bool hovering) { mIsHovering = hovering; }
	// set whether a channel will store faded toasts or not
	void		setStoreToasts(bool store) { mStoreToasts = store; }
	// tell all channels that the StartUp toast was shown and allow them showing of toasts
	static void	setStartUpToastShown() { mWasStartUpToastShown = true; }

	// Channel's other interface functions functions
	S32			getNumberOfHiddenToasts() { return mHiddenToastsNum;}
	// TODO: split StartUp and Overflow toasts
	void		setNumberOfHiddenToasts(S32 num) { mHiddenToastsNum = num;}
	e_notification_toast_alignment getToastAlignment() {return mToastAlignment;}

	// Channel's callbacks
	// callback for storing of faded toasts
	typedef boost::function<void (LLPanel* info_panel, const LLUUID id)> store_tost_callback_t;
	typedef boost::signals2::signal<void (LLPanel* info_panel, const LLUUID id)> store_tost_signal_t;
	store_tost_signal_t mOnStoreToast;	
	boost::signals2::connection setOnStoreToastCallback(store_tost_callback_t cb) { return mOnStoreToast.connect(cb); }

private:
	struct ToastElem
	{
		LLUUID		id;
		LLToast*	toast;
		ToastElem(LLUUID lluuid, LLPanel* panel) : id(lluuid)
		{
			toast = new LLToast(panel);
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
	void	onOverflowToastHide();

	//
	void	storeToast(ToastElem& toast_elem);
	
	// show-functions depending on allignment of toasts
	void	showToastsBottom();
	void	showToastsCentre();
	void	showToastsTop();
	
	// create the OverflowToast
	void	createOverflowToast(S32 bottom, F32 timer);

	// Channel's flags
	static bool	mWasStartUpToastShown;
	bool		mControlHovering;
	bool		mIsHovering;
	bool		mStoreToasts;
	bool		mOverflowToastHidden;
	// 
	e_notification_toast_alignment	mToastAlignment;

	S32			mHiddenToastsNum;
	LLToast*	mUnreadToastsPanel;
	std::string mOverflowFormatString;

	std::vector<ToastElem>		mToastList;
	std::vector<ToastElem>		mStoredToastList;
	std::map<LLToast*, bool>	mToastEventStack;
};

}
#endif
