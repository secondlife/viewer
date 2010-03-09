/** 
 * @file llhudobject.h
 * @brief LLHUDObject class definition
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

#ifndef LL_LLHUDOBJECT_H
#define LL_LLHUDOBJECT_H

/**
 * Base class and manager for in-world 2.5D non-interactive objects
 */

#include "llpointer.h"

#include "v4color.h"
#include "v3math.h"
#include "v3dmath.h"
#include "lldrawpool.h"		// TODO: eliminate, unused below
#include <list>

class LLViewerCamera;
class LLFontGL;
class LLFace;
class LLViewerObject;
class LLHUDEffect;

class LLHUDObject : public LLRefCount
{
public:
	virtual void markDead();
	virtual F32 getDistance() const;
	virtual void setSourceObject(LLViewerObject* objectp);
	virtual void setTargetObject(LLViewerObject* objectp);
	virtual LLViewerObject* getSourceObject() { return mSourceObject; }
	virtual LLViewerObject* getTargetObject() { return mTargetObject; }

	void setPositionGlobal(const LLVector3d &position_global);
	void setPositionAgent(const LLVector3 &position_agent);

	BOOL isVisible() const { return mVisible; }

	U8 getType() const { return mType; }

	LLVector3d getPositionGlobal() const { return mPositionGlobal; }

	static LLHUDObject *addHUDObject(const U8 type);
	static LLHUDEffect *addHUDEffect(const U8 type);
	static void updateAll();
	static void renderAll();
	static void renderAllForSelect();

	// Some objects may need to update when window shape changes
	static void reshapeAll();

	static void cleanupHUDObjects();

	enum
	{
		LL_HUD_TEXT,
		LL_HUD_ICON,
		LL_HUD_CONNECTOR,
		LL_HUD_FLEXIBLE_OBJECT,			// Ventrella
		LL_HUD_ANIMAL_CONTROLS,			// Ventrella
		LL_HUD_LOCAL_ANIMATION_OBJECT,	// Ventrella
		LL_HUD_CLOTH,					// Ventrella
		LL_HUD_EFFECT_BEAM,
		LL_HUD_EFFECT_GLOW,
		LL_HUD_EFFECT_POINT,
		LL_HUD_EFFECT_TRAIL,
		LL_HUD_EFFECT_SPHERE,
		LL_HUD_EFFECT_SPIRAL,
		LL_HUD_EFFECT_EDIT,
		LL_HUD_EFFECT_LOOKAT,
		LL_HUD_EFFECT_POINTAT,
		LL_HUD_EFFECT_VOICE_VISUALIZER,	// Ventrella
		LL_HUD_NAME_TAG
	};
protected:
	static void sortObjects();

	LLHUDObject(const U8 type);
	~LLHUDObject();

	virtual void render() = 0;
	virtual void renderForSelect() {};
	
protected:
	U8				mType;
	BOOL			mDead;
	BOOL			mVisible;
	LLVector3d		mPositionGlobal;
	LLPointer<LLViewerObject> mSourceObject;
	LLPointer<LLViewerObject> mTargetObject;

private:
	typedef std::list<LLPointer<LLHUDObject> > hud_object_list_t;
	static hud_object_list_t sHUDObjects;
};

#endif // LL_LLHUDOBJECT_H
