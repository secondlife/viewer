/**
 * @file lltoastnotifypanel.h
 * @brief Panel for notify toasts.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
 *
 * @deprecated this class will be removed after all toast panel types are
 *  implemented in separate classes.
 */
class LLToastNotifyPanel: public LLToastPanel 
{
public:
	/**
	 * Constructor for LLToastNotifyPanel.
	 * 
	 * @param pNotification a shared pointer to LLNotification
	 * @param rect an initial rectangle of the toast panel. 
	 * If it is null then a loaded from xml rectangle will be used. 
	 * @see LLNotification
	 * @deprecated if you intend to instantiate LLToastNotifyPanel - it's point to
	 * implement right class for desired toast panel. @see LLGenericTipPanel as example.
	 */
	LLToastNotifyPanel(const LLNotificationPtr& pNotification, const LLRect& rect = LLRect::null, bool show_images = true);
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

	typedef std::pair<int,LLButton*> index_button_pair_t; 
	void adjustPanelForScriptNotice(S32 max_width, S32 max_height);
	void adjustPanelForTipNotice();
	void addDefaultButton();
	/*
	 * It lays out buttons of the notification in  mControlPanel.
	 * Buttons will be placed from BOTTOM to TOP.
	 * @param  h_pad horizontal space between buttons. It is depend on number of buttons.
	 * @param buttons vector of button to be added. 
	 */
	void updateButtonsLayout(const std::vector<index_button_pair_t>& buttons, S32 h_pad);

	/**
	 * Disable specific button(s) based on notification name and clicked button
	 */
	void disableButtons(const std::string& notification_name, const std::string& selected_button);

	std::vector<index_button_pair_t> mButtons;

	// panel elements
	LLTextBase*		mTextBox;
	LLPanel*		mInfoPanel;		// a panel, that contains an information
	LLPanel*		mControlPanel;	// a panel, that contains buttons (if present)

	// internal handler for button being clicked
	static void onClickButton(void* data);

	typedef boost::signals2::signal <void (const LLUUID& notification_id, const std::string btn_name)>
		button_click_signal_t;
	static button_click_signal_t sButtonClickSignal;
	boost::signals2::connection mButtonClickConnection;

	/**
	 * handle sButtonClickSignal (to disable buttons) across all panels with given notification_id
	 */
	void onToastPanelButtonClicked(const LLUUID& notification_id, const std::string btn_name);

	/**
	 * Process response data. Will disable selected options
	 */
	void disableRespondedOptions(const LLNotificationPtr& notification);

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

class LLIMToastNotifyPanel : public LLToastNotifyPanel
{
public:

	LLIMToastNotifyPanel(LLNotificationPtr& pNotification, const LLUUID& session_id, const LLRect& rect = LLRect::null, bool show_images = true);

	~LLIMToastNotifyPanel();

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

protected:
	LLUUID	mSessionID;
};

#endif /* LLTOASTNOTIFYPANEL_H_ */
