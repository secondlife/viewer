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

#include "llbutton.h"
#include "llfloaterreg.h"

#include "llagent.h"
#include "llbottomtray.h"
#include "llcallfloater.h"
#include "lloutputmonitorctrl.h"
#include "lltransientfloatermgr.h"

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
	bool voiceenabled = gVoiceClient->voiceEnabled();
	mSpeakBtn->setToggleState(openmic && voiceenabled);
	mOutputMonitor->setIsMuted(!voiceenabled);
	LLUICtrl::draw();
}

LLSpeakButton::LLSpeakButton(const Params& p)
: LLUICtrl(p)
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

// 	mShowBtn->setClickedCallback(boost::bind(&LLSpeakButton::onClick_ShowBtn, this));
// 	mShowBtn->setToggleState(FALSE);

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

	//*TODO find a better place to do that
	LLVoiceChannel::setCurrentVoiceChannelChangedCallback(boost::bind(&LLCallFloater::sOnCurrentChannelChanged, _1));
}

LLSpeakButton::~LLSpeakButton()
{
	LLTransientFloaterMgr::getInstance()->removeControlView(mSpeakBtn);
	LLTransientFloaterMgr::getInstance()->removeControlView(mShowBtn);
}

void LLSpeakButton::setSpeakToolTip(const std::string& msg)
{
	mSpeakBtn->setToolTip(msg);
}

void LLSpeakButton::setShowToolTip(const std::string& msg)
{
	mShowBtn->setToolTip(msg);
}

void LLSpeakButton::setLabelVisible(bool visible)
{
	static std::string label_selected = mSpeakBtn->getLabelSelected();
	static std::string label_unselected = mSpeakBtn->getLabelUnselected();

	if (visible)
	{
		mSpeakBtn->setLabelSelected(label_selected);
		mSpeakBtn->setLabelUnselected(label_unselected);
	}
	else
	{
		static LLStringExplicit empty_string("");
		mSpeakBtn->setLabelSelected(empty_string);
		mSpeakBtn->setLabelUnselected(empty_string);
	}
}

//////////////////////////////////////////////////////////////////////////
/// PROTECTED SECTION
//////////////////////////////////////////////////////////////////////////
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

