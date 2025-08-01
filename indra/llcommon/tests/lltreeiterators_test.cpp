/**
 * @file   lltreeiterators.cpp
 * @author Nat Goodspeed
 * @date   2008-08-20
 * @brief  Test of lltreeiterators.h
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

// associated header
#include "../lltreeiterators.h"
#include "../llpointer.h"

#include "../test/lldoctest.h"

/*****************************************************************************
*   tut test group
*****************************************************************************/
TEST_SUITE("LLTreeIterators") {

TEST_CASE("test_1")
{

//      set_test_name("LLLinkedIter -- non-refcounted class");
        PlainNode* last(new PlainNode("c"));
        PlainNode* second(new PlainNode("b", last));
        PlainNode* first(new PlainNode("a", second));
        Cleanup<PlainNode*> cleanup(first);
        static const char* cseq[] = { "a", "b", "c" 
}

TEST_CASE("test_2")
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

TEST_CASE("test_3")
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

TEST_CASE("test_4")
{

//      set_test_name("getRootRange() tests");
        // This test function illustrates the looping techniques described in the
        // comments for the getRootRange() free function, the
        // EnhancedTreeNode::root_range template and the
        // EnhancedTreeNode::getRootRange() method. Obviously the for()
        // forms are more succinct.
        TreeNodePtr tnroot(example_tree<TreeNode>());
        TreeNodePtr tnB2b(get_B2b<TreeNode, TreeNode::child_iterator>
                          (tnroot, boost::bind(&TreeNode::child_begin, _1)));

        std::string desc1("for (TreeNodePr : getRootRange<LLTreeIter::UP>(tnB2b))");
//      std::cout << desc1 << "\n";
        // Although we've commented out the output statement, ensure that the
        // loop construct is still valid, as promised by the getRootRange()
        // documentation.
        for (TreeNodePtr node : getRootRange<LLTreeIter::UP>(tnB2b))
        {
//          std::cout << node->name() << '\n';
        
}

TEST_CASE("test_5")
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

} // TEST_SUITE
