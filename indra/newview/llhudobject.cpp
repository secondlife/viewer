/** 
 * @file llhudobject.cpp
 * @brief LLHUDObject class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// llhudobject.cpp
// Copyright 2002, Linden Research, Inc.

#include "llviewerprecompiledheaders.h"

#include "llhudobject.h"
#include "llviewerobject.h"

#include "llhudtext.h"
#include "llhudicon.h"
#include "llhudconnector.h"
#include "llhudeffectbeam.h"
#include "llhudeffecttrail.h"
#include "llhudeffectlookat.h"

//Ventrella
#include "llanimalcontrols.h"
#include "lllocalanimationobject.h"
#include "llcape.h"
// End Ventrella

#include "llagent.h"

// statics
std::list<LLPointer<LLHUDObject> > LLHUDObject::sHUDObjects;

struct hud_object_further_away
{
	bool operator()(const LLPointer<LLHUDObject>& lhs, const LLPointer<LLHUDObject>& rhs) const;
};


bool hud_object_further_away::operator()(const LLPointer<LLHUDObject>& lhs, const LLPointer<LLHUDObject>& rhs) const
{
	return (lhs->getDistance() > rhs->getDistance()) ? true : false;
}


LLHUDObject::LLHUDObject(const U8 type) : 
	mPositionGlobal(), 
	mSourceObject(NULL), 
	mTargetObject(NULL)
{
	mVisible = TRUE;
	mType = type;
	mDead = FALSE;
	mOnHUDAttachment = FALSE;
}

LLHUDObject::~LLHUDObject()
{
}

void LLHUDObject::markDead()
{
	mVisible = FALSE;
	mDead = TRUE;
	mSourceObject = NULL;
	mTargetObject = NULL;
}

F32 LLHUDObject::getDistance() const
{
	return 0.f;
}

void LLHUDObject::setSourceObject(LLViewerObject* objectp)
{
	if (objectp == mSourceObject)
	{
		return;
	}

	mSourceObject = objectp;
}

void LLHUDObject::setTargetObject(LLViewerObject* objectp)
{
	if (objectp == mTargetObject)
	{
		return;
	}

	mTargetObject = objectp;
}

void LLHUDObject::setPositionGlobal(const LLVector3d &position_global)
{
	mPositionGlobal = position_global;
}

void LLHUDObject::setPositionAgent(const LLVector3 &position_agent)
{
	mPositionGlobal = gAgent.getPosGlobalFromAgent(position_agent);
}

// static
void LLHUDObject::cleanupHUDObjects()
{
	LLHUDIcon::cleanupDeadIcons();
	hud_object_list_t::iterator object_it;
	for (object_it = sHUDObjects.begin(); object_it != sHUDObjects.end(); ++object_it)
	{
		(*object_it)->markDead();
		if ((*object_it)->getNumRefs() > 1)
		{
			llinfos << "LLHUDObject " << (LLHUDObject *)(*object_it) << " has " << (*object_it)->getNumRefs() << " refs!" << llendl;
		}
	}
	sHUDObjects.clear();
}

// static
LLHUDObject *LLHUDObject::addHUDObject(const U8 type)
{
	LLHUDObject *hud_objectp = NULL;
	
	switch (type)
	{
	case LL_HUD_TEXT:
		hud_objectp = new LLHUDText(type);
		break;
	case LL_HUD_ICON:
		hud_objectp = new LLHUDIcon(type);
		break;
	case LL_HUD_CONNECTOR:
		hud_objectp = new LLHUDConnector(type);
		break;
	case LL_HUD_EFFECT_BEAM:
		hud_objectp = new LLHUDEffectSpiral(type);
		((LLHUDEffectSpiral *)hud_objectp)->setDuration(0.7f);
		((LLHUDEffectSpiral *)hud_objectp)->setVMag(0.f);
		((LLHUDEffectSpiral *)hud_objectp)->setVOffset(0.f);
		((LLHUDEffectSpiral *)hud_objectp)->setInitialRadius(0.1f);
		((LLHUDEffectSpiral *)hud_objectp)->setFinalRadius(0.2f);
		((LLHUDEffectSpiral *)hud_objectp)->setSpinRate(10.f);
		((LLHUDEffectSpiral *)hud_objectp)->setFlickerRate(0.f);
		((LLHUDEffectSpiral *)hud_objectp)->setScaleBase(0.05f);
		((LLHUDEffectSpiral *)hud_objectp)->setScaleVar(0.02f);
		((LLHUDEffectSpiral *)hud_objectp)->setColor(LLColor4U(255, 255, 255, 255));
		break;
	case LL_HUD_EFFECT_GLOW:
		llerrs << "Glow not implemented!" << llendl;
		break;
	case LL_HUD_EFFECT_POINT:
		hud_objectp = new LLHUDEffectSpiral(type);
		((LLHUDEffectSpiral *)hud_objectp)->setDuration(0.5f);
		((LLHUDEffectSpiral *)hud_objectp)->setVMag(1.f);
		((LLHUDEffectSpiral *)hud_objectp)->setVOffset(0.f);
		((LLHUDEffectSpiral *)hud_objectp)->setInitialRadius(0.5f);
		((LLHUDEffectSpiral *)hud_objectp)->setFinalRadius(1.f);
		((LLHUDEffectSpiral *)hud_objectp)->setSpinRate(10.f);
		((LLHUDEffectSpiral *)hud_objectp)->setFlickerRate(0.f);
		((LLHUDEffectSpiral *)hud_objectp)->setScaleBase(0.1f);
		((LLHUDEffectSpiral *)hud_objectp)->setScaleVar(0.1f);
		((LLHUDEffectSpiral *)hud_objectp)->setColor(LLColor4U(255, 255, 255, 255));
		break;
	case LL_HUD_EFFECT_SPHERE:
		hud_objectp = new LLHUDEffectSpiral(type);
		((LLHUDEffectSpiral *)hud_objectp)->setDuration(0.5f);
		((LLHUDEffectSpiral *)hud_objectp)->setVMag(1.f);
		((LLHUDEffectSpiral *)hud_objectp)->setVOffset(0.f);
		((LLHUDEffectSpiral *)hud_objectp)->setInitialRadius(0.5f);
		((LLHUDEffectSpiral *)hud_objectp)->setFinalRadius(0.5f);
		((LLHUDEffectSpiral *)hud_objectp)->setSpinRate(20.f);
		((LLHUDEffectSpiral *)hud_objectp)->setFlickerRate(0.f);
		((LLHUDEffectSpiral *)hud_objectp)->setScaleBase(0.1f);
		((LLHUDEffectSpiral *)hud_objectp)->setScaleVar(0.1f);
		((LLHUDEffectSpiral *)hud_objectp)->setColor(LLColor4U(255, 255, 255, 255));
		break;
	case LL_HUD_EFFECT_SPIRAL:
		hud_objectp = new LLHUDEffectSpiral(type);
		((LLHUDEffectSpiral *)hud_objectp)->setDuration(2.f);
		((LLHUDEffectSpiral *)hud_objectp)->setVMag(-2.f);
		((LLHUDEffectSpiral *)hud_objectp)->setVOffset(0.5f);
		((LLHUDEffectSpiral *)hud_objectp)->setInitialRadius(1.f);
		((LLHUDEffectSpiral *)hud_objectp)->setFinalRadius(0.5f);
		((LLHUDEffectSpiral *)hud_objectp)->setSpinRate(10.f);
		((LLHUDEffectSpiral *)hud_objectp)->setFlickerRate(20.f);
		((LLHUDEffectSpiral *)hud_objectp)->setScaleBase(0.02f);
		((LLHUDEffectSpiral *)hud_objectp)->setScaleVar(0.02f);
		((LLHUDEffectSpiral *)hud_objectp)->setColor(LLColor4U(255, 255, 255, 255));
		break;
	case LL_HUD_EFFECT_EDIT:
		hud_objectp = new LLHUDEffectSpiral(type);
		((LLHUDEffectSpiral *)hud_objectp)->setDuration(2.f);
		((LLHUDEffectSpiral *)hud_objectp)->setVMag(2.f);
		((LLHUDEffectSpiral *)hud_objectp)->setVOffset(-1.f);
		((LLHUDEffectSpiral *)hud_objectp)->setInitialRadius(1.5f);
		((LLHUDEffectSpiral *)hud_objectp)->setFinalRadius(1.f);
		((LLHUDEffectSpiral *)hud_objectp)->setSpinRate(4.f);
		((LLHUDEffectSpiral *)hud_objectp)->setFlickerRate(200.f);
		((LLHUDEffectSpiral *)hud_objectp)->setScaleBase(0.1f);
		((LLHUDEffectSpiral *)hud_objectp)->setScaleVar(0.1f);
		((LLHUDEffectSpiral *)hud_objectp)->setColor(LLColor4U(255, 255, 255, 255));
		break;
	case LL_HUD_EFFECT_LOOKAT:
		hud_objectp = new LLHUDEffectLookAt(type);
		break;
	case LL_HUD_EFFECT_POINTAT:
		hud_objectp = new LLHUDEffectPointAt(type);
		break;
	default:
		llwarns << "Unknown type of hud object:" << (U32) type << llendl;
	}

	if (hud_objectp)
	{
		sHUDObjects.push_back(hud_objectp);
	}
	return hud_objectp;
}

// static
void LLHUDObject::updateAll()
{
	LLFastTimer ftm(LLFastTimer::FTM_HUD_UPDATE);
	LLHUDText::updateAll();
	LLHUDIcon::updateAll();
	sortObjects();
}

// static
void LLHUDObject::renderAll()
{
	LLHUDObject *hud_objp;
	
	hud_object_list_t::iterator object_it;
	for (object_it = sHUDObjects.begin(); object_it != sHUDObjects.end(); )
	{
		hud_object_list_t::iterator cur_it = object_it++;
		hud_objp = (*cur_it);
		if (hud_objp->getNumRefs() == 1)
		{
			sHUDObjects.erase(cur_it);
		}
		else if (hud_objp->isVisible())
		{
			hud_objp->render();
		}
	}
}

// static
void LLHUDObject::renderAllForSelect()
{
	LLHUDObject *hud_objp;
	
	hud_object_list_t::iterator object_it;
	for (object_it = sHUDObjects.begin(); object_it != sHUDObjects.end(); )
	{
		hud_object_list_t::iterator cur_it = object_it++;
		hud_objp = (*cur_it);
		if (hud_objp->getNumRefs() == 1)
		{
			sHUDObjects.erase(cur_it);
		}
		else if (hud_objp->isVisible())
		{
			hud_objp->renderForSelect();
		}
	}
}

// static
void LLHUDObject::sortObjects()
{
	sHUDObjects.sort(hud_object_further_away());	
}
