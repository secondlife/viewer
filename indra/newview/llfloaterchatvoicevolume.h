/*
 * llfloaterchatvoicevolume.h
 *
 *  Created on: Sep 27, 2012
 *      Author: pguslisty
 */

#ifndef LLFLOATERCHATVOICEVOLUME_H_
#define LLFLOATERCHATVOICEVOLUME_H_

#include "llinspect.h"
#include "lltransientfloatermgr.h"

class LLFloaterChatVoiceVolume : public LLInspect, LLTransientFloater
{
public:

	LLFloaterChatVoiceVolume(const LLSD& key);
	virtual ~LLFloaterChatVoiceVolume();

	virtual void onOpen(const LLSD& key);

	/*virtual*/ LLTransientFloaterMgr::ETransientGroup getGroup() { return LLTransientFloaterMgr::GLOBAL; }
};

#endif /* LLFLOATERCHATVOICEVOLUME_H_ */
