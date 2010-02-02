/** 
 * @file mean_collision_data.h
 * @brief data type to log interactions between stuff and agents that
 * might be community standards violations
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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
