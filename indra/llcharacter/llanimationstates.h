/** 
 * @file llanimationstates.h
 * @brief Implementation of animation state support.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLANIMATIONSTATES_H
#define LL_LLANIMATIONSTATES_H

#include <map>

#include "string_table.h"
#include "lluuid.h"

//-----------------------------------------------------------------------------
// These bit flags are generally used to track the animation state
// of characters.  The simulator and viewer share these flags to interpret
// the Animation name/value attribute on agents.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Agent Animation State
//-----------------------------------------------------------------------------
const S32 MAX_CONCURRENT_ANIMS = 16;

extern const LLUUID ANIM_AGENT_AFRAID;
extern const LLUUID ANIM_AGENT_AIM_BAZOOKA_R;
extern const LLUUID ANIM_AGENT_AIM_BOW_L;
extern const LLUUID ANIM_AGENT_AIM_HANDGUN_R;
extern const LLUUID ANIM_AGENT_AIM_RIFLE_R;
extern const LLUUID ANIM_AGENT_ANGRY;
extern const LLUUID ANIM_AGENT_AWAY;
extern const LLUUID ANIM_AGENT_BACKFLIP;
extern const LLUUID ANIM_AGENT_BELLY_LAUGH;
extern const LLUUID ANIM_AGENT_BLOW_KISS;
extern const LLUUID ANIM_AGENT_BORED;
extern const LLUUID ANIM_AGENT_BOW;
extern const LLUUID ANIM_AGENT_BRUSH;
extern const LLUUID ANIM_AGENT_BUSY;
extern const LLUUID ANIM_AGENT_CLAP;
extern const LLUUID ANIM_AGENT_COURTBOW;
extern const LLUUID ANIM_AGENT_CROUCH;
extern const LLUUID ANIM_AGENT_CROUCHWALK;
extern const LLUUID ANIM_AGENT_CRY;
extern const LLUUID ANIM_AGENT_CUSTOMIZE;
extern const LLUUID ANIM_AGENT_CUSTOMIZE_DONE;
extern const LLUUID ANIM_AGENT_DANCE1;
extern const LLUUID ANIM_AGENT_DANCE2;
extern const LLUUID ANIM_AGENT_DANCE3;
extern const LLUUID ANIM_AGENT_DANCE4;
extern const LLUUID ANIM_AGENT_DANCE5;
extern const LLUUID ANIM_AGENT_DANCE6;
extern const LLUUID ANIM_AGENT_DANCE7;
extern const LLUUID ANIM_AGENT_DANCE8;
extern const LLUUID ANIM_AGENT_DEAD;
extern const LLUUID ANIM_AGENT_DRINK;
extern const LLUUID ANIM_AGENT_EMBARRASSED;
extern const LLUUID ANIM_AGENT_EXPRESS_AFRAID;
extern const LLUUID ANIM_AGENT_EXPRESS_ANGER;
extern const LLUUID ANIM_AGENT_EXPRESS_BORED;
extern const LLUUID ANIM_AGENT_EXPRESS_CRY;
extern const LLUUID ANIM_AGENT_EXPRESS_DISDAIN;
extern const LLUUID ANIM_AGENT_EXPRESS_EMBARRASSED;
extern const LLUUID ANIM_AGENT_EXPRESS_FROWN;
extern const LLUUID ANIM_AGENT_EXPRESS_KISS;
extern const LLUUID ANIM_AGENT_EXPRESS_LAUGH;
extern const LLUUID ANIM_AGENT_EXPRESS_OPEN_MOUTH;
extern const LLUUID ANIM_AGENT_EXPRESS_REPULSED;
extern const LLUUID ANIM_AGENT_EXPRESS_SAD;
extern const LLUUID ANIM_AGENT_EXPRESS_SHRUG;
extern const LLUUID ANIM_AGENT_EXPRESS_SMILE;
extern const LLUUID ANIM_AGENT_EXPRESS_SURPRISE;
extern const LLUUID ANIM_AGENT_EXPRESS_TONGUE_OUT;
extern const LLUUID ANIM_AGENT_EXPRESS_TOOTHSMILE;
extern const LLUUID ANIM_AGENT_EXPRESS_WINK;
extern const LLUUID ANIM_AGENT_EXPRESS_WORRY;
extern const LLUUID ANIM_AGENT_FALLDOWN;
extern const LLUUID ANIM_AGENT_FEMALE_RUN_NEW;
extern const LLUUID ANIM_AGENT_FEMALE_WALK;
extern const LLUUID ANIM_AGENT_FEMALE_WALK_NEW;
extern const LLUUID ANIM_AGENT_FINGER_WAG;
extern const LLUUID ANIM_AGENT_FIST_PUMP;
extern const LLUUID ANIM_AGENT_FLY;
extern const LLUUID ANIM_AGENT_FLYSLOW;
extern const LLUUID ANIM_AGENT_HELLO;
extern const LLUUID ANIM_AGENT_HOLD_BAZOOKA_R;
extern const LLUUID ANIM_AGENT_HOLD_BOW_L;
extern const LLUUID ANIM_AGENT_HOLD_HANDGUN_R;
extern const LLUUID ANIM_AGENT_HOLD_RIFLE_R;
extern const LLUUID ANIM_AGENT_HOLD_THROW_R;
extern const LLUUID ANIM_AGENT_HOVER;
extern const LLUUID ANIM_AGENT_HOVER_DOWN;
extern const LLUUID ANIM_AGENT_HOVER_UP;
extern const LLUUID ANIM_AGENT_IMPATIENT;
extern const LLUUID ANIM_AGENT_JUMP;
extern const LLUUID ANIM_AGENT_JUMP_FOR_JOY;
extern const LLUUID ANIM_AGENT_KISS_MY_BUTT;
extern const LLUUID ANIM_AGENT_LAND;
extern const LLUUID ANIM_AGENT_LAUGH_SHORT;
extern const LLUUID ANIM_AGENT_MEDIUM_LAND;
extern const LLUUID ANIM_AGENT_MOTORCYCLE_SIT;
extern const LLUUID ANIM_AGENT_MUSCLE_BEACH;
extern const LLUUID ANIM_AGENT_NO;
extern const LLUUID ANIM_AGENT_NO_UNHAPPY;
extern const LLUUID ANIM_AGENT_NYAH_NYAH;
extern const LLUUID ANIM_AGENT_ONETWO_PUNCH;
extern const LLUUID ANIM_AGENT_PEACE;
extern const LLUUID ANIM_AGENT_POINT_ME;
extern const LLUUID ANIM_AGENT_POINT_YOU;
extern const LLUUID ANIM_AGENT_PRE_JUMP;
extern const LLUUID ANIM_AGENT_PUNCH_LEFT;
extern const LLUUID ANIM_AGENT_PUNCH_RIGHT;
extern const LLUUID ANIM_AGENT_REPULSED;
extern const LLUUID ANIM_AGENT_ROUNDHOUSE_KICK;
extern const LLUUID ANIM_AGENT_RPS_COUNTDOWN;
extern const LLUUID ANIM_AGENT_RPS_PAPER;
extern const LLUUID ANIM_AGENT_RPS_ROCK;
extern const LLUUID ANIM_AGENT_RPS_SCISSORS;
extern const LLUUID ANIM_AGENT_RUN;
extern const LLUUID ANIM_AGENT_RUN_NEW;
extern const LLUUID ANIM_AGENT_SAD;
extern const LLUUID ANIM_AGENT_SALUTE;
extern const LLUUID ANIM_AGENT_SHOOT_BOW_L;
extern const LLUUID ANIM_AGENT_SHOUT;
extern const LLUUID ANIM_AGENT_SHRUG;
extern const LLUUID ANIM_AGENT_SIT;
extern const LLUUID ANIM_AGENT_SIT_FEMALE;
extern const LLUUID ANIM_AGENT_SIT_GENERIC;
extern const LLUUID ANIM_AGENT_SIT_GROUND;
extern const LLUUID ANIM_AGENT_SIT_GROUND_CONSTRAINED;
extern const LLUUID ANIM_AGENT_SIT_TO_STAND;
extern const LLUUID ANIM_AGENT_SLEEP;
extern const LLUUID ANIM_AGENT_SMOKE_IDLE;
extern const LLUUID ANIM_AGENT_SMOKE_INHALE;
extern const LLUUID ANIM_AGENT_SMOKE_THROW_DOWN;
extern const LLUUID ANIM_AGENT_SNAPSHOT;
extern const LLUUID ANIM_AGENT_STAND;
extern const LLUUID ANIM_AGENT_STANDUP;
extern const LLUUID ANIM_AGENT_STAND_1;
extern const LLUUID ANIM_AGENT_STAND_2;
extern const LLUUID ANIM_AGENT_STAND_3;
extern const LLUUID ANIM_AGENT_STAND_4;
extern const LLUUID ANIM_AGENT_STRETCH;
extern const LLUUID ANIM_AGENT_STRIDE;
extern const LLUUID ANIM_AGENT_SURF;
extern const LLUUID ANIM_AGENT_SURPRISE;
extern const LLUUID ANIM_AGENT_SWORD_STRIKE;
extern const LLUUID ANIM_AGENT_TALK;
extern const LLUUID ANIM_AGENT_TANTRUM;
extern const LLUUID ANIM_AGENT_THROW_R;
extern const LLUUID ANIM_AGENT_TRYON_SHIRT;
extern const LLUUID ANIM_AGENT_TURNLEFT;
extern const LLUUID ANIM_AGENT_TURNRIGHT;
extern const LLUUID ANIM_AGENT_TYPE;
extern const LLUUID ANIM_AGENT_WALK;
extern const LLUUID ANIM_AGENT_WALK_NEW;
extern const LLUUID ANIM_AGENT_WHISPER;
extern const LLUUID ANIM_AGENT_WHISTLE;
extern const LLUUID ANIM_AGENT_WINK;
extern const LLUUID ANIM_AGENT_WINK_HOLLYWOOD;
extern const LLUUID ANIM_AGENT_WORRY;
extern const LLUUID ANIM_AGENT_YES;
extern const LLUUID ANIM_AGENT_YES_HAPPY;
extern const LLUUID ANIM_AGENT_YOGA_FLOAT;

extern LLUUID AGENT_WALK_ANIMS[];
extern S32 NUM_AGENT_WALK_ANIMS;

extern LLUUID AGENT_GUN_HOLD_ANIMS[];
extern S32 NUM_AGENT_GUN_HOLD_ANIMS;

extern LLUUID AGENT_GUN_AIM_ANIMS[];
extern S32 NUM_AGENT_GUN_AIM_ANIMS;

extern LLUUID AGENT_NO_ROTATE_ANIMS[];
extern S32 NUM_AGENT_NO_ROTATE_ANIMS;

extern LLUUID AGENT_STAND_ANIMS[];
extern S32 NUM_AGENT_STAND_ANIMS;

class LLAnimationLibrary
{
private:
	LLStringTable mAnimStringTable;

	typedef std::map<LLUUID, char *> anim_map_t;
	anim_map_t mAnimMap;

public:
	LLAnimationLibrary();
	~LLAnimationLibrary();

	//-----------------------------------------------------------------------------
	// Return the text name of a single animation state,
	// Return NULL if the state is invalid
	//-----------------------------------------------------------------------------
	const char *animStateToString( const LLUUID& state );

	//-----------------------------------------------------------------------------
	// Return the animation state for the given name.
	// Retun NULL if the name is invalid.
	//-----------------------------------------------------------------------------
	LLUUID stringToAnimState( const std::string& name, BOOL allow_ids = TRUE );

	//-----------------------------------------------------------------------------
	// Associate an anim state with a name
	//-----------------------------------------------------------------------------
	void animStateSetString( const LLUUID& state, const std::string& name);

	//-----------------------------------------------------------------------------
	// Find the name for a given animation, or UUID string if none defined.
	//-----------------------------------------------------------------------------
	std::string animationName( const LLUUID& id ) const;
};

struct LLAnimStateEntry
{
	LLAnimStateEntry(const char* name, const LLUUID& id) :
		mName(name),
		mID(id)
	{ 
		// LABELS:
		// Look to newview/LLAnimStateLabels.* for how to get the labels.
		// The labels should no longer be stored in this structure. The server
		// shouldn't care about the local friendly name of an animation, and
		// this is common code.
	}


	const char* mName;
	const LLUUID mID;
};

// Animation states that the user can trigger
extern const LLAnimStateEntry gUserAnimStates[];
extern const S32 gUserAnimStatesCount;
extern LLAnimationLibrary gAnimLibrary;

#endif // LL_LLANIMATIONSTATES_H



