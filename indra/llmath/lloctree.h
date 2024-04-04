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

#define OCTREE_DEBUG_COLOR_REMOVE   0x0000FF // r
#define OCTREE_DEBUG_COLOR_INSERT   0x00FF00 // g
#define OCTREE_DEBUG_COLOR_BALANCE  0xFF0000 // b

extern U32 gOctreeMaxCapacity;
extern float gOctreeMinSize;

/*#define LL_OCTREE_PARANOIA_CHECK 0
#if LL_DARWIN
#define LL_OCTREE_MAX_CAPACITY 32
#else
#define LL_OCTREE_MAX_CAPACITY 128
#endif*/

// T is the type of the element referenced by the octree node.
// T_PTR determines how pointers to elements are stored internally.
// LLOctreeNode<T, LLPointer<T>> assumes ownership of inserted elements and
// deletes elements removed from the tree.
// LLOctreeNode<T, T*> doesn't take ownership of inserted elements, so the API
// user is responsible for managing the storage lifecycle of elements added to
// the tree.
template <class T, typename T_PTR> class LLOctreeNode;

template <class T, typename T_PTR>
class LLOctreeListener: public LLTreeListener<T>
{
public:
	typedef LLTreeListener<T> BaseType;
    typedef LLOctreeNode<T, T_PTR> oct_node;

	virtual void handleChildAddition(const oct_node* parent, oct_node* child) = 0;
	virtual void handleChildRemoval(const oct_node* parent, const oct_node* child) = 0;
};

template <class T, typename T_PTR>
class LLOctreeTraveler
{
public:
    virtual void traverse(const LLOctreeNode<T, T_PTR>* node);
    virtual void visit(const LLOctreeNode<T, T_PTR>* branch) = 0;
};

template <class T, typename T_PTR>
class LLOctreeTravelerDepthFirst : public LLOctreeTraveler<T, T_PTR>
{
public:
	virtual void traverse(const LLOctreeNode<T, T_PTR>* node) override;
};

template <class T, typename T_PTR>
class alignas(16) LLOctreeNode : public LLTreeNode<T>
{
    LL_ALIGN_NEW
public:

    typedef LLOctreeTraveler<T, T_PTR>                          oct_traveler;
    typedef LLTreeTraveler<T>                                   tree_traveler;
    typedef std::vector<T_PTR>                                  element_list;
    typedef typename element_list::iterator                     element_iter;
    typedef typename element_list::const_iterator               const_element_iter;
	typedef typename std::vector<LLTreeListener<T>*>::iterator	tree_listener_iter;
    typedef LLOctreeNode<T, T_PTR>**                            child_list;
    typedef LLOctreeNode<T, T_PTR>**                            child_iter;

    typedef LLTreeNode<T>               BaseType;
    typedef LLOctreeNode<T, T_PTR>      oct_node;
    typedef LLOctreeListener<T, T_PTR>  oct_listener;

    enum
    {
        NO_CHILD_NODES = 255 // Note: This is an U8 to match the max value in mChildMap[]
    };

	LLOctreeNode(	const LLVector4a& center, 
					const LLVector4a& size, 
					BaseType* parent, 
					U8 octant = NO_CHILD_NODES)
	:	mParent((oct_node*)parent), 
		mOctant(octant) 
	{ 
		llassert(size[0] >= gOctreeMinSize*0.5f);

		mCenter = center;
		mSize = size;

		updateMinMax();
		if ((mOctant == NO_CHILD_NODES) && mParent)
		{
			mOctant = ((oct_node*) mParent)->getOctant(mCenter);
		}

		clearChildren();
	}

	virtual ~LLOctreeNode()
	{ 
		BaseType::destroyListeners();
		
        const U32 element_count = getElementCount();
        for (U32 i = 0; i < element_count; ++i)
		{
			mData[i]->setBinIndex(-1);
			mData[i] = NULL;
		}

		mData.clear();

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

		return (radius <= gOctreeMinSize && size <= gOctreeMinSize) ||
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
	
    U32 getElementCount() const                     { return (U32)mData.size(); }
    bool isEmpty() const                            { return mData.empty(); }
    element_iter getDataBegin()                     { return mData.begin(); }
    element_iter getDataEnd()                       { return mData.end(); }
    const_element_iter getDataBegin() const         { return mData.cbegin(); }
    const_element_iter getDataEnd() const           { return mData.cend(); }
		
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
			if (idx != NO_CHILD_NODES)
			{
                oct_node* child = mChild[idx];

				if (child->getOctant() != i)
				{
					LL_ERRS() << "Invalid child map, bad octant data." << LL_ENDL;
				}

				if (getOctant(child->getCenter()) != child->getOctant())
				{
					LL_ERRS() << "Invalid child octant compared to position data." << LL_ENDL;
				}
			}
		}
	}


	oct_node* getNodeAt(const LLVector4a& pos, const F32& rad)
	{ 
        oct_node* node = this;

		if (node->isInside(pos, rad))
		{
			//do a quick search by octant
			U8 octant = node->getOctant(pos);
			
			//traverse the tree until we find a node that has no node
			//at the appropriate octant or is smaller than the object.  
			//by definition, that node is the smallest node that contains 
			// the data
			U8 next_node = node->mChildMap[octant];
			
			while (next_node != NO_CHILD_NODES && node->getSize()[0] >= rad)
			{	
				node = node->getChild(next_node);
				octant = node->getOctant(pos);
				next_node = node->mChildMap[octant];
			}
		}
		else if (!node->contains(rad) && node->getParent())
		{ //if we got here, data does not exist in this node
            return ((oct_node*) node->getParent())->getNodeAt(pos, rad);
		}

		return node;
	}
	
	virtual bool insert(T* data)
	{
        //LL_PROFILE_ZONE_NAMED_COLOR("Octree::insert()",OCTREE_DEBUG_COLOR_INSERT);

		if (data == NULL || data->getBinIndex() != -1)
		{
			OCT_ERRS << "!!! INVALID ELEMENT ADDED TO OCTREE BRANCH !!!" << LL_ENDL;
			return false;
		}
        oct_node* parent = getOctParent();

		//is it here?
		if (isInside(data->getPositionGroup()))
		{
			if ((((getElementCount() < gOctreeMaxCapacity || getSize()[0] <= gOctreeMinSize) && contains(data->getBinRadius())) ||
				(data->getBinRadius() > getSize()[0] &&	parent && parent->getElementCount() >= gOctreeMaxCapacity))) 
			{ //it belongs here
                mData.push_back(data);
                data->setBinIndex(getElementCount() - 1);
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
                oct_node::pushCenter(center, size, data);

				// handle case where floating point number gets too small
				LLVector4a val;
				val.setSub(center, getCenter());
				val.setAbs(val);
				LLVector4a min_diff(gOctreeMinSize);

				S32 lt = val.lessThan(min_diff).getGatheredBits() & 0x7;

				if( lt == 0x7 )
				{
                    mData.push_back(data);
                    data->setBinIndex(getElementCount() - 1);
					BaseType::insert(data);
					return true;
				}

#if LL_OCTREE_PARANOIA_CHECK
				if (getChildCount() == 8)
				{
					//this really isn't possible, something bad has happened
					OCT_ERRS << "Octree detected floating point error and gave up." << LL_ENDL;
					return false;
				}
				
				//make sure no existing node matches this position
				for (U32 i = 0; i < getChildCount(); i++)
				{
					if (mChild[i]->getCenter().equals3(center))
					{
						OCT_ERRS << "Octree detected duplicate child center and gave up." << LL_ENDL;
						return false;
					}
				}
#endif

				llassert(size[0] >= gOctreeMinSize*0.5f);
				//make the new kid
                child = new oct_node(center, size, this);
				addChild(child);
								
				child->insert(data);
			}
		}
		else if (parent)
		{
			//it's not in here, give it to the root
			OCT_ERRS << "Octree insertion failed, starting over from root!" << LL_ENDL;

			oct_node* node = this;

			while (parent)
			{
				node = parent;
				parent = node->getOctParent();
			}

			node->insert(data);
		}
		else
		{
			// It's not in here, and we are root.
			// LLOctreeRoot::insert() should have expanded
			// root by now, something is wrong
			OCT_ERRS << "Octree insertion failed! Root expansion failed." << LL_ENDL;
		}

		return false;
	}

	void _remove(T* data, S32 i)
    { //precondition -- getElementCount() > 0, idx is in range [0, getElementCount())

		data->setBinIndex(-1); 
		
        const U32 new_element_count = getElementCount() - 1;
		if (new_element_count > 0)
		{
			if (new_element_count != i)
			{
				mData[i] = mData[new_element_count]; //might unref data, do not access data after this point
				mData[i]->setBinIndex(i);
			}

			mData[new_element_count] = NULL;
			mData.pop_back();
		}
		else
		{
			mData.clear();
		}

		this->notifyRemoval(data);
		checkAlive();
	}

	bool remove(T* data)
	{
        //LL_PROFILE_ZONE_NAMED_COLOR("Octree::remove()", OCTREE_DEBUG_COLOR_REMOVE);

		S32 i = data->getBinIndex();

        if (i >= 0 && i < getElementCount())
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
		LL_WARNS() << "!!! OCTREE REMOVING ELEMENT BY ADDRESS, SEVERE PERFORMANCE PENALTY |||" << LL_ENDL;
		node->removeByAddress(data);
		llassert(data->getBinIndex() == -1);
		return true;
	}

	void removeByAddress(T* data)
	{
        const U32 element_count = getElementCount();
        for (U32 i = 0; i < element_count; ++i)
		{
			if (mData[i] == data)
			{ //we have data
				_remove(data, i);
				LL_WARNS() << "FOUND!" << LL_ENDL;
				return;
			}
		}
		
		for (U32 i = 0; i < getChildCount(); i++)
		{	//we don't contain data, so pass this guy down
            oct_node* child = (oct_node*) getChild(i);
			child->removeByAddress(data);
		}
	}

	void clearChildren()
	{
		mChildCount = 0;
		memset(mChildMap, NO_CHILD_NODES, sizeof(mChildMap));
	}

	void validate()
	{
#if LL_OCTREE_PARANOIA_CHECK
		for (U32 i = 0; i < getChildCount(); i++)
		{
			mChild[i]->validate();
			if (mChild[i]->getParent() != this)
			{
				LL_ERRS() << "Octree child has invalid parent." << LL_ENDL;
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

	void addChild(oct_node* child, bool silent = false)
	{
#if LL_OCTREE_PARANOIA_CHECK

		if (child->getSize().equals3(getSize()))
		{
			OCT_ERRS << "Child size is same as parent size!" << LL_ENDL;
		}

		for (U32 i = 0; i < getChildCount(); i++)
		{
			if(!mChild[i]->getSize().equals3(child->getSize())) 
			{
				OCT_ERRS <<"Invalid octree child size." << LL_ENDL;
			}
			if (mChild[i]->getCenter().equals3(child->getCenter()))
			{
				OCT_ERRS <<"Duplicate octree child position." << LL_ENDL;
			}
		}

		if (mChild.size() >= 8)
		{
			OCT_ERRS <<"Octree node has too many children... why?" << LL_ENDL;
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

	void removeChild(S32 index, bool destroy = false)
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
		memset(mChildMap, NO_CHILD_NODES, sizeof(mChildMap));

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
				removeChild(i, true);
				return;
			}
		}

		OCT_ERRS << "Octree failed to delete requested child." << LL_ENDL;
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

    oct_node* mChild[8];
	U8 mChildMap[8];
	U32 mChildCount;

	element_list mData;
}; 

//just like a regular node, except it might expand on insert and compress on balance
template <class T, typename T_PTR>
class LLOctreeRoot : public LLOctreeNode<T, T_PTR>
{
public:
    typedef LLOctreeNode<T, T_PTR> BaseType;
    typedef LLOctreeNode<T, T_PTR> oct_node;

	LLOctreeRoot(const LLVector4a& center, 
				 const LLVector4a& size, 
				 BaseType* parent)
	:	BaseType(center, size, parent)
	{
	}
	
	bool balance() override
	{	
        //LL_PROFILE_ZONE_NAMED_COLOR("Octree::balance()",OCTREE_DEBUG_COLOR_BALANCE);

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
				this->addChild(child->getChild(i), true);
			}

			//destroy child
			child->clearChildren();
			delete child;

			return false;
		}
		
		return true;
	}

	// LLOctreeRoot::insert
	bool insert(T* data) override
	{
		if (data == nullptr) 
		{
			OCT_ERRS << "!!! INVALID ELEMENT ADDED TO OCTREE ROOT !!!" << LL_ENDL;
			return false;
		}
		
		if (data->getBinRadius() > 4096.0)
		{
			OCT_ERRS << "!!! ELEMENT EXCEEDS MAXIMUM SIZE IN OCTREE ROOT !!!" << LL_ENDL;
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
			//OCT_ERRS << "!!! ELEMENT EXCEEDS RANGE OF SPATIAL PARTITION !!!" << LL_ENDL;
			return false;
		}

		if (this->getSize()[0] > data->getBinRadius() && this->isInside(data->getPositionGroup()))
		{
			//we got it, just act like a branch
			oct_node* node = this->getNodeAt(data);
			if (node == this)
			{
                oct_node::insert(data);
			}
			else if (node->isInside(data->getPositionGroup()))
			{
				node->insert(data);
			}
			else
			{
				// calling node->insert(data) will return us to root
				OCT_ERRS << "Failed to insert data at child node" << LL_ENDL;
			}
		}
		else if (this->getChildCount() == 0)
		{
			//first object being added, just wrap it up
			while (!(this->getSize()[0] > data->getBinRadius() && this->isInside(data->getPositionGroup())))
			{
				LLVector4a center, size;
				center = this->getCenter();
				size = this->getSize();
                oct_node::pushCenter(center, size, data);
				this->setCenter(center);
				size.mul(2.f);
				this->setSize(size);
				this->updateMinMax();
			}
            oct_node::insert(data);
		}
		else
		{
			while (!(this->getSize()[0] > data->getBinRadius() && this->isInside(data->getPositionGroup())))
			{
				//the data is outside the root node, we need to grow
				LLVector4a center(this->getCenter());
				LLVector4a size(this->getSize());

				//expand this node
				LLVector4a newcenter(center);
                oct_node::pushCenter(newcenter, size, data);
				this->setCenter(newcenter);
				LLVector4a size2 = size;
				size2.mul(2.f);
				this->setSize(size2);
				this->updateMinMax();

				llassert(size[0] >= gOctreeMinSize);

				//copy our children to a new branch
                oct_node* newnode = new oct_node(center, size, this);
				
				for (U32 i = 0; i < this->getChildCount(); i++)
				{
                    oct_node* child = this->getChild(i);
					newnode->addChild(child);
				}

				//clear our children and add the root copy
				this->clearChildren();
				this->addChild(newnode);
			}

			//insert the data
			insert(data);
		}

		return false;
	}

    bool isLeaf() const override
    {
        // root can't be a leaf
        return false;
    }
};

//========================
//		LLOctreeTraveler
//========================
template <class T, typename T_PTR>
void LLOctreeTraveler<T, T_PTR>::traverse(const LLOctreeNode<T, T_PTR>* node)
{
	node->accept(this);
	for (U32 i = 0; i < node->getChildCount(); i++)
	{
		traverse(node->getChild(i));
	}
}

template <class T, typename T_PTR>
void LLOctreeTravelerDepthFirst<T, T_PTR>::traverse(const LLOctreeNode<T, T_PTR>* node)
{
	for (U32 i = 0; i < node->getChildCount(); i++)
	{
		traverse(node->getChild(i));
	}
	node->accept(this);
}

#endif
