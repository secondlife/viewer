/** 
 * @file llfloaterlagmeter.h
 * @brief The "Lag-o-Meter" floater used to tell users what is causing lag.
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LLFLOATERLAGMETER_H
#define LLFLOATERLAGMETER_H

#include "llfloater.h"

class LLFloaterLagMeter : public LLFloater
{
public:
	/*virtual*/ void draw();
	static void show(void*);

private:
	LLFloaterLagMeter();
	/*virtual*/ ~LLFloaterLagMeter();

	void determineClient();
	void determineNetwork();
	void determineServer();

	static void onClickShrink(void * data);

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

	static LLFloaterLagMeter * sInstance;
};

#endif
