/** 
* @file llspeakbutton.h
* @brief LLSpeakButton class header file
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

#ifndef LL_LLSPEAKBUTTON_H
#define LL_LLSPEAKBUTTON_H

#include "llinitparam.h"
#include "lluictrl.h"

class LLCallFloater;
class LLButton;
class LLOutputMonitorCtrl;

/*
 * Button displaying voice chat status. Displays voice chat options when
 * clicked.
*/
class LLSpeakButton : public LLUICtrl
{
public:

	struct Params :	public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<LLButton::Params>
			speak_button,
			show_button;

		Optional<LLOutputMonitorCtrl::Params> monitor;

		Params();
	};

	/*virtual*/ ~LLSpeakButton();
	/*virtual*/ void draw();
	
	// methods for enabling/disabling right and left parts of speak button separately(EXT-4648)
	void setSpeakBtnEnabled(bool enabled);
	void setFlyoutBtnEnabled(bool enabled);

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
	LLButton*	mShowBtn;
	LLHandle<LLFloater> mPrivateCallPanel;
	LLOutputMonitorCtrl* mOutputMonitor;
};

#endif // LL_LLSPEAKBUTTON_H
