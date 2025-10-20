/**
 * @file llinstancetracker_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for LL instancetracker
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
// Auto-generated from llinstancetracker_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "llinstancetracker.h"
#include <string>
#include <vector>
#include <set>
#include <stdexcept>

TUT_SUITE("llcommon")
{
    TUT_CASE("llinstancetracker_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert llinstancetracker_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         ensure_equals(Keyed::instanceCount(), 0);
        //         {
        //             Keyed one("one");
        //             ensure_equals(Keyed::instanceCount(), 1);
        //             auto found = Keyed::getInstance("one");
        //             ensure("couldn't find stack Keyed", bool(found));
        //             ensure_equals("found wrong Keyed instance", found.get(), &one);
        //             {
        //                 std::unique_ptr<Keyed> two(new Keyed("two"));
        //                 ensure_equals(Keyed::instanceCount(), 2);
        //                 auto found = Keyed::getInstance("two");
        //                 ensure("couldn't find heap Keyed", bool(found));
        //                 ensure_equals("found wrong Keyed instance", found.get(), two.get());
        //             }
        //             ensure_equals(Keyed::instanceCount(), 1);
        //         }
        //         auto found = Keyed::getInstance("one");
        //         ensure("Keyed key lives too long", ! found);
        //         ensure_equals(Keyed::instanceCount(), 0);
        //     }
    }

    TUT_CASE("llinstancetracker_test::object_test_2")
    {
        DOCTEST_FAIL("TODO: convert llinstancetracker_test.cpp::object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<2>()
        //     {
        //         ensure_equals(Unkeyed::instanceCount(), 0);
        //         std::weak_ptr<Unkeyed> dangling;
        //         {
        //             Unkeyed one;
        //             ensure_equals(Unkeyed::instanceCount(), 1);
        //             std::weak_ptr<Unkeyed> found = one.getWeak();
        //             ensure(! found.expired());
        //             {
        //                 std::unique_ptr<Unkeyed> two(new Unkeyed);
        //                 ensure_equals(Unkeyed::instanceCount(), 2);
        //             }
        //             ensure_equals(Unkeyed::instanceCount(), 1);
        //             // store a weak pointer to a temp Unkeyed instance
        //             dangling = found;
        //         } // make that instance vanish
        //         // check the now-invalid pointer to the destroyed instance
        //         ensure("weak_ptr<Unkeyed> failed to track destruction", dangling.expired());
        //         ensure_equals(Unkeyed::instanceCount(), 0);
        //     }
    }

    TUT_CASE("llinstancetracker_test::object_test_3")
    {
        DOCTEST_FAIL("TODO: convert llinstancetracker_test.cpp::object::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<3>()
        //     {
        //         Keyed one("one"), two("two"), three("three");
        //         // We don't want to rely on the underlying container delivering keys
        //         // in any particular order. That allows us the flexibility to
        //         // reimplement LLInstanceTracker using, say, a hash map instead of a
        //         // std::map. We DO insist that every key appear exactly once.
        //         typedef std::vector<std::string> StringVector;
        //         auto snap = Keyed::key_snapshot();
        //         StringVector keys(snap.begin(), snap.end());
        //         std::sort(keys.begin(), keys.end());
        //         StringVector::const_iterator ki(keys.begin());
        //         ensure_equals(*ki++, "one");
        //         ensure_equals(*ki++, "three");
        //         ensure_equals(*ki++, "two");
        //         // Use ensure() here because ensure_equals would want to display
        //         // mismatched values, and frankly that wouldn't help much.
        //         ensure("didn't reach end", ki == keys.end());

        //         // Use a somewhat different approach to order independence with
        //         // instance_snapshot(): explicitly capture the instances we know in a
        //         // set, and delete them as we iterate through.
        //         typedef std::set<Keyed*> InstanceSet;
        //         InstanceSet instances;
        //         instances.insert(&one);
        //         instances.insert(&two);
        //         instances.insert(&three);
        //         for (auto& ref : Keyed::instance_snapshot())
        //         {
        //             ensure_equals("spurious instance", instances.erase(&ref), 1);
        //         }
        //         ensure_equals("unreported instance", instances.size(), 0);
        //     }
    }

    TUT_CASE("llinstancetracker_test::object_test_4")
    {
        DOCTEST_FAIL("TODO: convert llinstancetracker_test.cpp::object::test<4> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<4>()
        //     {
        //         Unkeyed one, two, three;
        //         typedef std::set<Unkeyed*> KeySet;

        //         KeySet instances;
        //         instances.insert(&one);
        //         instances.insert(&two);
        //         instances.insert(&three);

        //         for (auto& ref : Unkeyed::instance_snapshot())
        //         {
        //             ensure_equals("spurious instance", instances.erase(&ref), 1);
        //         }

        //         ensure_equals("unreported instance", instances.size(), 0);
        //     }
    }

    TUT_CASE("llinstancetracker_test::object_test_5")
    {
        DOCTEST_FAIL("TODO: convert llinstancetracker_test.cpp::object::test<5> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<5>()
        //     {
        //         std::string desc("delete Keyed with outstanding instance_snapshot");
        //         set_test_name(desc);
        //         Keyed* keyed = new Keyed(desc);
        //         // capture a snapshot but do not yet traverse it
        //         auto snapshot = Keyed::instance_snapshot();
        //         // delete the one instance
        //         delete keyed;
        //         // traversing the snapshot should reflect the deletion
        //         // avoid ensure_equals() because it requires the ability to stream the
        //         // two values to std::ostream
        //         ensure(snapshot.begin() == snapshot.end());
        //     }
    }

    TUT_CASE("llinstancetracker_test::object_test_6")
    {
        DOCTEST_FAIL("TODO: convert llinstancetracker_test.cpp::object::test<6> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<6>()
        //     {
        //         std::string desc("delete Keyed with outstanding key_snapshot");
        //         set_test_name(desc);
        //         Keyed* keyed = new Keyed(desc);
        //         // capture a snapshot but do not yet traverse it
        //         auto snapshot = Keyed::key_snapshot();
        //         // delete the one instance
        //         delete keyed;
        //         // traversing the snapshot should reflect the deletion
        //         // avoid ensure_equals() because it requires the ability to stream the
        //         // two values to std::ostream
        //         ensure(snapshot.begin() == snapshot.end());
        //     }
    }

    TUT_CASE("llinstancetracker_test::object_test_7")
    {
        DOCTEST_FAIL("TODO: convert llinstancetracker_test.cpp::object::test<7> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<7>()
        //     {
        //         set_test_name("delete Unkeyed with outstanding instance_snapshot");
        //         std::string what;
        //         Unkeyed* unkeyed = new Unkeyed;
        //         // capture a snapshot but do not yet traverse it
        //         auto snapshot = Unkeyed::instance_snapshot();
        //         // delete the one instance
        //         delete unkeyed;
        //         // traversing the snapshot should reflect the deletion
        //         // avoid ensure_equals() because it requires the ability to stream the
        //         // two values to std::ostream
        //         ensure(snapshot.begin() == snapshot.end());
        //     }
    }

    TUT_CASE("llinstancetracker_test::object_test_8")
    {
        DOCTEST_FAIL("TODO: convert llinstancetracker_test.cpp::object::test<8> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<8>()
        //     {
        //         set_test_name("exception in subclass ctor");
        //         typedef std::set<Unkeyed*> InstanceSet;
        //         InstanceSet existing;
        //         // We can't use the iterator-range InstanceSet constructor because
        //         // beginInstances() returns an iterator that dereferences to an
        //         // Unkeyed&, not an Unkeyed*.
        //         for (auto& ref : Unkeyed::instance_snapshot())
        //         {
        //             existing.insert(&ref);
        //         }
        //         try
        //         {
        //             // We don't expect the assignment to take place because we expect
        //             // Unkeyed to respond to the non-empty string param by throwing.
        //             // We know the LLInstanceTracker base-class constructor will have
        //             // run before Unkeyed's constructor, therefore the new instance
        //             // will have added itself to the underlying set. The whole
        //             // question is, when Unkeyed's constructor throws, will
        //             // LLInstanceTracker's destructor remove it from the set? I
        //             // realize we're testing the C++ implementation more than
        //             // Unkeyed's implementation, but this seems an important point to
        //             // nail down.
        //             new Unkeyed("throw");
        //         }
        //         catch (const Badness&)
        //         {
        //         }
        //         // Ensure that every member of the new, updated set of Unkeyed
        //         // instances was also present in the original set. If that's not true,
        //         // it's because our new Unkeyed ended up in the updated set despite
        //         // its constructor exception.
        //         for (auto& ref : Unkeyed::instance_snapshot())
        //         {
        //             ensure("failed to remove instance", existing.find(&ref) != existing.end());
        //         }
        //     }
    }

}

