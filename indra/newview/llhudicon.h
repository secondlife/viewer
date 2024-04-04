/** 
 * @file llhudicon.h
 * @brief LLHUDIcon class definition
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLHUDICON_H
#define LL_LLHUDICON_H

#include "llpointer.h"

#include "llhudobject.h"
#include "v4color.h"
#include "v4coloru.h"
#include "v2math.h"
#include "llrect.h"
#include "llframetimer.h"
#include "llfontgl.h"
#include <set>
#include <vector>

// Renders a 2D icon billboard floating at the location specified.
class LLViewerTexture;

class LLHUDIcon : public LLHUDObject
{
friend class LLHUDObject;

public:
	/*virtual*/ void render();
	/*virtual*/ void markDead();
	/*virtual*/ F32 getDistance() const { return mDistance; }

	void setImage(LLViewerTexture* imagep);
	void setScale(F32 fraction_of_fov);

	void restartLifeTimer() { mLifeTimer.reset(); }

	static LLHUDIcon* lineSegmentIntersectAll(const LLVector4a& start, const LLVector4a& end, LLVector4a* intersection);

	static void updateAll();
	static void cleanupDeadIcons();
	static S32 getNumInstances();

	static bool iconsNearby();

	bool getHidden() const { return mHidden; }
	void setHidden( bool hide ) { mHidden = hide; }

	bool lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end, LLVector4a* intersection);

protected:
	LLHUDIcon(const U8 type);
	~LLHUDIcon();

private:
	LLPointer<LLViewerTexture> mImagep;
	LLFrameTimer	mAnimTimer;
	LLFrameTimer	mLifeTimer;
	F32				mDistance;
	F32				mScale;
	bool			mHidden;

	typedef std::vector<LLPointer<LLHUDIcon> > icon_instance_t;
	static icon_instance_t sIconInstances;
};

#endif // LL_LLHUDICON_H
