/** 
 * @file llstatemachine.cpp
 * @brief LLStateMachine implementation file.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "llstatemachine.h"
#include "llapr.h"

#define FSM_PRINT_STATE_TRANSITIONS (0)

U32 LLUniqueID::sNextID = 0;

bool	operator==(const LLUniqueID &a, const LLUniqueID &b)
{
	return (a.mId == b.mId);
}

bool	operator!=(const LLUniqueID &a, const LLUniqueID &b)
{
	return (a.mId != b.mId);
}

//-----------------------------------------------------------------------------
// LLStateDiagram
//-----------------------------------------------------------------------------
LLStateDiagram::LLStateDiagram()
{
	mUseDefaultState = FALSE;
}

LLStateDiagram::~LLStateDiagram()
{

}

// add a state to the state graph
BOOL LLStateDiagram::addState(LLFSMState *state)
{
	mStates[state] = Transitions();
	return TRUE;
}

// add a directed transition between 2 states
BOOL LLStateDiagram::addTransition(LLFSMState& start_state, LLFSMState& end_state, LLFSMTransition& transition)
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
		llerrs << "LLStateTable::addDirectedTransition() : transition already exists" << llendl;
		return FALSE; // transition already exists
	}

	(*state_transitions)[&transition] = &end_state;
	return TRUE;
}

// add an undirected transition between 2 states
BOOL LLStateDiagram::addUndirectedTransition(LLFSMState& start_state, LLFSMState& end_state, LLFSMTransition& transition)
{
	BOOL result;
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
	mUseDefaultState = TRUE;
	mDefaultState = &default_state;
}

S32 LLStateDiagram::numDeadendStates()
{
	S32 numDeadends = 0;
	StateMap::iterator state_it;
	for(state_it = mStates.begin(); state_it != mStates.end(); ++state_it)
	{
		if (state_it->second.size() == 0)
		{
			numDeadends++;
		}
	}
	return numDeadends;
}

BOOL LLStateDiagram::stateIsValid(LLFSMState& state)
{
	if (mStates.find(&state) != mStates.end())
	{
		return TRUE;
	}
	return FALSE;
}

LLFSMState* LLStateDiagram::getState(U32 state_id)
{
	StateMap::iterator state_it;
	for(state_it = mStates.begin(); state_it != mStates.end(); ++state_it)
	{
		if (state_it->first->getID() == state_id)
		{
			return state_it->first;
		}
	}
	return NULL;
}

BOOL LLStateDiagram::saveDotFile(const std::string& filename)
{
	LLAPRFile outfile ;
	outfile.open(filename, LL_APR_W);
	apr_file_t* dot_file = outfile.getFileHandle() ;

	if (!dot_file)
	{
		llwarns << "LLStateDiagram::saveDotFile() : Couldn't open " << filename << " to save state diagram." << llendl;
		return FALSE;
	}
	apr_file_printf(dot_file, "digraph StateMachine {\n\tsize=\"100,100\";\n\tfontsize=40;\n\tlabel=\"Finite State Machine\";\n\torientation=landscape\n\tratio=.77\n");
	
	StateMap::iterator state_it;
	for(state_it = mStates.begin(); state_it != mStates.end(); ++state_it)
	{
		apr_file_printf(dot_file, "\t\"%s\" [fontsize=28,shape=box]\n", state_it->first->getName().c_str());
	}
	apr_file_printf(dot_file, "\t\"All States\" [fontsize=30,style=bold,shape=box]\n");

	Transitions::iterator transitions_it;
	for(transitions_it = mDefaultTransitions.begin(); transitions_it != mDefaultTransitions.end(); ++transitions_it)
	{
		apr_file_printf(dot_file, "\t\"All States\" -> \"%s\" [label = \"%s\",fontsize=24];\n", transitions_it->second->getName().c_str(), 
			transitions_it->second->getName().c_str());
	}

	if (mDefaultState)
	{
		apr_file_printf(dot_file, "\t\"All States\" -> \"%s\";\n", mDefaultState->getName().c_str());
	}

	
	for(state_it = mStates.begin(); state_it != mStates.end(); ++state_it)
	{
		LLFSMState *state = state_it->first;

		Transitions::iterator transitions_it;
		for(transitions_it = state_it->second.begin();
			transitions_it != state_it->second.end();
			++transitions_it)
		{
			std::string state_name = state->getName();
			std::string target_name = transitions_it->second->getName();
			std::string transition_name = transitions_it->first->getName();
			apr_file_printf(dot_file, "\t\"%s\" -> \"%s\" [label = \"%s\",fontsize=24];\n", state->getName().c_str(), 
				target_name.c_str(), 
				transition_name.c_str());
		}
	}

	apr_file_printf(dot_file, "}\n");

	return TRUE;
}

std::ostream& operator<<(std::ostream &s, LLStateDiagram &FSM)
{
	if (FSM.mDefaultState)
	{
		s << "Default State: " << FSM.mDefaultState->getName() << "\n";
	}

	LLStateDiagram::Transitions::iterator transitions_it;
	for(transitions_it = FSM.mDefaultTransitions.begin(); 
		transitions_it != FSM.mDefaultTransitions.end(); 
		++transitions_it)
	{
		s << "Any State -- " << transitions_it->first->getName()
			<< " --> " << transitions_it->second->getName() << "\n";
	}

	LLStateDiagram::StateMap::iterator state_it;
	for(state_it = FSM.mStates.begin(); state_it != FSM.mStates.end(); ++state_it)
	{
		LLStateDiagram::Transitions::iterator transitions_it;
		for(transitions_it = state_it->second.begin();
			transitions_it != state_it->second.end();
			++transitions_it)
		{
			s << state_it->first->getName() << " -- " << transitions_it->first->getName() 
				<< " --> " << transitions_it->second->getName() << "\n";
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
LLFSMState*	LLStateMachine::getCurrentState() const
{
	return mCurrentState;
}

// executes current state
void LLStateMachine::runCurrentState(void *data)
{
	mCurrentState->execute(data);
}

// set current state
BOOL LLStateMachine::setCurrentState(LLFSMState *initial_state, void* user_data, BOOL skip_entry)
{
	llassert(mStateDiagram);

	if (mStateDiagram->stateIsValid(*initial_state))
	{
		mLastState = mCurrentState = initial_state;
		if (!skip_entry)
		{
			initial_state->onEntry(user_data);
		}
		return TRUE;
	}

	return FALSE;
}

BOOL LLStateMachine::setCurrentState(U32 state_id, void* user_data, BOOL skip_entry)
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
		return TRUE;
	}

	return FALSE;
}

void LLStateMachine::processTransition(LLFSMTransition& transition, void* user_data)
{
	llassert(mStateDiagram);
	
	if (NULL == mCurrentState)
	{
		llwarns << "mCurrentState == NULL; aborting processTransition()" << llendl;
		return;
	}

	LLFSMState* new_state = mStateDiagram->processTransition(*mCurrentState, transition);

	if (NULL == new_state)
	{
		llwarns << "new_state == NULL; aborting processTransition()" << llendl;
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
		llinfos << "Entering state " << mCurrentState->getName() <<
			" on transition " << transition.getName() << " from state " << 
			mLastState->getName() << llendl;
#endif
	}
}

void LLStateMachine::setStateDiagram(LLStateDiagram* diagram)
{
	mStateDiagram = diagram;
}
