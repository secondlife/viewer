/**
 * @file llsdutil_tut.cpp
 * @author Adroit
 * @date 2007-02
 * @brief LLSD conversion routines test cases.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "linden_common.h"
#include "lltut.h"
#include "m4math.h"
#include "v2math.h"
#include "v2math.h"
#include "v3color.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v4coloru.h"
#include "v4math.h"
#include "llquaternion.h"
#include "llsdutil.h"
#include "llsdutil_math.h"
#include "stringize.h"
#include <set>
#include <boost/range.hpp>

namespace tut
{
    struct llsdutil_data
    {
        void test_matches(const std::string& proto_key, const LLSD& possibles,
                          const char** begin, const char** end)
        {
            std::set<std::string> succeed(begin, end);
            LLSD prototype(possibles[proto_key]);
            for (LLSD::map_const_iterator pi(possibles.beginMap()), pend(possibles.endMap());
                 pi != pend; ++pi)
            {
                std::string match(llsd_matches(prototype, pi->second));
                std::set<std::string>::const_iterator found = succeed.find(pi->first);
                if (found != succeed.end())
                {
                    // This test is supposed to succeed. Comparing to the
                    // empty string ensures that if the test fails, it will
                    // display the string received so we can tell what failed.
                    ensure_equals("match", match, "");
                }
                else
                {
                    // This test is supposed to fail. If we get a false match,
                    // the string 'match' will be empty, which doesn't tell us
                    // much about which case went awry. So construct a more
                    // detailed description string.
                    ensure(proto_key + " shouldn't match " + pi->first, ! match.empty());
                }
            }
        }
    };
    typedef test_group<llsdutil_data> llsdutil_test;;
    typedef llsdutil_test::object llsdutil_object;
    tut::llsdutil_test tutil("llsdutil");

    template<> template<>
    void llsdutil_object::test<1>()
    {
        LLSD sd;
        U64 valueIn , valueOut;
        valueIn = U64L(0xFEDCBA9876543210);
        sd = ll_sd_from_U64(valueIn);
        valueOut = ll_U64_from_sd(sd);
        ensure_equals("U64 valueIn->sd->valueOut", valueIn, valueOut);
    }

    template<> template<>
    void llsdutil_object::test<2>()
    {
        LLSD sd;
        U32 valueIn, valueOut;
        valueIn = 0x87654321;
        sd = ll_sd_from_U32(valueIn);
        valueOut = ll_U32_from_sd(sd);
        ensure_equals("U32 valueIn->sd->valueOut", valueIn, valueOut);
    }

    template<> template<>
    void llsdutil_object::test<3>()
    {
        U32 valueIn, valueOut;
        valueIn = 0x87654321;
        LLSD sd;
        sd = ll_sd_from_ipaddr(valueIn);
        valueOut = ll_ipaddr_from_sd(sd);
        ensure_equals("valueIn->sd->valueOut", valueIn, valueOut);
    }

    template<> template<>
    void llsdutil_object::test<4>()
    {
        LLSD sd;
        LLVector3 vec1(-1.0, 2.0, -3.0);
        sd = ll_sd_from_vector3(vec1);
        LLVector3 vec2 = ll_vector3_from_sd(sd);
        ensure_equals("vector3 -> sd -> vector3: 1", vec1, vec2);

        LLVector3 vec3(sd);
        ensure_equals("vector3 -> sd -> vector3: 2", vec1, vec3);

        sd.clear();
        vec1.setVec(0., 0., 0.);
        sd = ll_sd_from_vector3(vec1);
        vec2 = ll_vector3_from_sd(sd);
        ensure_equals("vector3 -> sd -> vector3: 3", vec1, vec2);
        sd.clear();
    }

    template<> template<>
    void llsdutil_object::test<5>()
    {
        LLSD sd;
        LLVector3d vec1((F64)(U64L(0xFEDCBA9876543210) << 2), -1., 0);
        sd = ll_sd_from_vector3d(vec1);
        LLVector3d vec2 = ll_vector3d_from_sd(sd);
        ensure_equals("vector3d -> sd -> vector3d: 1", vec1, vec2);

        LLVector3d vec3(sd);
        ensure_equals("vector3d -> sd -> vector3d : 2", vec1, vec3);
    }

    template<> template<>
    void llsdutil_object::test<6>()
    {
        LLSD sd;
        LLVector2 vec((F32) -3., (F32) 4.2);
        sd = ll_sd_from_vector2(vec);
        LLVector2 vec1 = ll_vector2_from_sd(sd);
        ensure_equals("vector2 -> sd -> vector2", vec, vec1);

        LLSD sd2 = ll_sd_from_vector2(vec1);
        ensure_equals("sd -> vector2 -> sd: 2", sd, sd2);
    }

    template<> template<>
    void llsdutil_object::test<7>()
    {
        LLSD sd;
        LLQuaternion quat((F32) 1., (F32) -0.98, (F32) 2.3, (F32) 0xffff);
        sd = ll_sd_from_quaternion(quat);
        LLQuaternion quat1 = ll_quaternion_from_sd(sd);
        ensure_equals("LLQuaternion -> sd -> LLQuaternion", quat, quat1);

        LLSD sd2 = ll_sd_from_quaternion(quat1);
        ensure_equals("sd -> LLQuaternion -> sd ", sd, sd2);
    }

    template<> template<>
    void llsdutil_object::test<8>()
    {
        LLSD sd;
        LLColor4 c(1.0f, 2.2f, 4.0f, 7.f);
        sd = ll_sd_from_color4(c);
        LLColor4 c1 =ll_color4_from_sd(sd);
        ensure_equals("LLColor4 -> sd -> LLColor4", c, c1);

        LLSD sd1 = ll_sd_from_color4(c1);
        ensure_equals("sd -> LLColor4 -> sd", sd, sd1);
    }

    template<> template<>
    void llsdutil_object::test<9>()
    {
        set_test_name("llsd_matches");

        // for this test, construct a map of all possible LLSD types
        LLSD map;
        map.insert("empty",     LLSD());
        map.insert("Boolean",   LLSD::Boolean());
        map.insert("Integer",   LLSD::Integer(0));
        map.insert("Real",      LLSD::Real(0.0));
        map.insert("String",    LLSD::String("bah"));
        map.insert("NumString", LLSD::String("1"));
        map.insert("UUID",      LLSD::UUID());
        map.insert("Date",      LLSD::Date());
        map.insert("URI",       LLSD::URI());
        map.insert("Binary",    LLSD::Binary());
        map.insert("Map",       LLSD().with("foo", LLSD()));
        // Only an empty array can be constructed on the fly
        LLSD array;
        array.append(LLSD());
        map.insert("Array",     array);

        // These iterators are declared outside our various for loops to avoid
        // fatal MSVC warning: "I used to be broken, but I'm all better now!"
        LLSD::map_const_iterator mi, mend(map.endMap());

        /*-------------------------- llsd_matches --------------------------*/

        // empty prototype matches anything
        for (mi = map.beginMap(); mi != mend; ++mi)
        {
            ensure_equals(std::string("empty matches ") + mi->first, llsd_matches(LLSD(), mi->second), "");
        }

        LLSD proto_array, data_array;
        for (int i = 0; i < 3; ++i)
        {
            proto_array.append(LLSD());
            data_array.append(LLSD());
        }

        // prototype array matches only array
        for (mi = map.beginMap(); mi != mend; ++mi)
        {
            ensure(std::string("array doesn't match ") + mi->first,
                   ! llsd_matches(proto_array, mi->second).empty());
        }

        // data array must be at least as long as prototype array
        proto_array.append(LLSD());
        ensure_equals("data array too short", llsd_matches(proto_array, data_array),
                      "Array size 4 required instead of Array size 3");
        data_array.append(LLSD());
        ensure_equals("data array just right", llsd_matches(proto_array, data_array), "");
        data_array.append(LLSD());
        ensure_equals("data array longer", llsd_matches(proto_array, data_array), "");

        // array element matching
        data_array[0] = LLSD::String();
        ensure_equals("undefined prototype array entry", llsd_matches(proto_array, data_array), "");
        proto_array[0] = LLSD::Binary();
        ensure_equals("scalar prototype array entry", llsd_matches(proto_array, data_array),
                      "[0]: Binary required instead of String");
        data_array[0] = LLSD::Binary();
        ensure_equals("matching prototype array entry", llsd_matches(proto_array, data_array), "");

        // build a coupla maps
        LLSD proto_map, data_map;
        data_map["got"] = LLSD();
        data_map["found"] = LLSD();
        for (LLSD::map_const_iterator dmi(data_map.beginMap()), dmend(data_map.endMap());
             dmi != dmend; ++dmi)
        {
            proto_map[dmi->first] = dmi->second;
        }
        proto_map["foo"] = LLSD();
        proto_map["bar"] = LLSD();

        // prototype map matches only map
        for (mi = map.beginMap(); mi != mend; ++mi)
        {
            ensure(std::string("map doesn't match ") + mi->first,
                   ! llsd_matches(proto_map, mi->second).empty());
        }

        // data map must contain all keys in prototype map
        std::string error(llsd_matches(proto_map, data_map));
        ensure_contains("missing keys", error, "missing keys");
        ensure_contains("missing foo", error, "foo");
        ensure_contains("missing bar", error, "bar");
        ensure_does_not_contain("found found", error, "found");
        ensure_does_not_contain("got got", error, "got");
        data_map["bar"] = LLSD();
        error = llsd_matches(proto_map, data_map);
        ensure_contains("missing foo", error, "foo");
        ensure_does_not_contain("got bar", error, "bar");
        data_map["foo"] = LLSD();
        ensure_equals("data map just right", llsd_matches(proto_map, data_map), "");
        data_map["extra"] = LLSD();
        ensure_equals("data map with extra", llsd_matches(proto_map, data_map), "");

        // map element matching
        data_map["foo"] = LLSD::String();
        ensure_equals("undefined prototype map entry", llsd_matches(proto_map, data_map), "");
        proto_map["foo"] = LLSD::Binary();
        ensure_equals("scalar prototype map entry", llsd_matches(proto_map, data_map),
                      "['foo']: Binary required instead of String");
        data_map["foo"] = LLSD::Binary();
        ensure_equals("matching prototype map entry", llsd_matches(proto_map, data_map), "");

        // String
        {
            static const char* matches[] = { "String", "NumString", "Boolean", "Integer",
                                             "Real", "UUID", "Date", "URI" };
            test_matches("String", map, boost::begin(matches), boost::end(matches));
        }

        // Boolean, Integer, Real
        static const char* numerics[] = { "Boolean", "Integer", "Real" };
        for (const char **ni = boost::begin(numerics), **nend = boost::end(numerics);
             ni != nend; ++ni)
        {
            static const char* matches[] = { "Boolean", "Integer", "Real", "String", "NumString" };
            test_matches(*ni, map, boost::begin(matches), boost::end(matches));
        }

        // UUID
        {
            static const char* matches[] = { "UUID", "String", "NumString" };
            test_matches("UUID", map, boost::begin(matches), boost::end(matches));
        }

        // Date
        {
            static const char* matches[] = { "Date", "String", "NumString" };
            test_matches("Date", map, boost::begin(matches), boost::end(matches));
        }

        // URI
        {
            static const char* matches[] = { "URI", "String", "NumString" };
            test_matches("URI", map, boost::begin(matches), boost::end(matches));
        }

        // Binary
        {
            static const char* matches[] = { "Binary" };
            test_matches("Binary", map, boost::begin(matches), boost::end(matches));
        }

        /*-------------------------- llsd_equals ---------------------------*/

        // Cross-product of each LLSD type with every other
        for (LLSD::map_const_iterator lmi(map.beginMap()), lmend(map.endMap());
             lmi != lmend; ++lmi)
        {
            for (LLSD::map_const_iterator rmi(map.beginMap()), rmend(map.endMap());
                 rmi != rmend; ++rmi)
            {
                // Name this test based on the map keys naming the types of
                // interest, e.g "String::Integer".
                // We expect the values (xmi->second) to be equal if and only
                // if the type names (xmi->first) are equal.
                ensure(STRINGIZE(lmi->first << "::" << rmi->first),
                       bool(lmi->first == rmi->first) ==
                       bool(llsd_equals(lmi->second, rmi->second)));
            }
        }

        // Array cases
        LLSD rarray;
        rarray.append(1.0);
        rarray.append(2);
        rarray.append("3");
        LLSD larray(rarray);
        ensure("llsd_equals(equal arrays)", llsd_equals(larray, rarray));
        rarray[2] = "4";
        ensure("llsd_equals(different [2])", ! llsd_equals(larray, rarray));
        rarray = larray;
        rarray.append(LLSD::Date());
        ensure("llsd_equals(longer right array)", ! llsd_equals(larray, rarray));
        rarray = larray;
        rarray.erase(2);
        ensure("llsd_equals(shorter right array)", ! llsd_equals(larray, rarray));

        // Map cases
        LLSD rmap;
        rmap["San Francisco"] = 65;
        rmap["Phoenix"] = 92;
        rmap["Boston"] = 77;
        LLSD lmap(rmap);
        ensure("llsd_equals(equal maps)", llsd_equals(lmap, rmap));
        rmap["Boston"] = 80;
        ensure("llsd_equals(different [\"Boston\"])", ! llsd_equals(lmap, rmap));
        rmap = lmap;
        rmap["Atlanta"] = 95;
        ensure("llsd_equals(superset right map)", ! llsd_equals(lmap, rmap));
        rmap = lmap;
        lmap["Seattle"] = 72;
        ensure("llsd_equals(superset left map)", ! llsd_equals(lmap, rmap));
    }

    template<> template<>
    void llsdutil_object::test<10>()
    {
        set_test_name("llsd_hashing");

        {
            LLSD data_s1 = LLSD::String("The quick brown aardvark jumped over the lazy lemming.");
            LLSD data_s2 = LLSD::String("The quick brown aardvark jumped over the lazy lemming.");

            ensure("hash: Identical string hashes match.", boost::hash<LLSD>{}(data_s1) == boost::hash<LLSD>{}(data_s2));
        }
        {
            LLSD data_r1 = LLSD::Real(3.0f);
            LLSD data_i1 = LLSD::Integer(3);
            ensure("hash: equivalent values but different types do not match.", boost::hash<LLSD>{}(data_r1) != boost::hash<LLSD>{}(data_i1));
        }
        {
            LLSD data_a1 = llsd::array("A", "B", "C");
            LLSD data_a2 = llsd::array("A", "B", "C");

            ensure("hash: identical arrays produce identical results", boost::hash<LLSD>{}(data_a1) == boost::hash<LLSD>{}(data_a2));

            data_a2.append(llsd::array(1, 2));

            ensure("hash: changing the array changes the hash.", boost::hash<LLSD>{}(data_a1) != boost::hash<LLSD>{}(data_a2));

            data_a1.append(llsd::array(1, 2));
            ensure("hash: identical arrays produce identical results with nested arrays", boost::hash<LLSD>{}(data_a1) == boost::hash<LLSD>{}(data_a2));
        }
        {
            LLSD data_m1 = LLSDMap("key1", LLSD::Real(3.0))("key2", "value2")("key3", llsd::array(1, 2, 3));
            LLSD data_m2 = LLSDMap("key1", LLSD::Real(3.0))("key2", "value2")("key3", llsd::array(1, 2, 3));

            ensure("hash: identical maps produce identical results", boost::hash<LLSD>{}(data_m1) == boost::hash<LLSD>{}(data_m2));

            LLSD data_m3 = LLSDMap("key1", LLSD::Real(5.0))("key2", "value2")("key3", llsd::array(1, 2, 3));
            ensure("hash: Different values in the map produce different hashes.", boost::hash<LLSD>{}(data_m1) != boost::hash<LLSD>{}(data_m3));

            LLSD data_m4 = LLSDMap("keyA", LLSD::Real(3.0))("key2", "value2")("key3", llsd::array(1, 2, 3));
            ensure("hash: Different keys in the map produce different hashes.", boost::hash<LLSD>{}(data_m1) != boost::hash<LLSD>{}(data_m4));

        }
    }

}
