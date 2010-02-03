/**
 * @file   lltreeiterators.h
 * @author Nat Goodspeed
 * @date   2008-08-19
 * @brief  This file defines iterators useful for traversing arbitrary node
 *         classes, potentially polymorphic, linked into strict tree
 *         structures.
 *
 *         Dereferencing any one of these iterators actually yields a @em
 *         pointer to the node in question. For example, given an
 *         LLLinkedIter<MyNode> <tt>li</tt>, <tt>*li</tt> gets you a pointer
 *         to MyNode, and <tt>**li</tt> gets you the MyNode instance itself.
 *         More commonly, instead of writing <tt>li->member</tt>, you write
 *         <tt>(*li)->member</tt> -- as you would if you were traversing an
 *         STL container of MyNode pointers.
 *
 *         It would certainly be possible to build these iterators so that
 *         <tt>*iterator</tt> would return a reference to the node itself
 *         rather than a pointer to the node, and for many purposes it would
 *         even be more convenient. However, that would be insufficiently
 *         flexible. If you want to use an iterator range to (e.g.) initialize
 *         a std::vector collecting results -- you rarely want to actually @em
 *         copy the nodes in question. You're much more likely to want to copy
 *         <i>pointers to</i> the traversed nodes. Hence these iterators
 *         produce pointers.
 *
 *         Though you specify the actual NODE class as the template parameter,
 *         these iterators internally use LLPtrTo<> to discover whether to
 *         store and return an LLPointer<NODE> or a simple NODE*.
 *
 *         By strict tree structures, we mean that each child must have
 *         exactly one parent. This forbids a child claiming any ancestor as a
 *         child of its own. Child nodes with multiple parents will be visited
 *         once for each parent. Cycles in the graph will result in either an
 *         infinite loop or an out-of-memory crash. You Have Been Warned.
 * 
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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

#if ! defined(LL_LLTREEITERATORS_H)
#define LL_LLTREEITERATORS_H

#include "llptrto.h"
#include <vector>
#include <deque>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/function.hpp>
#include <boost/static_assert.hpp>

namespace LLTreeIter
{
    /// Discriminator between LLTreeUpIter and LLTreeDownIter
    enum RootIter { UP, DOWN };
    /// Discriminator between LLTreeDFSIter, LLTreeDFSPostIter and LLTreeBFSIter
    enum WalkIter { DFS_PRE, DFS_POST, BFS };
}

/**
 * LLBaseIter defines some machinery common to all these iterators. We use
 * boost::iterator_facade to define the iterator boilerplate: the conventional
 * operators and methods necessary to implement a standards-conforming
 * iterator. That allows us to specify the actual iterator semantics in terms
 * of equal(), dereference() and increment() methods.
 */
template <class SELFTYPE, class NODE>
class LLBaseIter: public boost::iterator_facade<SELFTYPE,
                                                // use pointer type as the
                                                // reference type
                                                typename LLPtrTo<NODE>::type,
                                                boost::forward_traversal_tag>
{
protected:
    /// LLPtrTo<NODE>::type is either NODE* or LLPointer<NODE>, as appropriate
    typedef typename LLPtrTo<NODE>::type ptr_type;
    /// function that advances from this node to next accepts a node pointer
    /// and returns another
    typedef boost::function<ptr_type(const ptr_type&)> func_type;
    typedef SELFTYPE self_type;
};

/// Functor returning NULL, suitable for an end-iterator's 'next' functor
template <class NODE>
typename LLPtrTo<NODE>::type LLNullNextFunctor(const typename LLPtrTo<NODE>::type&)
{
    return typename LLPtrTo<NODE>::type();
}

/**
 * LLLinkedIter is an iterator over an intrusive singly-linked list. The
 * beginning of the list is represented by LLLinkedIter(list head); the end is
 * represented by LLLinkedIter().
 *
 * The begin LLLinkedIter must be instantiated with a functor to extract the
 * 'next' pointer from the current node. Supposing that the link pointer is @c
 * public, something like:
 *
 * @code
 * NODE* mNext;
 * @endcode
 *
 * you can use (e.g.) <tt>boost::bind(&NODE::mNext, _1)</tt> for the purpose.
 * Alternatively, you can bind whatever accessor method is normally used to
 * advance to the next node, e.g. for:
 *
 * @code
 * NODE* next() const;
 * @endcode
 *
 * you can use <tt>boost::bind(&NODE::next, _1)</tt>.
 */
template <class NODE>
class LLLinkedIter: public LLBaseIter<LLLinkedIter<NODE>, NODE>
{
    typedef LLBaseIter<LLLinkedIter<NODE>, NODE> super;
protected:
    /// some methods need to return a reference to self
    typedef typename super::self_type self_type;
    typedef typename super::ptr_type ptr_type;
    typedef typename super::func_type func_type;
public:
    /// Instantiate an LLLinkedIter to start a range, or to end a range before
    /// a particular list entry. Pass a functor to extract the 'next' pointer
    /// from the current node.
    LLLinkedIter(const ptr_type& entry, const func_type& nextfunc):
        mCurrent(entry),
        mNextFunc(nextfunc)
    {}
    /// Instantiate an LLLinkedIter to end a range at the end of the list
    LLLinkedIter():
        mCurrent(),
        mNextFunc(LLNullNextFunctor<NODE>)
    {}

private:
    /// leverage boost::iterator_facade
    friend class boost::iterator_core_access;

    /// advance
    void increment()
    {
        mCurrent = mNextFunc(mCurrent);
    }
    /// equality
    bool equal(const self_type& that) const { return this->mCurrent == that.mCurrent; }
    /// dereference
    ptr_type& dereference() const { return const_cast<ptr_type&>(mCurrent); }

    ptr_type mCurrent;
    func_type mNextFunc;
};

/**
 * LLTreeUpIter walks from the node in hand to the root of the tree. The term
 * "up" is applied to a tree visualized with the root at the top.
 *
 * LLTreeUpIter is an alias for LLLinkedIter, since any linked tree that you
 * can navigate that way at all contains parent pointers.
 */
template <class NODE>
class LLTreeUpIter: public LLLinkedIter<NODE>
{
    typedef LLLinkedIter<NODE> super;
public:
    /// Instantiate an LLTreeUpIter to start from a particular tree node, or
    /// to end a parent traversal before reaching a particular ancestor. Pass
    /// a functor to extract the 'parent' pointer from the current node.
    LLTreeUpIter(const typename super::ptr_type& node,
                 const typename super::func_type& parentfunc):
        super(node, parentfunc)
    {}
    /// Instantiate an LLTreeUpIter to end a range at the root of the tree
    LLTreeUpIter():
        super()
    {}
};

/**
 * LLTreeDownIter walks from the root of the tree to the node in hand. The
 * term "down" is applied to a tree visualized with the root at the top.
 *
 * Though you instantiate the begin() LLTreeDownIter with a pointer to some
 * node at an arbitrary location in the tree, the root will be the first node
 * you dereference and the passed node will be the last node you dereference.
 *
 * On construction, LLTreeDownIter walks from the current node to the root,
 * capturing the path. Then in use, it replays that walk in reverse. As with
 * all traversals of interesting data structures, it is actively dangerous to
 * modify the tree during an LLTreeDownIter walk.
 */
template <class NODE>
class LLTreeDownIter: public LLBaseIter<LLTreeDownIter<NODE>, NODE>
{
    typedef LLBaseIter<LLTreeDownIter<NODE>, NODE> super;
    typedef typename super::self_type self_type;
protected:
    typedef typename super::ptr_type ptr_type;
    typedef typename super::func_type func_type;
private:
    typedef std::vector<ptr_type> list_type;
public:
    /// Instantiate an LLTreeDownIter to end at a particular tree node. Pass a
    /// functor to extract the 'parent' pointer from the current node.
    LLTreeDownIter(const ptr_type& node,
                   const func_type& parentfunc)
    {
        for (ptr_type n = node; n; n = parentfunc(n))
            mParents.push_back(n);
    }
    /// Instantiate an LLTreeDownIter representing "here", the end of the loop
    LLTreeDownIter() {}

private:
    /// leverage boost::iterator_facade
    friend class boost::iterator_core_access;

    /// advance
    void increment()
    {
        mParents.pop_back();
    }
    /// equality
    bool equal(const self_type& that) const { return this->mParents == that.mParents; }
    /// implement dereference/indirection operators
    ptr_type& dereference() const { return const_cast<ptr_type&>(mParents.back()); }

    list_type mParents;
};

/**
 * When you want to select between LLTreeUpIter and LLTreeDownIter with a
 * compile-time discriminator, use LLTreeRootIter with an LLTreeIter::RootIter
 * template arg.
 */
template <LLTreeIter::RootIter DISCRIM, class NODE>
class LLTreeRootIter
{
    enum { use_a_valid_LLTreeIter_RootIter_value = false };
public:
    /// Bogus constructors for default (unrecognized discriminator) case
    template <typename TYPE1, typename TYPE2>
    LLTreeRootIter(TYPE1, TYPE2)
    {
        BOOST_STATIC_ASSERT(use_a_valid_LLTreeIter_RootIter_value);
    }
    LLTreeRootIter()
    {
        BOOST_STATIC_ASSERT(use_a_valid_LLTreeIter_RootIter_value);
    }
};

/// Specialize for LLTreeIter::UP
template <class NODE>
class LLTreeRootIter<LLTreeIter::UP, NODE>: public LLTreeUpIter<NODE>
{
    typedef LLTreeUpIter<NODE> super;
public:
    /// forward begin ctor
    LLTreeRootIter(const typename super::ptr_type& node,
                   const typename super::func_type& parentfunc):
        super(node, parentfunc)
    {}
    /// forward end ctor
    LLTreeRootIter():
        super()
    {}
};

/// Specialize for LLTreeIter::DOWN
template <class NODE>
class LLTreeRootIter<LLTreeIter::DOWN, NODE>: public LLTreeDownIter<NODE>
{
    typedef LLTreeDownIter<NODE> super;
public:
    /// forward begin ctor
    LLTreeRootIter(const typename super::ptr_type& node,
                   const typename super::func_type& parentfunc):
        super(node, parentfunc)
    {}
    /// forward end ctor
    LLTreeRootIter():
        super()
    {}
};

/**
 * Instantiated with a tree node, typically the root, LLTreeDFSIter "flattens"
 * a depth-first tree walk through that node and all its descendants.
 *
 * The begin() LLTreeDFSIter must be instantiated with functors to obtain from
 * a given node begin() and end() iterators for that node's children. For this
 * reason, you must specify the type of the node's child iterator as an
 * additional template parameter.
 *
 * Specifically, the begin functor must return an iterator whose dereferenced
 * value is a @em pointer to a child tree node. For instance, if each node
 * tracks its children in an STL container of node* pointers, you can simply
 * return that container's begin() iterator.
 *
 * Alternatively, if a node tracks its children with a classic linked list,
 * write a functor returning LLLinkedIter<NODE>.
 *
 * The end() LLTreeDFSIter must, of course, match the begin() iterator's
 * template parameters, but is constructed without runtime parameters.
 */
template <class NODE, typename CHILDITER>
class LLTreeDFSIter: public LLBaseIter<LLTreeDFSIter<NODE, CHILDITER>, NODE>
{
    typedef LLBaseIter<LLTreeDFSIter<NODE, CHILDITER>, NODE> super;
    typedef typename super::self_type self_type;
protected:
    typedef typename super::ptr_type ptr_type;
    // The func_type is different for this: from a NODE pointer, we must
    // obtain a CHILDITER.
    typedef boost::function<CHILDITER(const ptr_type&)> func_type;
private:
    typedef std::vector<ptr_type> list_type;
public:
    /// Instantiate an LLTreeDFSIter to start a depth-first walk. Pass
    /// functors to extract the 'child begin' and 'child end' iterators from
    /// each node.
    LLTreeDFSIter(const ptr_type& node, const func_type& beginfunc, const func_type& endfunc):
        mBeginFunc(beginfunc),
        mEndFunc(endfunc),
		mSkipChildren(false)
    {
        // Only push back this node if it's non-NULL!
        if (node)
            mPending.push_back(node);
    }
    /// Instantiate an LLTreeDFSIter to mark the end of the walk
    LLTreeDFSIter() {}

	/// flags iterator logic to skip traversing children of current node on next increment
	void skipDescendants(bool skip = true) { mSkipChildren = skip; }

private:
    /// leverage boost::iterator_facade
    friend class boost::iterator_core_access;

    /// advance
    /// This implementation is due to http://en.wikipedia.org/wiki/Depth-first_search
    void increment()
    {
        // Capture the node we were just looking at
        ptr_type current = mPending.back();
        // Remove it from mPending so we don't process it again later
        mPending.pop_back();
		if (!mSkipChildren)
		{
			// Add all its children to mPending
			addChildren(current);
		}
		// reset flag after each step
		mSkipChildren = false;
    }
    /// equality
    bool equal(const self_type& that) const { return this->mPending == that.mPending; }
    /// implement dereference/indirection operators
    ptr_type& dereference() const { return const_cast<ptr_type&>(mPending.back()); }

    /// Add the direct children of the specified node to mPending
    void addChildren(const ptr_type& node)
    {
        // If we just use push_back() for each child in turn, we'll end up
        // processing children in reverse order. We don't want to assume
        // CHILDITER is reversible: some of the linked trees we'll be
        // processing manage their children using singly-linked lists. So
        // figure out how many children there are, grow mPending by that size
        // and reverse-copy the children into the new space.
        CHILDITER chi = mBeginFunc(node), chend = mEndFunc(node);
        // grow mPending by the number of children
        mPending.resize(mPending.size() + std::distance(chi, chend));
        // reverse-copy the children into the newly-expanded space
        std::copy(chi, chend, mPending.rbegin());
    }

    /// list of the nodes yet to be processed
    list_type mPending;
    /// functor to extract begin() child iterator
    func_type mBeginFunc;
    /// functor to extract end() child iterator
    func_type mEndFunc;
	/// flag which controls traversal of children (skip children of current node if true)
	bool	mSkipChildren;
};

/**
 * Instantiated with a tree node, typically the root, LLTreeDFSPostIter
 * "flattens" a depth-first tree walk through that node and all its
 * descendants. Whereas LLTreeDFSIter visits each node before visiting any of
 * its children, LLTreeDFSPostIter visits all of a node's children before
 * visiting the node itself.
 *
 * The begin() LLTreeDFSPostIter must be instantiated with functors to obtain
 * from a given node begin() and end() iterators for that node's children. For
 * this reason, you must specify the type of the node's child iterator as an
 * additional template parameter.
 *
 * Specifically, the begin functor must return an iterator whose dereferenced
 * value is a @em pointer to a child tree node. For instance, if each node
 * tracks its children in an STL container of node* pointers, you can simply
 * return that container's begin() iterator.
 *
 * Alternatively, if a node tracks its children with a classic linked list,
 * write a functor returning LLLinkedIter<NODE>.
 *
 * The end() LLTreeDFSPostIter must, of course, match the begin() iterator's
 * template parameters, but is constructed without runtime parameters.
 */
template <class NODE, typename CHILDITER>
class LLTreeDFSPostIter: public LLBaseIter<LLTreeDFSPostIter<NODE, CHILDITER>, NODE>
{
    typedef LLBaseIter<LLTreeDFSPostIter<NODE, CHILDITER>, NODE> super;
    typedef typename super::self_type self_type;
protected:
    typedef typename super::ptr_type ptr_type;
    // The func_type is different for this: from a NODE pointer, we must
    // obtain a CHILDITER.
    typedef boost::function<CHILDITER(const ptr_type&)> func_type;
private:
    // Upon reaching a given node in our pending list, we need to know whether
    // we've already pushed that node's children, so we must associate a bool
    // with each node pointer.
    typedef std::vector< std::pair<ptr_type, bool> > list_type;
public:
    /// Instantiate an LLTreeDFSPostIter to start a depth-first walk. Pass
    /// functors to extract the 'child begin' and 'child end' iterators from
    /// each node.
    LLTreeDFSPostIter(const ptr_type& node, const func_type& beginfunc, const func_type& endfunc)
	    : mBeginFunc(beginfunc),
	    mEndFunc(endfunc),
	    mSkipAncestors(false)
    {
        if (! node)
            return;
        mPending.push_back(typename list_type::value_type(node, false));
        makeCurrent();
    }
    /// Instantiate an LLTreeDFSPostIter to mark the end of the walk
     LLTreeDFSPostIter() : mSkipAncestors(false) {}

	/// flags iterator logic to skip traversing ancestors of current node on next increment
	void skipAncestors(bool skip = true) { mSkipAncestors = skip; }

private:
    /// leverage boost::iterator_facade
    friend class boost::iterator_core_access;

    /// advance
    /// This implementation is due to http://en.wikipedia.org/wiki/Depth-first_search
    void increment()
    {
        // Pop the previous current node
        mPending.pop_back();
        makeCurrent();
    }
    /// equality
    bool equal(const self_type& that) const { return this->mPending == that.mPending; }
    /// implement dereference/indirection operators
    ptr_type& dereference() const { return const_cast<ptr_type&>(mPending.back().first); }

	struct isOpen
	{
		bool operator()(const typename list_type::value_type& item)
		{
			return item.second;
		}
	};

    /// Call this each time we change mPending.back() -- that is, every time
    /// we're about to change the value returned by dereference(). If we
    /// haven't yet pushed the new node's children, do so now.
    void makeCurrent()
    {
		if (mSkipAncestors)
		{
			mPending.erase(std::remove_if(mPending.begin(), mPending.end(), isOpen()), mPending.end());
			mSkipAncestors = false;
		}

        // Once we've popped the last node, this becomes a no-op.
        if (mPending.empty())
            return;
        // Here mPending.back() holds the node pointer we're proposing to
        // dereference next. Have we pushed that node's children yet?
        if (mPending.back().second)
            return;                 // if so, it's okay to visit this node now
        // We haven't yet pushed this node's children. Do so now. Remember
        // that we did -- while the node in question is still back().
        mPending.back().second = true;
        addChildren(mPending.back().first);
        // Now, because we've just changed mPending.back(), make that new node
        // current.
        makeCurrent();
    }

    /// Add the direct children of the specified node to mPending
    void addChildren(const ptr_type& node)
    {
        // If we just use push_back() for each child in turn, we'll end up
        // processing children in reverse order. We don't want to assume
        // CHILDITER is reversible: some of the linked trees we'll be
        // processing manage their children using singly-linked lists. So
        // figure out how many children there are, grow mPending by that size
        // and reverse-copy the children into the new space.
        CHILDITER chi = mBeginFunc(node), chend = mEndFunc(node);
        // grow mPending by the number of children
        mPending.resize(mPending.size() + std::distance(chi, chend));
        // Reverse-copy the children into the newly-expanded space. We can't
        // just use std::copy() because the source is a ptr_type, whereas the
        // dest is a pair of (ptr_type, bool).
        for (typename list_type::reverse_iterator pi = mPending.rbegin(); chi != chend; ++chi, ++pi)
        {
            pi->first = *chi;       // copy the child pointer
            pi->second = false;     // we haven't yet pushed this child's chldren
        }
    }

    /// list of the nodes yet to be processed
    list_type	mPending;
    /// functor to extract begin() child iterator
    func_type	mBeginFunc;
    /// functor to extract end() child iterator
    func_type	mEndFunc;
	/// flags logic to skip traversal of ancestors of current node
	bool		mSkipAncestors;
};

/**
 * Instantiated with a tree node, typically the root, LLTreeBFSIter "flattens"
 * a breadth-first tree walk through that node and all its descendants.
 *
 * The begin() LLTreeBFSIter must be instantiated with functors to obtain from
 * a given node the begin() and end() iterators of that node's children. For
 * this reason, you must specify the type of the node's child iterator as an
 * additional template parameter.
 *
 * Specifically, the begin functor must return an iterator whose dereferenced
 * value is a @em pointer to a child tree node. For instance, if each node
 * tracks its children in an STL container of node* pointers, you can simply
 * return that container's begin() iterator.
 *
 * Alternatively, if a node tracks its children with a classic linked list,
 * write a functor returning LLLinkedIter<NODE>.
 *
 * The end() LLTreeBFSIter must, of course, match the begin() iterator's
 * template parameters, but is constructed without runtime parameters.
 */
template <class NODE, typename CHILDITER>
class LLTreeBFSIter: public LLBaseIter<LLTreeBFSIter<NODE, CHILDITER>, NODE>
{
    typedef LLBaseIter<LLTreeBFSIter<NODE, CHILDITER>, NODE> super;
    typedef typename super::self_type self_type;
protected:
    typedef typename super::ptr_type ptr_type;
    // The func_type is different for this: from a NODE pointer, we must
    // obtain a CHILDITER.
    typedef boost::function<CHILDITER(const ptr_type&)> func_type;
private:
    // We need a FIFO queue rather than a LIFO stack. Use a deque rather than
    // a vector, since vector can't implement pop_front() efficiently.
    typedef std::deque<ptr_type> list_type;
public:
    /// Instantiate an LLTreeBFSIter to start a depth-first walk. Pass
    /// functors to extract the 'child begin' and 'child end' iterators from
    /// each node.
    LLTreeBFSIter(const ptr_type& node, const func_type& beginfunc, const func_type& endfunc):
        mBeginFunc(beginfunc),
        mEndFunc(endfunc)
    {
        if (node)
            mPending.push_back(node);
    }
    /// Instantiate an LLTreeBFSIter to mark the end of the walk
    LLTreeBFSIter() {}

private:
    /// leverage boost::iterator_facade
    friend class boost::iterator_core_access;

    /// advance
    /// This implementation is due to http://en.wikipedia.org/wiki/Breadth-first_search
    void increment()
    {
        // Capture the node we were just looking at
        ptr_type current = mPending.front();
        // Remove it from mPending so we don't process it again later
        mPending.pop_front();
        // Add all its children to mPending
        CHILDITER chend = mEndFunc(current);
        for (CHILDITER chi = mBeginFunc(current); chi != chend; ++chi)
            mPending.push_back(*chi);
    }
    /// equality
    bool equal(const self_type& that) const { return this->mPending == that.mPending; }
    /// implement dereference/indirection operators
    ptr_type& dereference() const { return const_cast<ptr_type&>(mPending.front()); }

    /// list of the nodes yet to be processed
    list_type mPending;
    /// functor to extract begin() child iterator
    func_type mBeginFunc;
    /// functor to extract end() child iterator
    func_type mEndFunc;
};

/**
 * When you want to select between LLTreeDFSIter, LLTreeDFSPostIter and
 * LLTreeBFSIter with a compile-time discriminator, use LLTreeWalkIter with an
 * LLTreeIter::WalkIter template arg.
 */
template <LLTreeIter::WalkIter DISCRIM, class NODE, typename CHILDITER>
class LLTreeWalkIter
{
    enum { use_a_valid_LLTreeIter_WalkIter_value = false };
public:
    /// Bogus constructors for default (unrecognized discriminator) case
    template <typename TYPE1, typename TYPE2>
    LLTreeWalkIter(TYPE1, TYPE2)
    {
        BOOST_STATIC_ASSERT(use_a_valid_LLTreeIter_WalkIter_value);
    }
    LLTreeWalkIter()
    {
        BOOST_STATIC_ASSERT(use_a_valid_LLTreeIter_WalkIter_value);
    }
};

/// Specialize for LLTreeIter::DFS_PRE
template <class NODE, typename CHILDITER>
class LLTreeWalkIter<LLTreeIter::DFS_PRE, NODE, CHILDITER>:
    public LLTreeDFSIter<NODE, CHILDITER>
{
    typedef LLTreeDFSIter<NODE, CHILDITER> super;
public:
    /// forward begin ctor
    LLTreeWalkIter(const typename super::ptr_type& node,
                   const typename super::func_type& beginfunc,
                   const typename super::func_type& endfunc):
        super(node, beginfunc, endfunc)
    {}
    /// forward end ctor
    LLTreeWalkIter():
        super()
    {}
};

/// Specialize for LLTreeIter::DFS_POST
template <class NODE, typename CHILDITER>
class LLTreeWalkIter<LLTreeIter::DFS_POST, NODE, CHILDITER>:
    public LLTreeDFSPostIter<NODE, CHILDITER>
{
    typedef LLTreeDFSPostIter<NODE, CHILDITER> super;
public:
    /// forward begin ctor
    LLTreeWalkIter(const typename super::ptr_type& node,
                   const typename super::func_type& beginfunc,
                   const typename super::func_type& endfunc):
        super(node, beginfunc, endfunc)
    {}
    /// forward end ctor
    LLTreeWalkIter():
        super()
    {}
};

/// Specialize for LLTreeIter::BFS
template <class NODE, typename CHILDITER>
class LLTreeWalkIter<LLTreeIter::BFS, NODE, CHILDITER>:
    public LLTreeBFSIter<NODE, CHILDITER>
{
    typedef LLTreeBFSIter<NODE, CHILDITER> super;
public:
    /// forward begin ctor
    LLTreeWalkIter(const typename super::ptr_type& node,
                   const typename super::func_type& beginfunc,
                   const typename super::func_type& endfunc):
        super(node, beginfunc, endfunc)
    {}
    /// forward end ctor
    LLTreeWalkIter():
        super()
    {}
};

#endif /* ! defined(LL_LLTREEITERATORS_H) */
