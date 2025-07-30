/**
 * @file llstatemachine.h
 * @brief LLStateMachine class header file.
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

#ifndef LL_LLSTATEMACHINE_H
#define LL_LLSTATEMACHINE_H

#include <string>

#include "llerror.h"
#include <map>

/**
 * @brief Base class that provides unique integer identifiers for objects.
 * 
 * LLUniqueID serves as a foundation class for objects that need globally unique
 * identifiers within the application. It uses a simple incrementing counter to
 * assign each instance a unique ID number.
 * 
 * This class is used as a base for state machine components (states and transitions)
 * where unique identification is needed for lookup and comparison operations.
 * 
 * Key Features:
 * - Automatic ID assignment on construction
 * - Global uniqueness across all instances
 * - Comparison operators for container usage
 * - Virtual destructor for safe inheritance
 * 
 * Usage Pattern:
 * Objects inherit from LLUniqueID to gain automatic unique identification:
 * @code
 * class MyState : public LLUniqueID {
 *     // State-specific implementation
 * };
 * 
 * MyState state1, state2;
 * assert(state1.getID() != state2.getID());  // Always unique
 * @endcode
 * 
 * Thread Safety: Not thread-safe. ID assignment should occur on main thread.
 */
class LLUniqueID
{
    friend bool operator==(const LLUniqueID &a, const LLUniqueID &b);
    friend bool operator!=(const LLUniqueID &a, const LLUniqueID &b);
protected:
    /// Static counter for generating unique IDs across all instances
    static U32  sNextID;
    /// The unique identifier assigned to this instance
    U32         mId;
public:
    /**
     * @brief Constructs a new unique ID object with auto-assigned identifier.
     * 
     * Automatically assigns the next available unique ID from the global counter.
     * Each construction is guaranteed to produce a different ID value.
     */
    LLUniqueID(){mId = sNextID++;}
    
    /**
     * @brief Virtual destructor for safe inheritance.
     * 
     * Enables proper cleanup when deleting objects through base class pointers.
     */
    virtual ~LLUniqueID(){}
    
    /**
     * @brief Gets the unique identifier for this object.
     * 
     * @return The unique integer ID assigned during construction
     */
    U32     getID() {return mId;}
};

/**
 * @brief Represents a transition between states in a finite state machine.
 * 
 * LLFSMTransition defines the conditions and identity for moving from one state
 * to another in a state machine. Each transition has a unique ID and can be
 * given a descriptive name for debugging and visualization.
 * 
 * Transitions are the "edges" in the state machine graph, defining valid paths
 * between states. They can be:
 * - Directed (one-way transitions)
 * - Undirected (bidirectional transitions) 
 * - Default transitions (available from any state)
 * 
 * Real-world Usage:
 * In the animation system, transitions might represent events like "animation_complete",
 * "user_input", or "timer_expired" that cause state changes.
 * 
 * @code
 * // Creating a custom transition
 * class AnimCompleteTransition : public LLFSMTransition {
 * public:
 *     virtual std::string getName() const override { 
 *         return "animation_complete"; 
 *     }
 * };
 * @endcode
 */
class LLFSMTransition : public LLUniqueID
{
public:
    /**
     * @brief Constructs a new FSM transition with unique ID.
     * 
     * Creates a transition with automatically assigned unique identifier.
     * The transition is ready to be added to state diagrams.
     */
    LLFSMTransition() : LLUniqueID(){};
    
    /**
     * @brief Gets a descriptive name for this transition.
     * 
     * Returns a human-readable name for debugging and visualization.
     * Subclasses should override this to provide meaningful names.
     * 
     * @return String name for this transition (default: "unnamed")
     */
    virtual std::string getName()const { return "unnamed"; }
};

/**
 * @brief Represents a state in a finite state machine with lifecycle callbacks.
 * 
 * LLFSMState defines a state that can be entered, executed, and exited as part
 * of a state machine's operation. Each state has a unique ID and provides virtual
 * methods for custom behavior during state transitions and execution.
 * 
 * State Lifecycle:
 * 1. onEntry() - Called when transitioning into this state
 * 2. execute() - Called repeatedly while in this state
 * 3. onExit() - Called when transitioning out of this state
 * 
 * This design enables complex state-based behavior with proper initialization,
 * ongoing processing, and cleanup for each state.
 * 
 * Real-world Usage in Animation System:
 * States might represent animation phases like "loading", "playing", "paused",
 * "stopping" with appropriate setup and teardown logic.
 * 
 * @code
 * // Example animation state implementation
 * class PlayingState : public LLFSMState {
 * public:
 *     virtual void onEntry(void* data) override {
 *         // Start animation playback
 *         startAnimation();
 *     }
 *     
 *     virtual void execute(void* data) override {
 *         // Update animation each frame
 *         updateAnimation();
 *     }
 *     
 *     virtual void onExit(void* data) override {
 *         // Clean up when leaving state
 *         cleanupAnimation();
 *     }
 *     
 *     virtual std::string getName() const override {
 *         return "playing";
 *     }
 * };
 * @endcode
 */
class LLFSMState : public LLUniqueID
{
public:
    /**
     * @brief Constructs a new FSM state with unique ID.
     * 
     * Creates a state with automatically assigned unique identifier.
     * The state is ready to be added to state diagrams.
     */
    LLFSMState() : LLUniqueID(){};
    
    /**
     * @brief Called when the state machine transitions into this state.
     * 
     * Override this method to perform initialization logic specific to entering
     * this state. Called exactly once per state entry, before any execute() calls.
     * 
     * @param user_data Optional user data passed during state transition
     */
    virtual void onEntry(void *user_data){};
    
    /**
     * @brief Called when the state machine transitions out of this state.
     * 
     * Override this method to perform cleanup logic when leaving this state.
     * Called exactly once per state exit, after all execute() calls are complete.
     * 
     * @param user_data Optional user data passed during state transition
     */
    virtual void onExit(void *user_data){};
    
    /**
     * @brief Called repeatedly while the state machine is in this state.
     * 
     * Override this method to perform ongoing processing while in this state.
     * Called each time runCurrentState() is invoked on the state machine.
     * 
     * @param user_data Optional user data passed during execution
     */
    virtual void execute(void *user_data){};
    
    /**
     * @brief Gets a descriptive name for this state.
     * 
     * Returns a human-readable name for debugging and visualization.
     * Subclasses should override this to provide meaningful names.
     * 
     * @return String name for this state (default: "unnamed")
     */
    virtual std::string getName() const { return "unnamed"; }
};

/**
 * @brief Defines the structure and rules of a finite state machine.
 * 
 * LLStateDiagram represents the complete topology of a state machine, including
 * all states, transitions between them, and the rules governing state changes.
 * It serves as the "blueprint" that LLStateMachine instances use to determine
 * valid transitions and state relationships.
 * 
 * Key Capabilities:
 * - Directed and undirected transitions between states
 * - Default transitions available from any state  
 * - Default fallback state when no transitions match
 * - Validation of state machine structure
 * - DOT file export for visualization
 * 
 * Architecture:
 * The diagram uses a two-level map structure:
 * - StateMap: Maps each state to its outgoing transitions
 * - Transitions: Maps each transition to its destination state
 * 
 * This enables efficient lookup of "from state X via transition Y, go to state Z".
 * 
 * Real-world Usage:
 * In animation systems, state diagrams might define valid animation sequences:
 * @code
 * // Example: Animation state machine setup
 * LLStateDiagram* diagram = new LLStateDiagram();
 * 
 * // Create states
 * IdleState* idle = new IdleState();
 * WalkingState* walking = new WalkingState();
 * RunningState* running = new RunningState();
 * 
 * // Create transitions
 * StartWalkTransition* start_walk = new StartWalkTransition();
 * StartRunTransition* start_run = new StartRunTransition();
 * StopTransition* stop = new StopTransition();
 * 
 * // Build state machine
 * diagram->addTransition(*idle, *walking, *start_walk);
 * diagram->addTransition(*walking, *running, *start_run);
 * diagram->addDefaultTransition(*idle, *stop);  // Can always stop
 * @endcode
 * 
 * Thread Safety: Not thread-safe. Diagram modification should occur on main thread.
 */
class LLStateDiagram
{
    /// Map type for transitions from a state (transition -> destination state)
    typedef std::map<LLFSMTransition*, LLFSMState*> Transitions;

    friend std::ostream& operator<<(std::ostream &s, LLStateDiagram &FSM);
    friend class LLStateMachine;

protected:
    /// Map type for the complete state graph (state -> its outgoing transitions)
    typedef std::map<LLFSMState*, Transitions> StateMap;
    
    /// Complete state graph mapping states to their outgoing transitions
    StateMap            mStates;
    /// Default transitions available from any state
    Transitions         mDefaultTransitions;
    /// Fallback state used when no specific transitions match
    LLFSMState*         mDefaultState;
    /// Whether to use the default state fallback mechanism
    bool                mUseDefaultState;

public:
    /**
     * @brief Constructs an empty state diagram.
     * 
     * Creates a state diagram with no states or transitions. States and
     * transitions must be added before the diagram can be used.
     */
    LLStateDiagram();
    
    /**
     * @brief Destroys the state diagram and cleans up resources.
     * 
     * Note: Does not destroy the actual state and transition objects,
     * only the diagram structure. State/transition cleanup is the
     * responsibility of the creating code.
     */
    virtual ~LLStateDiagram();

protected:
    /**
     * @brief Adds a state to the state graph.
     * 
     * Registers a state with the diagram, creating an empty transition set for it.
     * This method is typically called implicitly when adding transitions, but can
     * be called explicitly to add states with no outgoing transitions.
     * 
     * @param state Pointer to state to add to the diagram
     * @return true if state was added successfully
     */
    bool addState(LLFSMState *state);

    /**
     * @brief Adds a directed transition between two states.
     * 
     * Creates a one-way transition from start_state to end_state via the specified
     * transition. If the states don't exist in the diagram, they're added automatically.
     * 
     * @param start_state State where transition originates
     * @param end_state State where transition leads
     * @param transition Transition object that triggers this state change
     * @return true if transition was added successfully
     */
    bool addTransition(LLFSMState& start_state, LLFSMState& end_state, LLFSMTransition& transition);

    /**
     * @brief Adds a bidirectional transition between two states.
     * 
     * Creates transitions in both directions between the states using the same
     * transition object. Equivalent to calling addTransition() twice.
     * 
     * @param start_state First state in bidirectional relationship
     * @param end_state Second state in bidirectional relationship 
     * @param transition Transition object that works in both directions
     * @return true if both transitions were added successfully
     */
    bool addUndirectedTransition(LLFSMState& start_state, LLFSMState& end_state, LLFSMTransition& transition);

    /**
     * @brief Adds a transition available from any state.
     * 
     * Creates a "global" transition that can be triggered from any state in the
     * diagram. Useful for emergency stops, resets, or other universal actions.
     * 
     * @param end_state Destination state for the default transition
     * @param transition Transition object available from all states
     */
    void addDefaultTransition(LLFSMState& end_state, LLFSMTransition& transition);

    /**
     * @brief Processes a transition request and returns the resulting state.
     * 
     * Given a current state and transition, determines what the next state should be.
     * Checks state-specific transitions first, then default transitions, then
     * default state as fallback.
     * 
     * This is the core logic that drives state machine behavior.
     * 
     * @param start_state Current state to transition from
     * @param transition Transition being attempted
     * @return Pointer to destination state, or nullptr if no valid transition
     */
    LLFSMState* processTransition(LLFSMState& start_state, LLFSMTransition& transition);

    /**
     * @brief Sets a fallback state for use when no transitions match.
     * 
     * Enables a "default state" mode where any unhandled transition leads to
     * the specified fallback state. Useful for error recovery.
     * 
     * @param default_state State to use as fallback destination
     */
    void setDefaultState(LLFSMState& default_state);

    /**
     * @brief Counts states with no outgoing transitions.
     * 
     * Returns the number of "dead-end" states that have no outgoing transitions.
     * Useful for validating state machine completeness.
     * 
     * @return Number of states with no outgoing transitions
     */
    S32 numDeadendStates();

    /**
     * @brief Checks if a state is registered in this diagram.
     * 
     * Validates that the specified state exists in the diagram and can be
     * used for transitions.
     * 
     * @param state State to check for existence
     * @return true if state exists in diagram, false otherwise
     */
    bool stateIsValid(LLFSMState& state);

    /**
     * @brief Finds a state by its unique ID.
     * 
     * Searches all states in the diagram for one with the specified ID.
     * Linear search, so use sparingly in performance-critical code.
     * 
     * @param state_id Unique ID of state to find
     * @return Pointer to state if found, nullptr otherwise
     */
    LLFSMState* getState(U32 state_id);

public:
    /**
     * @brief Exports the state diagram to a DOT file for visualization.
     * 
     * Creates a Graphviz DOT format file that can be rendered into a visual
     * representation of the state machine. Useful for debugging and documentation.
     * 
     * @param filename Path where DOT file should be written
     * @return true if file was written successfully
     */
    bool saveDotFile(const std::string& filename);
};

/**
 * @brief Executes a finite state machine using a state diagram definition.
 * 
 * LLStateMachine is the runtime execution engine for finite state machines.
 * It maintains current state, processes transitions according to the rules
 * defined in its LLStateDiagram, and manages state lifecycle (entry/exit/execute).
 * 
 * The state machine operates by:
 * 1. Maintaining a current state
 * 2. Processing transition requests through the state diagram
 * 3. Calling appropriate lifecycle methods on states
 * 4. Tracking state history for debugging
 * 
 * Key Features:
 * - Automatic state lifecycle management (onEntry/onExit calls)
 * - Transition validation through state diagram
 * - State history tracking
 * - User data passing to state methods
 * - Manual state setting with optional entry skip
 * 
 * Real-world Usage in Animation System:
 * State machines can control complex animation sequences, UI flows, or 
 * any system that needs structured state-based behavior:
 * 
 * @code
 * // Example: Animation state machine usage
 * LLStateDiagram* diagram = createAnimationStateDiagram();
 * LLStateMachine* machine = new LLStateMachine();
 * machine->setStateDiagram(diagram);
 * 
 * // Set initial state
 * machine->setCurrentState(idle_state, nullptr, false);
 * 
 * // Process transitions based on events
 * if (user_starts_walking) {
 *     machine->processTransition(start_walk_transition, &walk_data);
 * }
 * 
 * // Execute current state logic each frame
 * machine->runCurrentState(&frame_data);
 * @endcode
 * 
 * Thread Safety: Not thread-safe. Should be used from a single thread.
 */
class LLStateMachine
{
protected:
    /// Pointer to the currently active state
    LLFSMState*         mCurrentState;
    /// Pointer to the previously active state (for history/debugging)
    LLFSMState*         mLastState;
    /// Pointer to the last transition that was processed
    LLFSMTransition*    mLastTransition;
    /// Pointer to the state diagram that defines valid states and transitions
    LLStateDiagram*     mStateDiagram;

public:
    /**
     * @brief Constructs a new state machine with no initial state or diagram.
     * 
     * Creates an empty state machine. A state diagram must be set and an initial
     * state chosen before the machine can be used.
     */
    LLStateMachine();
    
    /**
     * @brief Destroys the state machine and cleans up resources.
     * 
     * Note: Does not destroy the state diagram or individual states/transitions.
     * Those objects must be managed separately by the creating code.
     */
    virtual ~LLStateMachine();

    /**
     * @brief Associates this state machine with a state diagram.
     * 
     * Sets the state diagram that defines the structure and rules for this
     * state machine. Must be called before any state operations.
     * 
     * @param diagram Pointer to state diagram to use (not owned by state machine)
     */
    void setStateDiagram(LLStateDiagram* diagram);

    /**
     * @brief Processes a transition request and changes state if valid.
     * 
     * Attempts to process the specified transition from the current state.
     * If the transition is valid according to the state diagram:
     * 1. Calls onExit() on current state
     * 2. Updates current state to destination
     * 3. Calls onEntry() on new state
     * 4. Updates state history
     * 
     * If the transition is invalid, no state change occurs.
     * 
     * @param transition Transition to attempt from current state
     * @param user_data Optional data passed to onExit/onEntry methods
     */
    void processTransition(LLFSMTransition &transition, void* user_data);

    /**
     * @brief Gets the currently active state.
     * 
     * @return Pointer to current state, or nullptr if no state is set
     */
    LLFSMState* getCurrentState() const;

    /**
     * @brief Executes the current state's processing logic.
     * 
     * Calls the execute() method on the currently active state with the
     * provided user data. Should be called regularly (e.g., each frame)
     * to drive state-specific processing.
     * 
     * @param data Optional user data passed to state's execute() method
     */
    void runCurrentState(void *data);

    /**
     * @brief Directly sets the current state bypassing transition validation.
     * 
     * Manually sets the current state without going through normal transition
     * processing. Useful for initialization or emergency state changes.
     * 
     * @param initial_state Pointer to state to make current
     * @param user_data Optional data passed to state lifecycle methods
     * @param skip_entry If true, skips calling onEntry() on the new state
     * @return true if state was set successfully
     */
    bool setCurrentState(LLFSMState *initial_state, void* user_data, bool skip_entry = true);

    /**
     * @brief Sets current state by unique ID bypassing transition validation.
     * 
     * Looks up a state by ID in the state diagram and sets it as current.
     * Equivalent to setCurrentState() but uses ID lookup.
     * 
     * @param state_id Unique ID of state to make current
     * @param user_data Optional data passed to state lifecycle methods
     * @param skip_entry If true, skips calling onEntry() on the new state
     * @return true if state was found and set successfully
     */
    bool setCurrentState(U32 state_id, void* user_data, bool skip_entry = true);
};

#endif // LL_LLSTATEMACHINE_H
