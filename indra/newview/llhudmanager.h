/** 
 * @file llhudmanager.h
 * @brief LLHUDManager class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLHUDMANAGER_H
#define LL_LLHUDMANAGER_H

// Responsible for managing all HUD elements.

#include "llhudobject.h"
#include "lldarray.h"
#include "llanimalcontrols.h"
#include "lllocalanimationobject.h"
#include "llcape.h"

class LLViewerObject;
class LLHUDEffect;
//Ventrella 9/16/05
class LLHUDAnimalControls;
// End Ventrella
class LLMessageSystem;

class LLHUDManager
{
public:
	LLHUDManager();
	~LLHUDManager();

	void toggleShowPhysical(const BOOL show_physical);
	void showJoints(LLDynamicArray < LLViewerObject* > *object_list);
	void clearJoints();
	BOOL getShowPhysical() const;

	LLHUDEffect *createViewerEffect(const U8 type, BOOL send_to_sim = TRUE, BOOL originated_here = TRUE);

	void updateEffects();
	void sendEffects();
	void cleanupEffects();

	static void processViewerEffect(LLMessageSystem *mesgsys, void **user_data);

	static LLColor4 sParentColor;
	static LLColor4 sChildColor;

protected:
	LLDynamicArrayPtr<LLPointer<LLHUDObject>				> mHUDJoints;
	LLDynamicArrayPtr<LLPointer<LLHUDObject>				> mHUDSelectedJoints;
	LLDynamicArrayPtr<LLPointer<LLHUDEffect>				> mHUDEffects;

	// ALT held down this frame?
	BOOL mShowPhysical;
};

extern LLHUDManager *gHUDManager;

#endif // LL_LLHUDMANAGER_H
