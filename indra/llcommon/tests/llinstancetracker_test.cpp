/**
 * @file   llinstancetracker_test.cpp
 * @author Nat Goodspeed
 * @date   2009-11-10
 * @brief  Test for llinstancetracker.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
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
// std headers
// external library headers
#include <boost/scoped_ptr.hpp>
// other Linden headers
#include "../test/lltut.h"

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
};

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llinstancetracker_data
    {
    };
    typedef test_group<llinstancetracker_data> llinstancetracker_group;
    typedef llinstancetracker_group::object object;
    llinstancetracker_group llinstancetrackergrp("llinstancetracker");

    template<> template<>
    void object::test<1>()
    {
        ensure_equals(Keyed::instanceCount(), 0);
        {
            Keyed one("one");
            ensure_equals(Keyed::instanceCount(), 1);
            Keyed* found = Keyed::getInstance("one");
            ensure("couldn't find stack Keyed", found);
            ensure_equals("found wrong Keyed instance", found, &one);
            {
                boost::scoped_ptr<Keyed> two(new Keyed("two"));
                ensure_equals(Keyed::instanceCount(), 2);
                Keyed* found = Keyed::getInstance("two");
                ensure("couldn't find heap Keyed", found);
                ensure_equals("found wrong Keyed instance", found, two.get());
            }
            ensure_equals(Keyed::instanceCount(), 1);
        }
        Keyed* found = Keyed::getInstance("one");
        ensure("Keyed key lives too long", ! found);
        ensure_equals(Keyed::instanceCount(), 0);
    }

    template<> template<>
    void object::test<2>()
    {
        ensure_equals(Unkeyed::instanceCount(), 0);
        {
            Unkeyed one;
            ensure_equals(Unkeyed::instanceCount(), 1);
            Unkeyed* found = Unkeyed::getInstance(&one);
            ensure_equals(found, &one);
            {
                boost::scoped_ptr<Unkeyed> two(new Unkeyed);
                ensure_equals(Unkeyed::instanceCount(), 2);
                Unkeyed* found = Unkeyed::getInstance(two.get());
                ensure_equals(found, two.get());
            }
            ensure_equals(Unkeyed::instanceCount(), 1);
        }
        ensure_equals(Unkeyed::instanceCount(), 0);
    }

    template<> template<>
    void object::test<3>()
    {
        Keyed one("one"), two("two"), three("three");
        // We don't want to rely on the underlying container delivering keys
        // in any particular order. That allows us the flexibility to
        // reimplement LLInstanceTracker using, say, a hash map instead of a
        // std::map. We DO insist that every key appear exactly once.
        typedef std::vector<std::string> StringVector;
        StringVector keys(Keyed::beginKeys(), Keyed::endKeys());
        std::sort(keys.begin(), keys.end());
        StringVector::const_iterator ki(keys.begin());
        ensure_equals(*ki++, "one");
        ensure_equals(*ki++, "three");
        ensure_equals(*ki++, "two");
        // Use ensure() here because ensure_equals would want to display
        // mismatched values, and frankly that wouldn't help much.
        ensure("didn't reach end", ki == keys.end());

        // Use a somewhat different approach to order independence with
        // beginInstances(): explicitly capture the instances we know in a
        // set, and delete them as we iterate through.
        typedef std::set<Keyed*> InstanceSet;
        InstanceSet instances;
        instances.insert(&one);
        instances.insert(&two);
        instances.insert(&three);
        for (Keyed::instance_iter ii(Keyed::beginInstances()), iend(Keyed::endInstances());
             ii != iend; ++ii)
        {
            Keyed& ref = *ii;
            ensure_equals("spurious instance", instances.erase(&ref), 1);
        }
        ensure_equals("unreported instance", instances.size(), 0);
    }

    template<> template<>
    void object::test<4>()
    {
        Unkeyed one, two, three;
        typedef std::set<Unkeyed*> KeySet;
        KeySet keys;
        keys.insert(&one);
        keys.insert(&two);
        keys.insert(&three);
	{
		Unkeyed::LLInstanceTrackerScopedGuard guard;
		for (Unkeyed::key_iter ki(guard.beginKeys()), kend(guard.endKeys());
		     ki != kend; ++ki)
		{
			ensure_equals("spurious key", keys.erase(*ki), 1);
		}
	}
        ensure_equals("unreported key", keys.size(), 0);

        KeySet instances;
        instances.insert(&one);
        instances.insert(&two);
        instances.insert(&three);
	{
		Unkeyed::LLInstanceTrackerScopedGuard guard;
		for (Unkeyed::instance_iter ii(guard.beginInstances()), iend(guard.endInstances());
		     ii != iend; ++ii)
		{
			Unkeyed& ref = *ii;
			ensure_equals("spurious instance", instances.erase(&ref), 1);
		}
	}
        ensure_equals("unreported instance", instances.size(), 0);
    }
} // namespace tut
