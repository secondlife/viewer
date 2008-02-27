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
	virtual ~LLTreeNode();
	
	virtual bool insert(T* data);
	virtual bool remove(T* data);
	virtual void notifyRemoval(T* data);
	virtual U32 getListenerCount()					{ return mListeners.size(); }
	virtual LLTreeListener<T>* getListener(U32 index) const { return mListeners[index]; }
	virtual void addListener(LLTreeListener<T>* listener) { mListeners.push_back(listener); }

protected:
	void destroyListeners()
	{
		for (U32 i = 0; i < mListeners.size(); i++)
		{
			mListeners[i]->handleDestruction(this);
		}
		mListeners.clear();
	}
	
public:
	std::vector<LLPointer<LLTreeListener<T> > > mListeners;
};

template <class T>
class LLTreeTraveler
{
public:
	virtual ~LLTreeTraveler() { }; 
	virtual void traverse(const LLTreeNode<T>* node) = 0;
	virtual void visit(const LLTreeNode<T>* node) = 0;
};

template <class T>
LLTreeNode<T>::~LLTreeNode()
{ 
	destroyListeners();
};

template <class T>
bool LLTreeNode<T>::insert(T* data)
{ 
	for (U32 i = 0; i < mListeners.size(); i++)
	{
		mListeners[i]->handleInsertion(this, data);
	}
	return true;
};

template <class T>
bool LLTreeNode<T>::remove(T* data)
{
	return true;
};

template <class T>
void LLTreeNode<T>::notifyRemoval(T* data)
{
	for (U32 i = 0; i < mListeners.size(); i++)
	{
		mListeners[i]->handleRemoval(this, data);
	}
}

#endif
