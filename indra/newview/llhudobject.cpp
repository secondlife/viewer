/** 
 * @file llhudobject.cpp
 * @brief LLHUDObject class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llhudobject.h"
#include "llviewerobject.h"

#include "llhudtext.h"
#include "llhudicon.h"
#include "llhudeffectbeam.h"
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
			llinfos << "LLHUDObject " << (LLHUDObject *)(*object_it) << " type " << (S32)(*object_it)->getType() << " has " << (*object_it)->getNumRefs() << " refs!" << llendl;			
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
		llwarns << "Unknown type of hud object:" << (U32) type << llendl;
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
	
	switch (type)
	{
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
		// deprecated
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
	case LL_HUD_EFFECT_VOICE_VISUALIZER:
		hud_objectp = new LLVoiceVisualizer(type);
		break;
	case LL_HUD_EFFECT_POINTAT:
		hud_objectp = new LLHUDEffectPointAt(type);
		break;
	default:
		llwarns << "Unknown type of hud effect:" << (U32) type << llendl;
	}

	if (hud_objectp)
	{
		sHUDObjects.push_back(hud_objectp);
	}
	return hud_objectp;
}

static LLFastTimer::DeclareTimer FTM_HUD_UPDATE("Update Hud");

// static
void LLHUDObject::updateAll()
{
	LLFastTimer ftm(FTM_HUD_UPDATE);
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
