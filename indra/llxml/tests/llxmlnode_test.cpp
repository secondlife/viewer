/**
 * @file llxmlnode_test.cpp
 * @date  November 2026
 * @brief LLXMLNode unit tests
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

#include "linden_common.h"

#include "../llxmlnode.h"

#include "../test/lltut.h"

#include <vector>
#include <string>

namespace tut
{
    struct xmlnode_data
    {
        xmlnode_data()
        {
        }

        // Build a root node with several named children, added in a
        // specific, intentionally non-alphabetical, non-pointer-value
        // order, to make sure that order isn't accidentally preserved by
        // some unrelated coincidence (e.g. insertion order happening to
        // match address order).
        static LLXMLNodePtr makeTestTree()
        {
            LLXMLNodePtr root = new LLXMLNode("root", false);

            const char* names[] = { "alpha", "Great", "Alexandia", "Library", "Ptolemaic", "dynasty" };
            for (const char* name : names)
            {
                LLXMLNodePtr child = root->createChild(name, false);
                child->setStringValue(std::string(name));
            }

            return root;
        }

        // Collect the names of all direct children, in traversal order,
        // using the public getFirstChild()/getNextSibling() API -- this is
        // the only API that is documented/guaranteed to reflect actual
        // document order.
        static std::vector<std::string> childNamesInOrder(LLXMLNodePtr node)
        {
            std::vector<std::string> result;
            for (LLXMLNodePtr child = node->getFirstChild(); child.notNull();
                 child = child->getNextSibling())
            {
                result.push_back(child->getName()->mString);
            }
            return result;
        }
    };

    typedef test_group<xmlnode_data> xmlnode_test;
    typedef xmlnode_test::object xmlnode_object;
    xmlnode_test tut_xmlnode("LLXMLNode");

    // deepCopy() must preserve the original child ordering
    template<> template<>
    void xmlnode_object::test<1>()
    {
        LLXMLNodePtr root = makeTestTree();

        std::vector<std::string> original_order = childNamesInOrder(root);

        LLXMLNodePtr copy = root->deepCopy();

        std::vector<std::string> copy_order = childNamesInOrder(copy);

        ensure_equals("deepCopy() preserves child count",
                      copy_order.size(), original_order.size());
        ensure("deepCopy() preserves child order",
               copy_order == original_order);
    }

    // deepCopy() must preserve child ordering for nested (grandchild) trees too
    template<> template<>
    void xmlnode_object::test<2>()
    {
        LLXMLNodePtr root = makeTestTree();

        // Attach a nested child tree, in a deliberately scrambled order, to
        // one of the top-level children.
        LLXMLNodePtr nested_parent;
        root->getChild("alpha", nested_parent, false);
        ensure("found 'alpha' child", nested_parent.notNull());

        const char* nested_names[] = { "zulu", "yankee", "xray", "whiskey" };
        for (const char* name : nested_names)
        {
            LLXMLNodePtr grandchild = nested_parent->createChild(name, false);
            grandchild->setStringValue(std::string(name));
        }

        std::vector<std::string> original_order = childNamesInOrder(nested_parent);

        LLXMLNodePtr copy = root->deepCopy();

        LLXMLNodePtr copied_parent;
        copy->getChild("alpha", copied_parent, false);
        ensure("found copied 'alpha' child", copied_parent.notNull());

        std::vector<std::string> copy_order = childNamesInOrder(copied_parent);

        ensure_equals("deepCopy() preserves nested child count",
                      copy_order.size(), original_order.size());
        ensure("deepCopy() preserves nested child order",
               copy_order == original_order);
    }

    // deepCopy() must not share any child node pointers with the original --
    // it is supposed to be a genuine deep copy.
    template<> template<>
    void xmlnode_object::test<3>()
    {
        LLXMLNodePtr root = makeTestTree();
        LLXMLNodePtr copy = root->deepCopy();

        LLXMLNodePtr orig_child = root->getFirstChild();
        LLXMLNodePtr copy_child = copy->getFirstChild();

        ensure("original has children", orig_child.notNull());
        ensure("copy has children", copy_child.notNull());
        ensure("deepCopy() children are distinct objects",
               orig_child.get() != copy_child.get());

        // Mutating the copy must not affect the original.
        copy_child->setStringValue(std::string("mutated"));
        ensure("mutating copy doesn't affect original",
               orig_child->getValue() != copy_child->getValue());
    }

    // deepCopy() must preserve attribute values (order independent, since
    // LLXMLAttribList is explicitly sorted by name via CompareAttributes).
    template<> template<>
    void xmlnode_object::test<4>()
    {
        LLXMLNodePtr root = new LLXMLNode("root", false);
        root->createChild("zzz_attr", true)->setStringValue("z_value");
        root->createChild("aaa_attr", true)->setStringValue("a_value");

        LLXMLNodePtr copy = root->deepCopy();

        std::string value;
        ensure("copy has 'zzz_attr'", copy->getAttributeString("zzz_attr", value));
        ensure_equals("copy 'zzz_attr' value", value, std::string("z_value"));

        ensure("copy has 'aaa_attr'", copy->getAttributeString("aaa_attr", value));
        ensure_equals("copy 'aaa_attr' value", value, std::string("a_value"));
    }
}
