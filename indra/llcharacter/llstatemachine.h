/** 
 * @file llstatemachine.h
 * @brief LLStateMachine class header file.
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
	BOOL				mUseDefaultState;

public:
	LLStateDiagram();
	virtual ~LLStateDiagram();

protected:
	// add a state to the state graph, executed implicitly when adding transitions
	BOOL addState(LLFSMState *state);

	// add a directed transition between 2 states
	BOOL addTransition(LLFSMState& start_state, LLFSMState& end_state, LLFSMTransition& transition);

	// add an undirected transition between 2 states
	BOOL addUndirectedTransition(LLFSMState& start_state, LLFSMState& end_state, LLFSMTransition& transition);

	// add a transition that is taken if none other exist
	void addDefaultTransition(LLFSMState& end_state, LLFSMTransition& transition);

	// process a possible transition, and get the resulting state
	LLFSMState* processTransition(LLFSMState& start_state, LLFSMTransition& transition);

	// add a transition that exists for every state
	void setDefaultState(LLFSMState& default_state);

	// return total number of states with no outgoing transitions
	S32 numDeadendStates();

	// does this state exist in the state diagram?
	BOOL stateIsValid(LLFSMState& state);

	// get a state pointer by ID
	LLFSMState* getState(U32 state_id);

public:
	// save the graph in a DOT file for rendering and visualization
	BOOL saveDotFile(const std::string& filename);
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
	BOOL setCurrentState(LLFSMState *initial_state, void* user_data, BOOL skip_entry = TRUE);

	// set state by unique ID
	BOOL setCurrentState(U32 state_id, void* user_data, BOOL skip_entry = TRUE);
};

#endif //_LL_LLSTATEMACHINE_H
