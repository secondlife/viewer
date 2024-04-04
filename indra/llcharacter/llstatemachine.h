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

class LLUniqueID
{
	friend bool	operator==(const LLUniqueID &a, const LLUniqueID &b);
	friend bool	operator!=(const LLUniqueID &a, const LLUniqueID &b);
protected:
	static U32	sNextID;
	U32			mId;
public:
	LLUniqueID(){mId = sNextID++;}
	virtual ~LLUniqueID(){}
	U32		getID() {return mId;}
};

class LLFSMTransition : public LLUniqueID
{
public:
	LLFSMTransition() : LLUniqueID(){};
	virtual std::string getName()const { return "unnamed"; }
};

class LLFSMState : public LLUniqueID
{
public:
	LLFSMState() : LLUniqueID(){};
	virtual void onEntry(void *){};
	virtual void onExit(void *){};
	virtual void execute(void *){};
	virtual std::string getName() const { return "unnamed"; }
};

class LLStateDiagram
{
typedef std::map<LLFSMTransition*, LLFSMState*> Transitions;

friend std::ostream& operator<<(std::ostream &s, LLStateDiagram &FSM);
friend class LLStateMachine;

protected:
	typedef std::map<LLFSMState*, Transitions> StateMap;
	StateMap			mStates;
	Transitions			mDefaultTransitions;
	LLFSMState*			mDefaultState;
	bool				mUseDefaultState;

public:
	LLStateDiagram();
	virtual ~LLStateDiagram();

protected:
	// add a state to the state graph, executed implicitly when adding transitions
	bool addState(LLFSMState *state);

	// add a directed transition between 2 states
	bool addTransition(LLFSMState& start_state, LLFSMState& end_state, LLFSMTransition& transition);

	// add an undirected transition between 2 states
	bool addUndirectedTransition(LLFSMState& start_state, LLFSMState& end_state, LLFSMTransition& transition);

	// add a transition that is taken if none other exist
	void addDefaultTransition(LLFSMState& end_state, LLFSMTransition& transition);

	// process a possible transition, and get the resulting state
	LLFSMState* processTransition(LLFSMState& start_state, LLFSMTransition& transition);

	// add a transition that exists for every state
	void setDefaultState(LLFSMState& default_state);

	// return total number of states with no outgoing transitions
	S32 numDeadendStates();

	// does this state exist in the state diagram?
	bool stateIsValid(LLFSMState& state);

	// get a state pointer by ID
	LLFSMState* getState(U32 state_id);

public:
	// save the graph in a DOT file for rendering and visualization
	bool saveDotFile(const std::string& filename);
};

class LLStateMachine
{
protected:
	LLFSMState*			mCurrentState;
	LLFSMState*			mLastState;
	LLFSMTransition*	mLastTransition;
	LLStateDiagram*		mStateDiagram;

public:
	LLStateMachine();
	virtual ~LLStateMachine();

	// set state diagram
	void setStateDiagram(LLStateDiagram* diagram);

	// process this transition
	void processTransition(LLFSMTransition &transition, void* user_data);

	// returns current state
	LLFSMState*	getCurrentState() const;

	// execute current state
	void runCurrentState(void *data);

	// set state by state pointer
	bool setCurrentState(LLFSMState *initial_state, void* user_data, bool skip_entry = true);

	// set state by unique ID
	bool setCurrentState(U32 state_id, void* user_data, bool skip_entry = true);
};

#endif //_LL_LLSTATEMACHINE_H
