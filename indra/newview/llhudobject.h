/** 
 * @file llhudobject.h
 * @brief LLHUDObject class definition
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
	static void renderAllForTimer();

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
	virtual void renderForTimer() {};
	
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
