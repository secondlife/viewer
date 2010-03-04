/**
 * @file lltoastnotifypanel.h
 * @brief Panel for notify toasts.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LLTOASTNOTIFYPANEL_H_
#define LLTOASTNOTIFYPANEL_H_

#include "llpanel.h"
#include "llfontgl.h"
#include "llnotificationptr.h"
#include "llbutton.h"
#include "lltoastpanel.h"
#include "lliconctrl.h"
#include "lltexteditor.h"
#include "lltextbox.h"

class LLNotificationForm;

/**
 * Toast panel for notification.
 * Notification panel should be used for notifications that require a response from the user.
 *
 * Replaces class LLNotifyBox.
 */
class LLToastNotifyPanel: public LLToastPanel 
{
public:
	LLToastNotifyPanel(LLNotificationPtr&);
	virtual ~LLToastNotifyPanel();
	LLPanel * getControlPanel() { return mControlPanel; }

	void setCloseNotificationOnDestroy(bool close) { mCloseNotificationOnDestroy = close; }
protected:
	LLButton* createButton(const LLSD& form_element, BOOL is_option);

	// Used for callbacks
	struct InstanceAndS32
	{
		LLToastNotifyPanel* mSelf;
		std::string	mButtonName;
	};
	std::vector<InstanceAndS32*> mBtnCallbackData;

	bool mCloseNotificationOnDestroy;

private:

	typedef std::pair<int,LLButton*> index_button_pair_t; 
	void adjustPanelForScriptNotice(S32 max_width, S32 max_height);
	void adjustPanelForTipNotice();
	void addDefaultButton();
	/*
	 * It lays out buttons of the notification in  mControlPanel.
	 * Buttons will be placed from BOTTOM to TOP.
	 * @param  h_pad horizontal space between buttons. It is depent on number of buttons.
	 * @param buttons vector of button to be added. 
	 */
	void updateButtonsLayout(const std::vector<index_button_pair_t>& buttons, S32 h_pad);

	// panel elements
	LLTextBase*		mTextBox;
	LLPanel*		mInfoPanel;		// a panel, that contains an information
	LLPanel*		mControlPanel;	// a panel, that contains buttons (if present)

	// internal handler for button being clicked
	static void onClickButton(void* data);

	bool mIsTip;
	bool mAddedDefaultBtn;
	bool mIsScriptDialog;
	bool mIsCaution; 

	std::string mMessage;
	S32 mNumOptions;
	S32 mNumButtons;

	static const LLFontGL* sFont;
	static const LLFontGL* sFontSmall;
};

#endif /* LLTOASTNOTIFYPANEL_H_ */
