/**
 * @file   lldoubledispatch_tut.cpp
 * @author Nat Goodspeed
 * @date   2008-11-13
 * @brief  Test for lldoubledispatch.h
 *
 * This program tests the DoubleDispatch class, using a variation on the example
 * from Scott Meyers' "More Effective C++", Item 31.
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
// associated header
#include "lldoubledispatch.h"
// STL headers
// std headers
#include <iostream>
#include <memory>
#include <string>
#include <typeinfo>
// external library headers
// other Linden headers
#include "lltut.h"


/*---------------------------- Class hierarchy -----------------------------*/
// All objects are GameObjects.
class GameObject
{
public:
    GameObject(const std::string& name): mName(name) {}
    virtual ~GameObject() {}
    virtual std::string stringize() { return std::string(typeid(*this).name()) + " " + mName; }

protected:
    std::string mName;
};

// SpaceStation, Asteroid and SpaceShip are peer GameObjects.
struct SpaceStation: public GameObject
{
    SpaceStation(const std::string& name): GameObject(name) {}
    // Only a dummy SpaceStation is constructed without a name
    SpaceStation(): GameObject("dummy") {}
};

struct Asteroid: public GameObject
{
    Asteroid(const std::string& name): GameObject(name) {}
    Asteroid(): GameObject("dummy") {}
};

struct SpaceShip: public GameObject
{
    SpaceShip(const std::string& name): GameObject(name) {}
    SpaceShip(): GameObject("dummy") {}
};

// SpaceShip is specialized further into CommercialShip and MilitaryShip.
struct CommercialShip: public SpaceShip
{
    CommercialShip(const std::string& name): SpaceShip(name) {}
    CommercialShip(): SpaceShip("dummy") {}
};

struct MilitaryShip: public SpaceShip
{
    MilitaryShip(const std::string& name): SpaceShip(name) {}
    MilitaryShip(): SpaceShip("dummy") {}
};

/*-------------------------- Collision functions ---------------------------*/
// This mechanism permits us to overcome a limitation of Meyers' approach:  we
// can declare the parameter types exactly as we want, rather than having to
// make them all GameObject& parameters.
std::string shipAsteroid(SpaceShip& ship, Asteroid& rock)
{
//  std::cout << rock.stringize() << " has pulverized " << ship.stringize() << std::endl;
    return "shipAsteroid";
}

std::string militaryShipAsteroid(MilitaryShip& ship, Asteroid& rock)
{
//  std::cout << rock.stringize() << " has severely damaged " << ship.stringize() << std::endl;
    return "militaryShipAsteroid";
}

std::string shipStation(SpaceShip& ship, SpaceStation& dock)
{
//  std::cout << ship.stringize() << " has docked at " << dock.stringize() << std::endl;
    return "shipStation";
}

std::string asteroidStation(Asteroid& rock, SpaceStation& dock)
{
//  std::cout << rock.stringize() << " has damaged " << dock.stringize() << std::endl;
    return "asteroidStation";
}

/*------------------------------- Test code --------------------------------*/
namespace tut
{
    struct dispatch_data
    {
        dispatch_data():
            home(new SpaceStation("Terra Station")),
            obstacle(new Asteroid("Ganymede")),
            tug(new CommercialShip("Pilotfish")),
            patrol(new MilitaryShip("Enterprise"))
        {}

        // Instantiate and populate the DoubleDispatch object.
        typedef LLDoubleDispatch<std::string, GameObject> DD;
        DD dispatcher;

        // Instantiate a few GameObjects.  Make sure we refer to them
        // polymorphically, and don't let them leak.
        std::unique_ptr<GameObject> home;
        std::unique_ptr<GameObject> obstacle;
        std::unique_ptr<GameObject> tug;
        std::unique_ptr<GameObject> patrol;

        // prototype objects
        Asteroid dummyAsteroid;
        SpaceShip dummyShip;
        MilitaryShip dummyMilitary;
        CommercialShip dummyCommercial;
        SpaceStation dummyStation;
    };
    typedef test_group<dispatch_data> dispatch_group;
    typedef dispatch_group::object dispatch_object;
    tut::dispatch_group ddgr("double dispatch");

    template<> template<>
    void dispatch_object::test<1>()
    {
        // Describe param types using explicit DD::Type objects
        // (order-sensitive add() variant)
        dispatcher.add(DD::Type<SpaceShip>(), DD::Type<Asteroid>(), shipAsteroid, true);
        // naive adding, won't work
        dispatcher.add(DD::Type<MilitaryShip>(), DD::Type<Asteroid>(), militaryShipAsteroid, true);
        dispatcher.add(DD::Type<SpaceShip>(), DD::Type<SpaceStation>(), shipStation, true);
        dispatcher.add(DD::Type<Asteroid>(), DD::Type<SpaceStation>(), asteroidStation, true);

        // Try colliding them.
        ensure_equals(dispatcher(*home, *tug),        // reverse params, SpaceShip subclass
                      "shipStation");
        ensure_equals(dispatcher(*patrol, *home),     // forward params, SpaceShip subclass
                      "shipStation");
        ensure_equals(dispatcher(*obstacle, *home),   // forward params
                      "asteroidStation");
        ensure_equals(dispatcher(*home, *obstacle),   // reverse params
                      "asteroidStation");
        ensure_equals(dispatcher(*tug, *obstacle),    // forward params, SpaceShip subclass
                      "shipAsteroid");
        ensure_equals(dispatcher(*obstacle, *patrol), // reverse params, SpaceShip subclass
                      // won't use militaryShipAsteroid() because it was added
                      // in wrong order
                      "shipAsteroid");
    }

    template<> template<>
    void dispatch_object::test<2>()
    {
        // Describe param types using explicit DD::Type objects
        // (order-sensitive add() variant)
        // adding in correct order
        dispatcher.add(DD::Type<MilitaryShip>(), DD::Type<Asteroid>(), militaryShipAsteroid, true);
        dispatcher.add(DD::Type<SpaceShip>(), DD::Type<Asteroid>(), shipAsteroid, true);
        dispatcher.add(DD::Type<SpaceShip>(), DD::Type<SpaceStation>(), shipStation, true);
        dispatcher.add(DD::Type<Asteroid>(), DD::Type<SpaceStation>(), asteroidStation, true);
        
        ensure_equals(dispatcher(*patrol, *obstacle), "militaryShipAsteroid");
        ensure_equals(dispatcher(*tug, *obstacle), "shipAsteroid");
    }

    template<> template<>
    void dispatch_object::test<3>()
    {
        // Describe param types with actual prototype instances
        // (order-insensitive add() variant)
        dispatcher.add(dummyMilitary, dummyAsteroid, militaryShipAsteroid);
        dispatcher.add(dummyShip, dummyAsteroid, shipAsteroid);
        dispatcher.add(dummyShip, dummyStation, shipStation);
        dispatcher.add(dummyAsteroid, dummyStation, asteroidStation);

        ensure_equals(dispatcher(*patrol, *obstacle), "militaryShipAsteroid");
        ensure_equals(dispatcher(*tug, *obstacle), "shipAsteroid");
        ensure_equals(dispatcher(*obstacle, *patrol), "");
    }

    template<> template<>
    void dispatch_object::test<4>()
    {
        // Describe param types with actual prototype instances
        // (order-insensitive add() variant)
        dispatcher.add(dummyShip, dummyAsteroid, shipAsteroid);
        // Even if we add the militaryShipAsteroid in the wrong order, it
        // should still work.
        dispatcher.add(dummyMilitary, dummyAsteroid, militaryShipAsteroid);
        dispatcher.add(dummyShip, dummyStation, shipStation);
        dispatcher.add(dummyAsteroid, dummyStation, asteroidStation);

        ensure_equals(dispatcher(*patrol, *obstacle), "militaryShipAsteroid");
        ensure_equals(dispatcher(*tug, *obstacle), "shipAsteroid");
    }

    template<> template<>
    void dispatch_object::test<5>()
    {
        dispatcher.add<SpaceShip, Asteroid>(shipAsteroid);
        dispatcher.add<MilitaryShip, Asteroid>(militaryShipAsteroid);
        dispatcher.add<SpaceShip, SpaceStation>(shipStation);
        dispatcher.add<Asteroid, SpaceStation>(asteroidStation);

        ensure_equals(dispatcher(*patrol, *obstacle), "militaryShipAsteroid");
        ensure_equals(dispatcher(*tug, *obstacle), "shipAsteroid");
    }
} // namespace tut
