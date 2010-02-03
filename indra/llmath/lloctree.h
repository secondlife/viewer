/** 
 * @file lloctree.h
 * @brief Octree declaration. 
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#ifndef LL_LLOCTREE_H
#define LL_LLOCTREE_H

#include "lltreenode.h"
#include "v3math.h"
#include <vector>
#include <set>

#if LL_RELEASE_WITH_DEBUG_INFO || LL_DEBUG
#define OCT_ERRS LL_ERRS("OctreeErrors")
#else
#define OCT_ERRS LL_WARNS("OctreeErrors")
#endif

#define LL_OCTREE_PARANOIA_CHECK 0
#if LL_DARWIN
#define LL_OCTREE_MAX_CAPACITY 32
#else
#define LL_OCTREE_MAX_CAPACITY 128
#endif

template <class T> class LLOctreeNode;

template <class T>
class LLOctreeListener: public LLTreeListener<T>
{
public:
	typedef LLTreeListener<T> BaseType;
	typedef LLOctreeNode<T> oct_node;

	virtual void handleChildAddition(const oct_node* parent, oct_node* child) = 0;
	virtual void handleChildRemoval(const oct_node* parent, const oct_node* child) = 0;
};

template <class T>
class LLOctreeTraveler
{
public:
	virtual void traverse(const LLOctreeNode<T>* node);
	virtual void visit(const LLOctreeNode<T>* branch) = 0;
};

template <class T>
class LLOctreeNode : public LLTreeNode<T>
{
public:
	typedef LLOctreeTraveler<T>									oct_traveler;
	typedef LLTreeTraveler<T>									tree_traveler;
	typedef typename std::set<LLPointer<T> >					element_list;
	typedef typename std::set<LLPointer<T> >::iterator			element_iter;
	typedef typename std::set<LLPointer<T> >::const_iterator	const_element_iter;
	typedef typename std::vector<LLTreeListener<T>*>::iterator	tree_listener_iter;
	typedef typename std::vector<LLOctreeNode<T>* >				child_list;
	typedef LLTreeNode<T>		BaseType;
	typedef LLOctreeNode<T>		oct_node;
	typedef LLOctreeListener<T>	oct_listener;

	static const U8 OCTANT_POSITIVE_X = 0x01;
	static const U8 OCTANT_POSITIVE_Y = 0x02;
	static const U8 OCTANT_POSITIVE_Z = 0x04;
		
	LLOctreeNode(	LLVector3d center, 
					LLVector3d size, 
					BaseType* parent, 
					U8 octant = 255)
	:	mParent((oct_node*)parent), 
		mCenter(center), 
		mSize(size), 
		mOctant(octant) 
	{ 
		updateMinMax();
		if ((mOctant == 255) && mParent)
		{
			mOctant = ((oct_node*) mParent)->getOctant(mCenter.mdV);
		}

		clearChildren();
	}

	virtual ~LLOctreeNode()								
	{ 
		BaseType::destroyListeners(); 
		
		for (U32 i = 0; i < getChildCount(); i++)
		{
			delete getChild(i);
		} 
	}

	inline const BaseType* getParent()	const			{ return mParent; }
	inline void setParent(BaseType* parent)			{ mParent = (oct_node*) parent; }
	inline const LLVector3d& getCenter() const			{ return mCenter; }
	inline const LLVector3d& getSize() const			{ return mSize; }
	inline void setCenter(LLVector3d center)			{ mCenter = center; }
	inline void setSize(LLVector3d size)				{ mSize = size; }
    inline oct_node* getNodeAt(T* data)				{ return getNodeAt(data->getPositionGroup(), data->getBinRadius()); }
	inline U8 getOctant() const						{ return mOctant; }
	inline void setOctant(U8 octant)					{ mOctant = octant; }
	inline const oct_node*	getOctParent() const		{ return (const oct_node*) getParent(); }
	inline oct_node* getOctParent() 					{ return (oct_node*) getParent(); }
	
	U8 getOctant(const F64 pos[]) const	//get the octant pos is in
	{
		U8 ret = 0;

		if (pos[0] > mCenter.mdV[0])
		{
			ret |= OCTANT_POSITIVE_X;
		}
		if (pos[1] > mCenter.mdV[1])
		{
			ret |= OCTANT_POSITIVE_Y;
		}
		if (pos[2] > mCenter.mdV[2])
		{
			ret |= OCTANT_POSITIVE_Z;
		}

		return ret;
	}
	
	inline bool isInside(const LLVector3d& pos, const F64& rad) const
	{
		return rad <= mSize.mdV[0]*2.0 && isInside(pos); 
	}

	inline bool isInside(T* data) const			
	{ 
		return isInside(data->getPositionGroup(), data->getBinRadius());
	}

	bool isInside(const LLVector3d& pos) const
	{
		const F64& x = pos.mdV[0];
		const F64& y = pos.mdV[1];
		const F64& z = pos.mdV[2];
			
		if (x > mMax.mdV[0] || x <= mMin.mdV[0] ||
			y > mMax.mdV[1] || y <= mMin.mdV[1] ||
			z > mMax.mdV[2] || z <= mMin.mdV[2])
		{
			return false;
		}
		
		return true;
	}
	
	void updateMinMax()
	{
		for (U32 i = 0; i < 3; i++)
		{
			mMax.mdV[i] = mCenter.mdV[i] + mSize.mdV[i];
			mMin.mdV[i] = mCenter.mdV[i] - mSize.mdV[i];
		}
	}

	inline oct_listener* getOctListener(U32 index) 
	{ 
		return (oct_listener*) BaseType::getListener(index); 
	}

	inline bool contains(T* xform)
	{
		return contains(xform->getBinRadius());
	}

	bool contains(F64 radius)
	{
		if (mParent == NULL)
		{	//root node contains nothing
			return false;
		}

		F64 size = mSize.mdV[0];
		F64 p_size = size * 2.0;

		return (radius <= 0.001 && size <= 0.001) ||
				(radius <= p_size && radius > size);
	}

	static void pushCenter(LLVector3d &center, const LLVector3d &size, const T* data)
	{
		const LLVector3d& pos = data->getPositionGroup();
		for (U32 i = 0; i < 3; i++)
		{
			if (pos.mdV[i] > center.mdV[i])
			{
				center.mdV[i] += size.mdV[i];
			}
			else 
			{
				center.mdV[i] -= size.mdV[i];
			}
		}
	}

	void accept(oct_traveler* visitor)				{ visitor->visit(this); }
	virtual bool isLeaf() const						{ return mChild.empty(); }
	
	U32 getElementCount() const						{ return mData.size(); }
	element_list& getData()							{ return mData; }
	const element_list& getData() const				{ return mData; }
	
	U32 getChildCount()	const						{ return mChild.size(); }
	oct_node* getChild(U32 index)					{ return mChild[index]; }
	const oct_node* getChild(U32 index) const		{ return mChild[index]; }
	child_list& getChildren()						{ return mChild; }
	const child_list& getChildren() const			{ return mChild; }
	
	void accept(tree_traveler* visitor) const		{ visitor->visit(this); }
	void accept(oct_traveler* visitor) const		{ visitor->visit(this); }
	
	oct_node* getNodeAt(const LLVector3d& pos, const F64& rad)
	{ 
		LLOctreeNode<T>* node = this;

		if (node->isInside(pos, rad))
		{		
			//do a quick search by octant
			U8 octant = node->getOctant(pos.mdV);
			BOOL keep_going = TRUE;

			//traverse the tree until we find a node that has no node
			//at the appropriate octant or is smaller than the object.  
			//by definition, that node is the smallest node that contains 
			// the data
			while (keep_going && node->getSize().mdV[0] >= rad)
			{	
				keep_going = FALSE;
				for (U32 i = 0; i < node->getChildCount() && !keep_going; i++)
				{
					if (node->getChild(i)->getOctant() == octant)
					{
						node = node->getChild(i);
						octant = node->getOctant(pos.mdV);
						keep_going = TRUE;
					}
				}
			}
		}
		else if (!node->contains(rad) && node->getParent())
		{ //if we got here, data does not exist in this node
			return ((LLOctreeNode<T>*) node->getParent())->getNodeAt(pos, rad);
		}

		return node;
	}
	
	virtual bool insert(T* data)
	{
		if (data == NULL)
		{
			//OCT_ERRS << "!!! INVALID ELEMENT ADDED TO OCTREE BRANCH !!!" << llendl;
			return false;
		}
		LLOctreeNode<T>* parent = getOctParent();

		//is it here?
		if (isInside(data->getPositionGroup()))
		{
			if (getElementCount() < LL_OCTREE_MAX_CAPACITY &&
				(contains(data->getBinRadius()) ||
				(data->getBinRadius() > getSize().mdV[0] &&
				parent && parent->getElementCount() >= LL_OCTREE_MAX_CAPACITY))) 
			{ //it belongs here
#if LL_OCTREE_PARANOIA_CHECK
				//if this is a redundant insertion, error out (should never happen)
				if (mData.find(data) != mData.end())
				{
					llwarns << "Redundant octree insertion detected. " << data << llendl;
					return false;
				}
#endif

				mData.insert(data);
				BaseType::insert(data);
				return true;
			}
			else
			{ 	
				//find a child to give it to
				oct_node* child = NULL;
				for (U32 i = 0; i < getChildCount(); i++)
				{
					child = getChild(i);
					if (child->isInside(data->getPositionGroup()))
					{
						child->insert(data);
						return false;
					}
				}
				
				//it's here, but no kids are in the right place, make a new kid
				LLVector3d center(getCenter());
				LLVector3d size(getSize()*0.5);
		        		
				//push center in direction of data
				LLOctreeNode<T>::pushCenter(center, size, data);

				// handle case where floating point number gets too small
				if( llabs(center.mdV[0] - getCenter().mdV[0]) < F_APPROXIMATELY_ZERO &&
					llabs(center.mdV[1] - getCenter().mdV[1]) < F_APPROXIMATELY_ZERO &&
					llabs(center.mdV[2] - getCenter().mdV[2]) < F_APPROXIMATELY_ZERO)
				{
					mData.insert(data);
					BaseType::insert(data);
					return true;
				}

#if LL_OCTREE_PARANOIA_CHECK
				if (getChildCount() == 8)
				{
					//this really isn't possible, something bad has happened
					OCT_ERRS << "Octree detected floating point error and gave up." << llendl;
					return false;
				}
				
				//make sure no existing node matches this position
				for (U32 i = 0; i < getChildCount(); i++)
				{
					if (mChild[i]->getCenter() == center)
					{
						OCT_ERRS << "Octree detected duplicate child center and gave up." << llendl;
						return false;
					}
				}
#endif

				//make the new kid
				child = new LLOctreeNode<T>(center, size, this);
				addChild(child);
								
				child->insert(data);
			}
		}
		else 
		{
			//it's not in here, give it to the root
			//OCT_ERRS << "Octree insertion failed, starting over from root!" << llendl;

			oct_node* node = this;

			while (parent)
			{
				node = parent;
				parent = node->getOctParent();
			}

			node->insert(data);
		}

		return false;
	}

	bool remove(T* data)
	{
		if (mData.find(data) != mData.end())
		{	//we have data
			mData.erase(data);
			notifyRemoval(data);
			checkAlive();
			return true;
		}
		else if (isInside(data))
		{
			oct_node* dest = getNodeAt(data);

			if (dest != this)
			{
				return dest->remove(data);
			}
		}

		//SHE'S GONE MISSING...
		//none of the children have it, let's just brute force this bastard out
		//starting with the root node (UGLY CODE COMETH!)
		oct_node* parent = getOctParent();
		oct_node* node = this;

		while (parent != NULL)
		{
			node = parent;
			parent = node->getOctParent();
		}

		//node is now root
		llwarns << "!!! OCTREE REMOVING FACE BY ADDRESS, SEVERE PERFORMANCE PENALTY |||" << llendl;
		node->removeByAddress(data);
		return true;
	}

	void removeByAddress(T* data)
	{
        if (mData.find(data) != mData.end())
		{
			mData.erase(data);
			notifyRemoval(data);
			llwarns << "FOUND!" << llendl;
			checkAlive();
			return;
		}
		
		for (U32 i = 0; i < getChildCount(); i++)
		{	//we don't contain data, so pass this guy down
			LLOctreeNode<T>* child = (LLOctreeNode<T>*) getChild(i);
			child->removeByAddress(data);
		}
	}

	void clearChildren()
	{
		mChild.clear();
	}

	void validate()
	{
#if LL_OCTREE_PARANOIA_CHECK
		for (U32 i = 0; i < getChildCount(); i++)
		{
			mChild[i]->validate();
			if (mChild[i]->getParent() != this)
			{
				llerrs << "Octree child has invalid parent." << llendl;
			}
		}
#endif
	}

	virtual bool balance()
	{	
		return false;
	}

	void destroy()
	{
		for (U32 i = 0; i < getChildCount(); i++) 
		{	
			mChild[i]->destroy();
			delete mChild[i];
		}
	}

	void addChild(oct_node* child, BOOL silent = FALSE) 
	{
#if LL_OCTREE_PARANOIA_CHECK
		for (U32 i = 0; i < getChildCount(); i++)
		{
			if(mChild[i]->getSize() != child->getSize()) 
			{
				OCT_ERRS <<"Invalid octree child size." << llendl;
			}
			if (mChild[i]->getCenter() == child->getCenter())
			{
				OCT_ERRS <<"Duplicate octree child position." << llendl;
			}
		}

		if (mChild.size() >= 8)
		{
			OCT_ERRS <<"Octree node has too many children... why?" << llendl;
		}
#endif

		mChild.push_back(child);
		child->setParent(this);

		if (!silent)
		{
			for (U32 i = 0; i < this->getListenerCount(); i++)
			{
				oct_listener* listener = getOctListener(i);
				listener->handleChildAddition(this, child);
			}
		}
	}

	void removeChild(U8 index, BOOL destroy = FALSE)
	{
		for (U32 i = 0; i < this->getListenerCount(); i++)
		{
			oct_listener* listener = getOctListener(i);
			listener->handleChildRemoval(this, getChild(index));
		}

		if (destroy)
		{
			mChild[index]->destroy();
			delete mChild[index];
		}
		mChild.erase(mChild.begin() + index);

		checkAlive();
	}

	void checkAlive()
	{
		if (getChildCount() == 0 && getElementCount() == 0)
		{
			oct_node* parent = getOctParent();
			if (parent)
			{
				parent->deleteChild(this);
			}
		}
	}

	void deleteChild(oct_node* node)
	{
		for (U32 i = 0; i < getChildCount(); i++)
		{
			if (getChild(i) == node)
			{
				removeChild(i, TRUE);
				return;
			}
		}

		//OCT_ERRS << "Octree failed to delete requested child." << llendl;
	}

protected:	
	child_list mChild;
	element_list mData;
	oct_node* mParent;
	LLVector3d mCenter;
	LLVector3d mSize;
	LLVector3d mMax;
	LLVector3d mMin;
	U8 mOctant;
};

//just like a regular node, except it might expand on insert and compress on balance
template <class T>
class LLOctreeRoot : public LLOctreeNode<T>
{
public:
	typedef LLOctreeNode<T>	BaseType;
	typedef LLOctreeNode<T>		oct_node;

	LLOctreeRoot(	LLVector3d center, 
					LLVector3d size, 
					BaseType* parent)
	:	BaseType(center, size, parent)
	{
	}
	
	bool balance()
	{	
		if (this->getChildCount() == 1 && 
			!(this->mChild[0]->isLeaf()) &&
			this->mChild[0]->getElementCount() == 0) 
		{ //if we have only one child and that child is an empty branch, make that child the root
			oct_node* child = this->mChild[0];
					
			//make the root node look like the child
			this->setCenter(this->mChild[0]->getCenter());
			this->setSize(this->mChild[0]->getSize());
			this->updateMinMax();

			//reset root node child list
			this->clearChildren();

			//copy the child's children into the root node silently 
			//(don't notify listeners of addition)
			for (U32 i = 0; i < child->getChildCount(); i++)
			{
				addChild(child->getChild(i), TRUE);
			}

			//destroy child
			child->clearChildren();
			delete child;
		}
		
		return true;
	}

	// LLOctreeRoot::insert
	bool insert(T* data)
	{
		if (data == NULL) 
		{
			//OCT_ERRS << "!!! INVALID ELEMENT ADDED TO OCTREE ROOT !!!" << llendl;
			return false;
		}
		
		if (data->getBinRadius() > 4096.0)
		{
			//OCT_ERRS << "!!! ELEMENT EXCEEDS MAXIMUM SIZE IN OCTREE ROOT !!!" << llendl;
			return false;
		}
		
		const F64 MAX_MAG = 1024.0*1024.0;

		const LLVector3d& v = data->getPositionGroup();
		if (!(fabs(v.mdV[0]-this->mCenter.mdV[0]) < MAX_MAG &&
		      fabs(v.mdV[1]-this->mCenter.mdV[1]) < MAX_MAG &&
		      fabs(v.mdV[2]-this->mCenter.mdV[2]) < MAX_MAG))
		{
			//OCT_ERRS << "!!! ELEMENT EXCEEDS RANGE OF SPATIAL PARTITION !!!" << llendl;
			return false;
		}

		if (this->getSize().mdV[0] > data->getBinRadius() && isInside(data->getPositionGroup()))
		{
			//we got it, just act like a branch
			oct_node* node = getNodeAt(data);
			if (node == this)
			{
				LLOctreeNode<T>::insert(data);
			}
			else
			{
				node->insert(data);
			}
		}
		else if (this->getChildCount() == 0)
		{
			//first object being added, just wrap it up
			while (!(this->getSize().mdV[0] > data->getBinRadius() && isInside(data->getPositionGroup())))
			{
				LLVector3d center, size;
				center = this->getCenter();
				size = this->getSize();
				LLOctreeNode<T>::pushCenter(center, size, data);
				this->setCenter(center);
				this->setSize(size*2);
				this->updateMinMax();
			}
			LLOctreeNode<T>::insert(data);
		}
		else
		{
			while (!(this->getSize().mdV[0] > data->getBinRadius() && isInside(data->getPositionGroup())))
			{
				//the data is outside the root node, we need to grow
				LLVector3d center(this->getCenter());
				LLVector3d size(this->getSize());

				//expand this node
				LLVector3d newcenter(center);
				LLOctreeNode<T>::pushCenter(newcenter, size, data);
				this->setCenter(newcenter);
				this->setSize(size*2);
				this->updateMinMax();

				//copy our children to a new branch
				LLOctreeNode<T>* newnode = new LLOctreeNode<T>(center, size, this);
				
				for (U32 i = 0; i < this->getChildCount(); i++)
				{
					LLOctreeNode<T>* child = this->getChild(i);
					newnode->addChild(child);
				}

				//clear our children and add the root copy
				this->clearChildren();
				addChild(newnode);
			}

			//insert the data
			insert(data);
		}

		return false;
	}
};

//========================
//		LLOctreeTraveler
//========================
template <class T>
void LLOctreeTraveler<T>::traverse(const LLOctreeNode<T>* node)
{
	node->accept(this);
	for (U32 i = 0; i < node->getChildCount(); i++)
	{
		traverse(node->getChild(i));
	}
}
#endif
