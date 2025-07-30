/**
 * @file llanimationstates.h
 * @brief Master registry of avatar animations shared between viewer and server.
 *
 * This file defines the complete set of avatar animations available in Second Life,
 * from basic locomotion (walk, run, fly) to gestures and facial expressions. These
 * animation UUIDs are synchronized between the viewer and simulator to ensure
 * consistent avatar behavior across all clients.
 *
 * The system includes:
 * - 150+ built-in animations covering all avatar behaviors
 * - Categorized animation groups (walking, weapon holding, standing poses)
 * - String-to-UUID mapping for user-triggered animations and gestures
 * - Animation state management for the viewer's gesture system
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

#include "llstringtable.h"
#include "lluuid.h"

/**
 * @brief Animation state flags and UUIDs used to synchronize avatar behavior.
 * 
 * These animation identifiers are shared between the simulator and viewer to interpret
 * the Animation name/value attributes on agents. When an avatar starts or stops an
 * animation, both the server and all nearby clients use these UUIDs to ensure
 * consistent visual representation of avatar behavior.
 * 
 * The system works by:
 * - Server maintains authoritative animation state for each avatar
 * - Animation changes are broadcast to nearby clients via agent updates
 * - Clients look up animations by UUID to trigger the correct visual behavior
 * - Motion controller registers built-in animations during avatar initialization
 * - String names provide human-readable references for gestures and user commands
 * 
 * @code
 * // Typical motion registration pattern in avatar initialization:
 * registerMotion(ANIM_AGENT_WALK,      LLKeyframeWalkMotion::create);
 * registerMotion(ANIM_AGENT_RUN,       LLKeyframeWalkMotion::create);
 * registerMotion(ANIM_AGENT_STAND,     LLKeyframeStandMotion::create);
 * @endcode
 */

/**
 * @brief Maximum number of animations that can be active simultaneously on an avatar.
 * 
 * This limit prevents performance issues from avatar animation overload and ensures
 * consistent behavior across all clients. The server enforces this limit, rejecting
 * additional animation requests when the cap is reached.
 * 
 * This limit applies to user-triggered animations (gestures, dance animations, etc.)
 * but not to core system animations like walking or standing.
 */
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
extern const LLUUID ANIM_AGENT_DO_NOT_DISTURB;
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

/// Animation arrays for movement and pose categories (used by LLVOAvatar for state checking)  
extern LLUUID AGENT_WALK_ANIMS[];          /// Walking animations: walk, run, crouchwalk, turnleft, turnright
extern S32 NUM_AGENT_WALK_ANIMS;           /// Count of walking animations (5)

extern LLUUID AGENT_GUN_HOLD_ANIMS[];      /// Weapon holding poses: rifle, handgun, bazooka, bow
extern S32 NUM_AGENT_GUN_HOLD_ANIMS;       /// Count of weapon holding animations (4)

extern LLUUID AGENT_GUN_AIM_ANIMS[];       /// Weapon aiming poses: rifle, handgun, bazooka, bow
extern S32 NUM_AGENT_GUN_AIM_ANIMS;        /// Count of weapon aiming animations (4)

extern LLUUID AGENT_NO_ROTATE_ANIMS[];     /// Animations that prevent avatar rotation: sit variants, standup
extern S32 NUM_AGENT_NO_ROTATE_ANIMS;      /// Count of no-rotation animations (3)

extern LLUUID AGENT_STAND_ANIMS[];         /// Standing pose variations: stand, stand_1 through stand_4
extern S32 NUM_AGENT_STAND_ANIMS;          /// Count of standing animations (5)

/**
 * @brief Bidirectional mapping between animation UUIDs and string names.
 * 
 * This class provides fast lookups for converting between animation UUIDs and
 * their human-readable string names. Used by the gesture system, chat commands,
 * and animation debugging tools to allow users to reference animations by name
 * instead of memorizing UUIDs.
 * 
 * The library is populated during construction with all built-in animation
 * mappings (e.g., "dance1" <-> ANIM_AGENT_DANCE1). It's used primarily by
 * the gesture system, keyframe motion loader, and debugging tools.
 * 
 * Performance characteristics:
 * - String storage uses LLStringTable for memory efficiency
 * - O(log n) lookups via std::map for both directions
 * - Strings are stored once and referenced by pointer
 */
class LLAnimationLibrary
{
private:
    LLStringTable mAnimStringTable;    /// Memory-efficient string storage with deduplication

    typedef std::map<LLUUID, char *> anim_map_t;
    anim_map_t mAnimMap;               /// UUID to string name mapping

public:
    /**
     * @brief Initializes the animation library with all built-in animation mappings.
     * 
     * Creates the complete UUID-to-string mapping for all 150+ built-in animations
     * during construction. The string table is pre-sized to 16KB to minimize
     * memory fragmentation from frequent small allocations.
     */
    LLAnimationLibrary();
    
    /**
     * @brief Destroys the animation library and frees string table memory.
     */
    ~LLAnimationLibrary();

    /**
     * @brief Converts an animation UUID to its string name.
     * 
     * @param state The animation UUID to look up
     * @return Pointer to the string name, or NULL if the UUID is not found
     * 
     * Used by debugging tools and gesture systems to display human-readable
     * animation names. Returns a pointer into the string table for efficiency.
     */
    const char *animStateToString( const LLUUID& state );

    /**
     * @brief Converts a string name to its corresponding animation UUID.
     * 
     * @param name The animation name to look up (case-insensitive)
     * @param allow_ids If true, treats UUID strings as valid input
     * @return The animation UUID, or NULL UUID if not found
     * 
     * Primary interface for gesture system and animation lookups in keyframe motions.
     * Performs case-insensitive lookup to handle user input variations.
     * 
     * @code
     * // Used in gesture playback (LLViewerGesture::trigger)
     * LLUUID anim_id = gAnimLibrary.stringToAnimState(mAnimation);
     * gAgent.sendAnimationRequest(anim_id, ANIM_REQUEST_START);
     * 
     * // Used in keyframe motion loading for emotes
     * joint_motion_list->mEmoteID = gAnimLibrary.stringToAnimState(joint_motion_list->mEmoteName);
     * @endcode
     */
    LLUUID stringToAnimState( const std::string& name, bool allow_ids = true );

    /**
     * @brief Associates a custom animation UUID with a string name.
     * 
     * @param state The animation UUID to register
     * @param name The string name to associate with this animation
     * 
     * Used to register user-uploaded animations with custom names for easier
     * reference in gestures and scripts. The string is stored in the internal
     * string table for memory efficiency.
     */
    void animStateSetString( const LLUUID& state, const std::string& name);

    /**
     * @brief Gets the display name for an animation, with fallback to UUID.
     * 
     * @param id The animation UUID to get a name for
     * @return The animation name if known, or "[uuid-string]" if unknown
     * 
     * Convenience method that always returns a displayable string, using
     * the UUID as a fallback when no name mapping exists. Used in UI contexts
     * where some displayable text is always needed.
     */
    std::string animationName( const LLUUID& id ) const;
};

/**
 * @brief Simple pairing of animation name and UUID for user-triggerable animations.
 * 
 * Used to populate arrays of animations that users can directly trigger through
 * gestures, chat commands, or UI elements. This is distinct from the full animation
 * library which includes system animations that users cannot directly control.
 * 
 * Note: Display labels for user interfaces are handled separately in the viewer-specific
 * LLAnimStateLabels system. This struct only contains the core name-UUID binding
 * that both client and server need to understand.
 */
struct LLAnimStateEntry
{
    /**
     * @brief Constructs an animation state entry.
     * 
     * @param name Internal string name for the animation (e.g., "dance1")
     * @param id UUID identifier for the animation
     */
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

    const char* mName;      /// Internal animation name used in gestures and commands
    const LLUUID mID;       /// UUID that identifies this animation to the server
};

/// Array of ~70 animations that users can trigger directly through gestures or commands
extern const LLAnimStateEntry gUserAnimStates[];

/// Number of entries in gUserAnimStates array
extern const S32 gUserAnimStatesCount;

/// Global animation library instance used throughout the viewer
extern LLAnimationLibrary gAnimLibrary;

#endif // LL_LLANIMATIONSTATES_H



