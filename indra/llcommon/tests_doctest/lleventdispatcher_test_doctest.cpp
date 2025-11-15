/**
 * @file lleventdispatcher_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for LL eventdispatcher
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
// Auto-generated from lleventdispatcher_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "lleventdispatcher.h"
#include "lleventfilter.h"
#include "llsd.h"
#include "llsdutil.h"
#include "llevents.h"
#include "stringize.h"
// #include "StringVec.h"  // not available on Windows
#include "tests/wrapllerrs.h"
#include "../test/catch_and_store_what_in.h"
#include "../test/debug.h"
#include <map>
#include <string>
#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/range.hpp>
#include <boost/lambda/lambda.hpp>
#include <iostream>
#include <iomanip>

TUT_SUITE("llcommon")
{
    TUT_CASE("lleventdispatcher_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         set_test_name("map-style registration with non-array params");
        //         // Pass "param names" as scalar or as map
        //         LLSD attempts(llsd::array(17, LLSDMap("pi", 3.14)("two", 2)));
        //         for (LLSD ae: inArray(attempts))
        //         {
        //             std::string threw = catch_what<std::exception>([this, &ae](){
        //                     work.add("freena_err", "freena", freena, ae);
        //                 });
        //             ensure_has(threw, "must be an array");
        //         }
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_2")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<2>()
        //     {
        //         set_test_name("map-style registration with badly-formed defaults");
        //         std::string threw = catch_what<std::exception>([this](){
        //                 work.add("freena_err", "freena", freena, llsd::array("a", "b"), 17);
        //             });
        //         ensure_has(threw, "must be a map or an array");
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_3")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<3>()
        //     {
        //         set_test_name("map-style registration with too many array defaults");
        //         std::string threw = catch_what<std::exception>([this](){
        //                 work.add("freena_err", "freena", freena,
        //                          llsd::array("a", "b"),
        //                          llsd::array(17, 0.9, "gack"));
        //             });
        //         ensure_has(threw, "shorter than");
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_4")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<4> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<4>()
        //     {
        //         set_test_name("map-style registration with too many map defaults");
        //         std::string threw = catch_what<std::exception>([this](){
        //                 work.add("freena_err", "freena", freena,
        //                          llsd::array("a", "b"),
        //                          LLSDMap("b", 17)("foo", 3.14)("bar", "sinister"));
        //             });
        //         ensure_has(threw, "nonexistent params");
        //         ensure_has(threw, "foo");
        //         ensure_has(threw, "bar");
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_5")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<5> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<5>()
        //     {
        //         set_test_name("query all");
        //         verify_descs();
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_6")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<6> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<6>()
        //     {
        //         set_test_name("query all with remove()");
        //         ensure("remove('bogus') returned true", ! work.remove("bogus"));
        //         ensure("remove('real') returned false", work.remove("free1"));
        //         // Of course, remove that from 'descs' too...
        //         descs.erase("free1");
        //         verify_descs();
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_7")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<7> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<7>()
        //     {
        //         set_test_name("getDispatchKey()");
        //         ensure_equals(work.getDispatchKey(), "op");
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_8")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<8> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<8>()
        //     {
        //         set_test_name("query Callables with/out required params");
        //         LLSD names(llsd::array("free1", "Dmethod1", "Dcmethod1", "method1"));
        //         for (LLSD nm: inArray(names))
        //         {
        //             LLSD metadata(getMetadata(nm));
        //             ensure_equals("name mismatch", metadata["name"], nm);
        //             ensure_equals(metadata["desc"].asString(), descs[nm]);
        //             ensure("should not have required structure", metadata["required"].isUndefined());
        //             ensure("should not have optional", metadata["optional"].isUndefined());

        //             std::string name_req(nm.asString() + "_req");
        //             metadata = getMetadata(name_req);
        //             ensure_equals(metadata["name"].asString(), name_req);
        //             ensure_equals(metadata["desc"].asString(), descs[name_req]);
        //             ensure_equals("required mismatch", required, metadata["required"]);
        //             ensure("should not have optional", metadata["optional"].isUndefined());
        //         }
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_9")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<9> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<9>()
        //     {
        //         set_test_name("query array-style functions/methods");
        //         // Associate each registered name with expected arity.
        //         LLSD expected(llsd::array
        //                       (llsd::array
        //                        (0, llsd::array("free0_array", "smethod0_array", "method0_array")),
        //                        llsd::array
        //                        (5, llsd::array("freena_array", "smethodna_array", "methodna_array")),
        //                        llsd::array
        //                        (5, llsd::array("freenb_array", "smethodnb_array", "methodnb_array"))));
        //         for (LLSD ae: inArray(expected))
        //         {
        //             LLSD::Integer arity(ae[0].asInteger());
        //             LLSD names(ae[1]);
        //             LLSD req(LLSD::emptyArray());
        //             if (arity)
        //                 req[arity - 1] = LLSD();
        //             for (LLSD nm: inArray(names))
        //             {
        //                 LLSD metadata(getMetadata(nm));
        //                 ensure_equals("name mismatch", metadata["name"], nm);
        //                 ensure_equals(metadata["desc"].asString(), descs[nm]);
        //                 ensure_equals(stringize("mismatched required for ", nm.asString()),
        //                               metadata["required"], req);
        //                 ensure("should not have optional", metadata["optional"].isUndefined());
        //             }
        //         }
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_10")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<10> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<10>()
        //     {
        //         set_test_name("query map-style no-params functions/methods");
        //         // - (Free function | non-static method), map style, no params (ergo
        //         //   no defaults)
        //         LLSD names(llsd::array("free0_map", "smethod0_map", "method0_map"));
        //         for (LLSD nm: inArray(names))
        //         {
        //             LLSD metadata(getMetadata(nm));
        //             ensure_equals("name mismatch", metadata["name"], nm);
        //             ensure_equals(metadata["desc"].asString(), descs[nm]);
        //             ensure("should not have required",
        //                    (metadata["required"].isUndefined() || metadata["required"].size() == 0));
        //             ensure("should not have optional", metadata["optional"].isUndefined());
        //         }
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_11")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<11> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<11>()
        //     {
        //         set_test_name("query map-style arbitrary-params functions/methods: "
        //                       "full array defaults vs. full map defaults");
        //         // With functions registered with no defaults ("_allreq" suffixes),
        //         // there is of course no difference between array defaults and map
        //         // defaults. (We don't even bother registering with LLSD::emptyArray()
        //         // vs. LLSD::emptyMap().) With functions registered with all defaults,
        //         // there should (!) be no difference beween array defaults and map
        //         // defaults. Verify, so we can ignore the distinction for all other
        //         // tests.
        //         LLSD equivalences(llsd::array
        //                           (llsd::array("freena_map_adft", "freena_map_mdft"),
        //                            llsd::array("freenb_map_adft", "freenb_map_mdft"),
        //                            llsd::array("smethodna_map_adft", "smethodna_map_mdft"),
        //                            llsd::array("smethodnb_map_adft", "smethodnb_map_mdft"),
        //                            llsd::array("methodna_map_adft", "methodna_map_mdft"),
        //                            llsd::array("methodnb_map_adft", "methodnb_map_mdft")));
        //         for (LLSD eq: inArray(equivalences))
        //         {
        //             LLSD adft(eq[0]);
        //             LLSD mdft(eq[1]);
        //             // We can't just compare the results of the two getMetadata()
        //             // calls, because they contain ["name"], which are different. So
        //             // capture them, verify that each ["name"] is as expected, then
        //             // remove for comparing the rest.
        //             LLSD ameta(getMetadata(adft));
        //             LLSD mmeta(getMetadata(mdft));
        //             ensure_equals("adft name", adft, ameta["name"]);
        //             ensure_equals("mdft name", mdft, mmeta["name"]);
        //             ameta.erase("name");
        //             mmeta.erase("name");
        //             ensure_equals(stringize("metadata for ", adft.asString(),
        //                                     " vs. ", mdft.asString()),
        //                           ameta, mmeta);
        //         }
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_12")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<12> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<12>()
        //     {
        //         set_test_name("query map-style arbitrary-params functions/methods");
        //         // - (Free function | non-static method), map style, arbitrary params,
        //         //   (empty | full | partial (array | map)) defaults

        //         // Generate maps containing all parameter names for cases in which all
        //         // params are required. Also maps containing left requirements for
        //         // partial defaults arrays. Also defaults maps from defaults arrays.
        //         LLSD allreq, leftreq, rightdft;
        //         for (LLSD::String a: ab)
        //         {
        //             // The map in which all params are required uses params[a] as
        //             // keys, with all isUndefined() as values. We can accomplish that
        //             // by passing zipmap() an empty values array.
        //             allreq[a] = zipmap(params[a], LLSD::emptyArray());
        //             // Same for leftreq, save that we use the subset of the params not
        //             // supplied by dft_array_partial[a].
        //             LLSD::Integer partition(static_cast<LLSD::Integer>(params[a].size() - dft_array_partial[a].size()));
        //             leftreq[a] = zipmap(llsd_copy_array(params[a].beginArray(),
        //                                                 params[a].beginArray() + partition),
        //                                 LLSD::emptyArray());
        //             // Generate map pairing dft_array_partial[a] values with their
        //             // param names.
        //             rightdft[a] = zipmap(llsd_copy_array(params[a].beginArray() + partition,
        //                                                  params[a].endArray()),
        //                                  dft_array_partial[a]);
        //         }
        //         debug("allreq:\n",
        //               allreq, "\n"
        //               "leftreq:\n",
        //               leftreq, "\n"
        //               "rightdft:\n",
        //               rightdft);

        //         // Generate maps containing parameter names not provided by the
        //         // dft_map_partial maps.
        //         LLSD skipreq(allreq);
        //         for (LLSD::String a: ab)
        //         {
        //             for (const MapEntry& me: inMap(dft_map_partial[a]))
        //             {
        //                 skipreq[a].erase(me.first);
        //             }
        //         }
        //         debug("skipreq:\n",
        //               skipreq);

        //         LLSD groups(llsd::array       // array of groups

        //                     (llsd::array      // group
        //                      (llsd::array("freena_map_allreq", "smethodna_map_allreq", "methodna_map_allreq"),
        //                       llsd::array(allreq["a"], LLSD())),  // required, optional

        //                      llsd::array        // group
        //                      (llsd::array("freenb_map_allreq", "smethodnb_map_allreq", "methodnb_map_allreq"),
        //                       llsd::array(allreq["b"], LLSD())),  // required, optional

        //                      llsd::array        // group
        //                      (llsd::array("freena_map_leftreq", "smethodna_map_leftreq", "methodna_map_leftreq"),
        //                       llsd::array(leftreq["a"], rightdft["a"])),  // required, optional

        //                      llsd::array        // group
        //                      (llsd::array("freenb_map_leftreq", "smethodnb_map_leftreq", "methodnb_map_leftreq"),
        //                       llsd::array(leftreq["b"], rightdft["b"])),  // required, optional

        //                      llsd::array        // group
        //                      (llsd::array("freena_map_skipreq", "smethodna_map_skipreq", "methodna_map_skipreq"),
        //                       llsd::array(skipreq["a"], dft_map_partial["a"])),  // required, optional

        //                      llsd::array        // group
        //                      (llsd::array("freenb_map_skipreq", "smethodnb_map_skipreq", "methodnb_map_skipreq"),
        //                       llsd::array(skipreq["b"], dft_map_partial["b"])),  // required, optional

        //                      // We only need mention the full-map-defaults ("_mdft" suffix)
        //                      // registrations, having established their equivalence with the
        //                      // full-array-defaults ("_adft" suffix) registrations in another test.
        //                      llsd::array        // group
        //                      (llsd::array("freena_map_mdft", "smethodna_map_mdft", "methodna_map_mdft"),
        //                       llsd::array(LLSD::emptyMap(), dft_map_full["a"])),  // required, optional

        //                      llsd::array        // group
        //                      (llsd::array("freenb_map_mdft", "smethodnb_map_mdft", "methodnb_map_mdft"),
        //                       llsd::array(LLSD::emptyMap(), dft_map_full["b"])))); // required, optional

        //         for (LLSD grp: inArray(groups))
        //         {
        //             // Internal structure of each group in 'groups':
        //             LLSD names(grp[0]);
        //             LLSD required(grp[1][0]);
        //             LLSD optional(grp[1][1]);
        //             debug("For ", names, ",\n",
        //                   "required:\n",
        //                   required, "\n"
        //                   "optional:\n",
        //                   optional);

        //             // Loop through 'names'
        //             for (LLSD nm: inArray(names))
        //             {
        //                 LLSD metadata(getMetadata(nm));
        //                 ensure_equals("name mismatch", metadata["name"], nm);
        //                 ensure_equals(nm.asString(), metadata["desc"].asString(), descs[nm]);
        //                 ensure_equals(stringize(nm, " required mismatch"),
        //                               metadata["required"], required);
        //                 ensure_equals(stringize(nm, " optional mismatch"),
        //                               metadata["optional"], optional);
        //             }
        //         }
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_13")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<13> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<13>()
        //     {
        //         set_test_name("try_call()");
        //         ensure("try_call(bogus name, LLSD()) returned true", ! work.try_call("freek", LLSD()));
        //         ensure("try_call(bogus name) returned true", ! work.try_call(LLSDMap("op", "freek")));
        //         ensure("try_call(real name, LLSD()) returned false", work.try_call("free0_array", LLSD()));
        //         ensure("try_call(real name) returned false", work.try_call(LLSDMap("op", "free0_map")));
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_14")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<14> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<14>()
        //     {
        //         set_test_name("call with bad name");
        //         call_exc("freek", LLSD(), "not found");
        //         std::string threw = call_exc("", LLSDMap("op", "freek"), "bad");
        //         ensure_has(threw, "op");
        //         ensure_has(threw, "freek");
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_15")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<15> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<15>()
        //     {
        //         set_test_name("call with event key");
        //         // We don't need a separate test for operator()(string, LLSD) with
        //         // valid name, because all the rest of the tests exercise that case.
        //         // The one we don't exercise elsewhere is operator()(LLSD) with valid
        //         // name, so here it is.
        //         work(LLSDMap("op", "free0_map"));
        //         ensure_equals(g.i, 17);
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_16")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<16> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<16>()
        //     {
        //         set_test_name("call Callables");
        //         CallablesTriple tests[] =
        //         {
        //             { "free1",     "free1_req",     g.llsd },
        //             { "Dmethod1",  "Dmethod1_req",  work.llsd },
        //             { "Dcmethod1", "Dcmethod1_req", work.llsd },
        //             { "method1",   "method1_req",   v.llsd }
        //         };
        //         // Arbitrary LLSD value that we should be able to pass to Callables
        //         // without 'required', but should not be able to pass to Callables
        //         // with 'required'.
        //         LLSD answer(42);
        //         // LLSD value matching 'required' according to llsd_matches() rules.
        //         LLSD matching(LLSDMap("d", 3.14)("array", llsd::array("answer", true, answer)));
        //         // Okay, walk through 'tests'.
        //         for (const CallablesTriple& tr: tests)
        //         {
        //             // Should be able to pass 'answer' to Callables registered
        //             // without 'required'.
        //             work(tr.name, answer);
        //             ensure_equals("answer mismatch", tr.llsd, answer);
        //             // Should NOT be able to pass 'answer' to Callables registered
        //             // with 'required'.
        //             call_logerr(tr.name_req, answer, "bad request");
        //             // But SHOULD be able to pass 'matching' to Callables registered
        //             // with 'required'.
        //             work(tr.name_req, matching);
        //             ensure_equals("matching mismatch", tr.llsd, matching);
        //         }
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_17")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<17> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<17>()
        //     {
        //         set_test_name("passing wrong args to (map | array)-style registrations");

        //         // Pass scalar/map to array-style functions, scalar/array to map-style
        //         // functions. It seems pointless to repeat this with every variation:
        //         // (free function | non-static method), (no | arbitrary) args. We
        //         // should only need to engage it for one map-style registration and
        //         // one array-style registration.
        //         // Now that LLEventDispatcher has been extended to treat an LLSD
        //         // scalar as a single-entry array, the error we expect in this case is
        //         // that apply() is trying to pass that non-empty array to a nullary
        //         // function.
        //         call_logerr("free0_array", 17, "LL::apply");
        //         // similarly, apply() doesn't accept an LLSD Map
        //         call_logerr("free0_array", LLSDMap("pi", 3.14), "unsupported");

        //         std::string map_exc("needs a map");
        //         call_logerr("free0_map", 17, map_exc);
        //         // Passing an array to a map-style function works now! No longer an
        //         // error case!
        // //      call_exc("free0_map", llsd::array("a", "b"), map_exc);
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_18")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<18> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<18>()
        //     {
        //         set_test_name("call no-args functions");
        //         LLSD names(llsd::array
        //                    ("free0_array", "free0_map",
        //                     "smethod0_array", "smethod0_map",
        //                     "method0_array", "method0_map"));
        //         for (LLSD name: inArray(names))
        //         {
        //             // Look up the Vars instance for this function.
        //             Vars* vars(varsfor(name));
        //             // Both the global and stack Vars instances are automatically
        //             // cleared at the start of each test<n> method. But since we're
        //             // calling these things several different times in the same
        //             // test<n> method, manually reset the Vars between each.
        //             *vars = Vars();
        //             ensure_equals(vars->i, 0);
        //             // call function with empty array (or LLSD(), should be equivalent)
        //             work(name, LLSD());
        //             ensure_equals(vars->i, 17);
        //         }
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_19")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<19> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<19>()
        //     {
        //         set_test_name("call array-style functions with wrong-length arrays");
        //         // Could have different wrong-length arrays for *na and for *nb, but
        //         // since they both take 5 params...
        //         LLSD tooshort(llsd::array("this", "array", "too", "short"));
        //         LLSD toolong (llsd::array("this", "array", "is",  "one", "too", "long"));
        //         LLSD badargs (llsd::array(tooshort, toolong));
        //         for (const LLSD& toosomething: inArray(badargs))
        //         {
        //             for (const LLSD& funcsab: inArray(array_funcs))
        //             {
        //                 for (const llsd::MapEntry& e: inMap(funcsab))
        //                 {
        //                     // apply() complains about wrong number of array entries
        //                     call_logerr(e.second, toosomething, "LL::apply");
        //                 }
        //             }
        //         }
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_20")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<20> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<20>()
        //     {
        //         set_test_name("call array-style functions with right-size arrays");
        //         std::vector<U8> binary;
        //         for (size_t h(0x01), i(0); i < 5; h+= 0x22, ++i)
        //         {
        //             binary.push_back((U8)h);
        //         }
        //         LLSD args(LLSDMap("a", llsd::array(true, 17, 3.14, 123.456, "char*"))
        //                          ("b", llsd::array("string",
        //                                            LLUUID("01234567-89ab-cdef-0123-456789abcdef"),
        //                                            LLDate("2011-02-03T15:07:00Z"),
        //                                            LLURI("http://secondlife.com"),
        //                                            binary)));
        //         LLSD expect;
        //         for (LLSD::String a: ab)
        //         {
        //             expect[a] = zipmap(params[a], args[a]);
        //         }
        //         // Adjust expect["a"]["cp"] for special Vars::cp treatment.
        //         expect["a"]["cp"] = stringize("'", expect["a"]["cp"].asString(), "'");
        //         debug("expect: ", expect);

        //         for (const LLSD& funcsab: inArray(array_funcs))
        //         {
        //             for (LLSD::String a: ab)
        //             {
        //                 // Reset the Vars instance before each call
        //                 Vars* vars(varsfor(funcsab[a]));
        //                 *vars = Vars();
        //                 work(funcsab[a], args[a]);
        //                 ensure_llsd(stringize(funcsab[a].asString(), ": expect[\"", a, "\"] mismatch"),
        //                             vars->inspect(), expect[a], 7); // 7 bits ~= 2 decimal digits
        //             }
        //         }
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_21")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<21> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<21>()
        //     {
        //         set_test_name("verify that passing LLSD() to const char* sends NULL");

        //         ensure_equals("Vars::cp init", v.cp, "");
        //         work("methodna_map_mdft", LLSDMap("cp", LLSD()));
        //         ensure_equals("passing LLSD()", v.cp, "NULL");
        //         work("methodna_map_mdft", LLSDMap("cp", ""));
        //         ensure_equals("passing \"\"", v.cp, "''");
        //         work("methodna_map_mdft", LLSDMap("cp", "non-NULL"));
        //         ensure_equals("passing \"non-NULL\"", v.cp, "'non-NULL'");
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_22")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<22> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<22>()
        //     {
        //         set_test_name("call map-style functions with (full | oversized) (arrays | maps)");
        //         const char binary[] = "\x99\x88\x77\x66\x55";
        //         LLSD array_full(LLSDMap
        //                         ("a", llsd::array(false, 255, 98.6, 1024.5, "pointer"))
        //                         ("b", llsd::array("object", LLUUID::generateNewID(), LLDate::now(), LLURI("http://wiki.lindenlab.com/wiki"), LLSD::Binary(boost::begin(binary), boost::end(binary)))));
        //         LLSD array_overfull(array_full);
        //         for (LLSD::String a: ab)
        //         {
        //             array_overfull[a].append("bogus");
        //         }
        //         debug("array_full: ", array_full, "\n"
        //               "array_overfull: ", array_overfull);
        //         // We rather hope that LLDate::now() will generate a timestamp
        //         // distinct from the one it generated in the constructor, moments ago.
        //         ensure_not_equals("Timestamps too close",
        //                           array_full["b"][2].asDate(), dft_array_full["b"][2].asDate());
        //         // We /insist/ that LLUUID::generateNewID() do so.
        //         ensure_not_equals("UUID collision",
        //                           array_full["b"][1].asUUID(), dft_array_full["b"][1].asUUID());
        //         LLSD map_full, map_overfull;
        //         for (LLSD::String a: ab)
        //         {
        //             map_full[a] = zipmap(params[a], array_full[a]);
        //             map_overfull[a] = map_full[a];
        //             map_overfull[a]["extra"] = "ignore";
        //         }
        //         debug("map_full: ", map_full, "\n"
        //               "map_overfull: ", map_overfull);
        //         LLSD expect(map_full);
        //         // Twiddle the const char* param.
        //         expect["a"]["cp"] = std::string("'") + expect["a"]["cp"].asString() + "'";
        //         // Another adjustment. For each data type, we're trying to distinguish
        //         // three values: the Vars member's initial value (member wasn't
        //         // stored; control never reached the set function), the registered
        //         // default param value from dft_array_full, and the array_full value
        //         // in this test. But bool can only distinguish two values. In this
        //         // case, we want to differentiate the local array_full value from the
        //         // dft_array_full value, so we use 'false'. However, that means
        //         // Vars::inspect() doesn't differentiate it from the initial value,
        //         // so won't bother returning it. Predict that behavior to match the
        //         // LLSD values.
        //         expect["a"].erase("b");
        //         debug("expect: ", expect);
        //         // For this test, calling functions registered with different sets of
        //         // parameter defaults should make NO DIFFERENCE WHATSOEVER. Every call
        //         // should pass all params.
        //         LLSD names(LLSDMap
        //                    ("a", llsd::array
        //                     ("freena_map_allreq",  "smethodna_map_allreq",  "methodna_map_allreq",
        //                      "freena_map_leftreq", "smethodna_map_leftreq", "methodna_map_leftreq",
        //                      "freena_map_skipreq", "smethodna_map_skipreq", "methodna_map_skipreq",
        //                      "freena_map_adft",    "smethodna_map_adft",    "methodna_map_adft",
        //                      "freena_map_mdft",    "smethodna_map_mdft",    "methodna_map_mdft"))
        //                    ("b", llsd::array
        //                     ("freenb_map_allreq",  "smethodnb_map_allreq",  "methodnb_map_allreq",
        //                      "freenb_map_leftreq", "smethodnb_map_leftreq", "methodnb_map_leftreq",
        //                      "freenb_map_skipreq", "smethodnb_map_skipreq", "methodnb_map_skipreq",
        //                      "freenb_map_adft",    "smethodnb_map_adft",    "methodnb_map_adft",
        //                      "freenb_map_mdft",    "smethodnb_map_mdft",    "methodnb_map_mdft")));
        //         // Treat (full | overfull) (array | map) the same.
        //         LLSD argssets(llsd::array(array_full, array_overfull, map_full, map_overfull));
        //         for (const LLSD& args: inArray(argssets))
        //         {
        //             for (LLSD::String a: ab)
        //             {
        //                 for (LLSD::String name: inArray(names[a]))
        //                 {
        //                     // Reset the Vars instance
        //                     Vars* vars(varsfor(name));
        //                     *vars = Vars();
        //                     work(name, args[a]);
        //                     ensure_llsd(stringize(name, ": expect[\"", a, "\"] mismatch"),
        //                                 vars->inspect(), expect[a], 7); // 7 bits, 2 decimal digits
        //                     // intercept LL_WARNS for the two overfull cases?
        //                 }
        //             }
        //         }
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_23")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<23> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<23>()
        //     {
        //         set_test_name("string result");
        //         DispatchResult service;
        //         LLSD result{ service("strfunc", "a string") };
        //         ensure_equals("strfunc() mismatch", result.asString(), "got a string");
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_24")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<24> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<24>()
        //     {
        //         set_test_name("void result");
        //         DispatchResult service;
        //         LLSD result{ service("voidfunc", LLSD()) };
        //         ensure("voidfunc() returned defined", result.isUndefined());
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_25")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<25> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<25>()
        //     {
        //         set_test_name("Integer result");
        //         DispatchResult service;
        //         LLSD result{ service("intfunc", -17) };
        //         ensure_equals("intfunc() mismatch", result.asInteger(), 17);
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_26")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<26> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<26>()
        //     {
        //         set_test_name("LLSD echo");
        //         DispatchResult service;
        //         LLSD result{ service("llsdfunc", llsd::map("op", "llsdfunc", "reqid", 17)) };
        //         ensure_equals("llsdfunc() mismatch", result,
        //                       llsd::map("op", "llsdfunc", "reqid", 17, "with", "string"));
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_27")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<27> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<27>()
        //     {
        //         set_test_name("map LLSD result");
        //         DispatchResult service;
        //         LLSD result{ service("mapfunc", llsd::array(-12, "value")) };
        //         ensure_equals("mapfunc() mismatch", result, llsd::map("i", 12, "str", "got value"));
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_28")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<28> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<28>()
        //     {
        //         set_test_name("array LLSD result");
        //         DispatchResult service;
        //         LLSD result{ service("arrayfunc", llsd::array(-8, "word")) };
        //         ensure_equals("arrayfunc() mismatch", result, llsd::array(8, "got word"));
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_29")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<29> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<29>()
        //     {
        //         set_test_name("listener error, no reply");
        //         DispatchResult service;
        //         tut::call_exc(
        //             [&service]()
        //             { service.post(llsd::map("op", "nosuchfunc", "reqid", 17)); },
        //             "nosuchfunc");
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_30")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<30> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<30>()
        //     {
        //         set_test_name("listener error with reply");
        //         DispatchResult service;
        //         LLCaptureListener<LLSD> result;
        //         service.post(llsd::map("op", "nosuchfunc", "reqid", 17, "reply", result.getName()));
        //         LLSD reply{ result.get() };
        //         ensure("no reply", reply.isDefined());
        //         ensure_equals("reqid not echoed", reply["reqid"].asInteger(), 17);
        //         ensure_has(reply["error"].asString(), "nosuchfunc");
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_31")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<31> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<31>()
        //     {
        //         set_test_name("listener call to void function");
        //         DispatchResult service;
        //         LLCaptureListener<LLSD> result;
        //         result.set("non-empty");
        //         for (const auto& func: StringVec{ "voidfunc", "emptyfunc" })
        //         {
        //             service.post(llsd::map(
        //                              "op", func,
        //                              "reqid", 17,
        //                              "reply", result.getName()));
        //             ensure_equals("reply from " + func, result.get().asString(), "non-empty");
        //         }
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_32")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<32> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<32>()
        //     {
        //         set_test_name("listener call to string function");
        //         DispatchResult service;
        //         LLCaptureListener<LLSD> result;
        //         service.post(llsd::map(
        //                          "op", "strfunc",
        //                          "args", llsd::array("a string"),
        //                          "reqid", 17,
        //                          "reply", result.getName()));
        //         LLSD reply{ result.get() };
        //         ensure_equals("reqid not echoed", reply["reqid"].asInteger(), 17);
        //         ensure_equals("bad reply from strfunc", reply["data"].asString(), "got a string");
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_33")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<33> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<33>()
        //     {
        //         set_test_name("listener call to map function");
        //         DispatchResult service;
        //         LLCaptureListener<LLSD> result;
        //         service.post(llsd::map(
        //                          "op", "mapfunc",
        //                          "args", llsd::array(-7, "value"),
        //                          "reqid", 17,
        //                          "reply", result.getName()));
        //         LLSD reply{ result.get() };
        //         ensure_equals("reqid not echoed", reply["reqid"].asInteger(), 17);
        //         ensure_equals("bad i from mapfunc", reply["i"].asInteger(), 7);
        //         ensure_equals("bad str from mapfunc", reply["str"], "got value");
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_34")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<34> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<34>()
        //     {
        //         set_test_name("batched map success");
        //         DispatchResult service;
        //         LLCaptureListener<LLSD> result;
        //         service.post(llsd::map(
        //                          "op", llsd::map(
        //                              "strfunc", "some string",
        //                              "intfunc", 2,
        //                              "voidfunc", LLSD(),
        //                              "arrayfunc", llsd::array(-5, "other string")),
        //                          "reqid", 17,
        //                          "reply", result.getName()));
        //         LLSD reply{ result.get() };
        //         ensure_equals("reqid not echoed", reply["reqid"].asInteger(), 17);
        //         reply.erase("reqid");
        //         ensure_equals(
        //             "bad map batch",
        //             reply,
        //             llsd::map(
        //                 "strfunc", "got some string",
        //                 "intfunc", -2,
        //                 "voidfunc", LLSD(),
        //                 "arrayfunc", llsd::array(5, "got other string")));
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_35")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<35> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<35>()
        //     {
        //         set_test_name("batched map error");
        //         DispatchResult service;
        //         LLCaptureListener<LLSD> result;
        //         service.post(llsd::map(
        //                          "op", llsd::map(
        //                              "badfunc", 34, // !
        //                              "strfunc", "some string",
        //                              "intfunc", 2,
        //                              "missing", LLSD(), // !
        //                              "voidfunc", LLSD(),
        //                              "arrayfunc", llsd::array(-5, "other string")),
        //                          "reqid", 17,
        //                          "reply", result.getName()));
        //         LLSD reply{ result.get() };
        //         ensure_equals("reqid not echoed", reply["reqid"].asInteger(), 17);
        //         reply.erase("reqid");
        //         auto error{ reply["error"].asString() };
        //         reply.erase("error");
        //         ensure_has(error, "badfunc");
        //         ensure_has(error, "missing");
        //         ensure_equals(
        //             "bad partial batch",
        //             reply,
        //             llsd::map(
        //                 "strfunc", "got some string",
        //                 "intfunc", -2,
        //                 "voidfunc", LLSD(),
        //                 "arrayfunc", llsd::array(5, "got other string")));
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_36")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<36> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<36>()
        //     {
        //         set_test_name("batched map exception");
        //         DispatchResult service;
        //         auto error = tut::call_exc(
        //             [&service]()
        //             {
        //                 service.post(llsd::map(
        //                                  "op", llsd::map(
        //                                      "badfunc", 34, // !
        //                                      "strfunc", "some string",
        //                                      "intfunc", 2,
        //                                      "missing", LLSD(), // !
        //                                      "voidfunc", LLSD(),
        //                                      "arrayfunc", llsd::array(-5, "other string")),
        //                                  "reqid", 17));
        //                 // no "reply"
        //             },
        //             "badfunc");
        //         ensure_has(error, "missing");
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_37")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<37> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<37>()
        //     {
        //         set_test_name("batched array success");
        //         DispatchResult service;
        //         LLCaptureListener<LLSD> result;
        //         service.post(llsd::map(
        //                          "op", llsd::array(
        //                              llsd::array("strfunc", "some string"),
        //                              llsd::array("intfunc", 2),
        //                              "arrayfunc",
        //                              "voidfunc"),
        //                          "args", llsd::array(
        //                              LLSD(),
        //                              LLSD(),
        //                              llsd::array(-5, "other string")),
        //                          // args array deliberately short, since the default
        //                          // [3] is undefined, which should work for voidfunc
        //                          "reqid", 17,
        //                          "reply", result.getName()));
        //         LLSD reply{ result.get() };
        //         ensure_equals("reqid not echoed", reply["reqid"].asInteger(), 17);
        //         reply.erase("reqid");
        //         ensure_equals(
        //             "bad array batch",
        //             reply,
        //             llsd::map(
        //                 "data", llsd::array(
        //                     "got some string",
        //                     -2,
        //                     llsd::array(5, "got other string"),
        //                     LLSD())));
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_38")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<38> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<38>()
        //     {
        //         set_test_name("batched array error");
        //         DispatchResult service;
        //         LLCaptureListener<LLSD> result;
        //         service.post(llsd::map(
        //                          "op", llsd::array(
        //                              llsd::array("strfunc", "some string"),
        //                              llsd::array("intfunc", 2, "whoops"), // bad form
        //                              "arrayfunc",
        //                              "voidfunc"),
        //                          "args", llsd::array(
        //                              LLSD(),
        //                              LLSD(),
        //                              llsd::array(-5, "other string")),
        //                          // args array deliberately short, since the default
        //                          // [3] is undefined, which should work for voidfunc
        //                          "reqid", 17,
        //                          "reply", result.getName()));
        //         LLSD reply{ result.get() };
        //         ensure_equals("reqid not echoed", reply["reqid"].asInteger(), 17);
        //         reply.erase("reqid");
        //         auto error{ reply["error"] };
        //         reply.erase("error");
        //         ensure_has(error, "[1]");
        //         ensure_has(error, "unsupported");
        //         ensure_equals("bad array batch", reply,
        //                       llsd::map("data", llsd::array("got some string")));
        //     }
    }

    TUT_CASE("lleventdispatcher_test::object_test_39")
    {
        DOCTEST_FAIL("TODO: convert lleventdispatcher_test.cpp::object::test<39> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<39>()
        //     {
        //         set_test_name("batched array exception");
        //         DispatchResult service;
        //         auto error = tut::call_exc(
        //             [&service]()
        //             {
        //                 service.post(llsd::map(
        //                                  "op", llsd::array(
        //                                      llsd::array("strfunc", "some string"),
        //                                      llsd::array("intfunc", 2, "whoops"), // bad form
        //                                      "arrayfunc",
        //                                      "voidfunc"),
        //                                  "args", llsd::array(
        //                                      LLSD(),
        //                                      LLSD(),
        //                                      llsd::array(-5, "other string")),
        //                                  // args array deliberately short, since the default
        //                                  // [3] is undefined, which should work for voidfunc
        //                                  "reqid", 17));
        //                 // no "reply"
        //             },
        //             "[1]");
        //         ensure_has(error, "unsupported");
        //     }
    }

}

