/** 
 * @file llfloaterlagmeter.h
 * @brief The "Lag-o-Meter" floater used to tell users what is causing lag.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
    BOOL isShrunk();

    void onClickShrink();

    bool mShrunk;
    S32 mMaxWidth, mMinWidth;

    F32Milliseconds mClientFrameTimeCritical;
    F32Milliseconds mClientFrameTimeWarning;
    LLButton*       mClientButton;
    LLTextBox*      mClientText;
    LLTextBox*      mClientCause;

    F32Percent      mNetworkPacketLossCritical;
    F32Percent      mNetworkPacketLossWarning;
    F32Milliseconds mNetworkPingCritical;
    F32Milliseconds mNetworkPingWarning;
    LLButton*       mNetworkButton;
    LLTextBox*      mNetworkText;
    LLTextBox*      mNetworkCause;

    F32Milliseconds mServerFrameTimeCritical;
    F32Milliseconds mServerFrameTimeWarning;
    F32Milliseconds mServerSingleProcessMaxTime;
    LLButton*       mServerButton;
    LLTextBox*      mServerText;
    LLTextBox*      mServerCause;

    LLStringUtil::format_map_t mStringArgs;
};

#endif
