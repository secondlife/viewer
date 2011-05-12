/** 
* @file llspeakbutton.cpp
* @brief LLSpeakButton class implementation
*
* $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h" // must be first include

#include "llbutton.h"
#include "llfloaterreg.h"

#include "llagent.h"
#include "llbottomtray.h"
#include "llcallfloater.h"
#include "lloutputmonitorctrl.h"
#include "lltransientfloatermgr.h"

#include "llspeakbutton.h"

#include "llbottomtray.h"
#include "llfirstuse.h"

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

	LLBottomtrayButton::Params show_params = p.show_button;
	show_params.rect(show_rect);
	mShowBtn = LLUICtrlFactory::create<LLBottomtrayButton>(show_params);
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
	LLVoiceChannel::setCurrentVoiceChannelChangedCallback(boost::bind(&LLCallFloater::sOnCurrentChannelChanged, _1), true);
}

LLSpeakButton::~LLSpeakButton()
{
	if(LLTransientFloaterMgr::instanceExists())
	{
		LLTransientFloaterMgr::getInstance()->removeControlView(mSpeakBtn);
		LLTransientFloaterMgr::getInstance()->removeControlView(mShowBtn);
	}
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
	LLVoiceClient::getInstance()->inputUserControlState(down); // this method knows/care about whether this translates into a toggle-to-talk or down-to-talk
	LLFirstUse::speak(false);
}
void LLSpeakButton::onMouseUp_SpeakBtn()
{
	bool down = false;
	LLVoiceClient::getInstance()->inputUserControlState(down);
}

