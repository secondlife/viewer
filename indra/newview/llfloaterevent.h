/** 
 * @file llfloaterevent.h
 * @brief LLFloaterEvent class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/**
* Event information as shown in a floating window from a secondlife:// url.
* Just a wrapper for LLPanelEvent.
*/

#ifndef LL_LLFLOATEREVENT_H
#define LL_LLFLOATEREVENT_H

#include "llfloater.h"

class LLPanelEvent;

class LLFloaterEventInfo : public LLFloater
{
public:
	LLFloaterEventInfo(const std::string& name, const U32 event_id );
	/*virtual*/ ~LLFloaterEventInfo();

	void displayEventInfo(const U32 event_id);

	static LLFloaterEventInfo* show(const U32 event_id);
	
	static void* createEventDetail(void* userdata);

private:
	U32				mEventID;			// for which event is this window?
	LLPanelEvent*	mPanelEventp;

};

#endif // LL_LLFLOATEREVENT_H
