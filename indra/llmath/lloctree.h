/** 
 * @file lloctree.h
 * @brief Octree declaration. 
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLOCTREE_H
#define LL_LLOCTREE_H

#include "lltreenode.h"
#include "v3math.h"
#include <vector>
#include <set>

#ifdef LL_RELEASE_FOR_DOWNLOAD
#define OCT_ERRS llwarns
#else
#define OCT_ERRS llerrs
#endif

#define LL_OCTREE_PARANOIA_CHECK 0
#define LL_OCTREE_MAX_CAPACITY 256

template <class T> class LLOctreeState;
template <class T> class LLOctreeNode;

template <class T>
class LLOctreeListener: public LLTreeListener<T>
{
public:
	typedef LLTreeListener<T> BaseType;
	typedef LLOctreeNode<T> oct_node;

	virtual ~LLOctreeListener() { };
	virtual void handleChildAddition(const oct_node* parent, oct_node* child) = 0;
	virtual void handleChildRemoval(const oct_node* parent, const oct_node* child) = 0;
};


template <class T>
class LLOctreeNode : public LLTreeNode<T>
{
public:

	typedef LLTreeNode<T>		BaseType;
	typedef LLTreeState<T>		tree_state;
	typedef LLOctreeState<T>	oct_state;
	typedef LLOctreeNode<T>		oct_node;
	typedef LLOctreeListener<T>	oct_listener;

	static const U8 OCTANT_POSITIVE_X = 0x01;
	static const U8 OCTANT_POSITIVE_Y = 0x02;
	static const U8 OCTANT_POSITIVE_Z = 0x04;
		
	LLOctreeNode(	LLVector3d center, 
					LLVector3d size, 
					tree_state* state, 
					BaseType* parent, 
					U8 octant = 255)
	:	BaseType(state), 
		mParent((oct_node*)parent), 
		mCenter(center), 
		mSize(size), 
		mOctant(octant) 
	{ 
		updateMinMax();
		if ((mOctant == 255) && mParent)
		{
			mOctant = ((oct_node*) mParent)->getOctant(mCenter.mdV);
		}
	}

	virtual ~LLOctreeNode()								{ BaseType::destroyListeners(); delete this->mState; }

	virtual const BaseType* getParent()	const			{ return mParent; }
	virtual void setParent(BaseType* parent)			{ mParent = (oct_node*) parent; }
	virtual const LLVector3d& getCenter() const			{ return mCenter; }
	virtual const LLVector3d& getSize() const			{ return mSize; }
	virtual void setCenter(LLVector3d center)			{ mCenter = center; }
	virtual void setSize(LLVector3d size)				{ mSize = size; }
    virtual bool balance()								{ return getOctState()->balance(); }
	virtual void validate()								{ getOctState()->validate(); }
	virtual U32 getChildCount()	const					{ return getOctState()->getChildCount(); }
	virtual oct_node* getChild(U32 index)				{ return getOctState()->getChild(index); }
	virtual const oct_node* getChild(U32 index) const	{ return getOctState()->getChild(index); }
	virtual U32 getElementCount() const					{ return getOctState()->getElementCount(); }
	virtual void removeByAddress(T* data)				{ getOctState()->removeByAddress(data); }
	virtual bool hasLeafState()	const					{ return getOctState()->isLeaf(); }
	virtual void destroy()								{ getOctState()->destroy(); }
	virtual oct_node* getNodeAt(T* data)				{ return getNodeAt(data->getPositionGroup(), data->getBinRadius()); }
	virtual oct_node* getNodeAt(const LLVector3d& pos, const F64& rad) { return getOctState()->getNodeAt(pos, rad); }
	virtual U8 getOctant() const						{ return mOctant; }
	virtual void setOctant(U8 octant)					{ mOctant = octant; }
	virtual const oct_state* getOctState() const		{ return (const oct_state*) BaseType::mState; }
	virtual oct_state* getOctState()	 				{ return (oct_state*) BaseType::mState; }
	virtual const oct_node*	getOctParent() const		{ return (const oct_node*) getParent(); }
	virtual oct_node* getOctParent() 					{ return (oct_node*) getParent(); }
	virtual void deleteChild(oct_node* child)			{ getOctState()->deleteChild(child); }

	virtual U8 getOctant(const F64 pos[]) const	//get the octant pos is in
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
	
	virtual bool isInside(const LLVector3d& pos, const F64& rad) const
	{
		return rad <= mSize.mdV[0]*2.0 && isInside(pos); 
	}

	virtual bool isInside(T* data) const			
	{ 
		return isInside(data->getPositionGroup(), data->getBinRadius());
	}

	virtual bool isInside(const LLVector3d& pos) const
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
	
	virtual void updateMinMax()
	{
		for (U32 i = 0; i < 3; i++)
		{
			mMax.mdV[i] = mCenter.mdV[i] + mSize.mdV[i];
			mMin.mdV[i] = mCenter.mdV[i] - mSize.mdV[i];
			mCenter.mdV[i] = mCenter.mdV[i];
		}
	}

	virtual oct_listener* getOctListener(U32 index) 
	{ 
		return (oct_listener*) BaseType::getListener(index); 
	}

	bool contains(T* xform)
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

	static void pushCenter(LLVector3d &center, LLVector3d &size, T* data)
	{
		LLVector3d pos(data->getPositionGroup());
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

protected:
	oct_node* mParent;
	LLVector3d mCenter;
	LLVector3d mSize;
	LLVector3d mMax;
	LLVector3d mMin;
	U8 mOctant;
};

template <class T>
class LLOctreeTraveler : public LLTreeTraveler<T>
{
public:
	virtual void traverse(const LLTreeNode<T>* node);
	virtual void visit(const LLTreeState<T>* state) { }
	virtual void visit(const LLOctreeState<T>* branch) = 0;
};

//will pass requests to a child, might make a new child
template <class T>
class LLOctreeState : public LLTreeState<T> 
{
public:
	typedef LLTreeState<T>										BaseType;
	typedef LLOctreeTraveler<T>									oct_traveler;
	typedef LLOctreeNode<T>										oct_node;
	typedef LLOctreeListener<T>									oct_listener;
	typedef LLTreeTraveler<T>									tree_traveler;
	typedef typename std::set<LLPointer<T> >					element_list;
	typedef typename std::set<LLPointer<T> >::iterator			element_iter;
	typedef typename std::set<LLPointer<T> >::const_iterator	const_element_iter;
	typedef typename std::vector<LLTreeListener<T>*>::iterator	tree_listener_iter;
	typedef typename std::vector<LLOctreeNode<T>* >				child_list;
	
	LLOctreeState(oct_node* node = NULL): BaseType(node)	{ this->clearChildren(); }
	virtual ~LLOctreeState() 
	{	
		for (U32 i = 0; i < getChildCount(); i++)
		{
			delete getChild(i);
		}
	}
																
	
	virtual void accept(oct_traveler* visitor)				{ visitor->visit(this); }
	virtual bool isLeaf() const								{ return mChild.empty(); }
	
	virtual U32 getElementCount() const						{ return mData.size(); }
	virtual element_list& getData()							{ return mData; }
	virtual const element_list& getData() const				{ return mData; }
	
	virtual U32 getChildCount()	const						{ return mChild.size(); }
	virtual oct_node* getChild(U32 index)					{ return mChild[index]; }
	virtual const oct_node* getChild(U32 index) const		{ return mChild[index]; }
	virtual child_list& getChildren()						{ return mChild; }
	virtual const child_list& getChildren() const			{ return mChild; }
	
	virtual void accept(tree_traveler* visitor) const		{ visitor->visit(this); }
	virtual void accept(oct_traveler* visitor) const		{ visitor->visit(this); }
	const oct_node* getOctNode() const						{ return (const oct_node*) BaseType::getNode(); }
	oct_node* getOctNode()									{ return (oct_node*) BaseType::getNode(); }

	virtual oct_node* getNodeAt(T* data)
	{
		return getNodeAt(data->getPositionGroup(), data->getBinRadius());
	}

	virtual oct_node* getNodeAt(const LLVector3d& pos, const F64& rad)
	{ 
		LLOctreeNode<T>* node = getOctNode();

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
			OCT_ERRS << "!!! INVALID ELEMENT ADDED TO OCTREE BRANCH !!!" << llendl;
			return false;
		}
		LLOctreeNode<T>* node = getOctNode();
		LLOctreeNode<T>* parent = node->getOctParent();

		//is it here?
		if (node->isInside(data->getPositionGroup()))
		{
			if (getElementCount() < LL_OCTREE_MAX_CAPACITY &&
				(node->contains(data->getBinRadius()) ||
				(data->getBinRadius() > node->getSize().mdV[0] &&
				parent && parent->getElementCount() >= LL_OCTREE_MAX_CAPACITY))) 
			{ //it belongs here
				if (data == NULL)
				{
					OCT_ERRS << "!!! INVALID ELEMENT ADDED TO OCTREE LEAF !!!" << llendl;
					return false;
				}
				
#if LL_OCTREE_PARANOIA_CHECK
				//if this is a redundant insertion, error out (should never happen)
				if (mData.find(data) != mData.end())
				{
					llwarns << "Redundant octree insertion detected. " << data << llendl;
					return false;
				}
#endif

				mData.insert(data);
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
				LLVector3d center(node->getCenter());
				LLVector3d size(node->getSize()*0.5);
		        		
				//push center in direction of data
				LLOctreeNode<T>::pushCenter(center, size, data);

#if LL_OCTREE_PARANOIA_CHECK
				if (getChildCount() == 8)
				{
					//this really isn't possible, something bad has happened
					OCT_ERRS << "Octree detected floating point error and gave up." << llendl;
					//bool check = node->isInside(data);
					return false;
				}
				
				//make sure no existing node matches this position
				for (U32 i = 0; i < getChildCount(); i++)
				{
					if (mChild[i]->getCenter() == center)
					{
						OCT_ERRS << "Octree detected duplicate child center and gave up." << llendl;
						//bool check = node->isInside(data);
						//check = getChild(i)->isInside(data);
						return false;
					}
				}
#endif

				//make the new kid
				LLOctreeState<T>* newstate = new LLOctreeState<T>();
				child = new LLOctreeNode<T>(center, size, newstate, node);
				addChild(child);

				child->insert(data);
			}
		}
		else 
		{
			//it's not in here, give it to the root
			LLOctreeNode<T>* parent = node->getOctParent();
			while (parent)
			{
				node = parent;
				parent = node->getOctParent();
			}

			node->insert(data);
		}

		return false;
	}

	virtual bool remove(T* data)
	{
		oct_node* node = getOctNode();

		if (mData.find(data) != mData.end())
		{	//we have data
			mData.erase(data);
			node->notifyRemoval(data);
			checkAlive();
			return true;
		}
		else if (node->isInside(data))
		{
			oct_node* dest = getNodeAt(data);

			if (dest != node)
			{
				return dest->remove(data);
			}
		}

		//SHE'S GONE MISSING...
		//none of the children have it, let's just brute force this bastard out
		//starting with the root node (UGLY CODE COMETH!)
		oct_node* parent = node->getOctParent();
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

	virtual void removeByAddress(T* data)
	{
        if (mData.find(data) != mData.end())
		{
			mData.erase(data);
			getOctNode()->notifyRemoval(data);
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

	virtual void clearChildren()
	{
		mChild.clear();
	}

	virtual void validate()
	{
#if LL_OCTREE_PARANOIA_CHECK
		LLOctreeNode<T>* node = this->getOctNode();

		for (U32 i = 0; i < getChildCount(); i++)
		{
			mChild[i]->validate();
			if (mChild[i]->getParent() != node)
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

	virtual void destroy()
	{
		for (U32 i = 0; i < getChildCount(); i++) 
		{	
			mChild[i]->destroy();
			delete mChild[i];
		}
	}

	virtual void addChild(oct_node* child, BOOL silent = FALSE) 
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
		child->setParent(getOctNode());

		if (!silent)
		{
			oct_node* node = getOctNode();
			
			for (U32 i = 0; i < node->getListenerCount(); i++)
			{
				oct_listener* listener = node->getOctListener(i);
				listener->handleChildAddition(node, child);
			}
		}
	}

	virtual void removeChild(U8 index, BOOL destroy = FALSE)
	{
		oct_node* node = getOctNode();
		for (U32 i = 0; i < node->getListenerCount(); i++)
		{
			oct_listener* listener = node->getOctListener(i);
			listener->handleChildRemoval(node, getChild(index));
		}

		if (destroy)
		{
			mChild[index]->destroy();
			delete mChild[index];
		}
		mChild.erase(mChild.begin() + index);

		checkAlive();
	}

	virtual void checkAlive()
	{
		if (getChildCount() == 0 && getElementCount() == 0)
		{
			oct_node* node = getOctNode();
			oct_node* parent = node->getOctParent();
			if (parent)
			{
				parent->deleteChild(node);
			}
		}
	}

	virtual void deleteChild(oct_node* node)
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
	child_list mChild;
	element_list mData;
};

//just like a branch, except it might expand the node it points to
template <class T>
class LLOctreeRoot : public LLOctreeState<T>
{
public:
	typedef LLOctreeState<T>	BaseType;
	typedef LLOctreeNode<T>		oct_node;
	
	LLOctreeRoot(oct_node* node = NULL) : BaseType(node) { }
	
	oct_node* getOctNode()	{ return BaseType::getOctNode(); }
	virtual bool isLeaf()	{ return false; }

	virtual bool balance()
	{	
		//the cached node might be invalid, so don't reference it
		if (this->getChildCount() == 1 && 
			!(this->mChild[0]->hasLeafState()) &&
			this->mChild[0]->getElementCount() == 0) 
		{ //if we have only one child and that child is an empty branch, make that child the root
			BaseType* state = this->mChild[0]->getOctState();
			oct_node* child = this->mChild[0];
			oct_node* root = getOctNode();
		
			//make the root node look like the child
			root->setCenter(this->mChild[0]->getCenter());
			root->setSize(this->mChild[0]->getSize());
			root->updateMinMax();

			//reset root node child list
			this->clearChildren();

			//copy the child's children into the root node silently 
			//(don't notify listeners of addition)
			for (U32 i = 0; i < state->getChildCount(); i++)
			{
				addChild(state->getChild(i), TRUE);
			}

			//destroy child
			state->clearChildren();
			delete child;
		}
		
		return true;
	}

	// LLOctreeRoot::insert
	virtual bool insert(T* data)
	{
		if (data == NULL) 
		{
			OCT_ERRS << "!!! INVALID ELEMENT ADDED TO OCTREE ROOT !!!" << llendl;
			return false;
		}
		
		if (data->getBinRadius() > 4096.0)
		{
			OCT_ERRS << "!!! ELEMENT EXCEDES MAXIMUM SIZE IN OCTREE ROOT !!!" << llendl;
		}
		
		LLOctreeNode<T>* node = getOctNode();
		if (node->getSize().mdV[0] > data->getBinRadius() && node->isInside(data->getPositionGroup()))
		{
			//we got it, just act like a branch
			LLOctreeState<T>::insert(data);
		}
		else if (this->getChildCount() == 0)
		{
			//first object being added, just wrap it up
			while (!(node->getSize().mdV[0] > data->getBinRadius() && node->isInside(data->getPositionGroup())))
			{
				LLVector3d center, size;
				center = node->getCenter();
				size = node->getSize();
				LLOctreeNode<T>::pushCenter(center, size, data);
				node->setCenter(center);
				node->setSize(size*2);
				node->updateMinMax();
			}
			LLOctreeState<T>::insert(data);
		}
		else
		{
			//the data is outside the root node, we need to grow
			LLVector3d center(node->getCenter());
			LLVector3d size(node->getSize());

			//expand this node
			LLVector3d newcenter(center);
			LLOctreeNode<T>::pushCenter(newcenter, size, data);
			node->setCenter(newcenter);
			node->setSize(size*2);
			node->updateMinMax();

			//copy our children to a new branch
			LLOctreeState<T>* newstate = new LLOctreeState<T>();
			LLOctreeNode<T>* newnode = new LLOctreeNode<T>(center, size, newstate, node);
			
			for (U32 i = 0; i < this->getChildCount(); i++)
			{
				LLOctreeNode<T>* child = this->getChild(i);
				newstate->addChild(child);
			}

			//clear our children and add the root copy
			this->clearChildren();
			addChild(newnode);

			//insert the data
			node->insert(data);
		}

		return false;
	}
};


//========================
//		LLOctreeTraveler
//========================
template <class T>
void LLOctreeTraveler<T>::traverse(const LLTreeNode<T>* node)
{
	const LLOctreeState<T>* state = (const LLOctreeState<T>*) node->getState();
	state->accept(this);
	for (U32 i = 0; i < state->getChildCount(); i++)
	{
		traverse(state->getChild(i));
	}
}

#endif
