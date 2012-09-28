/*
 * llfloaterchatvoicevolume.cpp
 *
 *  Created on: Sep 27, 2012
 *      Author: pguslisty
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterchatvoicevolume.h"

LLFloaterChatVoiceVolume::LLFloaterChatVoiceVolume(const LLSD& key)
: LLInspect(key)
{
}

void LLFloaterChatVoiceVolume::onOpen(const LLSD& key)
{
	LLInspect::onOpen(key);
	LLUI::positionViewNearMouse(this);
}

LLFloaterChatVoiceVolume::~LLFloaterChatVoiceVolume()
{
	LLTransientFloaterMgr::getInstance()->removeControlView(this);
};
