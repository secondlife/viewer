/** 
 * @file llhudobject.h
 * @brief LLHUDObject class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLHUDOBJECT_H
#define LL_LLHUDOBJECT_H

/**
 * Base class and manager for in-world 2.5D non-interactive objects
 */

#include "llmemory.h"

#include "v4color.h"
#include "v3math.h"
#include "v3dmath.h"
#include "linked_lists.h"
#include "lldrawpool.h"
#include <list>

class LLViewerCamera;
class LLFontGL;
class LLFace;
class LLViewerObject;

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

	static LLHUDObject *addHUDObject(const U8 type);
	static void updateAll();
	static void renderAll();
	static void renderAllForSelect();

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
		LL_HUD_EFFECT_POINTAT
	};
protected:
	static void sortObjects();

	LLHUDObject(const U8 type);
	virtual ~LLHUDObject();

	virtual void render() = 0;
	virtual void renderForSelect() {};
	
protected:
	U8				mType;
	BOOL			mDead;
	BOOL			mVisible;
	LLVector3d		mPositionGlobal;
	BOOL			mOnHUDAttachment;
	LLPointer<LLViewerObject> mSourceObject;
	LLPointer<LLViewerObject> mTargetObject;

private:
	typedef std::list<LLPointer<LLHUDObject> > hud_object_list_t;
	static hud_object_list_t sHUDObjects;
};

#endif // LL_LLHUDOBJECT_H
