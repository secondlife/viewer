/**
 * @file   lleventdispatcher_test.cpp
 * @author Nat Goodspeed
 * @date   2011-01-20
 * @brief  Test for lleventdispatcher.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Copyright (c) 2011, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "lleventdispatcher.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "../test/lldoctest.h"
#include "lleventfilter.h"
#include "llsd.h"
#include "llsdutil.h"
#include "llevents.h"
#include "stringize.h"
#include "StringVec.h"
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

using boost::lambda::constant;
using boost::lambda::constant_ref;
using boost::lambda::var;

using namespace llsd;

/*****************************************************************************
*   Example data, functions, classes
*****************************************************************************/
// We don't need a whole lot of different arbitrary-params methods, just (no |
// (const LLSD&) | arbitrary) args (function | static method | non-static
// method), where 'arbitrary' is (every LLSD datatype + (const char*)).
// But we need to register each one under different names for the different
// registration styles. Don't forget LLEventDispatcher subclass methods(const
// LLSD&).

// However, the number of target parameter conversions we want to try exceeds
// boost::fusion::invoke()'s supported parameter-list size. Break out two
// different lists.
#define NPARAMSa bool b, int i, float f, double d, const char* cp
#define NPARAMSb const std::string& s, const LLUUID& uuid, const LLDate& date, \
                 const LLURI& uri, const std::vector<U8>& bin
#define NARGSa   b, i, f, d, cp
#define NARGSb   s, uuid, date, uri, bin

// For some registration methods we need methods on a subclass of
// LLEventDispatcher. To simplify things, we'll use this Dispatcher subclass
// for all our testing, including testing its own methods.
class Dispatcher: public LLEventDispatcher
{
public:
    Dispatcher(const std::string& name, const std::string& key):
        LLEventDispatcher(name, key)
    {}

    // sensing member, mutable because we want to know when we've reached our
    // const method too
    mutable LLSD llsd;

    void method1(const LLSD& obj) { llsd = obj; }
    void cmethod1(const LLSD& obj) const { llsd = obj; }
};

// sensing vars, captured in a struct to make it convenient to clear them
struct Vars
{
    LLSD llsd;
    bool b;
    int i;
    float f;
    double d;
    // Capture param passed as char*. But merely storing a char* received from
    // our caller, possibly the .c_str() from a concatenation expression,
    // would be Bad: the pointer will be invalidated long before we can query
    // it. We could allocate a new chunk of memory, copy the string data and
    // point to that instead -- but hey, guess what, we already have a class
    // that does that!
    std::string cp;
    std::string s;
    LLUUID uuid;
    LLDate date;
    LLURI uri;
    std::vector<U8> bin;

    Vars():
        // Only need to initialize the POD types, the rest should take care of
        // default-constructing themselves.
        b(false),
        i(0),
        f(0),
        d(0)
    {}

    // Detect any non-default values for convenient testing
    LLSD inspect() const
    {
        LLSD result;

        if (llsd.isDefined())
            result["llsd"] = llsd;
        if (b)
            result["b"] = b;
        if (i)
            result["i"] = i;
        if (f)
            result["f"] = f;
        if (d)
            result["d"] = d;
        if (! cp.empty())
            result["cp"] = cp;
        if (! s.empty())
            result["s"] = s;
        if (uuid != LLUUID())
            result["uuid"] = uuid;
        if (date != LLDate())
            result["date"] = date;
        if (uri != LLURI())
            result["uri"] = uri;
        if (! bin.empty())
            result["bin"] = bin;

        return result;
    }

    /*------------- no-args (non-const, const, static) methods -------------*/
    void method0()
    {
        debug()("method0()");
        i = 17;
    }

    void cmethod0() const
    {
        debug()('c', NONL);
        const_cast<Vars*>(this)->method0();
    }

    static void smethod0();

    /*------------ Callable (non-const, const, static) methods -------------*/
    void method1(const LLSD& obj)
    {
        debug()("method1(", obj, ")");
        llsd = obj;
    }

    void cmethod1(const LLSD& obj) const
    {
        debug()('c', NONL);
        const_cast<Vars*>(this)->method1(obj);
    }

    static void smethod1(const LLSD& obj);

    /*-------- Arbitrary-params (non-const, const, static) methods ---------*/
    void methodna(NPARAMSa)
    {
        DEBUG;
        // Because our const char* param cp might be NULL, and because we
        // intend to capture the value in a std::string, have to distinguish
        // between the NULL value and any non-NULL value. Use a convention
        // easy for a human reader: enclose any non-NULL value in single
        // quotes, reserving the unquoted string "NULL" to represent a NULL ptr.
        std::string vcp;
        if (cp == NULL)
            vcp = "NULL";
        else
            vcp = std::string("'") + cp + "'";

        this->debug()("methodna(", b,
              ", ", i,
              ", ", f,
              ", ", d,
              ", ", vcp,
              ")");

        this->b = b;
        this->i = i;
        this->f = f;
        this->d = d;
        this->cp = vcp;
    }

    void methodnb(NPARAMSb)
    {
        std::ostringstream vbin;
        for (U8 byte: bin)
        {
            vbin << std::hex << std::setfill('0') << std::setw(2) << unsigned(byte);
        }

        debug()("methodnb(", "'", s, "'",
              ", ", uuid,
              ", ", date,
              ", '", uri, "'",
              ", ", vbin.str(),
              ")");

        this->s = s;
        this->uuid = uuid;
        this->date = date;
        this->uri = uri;
        this->bin = bin;
    }

    void cmethodna(NPARAMSa) const
    {
        DEBUG;
        this->debug()('c', NONL);
        const_cast<Vars*>(this)->methodna(NARGSa);
    }

    void cmethodnb(NPARAMSb) const
    {
        debug()('c', NONL);
        const_cast<Vars*>(this)->methodnb(NARGSb);
    }

    static void smethodna(NPARAMSa);
    static void smethodnb(NPARAMSb);

    static Debug& debug()
    {
        // Lazily initialize this Debug instance so it can notice if main()
        // has forcibly set LOGTEST. If it were simply a static member, it
        // would already have examined the environment variable by the time
        // main() gets around to checking command-line switches. Since we have
        // a global static Vars instance, the same would be true of a plain
        // non-static member.
        static Debug sDebug("Vars");
        return sDebug;
    }
};
/*------- Global Vars instance for free functions and static methods -------*/
static Vars g;

/*------------ Static Vars method implementations reference 'g' ------------*/
void Vars::smethod0()
{
    debug()("smethod0() -> ", NONL);
    g.method0();
}

void Vars::smethod1(const LLSD& obj)
{
    debug()("smethod1(", obj, ") -> ", NONL);
    g.method1(obj);
}

void Vars::smethodna(NPARAMSa)
{
    debug()("smethodna(...) -> ", NONL);
    g.methodna(NARGSa);
}

void Vars::smethodnb(NPARAMSb)
{
    debug()("smethodnb(...) -> ", NONL);
    g.methodnb(NARGSb);
}

/*--------------------------- Reset global Vars ----------------------------*/
void clear()
{
    g = Vars();
}

/*------------------- Free functions also reference 'g' --------------------*/
void free0()
{
    g.debug()("free0() -> ", NONL);
    g.method0();
}

void free1(const LLSD& obj)
{
    g.debug()("free1(", obj, ") -> ", NONL);
    g.method1(obj);
}

void freena(NPARAMSa)
{
    g.debug()("freena(...) -> ", NONL);
    g.methodna(NARGSa);
}

void freenb(NPARAMSb)
{
    g.debug()("freenb(...) -> ", NONL);
    g.methodnb(NARGSb);
}

/*****************************************************************************
*   TUT
*****************************************************************************/
TEST_SUITE("UnknownSuite") {

struct lleventdispatcher_data
{

        Debug debug{"test"
};

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_1")
{

        set_test_name("map-style registration with non-array params");
        // Pass "param names" as scalar or as map
        LLSD attempts(llsd::array(17, LLSDMap("pi", 3.14)("two", 2)));
        for (LLSD ae: inArray(attempts))
        {
            std::string threw = catch_what<std::exception>([this, &ae](){
                    work.add("freena_err", "freena", freena, ae);
                
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_2")
{

        set_test_name("map-style registration with badly-formed defaults");
        std::string threw = catch_what<std::exception>([this](){
                work.add("freena_err", "freena", freena, llsd::array("a", "b"), 17);
            
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_3")
{

        set_test_name("map-style registration with too many array defaults");
        std::string threw = catch_what<std::exception>([this](){
                work.add("freena_err", "freena", freena,
                         llsd::array("a", "b"),
                         llsd::array(17, 0.9, "gack"));
            
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_4")
{

        set_test_name("map-style registration with too many map defaults");
        std::string threw = catch_what<std::exception>([this](){
                work.add("freena_err", "freena", freena,
                         llsd::array("a", "b"),
                         LLSDMap("b", 17)("foo", 3.14)("bar", "sinister"));
            
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_5")
{

        set_test_name("query all");
        verify_descs();
    
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_6")
{

        set_test_name("query all with remove()");
        CHECK_MESSAGE(! work.remove("bogus", "remove('bogus') returned true"));
        CHECK_MESSAGE(work.remove("free1", "remove('real') returned false"));
        // Of course, remove that from 'descs' too...
        descs.erase("free1");
        verify_descs();
    
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_7")
{

        set_test_name("getDispatchKey()");
        ensure_equals(work.getDispatchKey(), "op");
    
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_8")
{

        set_test_name("query Callables with/out required params");
        LLSD names(llsd::array("free1", "Dmethod1", "Dcmethod1", "method1"));
        for (LLSD nm: inArray(names))
        {
            LLSD metadata(getMetadata(nm));
            CHECK_MESSAGE(metadata["name"] == nm, "name mismatch");
            ensure_equals(metadata["desc"].asString(), descs[nm]);
            CHECK_MESSAGE(metadata["required"].isUndefined(, "should not have required structure"));
            CHECK_MESSAGE(metadata["optional"].isUndefined(, "should not have optional"));

            std::string name_req(nm.asString() + "_req");
            metadata = getMetadata(name_req);
            ensure_equals(metadata["name"].asString(), name_req);
            ensure_equals(metadata["desc"].asString(), descs[name_req]);
            CHECK_MESSAGE(required == metadata["required"], "required mismatch");
            CHECK_MESSAGE(metadata["optional"].isUndefined(, "should not have optional"));
        
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_9")
{

        set_test_name("query array-style functions/methods");
        // Associate each registered name with expected arity.
        LLSD expected(llsd::array
                      (llsd::array
                       (0, llsd::array("free0_array", "smethod0_array", "method0_array")),
                       llsd::array
                       (5, llsd::array("freena_array", "smethodna_array", "methodna_array")),
                       llsd::array
                       (5, llsd::array("freenb_array", "smethodnb_array", "methodnb_array"))));
        for (LLSD ae: inArray(expected))
        {
            LLSD::Integer arity(ae[0].asInteger());
            LLSD names(ae[1]);
            LLSD req(LLSD::emptyArray());
            if (arity)
                req[arity - 1] = LLSD();
            for (LLSD nm: inArray(names))
            {
                LLSD metadata(getMetadata(nm));
                CHECK_MESSAGE(metadata["name"] == nm, "name mismatch");
                ensure_equals(metadata["desc"].asString(), descs[nm]);
                ensure_equals(stringize("mismatched required for ", nm.asString()),
                              metadata["required"], req);
                CHECK_MESSAGE(metadata["optional"].isUndefined(, "should not have optional"));
            
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_10")
{

        set_test_name("query map-style no-params functions/methods");
        // - (Free function | non-static method), map style, no params (ergo
        //   no defaults)
        LLSD names(llsd::array("free0_map", "smethod0_map", "method0_map"));
        for (LLSD nm: inArray(names))
        {
            LLSD metadata(getMetadata(nm));
            CHECK_MESSAGE(metadata["name"] == nm, "name mismatch");
            ensure_equals(metadata["desc"].asString(), descs[nm]);
            CHECK_MESSAGE((metadata["required"].isUndefined(, "should not have required") || metadata["required"].size() == 0));
            CHECK_MESSAGE(metadata["optional"].isUndefined(, "should not have optional"));
        
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_11")
{

        set_test_name("query map-style arbitrary-params functions/methods: "
                      "full array defaults vs. full map defaults");
        // With functions registered with no defaults ("_allreq" suffixes),
        // there is of course no difference between array defaults and map
        // defaults. (We don't even bother registering with LLSD::emptyArray()
        // vs. LLSD::emptyMap().) With functions registered with all defaults,
        // there should (!) be no difference beween array defaults and map
        // defaults. Verify, so we can ignore the distinction for all other
        // tests.
        LLSD equivalences(llsd::array
                          (llsd::array("freena_map_adft", "freena_map_mdft"),
                           llsd::array("freenb_map_adft", "freenb_map_mdft"),
                           llsd::array("smethodna_map_adft", "smethodna_map_mdft"),
                           llsd::array("smethodnb_map_adft", "smethodnb_map_mdft"),
                           llsd::array("methodna_map_adft", "methodna_map_mdft"),
                           llsd::array("methodnb_map_adft", "methodnb_map_mdft")));
        for (LLSD eq: inArray(equivalences))
        {
            LLSD adft(eq[0]);
            LLSD mdft(eq[1]);
            // We can't just compare the results of the two getMetadata()
            // calls, because they contain ["name"], which are different. So
            // capture them, verify that each ["name"] is as expected, then
            // remove for comparing the rest.
            LLSD ameta(getMetadata(adft));
            LLSD mmeta(getMetadata(mdft));
            CHECK_MESSAGE(adft == ameta["name"], "adft name");
            CHECK_MESSAGE(mdft == mmeta["name"], "mdft name");
            ameta.erase("name");
            mmeta.erase("name");
            ensure_equals(stringize("metadata for ", adft.asString(),
                                    " vs. ", mdft.asString()),
                          ameta, mmeta);
        
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_12")
{

        set_test_name("query map-style arbitrary-params functions/methods");
        // - (Free function | non-static method), map style, arbitrary params,
        //   (empty | full | partial (array | map)) defaults

        // Generate maps containing all parameter names for cases in which all
        // params are required. Also maps containing left requirements for
        // partial defaults arrays. Also defaults maps from defaults arrays.
        LLSD allreq, leftreq, rightdft;
        for (LLSD::String a: ab)
        {
            // The map in which all params are required uses params[a] as
            // keys, with all isUndefined() as values. We can accomplish that
            // by passing zipmap() an empty values array.
            allreq[a] = zipmap(params[a], LLSD::emptyArray());
            // Same for leftreq, save that we use the subset of the params not
            // supplied by dft_array_partial[a].
            LLSD::Integer partition(static_cast<LLSD::Integer>(params[a].size() - dft_array_partial[a].size()));
            leftreq[a] = zipmap(llsd_copy_array(params[a].beginArray(),
                                                params[a].beginArray() + partition),
                                LLSD::emptyArray());
            // Generate map pairing dft_array_partial[a] values with their
            // param names.
            rightdft[a] = zipmap(llsd_copy_array(params[a].beginArray() + partition,
                                                 params[a].endArray()),
                                 dft_array_partial[a]);
        
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_13")
{

        set_test_name("try_call()");
        CHECK_MESSAGE(! work.try_call("freek", LLSD(, "try_call(bogus name, LLSD()) returned true")));
        CHECK_MESSAGE(! work.try_call(LLSDMap("op", "freek", "try_call(bogus name) returned true")));
        CHECK_MESSAGE(work.try_call("free0_array", LLSD(, "try_call(real name, LLSD()) returned false")));
        CHECK_MESSAGE(work.try_call(LLSDMap("op", "free0_map", "try_call(real name) returned false")));
    
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_14")
{

        set_test_name("call with bad name");
        call_exc("freek", LLSD(), "not found");
        std::string threw = call_exc("", LLSDMap("op", "freek"), "bad");
        ensure_has(threw, "op");
        ensure_has(threw, "freek");
    
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_15")
{

        set_test_name("call with event key");
        // We don't need a separate test for operator()(string, LLSD) with
        // valid name, because all the rest of the tests exercise that case.
        // The one we don't exercise elsewhere is operator()(LLSD) with valid
        // name, so here it is.
        work(LLSDMap("op", "free0_map"));
        ensure_equals(g.i, 17);
    
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_16")
{

        set_test_name("call Callables");
        CallablesTriple tests[] =
        {
            { "free1",     "free1_req",     g.llsd 
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_17")
{

        set_test_name("passing wrong args to (map | array)-style registrations");

        // Pass scalar/map to array-style functions, scalar/array to map-style
        // functions. It seems pointless to repeat this with every variation:
        // (free function | non-static method), (no | arbitrary) args. We
        // should only need to engage it for one map-style registration and
        // one array-style registration.
        // Now that LLEventDispatcher has been extended to treat an LLSD
        // scalar as a single-entry array, the error we expect in this case is
        // that apply() is trying to pass that non-empty array to a nullary
        // function.
        call_logerr("free0_array", 17, "LL::apply");
        // similarly, apply() doesn't accept an LLSD Map
        call_logerr("free0_array", LLSDMap("pi", 3.14), "unsupported");

        std::string map_exc("needs a map");
        call_logerr("free0_map", 17, map_exc);
        // Passing an array to a map-style function works now! No longer an
        // error case!
//      call_exc("free0_map", llsd::array("a", "b"), map_exc);
    
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_18")
{

        set_test_name("call no-args functions");
        LLSD names(llsd::array
                   ("free0_array", "free0_map",
                    "smethod0_array", "smethod0_map",
                    "method0_array", "method0_map"));
        for (LLSD name: inArray(names))
        {
            // Look up the Vars instance for this function.
            Vars* vars(varsfor(name));
            // Both the global and stack Vars instances are automatically
            // cleared at the start of each test<n> method. But since we're
            // calling these things several different times in the same
            // test<n> method, manually reset the Vars between each.
            *vars = Vars();
            ensure_equals(vars->i, 0);
            // call function with empty array (or LLSD(), should be equivalent)
            work(name, LLSD());
            ensure_equals(vars->i, 17);
        
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_19")
{

        set_test_name("call array-style functions with wrong-length arrays");
        // Could have different wrong-length arrays for *na and for *nb, but
        // since they both take 5 params...
        LLSD tooshort(llsd::array("this", "array", "too", "short"));
        LLSD toolong (llsd::array("this", "array", "is",  "one", "too", "long"));
        LLSD badargs (llsd::array(tooshort, toolong));
        for (const LLSD& toosomething: inArray(badargs))
        {
            for (const LLSD& funcsab: inArray(array_funcs))
            {
                for (const llsd::MapEntry& e: inMap(funcsab))
                {
                    // apply() complains about wrong number of array entries
                    call_logerr(e.second, toosomething, "LL::apply");
                
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_20")
{

        set_test_name("call array-style functions with right-size arrays");
        std::vector<U8> binary;
        for (size_t h(0x01), i(0); i < 5; h+= 0x22, ++i)
        {
            binary.push_back((U8)h);
        
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_21")
{

        set_test_name("verify that passing LLSD() to const char* sends NULL");

        CHECK_MESSAGE(v.cp == "", "Vars::cp init");
        work("methodna_map_mdft", LLSDMap("cp", LLSD()));
        CHECK_MESSAGE(v.cp == "NULL", "passing LLSD()");
        work("methodna_map_mdft", LLSDMap("cp", ""));
        ensure_equals("passing \"\"", v.cp, "''");
        work("methodna_map_mdft", LLSDMap("cp", "non-NULL"));
        ensure_equals("passing \"non-NULL\"", v.cp, "'non-NULL'");
    
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_22")
{

        set_test_name("call map-style functions with (full | oversized) (arrays | maps)");
        const char binary[] = "\x99\x88\x77\x66\x55";
        LLSD array_full(LLSDMap
                        ("a", llsd::array(false, 255, 98.6, 1024.5, "pointer"))
                        ("b", llsd::array("object", LLUUID::generateNewID(), LLDate::now(), LLURI("http://wiki.lindenlab.com/wiki"), LLSD::Binary(boost::begin(binary), boost::end(binary)))));
        LLSD array_overfull(array_full);
        for (LLSD::String a: ab)
        {
            array_overfull[a].append("bogus");
        
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_23")
{

        set_test_name("string result");
        DispatchResult service;
        LLSD result{ service("strfunc", "a string") 
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_24")
{

        set_test_name("void result");
        DispatchResult service;
        LLSD result{ service("voidfunc", LLSD()) 
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_25")
{

        set_test_name("Integer result");
        DispatchResult service;
        LLSD result{ service("intfunc", -17) 
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_26")
{

        set_test_name("LLSD echo");
        DispatchResult service;
        LLSD result{ service("llsdfunc", llsd::map("op", "llsdfunc", "reqid", 17)) 
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_27")
{

        set_test_name("map LLSD result");
        DispatchResult service;
        LLSD result{ service("mapfunc", llsd::array(-12, "value")) 
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_28")
{

        set_test_name("array LLSD result");
        DispatchResult service;
        LLSD result{ service("arrayfunc", llsd::array(-8, "word")) 
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_29")
{

        set_test_name("listener error, no reply");
        DispatchResult service;
        tut::call_exc(
            [&service]()
            { service.post(llsd::map("op", "nosuchfunc", "reqid", 17)); 
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_30")
{

        set_test_name("listener error with reply");
        DispatchResult service;
        LLCaptureListener<LLSD> result;
        service.post(llsd::map("op", "nosuchfunc", "reqid", 17, "reply", result.getName()));
        LLSD reply{ result.get() 
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_31")
{

        set_test_name("listener call to void function");
        DispatchResult service;
        LLCaptureListener<LLSD> result;
        result.set("non-empty");
        for (const auto& func: StringVec{ "voidfunc", "emptyfunc" 
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_32")
{

        set_test_name("listener call to string function");
        DispatchResult service;
        LLCaptureListener<LLSD> result;
        service.post(llsd::map(
                         "op", "strfunc",
                         "args", llsd::array("a string"),
                         "reqid", 17,
                         "reply", result.getName()));
        LLSD reply{ result.get() 
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_33")
{

        set_test_name("listener call to map function");
        DispatchResult service;
        LLCaptureListener<LLSD> result;
        service.post(llsd::map(
                         "op", "mapfunc",
                         "args", llsd::array(-7, "value"),
                         "reqid", 17,
                         "reply", result.getName()));
        LLSD reply{ result.get() 
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_34")
{

        set_test_name("batched map success");
        DispatchResult service;
        LLCaptureListener<LLSD> result;
        service.post(llsd::map(
                         "op", llsd::map(
                             "strfunc", "some string",
                             "intfunc", 2,
                             "voidfunc", LLSD(),
                             "arrayfunc", llsd::array(-5, "other string")),
                         "reqid", 17,
                         "reply", result.getName()));
        LLSD reply{ result.get() 
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_35")
{

        set_test_name("batched map error");
        DispatchResult service;
        LLCaptureListener<LLSD> result;
        service.post(llsd::map(
                         "op", llsd::map(
                             "badfunc", 34, // !
                             "strfunc", "some string",
                             "intfunc", 2,
                             "missing", LLSD(), // !
                             "voidfunc", LLSD(),
                             "arrayfunc", llsd::array(-5, "other string")),
                         "reqid", 17,
                         "reply", result.getName()));
        LLSD reply{ result.get() 
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_36")
{

        set_test_name("batched map exception");
        DispatchResult service;
        auto error = tut::call_exc(
            [&service]()
            {
                service.post(llsd::map(
                                 "op", llsd::map(
                                     "badfunc", 34, // !
                                     "strfunc", "some string",
                                     "intfunc", 2,
                                     "missing", LLSD(), // !
                                     "voidfunc", LLSD(),
                                     "arrayfunc", llsd::array(-5, "other string")),
                                 "reqid", 17));
                // no "reply"
            
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_37")
{

        set_test_name("batched array success");
        DispatchResult service;
        LLCaptureListener<LLSD> result;
        service.post(llsd::map(
                         "op", llsd::array(
                             llsd::array("strfunc", "some string"),
                             llsd::array("intfunc", 2),
                             "arrayfunc",
                             "voidfunc"),
                         "args", llsd::array(
                             LLSD(),
                             LLSD(),
                             llsd::array(-5, "other string")),
                         // args array deliberately short, since the default
                         // [3] is undefined, which should work for voidfunc
                         "reqid", 17,
                         "reply", result.getName()));
        LLSD reply{ result.get() 
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_38")
{

        set_test_name("batched array error");
        DispatchResult service;
        LLCaptureListener<LLSD> result;
        service.post(llsd::map(
                         "op", llsd::array(
                             llsd::array("strfunc", "some string"),
                             llsd::array("intfunc", 2, "whoops"), // bad form
                             "arrayfunc",
                             "voidfunc"),
                         "args", llsd::array(
                             LLSD(),
                             LLSD(),
                             llsd::array(-5, "other string")),
                         // args array deliberately short, since the default
                         // [3] is undefined, which should work for voidfunc
                         "reqid", 17,
                         "reply", result.getName()));
        LLSD reply{ result.get() 
}

TEST_CASE_FIXTURE(lleventdispatcher_data, "test_39")
{

        set_test_name("batched array exception");
        DispatchResult service;
        auto error = tut::call_exc(
            [&service]()
            {
                service.post(llsd::map(
                                 "op", llsd::array(
                                     llsd::array("strfunc", "some string"),
                                     llsd::array("intfunc", 2, "whoops"), // bad form
                                     "arrayfunc",
                                     "voidfunc"),
                                 "args", llsd::array(
                                     LLSD(),
                                     LLSD(),
                                     llsd::array(-5, "other string")),
                                 // args array deliberately short, since the default
                                 // [3] is undefined, which should work for voidfunc
                                 "reqid", 17));
                // no "reply"
            
}

} // TEST_SUITE
