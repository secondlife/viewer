/** 
 * @file llhudmanager.cpp
 * @brief LLHUDManager class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llhudmanager.h"

#include "message.h"
#include "object_flags.h"

#include "llagent.h"
#include "llhudconnector.h"
#include "llhudeffect.h"
#include "pipeline.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"

LLHUDManager *gHUDManager = NULL;

extern BOOL gNoRender;

// These are loaded from saved settings.
LLColor4 LLHUDManager::sParentColor;
LLColor4 LLHUDManager::sChildColor;

LLHUDManager::LLHUDManager()
{
	mShowPhysical = FALSE;

	LLHUDManager::sParentColor = gColors.getColor("FocusColor");
	// rdw commented out since it's not used.  Also removed from colors_base.xml
	//LLHUDManager::sChildColor = gColors.getColor("FocusSecondaryColor");
}

LLHUDManager::~LLHUDManager()
{
	mHUDJoints.reset();
	mHUDSelectedJoints.reset();
	mHUDEffects.reset();
}


void LLHUDManager::toggleShowPhysical(const BOOL show_physical)
{
	if (show_physical == mShowPhysical)
	{
		return;
	}

	mShowPhysical = show_physical;
	if (show_physical)
	{
		S32 i;
		for (i = 0; i < gObjectList.getNumObjects(); i++)
		{
			LLViewerObject *vobjp = gObjectList.getObject(i);

			if (vobjp && vobjp->isJointChild() && vobjp->getParent())
			{
				LLHUDConnector *connectorp = (LLHUDConnector *)LLHUDObject::addHUDObject(LLHUDObject::LL_HUD_CONNECTOR);
				connectorp->setTargets(vobjp, (LLViewerObject *)vobjp->getParent());
				connectorp->setColors(LLColor4(1.f, 1.f, 1.f, 1.f), sChildColor, sParentColor);
				EHavokJointType joint_type = vobjp->getJointType();
				if (HJT_HINGE == joint_type)
				{
					connectorp->setLabel("Hinge");
				}
				else if (HJT_POINT == joint_type)
				{
					connectorp->setLabel("P2P");
				}
#if 0
				else if (HJT_LPOINT == joint_type)
				{
					connectorp->setLabel("LP2P");
				}
#endif
#if 0
				else if (HJT_WHEEL == joint_type)
				{
					connectorp->setLabel("Wheel");
				}
#endif
				mHUDJoints.put(connectorp);
			}
		}
	}
	else
	{
		mHUDJoints.reset();
	}
}

void LLHUDManager::showJoints(LLDynamicArray < LLViewerObject* > *object_list)
{
	for (S32 i=0; i<object_list->count(); i++)
	{
		LLViewerObject *vobjp = object_list->get(i);
		if (vobjp && vobjp->isJointChild() && vobjp->getParent())
		{
			LLHUDConnector *connectorp = (LLHUDConnector *)LLHUDObject::addHUDObject(LLHUDObject::LL_HUD_CONNECTOR);
			connectorp->setTargets(vobjp, (LLViewerObject *)vobjp->getParent());
			connectorp->setColors(LLColor4(1.f, 1.f, 1.f, 1.f), sChildColor, sParentColor);

			EHavokJointType joint_type = vobjp->getJointType();
			if (HJT_HINGE == joint_type)
			{
				connectorp->setLabel("Hinge");
			}
			else if (HJT_POINT == joint_type)
			{
				connectorp->setLabel("P2P");
			}
#if 0
			else if (HJT_LPOINT == joint_type)
			{
				connectorp->setLabel("LP2P");
			}
#endif
#if 0
			else if (HJT_WHEEL == joint_type)
			{
				connectorp->setLabel("Wheel");
			}
#endif
			mHUDSelectedJoints.put(connectorp);
		}
	}
}

void LLHUDManager::clearJoints()
{
	mHUDSelectedJoints.reset();
}

BOOL LLHUDManager::getShowPhysical() const
{
	return mShowPhysical;
}

void LLHUDManager::updateEffects()
{
	LLFastTimer ftm(LLFastTimer::FTM_HUD_EFFECTS);
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
		if (hep->mType <= LLHUDObject::LL_HUD_CONNECTOR)
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
	// Should assert that this is actually an LLHUDEffect
	LLHUDEffect *hep = (LLHUDEffect *)LLHUDObject::addHUDObject(type);
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
	if (gNoRender)
	{
		return;
	}

	if (!gHUDManager)
	{
		llwarns << "No gHUDManager!" << llendl;
		return;
	}

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
		for (i = 0; i < gHUDManager->mHUDEffects.count(); i++)
		{
			LLHUDEffect *cur_effectp = gHUDManager->mHUDEffects[i];
			if (!cur_effectp)
			{
				llwarns << "Null effect in effect manager, skipping" << llendl;
				gHUDManager->mHUDEffects.remove(i);
				i--;
				continue;
			}
			if (cur_effectp->isDead())
			{
	//			llwarns << "Dead effect in effect manager, removing" << llendl;
				gHUDManager->mHUDEffects.remove(i);
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
				effectp = gHUDManager->createViewerEffect(effect_type, FALSE, FALSE);
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

