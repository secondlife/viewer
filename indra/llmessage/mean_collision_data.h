/** 
 * @file mean_collision_data.h
 * @brief data type to log interactions between stuff and agents that
 * might be community standards violations
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_MEAN_COLLISIONS_DATA_H
#define LL_MEAN_COLLISIONS_DATA_H

#include "lldbstrings.h"
#include "lluuid.h"

const F32 MEAN_COLLISION_TIMEOUT = 5.f;
const S32 MAX_MEAN_COLLISIONS = 5;

typedef enum e_mean_collision_types
{
	MEAN_INVALID,
	MEAN_BUMP,
	MEAN_LLPUSHOBJECT,
	MEAN_SELECTED_OBJECT_COLLIDE,
	MEAN_SCRIPTED_OBJECT_COLLIDE,
	MEAN_PHYSICAL_OBJECT_COLLIDE,
	MEAN_EOF
} EMeanCollisionType;

class LLMeanCollisionData
{
public:
	LLMeanCollisionData(const LLUUID &victim, const LLUUID &perp, time_t time, EMeanCollisionType type, F32 mag)
		: mVictim(victim), mPerp(perp), mTime(time), mType(type), mMag(mag)
	{
	}
	
	LLMeanCollisionData(LLMeanCollisionData *mcd)
		: mVictim(mcd->mVictim), mPerp(mcd->mPerp), mTime(mcd->mTime), mType(mcd->mType), mMag(mcd->mMag),
		  mFullName(mcd->mFullName)
	{
	}		
	
	friend std::ostream&	 operator<<(std::ostream& s, const LLMeanCollisionData &a)
	{
		switch(a.mType)
		{
		case MEAN_BUMP:
			s << "Mean Collision: " << a.mPerp << " bumped " << a.mVictim << " with a velocity of " << a.mMag << " at " << ctime(&a.mTime);
			break;
		case MEAN_LLPUSHOBJECT:
			s << "Mean Collision: " << a.mPerp << " llPushObject-ed " << a.mVictim << " with a total force of " << a.mMag  << " at "<<  ctime(&a.mTime);
			break;
		case MEAN_SELECTED_OBJECT_COLLIDE:
			s << "Mean Collision: " << a.mPerp << " dragged an object into " << a.mVictim << " with a velocity of " << a.mMag  << " at "<<  ctime(&a.mTime);
			break;
		case MEAN_SCRIPTED_OBJECT_COLLIDE:
			s << "Mean Collision: " << a.mPerp << " smacked " << a.mVictim << " with a scripted object with velocity of " << a.mMag  << " at "<<  ctime(&a.mTime);
			break;
		case MEAN_PHYSICAL_OBJECT_COLLIDE:
			s << "Mean Collision: " << a.mPerp << " smacked " << a.mVictim << " with a physical object with velocity of " << a.mMag  << " at "<<  ctime(&a.mTime);
			break;
		default:
			break;
		}
		return s;
	}

	LLUUID mVictim;
	LLUUID mPerp;
	time_t mTime;
	EMeanCollisionType mType;
	F32	   mMag;
	std::string mFullName;
};


#endif
