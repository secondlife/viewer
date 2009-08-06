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

	void		reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	LLToast*	addToast(LLUUID id, LLPanel* panel, bool is_not_tip = true);
	void		init(S32 channel_left, S32 channel_right);
	
	void		killToastByNotificationID(LLUUID id);
	void		modifyToastByNotificationID(LLUUID id, LLPanel* panel);
	
	void		setToastAlignment(e_notification_toast_alignment align) {mToastAlignment = align;}

	void		setControlHovering(bool control) { mControlHovering = control; }
	void		setHovering(bool hovering) { mIsHovering = hovering; }

	void		removeToastsFromChannel();
	void 		closeUnreadToastsPanel();
	void		hideToastsFromScreen();

	void		setStoreToasts(bool store) { mStoreToasts = store; }
	void		loadStoredToastsToChannel();
	
	void		showToasts();

	S32			getNumberOfHiddenToasts() { return mHiddenToastsNum;}
	void		setNumberOfHiddenToasts(S32 num) { mHiddenToastsNum = num;}

	static void	setStartUpToastShown() { mWasStartUpToastShown = true; }

	e_notification_toast_alignment getToastAlignment() {return mToastAlignment;}

	void		setOverflowFormatString ( std::string str)  { mOverflowFormatString = str; }

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

	void	onToastHover(LLToast* toast, bool mouse_enter);

	void	onToastFade(LLToast* toast);
	void	storeToast(ToastElem& toast_elem);
	
	void	showToastsBottom();
	void	showToastsCentre();
	void	showToastsTop();
	
	void	createOverflowToast(S32 bottom, F32 timer);
	void	onOverflowToastHide();

	static bool	mWasStartUpToastShown;
	bool		mControlHovering;
	bool		mIsHovering;
	bool		mStoreToasts;
	bool		mOverflowToastHidden;
	S32			mHiddenToastsNum;
	LLToast*	mUnreadToastsPanel;
	std::vector<ToastElem>	mToastList;
	std::vector<ToastElem>	mStoredToastList;
	e_notification_toast_alignment	mToastAlignment;
	std::map<LLToast*, bool>	mToastEventStack;

	std::string mOverflowFormatString;
};

}
#endif
