/**
 * @file   llviewercontrollistener.h
 * @author Brad Kittenbrink
 * @date   2009-07-09
 * @brief  Event API for subset of LLViewerControl methods
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#ifndef LL_LLVIEWERCONTROLLISTENER_H
#define LL_LLVIEWERCONTROLLISTENER_H

#include "lleventdispatcher.h"

class LLControlGroup;
class LLSD;

class  LLViewerControlListener : public LLDispatchListener
{
public:
	LLViewerControlListener();

private:
	static void set(LLControlGroup *controls, LLSD const & event_data);
	static void toggleControl(LLControlGroup *controls, LLSD const & event_data);
	static void setDefault(LLControlGroup *controls, LLSD const & event_data);
};

extern LLViewerControlListener gSavedSettingsListener;

#endif // LL_LLVIEWERCONTROLLISTENER_H
