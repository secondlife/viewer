/**
 * @file   lltreeiterators.cpp
 * @author Nat Goodspeed
 * @date   2008-08-20
 * @brief  Test of lltreeiterators.h
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

// Precompiled header
#include "linden_common.h"


// STL headers
// std headers
#include <iostream>
#include <sstream>
#include <string>
// external library headers
#include <boost/bind.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/foreach.hpp>

// associated header
#include "../lltreeiterators.h"
#include "../llpointer.h"

#include "../test/lltut.h"

/*****************************************************************************
*   tut test group
*****************************************************************************/
namespace tut
{
    struct iter_data
    {
    };
    typedef test_group<iter_data> iter_group;
    typedef iter_group::object iter_object;
    tut::iter_group ig("lltreeiterators");
} // namespace tut

/*****************************************************************************
*   boost::get_pointer() specialization for LLPointer<>
*****************************************************************************/
// This specialization of boost::get_pointer() undoubtedly belongs in
// llmemory.h. It's used by boost::bind() so that you can pass an
// LLPointer<Foo> as well as a Foo* to a functor such as
// boost::bind(&Foo::method, _1).
//namespace boost
//{
    template <class NODE>
    NODE* get_pointer(const LLPointer<NODE>& ptr) { return ptr.get(); }
//};

/*****************************************************************************
*   ScopeLabel
*****************************************************************************/
class ScopeLabel
{
public:
    ScopeLabel(const std::string& label): mLabel(label)
    {
        std::cout << "Entering " << mLabel << '\n';
    }
    ~ScopeLabel()
    {
        std::cout << "Leaving  " << mLabel << '\n';
    }
private:
    std::string mLabel;
};

/*****************************************************************************
*   Cleanup
*****************************************************************************/
// Yes, we realize this is redundant with auto_ptr and LLPointer and all
// kinds of better mechanisms. But in this particular source file, we need to
// test nodes managed with plain old dumb pointers as well as nodes managed
// with LLPointer, so we introduce this mechanism.
//
// In the general case, when we declare a Cleanup for some pointer, delete the
// pointer when the Cleanup goes out of scope.
template <typename PTRTYPE>
struct Cleanup
{
    Cleanup(const PTRTYPE& ptr): mPtr(ptr) {}
    ~Cleanup()
    {
        delete mPtr;
    }
    PTRTYPE mPtr;
};

// But when the pointer is an LLPointer<something>, Cleanup is a no-op:
// LLPointer will handle the cleanup automagically.
template <typename NODE>
struct Cleanup< LLPointer<NODE> >
{
    Cleanup(const LLPointer<NODE>& ptr) {}
    ~Cleanup() {}
};

/*****************************************************************************
*   Expected
*****************************************************************************/
// Expected is a base class used to capture the expected results -- a sequence
// of string node names -- from one of our traversals of this example data.
// Its subclasses initialize it with a pair of string iterators. It's not
// strictly necessary to customize Expected to model Boost.Range, it's just
// convenient.
struct Expected
{
    template <typename ITER>
    Expected(ITER begin, ITER end):
        strings(begin, end)
    {}
    /*------ The following are to make Expected work with Boost.Range ------*/
    typedef std::vector<std::string> container_type;
    typedef container_type::iterator iterator;
    typedef container_type::const_iterator const_iterator;
    typedef container_type::size_type size_type;
    container_type strings;
    iterator begin() { return strings.begin(); }
    iterator end()   { return strings.end(); }
    size_type size() { return strings.size(); }
    const_iterator begin() const { return strings.begin(); }
    const_iterator end() const   { return strings.end(); }
};

// We have a couple of generic Expected template subclasses. This list of
// strings is used for the "else" case when all specializations fail.
const char* bad_strings[] = { "FAIL" };

/*****************************************************************************
*   verify()
*****************************************************************************/
// test function: given (an object modeling) a Boost.Range of tree nodes,
// compare the sequence of visited node names with a range of expected name
// strings. Report success both with std::cout output and a bool return. The
// string desc parameter is to identify the different tests.
template <typename NODERANGE, typename STRINGRANGE>
bool verify(const std::string& desc, NODERANGE noderange, STRINGRANGE expected)
{
    typename boost::range_iterator<NODERANGE>::type
        nri = boost::begin(noderange),
        nrend = boost::end(noderange);
    typename boost::range_iterator<STRINGRANGE>::type
        sri = boost::begin(expected),
        srend = boost::end(expected);
    // We choose to loop over both sequences explicitly rather than using
    // std::equal() or std::lexicographical_compare(). The latter tells you
    // whether one sequence is *less* than the other -- it doesn't tell you
    // equality. std::equal() needs you to verify the sequence lengths ahead
    // of time. Anyway, comparing explicitly allows us to report much more
    // information about any sequence mismatch.
    for ( ; nri != nrend && sri != srend; ++nri, ++sri)
    {
        if ((*nri)->name() != *sri)
        {
            std::cout << desc << " mismatch: "
                      << "expected " << *sri << ", got " << (*nri)->name() << "\n";
            return false;
        }
    }
    if (nri != nrend)
    {
        std::cout << desc << " produced too many items:\n";
        for ( ; nri != nrend; ++nri)
        {
            std::cout << "  " << (*nri)->name() << '\n';
        }
        return false;
    }
    if (sri != srend)
    {
        std::cout << desc << " produced too few items, omitting:\n";
        for ( ; sri != srend; ++sri)
        {
            std::cout << "  " << *sri << '\n';
        }
        return false;
    }
//  std::cout << desc << " test passed\n";
    return true;
}

/*****************************************************************************
*   PlainNode: LLLinkIter, non-refcounted
*****************************************************************************/
class PlainNode
{
public:
    PlainNode(const std::string& name, PlainNode* next=NULL):
        mName(name),
        mNext(next)
    {}
    ~PlainNode()
    {
        delete mNext;
    }
    std::string name() const { return mName; }
    PlainNode* next() const { return mNext; }
public:                             // if this were 'private', couldn't bind mNext
    PlainNode* mNext;
private:
    std::string mName;
};

namespace tut
{
    template<> template<>
    void iter_object::test<1>()
    {
//      set_test_name("LLLinkedIter -- non-refcounted class");
        PlainNode* last(new PlainNode("c"));
        PlainNode* second(new PlainNode("b", last));
        PlainNode* first(new PlainNode("a", second));
        Cleanup<PlainNode*> cleanup(first);
        static const char* cseq[] = { "a", "b", "c" };
        Expected seq(boost::begin(cseq), boost::end(cseq));
        std::string desc1("Iterate by public link member");
//      std::cout << desc1 << ":\n";
        // Try instantiating an iterator with NULL. This test is less about
        // "did we iterate once?" than "did we avoid blowing up?"
        for (LLLinkedIter<PlainNode> pni(NULL, boost::bind(&PlainNode::mNext, _1)), end;
             pni != end; ++pni)
        {
//          std::cout << (*pni)->name() << '\n';
            ensure("LLLinkedIter<PlainNode>(NULL)", false);
        }
        ensure(desc1,
               verify(desc1,
                      boost::make_iterator_range(LLLinkedIter<PlainNode>(first,
                                                                         boost::bind(&PlainNode::mNext, _1)),
                                                 LLLinkedIter<PlainNode>()),
                      seq));
        std::string desc2("Iterate by next() method");
//      std::cout << desc2 << ":\n";
//      for (LLLinkedIter<PlainNode> pni(first, boost::bind(&PlainNode::next, _1)); ! (pni == end); ++pni)
//          std::cout << (**pni).name() << '\n';
        ensure(desc2,
               verify(desc2,
                      boost::make_iterator_range(LLLinkedIter<PlainNode>(first,
                                                                         boost::bind(&PlainNode::next, _1)),
                                                 LLLinkedIter<PlainNode>()),
                      seq));
        {
//          LLLinkedIter<PlainNode> pni(first, boost::bind(&PlainNode::next, _1));
//          std::cout << "First  is " << (*pni++)->name() << '\n';
//          std::cout << "Second is " << (*pni  )->name() << '\n';
        }
        {
            LLLinkedIter<PlainNode> pni(first, boost::bind(&PlainNode::next, _1));
            ensure_equals("first",  (*pni++)->name(), "a");
            ensure_equals("second", (*pni  )->name(), "b");
        }
    }
} // tut

/*****************************************************************************
*   RCNode: LLLinkIter, refcounted
*****************************************************************************/
class RCNode;
typedef LLPointer<RCNode> RCNodePtr;

class RCNode: public LLRefCount
{
public:
    RCNode(const std::string& name, const RCNodePtr& next=RCNodePtr()):
        mName(name),
        mNext(next)
    {
//      std::cout << "New  RCNode(" << mName << ")\n";
    }
    RCNode(const RCNode& that):
        mName(that.mName),
        mNext(that.mNext)
    {
//      std::cout << "Copy RCNode(" << mName << ")\n";
    }
    virtual ~RCNode();
    std::string name() const { return mName; }
    RCNodePtr next() const { return mNext; }
public:                             // if this were 'private', couldn't bind mNext
    RCNodePtr mNext;
private:
    std::string mName;
};

std::ostream& operator<<(std::ostream& out, const RCNode& node)
{
    out << "RCNode(" << node.name() << ')';
    return out;
}

// This string contains the node name of the last RCNode destroyed. We use it
// to validate that LLLinkedIter<RCNode> in fact contains LLPointer<RCNode>,
// and that therefore an outstanding LLLinkedIter to an instance of a
// refcounted class suffices to keep that instance alive.
std::string last_RCNode_destroyed;

RCNode::~RCNode()
{
//  std::cout << "Kill " << *this << "\n";
    last_RCNode_destroyed = mName;
}

namespace tut
{
    template<> template<>
    void iter_object::test<2>()
    {
//      set_test_name("LLLinkedIter -- refcounted class");
        LLLinkedIter<RCNode> rcni, end2;
        {
//          ScopeLabel label("inner scope");
            RCNodePtr head(new RCNode("x", new RCNode("y", new RCNode("z"))));
//          for (rcni = LLLinkedIter<RCNode>(head, boost::bind(&RCNode::mNext, _1)); rcni != end2; ++rcni)
//              std::cout << **rcni << '\n';
            rcni = LLLinkedIter<RCNode>(head, boost::bind(&RCNode::next, _1));
        }
//      std::cout << "Now the LLLinkedIter<RCNode> is the only remaining reference to RCNode chain\n";
        ensure_equals(last_RCNode_destroyed, "");
        ensure(rcni != end2);
        ensure_equals((*rcni)->name(), "x");
        ++rcni;
        ensure_equals(last_RCNode_destroyed, "x");
        ensure(rcni != end2);
        ensure_equals((*rcni)->name(), "y");
        ++rcni;
        ensure_equals(last_RCNode_destroyed, "y");
        ensure(rcni != end2);
        ensure_equals((*rcni)->name(), "z");
        ++rcni;
        ensure_equals(last_RCNode_destroyed, "z");
        ensure(rcni == end2);
    }
}

/*****************************************************************************
*   TreeNode
*****************************************************************************/
class TreeNode;
typedef LLPointer<TreeNode> TreeNodePtr;

/**
 * TreeNode represents a refcounted tree-node class that hasn't (yet) been
 * modified to incorporate LLTreeIter methods. This illustrates how you can
 * use tree iterators either standalone, or with free functions.
 */
class TreeNode: public LLRefCount
{
public:
    typedef std::vector<TreeNodePtr> list_type;
    typedef list_type::const_iterator child_iterator;

    // To avoid cycles, use a "weak" raw pointer for the parent link
    TreeNode(const std::string& name, TreeNode* parent=0):
        mParent(parent),
        mName(name)
    {}
    TreeNodePtr newChild(const std::string& name)
    {
        TreeNodePtr child(new TreeNode(name, this));
        mChildren.push_back(child);
        return child;
    }
    std::string name() const { return mName; }
    TreeNodePtr getParent() const { return mParent; }
    child_iterator child_begin() const { return mChildren.begin(); }
    child_iterator child_end() const   { return mChildren.end(); }
private:
    std::string mName;
    // To avoid cycles, use a "weak" raw pointer for the parent link
    TreeNode* mParent;
    list_type mChildren;
};

/**
 * This is an example of a helper function to facilitate iterating from a
 * TreeNode up to the root or down from the root (see LLTreeIter::RootIter).
 *
 * Example:
 * @code
 * BOOST_FOREACH(TreeNodePtr node, getRootRange<LLTreeIter::UP>(somenode))
 * {
 *     std::cout << node->name() << '\n';
 * }
 * @endcode
 */
template <LLTreeIter::RootIter DISCRIM>
boost::iterator_range< LLTreeRootIter<DISCRIM, TreeNode> >
getRootRange(const TreeNodePtr& node)
{
    typedef LLTreeRootIter<DISCRIM, TreeNode> iter_type;
    typedef boost::iterator_range<iter_type> range_type;
    return range_type(iter_type(node, boost::bind(&TreeNode::getParent, _1)),
                      iter_type());
}

/**
 * This is an example of a helper function to facilitate walking a given
 * TreeNode's subtree in any supported order (see LLTreeIter::WalkIter).
 *
 * Example:
 * @code
 * BOOST_FOREACH(TreeNodePtr node, getWalkRange<LLTreeIter::DFS_PRE>(root))
 * {
 *     std::cout << node->name() << '\n';
 * }
 * @endcode
 */
template <LLTreeIter::WalkIter DISCRIM>
boost::iterator_range< LLTreeWalkIter<DISCRIM, TreeNode, TreeNode::child_iterator> >
getWalkRange(const TreeNodePtr& node)
{
    typedef LLTreeWalkIter<DISCRIM, TreeNode, TreeNode::child_iterator> iter_type;
    typedef boost::iterator_range<iter_type> range_type;
    return range_type(iter_type(node,
                                boost::bind(&TreeNode::child_begin, _1),
                                boost::bind(&TreeNode::child_end, _1)),
                      iter_type());
}

/*****************************************************************************
*   EnhancedTreeNode
*****************************************************************************/
class EnhancedTreeNode;
typedef LLPointer<EnhancedTreeNode> EnhancedTreeNodePtr;

/**
 * More typically, you enhance the tree-node class itself with template
 * methods like the above. This EnhancedTreeNode class illustrates the
 * technique. Normally, of course, you'd simply add these methods to TreeNode;
 * we put them in a separate class to preserve the undecorated TreeNode class
 * to illustrate (and test) the use of plain tree iterators and standalone
 * helper functions.
 *
 * We originally implemented EnhancedTreeNode as a subclass of TreeNode -- but
 * because TreeNode stores and manipulates TreeNodePtrs and TreeNode*s,
 * reusing its methods required so much ugly downcast logic that we gave up
 * and restated the whole class. Bear in mind that logically these aren't two
 * separate classes; logically they're two snapshots of the @em same class at
 * different moments in time.
 */
class EnhancedTreeNode: public LLRefCount
{
public:
    /*-------------- The following is restated from TreeNode ---------------*/
    typedef std::vector<EnhancedTreeNodePtr> list_type;
    typedef list_type::const_iterator child_iterator;

    // To avoid cycles, use a "weak" raw pointer for the parent link
    EnhancedTreeNode(const std::string& name, EnhancedTreeNode* parent=0):
        mParent(parent),
        mName(name)
    {}
    EnhancedTreeNodePtr newChild(const std::string& name)
    {
        EnhancedTreeNodePtr child(new EnhancedTreeNode(name, this));
        mChildren.push_back(child);
        return child;
    }
    std::string name() const { return mName; }
    EnhancedTreeNodePtr getParent() const { return mParent; }
    child_iterator child_begin() const { return mChildren.begin(); }
    child_iterator child_end() const   { return mChildren.end(); }

private:
    std::string mName;
    // To avoid cycles, use a "weak" raw pointer for the parent link
    EnhancedTreeNode* mParent;
    list_type mChildren;
public:
    /*----- End of TreeNode; what follows is new with EnhancedTreeNode -----*/

    /**
     * Because the type of the iterator range returned by getRootRange()
     * depends on the discriminator enum value, instead of a simple typedef we
     * use a templated struct. Example usage:
     *
     * @code
     * for (EnhancedTreeNode::root_range<LLTreeIter::UP>::type range =
     *      somenode->getRootRange<LLTreeIter::UP>();
     *      range.first != range.second; ++range.first)
     * {
     *     std::cout << (*range.first)->name() << '\n';
     * }
     * @endcode
     */
    template <LLTreeIter::RootIter DISCRIM>
    struct root_range
    {
        typedef boost::iterator_range< LLTreeRootIter<DISCRIM, EnhancedTreeNode> > type;
    };

    /**
     * Helper method for walking up to (or down from) the tree root. See
     * LLTreeIter::RootIter.
     *
     * Example usage:
     * @code
     * BOOST_FOREACH(EnhancedTreeNodePtr node, somenode->getRootRange<LLTreeIter::UP>())
     * {
     *     std::cout << node->name() << '\n';
     * }
     * @endcode
     */
    template <LLTreeIter::RootIter DISCRIM>
    typename root_range<DISCRIM>::type getRootRange() const
    {
        typedef typename root_range<DISCRIM>::type range_type;
        typedef typename range_type::iterator iter_type;
        return range_type(iter_type(const_cast<EnhancedTreeNode*>(this),
                                    boost::bind(&EnhancedTreeNode::getParent, _1)),
                          iter_type());
    }

    /**
     * Because the type of the iterator range returned by getWalkRange()
     * depends on the discriminator enum value, instead of a simple typedef we
     * use a templated stuct. Example usage:
     *
     * @code
     * for (EnhancedTreeNode::walk_range<LLTreeIter::DFS_PRE>::type range =
     *      somenode->getWalkRange<LLTreeIter::DFS_PRE>();
     *      range.first != range.second; ++range.first)
     * {
     *     std::cout << (*range.first)->name() << '\n';
     * }
     * @endcode
     */
    template <LLTreeIter::WalkIter DISCRIM>
    struct walk_range
    {
        typedef boost::iterator_range< LLTreeWalkIter<DISCRIM,
                                                      EnhancedTreeNode,
                                                      EnhancedTreeNode::child_iterator> > type;
    };

    /**
     * Helper method for walking a given node's subtree in any supported
     * order (see LLTreeIter::WalkIter).
     *
     * Example usage:
     * @code
     * BOOST_FOREACH(EnhancedTreeNodePtr node, somenode->getWalkRange<LLTreeIter::DFS_PRE>())
     * {
     *     std::cout << node->name() << '\n';
     * }
     * @endcode
     */
    template <LLTreeIter::WalkIter DISCRIM>
    typename walk_range<DISCRIM>::type getWalkRange() const
    {
        typedef typename walk_range<DISCRIM>::type range_type;
        typedef typename range_type::iterator iter_type;
        return range_type(iter_type(const_cast<EnhancedTreeNode*>(this),
                                    boost::bind(&EnhancedTreeNode::child_begin, _1),
                                    boost::bind(&EnhancedTreeNode::child_end, _1)),
                          iter_type());
    }
};

/*****************************************************************************
*   PlainTree
*****************************************************************************/
struct PlainTree
{
    PlainTree(const std::string& name, PlainTree* parent=0):
        mName(name),
        mParent(parent),
        mNextSibling(0),
        mFirstChild(0)
    {
        mLastChildLink = &mFirstChild;
    }
    ~PlainTree()
    {
        delete mNextSibling;
        delete mFirstChild;
    }
    PlainTree* newChild(const std::string& name)
    {
        PlainTree* child(new PlainTree(name, this));
        *mLastChildLink = child;
        mLastChildLink = &child->mNextSibling;
        return child;
    }
    std::string name() const { return mName; }

    std::string mName;
    PlainTree* mParent;
    PlainTree* mNextSibling;
    PlainTree* mFirstChild;
    PlainTree** mLastChildLink;
};

// This "classic" tree tracks each node's children with a linked list anchored
// at the parent's mFirstChild and linked through each child's mNextSibling.
// LLTreeDFSIter<> and LLTreeBFSIter<> need functors to return begin()/end()
// iterators over a given node's children. But because this tree's children
// aren't stored in an STL container, we can't just export that container's
// begin()/end(). Instead we'll use LLLinkedIter<> to view the hand-maintained
// linked list as an iterator range. The straightforward way to do that would
// be to add child_begin() and child_end() methods. But let's say (for the
// sake of argument) that this struct is so venerable we don't dare modify it
// even to add new methods. Well, we can use free functions (or functors) too.
LLLinkedIter<PlainTree> PlainTree_child_begin(PlainTree* node)
{
    return LLLinkedIter<PlainTree>(node->mFirstChild, boost::bind(&PlainTree::mNextSibling, _1));
}

LLLinkedIter<PlainTree> PlainTree_child_end(PlainTree* node)
{
    return LLLinkedIter<PlainTree>();
}

/**
 * This is an example of a helper function to facilitate iterating from a
 * PlainTree up to the root or down from the root (see LLTreeIter::RootIter).
 * Note that we're simply overloading the same getRootRange() helper function
 * name we used for TreeNode.
 *
 * Example:
 * @code
 * BOOST_FOREACH(PlainTree* node, getRootRange<LLTreeIter::UP>(somenode))
 * {
 *     std::cout << node->name() << '\n';
 * }
 * @endcode
 */
template <LLTreeIter::RootIter DISCRIM>
boost::iterator_range< LLTreeRootIter<DISCRIM, PlainTree> >
getRootRange(PlainTree* node)
{
    typedef LLTreeRootIter<DISCRIM, PlainTree> iter_type;
    typedef boost::iterator_range<iter_type> range_type;
    return range_type(iter_type(node, boost::bind(&PlainTree::mParent, _1)),
                      iter_type());
}

/**
 * This is an example of a helper function to facilitate walking a given
 * PlainTree's subtree in any supported order (see LLTreeIter::WalkIter). Note
 * that we're simply overloading the same getWalkRange() helper function name
 * we used for TreeNode.
 *
 * Example:
 * @code
 * BOOST_FOREACH(PlainTree* node, getWalkRange<LLTreeIter::DFS_PRE>(root))
 * {
 *     std::cout << node->name() << '\n';
 * }
 * @endcode
 */
template <LLTreeIter::WalkIter DISCRIM>
boost::iterator_range< LLTreeWalkIter<DISCRIM, PlainTree, LLLinkedIter<PlainTree> > >
getWalkRange(PlainTree* node)
{
    typedef LLTreeWalkIter<DISCRIM, PlainTree, LLLinkedIter<PlainTree> > iter_type;
    typedef boost::iterator_range<iter_type> range_type;
    return range_type(iter_type(node,
                                PlainTree_child_begin,
                                PlainTree_child_end),
                      iter_type());
}

// We could go through the exercise of writing EnhancedPlainTree containing
// root_range, getRootRange(), walk_range and getWalkRange() members -- but we
// won't. See EnhancedTreeNode for examples.

/*****************************************************************************
*   Generic tree test data
*****************************************************************************/
template <class NODE>
typename LLPtrTo<NODE>::type example_tree()
{
    typedef typename LLPtrTo<NODE>::type NodePtr;
    NodePtr root(new NODE("root"));
    NodePtr A(root->newChild("A"));
    NodePtr A1(A->newChild("A1"));
/*  NodePtr A1a*/(A1->newChild("A1a"));
/*  NodePtr A1b*/(A1->newChild("A1b"));
/*  NodePtr A1c*/(A1->newChild("A1c"));
    NodePtr A2(A->newChild("A2"));
/*  NodePtr A2a*/(A2->newChild("A2a"));
/*  NodePtr A2b*/(A2->newChild("A2b"));
/*  NodePtr A2c*/(A2->newChild("A2c"));
    NodePtr A3(A->newChild("A3"));
/*  NodePtr A3a*/(A3->newChild("A3a"));
/*  NodePtr A3b*/(A3->newChild("A3b"));
/*  NodePtr A3c*/(A3->newChild("A3c"));
    NodePtr B(root->newChild("B"));
    NodePtr B1(B->newChild("B1"));
/*  NodePtr B1a*/(B1->newChild("B1a"));
/*  NodePtr B1b*/(B1->newChild("B1b"));
/*  NodePtr B1c*/(B1->newChild("B1c"));
    NodePtr B2(B->newChild("B2"));
/*  NodePtr B2a*/(B2->newChild("B2a"));
/*  NodePtr B2b*/(B2->newChild("B2b"));
/*  NodePtr B2c*/(B2->newChild("B2c"));
    NodePtr B3(B->newChild("B3"));
/*  NodePtr B3a*/(B3->newChild("B3a"));
/*  NodePtr B3b*/(B3->newChild("B3b"));
/*  NodePtr B3c*/(B3->newChild("B3c"));
    NodePtr C(root->newChild("C"));
    NodePtr C1(C->newChild("C1"));
/*  NodePtr C1a*/(C1->newChild("C1a"));
/*  NodePtr C1b*/(C1->newChild("C1b"));
/*  NodePtr C1c*/(C1->newChild("C1c"));
    NodePtr C2(C->newChild("C2"));
/*  NodePtr C2a*/(C2->newChild("C2a"));
/*  NodePtr C2b*/(C2->newChild("C2b"));
/*  NodePtr C2c*/(C2->newChild("C2c"));
    NodePtr C3(C->newChild("C3"));
/*  NodePtr C3a*/(C3->newChild("C3a"));
/*  NodePtr C3b*/(C3->newChild("C3b"));
/*  NodePtr C3c*/(C3->newChild("C3c"));
    return root;
}

// WalkExpected<WalkIter> is the list of string node names we expect from a
// WalkIter traversal of our example_tree() data.
template <LLTreeIter::WalkIter DISCRIM>
struct WalkExpected: public Expected
{
    // Initialize with bad_strings: we don't expect to use this generic case,
    // only the specializations. Note that for a classic C-style array we must
    // pass a pair of iterators rather than extracting boost::begin() and
    // boost::end() within the target constructor: a template ctor accepts
    // these classic C-style arrays as char** rather than char*[length]. Oh well.
    WalkExpected(): Expected(boost::begin(bad_strings), boost::end(bad_strings)) {}
};

// list of string node names we expect from traversing example_tree() in
// DFS_PRE order
const char* dfs_pre_strings[] =
{
    "root",
    "A",
    "A1",
    "A1a",
    "A1b",
    "A1c",
    "A2",
    "A2a",
    "A2b",
    "A2c",
    "A3",
    "A3a",
    "A3b",
    "A3c",
    "B",
    "B1",
    "B1a",
    "B1b",
    "B1c",
    "B2",
    "B2a",
    "B2b",
    "B2c",
    "B3",
    "B3a",
    "B3b",
    "B3c",
    "C",
    "C1",
    "C1a",
    "C1b",
    "C1c",
    "C2",
    "C2a",
    "C2b",
    "C2c",
    "C3",
    "C3a",
    "C3b",
    "C3c"
};

// specialize WalkExpected<DFS_PRE> with the expected strings
template <>
struct WalkExpected<LLTreeIter::DFS_PRE>: public Expected
{
    WalkExpected(): Expected(boost::begin(dfs_pre_strings), boost::end(dfs_pre_strings)) {}
};

// list of string node names we expect from traversing example_tree() in
// DFS_POST order
const char* dfs_post_strings[] =
{
    "A1a",
    "A1b",
    "A1c",
    "A1",
    "A2a",
    "A2b",
    "A2c",
    "A2",
    "A3a",
    "A3b",
    "A3c",
    "A3",
    "A",
    "B1a",
    "B1b",
    "B1c",
    "B1",
    "B2a",
    "B2b",
    "B2c",
    "B2",
    "B3a",
    "B3b",
    "B3c",
    "B3",
    "B",
    "C1a",
    "C1b",
    "C1c",
    "C1",
    "C2a",
    "C2b",
    "C2c",
    "C2",
    "C3a",
    "C3b",
    "C3c",
    "C3",
    "C",
    "root"
};

// specialize WalkExpected<DFS_POST> with the expected strings
template <>
struct WalkExpected<LLTreeIter::DFS_POST>: public Expected
{
    WalkExpected(): Expected(boost::begin(dfs_post_strings), boost::end(dfs_post_strings)) {}
};

// list of string node names we expect from traversing example_tree() in BFS order
const char* bfs_strings[] =
{
    "root",
    "A",
    "B",
    "C",
    "A1",
    "A2",
    "A3",
    "B1",
    "B2",
    "B3",
    "C1",
    "C2",
    "C3",
    "A1a",
    "A1b",
    "A1c",
    "A2a",
    "A2b",
    "A2c",
    "A3a",
    "A3b",
    "A3c",
    "B1a",
    "B1b",
    "B1c",
    "B2a",
    "B2b",
    "B2c",
    "B3a",
    "B3b",
    "B3c",
    "C1a",
    "C1b",
    "C1c",
    "C2a",
    "C2b",
    "C2c",
    "C3a",
    "C3b",
    "C3c"
};

// specialize WalkExpected<BFS> with the expected strings
template <>
struct WalkExpected<LLTreeIter::BFS>: public Expected
{
    WalkExpected(): Expected(boost::begin(bfs_strings), boost::end(bfs_strings)) {}
};

// extract a particular "arbitrary" node from the example_tree() data: the
// second (middle) node at each child level
template <class NODE, typename CHILDITER>
typename LLPtrTo<NODE>::type
get_B2b(const typename LLPtrTo<NODE>::type& root,
        const boost::function<CHILDITER(const typename LLPtrTo<NODE>::type&)>& child_begin)
{
    typedef typename LLPtrTo<NODE>::type NodePtr;
    CHILDITER Bi(child_begin(root));
    ++Bi;
    NodePtr B(*Bi);
    CHILDITER B2i(child_begin(B));
    ++B2i;
    NodePtr B2(*B2i);
    CHILDITER B2bi(child_begin(B2));
    ++B2bi;
    NodePtr B2b(*B2bi);
    return B2b;
}

// RootExpected<RootIter> is the list of string node names we expect from a
// RootIter traversal of our example_tree() data.
template <LLTreeIter::RootIter DISCRIM>
struct RootExpected: public Expected
{
    // Initialize with bad_strings: we don't expect to use this generic case,
    // only the specializations.
    RootExpected(): Expected(boost::begin(bad_strings), boost::end(bad_strings)) {}
};

// list of string node names we expect from traversing UP from
// example_tree()'s B2b node
const char* up_from_B2b[] =
{
    "B2b",
    "B2",
    "B",
    "root"
};

// specialize RootExpected<UP> with the expected strings
template <>
struct RootExpected<LLTreeIter::UP>: public Expected
{
    RootExpected(): Expected(boost::begin(up_from_B2b), boost::end(up_from_B2b)) {}
};

// list of string node names we expect from traversing DOWN to
// example_tree()'s B2b node
const char* down_to_B2b[] =
{
    "root",
    "B",
    "B2",
    "B2b"
};

// specialize RootExpected<DOWN> with the expected strings
template <>
struct RootExpected<LLTreeIter::DOWN>: public Expected
{
    RootExpected(): Expected(boost::begin(down_to_B2b), boost::end(down_to_B2b)) {}
};

/*****************************************************************************
*   Generic tree test functions
*****************************************************************************/
template<LLTreeIter::RootIter DISCRIM, class NODE, typename PARENTFUNC>
bool LLTreeRootIter_test(const std::string& itername, const std::string& nodename,
                         const typename LLPtrTo<NODE>::type& node,
                         PARENTFUNC parentfunc)
{
    std::ostringstream desc;
    desc << itername << '<' << nodename << "> from " << node->name();
    if (!  verify(desc.str(),
                  boost::make_iterator_range(LLTreeRootIter<DISCRIM, NODE>(node, parentfunc),
                                             LLTreeRootIter<DISCRIM, NODE>()),
                  RootExpected<DISCRIM>()))
        return false;
//  std::cout << desc.str() << '\n';
    // Try instantiating an iterator with NULL (that is, a default-constructed
    // node pointer). This test is less about "did we iterate once?" than "did
    // we avoid blowing up?"
    for (LLTreeRootIter<DISCRIM, NODE> hri = LLTreeRootIter<DISCRIM, NODE>(typename LLPtrTo<NODE>::type(), parentfunc), hrend;
         hri != hrend; /* ++hri */) // incrementing is moot, and MSVC complains
    {
//      std::cout << nodename << '(' << (*hri)->name() << ")\n";
        std::cout << itername << '<' << nodename << ">(NULL)\n";
        return false;
    }
    return true;
}

template<class NODE, typename CHILDITER, typename PARENTFUNC, typename CHILDFUNC>
bool LLTreeUpIter_test(const std::string& nodename, PARENTFUNC parentfunc, CHILDFUNC childfunc)
{
    bool success = true;
    typedef typename LLPtrTo<NODE>::type ptr_type;
    ptr_type root(example_tree<NODE>());
    Cleanup<ptr_type> cleanup(root);
    ptr_type B2b(get_B2b<NODE, CHILDITER>(root, childfunc));
    if (! LLTreeRootIter_test<LLTreeIter::UP,   NODE>("LLTreeUpIter",   nodename, B2b, parentfunc))
        success = false;
    if (! LLTreeRootIter_test<LLTreeIter::DOWN, NODE>("LLTreeDownIter", nodename, B2b, parentfunc))
        success = false;
    return success;
}

template <LLTreeIter::WalkIter DISCRIM, class NODE, typename CHILDITER,
          typename CHILDBEGINFUNC, typename CHILDENDFUNC>
bool LLTreeWalkIter_test(const std::string& itername, const std::string& nodename,
                      CHILDBEGINFUNC childbegin, CHILDENDFUNC childend)
{
    typename LLPtrTo<NODE>::type root(example_tree<NODE>());
    Cleanup<typename LLPtrTo<NODE>::type> cleanup(root);
    std::ostringstream desc;
    desc << itername << '<' << nodename << "> from " << root->name();
    if (!  verify(desc.str(),
                  boost::make_iterator_range(LLTreeWalkIter<DISCRIM, NODE, CHILDITER>(root,
                                                                                      childbegin,
                                                                                      childend),
                                             LLTreeWalkIter<DISCRIM, NODE, CHILDITER>()),
                  WalkExpected<DISCRIM>()))
        return false;
    // Try instantiating an iterator with NULL (that is, a default-constructed
    // node pointer). This test is less about "did we iterate once?" than "did
    // we avoid blowing up?"
    for (LLTreeWalkIter<DISCRIM, NODE, CHILDITER> twi = LLTreeWalkIter<DISCRIM, NODE, CHILDITER>(typename LLPtrTo<NODE>::type(),
                                                      childbegin,
                                                      childend),
                                                  twend;
         twi != twend; /* ++twi */) // incrementing is moot, and MSVC complains
    {
        std::cout << itername << '<' << nodename << ">(NULL)\n";
        return false;
    }             
    return true;
}

template <class NODE, typename CHILDITER,
          typename PARENTFUNC, typename CHILDBEGINFUNC, typename CHILDENDFUNC>
bool LLTreeIter_tests(const std::string& nodename,
                      PARENTFUNC parentfunc, CHILDBEGINFUNC childbegin, CHILDENDFUNC childend)
{
    bool success = true;
    if (! LLTreeUpIter_test<NODE, CHILDITER>(nodename, parentfunc, childbegin))
        success = false;
/*==========================================================================*|
    LLTreeIter_test<NODE, LLTreeDFSIter<NODE, CHILDITER> >("LLTreeDFSIter", nodename,
                                                           childbegin, childend);
    LLTreeIter_test<NODE, LLTreeDFSPostIter<NODE, CHILDITER> >("LLTreeDFSPostIter", nodename,
                                                               childbegin, childend);
    LLTreeIter_test<NODE, LLTreeBFSIter<NODE, CHILDITER> >("LLTreeBFSIter", nodename,
                                                           childbegin, childend);
|*==========================================================================*/
    if (! LLTreeWalkIter_test<LLTreeIter::DFS_PRE,  NODE, CHILDITER>("LLTreeDFSIter", nodename,
                                                                     childbegin, childend))
        success = false;
    if (! LLTreeWalkIter_test<LLTreeIter::DFS_POST, NODE, CHILDITER>("LLTreeDFSPostIter", nodename,
                                                                     childbegin, childend))
        success = false;
    if (! LLTreeWalkIter_test<LLTreeIter::BFS,      NODE, CHILDITER>("LLTreeBFSIter", nodename,
                                                                     childbegin, childend))
        success = false;
    return success;
}

namespace tut
{
    template<> template<>
    void iter_object::test<3>()
    {
//      set_test_name("LLTreeIter tests");
        ensure(LLTreeIter_tests<TreeNode, TreeNode::child_iterator>
                               ("TreeNode",
                                boost::bind(&TreeNode::getParent, _1),
                                boost::bind(&TreeNode::child_begin, _1),
                                boost::bind(&TreeNode::child_end, _1)));
        ensure(LLTreeIter_tests<PlainTree, LLLinkedIter<PlainTree> >
                               ("PlainTree",
                                boost::bind(&PlainTree::mParent, _1),
                                PlainTree_child_begin,
                                PlainTree_child_end));
    }

    template<> template<>
    void iter_object::test<4>()
    {
//      set_test_name("getRootRange() tests");
        // This test function illustrates the looping techniques described in the
        // comments for the getRootRange() free function, the
        // EnhancedTreeNode::root_range template and the
        // EnhancedTreeNode::getRootRange() method. Obviously the BOOST_FOREACH()
        // forms are more succinct.
        TreeNodePtr tnroot(example_tree<TreeNode>());
        TreeNodePtr tnB2b(get_B2b<TreeNode, TreeNode::child_iterator>
                          (tnroot, boost::bind(&TreeNode::child_begin, _1)));
    
        std::string desc1("BOOST_FOREACH(TreeNodePr, getRootRange<LLTreeIter::UP>(tnB2b))");
//      std::cout << desc1 << "\n";
        // Although we've commented out the output statement, ensure that the
        // loop construct is still valid, as promised by the getRootRange()
        // documentation.
        BOOST_FOREACH(TreeNodePtr node, getRootRange<LLTreeIter::UP>(tnB2b))
        {
//          std::cout << node->name() << '\n';
        }
        ensure(desc1,
               verify(desc1, getRootRange<LLTreeIter::UP>(tnB2b), RootExpected<LLTreeIter::UP>()));

        EnhancedTreeNodePtr etnroot(example_tree<EnhancedTreeNode>());
        EnhancedTreeNodePtr etnB2b(get_B2b<EnhancedTreeNode, EnhancedTreeNode::child_iterator>
                                   (etnroot, boost::bind(&EnhancedTreeNode::child_begin, _1)));

//      std::cout << "EnhancedTreeNode::root_range<LLTreeIter::DOWN>::type range =\n"
//                << "    etnB2b->getRootRange<LLTreeIter::DOWN>();\n"
//                << "for (EnhancedTreeNode::root_range<LLTreeIter::DOWN>::type::iterator ri = range.begin();\n"
//                << "     ri != range.end(); ++ri)\n";
        EnhancedTreeNode::root_range<LLTreeIter::DOWN>::type range =
            etnB2b->getRootRange<LLTreeIter::DOWN>();
        for (EnhancedTreeNode::root_range<LLTreeIter::DOWN>::type::iterator ri = range.begin();
             ri != range.end(); ++ri)
        {
//          std::cout << (*ri)->name() << '\n';
        }

        std::string desc2("BOOST_FOREACH(EnhancedTreeNodePtr node, etnB2b->getRootRange<LLTreeIter::UP>())");
//      std::cout << desc2 << '\n';
        BOOST_FOREACH(EnhancedTreeNodePtr node, etnB2b->getRootRange<LLTreeIter::UP>())
        {
//          std::cout << node->name() << '\n';
        }
        ensure(desc2,
               verify(desc2, etnB2b->getRootRange<LLTreeIter::UP>(), RootExpected<LLTreeIter::UP>()));
    }

    template<> template<>
    void iter_object::test<5>()
    {
//      set_test_name("getWalkRange() tests");
        // This test function doesn't illustrate the looping permutations for
        // getWalkRange(); see getRootRange_tests() for such examples. This
        // function simply verifies that they all work.

        // TreeNode, using helper function
        TreeNodePtr tnroot(example_tree<TreeNode>());
        std::string desc_tnpre("getWalkRange<LLTreeIter::DFS_PRE>(tnroot)");
        ensure(desc_tnpre,
               verify(desc_tnpre,
                      getWalkRange<LLTreeIter::DFS_PRE>(tnroot),
                      WalkExpected<LLTreeIter::DFS_PRE>()));
        std::string desc_tnpost("getWalkRange<LLTreeIter::DFS_POST>(tnroot)");
        ensure(desc_tnpost,
               verify(desc_tnpost,
                      getWalkRange<LLTreeIter::DFS_POST>(tnroot),
                      WalkExpected<LLTreeIter::DFS_POST>()));
        std::string desc_tnb("getWalkRange<LLTreeIter::BFS>(tnroot)");
        ensure(desc_tnb,
               verify(desc_tnb,
                      getWalkRange<LLTreeIter::BFS>(tnroot),
                      WalkExpected<LLTreeIter::BFS>()));

        // EnhancedTreeNode, using method
        EnhancedTreeNodePtr etnroot(example_tree<EnhancedTreeNode>());
        std::string desc_etnpre("etnroot->getWalkRange<LLTreeIter::DFS_PRE>()");
        ensure(desc_etnpre,
               verify(desc_etnpre,
                      etnroot->getWalkRange<LLTreeIter::DFS_PRE>(),
                      WalkExpected<LLTreeIter::DFS_PRE>()));
        std::string desc_etnpost("etnroot->getWalkRange<LLTreeIter::DFS_POST>()");
        ensure(desc_etnpost,
               verify(desc_etnpost,
                      etnroot->getWalkRange<LLTreeIter::DFS_POST>(),
                      WalkExpected<LLTreeIter::DFS_POST>()));
        std::string desc_etnb("etnroot->getWalkRange<LLTreeIter::BFS>()");
        ensure(desc_etnb,
               verify(desc_etnb,
                      etnroot->getWalkRange<LLTreeIter::BFS>(),
                      WalkExpected<LLTreeIter::BFS>()));

        // PlainTree, using helper function
        PlainTree* ptroot(example_tree<PlainTree>());
        Cleanup<PlainTree*> cleanup(ptroot);
        std::string desc_ptpre("getWalkRange<LLTreeIter::DFS_PRE>(ptroot)");
        ensure(desc_ptpre,
               verify(desc_ptpre,
                      getWalkRange<LLTreeIter::DFS_PRE>(ptroot),
                      WalkExpected<LLTreeIter::DFS_PRE>()));
        std::string desc_ptpost("getWalkRange<LLTreeIter::DFS_POST>(ptroot)");
        ensure(desc_ptpost,
               verify(desc_ptpost,
                      getWalkRange<LLTreeIter::DFS_POST>(ptroot),
                      WalkExpected<LLTreeIter::DFS_POST>()));
        std::string desc_ptb("getWalkRange<LLTreeIter::BFS>(ptroot)");
        ensure(desc_ptb,
               verify(desc_ptb,
                      getWalkRange<LLTreeIter::BFS>(ptroot),
                      WalkExpected<LLTreeIter::BFS>()));
    }
} // tut
