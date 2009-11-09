/** 
* @file llspeakbutton.cpp
* @brief LLSpeakButton class implementation
*
* $LicenseInfo:firstyear=2002&license=viewergpl$
* 
* Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h" // must be first include

#include "llagent.h"
#include "llbottomtray.h"
#include "llfloaterreg.h"
#include "llvoiceclient.h"
#include "llvoicecontrolpanel.h"
#include "lltransientfloatermgr.h"

#include "llavatariconctrl.h"
#include "llbutton.h"
#include "llpanel.h"
#include "lltextbox.h"
#include "lloutputmonitorctrl.h"
#include "llgroupmgr.h"

#include "llspeakbutton.h"

static LLDefaultChildRegistry::Register<LLSpeakButton> t1("talk_button");

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLSpeakButton::Params::Params()
 : speak_button("speak_button")
 , show_button("show_button")
 , monitor("monitor")
{
	// See widgets/talk_button.xml
}

void LLSpeakButton::draw()
{
	// gVoiceClient is the authoritative global source of info regarding our open-mic state, we merely reflect that state.
	bool openmic = gVoiceClient->getUserPTTState();
	mSpeakBtn->setToggleState(openmic);
	llinfos << "mic state " << int(openmic) << llendl;
	LLUICtrl::draw();
}

LLSpeakButton::LLSpeakButton(const Params& p)
: LLUICtrl(p)
, mPrivateCallPanel(NULL)
, mOutputMonitor(NULL)
, mSpeakBtn(NULL)
, mShowBtn(NULL)
{
	LLRect rect = p.rect();
	LLRect speak_rect(0, rect.getHeight(), rect.getWidth(), 0);
	LLRect show_rect = p.show_button.rect();
	show_rect.set(0, rect.getHeight(), show_rect.getWidth(), 0);

	speak_rect.mRight -= show_rect.getWidth();
	show_rect.mLeft = speak_rect.getWidth();
	show_rect.mRight = rect.getWidth();

	LLButton::Params speak_params = p.speak_button;
	speak_params.rect(speak_rect);
	mSpeakBtn = LLUICtrlFactory::create<LLButton>(speak_params);
	addChild(mSpeakBtn);
	LLTransientFloaterMgr::getInstance()->addControlView(mSpeakBtn);

	mSpeakBtn->setMouseDownCallback(boost::bind(&LLSpeakButton::onMouseDown_SpeakBtn, this));
	mSpeakBtn->setMouseUpCallback(boost::bind(&LLSpeakButton::onMouseUp_SpeakBtn, this));
	mSpeakBtn->setToggleState(FALSE);

	LLButton::Params show_params = p.show_button;
	show_params.rect(show_rect);
	mShowBtn = LLUICtrlFactory::create<LLButton>(show_params);
	addChild(mShowBtn);
	LLTransientFloaterMgr::getInstance()->addControlView(mShowBtn);

	mShowBtn->setClickedCallback(boost::bind(&LLSpeakButton::onClick_ShowBtn, this));
	mShowBtn->setToggleState(FALSE);

	static const S32 MONITOR_RIGHT_PAD = 2;

	LLRect monitor_rect = p.monitor.rect();
	S32 monitor_height = monitor_rect.getHeight();
	monitor_rect.mLeft = speak_rect.getWidth() - monitor_rect.getWidth() - MONITOR_RIGHT_PAD;
	monitor_rect.mRight = speak_rect.getWidth() - MONITOR_RIGHT_PAD;
	monitor_rect.mBottom = (rect.getHeight() / 2) - (monitor_height / 2);
	monitor_rect.mTop = monitor_rect.mBottom + monitor_height;

	LLOutputMonitorCtrl::Params monitor_params = p.monitor;
	monitor_params.draw_border(false);
	monitor_params.rect(monitor_rect);
	monitor_params.auto_update(true);
	monitor_params.speaker_id(gAgentID);
	mOutputMonitor = LLUICtrlFactory::create<LLOutputMonitorCtrl>(monitor_params);
	mSpeakBtn->addChild(mOutputMonitor);

	// never show "muted" because you can't mute yourself
	mOutputMonitor->setIsMuted(false);
	mOutputMonitor->setIsAgentControl(true);
}

LLSpeakButton::~LLSpeakButton()
{
}

void LLSpeakButton::onMouseDown_SpeakBtn()
{
	bool down = true;
	gVoiceClient->inputUserControlState(down); // this method knows/care about whether this translates into a toggle-to-talk or down-to-talk
}
void LLSpeakButton::onMouseUp_SpeakBtn()
{
	bool down = false;
	gVoiceClient->inputUserControlState(down);
}

void LLSpeakButton::onClick_ShowBtn()
{
	if(!mShowBtn->getToggleState())
	{
		mPrivateCallPanel->onClickClose(mPrivateCallPanel);
		delete mPrivateCallPanel;
		mPrivateCallPanel = NULL;
		mShowBtn->setToggleState(FALSE);
		return;
	}

	S32 x = mSpeakBtn->getRect().mLeft;
	S32 y = 0;

	localPointToScreen(x, y, &x, &y);

	mPrivateCallPanel = new LLVoiceControlPanel;
	getRootView()->addChild(mPrivateCallPanel);

	y = LLBottomTray::getInstance()->getRect().getHeight() + mPrivateCallPanel->getRect().getHeight();

	LLRect rect;
	rect.setLeftTopAndSize(x, y, mPrivateCallPanel->getRect().getWidth(), mPrivateCallPanel->getRect().getHeight());
	mPrivateCallPanel->setRect(rect);


	LLAvatarListItem* item = new LLAvatarListItem();
	item->showLastInteractionTime(false);
	item->showInfoBtn(true);
	item->showSpeakingIndicator(true);
	item->reshape(mPrivateCallPanel->getRect().getWidth(), item->getRect().getHeight(), FALSE);

	mPrivateCallPanel->addItem(item);
	mPrivateCallPanel->setVisible(TRUE);
	mPrivateCallPanel->setFrontmost(TRUE);

	mShowBtn->setToggleState(TRUE);
}

