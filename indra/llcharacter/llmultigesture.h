/**
 * @file llmultigesture.h
 * @brief Gestures that are asset-based and can have multiple steps.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLMULTIGESTURE_H
#define LL_LLMULTIGESTURE_H

#include <set>
#include <string>
#include <vector>

#include "lluuid.h"
#include "llframetimer.h"

class LLDataPacker;
class LLGestureStep;

/**
 * @brief Multi-step gesture sequence that combines animations, sounds, and chat.
 * 
 * LLMultiGesture represents a complex gesture that can contain multiple sequential
 * steps including animations, sound effects, chat messages, and timing delays.
 * These gestures are triggered by text chat commands or keyboard shortcuts and
 * provide rich interactive content for avatar communication.
 * 
 * The gesture system supports:
 * - Sequential execution of multiple gesture steps
 * - Text trigger matching in chat (e.g., "/wave", "hello")
 * - Keyboard shortcut activation (key + modifier combinations)
 * - Text replacement in chat when triggered
 * - Complex timing and synchronization with wait steps
 * - Asset loading and playback coordination
 * 
 * Real-world Usage:
 * Gestures are managed by LLGestureMgr and stored in the user's inventory.
 * When activated, they're moved to the mPlaying list and executed step by step:
 * 
 * @code
 * // Typical gesture creation in gesture preview system
 * LLMultiGesture* gesture = new LLMultiGesture();
 * gesture->mTrigger = "/wave";
 * gesture->mReplaceText = "waves hello";
 * 
 * // Add animation step
 * LLGestureStepAnimation* anim_step = new LLGestureStepAnimation();
 * anim_step->mAnimAssetID = ANIM_AGENT_HELLO;
 * gesture->mSteps.push_back(anim_step);
 * 
 * // Add chat step
 * LLGestureStepChat* chat_step = new LLGestureStepChat();
 * chat_step->mChatText = "Hello there!";
 * gesture->mSteps.push_back(chat_step);
 * 
 * // Gesture execution is handled by LLGestureMgr::playGesture()
 * gesture->mPlaying = true;
 * mPlaying.push_back(gesture);
 * @endcode
 * 
 * Performance Considerations:
 * - Assets (animations, sounds) are preloaded when gesture starts
 * - Gesture state is tracked through mCurrentStep counter
 * - Complex timing requires frame-based updates during playback
 * - Memory management handled by LLGestureMgr ownership
 * 
 * The gesture system integrates with the broader animation system by triggering
 * motion playback and coordinating with avatar animation states.
 */
class LLMultiGesture
{
public:
    /**
     * @brief Constructs a new empty gesture with default values.
     * 
     * Initializes all state variables to safe defaults. The gesture is not
     * playable until it has been populated with steps and properly configured.
     */
    LLMultiGesture();
    
    /**
     * @brief Destroys the gesture and cleans up all associated steps.
     * 
     * Automatically destroys all LLGestureStep instances in mSteps vector
     * and cleans up any allocated resources. The gesture should not be
     * playing when destroyed.
     */
    virtual ~LLMultiGesture();

    /**
     * @brief Calculates the maximum serialization size in bytes.
     * 
     * Computes the worst-case size needed to serialize this gesture to a
     * data stream. Used for buffer allocation before serialization to ensure
     * sufficient space is available.
     * 
     * @return Maximum number of bytes needed for serialization
     */
    S32 getMaxSerialSize() const;

    /**
     * @brief Serializes the gesture to a data packer for storage or transmission.
     * 
     * Writes all gesture data (trigger, steps, configuration) to the specified
     * data packer in a format suitable for file storage or network transmission.
     * Used when saving gestures to inventory or uploading to servers.
     * 
     * @param dp Data packer to write gesture data to
     * @return true if serialization succeeded, false on error
     */
    bool serialize(LLDataPacker& dp) const;
    
    /**
     * @brief Deserializes gesture data from a data packer.
     * 
     * Reads gesture configuration and steps from the data packer, reconstructing
     * the complete gesture. Used when loading gestures from inventory assets
     * or receiving them from servers.
     * 
     * @param dp Data packer to read gesture data from
     * @return true if deserialization succeeded, false on error or corruption
     */
    bool deserialize(LLDataPacker& dp);

    /**
     * @brief Outputs debug information about the gesture to the log.
     * 
     * Prints detailed information about the gesture's configuration, steps,
     * and current state for debugging purposes. Includes trigger text, step
     * count, and execution status.
     */
    void dump();

    /**
     * @brief Resets the gesture to its initial unplayed state.
     * 
     * Clears all runtime state including current step position, timing flags,
     * and animation tracking. Prepares the gesture for a fresh playback cycle
     * without destroying the gesture steps themselves.
     */
    void reset();

    /**
     * @brief Gets the text trigger that activates this gesture.
     * 
     * Returns the chat text or command that causes this gesture to play when
     * typed by the user. Common patterns include "/wave", "hello", etc.
     * 
     * @return Reference to the trigger string
     */
    const std::string& getTrigger() const { return mTrigger; }
protected:
    /**
     * @brief Copy constructor marked protected to prevent unintended copying.
     * 
     * Gestures contain complex step hierarchies and runtime state that make
     * copying semantically unclear. Use explicit gesture creation and step
     * copying when gesture duplication is needed.
     * 
     * @param gest Source gesture to copy from
     */
    LLMultiGesture(const LLMultiGesture& gest);
    
    /**
     * @brief Assignment operator marked protected to prevent unintended copying.
     * 
     * Similar to copy constructor, assignment has unclear semantics for gestures
     * with complex internal state and step ownership. Use explicit methods
     * for gesture manipulation instead.
     * 
     * @param rhs Source gesture to assign from
     * @return Reference to this gesture
     */
    const LLMultiGesture& operator=(const LLMultiGesture& rhs);

public:
    /// Keyboard key code for shortcut activation (0 = no key shortcut)
    KEY mKey { 0 };
    
    /// Modifier mask for key shortcut (CTRL, ALT, SHIFT combinations)
    MASK mMask { 0 };

    /**
     * @brief Display name of the gesture for UI presentation.
     * 
     * This name can be empty if the inventory item is not available and
     * the gesture manager has not yet set the name from inventory data.
     * Used in gesture floaters and selection lists.
     */
    std::string mName;

    /**
     * @brief Text trigger that activates the gesture in chat.
     * 
     * String patterns like "/foo" or "hello" that cause the gesture to play
     * when typed in chat. Case-sensitive matching against user input.
     * Empty string means no chat trigger (keyboard-only activation).
     */
    std::string mTrigger;

    /**
     * @brief Text that replaces the trigger in chat when gesture plays.
     * 
     * When the gesture is triggered by chat text, the trigger substring
     * is replaced with this text in the outgoing message. Allows gestures
     * to modify chat content while playing animations.
     */
    std::string mReplaceText;

    /**
     * @brief Ordered sequence of gesture steps to execute.
     * 
     * Vector of LLGestureStep pointers that define the gesture's behavior.
     * Steps are executed sequentially using mCurrentStep as index.
     * Memory ownership: this gesture owns all step pointers.
     */
    std::vector<LLGestureStep*> mSteps;

    /**
     * @brief Runtime flag indicating if the gesture is currently executing.
     * 
     * Set to true when gesture playback begins, false when complete or stopped.
     * Used by LLGestureMgr to track active gestures and update UI display.
     * Critical for preventing multiple simultaneous plays of same gesture.
     */
    bool mPlaying { false };

    /**
     * @brief Current execution position in the mSteps vector.
     * 
     * "Instruction pointer" for step-by-step execution. Increments as each
     * step completes, resets to 0 when gesture finishes or is reset.
     * Range: 0 to mSteps.size()-1 during normal execution.
     */
    S32 mCurrentStep { 0 };

    /**
     * @brief Flag indicating gesture is waiting for triggered animations to complete.
     * 
     * Set when a wait step requires all active animations to finish before
     * proceeding. Used with WAIT_FLAG_ALL_ANIM to synchronize gesture timing
     * with animation completion.
     */
    bool mWaitingAnimations { false };

    /**
     * @brief Flag indicating gesture is waiting for keyboard key release.
     * 
     * Set when gesture was triggered by key press and wait step requires
     * key release before continuing. Used with WAIT_FLAG_KEY_RELEASE for
     * hold-to-continue gesture behavior.
     */
    bool mWaitingKeyRelease { false };

    /**
     * @brief Flag indicating gesture is waiting for a fixed time duration.
     * 
     * Set during timed wait steps that pause execution for a specific duration.
     * Used with WAIT_FLAG_TIME and mWaitTimer for precise timing control.
     */
    bool mWaitingTimer { false };

    /**
     * @brief Flag indicating this gesture was activated by keyboard shortcut.
     * 
     * Distinguishes between chat-triggered and key-triggered activations for
     * different handling logic. Affects wait step behavior and gesture lifecycle.
     */
    bool mTriggeredByKey { false };

    /**
     * @brief Flag tracking if the trigger key has been released.
     * 
     * Used in conjunction with mWaitingKeyRelease to detect key release events
     * for gestures that require hold-and-release interaction patterns.
     */
    bool mKeyReleased { false };

    /**
     * @brief Flag indicating gesture is in final cleanup phase.
     * 
     * Set after the last step has been executed but gesture is waiting for
     * all triggered animations to complete before marking as finished.
     * Ensures smooth gesture conclusion.
     */
    bool mWaitingAtEnd { false };

    /**
     * @brief High-precision timer for wait step duration tracking.
     * 
     * Used during timed wait steps to measure elapsed time against target
     * duration. Provides frame-rate independent timing for gesture pacing.
     */
    LLFrameTimer mWaitTimer;

    /**
     * @brief Optional completion callback function pointer.
     * 
     * Called when gesture completes execution. Allows external systems to
     * receive notification of gesture completion for cleanup or chaining.
     * Set to NULL if no callback is needed.
     */
    void (*mDoneCallback)(LLMultiGesture* gesture, void* data) { NULL };
    
    /**
     * @brief User data pointer passed to completion callback.
     * 
     * Arbitrary data pointer passed to mDoneCallback when gesture completes.
     * Allows callback functions to access context-specific information.
     */
    void* mCallbackData { NULL };

    /**
     * @brief Set of animation UUIDs that have been requested to start.
     * 
     * Tracks animations that gesture steps have requested but which may not
     * yet be playing. IDs move to mPlayingAnimIDs once server confirms start.
     * Used for request/response tracking with animation system.
     */
    std::set<LLUUID> mRequestedAnimIDs;

    /**
     * @brief Set of animation UUIDs that are currently confirmed playing.
     * 
     * Contains animations that the server has confirmed are playing for this
     * gesture. Used for wait-for-animation logic and proper gesture cleanup.
     * IDs are moved here from mRequestedAnimIDs upon server confirmation.
     */
    std::set<LLUUID> mPlayingAnimIDs;
};


/**
 * @brief Enumeration of gesture step types.
 * 
 * Defines the different types of actions that can be performed as part of
 * a multi-step gesture sequence. Order must match the library_list in
 * floater_preview_gesture.xml for UI consistency.
 * 
 * Each step type corresponds to a specific LLGestureStep subclass:
 * - STEP_ANIMATION -> LLGestureStepAnimation
 * - STEP_SOUND -> LLGestureStepSound  
 * - STEP_CHAT -> LLGestureStepChat
 * - STEP_WAIT -> LLGestureStepWait
 */
enum EStepType
{
    STEP_ANIMATION = 0,  ///< Play an animation on the avatar
    STEP_SOUND = 1,      ///< Play a sound effect  
    STEP_CHAT = 2,       ///< Send a chat message
    STEP_WAIT = 3,       ///< Wait for time, animations, or key release

    STEP_EOF = 4         ///< End-of-file marker for serialization
};


/**
 * @brief Abstract base class for individual gesture step actions.
 * 
 * LLGestureStep defines the interface for all gesture step types that can
 * be executed as part of a multi-gesture sequence. Each concrete step type
 * (animation, sound, chat, wait) inherits from this base class.
 * 
 * The step system uses polymorphism to enable uniform handling of different
 * step types during gesture execution while preserving type-specific behavior.
 * Steps are owned by their containing LLMultiGesture and executed sequentially.
 * 
 * Implementation Pattern:
 * Each step type implements the virtual interface methods to provide:
 * - Type identification for runtime dispatching
 * - UI label generation for gesture editing
 * - Serialization for asset storage and network transmission
 * - Debug output for development and troubleshooting
 */
class LLGestureStep
{
public:
    /**
     * @brief Default constructor for gesture steps.
     * 
     * Initializes the step with default values. Concrete subclasses should
     * override this to set appropriate defaults for their step type.
     */
    LLGestureStep() {}
    
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     * 
     * Enables safe deletion of step instances through base class pointers.
     * Critical for proper memory management in gesture step vectors.
     */
    virtual ~LLGestureStep() {}

    /**
     * @brief Returns the specific type of this gesture step.
     * 
     * Pure virtual method that must be implemented by each concrete step type
     * to return its corresponding EStepType value. Used for runtime type
     * identification and step-specific processing.
     * 
     * @return EStepType enum value for this step type
     */
    virtual EStepType getType() = 0;

    /**
     * @brief Returns a user-readable description of this step for UI display.
     * 
     * Generates human-readable text describing what this step does, used in
     * gesture editing interfaces and debug output. Format varies by step type
     * but typically includes key parameters and action description.
     * 
     * @return Vector of strings describing the step (allows multi-line labels)
     */
    virtual std::vector<std::string> getLabel() const = 0;

    /**
     * @brief Calculates maximum serialization size for this step.
     * 
     * Returns the worst-case number of bytes needed to serialize this step
     * to a data stream. Used for buffer allocation before serialization.
     * 
     * @return Maximum serialization size in bytes
     */
    virtual S32 getMaxSerialSize() const = 0;
    
    /**
     * @brief Serializes this step to a data packer.
     * 
     * Writes step-specific data to the data packer in a format suitable for
     * storage or transmission. Each step type has its own serialization format.
     * 
     * @param dp Data packer to write step data to
     * @return true if serialization succeeded, false on error
     */
    virtual bool serialize(LLDataPacker& dp) const = 0;
    
    /**
     * @brief Deserializes step data from a data packer.
     * 
     * Reads step-specific data from the data packer, restoring the step's
     * configuration. Must match the format used by serialize().
     * 
     * @param dp Data packer to read step data from
     * @return true if deserialization succeeded, false on error
     */
    virtual bool deserialize(LLDataPacker& dp) = 0;

    /**
     * @brief Outputs debug information about this step to the log.
     * 
     * Pure virtual method for generating debug output specific to each step
     * type. Used for troubleshooting gesture execution and development.
     */
    virtual void dump() = 0;
};


/**
 * @brief Flag controlling animation step behavior.
 * 
 * By default, animation steps start animations. If the least significant
 * bit is set (ANIM_FLAG_STOP), the step will stop the specified animation
 * instead of starting it. This allows gestures to precisely control when
 * animations begin and end during complex sequences.
 */
const U32 ANIM_FLAG_STOP = 0x01;

/**
 * @brief Gesture step that starts or stops avatar animations.
 * 
 * LLGestureStepAnimation handles animation playback as part of gesture sequences.
 * It can either start animations (default behavior) or stop them based on the
 * ANIM_FLAG_STOP flag. This enables precise animation control during complex
 * gesture choreography.
 * 
 * The step coordinates with the avatar's motion controller to ensure animations
 * play correctly and can be synchronized with other gesture elements like sounds
 * and chat messages.
 * 
 * Real-world Usage:
 * Animation steps are commonly used in social gestures like waving, dancing,
 * or emoting. They're created in the gesture preview editor and stored as part
 * of the gesture asset.
 * 
 * @code
 * // Typical animation step creation
 * LLGestureStepAnimation* anim_step = new LLGestureStepAnimation();
 * anim_step->mAnimAssetID = ANIM_AGENT_HELLO;
 * anim_step->mAnimName = "Hello Animation";
 * anim_step->mFlags = 0;  // Start animation (default)
 * gesture->mSteps.push_back(anim_step);
 * @endcode
 */
class LLGestureStepAnimation : public LLGestureStep
{
public:
    /**
     * @brief Constructs a new animation step with default values.
     * 
     * Initializes the step to start an animation (flags = 0) with empty
     * name and null asset ID. These must be set before the step is usable.
     */
    LLGestureStepAnimation();
    
    /**
     * @brief Destroys the animation step and cleans up resources.
     * 
     * Performs any necessary cleanup for the animation step. The step should
     * not be actively playing when destroyed.
     */
    virtual ~LLGestureStepAnimation();

    /**
     * @brief Returns STEP_ANIMATION to identify this as an animation step.
     * 
     * @return STEP_ANIMATION type identifier
     */
    virtual EStepType getType() { return STEP_ANIMATION; }

    /**
     * @brief Generates user-readable label describing the animation action.
     * 
     * Creates a descriptive label for UI display, typically showing the
     * animation name and whether it starts or stops the animation.
     * 
     * @return Vector of strings describing the animation step
     */
    virtual std::vector<std::string> getLabel() const;

    /**
     * @brief Calculates maximum serialization size for this animation step.
     * 
     * @return Maximum bytes needed for serialization
     */
    virtual S32 getMaxSerialSize() const;
    
    /**
     * @brief Serializes animation step data to data packer.
     * 
     * @param dp Data packer to write to
     * @return true if serialization succeeded
     */
    virtual bool serialize(LLDataPacker& dp) const;
    
    /**
     * @brief Deserializes animation step data from data packer.
     * 
     * @param dp Data packer to read from
     * @return true if deserialization succeeded
     */
    virtual bool deserialize(LLDataPacker& dp);

    /**
     * @brief Outputs debug information about this animation step.
     */
    virtual void dump();

public:
    /// Human-readable name of the animation for UI display
    std::string mAnimName;
    
    /// UUID of the animation asset to play or stop
    LLUUID mAnimAssetID;
    
    /// Flags controlling animation behavior (ANIM_FLAG_STOP, etc.)
    U32 mFlags;
};


/**
 * @brief Gesture step that plays sound effects.
 * 
 * LLGestureStepSound handles audio playback as part of gesture sequences.
 * It triggers sound effects that accompany gesture animations and chat,
 * providing audio feedback and enhancing the gesture experience.
 * 
 * Sound steps integrate with the audio system to play sounds at appropriate
 * volume levels and can be synchronized with other gesture elements for
 * coordinated audio-visual effects.
 * 
 * Common Usage:
 * Sound steps add audio cues to gestures like applause, laughter, or
 * environmental sounds. They're particularly effective when combined with
 * matching animations and chat text.
 */
class LLGestureStepSound : public LLGestureStep
{
public:
    /**
     * @brief Constructs a new sound step with default values.
     * 
     * Initializes the step with empty sound name, null asset ID, and default
     * flags. Sound asset information must be set before step execution.
     */
    LLGestureStepSound();
    
    /**
     * @brief Destroys the sound step and cleans up resources.
     * 
     * Ensures proper cleanup of sound step resources. The sound should not
     * be actively playing when the step is destroyed.
     */
    virtual ~LLGestureStepSound();

    /**
     * @brief Returns STEP_SOUND to identify this as a sound step.
     * 
     * @return STEP_SOUND type identifier
     */
    virtual EStepType getType() { return STEP_SOUND; }

    /**
     * @brief Generates user-readable label describing the sound action.
     * 
     * Creates a descriptive label for UI display, typically showing the
     * sound name and any relevant playback information.
     * 
     * @return Vector of strings describing the sound step
     */
    virtual std::vector<std::string> getLabel() const;

    /**
     * @brief Calculates maximum serialization size for this sound step.
     * 
     * @return Maximum bytes needed for serialization
     */
    virtual S32 getMaxSerialSize() const;
    
    /**
     * @brief Serializes sound step data to data packer.
     * 
     * @param dp Data packer to write to
     * @return true if serialization succeeded
     */
    virtual bool serialize(LLDataPacker& dp) const;
    
    /**
     * @brief Deserializes sound step data from data packer.
     * 
     * @param dp Data packer to read from
     * @return true if deserialization succeeded
     */
    virtual bool deserialize(LLDataPacker& dp);

    /**
     * @brief Outputs debug information about this sound step.
     */
    virtual void dump();

public:
    /// Human-readable name of the sound for UI display
    std::string mSoundName;
    
    /// UUID of the sound asset to play
    LLUUID mSoundAssetID;
    
    /// Flags controlling sound playback behavior
    U32 mFlags;
};


/**
 * @brief Gesture step that sends chat messages.
 * 
 * LLGestureStepChat handles text output as part of gesture sequences.
 * It sends chat messages to the local or public chat channels, allowing
 * gestures to provide narrative context or dialogue alongside animations.
 * 
 * Chat steps are processed through the normal chat system, respecting
 * chat range limits, channel settings, and other communication rules.
 * They can be synchronized with animations and sounds for rich storytelling.
 * 
 * Typical Usage:
 * Chat steps provide spoken dialogue for gesture sequences, such as
 * "Hello there!" accompanying a wave animation, or narrative text
 * describing the avatar's actions.
 */
class LLGestureStepChat : public LLGestureStep
{
public:
    /**
     * @brief Constructs a new chat step with default values.
     * 
     * Initializes the step with empty chat text and default flags. The
     * chat text must be set before the step can be executed.
     */
    LLGestureStepChat();
    
    /**
     * @brief Destroys the chat step and cleans up resources.
     * 
     * Performs cleanup for the chat step. Chat messages are fire-and-forget
     * so no special cleanup is typically required.
     */
    virtual ~LLGestureStepChat();

    /**
     * @brief Returns STEP_CHAT to identify this as a chat step.
     * 
     * @return STEP_CHAT type identifier
     */
    virtual EStepType getType() { return STEP_CHAT; }

    /**
     * @brief Generates user-readable label showing the chat message.
     * 
     * Creates a descriptive label for UI display, typically showing the
     * chat text that will be sent when the step executes.
     * 
     * @return Vector of strings describing the chat step
     */
    virtual std::vector<std::string> getLabel() const;

    /**
     * @brief Calculates maximum serialization size for this chat step.
     * 
     * @return Maximum bytes needed for serialization
     */
    virtual S32 getMaxSerialSize() const;
    
    /**
     * @brief Serializes chat step data to data packer.
     * 
     * @param dp Data packer to write to
     * @return true if serialization succeeded
     */
    virtual bool serialize(LLDataPacker& dp) const;
    
    /**
     * @brief Deserializes chat step data from data packer.
     * 
     * @param dp Data packer to read from
     * @return true if deserialization succeeded
     */
    virtual bool deserialize(LLDataPacker& dp);

    /**
     * @brief Outputs debug information about this chat step.
     */
    virtual void dump();

public:
    /// Text message to send to chat when step executes
    std::string mChatText;
    
    /// Flags controlling chat behavior (channel, whisper/shout, etc.)
    U32 mFlags;
};


/// Flag indicating wait step should pause for a fixed time duration
const U32 WAIT_FLAG_TIME        = 0x01;

/// Flag indicating wait step should pause until all animations complete
const U32 WAIT_FLAG_ALL_ANIM    = 0x02;

/// Flag indicating wait step should pause until trigger key is released
const U32 WAIT_FLAG_KEY_RELEASE = 0x04;

/**
 * @brief Gesture step that pauses execution for timing and synchronization.
 * 
 * LLGestureStepWait provides various waiting mechanisms to control gesture
 * timing and synchronization. It can wait for fixed time durations, animation
 * completion, or user input, enabling precise choreography of complex gestures.
 * 
 * Wait types:
 * - WAIT_FLAG_TIME: Pause for mWaitSeconds duration
 * - WAIT_FLAG_ALL_ANIM: Wait until all gesture animations finish
 * - WAIT_FLAG_KEY_RELEASE: Wait for trigger key release (keyboard gestures)
 * 
 * Multiple flags can be combined for complex wait conditions. This step type
 * is essential for creating realistic gesture timing and ensuring proper
 * synchronization between animations, sounds, and chat.
 * 
 * Common Usage Patterns:
 * - Brief pauses between gesture phases for natural timing
 * - Waiting for animations to complete before showing chat text
 * - Hold-to-continue behaviors with key release detection
 */
class LLGestureStepWait : public LLGestureStep
{
public:
    /**
     * @brief Constructs a new wait step with default values.
     * 
     * Initializes the step with zero wait time and no wait flags. The
     * wait parameters must be configured before the step is usable.
     */
    LLGestureStepWait();
    
    /**
     * @brief Destroys the wait step and cleans up resources.
     * 
     * Performs cleanup for the wait step. Active waits should be cancelled
     * before the step is destroyed.
     */
    virtual ~LLGestureStepWait();

    /**
     * @brief Returns STEP_WAIT to identify this as a wait step.
     * 
     * @return STEP_WAIT type identifier
     */
    virtual EStepType getType() { return STEP_WAIT; }

    /**
     * @brief Generates user-readable label describing the wait condition.
     * 
     * Creates a descriptive label for UI display, showing what the step
     * is waiting for (time, animations, key release) and duration if applicable.
     * 
     * @return Vector of strings describing the wait step
     */
    virtual std::vector<std::string> getLabel() const;

    /**
     * @brief Calculates maximum serialization size for this wait step.
     * 
     * @return Maximum bytes needed for serialization
     */
    virtual S32 getMaxSerialSize() const;
    
    /**
     * @brief Serializes wait step data to data packer.
     * 
     * @param dp Data packer to write to
     * @return true if serialization succeeded
     */
    virtual bool serialize(LLDataPacker& dp) const;
    
    /**
     * @brief Deserializes wait step data from data packer.
     * 
     * @param dp Data packer to read from
     * @return true if deserialization succeeded
     */
    virtual bool deserialize(LLDataPacker& dp);

    /**
     * @brief Outputs debug information about this wait step.
     */
    virtual void dump();

public:
    /// Time duration to wait in seconds (used with WAIT_FLAG_TIME)
    F32 mWaitSeconds;
    
    /// Combination of WAIT_FLAG_* constants defining wait behavior
    U32 mFlags;
};

#endif // LL_LLMULTIGESTURE_H
