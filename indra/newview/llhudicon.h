/** 
 * @file llhudicon.h
 * @brief LLHUDIcon class definition
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#ifndef LL_LLHUDICON_H
#define LL_LLHUDICON_H

#include "llpointer.h"
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
class LLViewerTexture;

class LLHUDIcon : public LLHUDObject
{
friend class LLHUDObject;

public:
	/*virtual*/ void render();
	/*virtual*/ void renderForSelect();
	/*virtual*/ void markDead();
	/*virtual*/ F32 getDistance() const { return mDistance; }

	void setImage(LLViewerTexture* imagep);
	void setScale(F32 fraction_of_fov);

	void restartLifeTimer() { mLifeTimer.reset(); }

	static S32 generatePickIDs(S32 start_id, S32 step_size);
	static LLHUDIcon* handlePick(S32 pick_id);
	static LLHUDIcon* lineSegmentIntersectAll(const LLVector3& start, const LLVector3& end, LLVector3* intersection);

	static void updateAll();
	static void cleanupDeadIcons();
	static S32 getNumInstances();

	static BOOL iconsNearby();

	BOOL getHidden() const { return mHidden; }
	void setHidden( BOOL hide ) { mHidden = hide; }

	BOOL lineSegmentIntersect(const LLVector3& start, const LLVector3& end, LLVector3* intersection);

protected:
	LLHUDIcon(const U8 type);
	~LLHUDIcon();

	void renderIcon(BOOL for_select); // common render code

private:
	LLPointer<LLViewerTexture> mImagep;
	LLFrameTimer	mAnimTimer;
	LLFrameTimer	mLifeTimer;
	F32				mDistance;
	S32				mPickID;
	F32				mScale;
	BOOL			mHidden;

	typedef std::vector<LLPointer<LLHUDIcon> > icon_instance_t;
	static icon_instance_t sIconInstances;
};

#endif // LL_LLHUDICON_H
