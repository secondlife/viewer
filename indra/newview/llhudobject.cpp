/** 
 * @file llhudobject.cpp
 * @brief LLHUDObject class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2002-2010, Linden Research, Inc.
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

#include "llhudobject.h"
#include "llviewerobject.h"

#include "llhudtext.h"
#include "llhudicon.h"
#include "llhudeffectbeam.h"
#include "llhudeffectblob.h"
#include "llhudeffecttrail.h"
#include "llhudeffectlookat.h"
#include "llhudeffectpointat.h"
#include "llhudnametag.h"
#include "llvoicevisualizer.h"

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
			LL_INFOS() << "LLHUDObject " << (LLHUDObject *)(*object_it) << " type " << (S32)(*object_it)->getType() << " has " << (*object_it)->getNumRefs() << " refs!" << LL_ENDL;			
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
	case LL_HUD_NAME_TAG:
		hud_objectp = new LLHUDNameTag(type);
		break;
	default:
		LL_WARNS() << "Unknown type of hud object:" << (U32) type << LL_ENDL;
	}
	if (hud_objectp)
	{
		sHUDObjects.push_back(hud_objectp);
	}
	return hud_objectp;
}

LLHUDEffect *LLHUDObject::addHUDEffect(const U8 type)
{
	LLHUDEffect *hud_objectp = NULL;
	LLHUDEffectSpiral *spiralp = NULL;

	switch (type)
	{
	case LL_HUD_EFFECT_BEAM:
		spiralp = new LLHUDEffectSpiral(type);
		spiralp->setDuration(0.7f);
		spiralp->setVMag(0.f);
		spiralp->setVOffset(0.f);
		spiralp->setInitialRadius(0.1f);
		spiralp->setFinalRadius(0.2f);
		spiralp->setSpinRate(10.f);
		spiralp->setFlickerRate(0.f);
		spiralp->setScaleBase(0.05f);
		spiralp->setScaleVar(0.02f);
		spiralp->setColor(LLColor4U(255, 255, 255, 255));
		break;
	case LL_HUD_EFFECT_GLOW:
		// deprecated
		break;
	case LL_HUD_EFFECT_POINT:
		spiralp = new LLHUDEffectSpiral(type);
		spiralp->setDuration(0.5f);
		spiralp->setVMag(1.f);
		spiralp->setVOffset(0.f);
		spiralp->setInitialRadius(0.5f);
		spiralp->setFinalRadius(1.f);
		spiralp->setSpinRate(10.f);
		spiralp->setFlickerRate(0.f);
		spiralp->setScaleBase(0.1f);
		spiralp->setScaleVar(0.1f);
		spiralp->setColor(LLColor4U(255, 255, 255, 255));
		break;
	case LL_HUD_EFFECT_SPHERE:
		spiralp = new LLHUDEffectSpiral(type);
		spiralp->setDuration(0.5f);
		spiralp->setVMag(1.f);
		spiralp->setVOffset(0.f);
		spiralp->setInitialRadius(0.5f);
		spiralp->setFinalRadius(0.5f);
		spiralp->setSpinRate(20.f);
		spiralp->setFlickerRate(0.f);
		spiralp->setScaleBase(0.1f);
		spiralp->setScaleVar(0.1f);
		spiralp->setColor(LLColor4U(255, 255, 255, 255));
		break;
	case LL_HUD_EFFECT_SPIRAL:
		spiralp = new LLHUDEffectSpiral(type);
		spiralp->setDuration(2.f);
		spiralp->setVMag(-2.f);
		spiralp->setVOffset(0.5f);
		spiralp->setInitialRadius(1.f);
		spiralp->setFinalRadius(0.5f);
		spiralp->setSpinRate(10.f);
		spiralp->setFlickerRate(20.f);
		spiralp->setScaleBase(0.02f);
		spiralp->setScaleVar(0.02f);
		spiralp->setColor(LLColor4U(255, 255, 255, 255));
		break;
	case LL_HUD_EFFECT_EDIT:
		spiralp = new LLHUDEffectSpiral(type);
		spiralp->setDuration(2.f);
		spiralp->setVMag(2.f);
		spiralp->setVOffset(-1.f);
		spiralp->setInitialRadius(1.5f);
		spiralp->setFinalRadius(1.f);
		spiralp->setSpinRate(4.f);
		spiralp->setFlickerRate(200.f);
		spiralp->setScaleBase(0.1f);
		spiralp->setScaleVar(0.1f);
		spiralp->setColor(LLColor4U(255, 255, 255, 255));
		break;
	case LL_HUD_EFFECT_LOOKAT:
		hud_objectp = new LLHUDEffectLookAt(type);
		break;
	case LL_HUD_EFFECT_VOICE_VISUALIZER:
		hud_objectp = new LLVoiceVisualizer(type);
		break;
	case LL_HUD_EFFECT_POINTAT:
		hud_objectp = new LLHUDEffectPointAt(type);
		break;
	case LL_HUD_EFFECT_BLOB:
		hud_objectp = new LLHUDEffectBlob(type);
		break;
	default:
		LL_WARNS() << "Unknown type of hud effect:" << (U32) type << LL_ENDL;
	}

	if (spiralp)
	{
		hud_objectp = spiralp;
	}

	if (hud_objectp)
	{
		sHUDObjects.push_back(hud_objectp);
	}
	return hud_objectp;
}

static LLTrace::BlockTimerStatHandle FTM_HUD_UPDATE("Update Hud");

// static
void LLHUDObject::updateAll()
{
	LL_RECORD_BLOCK_TIME(FTM_HUD_UPDATE);
	LLHUDText::updateAll();
	LLHUDIcon::updateAll();
	LLHUDNameTag::updateAll();
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

	LLVertexBuffer::unbind();
}

// static
void LLHUDObject::renderAllForTimer()
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
			hud_objp->renderForTimer();
		}
	}
}

// static
void LLHUDObject::reshapeAll()
{
	// only hud objects that use fonts care about window size/scale changes
	LLHUDText::reshape();
	LLHUDNameTag::reshape();
}

// static
void LLHUDObject::sortObjects()
{
	sHUDObjects.sort(hud_object_further_away());	
}
