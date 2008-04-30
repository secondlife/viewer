/** 
 * @file llanimationstates.cpp
 * @brief Implementation of animation state related functions.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

//-----------------------------------------------------------------------------
// Agent Animation State
//-----------------------------------------------------------------------------

#include "linden_common.h"

#include "llanimationstates.h"
#include "llstring.h"

LLUUID AGENT_WALK_ANIMS[] = {ANIM_AGENT_WALK, ANIM_AGENT_RUN, ANIM_AGENT_CROUCHWALK, ANIM_AGENT_TURNLEFT, ANIM_AGENT_TURNRIGHT};
S32 NUM_AGENT_WALK_ANIMS = sizeof(AGENT_WALK_ANIMS) / sizeof(LLUUID);

LLUUID AGENT_GUN_HOLD_ANIMS[] = {ANIM_AGENT_HOLD_RIFLE_R, ANIM_AGENT_HOLD_HANDGUN_R, ANIM_AGENT_HOLD_BAZOOKA_R, ANIM_AGENT_HOLD_BOW_L};
S32 NUM_AGENT_GUN_HOLD_ANIMS = sizeof(AGENT_GUN_HOLD_ANIMS) / sizeof(LLUUID);

LLUUID AGENT_GUN_AIM_ANIMS[] = {ANIM_AGENT_AIM_RIFLE_R, ANIM_AGENT_AIM_HANDGUN_R, ANIM_AGENT_AIM_BAZOOKA_R, ANIM_AGENT_AIM_BOW_L};
S32 NUM_AGENT_GUN_AIM_ANIMS = sizeof(AGENT_GUN_AIM_ANIMS) / sizeof(LLUUID);

LLUUID AGENT_NO_ROTATE_ANIMS[] = {ANIM_AGENT_SIT_GROUND, ANIM_AGENT_SIT_GROUND_CONSTRAINED, ANIM_AGENT_STANDUP};
S32 NUM_AGENT_NO_ROTATE_ANIMS = sizeof(AGENT_NO_ROTATE_ANIMS) / sizeof(LLUUID);

LLUUID AGENT_STAND_ANIMS[] = {ANIM_AGENT_STAND, ANIM_AGENT_STAND_1, ANIM_AGENT_STAND_2, ANIM_AGENT_STAND_3, ANIM_AGENT_STAND_4};
S32 NUM_AGENT_STAND_ANIMS = sizeof(AGENT_STAND_ANIMS) / sizeof(LLUUID);


LLAnimationLibrary gAnimLibrary;

//-----------------------------------------------------------------------------
// LLAnimationLibrary()
//-----------------------------------------------------------------------------
LLAnimationLibrary::LLAnimationLibrary() :
	mAnimStringTable(16384)
{
	//add animation names to animmap
	mAnimMap[ANIM_AGENT_AFRAID]=			mAnimStringTable.addString("express_afraid");
	mAnimMap[ANIM_AGENT_AIM_BAZOOKA_R]=		mAnimStringTable.addString("aim_r_bazooka");
	mAnimMap[ANIM_AGENT_AIM_BOW_L]=   		mAnimStringTable.addString("aim_l_bow");
	mAnimMap[ANIM_AGENT_AIM_HANDGUN_R]=		mAnimStringTable.addString("aim_r_handgun");
	mAnimMap[ANIM_AGENT_AIM_RIFLE_R]=		mAnimStringTable.addString("aim_r_rifle");
	mAnimMap[ANIM_AGENT_ANGRY]=				mAnimStringTable.addString("express_anger");
	mAnimMap[ANIM_AGENT_AWAY]=				mAnimStringTable.addString("away");
	mAnimMap[ANIM_AGENT_BACKFLIP]=			mAnimStringTable.addString("backflip");
	mAnimMap[ANIM_AGENT_BELLY_LAUGH]=		mAnimStringTable.addString("express_laugh");
	mAnimMap[ANIM_AGENT_BLOW_KISS]=			mAnimStringTable.addString("blowkiss");
	mAnimMap[ANIM_AGENT_BORED]=				mAnimStringTable.addString("express_bored");
	mAnimMap[ANIM_AGENT_BOW]=				mAnimStringTable.addString("bow");
	mAnimMap[ANIM_AGENT_BRUSH]=				mAnimStringTable.addString("brush");
	mAnimMap[ANIM_AGENT_BUSY]=				mAnimStringTable.addString("busy");
	mAnimMap[ANIM_AGENT_CLAP]=				mAnimStringTable.addString("clap");
	mAnimMap[ANIM_AGENT_COURTBOW]=			mAnimStringTable.addString("courtbow");
	mAnimMap[ANIM_AGENT_CROUCH]=			mAnimStringTable.addString("crouch");
	mAnimMap[ANIM_AGENT_CROUCHWALK]=		mAnimStringTable.addString("crouchwalk");
	mAnimMap[ANIM_AGENT_CRY]=				mAnimStringTable.addString("express_cry");
	mAnimMap[ANIM_AGENT_CUSTOMIZE]=			mAnimStringTable.addString("turn_180");
	mAnimMap[ANIM_AGENT_CUSTOMIZE_DONE]=	mAnimStringTable.addString("turnback_180");
	mAnimMap[ANIM_AGENT_DANCE1]=			mAnimStringTable.addString("dance1");
	mAnimMap[ANIM_AGENT_DANCE2]=			mAnimStringTable.addString("dance2");
	mAnimMap[ANIM_AGENT_DANCE3]=			mAnimStringTable.addString("dance3");
	mAnimMap[ANIM_AGENT_DANCE4]=			mAnimStringTable.addString("dance4");
	mAnimMap[ANIM_AGENT_DANCE5]=			mAnimStringTable.addString("dance5");
	mAnimMap[ANIM_AGENT_DANCE6]=			mAnimStringTable.addString("dance6");
	mAnimMap[ANIM_AGENT_DANCE7]=			mAnimStringTable.addString("dance7");
	mAnimMap[ANIM_AGENT_DANCE8]=			mAnimStringTable.addString("dance8");
	mAnimMap[ANIM_AGENT_DEAD]=				mAnimStringTable.addString("dead");
	mAnimMap[ANIM_AGENT_DRINK]=				mAnimStringTable.addString("drink");
	mAnimMap[ANIM_AGENT_EMBARRASSED]=		mAnimStringTable.addString("express_embarrased");
	mAnimMap[ANIM_AGENT_EXPRESS_AFRAID]=	mAnimStringTable.addString("express_afraid_emote");
	mAnimMap[ANIM_AGENT_EXPRESS_ANGER]=		mAnimStringTable.addString("express_anger_emote");
	mAnimMap[ANIM_AGENT_EXPRESS_BORED]=		mAnimStringTable.addString("express_bored_emote");
	mAnimMap[ANIM_AGENT_EXPRESS_CRY]=		mAnimStringTable.addString("express_cry_emote");
	mAnimMap[ANIM_AGENT_EXPRESS_DISDAIN]=	mAnimStringTable.addString("express_disdain");
	mAnimMap[ANIM_AGENT_EXPRESS_EMBARRASSED]=	mAnimStringTable.addString("express_embarrassed_emote");
	mAnimMap[ANIM_AGENT_EXPRESS_FROWN]=			mAnimStringTable.addString("express_frown");
	mAnimMap[ANIM_AGENT_EXPRESS_KISS]=			mAnimStringTable.addString("express_kiss");
	mAnimMap[ANIM_AGENT_EXPRESS_LAUGH]=			mAnimStringTable.addString("express_laugh_emote");
	mAnimMap[ANIM_AGENT_EXPRESS_OPEN_MOUTH]=	mAnimStringTable.addString("express_open_mouth");
	mAnimMap[ANIM_AGENT_EXPRESS_REPULSED]=		mAnimStringTable.addString("express_repulsed_emote");
	mAnimMap[ANIM_AGENT_EXPRESS_SAD]=			mAnimStringTable.addString("express_sad_emote");
	mAnimMap[ANIM_AGENT_EXPRESS_SHRUG]=			mAnimStringTable.addString("express_shrug_emote");
	mAnimMap[ANIM_AGENT_EXPRESS_SMILE]=			mAnimStringTable.addString("express_smile");
	mAnimMap[ANIM_AGENT_EXPRESS_SURPRISE]=		mAnimStringTable.addString("express_surprise_emote");
	mAnimMap[ANIM_AGENT_EXPRESS_TONGUE_OUT]=	mAnimStringTable.addString("express_tongue_out");
	mAnimMap[ANIM_AGENT_EXPRESS_TOOTHSMILE]=	mAnimStringTable.addString("express_toothsmile");
	mAnimMap[ANIM_AGENT_EXPRESS_WINK]=			mAnimStringTable.addString("express_wink_emote");
	mAnimMap[ANIM_AGENT_EXPRESS_WORRY]=			mAnimStringTable.addString("express_worry_emote");
	mAnimMap[ANIM_AGENT_FALLDOWN]=				mAnimStringTable.addString("falldown");
	mAnimMap[ANIM_AGENT_FEMALE_WALK]=			mAnimStringTable.addString("female_walk");
	mAnimMap[ANIM_AGENT_FINGER_WAG]=			mAnimStringTable.addString("angry_fingerwag");
	mAnimMap[ANIM_AGENT_FIST_PUMP]=				mAnimStringTable.addString("fist_pump");
	mAnimMap[ANIM_AGENT_FLY]=					mAnimStringTable.addString("fly");
	mAnimMap[ANIM_AGENT_FLYSLOW]=				mAnimStringTable.addString("flyslow");
	mAnimMap[ANIM_AGENT_HELLO]=					mAnimStringTable.addString("hello");
	mAnimMap[ANIM_AGENT_HOLD_BAZOOKA_R]=		mAnimStringTable.addString("hold_r_bazooka");
	mAnimMap[ANIM_AGENT_HOLD_BOW_L]=			mAnimStringTable.addString("hold_l_bow");
	mAnimMap[ANIM_AGENT_HOLD_HANDGUN_R]=		mAnimStringTable.addString("hold_r_handgun");
	mAnimMap[ANIM_AGENT_HOLD_RIFLE_R]=			mAnimStringTable.addString("hold_r_rifle");
	mAnimMap[ANIM_AGENT_HOLD_THROW_R]=			mAnimStringTable.addString("hold_throw_r");
	mAnimMap[ANIM_AGENT_HOVER]=					mAnimStringTable.addString("hover");
	mAnimMap[ANIM_AGENT_HOVER_DOWN]=			mAnimStringTable.addString("hover_down");
	mAnimMap[ANIM_AGENT_HOVER_UP]=				mAnimStringTable.addString("hover_up");
	mAnimMap[ANIM_AGENT_IMPATIENT]=				mAnimStringTable.addString("impatient");
	mAnimMap[ANIM_AGENT_JUMP]=					mAnimStringTable.addString("jump");
	mAnimMap[ANIM_AGENT_JUMP_FOR_JOY]=			mAnimStringTable.addString("jumpforjoy");
	mAnimMap[ANIM_AGENT_KISS_MY_BUTT]=			mAnimStringTable.addString("kissmybutt");
	mAnimMap[ANIM_AGENT_LAND]=					mAnimStringTable.addString("land");
	mAnimMap[ANIM_AGENT_LAUGH_SHORT]=			mAnimStringTable.addString("laugh_short");
	mAnimMap[ANIM_AGENT_MEDIUM_LAND]=			mAnimStringTable.addString("soft_land");
	mAnimMap[ANIM_AGENT_MOTORCYCLE_SIT]=		mAnimStringTable.addString("motorcycle_sit");
	mAnimMap[ANIM_AGENT_MUSCLE_BEACH]=			mAnimStringTable.addString("musclebeach");
	mAnimMap[ANIM_AGENT_NO]=					mAnimStringTable.addString("no_head");
	mAnimMap[ANIM_AGENT_NO_UNHAPPY]=			mAnimStringTable.addString("no_unhappy");
	mAnimMap[ANIM_AGENT_NYAH_NYAH]=				mAnimStringTable.addString("nyanya");
	mAnimMap[ANIM_AGENT_ONETWO_PUNCH]=			mAnimStringTable.addString("punch_onetwo");
	mAnimMap[ANIM_AGENT_PEACE]=					mAnimStringTable.addString("peace");
	mAnimMap[ANIM_AGENT_POINT_ME]=				mAnimStringTable.addString("point_me");
	mAnimMap[ANIM_AGENT_POINT_YOU]=				mAnimStringTable.addString("point_you");
	mAnimMap[ANIM_AGENT_PRE_JUMP]=				mAnimStringTable.addString("prejump");
	mAnimMap[ANIM_AGENT_PUNCH_LEFT]=			mAnimStringTable.addString("punch_l");
	mAnimMap[ANIM_AGENT_PUNCH_RIGHT]=			mAnimStringTable.addString("punch_r");
	mAnimMap[ANIM_AGENT_REPULSED]=				mAnimStringTable.addString("express_repulsed");
	mAnimMap[ANIM_AGENT_ROUNDHOUSE_KICK]=		mAnimStringTable.addString("kick_roundhouse_r");
	mAnimMap[ANIM_AGENT_RPS_COUNTDOWN]=			mAnimStringTable.addString("rps_countdown");
	mAnimMap[ANIM_AGENT_RPS_PAPER]=				mAnimStringTable.addString("rps_paper");
	mAnimMap[ANIM_AGENT_RPS_ROCK]=				mAnimStringTable.addString("rps_rock");
	mAnimMap[ANIM_AGENT_RPS_SCISSORS]=			mAnimStringTable.addString("rps_scissors");
	mAnimMap[ANIM_AGENT_RUN]=					mAnimStringTable.addString("run");
	mAnimMap[ANIM_AGENT_SAD]=					mAnimStringTable.addString("express_sad");
	mAnimMap[ANIM_AGENT_SALUTE]=				mAnimStringTable.addString("salute");
	mAnimMap[ANIM_AGENT_SHOOT_BOW_L]=			mAnimStringTable.addString("shoot_l_bow");
	mAnimMap[ANIM_AGENT_SHOUT]=					mAnimStringTable.addString("shout");
	mAnimMap[ANIM_AGENT_SHRUG]=					mAnimStringTable.addString("express_shrug");
	mAnimMap[ANIM_AGENT_SIT]=					mAnimStringTable.addString("sit");
	mAnimMap[ANIM_AGENT_SIT_FEMALE]=			mAnimStringTable.addString("sit_female");
	mAnimMap[ANIM_AGENT_SIT_GROUND]=			mAnimStringTable.addString("sit_ground");
	mAnimMap[ANIM_AGENT_SIT_GROUND_CONSTRAINED]=	mAnimStringTable.addString("sit_ground_constrained");
	mAnimMap[ANIM_AGENT_SIT_GENERIC]=			mAnimStringTable.addString("sit_generic");
	mAnimMap[ANIM_AGENT_SIT_TO_STAND]=			mAnimStringTable.addString("sit_to_stand");
	mAnimMap[ANIM_AGENT_SLEEP]=					mAnimStringTable.addString("sleep");
	mAnimMap[ANIM_AGENT_SMOKE_IDLE]=			mAnimStringTable.addString("smoke_idle");
	mAnimMap[ANIM_AGENT_SMOKE_INHALE]=			mAnimStringTable.addString("smoke_inhale");
	mAnimMap[ANIM_AGENT_SMOKE_THROW_DOWN]=		mAnimStringTable.addString("smoke_throw_down");
	mAnimMap[ANIM_AGENT_SNAPSHOT]=				mAnimStringTable.addString("snapshot");
	mAnimMap[ANIM_AGENT_STAND]=					mAnimStringTable.addString("stand");
	mAnimMap[ANIM_AGENT_STANDUP]=				mAnimStringTable.addString("standup");
	mAnimMap[ANIM_AGENT_STAND_1]=				mAnimStringTable.addString("stand_1");
	mAnimMap[ANIM_AGENT_STAND_2]=				mAnimStringTable.addString("stand_2");
	mAnimMap[ANIM_AGENT_STAND_3]=				mAnimStringTable.addString("stand_3");
	mAnimMap[ANIM_AGENT_STAND_4]=				mAnimStringTable.addString("stand_4");
	mAnimMap[ANIM_AGENT_STRETCH]=				mAnimStringTable.addString("stretch");
	mAnimMap[ANIM_AGENT_STRIDE]=				mAnimStringTable.addString("stride");
	mAnimMap[ANIM_AGENT_SURF]=					mAnimStringTable.addString("surf");
	mAnimMap[ANIM_AGENT_SURPRISE]=				mAnimStringTable.addString("express_surprise");
	mAnimMap[ANIM_AGENT_SWORD_STRIKE]=			mAnimStringTable.addString("sword_strike_r");
	mAnimMap[ANIM_AGENT_TALK]=					mAnimStringTable.addString("talk");
	mAnimMap[ANIM_AGENT_TANTRUM]=				mAnimStringTable.addString("angry_tantrum");
	mAnimMap[ANIM_AGENT_THROW_R]=				mAnimStringTable.addString("throw_r");
	mAnimMap[ANIM_AGENT_TRYON_SHIRT]=			mAnimStringTable.addString("tryon_shirt");
	mAnimMap[ANIM_AGENT_TURNLEFT]=				mAnimStringTable.addString("turnleft");
	mAnimMap[ANIM_AGENT_TURNRIGHT]=				mAnimStringTable.addString("turnright");
	mAnimMap[ANIM_AGENT_TYPE]=					mAnimStringTable.addString("type");
	mAnimMap[ANIM_AGENT_WALK]=					mAnimStringTable.addString("walk");
	mAnimMap[ANIM_AGENT_WHISPER]=				mAnimStringTable.addString("whisper");
	mAnimMap[ANIM_AGENT_WHISTLE]=				mAnimStringTable.addString("whistle");
	mAnimMap[ANIM_AGENT_WINK]=					mAnimStringTable.addString("express_wink");
	mAnimMap[ANIM_AGENT_WINK_HOLLYWOOD]=		mAnimStringTable.addString("wink_hollywood");
	mAnimMap[ANIM_AGENT_WORRY]=					mAnimStringTable.addString("express_worry");
	mAnimMap[ANIM_AGENT_YES]=					mAnimStringTable.addString("yes_head");
	mAnimMap[ANIM_AGENT_YES_HAPPY]=				mAnimStringTable.addString("yes_happy");
	mAnimMap[ANIM_AGENT_YOGA_FLOAT]=			mAnimStringTable.addString("yoga_float");
}
	
//-----------------------------------------------------------------------------
// ~LLAnimationLibrary()
//-----------------------------------------------------------------------------
LLAnimationLibrary::~LLAnimationLibrary()
{
}

//-----------------------------------------------------------------------------
// Return the text name of an animation state
//-----------------------------------------------------------------------------
const char *LLAnimationLibrary::animStateToString( const LLUUID& state )
{
	if (state.isNull())
	{
		return NULL;
	}
	if (mAnimMap.count(state))
	{
		return mAnimMap[state];
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Return the animation state for a given name
//-----------------------------------------------------------------------------
LLUUID LLAnimationLibrary::stringToAnimState( const char *name, BOOL allow_ids )
{
	LLString lower_case_name(name);
	LLString::toLower(lower_case_name);

	char *true_name = mAnimStringTable.checkString(lower_case_name.c_str());

	LLUUID id;
	id.setNull();

	if (true_name)
	{
		for (anim_map_t::iterator iter = mAnimMap.begin();
			 iter != mAnimMap.end(); iter++)
		{
			if (iter->second == true_name)
			{
				id = iter->first;
				break;
			}
		}
	}
	else if (allow_ids)
	{
		// try to convert string to LLUUID
		id.set(name, FALSE);
	}

	return id;
}

// Animation states that the user can trigger as part of a gesture
// See struct LLAnimStateEntry in header for label location information
const LLAnimStateEntry gUserAnimStates[] = {
	LLAnimStateEntry("express_afraid",		ANIM_AGENT_AFRAID),
	LLAnimStateEntry("express_anger",		ANIM_AGENT_ANGRY),
	LLAnimStateEntry("away",					ANIM_AGENT_AWAY),
	LLAnimStateEntry("backflip",				ANIM_AGENT_BACKFLIP),
	LLAnimStateEntry("express_laugh",		ANIM_AGENT_BELLY_LAUGH),
	LLAnimStateEntry("express_toothsmile",	ANIM_AGENT_EXPRESS_TOOTHSMILE),
	LLAnimStateEntry("blowkiss",				ANIM_AGENT_BLOW_KISS),
	LLAnimStateEntry("express_bored",		ANIM_AGENT_BORED),
	LLAnimStateEntry("bow",					ANIM_AGENT_BOW),
	LLAnimStateEntry("clap",					ANIM_AGENT_CLAP),
	LLAnimStateEntry("courtbow",				ANIM_AGENT_COURTBOW),
	LLAnimStateEntry("express_cry",			ANIM_AGENT_CRY),
	LLAnimStateEntry("dance1",				ANIM_AGENT_DANCE1),
	LLAnimStateEntry("dance2",				ANIM_AGENT_DANCE2),
	LLAnimStateEntry("dance3",				ANIM_AGENT_DANCE3),
	LLAnimStateEntry("dance4",				ANIM_AGENT_DANCE4),
	LLAnimStateEntry("dance5",				ANIM_AGENT_DANCE5),
	LLAnimStateEntry("dance6",				ANIM_AGENT_DANCE6),
	LLAnimStateEntry("dance7",				ANIM_AGENT_DANCE7),
	LLAnimStateEntry("dance8",				ANIM_AGENT_DANCE8),
	LLAnimStateEntry("express_disdain",		ANIM_AGENT_EXPRESS_DISDAIN),
	LLAnimStateEntry("drink",				ANIM_AGENT_DRINK),
	LLAnimStateEntry("express_embarrased",	ANIM_AGENT_EMBARRASSED),
	LLAnimStateEntry("angry_fingerwag",		ANIM_AGENT_FINGER_WAG),
	LLAnimStateEntry("fist_pump",			ANIM_AGENT_FIST_PUMP),
	LLAnimStateEntry("yoga_float",			ANIM_AGENT_YOGA_FLOAT),
	LLAnimStateEntry("express_frown",		ANIM_AGENT_EXPRESS_FROWN),
	LLAnimStateEntry("impatient",			ANIM_AGENT_IMPATIENT),
	LLAnimStateEntry("jumpforjoy",			ANIM_AGENT_JUMP_FOR_JOY),
	LLAnimStateEntry("kissmybutt",			ANIM_AGENT_KISS_MY_BUTT),
	LLAnimStateEntry("express_kiss",			ANIM_AGENT_EXPRESS_KISS),
	LLAnimStateEntry("laugh_short",			ANIM_AGENT_LAUGH_SHORT),
	LLAnimStateEntry("musclebeach",			ANIM_AGENT_MUSCLE_BEACH),
	LLAnimStateEntry("no_unhappy",			ANIM_AGENT_NO_UNHAPPY),
	LLAnimStateEntry("no_head",				ANIM_AGENT_NO),
	LLAnimStateEntry("nyanya",				ANIM_AGENT_NYAH_NYAH),
	LLAnimStateEntry("punch_onetwo",			ANIM_AGENT_ONETWO_PUNCH),
	LLAnimStateEntry("express_open_mouth",	ANIM_AGENT_EXPRESS_OPEN_MOUTH),
	LLAnimStateEntry("peace",				ANIM_AGENT_PEACE),
	LLAnimStateEntry("point_you",			ANIM_AGENT_POINT_YOU),
	LLAnimStateEntry("point_me",				ANIM_AGENT_POINT_ME),
	LLAnimStateEntry("punch_l",				ANIM_AGENT_PUNCH_LEFT),
	LLAnimStateEntry("punch_r",				ANIM_AGENT_PUNCH_RIGHT),
	LLAnimStateEntry("rps_countdown",		ANIM_AGENT_RPS_COUNTDOWN),
	LLAnimStateEntry("rps_paper",			ANIM_AGENT_RPS_PAPER),
	LLAnimStateEntry("rps_rock",				ANIM_AGENT_RPS_ROCK),
	LLAnimStateEntry("rps_scissors",			ANIM_AGENT_RPS_SCISSORS),
	LLAnimStateEntry("express_repulsed",		ANIM_AGENT_EXPRESS_REPULSED),
	LLAnimStateEntry("kick_roundhouse_r",	ANIM_AGENT_ROUNDHOUSE_KICK),
	LLAnimStateEntry("express_sad",			ANIM_AGENT_SAD),
	LLAnimStateEntry("salute",				ANIM_AGENT_SALUTE),
	LLAnimStateEntry("shout",				ANIM_AGENT_SHOUT),
	LLAnimStateEntry("express_shrug",		ANIM_AGENT_SHRUG),
	LLAnimStateEntry("express_smile",		ANIM_AGENT_EXPRESS_SMILE),
	LLAnimStateEntry("smoke_idle",			ANIM_AGENT_SMOKE_IDLE),
	LLAnimStateEntry("smoke_inhale",			ANIM_AGENT_SMOKE_INHALE),
	LLAnimStateEntry("smoke_throw_down",		ANIM_AGENT_SMOKE_THROW_DOWN),
	LLAnimStateEntry("express_surprise",		ANIM_AGENT_SURPRISE),
	LLAnimStateEntry("sword_strike_r",		ANIM_AGENT_SWORD_STRIKE),
	LLAnimStateEntry("angry_tantrum",		ANIM_AGENT_TANTRUM),
	LLAnimStateEntry("express_tongue_out",	ANIM_AGENT_EXPRESS_TONGUE_OUT),
	LLAnimStateEntry("hello",				ANIM_AGENT_HELLO),
	LLAnimStateEntry("whisper",				ANIM_AGENT_WHISPER),
	LLAnimStateEntry("whistle",				ANIM_AGENT_WHISTLE),
	LLAnimStateEntry("express_wink",			ANIM_AGENT_WINK),
	LLAnimStateEntry("wink_hollywood",		ANIM_AGENT_WINK_HOLLYWOOD),
	LLAnimStateEntry("express_worry",		ANIM_AGENT_EXPRESS_WORRY),
	LLAnimStateEntry("yes_happy",			ANIM_AGENT_YES_HAPPY),
	LLAnimStateEntry("yes_head",				ANIM_AGENT_YES),
};

const S32 gUserAnimStatesCount = sizeof(gUserAnimStates) / sizeof(gUserAnimStates[0]);



// End

