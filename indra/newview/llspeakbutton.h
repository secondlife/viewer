/** 
* @file llspeakbutton.h
* @brief LLSpeakButton class header file
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

#ifndef LL_LLSPEAKBUTTON_H
#define LL_LLSPEAKBUTTON_H

#include "llinitparam.h"
#include "lluictrl.h"

class LLCallFloater;
class LLButton;
class LLOutputMonitorCtrl;
class LLBottomtrayButton;

/*
 * Button displaying voice chat status. Displays voice chat options when
 * clicked.
*/
class LLSpeakButton : public LLUICtrl
{
public:

	struct Params :	public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<LLButton::Params> speak_button;
		Optional<LLBottomtrayButton::Params> show_button;
		Optional<LLOutputMonitorCtrl::Params> monitor;

		Params();
	};

	/*virtual*/ ~LLSpeakButton();
	
	// *HACK: Need to put tooltips in a translatable location,
	// the panel that contains this button.
	void setSpeakToolTip(const std::string& msg);
	void setShowToolTip(const std::string& msg);

	/**
	 * Sets visibility of speak button's label according to passed parameter.
	 *
	 * It removes label/selected label if "visible" is false and restores otherwise.
	 *
	 * @param visible if true - show label and selected label.
	 * 
	 * @see mSpeakBtn
	 * @see LLBottomTray::processShrinkButtons()
	 */
	void setLabelVisible(bool visible);

protected:
	friend class LLUICtrlFactory;
	LLSpeakButton(const Params& p);

	void onMouseDown_SpeakBtn();
	void onMouseUp_SpeakBtn();

private:
	LLButton*	mSpeakBtn;
	LLBottomtrayButton*	mShowBtn;
	LLHandle<LLFloater> mPrivateCallPanel;
	LLOutputMonitorCtrl* mOutputMonitor;
};

#endif // LL_LLSPEAKBUTTON_H
