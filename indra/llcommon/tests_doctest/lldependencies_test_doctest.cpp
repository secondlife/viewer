/**
 * @file lldependencies_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for LL dependencies
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

// ---------------------------------------------------------------------------
// Auto-generated from lldependencies_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include <iostream>
#include <string>
#include <boost/assign/list_of.hpp>
#include "linden_common.h"
#include "../lldependencies.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("lldependencies_test::deps_object_test_1")
    {
        DOCTEST_FAIL("TODO: convert lldependencies_test.cpp::deps_object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void deps_object::test<1>()
        //     {
        //         StringDeps deps;
        //         StringList empty;
        //         // The quick brown fox jumps over the lazy yellow dog.
        //         // (note, "The" and "the" are distinct, else this test wouldn't work)
        //         deps.add("lazy");
        //         ensure_equals(sorted_keys(deps), make<StringList>(list_of("lazy")));
        //         deps.add("jumps");
        //         ensure("found lazy", deps.get("lazy"));
        //         ensure("not found dog.", ! deps.get("dog."));
        //         // NOTE: Maybe it's overkill to test each of these intermediate
        //         // results before all the interdependencies have been specified. My
        //         // thought is simply that if the order changes, I'd like to know why.
        //         // A change to the implementation of boost::topological_sort() would
        //         // be an acceptable reason, and you can simply update the expected
        //         // test output.
        //         ensure_equals(sorted_keys(deps), make<StringList>(list_of("lazy")("jumps")));
        //         deps.add("The", 0, empty, list_of("fox")("dog."));
        //         // Test key accessors
        //         ensure("empty before deps for missing key", is_empty(deps.get_before_range("bogus")));
        //         ensure("empty before deps for jumps", is_empty(deps.get_before_range("jumps")));
        //         ensure_equals(instance_from_range< std::set<std::string> >(deps.get_before_range("The")),
        //                       make< std::set<std::string> >(list_of("dog.")("fox")));
        //         // resume building dependencies
        //         ensure_equals(sorted_keys(deps), make<StringList>(list_of("lazy")("jumps")("The")));
        //         deps.add("the", 0, list_of("The"));
        //         ensure_equals(sorted_keys(deps), make<StringList>(list_of("lazy")("jumps")("The")("the")));
        //         deps.add("fox", 0, list_of("The"), list_of("jumps"));
        //         ensure_equals(sorted_keys(deps), make<StringList>(list_of("lazy")("The")("the")("fox")("jumps")));
        //         deps.add("the", 0, list_of("The")); // same, see if cache works
        //         ensure_equals(sorted_keys(deps), make<StringList>(list_of("lazy")("The")("the")("fox")("jumps")));
        //         deps.add("jumps", 0, empty, list_of("over")); // update jumps deps
        //         ensure_equals(sorted_keys(deps), make<StringList>(list_of("lazy")("The")("the")("fox")("jumps")));
        // /*==========================================================================*|
        //         // It drives me nuts that this test doesn't work in the test
        //         // framework, because -- for reasons unknown -- running the test
        //         // framework on Mac OS X 10.5 Leopard and Windows XP Pro, the catch
        //         // clause below doesn't catch the exception. Something about the TUT
        //         // test framework?!? The identical code works fine in a standalone
        //         // test program. Commenting out the test for now, in hopes that our
        //         // real builds will be able to catch Cycle exceptions...
        //         try
        //         {
        //             // We've already specified fox -> jumps and jumps -> over. Try an
        //             // impossible constraint.
        //             deps.add("over", 0, empty, list_of("fox"));
        //         }
        //         catch (const StringDeps::Cycle& e)
        //         {
        //             std::cout << "Cycle detected: " << e.what() << '\n';
        //             // It's legal to add() an impossible constraint because we don't
        //             // detect the cycle until sort(). So sort() can't know the minimum set
        //             // of nodes to remove to make the StringDeps object valid again.
        //             // Therefore we must break the cycle by hand.
        //             deps.remove("over");
        //         }
        // |*==========================================================================*/
        //         deps.add("dog.", 0, list_of("yellow")("lazy"));
        //         ensure_equals(instance_from_range< std::set<std::string> >(deps.get_after_range("dog.")),
        //                       make< std::set<std::string> >(list_of("lazy")("yellow")));
        //         ensure_equals(sorted_keys(deps), make<StringList>(list_of("lazy")("The")("the")("fox")("jumps")("dog.")));
        //         deps.add("quick", 0, list_of("The"), list_of("fox")("brown"));
        //         ensure_equals(sorted_keys(deps), make<StringList>(list_of("lazy")("The")("the")("quick")("fox")("jumps")("dog.")));
        //         deps.add("over", 0, list_of("jumps"), list_of("yellow")("the"));
        //         ensure_equals(sorted_keys(deps), make<StringList>(list_of("lazy")("The")("quick")("fox")("jumps")("over")("the")("dog.")));
        //         deps.add("yellow", 0, list_of("the"), list_of("lazy"));
        //         ensure_equals(sorted_keys(deps), make<StringList>(list_of("The")("quick")("fox")("jumps")("over")("the")("yellow")("lazy")("dog.")));
        //         deps.add("brown");
        //         // By now the dependencies are pretty well in place. A change to THIS
        //         // order should be viewed with suspicion.
        //         ensure_equals(sorted_keys(deps), make<StringList>(list_of("The")("quick")("brown")("fox")("jumps")("over")("the")("yellow")("lazy")("dog.")));

        //         StringList keys(make<StringList>(list_of("The")("brown")("dog.")("fox")("jumps")("lazy")("over")("quick")("the")("yellow")));
        //         ensure_equals(instance_from_range<StringList>(deps.get_key_range()), keys);
        // #if (! defined(__GNUC__)) || (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ > 3)
        //         // This is the succinct way, works on modern compilers
        //         ensure_equals(instance_from_range<StringList>(make_transform_range(deps.get_range(), extract_key)), keys);
        // #else   // gcc 3.3
        //         StringDeps::range got_range(deps.get_range());
        //         StringDeps::iterator kni = got_range.begin(), knend = got_range.end();
        //         StringList::iterator ki = keys.begin(), kend = keys.end();
        //         for ( ; kni != knend && ki != kend; ++kni, ++ki)
        //         {
        //             ensure_equals(kni->first, *ki);
        //         }
        //         ensure("get_range() returns proper length", kni == knend && ki == kend);
        // #endif  // gcc 3.3
        //         // blow off get_node_range() because they're all LLDependenciesEmpty instances
        //     }
    }

    TUT_CASE("lldependencies_test::deps_object_test_2")
    {
        DOCTEST_FAIL("TODO: convert lldependencies_test.cpp::deps_object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void deps_object::test<2>()
        //     {
        //         typedef LLDependencies<std::string, int> NameIndexDeps;
        //         NameIndexDeps nideps;
        //         const NameIndexDeps& const_nideps(nideps);
        //         nideps.add("def", 2, list_of("ghi"));
        //         nideps.add("ghi", 3);
        //         nideps.add("abc", 1, list_of("def"));
        //         NameIndexDeps::range range(nideps.get_range());
        //         ensure_equals(range.begin()->first, "abc");
        //         ensure_equals(range.begin()->second, 1);
        //         range.begin()->second = 0;
        //         range.begin()->second = 1;
        //         NameIndexDeps::const_range const_range(const_nideps.get_range());
        //         NameIndexDeps::const_iterator const_iterator(const_range.begin());
        //         ++const_iterator;
        //         ensure_equals(const_iterator->first, "def");
        //         ensure_equals(const_iterator->second, 2);
        // //        NameIndexDeps::node_range node_range(nideps.get_node_range());
        // //        ensure_equals(instance_from_range<std::vector<int> >(node_range), make< std::vector<int> >(list_of(1)(2)(3)));
        // //        *node_range.begin() = 0;
        // //        *node_range.begin() = 1;
        //         NameIndexDeps::const_node_range const_node_range(const_nideps.get_node_range());
        //         ensure_equals(instance_from_range<std::vector<int> >(const_node_range), make< std::vector<int> >(list_of(1)(2)(3)));
        //         NameIndexDeps::const_key_range const_key_range(const_nideps.get_key_range());
        //         ensure_equals(instance_from_range<StringList>(const_key_range), make<StringList>(list_of("abc")("def")("ghi")));
        //         NameIndexDeps::sorted_range sorted(const_nideps.sort());
        //         NameIndexDeps::sorted_iterator sortiter(sorted.begin());
        //         ensure_equals(sortiter->first, "ghi");
        //         ensure_equals(sortiter->second, 3);

        //         // test all iterator-flavored versions of get_after_range()
        //         StringList def(make<StringList>(list_of("def")));
        //         ensure("empty abc before list", is_empty(nideps.get_before_range(nideps.get_range().begin())));
        //         ensure_equals(instance_from_range<StringList>(nideps.get_after_range(nideps.get_range().begin())),
        //                       def);
        //         ensure_equals(instance_from_range<StringList>(const_nideps.get_after_range(const_nideps.get_range().begin())),
        //                       def);
        // //        ensure_equals(instance_from_range<StringList>(nideps.get_after_range(nideps.get_node_range().begin())),
        // //                      def);
        //         ensure_equals(instance_from_range<StringList>(const_nideps.get_after_range(const_nideps.get_node_range().begin())),
        //                       def);
        //         ensure_equals(instance_from_range<StringList>(nideps.get_after_range(nideps.get_key_range().begin())),
        //                       def);
        //         // advance from "ghi" to "def", which must come after "ghi"
        //         ++sortiter;
        //         ensure_equals(instance_from_range<StringList>(const_nideps.get_after_range(sortiter)),
        //                       make<StringList>(list_of("ghi")));
        //     }
    }

}

