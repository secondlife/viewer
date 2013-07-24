/** 
 * @file llhudmanager.cpp
 * @brief LLHUDManager class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llhudmanager.h"

#include "message.h"
#include "object_flags.h"

#include "llagent.h"
#include "llhudeffect.h"
#include "pipeline.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"

// These are loaded from saved settings.
LLColor4 LLHUDManager::sParentColor;
LLColor4 LLHUDManager::sChildColor;

LLHUDManager::LLHUDManager()
{

	LLHUDManager::sParentColor = LLUIColorTable::instance().getColor("FocusColor");
	// rdw commented out since it's not used.  Also removed from colors_base.xml
	//LLHUDManager::sChildColor =LLUIColorTable::instance().getColor("FocusSecondaryColor");
}

LLHUDManager::~LLHUDManager()
{
}

static LLFastTimer::DeclareTimer FTM_HUD_EFFECTS("Hud Effects");

void LLHUDManager::updateEffects()
{
	LLFastTimer ftm(FTM_HUD_EFFECTS);
	S32 i;
	for (i = 0; i < mHUDEffects.count(); i++)
	{
		LLHUDEffect *hep = mHUDEffects[i];
		if (hep->isDead())
		{
			continue;
		}
		hep->update();
	}
}

void LLHUDManager::sendEffects()
{
	S32 i;
	for (i = 0; i < mHUDEffects.count(); i++)
	{
		LLHUDEffect *hep = mHUDEffects[i];
		if (hep->isDead())
		{
			llwarns << "Trying to send dead effect!" << llendl;
			continue;
		}
		if (hep->mType < LLHUDObject::LL_HUD_EFFECT_BEAM)
		{
			llwarns << "Trying to send effect of type " << hep->mType << " which isn't really an effect and shouldn't be in this list!" << llendl;
			continue;
		}
		if (hep->getNeedsSendToSim() && hep->getOriginatedHere())
		{
			LLMessageSystem* msg = gMessageSystem;
			msg->newMessageFast(_PREHASH_ViewerEffect);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_Effect);
			hep->packData(msg);
			hep->setNeedsSendToSim(FALSE);
			gAgent.sendMessage();
		}
	}
}

//static
void LLHUDManager::shutdownClass()
{
	getInstance()->mHUDEffects.reset();
}

void LLHUDManager::cleanupEffects()
{
	S32 i = 0;

	while (i < mHUDEffects.count())
	{
		if (mHUDEffects[i]->isDead())
		{
			mHUDEffects.remove(i);
		}
		else
		{
			i++;
		}
	}
}

LLHUDEffect *LLHUDManager::createViewerEffect(const U8 type, BOOL send_to_sim, BOOL originated_here)
{
	// SJB: DO NOT USE addHUDObject!!! Not all LLHUDObjects are LLHUDEffects!
	LLHUDEffect *hep = LLHUDObject::addHUDEffect(type);
	if (!hep)
	{
		return NULL;
	}

	LLUUID tmp;
	tmp.generate();
	hep->setID(tmp);
	hep->setNeedsSendToSim(send_to_sim);
	hep->setOriginatedHere(originated_here);

	mHUDEffects.put(hep);
	return hep;
}


//static
void LLHUDManager::processViewerEffect(LLMessageSystem *mesgsys, void **user_data)
{
	LLHUDEffect *effectp = NULL;
	LLUUID effect_id;
	U8 effect_type = 0;
	S32 number_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_Effect);
	S32 k;

	for (k = 0; k < number_blocks; k++)
	{
		effectp = NULL;
		LLHUDEffect::getIDType(mesgsys, k, effect_id, effect_type);
		S32 i;
		for (i = 0; i < LLHUDManager::getInstance()->mHUDEffects.count(); i++)
		{
			LLHUDEffect *cur_effectp = LLHUDManager::getInstance()->mHUDEffects[i];
			if (!cur_effectp)
			{
				llwarns << "Null effect in effect manager, skipping" << llendl;
				LLHUDManager::getInstance()->mHUDEffects.remove(i);
				i--;
				continue;
			}
			if (cur_effectp->isDead())
			{
	//			llwarns << "Dead effect in effect manager, removing" << llendl;
				LLHUDManager::getInstance()->mHUDEffects.remove(i);
				i--;
				continue;
			}
			if (cur_effectp->getID() == effect_id)
			{
				if (cur_effectp->getType() != effect_type)
				{
					llwarns << "Viewer effect update doesn't match old type!" << llendl;
				}
				effectp = cur_effectp;
				break;
			}
		}

		if (effect_type)
		{
			if (!effectp)
			{
				effectp = LLHUDManager::getInstance()->createViewerEffect(effect_type, FALSE, FALSE);
			}

			if (effectp)
			{
				effectp->unpackData(mesgsys, k);
			}
		}
		else
		{
			llwarns << "Received viewer effect of type " << effect_type << " which isn't really an effect!" << llendl;
		}
	}

	//llinfos << "Got viewer effect: " << effect_id << llendl;
}

