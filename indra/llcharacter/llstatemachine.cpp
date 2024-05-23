/**
 * @file llstatemachine.cpp
 * @brief LLStateMachine implementation file.
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

#include "linden_common.h"

#include "llstatemachine.h"
#include "llapr.h"

#define FSM_PRINT_STATE_TRANSITIONS (0)

U32 LLUniqueID::sNextID = 0;

bool    operator==(const LLUniqueID &a, const LLUniqueID &b)
{
    return (a.mId == b.mId);
}

bool    operator!=(const LLUniqueID &a, const LLUniqueID &b)
{
    return (a.mId != b.mId);
}

//-----------------------------------------------------------------------------
// LLStateDiagram
//-----------------------------------------------------------------------------
LLStateDiagram::LLStateDiagram()
{
    mDefaultState = NULL;
    mUseDefaultState = false;
}

LLStateDiagram::~LLStateDiagram()
{

}

// add a state to the state graph
bool LLStateDiagram::addState(LLFSMState *state)
{
    mStates[state] = Transitions();
    return true;
}

// add a directed transition between 2 states
bool LLStateDiagram::addTransition(LLFSMState& start_state, LLFSMState& end_state, LLFSMTransition& transition)
{
    StateMap::iterator state_it;
    state_it = mStates.find(&start_state);
    Transitions* state_transitions = NULL;
    if (state_it == mStates.end() )
    {
        addState(&start_state);
        state_transitions = &mStates[&start_state];
    }
    else
    {
        state_transitions = &state_it->second;
    }
    state_it = mStates.find(&end_state);
    if (state_it == mStates.end() )
    {
        addState(&end_state);
    }

    Transitions::iterator transition_it = state_transitions->find(&transition);
    if (transition_it != state_transitions->end())
    {
        LL_ERRS() << "LLStateTable::addDirectedTransition() : transition already exists" << LL_ENDL;
        return false; // transition already exists
    }

    (*state_transitions)[&transition] = &end_state;
    return true;
}

// add an undirected transition between 2 states
bool LLStateDiagram::addUndirectedTransition(LLFSMState& start_state, LLFSMState& end_state, LLFSMTransition& transition)
{
    bool result;
    result = addTransition(start_state, end_state, transition);
    if (result)
    {
        result = addTransition(end_state, start_state, transition);
    }
    return result;
}

// add a transition that exists for every state
void LLStateDiagram::addDefaultTransition(LLFSMState& end_state, LLFSMTransition& transition)
{
    mDefaultTransitions[&transition] = &end_state;
}

// process a possible transition, and get the resulting state
LLFSMState* LLStateDiagram::processTransition(LLFSMState& start_state, LLFSMTransition& transition)
{
    // look up transition
    //LLFSMState** dest_state = (mStates.getValue(&start_state))->getValue(&transition);
    LLFSMState* dest_state = NULL;
    StateMap::iterator state_it = mStates.find(&start_state);
    if (state_it == mStates.end())
    {
        return NULL;
    }
    Transitions::iterator transition_it = state_it->second.find(&transition);

    // try default transitions if state-specific transition not found
    if (transition_it == state_it->second.end())
    {
        dest_state = mDefaultTransitions[&transition];
    }
    else
    {
        dest_state = transition_it->second;
    }

    // if we have a destination state...
    if (NULL != dest_state)
    {
        // ...return it...
        return dest_state;
    }
    // ... otherwise ...
    else
    {
        // ...look for default state...
        if (mUseDefaultState)
        {
            // ...return it if we have it...
            return mDefaultState;
        }
        else
        {
            // ...or else we're still in the same state.
            return &start_state;
        }
    }
}

void LLStateDiagram::setDefaultState(LLFSMState& default_state)
{
    mUseDefaultState = true;
    mDefaultState = &default_state;
}

S32 LLStateDiagram::numDeadendStates()
{
    S32 numDeadends = 0;
    for (StateMap::value_type& state_pair : mStates)
    {
        if (state_pair.second.size() == 0)
        {
            numDeadends++;
        }
    }
    return numDeadends;
}

bool LLStateDiagram::stateIsValid(LLFSMState& state)
{
    if (mStates.find(&state) != mStates.end())
    {
        return true;
    }
    return false;
}

LLFSMState* LLStateDiagram::getState(U32 state_id)
{
    for (StateMap::value_type& state_pair : mStates)
    {
        if (state_pair.first->getID() == state_id)
        {
            return state_pair.first;
        }
    }
    return NULL;
}

bool LLStateDiagram::saveDotFile(const std::string& filename)
{
    LLAPRFile outfile ;
    outfile.open(filename, LL_APR_W);
    apr_file_t* dot_file = outfile.getFileHandle() ;

    if (!dot_file)
    {
        LL_WARNS() << "LLStateDiagram::saveDotFile() : Couldn't open " << filename << " to save state diagram." << LL_ENDL;
        return false;
    }
    apr_file_printf(dot_file, "digraph StateMachine {\n\tsize=\"100,100\";\n\tfontsize=40;\n\tlabel=\"Finite State Machine\";\n\torientation=landscape\n\tratio=.77\n");

    for (StateMap::value_type& state_pair : mStates)
    {
        apr_file_printf(dot_file, "\t\"%s\" [fontsize=28,shape=box]\n", state_pair.first->getName().c_str());
    }
    apr_file_printf(dot_file, "\t\"All States\" [fontsize=30,style=bold,shape=box]\n");

    for (Transitions::value_type& transition_pair : mDefaultTransitions)
    {
        apr_file_printf(dot_file, "\t\"All States\" -> \"%s\" [label = \"%s\",fontsize=24];\n", transition_pair.second->getName().c_str(),
            transition_pair.second->getName().c_str());
    }

    if (mDefaultState)
    {
        apr_file_printf(dot_file, "\t\"All States\" -> \"%s\";\n", mDefaultState->getName().c_str());
    }


    for (StateMap::value_type& state_pair : mStates)
    {
        LLFSMState *state = state_pair.first;

        for (Transitions::value_type& transition_pair : state_pair.second)
        {
            std::string state_name = state->getName();
            std::string target_name = transition_pair.second->getName();
            std::string transition_name = transition_pair.first->getName();
            apr_file_printf(dot_file, "\t\"%s\" -> \"%s\" [label = \"%s\",fontsize=24];\n", state->getName().c_str(),
                target_name.c_str(),
                transition_name.c_str());
        }
    }

    apr_file_printf(dot_file, "}\n");

    return true;
}

std::ostream& operator<<(std::ostream &s, LLStateDiagram &FSM)
{
    if (FSM.mDefaultState)
    {
        s << "Default State: " << FSM.mDefaultState->getName() << "\n";
    }

    for (LLStateDiagram::Transitions::value_type& transition_pair : FSM.mDefaultTransitions)
    {
        s << "Any State -- " << transition_pair.first->getName()
            << " --> " << transition_pair.second->getName() << "\n";
    }

    for (LLStateDiagram::StateMap::value_type& state_pair : FSM.mStates)
    {
        for (LLStateDiagram::Transitions::value_type& transition_pair : state_pair.second)
        {
            s << state_pair.first->getName() << " -- " << transition_pair.first->getName()
                << " --> " << transition_pair.second->getName() << "\n";
        }
        s << "\n";
    }

    return s;
}

//-----------------------------------------------------------------------------
// LLStateMachine
//-----------------------------------------------------------------------------

LLStateMachine::LLStateMachine()
{
    // we haven't received a starting state yet
    mCurrentState = NULL;
    mLastState = NULL;
    mLastTransition = NULL;
    mStateDiagram = NULL;
}

LLStateMachine::~LLStateMachine()
{

}

// returns current state
LLFSMState* LLStateMachine::getCurrentState() const
{
    return mCurrentState;
}

// executes current state
void LLStateMachine::runCurrentState(void *data)
{
    mCurrentState->execute(data);
}

// set current state
bool LLStateMachine::setCurrentState(LLFSMState *initial_state, void* user_data, bool skip_entry)
{
    llassert(mStateDiagram);

    if (mStateDiagram->stateIsValid(*initial_state))
    {
        mLastState = mCurrentState = initial_state;
        if (!skip_entry)
        {
            initial_state->onEntry(user_data);
        }
        return true;
    }

    return false;
}

bool LLStateMachine::setCurrentState(U32 state_id, void* user_data, bool skip_entry)
{
    llassert(mStateDiagram);

    LLFSMState* state = mStateDiagram->getState(state_id);

    if (state)
    {
        mLastState = mCurrentState = state;
        if (!skip_entry)
        {
            state->onEntry(user_data);
        }
        return true;
    }

    return false;
}

void LLStateMachine::processTransition(LLFSMTransition& transition, void* user_data)
{
    llassert(mStateDiagram);

    if (NULL == mCurrentState)
    {
        LL_WARNS() << "mCurrentState == NULL; aborting processTransition()" << LL_ENDL;
        return;
    }

    LLFSMState* new_state = mStateDiagram->processTransition(*mCurrentState, transition);

    if (NULL == new_state)
    {
        LL_WARNS() << "new_state == NULL; aborting processTransition()" << LL_ENDL;
        return;
    }

    mLastTransition = &transition;
    mLastState = mCurrentState;

    if (*mCurrentState != *new_state)
    {
        mCurrentState->onExit(user_data);
        mCurrentState = new_state;
        mCurrentState->onEntry(user_data);
#if FSM_PRINT_STATE_TRANSITIONS
        LL_INFOS() << "Entering state " << mCurrentState->getName() <<
            " on transition " << transition.getName() << " from state " <<
            mLastState->getName() << LL_ENDL;
#endif
    }
}

void LLStateMachine::setStateDiagram(LLStateDiagram* diagram)
{
    mStateDiagram = diagram;
}
