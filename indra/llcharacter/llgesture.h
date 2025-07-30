/**
 * @file llgesture.h
 * @brief A gesture is a combination of a triggering chat phrase or
 * key, a sound, an animation, and a chat string.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLGESTURE_H
#define LL_LLGESTURE_H

#include "llanimationstates.h"
#include "lluuid.h"
#include "llstring.h"

/**
 * @brief Represents a single user-defined gesture combining trigger, animation, sound, and chat.
 * 
 * LLGesture encapsulates a complete gesture definition that can be triggered by:
 * - Keyboard shortcuts (key + modifier combinations)
 * - Chat text triggers (typing specific phrases)
 * 
 * When triggered, a gesture can perform multiple actions:
 * - Play a sound effect (from user's inventory)
 * - Start an avatar animation (by name)
 * - Send a chat message to nearby users
 * 
 * This class handles both the definition of gesture behavior and the triggering
 * logic to determine when gestures should activate. Gestures are commonly used
 * for social interactions, roleplay, and expressing emotions in Second Life.
 * 
 * @code
 * // Create a wave gesture triggered by F1 key
 * LLGesture wave_gesture(KEY_F1, MASK_NONE, "wave", 
 *                        wave_sound_id, "wave", "Hello there!");
 * 
 * // Check if gesture should trigger
 * if (wave_gesture.trigger(KEY_F1, MASK_NONE)) {
 *     // Gesture activated - play sound, animation, and chat
 * }
 * @endcode
 */
class LLGesture
{
public:
    /**
     * @brief Default constructor creates an empty gesture.
     */
    LLGesture();
    
    /**
     * @brief Constructor for a complete gesture definition.
     * 
     * @param key Keyboard key that triggers this gesture (e.g., KEY_F1)
     * @param mask Modifier mask (MASK_NONE, MASK_SHIFT, etc.)
     * @param trigger Text string that triggers gesture when typed in chat
     * @param sound_item_id UUID of inventory sound to play, LLUUID::null for none
     * @param animation Name of animation to play (canonical animation name)
     * @param output_string Chat message to send when gesture triggers
     */
    LLGesture(KEY key, MASK mask, const std::string &trigger,
        const LLUUID &sound_item_id, const std::string &animation,
        const std::string &output_string);

    /**
     * @brief Constructor that deserializes gesture from binary data.
     * 
     * @param buffer Pointer to binary data buffer (advances during read)
     * @param max_size Maximum bytes available in buffer
     */
    LLGesture(U8 **buffer, S32 max_size);
    
    /**
     * @brief Copy constructor.
     * 
     * @param gesture Gesture to copy
     */
    LLGesture(const LLGesture &gesture);
    
    /**
     * @brief Assignment operator.
     * 
     * @param rhs Gesture to copy from
     * @return Reference to this gesture
     */
    const LLGesture &operator=(const LLGesture &rhs);

    /**
     * @brief Virtual destructor for inheritance support.
     */
    virtual ~LLGesture() {};

    /**
     * @brief Gets the keyboard key that triggers this gesture.
     * 
     * @return Keyboard key code (KEY_F1, KEY_A, etc.)
     */
    KEY                 getKey() const          { return mKey; }
    
    /**
     * @brief Gets the modifier mask for the keyboard trigger.
     * 
     * @return Modifier combination (MASK_NONE, MASK_SHIFT, etc.)
     */
    MASK                getMask() const         { return mMask; }
    
    /**
     * @brief Gets the chat text trigger string.
     * 
     * @return Text that triggers gesture when typed in chat
     */
    const std::string&  getTrigger() const      { return mTrigger; }
    
    /**
     * @brief Gets the inventory sound UUID to play.
     * 
     * @return Sound item UUID, or LLUUID::null if no sound
     */
    const LLUUID&       getSound() const        { return mSoundItemID; }
    
    /**
     * @brief Gets the animation name to play.
     * 
     * @return Canonical animation name or face animation
     */
    const std::string&  getAnimation() const    { return mAnimation; }
    
    /**
     * @brief Gets the chat message to send when triggered.
     * 
     * @return Output string for chat
     */
    const std::string&  getOutputString() const { return mOutputString; }

    /**
     * @brief Checks if this gesture should trigger based on keyboard input.
     * 
     * @param key Pressed key code
     * @param mask Active modifier mask
     * @return true if key/mask combination matches this gesture
     */
    virtual bool trigger(KEY key, MASK mask);

    /**
     * @brief Checks if this gesture should trigger based on chat text.
     * 
     * Performs case-insensitive substring matching against the gesture's
     * trigger string. The input string is assumed to already be lowercase.
     * 
     * @param string Lowercase chat text to check for trigger matches
     * @return true if trigger string is found in the input
     */
    virtual bool trigger(const std::string &string);

    /**
     * @brief Serializes gesture to binary format for network transmission.
     * 
     * Note: This uses non-endian-neutral serialization for compatibility
     * with existing protocols.
     * 
     * @param buffer Output buffer to write serialized data
     * @return Pointer to next byte after written data
     */
    U8 *serialize(U8 *buffer) const;
    
    /**
     * @brief Deserializes gesture from binary format.
     * 
     * @param buffer Input buffer containing serialized gesture data
     * @param max_size Maximum bytes available in buffer
     * @return Pointer to next byte after read data
     */
    U8 *deserialize(U8 *buffer, S32 max_size);
    
    /**
     * @brief Gets the maximum size needed for gesture serialization.
     * 
     * @return Maximum bytes required for any gesture serialization
     */
    static S32 getMaxSerialSize();

protected:
    /// Keyboard key that triggers this gesture (typically function keys)
    KEY             mKey;
    /// Modifier mask for key trigger (usually MASK_NONE or MASK_SHIFT)
    MASK            mMask;
    /// Chat text trigger string (no whitespace allowed)
    std::string     mTrigger;
    /// Lowercase version of trigger for case-insensitive matching
    std::string     mTriggerLower;
    /// Inventory UUID of sound to play, LLUUID::null if no sound
    LLUUID          mSoundItemID;
    /// Animation name to play (canonical name or face animation)
    std::string     mAnimation;
    /// Chat message to send when gesture triggers
    std::string     mOutputString;

    /// Maximum bytes required for gesture serialization
    static const S32    MAX_SERIAL_SIZE;
};

/**
 * @brief Container managing a collection of user gestures.
 * 
 * LLGestureList provides a centralized system for managing multiple gestures,
 * handling trigger detection, and coordinating gesture activation. This class
 * serves as the primary interface between user input and gesture execution.
 * 
 * Key responsibilities:
 * - Storing and organizing gesture definitions
 * - Scanning for keyboard and chat trigger matches
 * - Processing trigger events and activating gestures
 * - Serialization/deserialization for persistent storage
 * - Chat text replacement when gestures modify outgoing messages
 * 
 * The list supports both immediate triggers (keyboard shortcuts) and text
 * replacement triggers (chat-based activation with message modification).
 * 
 * @code
 * // Typical usage in gesture system
 * LLGestureList user_gestures;
 * 
 * // Check for keyboard triggers
 * if (user_gestures.trigger(pressed_key, modifiers)) {
 *     // Gesture activated
 * }
 * 
 * // Check chat text and potentially modify it
 * std::string modified_text;
 * if (user_gestures.triggerAndReviseString(chat_input, &modified_text)) {
 *     // Send modified_text instead of original chat_input
 * }
 * @endcode
 */
class LLGestureList
{
public:
    /**
     * @brief Default constructor creates an empty gesture list.
     */
    LLGestureList();
    
    /**
     * @brief Virtual destructor cleans up all stored gestures.
     */
    virtual ~LLGestureList();

    /**
     * @brief Checks all gestures for keyboard trigger matches.
     * 
     * Scans through all gestures in the list to find one matching
     * the specified key/mask combination. If found, the gesture
     * is triggered and its associated actions are performed.
     * 
     * @param key Pressed keyboard key
     * @param mask Active modifier combination
     * @return true if a matching gesture was found and triggered
     */
    bool trigger(KEY key, MASK mask);

    /**
     * @brief Checks for chat triggers and modifies text if needed.
     * 
     * Scans all gestures for chat text triggers that match substrings
     * in the input text. If matches are found, the gestures are triggered
     * and the output text may be modified to include gesture output strings.
     * 
     * This enables gestures that both trigger actions (animations, sounds)
     * and modify the chat message being sent.
     * 
     * @param string Original chat text to scan for triggers
     * @param revised_string [out] Modified text after gesture processing
     * @return true if any gestures were triggered and text was modified
     */
    bool triggerAndReviseString(const std::string &string, std::string* revised_string);

    /**
     * @brief Gets the number of gestures in the list.
     * 
     * @return Count of stored gestures
     */
    S32 count() const                       { return static_cast<S32>(mList.size()); }
    
    /**
     * @brief Gets a gesture by index.
     * 
     * @param i Index of gesture to retrieve
     * @return Pointer to gesture at specified index
     */
    virtual LLGesture* get(S32 i) const     { return mList.at(i); }
    
    /**
     * @brief Adds a gesture to the list.
     * 
     * The list takes ownership of the gesture pointer and will
     * delete it when the list is destroyed or cleared.
     * 
     * @param gesture Gesture to add to the list
     */
    virtual void put(LLGesture* gesture)    { mList.push_back( gesture ); }
    
    /**
     * @brief Removes and deletes all gestures from the list.
     */
    void deleteAll();

    /**
     * @brief Serializes entire gesture list to binary format.
     * 
     * @param buffer Output buffer for serialized data
     * @return Pointer to next byte after written data
     */
    U8 *serialize(U8 *buffer) const;
    
    /**
     * @brief Deserializes gesture list from binary format.
     * 
     * @param buffer Input buffer containing serialized list data
     * @param max_size Maximum bytes available in buffer
     * @return Pointer to next byte after read data
     */
    U8 *deserialize(U8 *buffer, S32 max_size);
    
    /**
     * @brief Gets the maximum size needed for list serialization.
     * 
     * @return Maximum bytes required for serializing this gesture list
     */
    S32 getMaxSerialSize();

protected:
    /**
     * @brief Factory method for creating gesture objects during deserialization.
     * 
     * This virtual method allows derived classes to override gesture creation
     * to use specialized gesture implementations.
     * 
     * @param buffer Binary data buffer containing gesture data
     * @param max_size Maximum bytes available in buffer
     * @return New gesture object created from buffer data
     */
    virtual LLGesture *create_gesture(U8 **buffer, S32 max_size);

    /// Vector containing all gestures managed by this list
    std::vector<LLGesture*> mList;

    /// Size of serialization header in bytes
    static const S32    SERIAL_HEADER_SIZE;
};

#endif
