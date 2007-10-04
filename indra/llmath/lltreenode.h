/** 
 * @file lltreenode.h
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2007, Linden Research, Inc.
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

#ifndef LL_LLTREENODE_H
#define LL_LLTREENODE_H

#include "stdtypes.h"
#include "xform.h"
#include <vector>

template <class T> class LLTreeNode;
template <class T> class LLTreeTraveler;
template <class T> class LLTreeListener;

template <class T>
class LLTreeState
{
public:
	LLTreeState(LLTreeNode<T>* node)				{ setNode(node); }
	virtual ~LLTreeState() { };
	virtual bool insert(T* data) = 0;
	virtual bool remove(T* data) = 0;
	virtual void setNode(LLTreeNode<T>* node);
	virtual const LLTreeNode<T>* getNode() const	{ return mNode; }
	virtual LLTreeNode<T>* getNode()				{ return mNode; }
	virtual void accept(LLTreeTraveler<T>* traveler) const = 0;
	virtual LLTreeListener<T>* getListener(U32 index) const;
private:
	LLTreeNode<T>* mNode;
};

template <class T>
class LLTreeListener: public LLRefCount
{
public:
	virtual void handleInsertion(const LLTreeNode<T>* node, T* data) = 0;
	virtual void handleRemoval(const LLTreeNode<T>* node, T* data) = 0;
	virtual void handleDestruction(const LLTreeNode<T>* node) = 0;
	virtual void handleStateChange(const LLTreeNode<T>* node) = 0;
};

template <class T>
class LLTreeNode 
{
public:
	LLTreeNode(LLTreeState<T>* state)				{ setState(state); }
	virtual ~LLTreeNode();
	LLTreeState<T>* getState()				{ return mState; }
	const LLTreeState<T>* getState() const	{ return mState; }

	void setState(LLTreeState<T>* state);
	void insert(T* data);
	bool remove(T* data);
	void notifyRemoval(T* data);
	inline U32 getListenerCount()					{ return mListeners.size(); }
	inline LLTreeListener<T>* getListener(U32 index) const { return mListeners[index]; }
	inline void addListener(LLTreeListener<T>* listener) { mListeners.push_back(listener); }
	inline void removeListener(U32 index) { mListeners.erase(mListeners.begin()+index); }

protected:
	void destroyListeners()
	{
		for (U32 i = 0; i < mListeners.size(); i++)
		{
			mListeners[i]->handleDestruction(this);
		}
		mListeners.clear();
	}
	
	LLTreeState<T>* mState;
public:
	std::vector<LLPointer<LLTreeListener<T> > > mListeners;
};

template <class T>
class LLTreeTraveler
{
public:
	virtual ~LLTreeTraveler() { }; 
	virtual void traverse(const LLTreeNode<T>* node) = 0;
	virtual void visit(const LLTreeState<T>* state) = 0;
};

template <class T>
LLTreeNode<T>::~LLTreeNode()
{ 
	destroyListeners();
};

template <class T>
void LLTreeNode<T>::insert(T* data)
{ 
	if (mState->insert(data))
	{
		for (U32 i = 0; i < mListeners.size(); i++)
		{
			mListeners[i]->handleInsertion(this, data);
		}
	}
};

template <class T>
bool LLTreeNode<T>::remove(T* data)
{
	if (mState->remove(data))
	{
		return true;
	}
	return false;
};

template <class T>
void LLTreeNode<T>::notifyRemoval(T* data)
{
	for (U32 i = 0; i < mListeners.size(); i++)
	{
		mListeners[i]->handleRemoval(this, data);
	}
}

template <class T>
void LLTreeNode<T>::setState(LLTreeState<T>* state)
{
	mState = state;
	if (state)
	{
		if (state->getNode() != this)
		{
			state->setNode(this);
		}

		for (U32 i = 0; i < mListeners.size(); i++)
		{
			mListeners[i]->handleStateChange(this);
		}
	}
};

template <class T>
void LLTreeState<T>::setNode(LLTreeNode<T>* node)
{
	mNode = node;
	if (node && node->getState() != this) 
	{
		node->setState(this);
	}
};

template <class T>
LLTreeListener<T>* LLTreeState<T>::getListener(U32 index) const
{
	return mNode->getListener(index);
}

#endif
