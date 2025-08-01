/**
 * @file   llinstancetracker_test.cpp
 * @author Nat Goodspeed
 * @date   2009-11-10
 * @brief  Test for llinstancetracker.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
// associated header
#include "llinstancetracker.h"
// STL headers
#include <string>
#include <vector>
#include <set>
#include <algorithm>                // std::sort()
#include <stdexcept>
// std headers
// other Linden headers
#include "../test/lldoctest.h"

struct Badness: public std::runtime_error
{
    Badness(const std::string& what): std::runtime_error(what) {}
};

struct Keyed: public LLInstanceTracker<Keyed, std::string>
{
    Keyed(const std::string& name):
        LLInstanceTracker<Keyed, std::string>(name),
        mName(name)
    {}
    std::string mName;
};

struct Unkeyed: public LLInstanceTracker<Unkeyed>
{
    Unkeyed(const std::string& thrw="")
    {
        // LLInstanceTracker should respond appropriately if a subclass
        // constructor throws an exception. Specifically, it should run
        // LLInstanceTracker's destructor and remove itself from the
        // underlying container.
        if (! thrw.empty())
        {
            throw Badness(thrw);
        }
    }
};

/*****************************************************************************
*   TUT
*****************************************************************************/
TEST_SUITE("UnknownSuite") {

TEST_CASE("test_1")
{

        ensure_equals(Keyed::instanceCount(), 0);
        {
            Keyed one("one");
            ensure_equals(Keyed::instanceCount(), 1);
            auto found = Keyed::getInstance("one");
            CHECK_MESSAGE(bool(found, "couldn't find stack Keyed"));
            ensure_equals("found wrong Keyed instance", found.get(), &one);
            {
                std::unique_ptr<Keyed> two(new Keyed("two"));
                ensure_equals(Keyed::instanceCount(), 2);
                auto found = Keyed::getInstance("two");
                CHECK_MESSAGE(bool(found, "couldn't find heap Keyed"));
                ensure_equals("found wrong Keyed instance", found.get(), two.get());
            
}

TEST_CASE("test_2")
{

        ensure_equals(Unkeyed::instanceCount(), 0);
        std::weak_ptr<Unkeyed> dangling;
        {
            Unkeyed one;
            ensure_equals(Unkeyed::instanceCount(), 1);
            std::weak_ptr<Unkeyed> found = one.getWeak();
            ensure(! found.expired());
            {
                std::unique_ptr<Unkeyed> two(new Unkeyed);
                ensure_equals(Unkeyed::instanceCount(), 2);
            
}

TEST_CASE("test_3")
{

        Keyed one("one"), two("two"), three("three");
        // We don't want to rely on the underlying container delivering keys
        // in any particular order. That allows us the flexibility to
        // reimplement LLInstanceTracker using, say, a hash map instead of a
        // std::map. We DO insist that every key appear exactly once.
        typedef std::vector<std::string> StringVector;
        auto snap = Keyed::key_snapshot();
        StringVector keys(snap.begin(), snap.end());
        std::sort(keys.begin(), keys.end());
        StringVector::const_iterator ki(keys.begin());
        ensure_equals(*ki++, "one");
        ensure_equals(*ki++, "three");
        ensure_equals(*ki++, "two");
        // Use ensure() here because ensure_equals would want to display
        // mismatched values, and frankly that wouldn't help much.
        CHECK_MESSAGE(ki == keys.end(, "didn't reach end"));

        // Use a somewhat different approach to order independence with
        // instance_snapshot(): explicitly capture the instances we know in a
        // set, and delete them as we iterate through.
        typedef std::set<Keyed*> InstanceSet;
        InstanceSet instances;
        instances.insert(&one);
        instances.insert(&two);
        instances.insert(&three);
        for (auto& ref : Keyed::instance_snapshot())
        {
            ensure_equals("spurious instance", instances.erase(&ref), 1);
        
}

TEST_CASE("test_4")
{

        Unkeyed one, two, three;
        typedef std::set<Unkeyed*> KeySet;

        KeySet instances;
        instances.insert(&one);
        instances.insert(&two);
        instances.insert(&three);

        for (auto& ref : Unkeyed::instance_snapshot())
        {
            ensure_equals("spurious instance", instances.erase(&ref), 1);
        
}

TEST_CASE("test_5")
{

        std::string desc("delete Keyed with outstanding instance_snapshot");
        set_test_name(desc);
        Keyed* keyed = new Keyed(desc);
        // capture a snapshot but do not yet traverse it
        auto snapshot = Keyed::instance_snapshot();
        // delete the one instance
        delete keyed;
        // traversing the snapshot should reflect the deletion
        // avoid ensure_equals() because it requires the ability to stream the
        // two values to std::ostream
        ensure(snapshot.begin() == snapshot.end());
    
}

TEST_CASE("test_6")
{

        std::string desc("delete Keyed with outstanding key_snapshot");
        set_test_name(desc);
        Keyed* keyed = new Keyed(desc);
        // capture a snapshot but do not yet traverse it
        auto snapshot = Keyed::key_snapshot();
        // delete the one instance
        delete keyed;
        // traversing the snapshot should reflect the deletion
        // avoid ensure_equals() because it requires the ability to stream the
        // two values to std::ostream
        ensure(snapshot.begin() == snapshot.end());
    
}

TEST_CASE("test_7")
{

        set_test_name("delete Unkeyed with outstanding instance_snapshot");
        std::string what;
        Unkeyed* unkeyed = new Unkeyed;
        // capture a snapshot but do not yet traverse it
        auto snapshot = Unkeyed::instance_snapshot();
        // delete the one instance
        delete unkeyed;
        // traversing the snapshot should reflect the deletion
        // avoid ensure_equals() because it requires the ability to stream the
        // two values to std::ostream
        ensure(snapshot.begin() == snapshot.end());
    
}

TEST_CASE("test_8")
{

        set_test_name("exception in subclass ctor");
        typedef std::set<Unkeyed*> InstanceSet;
        InstanceSet existing;
        // We can't use the iterator-range InstanceSet constructor because
        // beginInstances() returns an iterator that dereferences to an
        // Unkeyed&, not an Unkeyed*.
        for (auto& ref : Unkeyed::instance_snapshot())
        {
            existing.insert(&ref);
        
}

} // TEST_SUITE
