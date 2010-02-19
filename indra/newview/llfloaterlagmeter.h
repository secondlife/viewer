/** 
 * @file llfloaterlagmeter.h
 * @brief The "Lag-o-Meter" floater used to tell users what is causing lag.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#ifndef LLFLOATERLAGMETER_H
#define LLFLOATERLAGMETER_H

#include "llfloater.h"

class LLTextBox;

class LLFloaterLagMeter : public LLFloater
{
	friend class LLFloaterReg;
	
public:
	/*virtual*/ void draw();
	/*virtual*/ BOOL postBuild();	
private:
	
	LLFloaterLagMeter(const LLSD& key);
	/*virtual*/ ~LLFloaterLagMeter();
	void determineClient();
	void determineNetwork();
	void determineServer();
	void updateControls(bool shrink);

	void onClickShrink();

	bool mShrunk;
	S32 mMaxWidth, mMinWidth;

	F32 mClientFrameTimeCritical;
	F32 mClientFrameTimeWarning;
	LLButton * mClientButton;
	LLTextBox * mClientText;
	LLTextBox * mClientCause;

	F32 mNetworkPacketLossCritical;
	F32 mNetworkPacketLossWarning;
	F32 mNetworkPingCritical;
	F32 mNetworkPingWarning;
	LLButton * mNetworkButton;
	LLTextBox * mNetworkText;
	LLTextBox * mNetworkCause;

	F32 mServerFrameTimeCritical;
	F32 mServerFrameTimeWarning;
	F32 mServerSingleProcessMaxTime;
	LLButton * mServerButton;
	LLTextBox * mServerText;
	LLTextBox * mServerCause;

	LLStringUtil::format_map_t mStringArgs;
};

#endif
