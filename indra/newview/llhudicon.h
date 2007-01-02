/** 
 * @file llhudicon.h
 * @brief LLHUDIcon class definition
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLHUDICON_H
#define LL_LLHUDICON_H

#include "llmemory.h"
#include "lldarrayptr.h"

#include "llhudobject.h"
#include "v4color.h"
#include "v4coloru.h"
#include "v2math.h"
#include "llrect.h"
#include "llframetimer.h"
#include "llfontgl.h"
#include <set>
#include <vector>
#include "lldarray.h"

// Renders a 2D icon billboard floating at the location specified.
class LLDrawable;
class LLViewerObject;

class LLHUDIcon : public LLHUDObject
{
friend class LLHUDObject;

public:
	/*virtual*/ void render();
	/*virtual*/ void renderForSelect();
	/*virtual*/ void markDead();
	/*virtual*/ F32 getDistance() const { return mDistance; }

	void setImage(LLViewerImage* imagep);
	void setScale(F32 fraction_of_fov);

	void restartLifeTimer() { mLifeTimer.reset(); }

	static S32 generatePickIDs(S32 start_id, S32 step_size);
	static LLHUDIcon* handlePick(S32 pick_id);

	static void updateAll();
	static void cleanupDeadIcons();
	static S32 getNumInstances();

	static BOOL iconsNearby();

protected:
	LLHUDIcon(const U8 type);
	~LLHUDIcon();

	void renderIcon(BOOL for_select); // common render code

private:
	LLPointer<LLViewerImage> mImagep;
	LLFrameTimer	mAnimTimer;
	LLFrameTimer	mLifeTimer;
	F32				mDistance;
	S32				mPickID;
	F32				mScale;

	typedef std::vector<LLPointer<LLHUDIcon> > icon_instance_t;
	static icon_instance_t sIconInstances;
};

#endif // LL_LLHUDICON_H
