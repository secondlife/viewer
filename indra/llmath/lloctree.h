/** 
 * @file lloctree.h
 * @brief Octree declaration. 
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#ifndef LL_LLOCTREE_H
#define LL_LLOCTREE_H

#include "lltreenode.h"
#include "v3math.h"
#include "llvector4a.h"
#include <vector>

#define OCT_ERRS LL_WARNS("OctreeErrors")


extern U32 gOctreeMaxCapacity;
/*#define LL_OCTREE_PARANOIA_CHECK 0
#if LL_DARWIN
#define LL_OCTREE_MAX_CAPACITY 32
#else
#define LL_OCTREE_MAX_CAPACITY 128
#endif*/

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
class LLOctreeTravelerDepthFirst : public LLOctreeTraveler<T>
{
public:
	virtual void traverse(const LLOctreeNode<T>* node);
};

template <class T>
class LLOctreeNode : public LLTreeNode<T>
{
public:

	typedef LLOctreeTraveler<T>									oct_traveler;
	typedef LLTreeTraveler<T>									tree_traveler;
	typedef LLPointer<T>*										element_list;
	typedef LLPointer<T>*										element_iter;
	typedef const LLPointer<T>*									const_element_iter;
	typedef typename std::vector<LLTreeListener<T>*>::iterator	tree_listener_iter;
	typedef LLOctreeNode<T>**									child_list;
	typedef LLOctreeNode<T>**									child_iter;

	typedef LLTreeNode<T>		BaseType;
	typedef LLOctreeNode<T>		oct_node;
	typedef LLOctreeListener<T>	oct_listener;

	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}

	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}

	LLOctreeNode(	const LLVector4a& center, 
					const LLVector4a& size, 
					BaseType* parent, 
					U8 octant = 255)
	:	mParent((oct_node*)parent), 
		mOctant(octant) 
	{ 
		mData = NULL;
		mDataEnd = NULL;

		mCenter = center;
		mSize = size;

		updateMinMax();
		if ((mOctant == 255) && mParent)
		{
			mOctant = ((oct_node*) mParent)->getOctant(mCenter);
		}

		mElementCount = 0;

		clearChildren();
	}

	virtual ~LLOctreeNode()								
	{ 
		BaseType::destroyListeners(); 
		
		for (U32 i = 0; i < mElementCount; ++i)
		{
			mData[i]->setBinIndex(-1);
			mData[i] = NULL;
		}

		free(mData);
		mData = NULL;
		mDataEnd = NULL;

		for (U32 i = 0; i < getChildCount(); i++)
		{
			delete getChild(i);
		} 
	}

	inline const BaseType* getParent()	const			{ return mParent; }
	inline void setParent(BaseType* parent)				{ mParent = (oct_node*) parent; }
	inline const LLVector4a& getCenter() const			{ return mCenter; }
	inline const LLVector4a& getSize() const			{ return mSize; }
	inline void setCenter(const LLVector4a& center)		{ mCenter = center; }
	inline void setSize(const LLVector4a& size)			{ mSize = size; }
    inline oct_node* getNodeAt(T* data)					{ return getNodeAt(data->getPositionGroup(), data->getBinRadius()); }
	inline U8 getOctant() const							{ return mOctant; }
	inline const oct_node*	getOctParent() const		{ return (const oct_node*) getParent(); }
	inline oct_node* getOctParent() 					{ return (oct_node*) getParent(); }
	
	U8 getOctant(const LLVector4a& pos) const			//get the octant pos is in
	{
		return (U8) (pos.greaterThan(mCenter).getGatheredBits() & 0x7);
	}
	
	inline bool isInside(const LLVector4a& pos, const F32& rad) const
	{
		return rad <= mSize[0]*2.f && isInside(pos); 
	}

	inline bool isInside(T* data) const			
	{ 
		return isInside(data->getPositionGroup(), data->getBinRadius());
	}

	bool isInside(const LLVector4a& pos) const
	{
		S32 gt = pos.greaterThan(mMax).getGatheredBits() & 0x7;
		if (gt)
		{
			return false;
		}

		S32 lt = pos.lessEqual(mMin).getGatheredBits() & 0x7;
		if (lt)
		{
			return false;
		}
				
		return true;
	}
	
	void updateMinMax()
	{
		mMax.setAdd(mCenter, mSize);
		mMin.setSub(mCenter, mSize);
	}

	inline oct_listener* getOctListener(U32 index) 
	{ 
		return (oct_listener*) BaseType::getListener(index); 
	}

	inline bool contains(T* xform)
	{
		return contains(xform->getBinRadius());
	}

	bool contains(F32 radius)
	{
		if (mParent == NULL)
		{	//root node contains nothing
			return false;
		}

		F32 size = mSize[0];
		F32 p_size = size * 2.f;

		return (radius <= 0.001f && size <= 0.001f) ||
				(radius <= p_size && radius > size);
	}

	static void pushCenter(LLVector4a &center, const LLVector4a &size, const T* data)
	{
		const LLVector4a& pos = data->getPositionGroup();

		LLVector4Logical gt = pos.greaterThan(center);

		LLVector4a up;
		up = _mm_and_ps(size, gt);

		LLVector4a down;
		down = _mm_andnot_ps(gt, size);

		center.add(up);
		center.sub(down);
	}

	void accept(oct_traveler* visitor)				{ visitor->visit(this); }
	virtual bool isLeaf() const						{ return mChildCount == 0; }
	
	U32 getElementCount() const						{ return mElementCount; }
	bool isEmpty() const							{ return mElementCount == 0; }
	element_list& getData()							{ return mData; }
	const element_list& getData() const				{ return mData; }
	element_iter getDataBegin()						{ return mData; }
	element_iter getDataEnd()						{ return mDataEnd; }
	const_element_iter getDataBegin() const			{ return mData; }
	const_element_iter getDataEnd() const			{ return mDataEnd; }
		
	U32 getChildCount()	const						{ return mChildCount; }
	oct_node* getChild(U32 index)					{ return mChild[index]; }
	const oct_node* getChild(U32 index) const		{ return mChild[index]; }
	child_list& getChildren()						{ return mChild; }
	const child_list& getChildren() const			{ return mChild; }
	
	void accept(tree_traveler* visitor) const		{ visitor->visit(this); }
	void accept(oct_traveler* visitor) const		{ visitor->visit(this); }
	
	void validateChildMap()
	{
		for (U32 i = 0; i < 8; i++)
		{
			U8 idx = mChildMap[i];
			if (idx != 255)
			{
				LLOctreeNode<T>* child = mChild[idx];

				if (child->getOctant() != i)
				{
					llerrs << "Invalid child map, bad octant data." << llendl;
				}

				if (getOctant(child->getCenter()) != child->getOctant())
				{
					llerrs << "Invalid child octant compared to position data." << llendl;
				}
			}
		}
	}


	oct_node* getNodeAt(const LLVector4a& pos, const F32& rad)
	{ 
		LLOctreeNode<T>* node = this;

		if (node->isInside(pos, rad))
		{		
			//do a quick search by octant
			U8 octant = node->getOctant(pos);
			
			//traverse the tree until we find a node that has no node
			//at the appropriate octant or is smaller than the object.  
			//by definition, that node is the smallest node that contains 
			// the data
			U8 next_node = node->mChildMap[octant];
			
			while (next_node != 255 && node->getSize()[0] >= rad)
			{	
				node = node->getChild(next_node);
				octant = node->getOctant(pos);
				next_node = node->mChildMap[octant];
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
		if (data == NULL || data->getBinIndex() != -1)
		{
			OCT_ERRS << "!!! INVALID ELEMENT ADDED TO OCTREE BRANCH !!!" << llendl;
			return false;
		}
		LLOctreeNode<T>* parent = getOctParent();

		//is it here?
		if (isInside(data->getPositionGroup()))
		{
			if ((getElementCount() < gOctreeMaxCapacity && contains(data->getBinRadius()) ||
				(data->getBinRadius() > getSize()[0] &&	parent && parent->getElementCount() >= gOctreeMaxCapacity))) 
			{ //it belongs here
				mElementCount++;
				mData = (element_list) realloc(mData, sizeof(LLPointer<T>)*mElementCount);

				//avoid unref on uninitialized memory
				memset(mData+mElementCount-1, 0, sizeof(LLPointer<T>));

				mData[mElementCount-1] = data;
				mDataEnd = mData + mElementCount;
				data->setBinIndex(mElementCount-1);
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
				LLVector4a center = getCenter();
				LLVector4a size = getSize();
				size.mul(0.5f);
		        		
				//push center in direction of data
				LLOctreeNode<T>::pushCenter(center, size, data);

				// handle case where floating point number gets too small
				LLVector4a val;
				val.setSub(center, getCenter());
				val.setAbs(val);
								
				S32 lt = val.lessThan(LLVector4a::getEpsilon()).getGatheredBits() & 0x7;

				if( lt == 0x7 )
				{
					mElementCount++;
					mData = (element_list) realloc(mData, sizeof(LLPointer<T>)*mElementCount);

					//avoid unref on uninitialized memory
					memset(mData+mElementCount-1, 0, sizeof(LLPointer<T>));

					mData[mElementCount-1] = data;
					mDataEnd = mData + mElementCount;
					data->setBinIndex(mElementCount-1);
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
					if (mChild[i]->getCenter().equals3(center))
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
			OCT_ERRS << "Octree insertion failed, starting over from root!" << llendl;

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

	void _remove(T* data, S32 i)
	{ //precondition -- mElementCount > 0, idx is in range [0, mElementCount)

		mElementCount--;
		data->setBinIndex(-1); 
		
		if (mElementCount > 0)
		{
			if (mElementCount != i)
			{
				mData[i] = mData[mElementCount]; //might unref data, do not access data after this point
				mData[i]->setBinIndex(i);
			}

			mData[mElementCount] = NULL; //needed for unref
			mData = (element_list) realloc(mData, sizeof(LLPointer<T>)*mElementCount);
			mDataEnd = mData+mElementCount;
		}
		else
		{
			mData[0] = NULL; //needed for unref
			free(mData);
			mData = NULL;
			mDataEnd = NULL;
		}

		notifyRemoval(data);
		checkAlive();
	}

	bool remove(T* data)
	{
		S32 i = data->getBinIndex();

		if (i >= 0 && i < mElementCount)
		{
			if (mData[i] == data)
			{ //found it
				_remove(data, i);
				llassert(data->getBinIndex() == -1);
				return true;
			}
		}
		
		if (isInside(data))
		{
			oct_node* dest = getNodeAt(data);

			if (dest != this)
			{
				bool ret = dest->remove(data);
				llassert(data->getBinIndex() == -1);
				return ret;
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
		llassert(data->getBinIndex() == -1);
		return true;
	}

	void removeByAddress(T* data)
	{
        for (U32 i = 0; i < mElementCount; ++i)
		{
			if (mData[i] == data)
			{ //we have data
				_remove(data, i);
				llwarns << "FOUND!" << llendl;
				return;
			}
		}
		
		for (U32 i = 0; i < getChildCount(); i++)
		{	//we don't contain data, so pass this guy down
			LLOctreeNode<T>* child = (LLOctreeNode<T>*) getChild(i);
			child->removeByAddress(data);
		}
	}

	void clearChildren()
	{
		mChildCount = 0;

		U32* foo = (U32*) mChildMap;
		foo[0] = foo[1] = 0xFFFFFFFF;
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

		if (child->getSize().equals3(getSize()))
		{
			OCT_ERRS << "Child size is same as parent size!" << llendl;
		}

		for (U32 i = 0; i < getChildCount(); i++)
		{
			if(!mChild[i]->getSize().equals3(child->getSize())) 
			{
				OCT_ERRS <<"Invalid octree child size." << llendl;
			}
			if (mChild[i]->getCenter().equals3(child->getCenter()))
			{
				OCT_ERRS <<"Duplicate octree child position." << llendl;
			}
		}

		if (mChild.size() >= 8)
		{
			OCT_ERRS <<"Octree node has too many children... why?" << llendl;
		}
#endif

		mChildMap[child->getOctant()] = mChildCount;

		mChild[mChildCount] = child;
		++mChildCount;
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

	void removeChild(S32 index, BOOL destroy = FALSE)
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

		--mChildCount;

		mChild[index] = mChild[mChildCount];
		

		//rebuild child map
		U32* foo = (U32*) mChildMap;
		foo[0] = foo[1] = 0xFFFFFFFF;

		for (U32 i = 0; i < mChildCount; ++i)
		{
			mChildMap[mChild[i]->getOctant()] = i;
		}

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

		OCT_ERRS << "Octree failed to delete requested child." << llendl;
	}

protected:	
	typedef enum
	{
		CENTER = 0,
		SIZE = 1,
		MAX = 2,
		MIN = 3
	} eDName;

	LLVector4a mCenter;
	LLVector4a mSize;
	LLVector4a mMax;
	LLVector4a mMin;
	
	oct_node* mParent;
	U8 mOctant;

	LLOctreeNode<T>* mChild[8];
	U8 mChildMap[8];
	U32 mChildCount;

	element_list mData;
	element_iter mDataEnd;
	U32 mElementCount;
		
}; 

//just like a regular node, except it might expand on insert and compress on balance
template <class T>
class LLOctreeRoot : public LLOctreeNode<T>
{
public:
	typedef LLOctreeNode<T>	BaseType;
	typedef LLOctreeNode<T>		oct_node;

	LLOctreeRoot(const LLVector4a& center, 
				 const LLVector4a& size, 
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

			return false;
		}
		
		return true;
	}

	// LLOctreeRoot::insert
	bool insert(T* data)
	{
		if (data == NULL) 
		{
			OCT_ERRS << "!!! INVALID ELEMENT ADDED TO OCTREE ROOT !!!" << llendl;
			return false;
		}
		
		if (data->getBinRadius() > 4096.0)
		{
			OCT_ERRS << "!!! ELEMENT EXCEEDS MAXIMUM SIZE IN OCTREE ROOT !!!" << llendl;
			return false;
		}
		
		LLVector4a MAX_MAG;
		MAX_MAG.splat(1024.f*1024.f);

		const LLVector4a& v = data->getPositionGroup();

		LLVector4a val;
		val.setSub(v, BaseType::mCenter);
		val.setAbs(val);
		S32 lt = val.lessThan(MAX_MAG).getGatheredBits() & 0x7;

		if (lt != 0x7)
		{
			//OCT_ERRS << "!!! ELEMENT EXCEEDS RANGE OF SPATIAL PARTITION !!!" << llendl;
			return false;
		}

		if (this->getSize()[0] > data->getBinRadius() && isInside(data->getPositionGroup()))
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
			while (!(this->getSize()[0] > data->getBinRadius() && isInside(data->getPositionGroup())))
			{
				LLVector4a center, size;
				center = this->getCenter();
				size = this->getSize();
				LLOctreeNode<T>::pushCenter(center, size, data);
				this->setCenter(center);
				size.mul(2.f);
				this->setSize(size);
				this->updateMinMax();
			}
			LLOctreeNode<T>::insert(data);
		}
		else
		{
			while (!(this->getSize()[0] > data->getBinRadius() && isInside(data->getPositionGroup())))
			{
				//the data is outside the root node, we need to grow
				LLVector4a center(this->getCenter());
				LLVector4a size(this->getSize());

				//expand this node
				LLVector4a newcenter(center);
				LLOctreeNode<T>::pushCenter(newcenter, size, data);
				this->setCenter(newcenter);
				LLVector4a size2 = size;
				size2.mul(2.f);
				this->setSize(size2);
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

template <class T>
void LLOctreeTravelerDepthFirst<T>::traverse(const LLOctreeNode<T>* node)
{
	for (U32 i = 0; i < node->getChildCount(); i++)
	{
		traverse(node->getChild(i));
	}
	node->accept(this);
}

#endif
