/** 
 * @file lltreenode.h
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	virtual LLTreeState<T>* getState()				{ return mState; }
	virtual const LLTreeState<T>* getState() const	{ return mState; }

	virtual void setState(LLTreeState<T>* state);
	virtual void insert(T* data);
	virtual bool remove(T* data);
	virtual void notifyRemoval(T* data);
	virtual U32 getListenerCount()					{ return mListeners.size(); }
	virtual LLTreeListener<T>* getListener(U32 index) const { return mListeners[index]; }
	virtual void addListener(LLTreeListener<T>* listener) { mListeners.push_back(listener); }
	virtual void removeListener(U32 index) { mListeners.erase(mListeners.begin()+index); }

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
