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
#include "../test/lltut.h"
#include "llsd.h"
#include "llsdutil.h"
#include "stringize.h"
#include "tests/wrapllerrs.h"

#include <map>
#include <string>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/range.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <boost/lambda/lambda.hpp>

#include <iostream>
#include <iomanip>

using boost::lambda::constant;
using boost::lambda::constant_ref;
using boost::lambda::var;

using namespace llsd;

/*****************************************************************************
*   Output control
*****************************************************************************/
#ifdef DEBUG_ON
using std::cout;
#else
static std::ostringstream cout;
#endif

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
        cout << "method0()\n";
        i = 17;
    }

    void cmethod0() const
    {
        cout << 'c';
        const_cast<Vars*>(this)->method0();
    }

    static void smethod0();

    /*------------ Callable (non-const, const, static) methods -------------*/
    void method1(const LLSD& obj)
    {
        cout << "method1(" << obj << ")\n";
        llsd = obj;
    }

    void cmethod1(const LLSD& obj) const
    {
        cout << 'c';
        const_cast<Vars*>(this)->method1(obj);
    }

    static void smethod1(const LLSD& obj);

    /*-------- Arbitrary-params (non-const, const, static) methods ---------*/
    void methodna(NPARAMSa)
    {
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

        cout << "methodna(" << b
             << ", " << i
             << ", " << f
             << ", " << d
             << ", " << vcp
             << ")\n";

        this->b = b;
        this->i = i;
        this->f = f;
        this->d = d;
        this->cp = vcp;
    }

    void methodnb(NPARAMSb)
    {
        std::ostringstream vbin;
        foreach(U8 byte, bin)
        {
            vbin << std::hex << std::setfill('0') << std::setw(2) << byte;
        }

        cout << "methodnb(" << "'" << s << "'"
             << ", " << uuid
             << ", " << date
             << ", '" << uri << "'"
             << ", " << vbin.str()
             << ")\n";

        this->s = s;
        this->uuid = uuid;
        this->date = date;
        this->uri = uri;
        this->bin = bin;
    }

    void cmethodna(NPARAMSa) const
    {
        cout << 'c';
        const_cast<Vars*>(this)->methodna(NARGSa);
    }

    void cmethodnb(NPARAMSb) const
    {
        cout << 'c';
        const_cast<Vars*>(this)->methodnb(NARGSb);
    }

    static void smethodna(NPARAMSa);
    static void smethodnb(NPARAMSb);
};
/*------- Global Vars instance for free functions and static methods -------*/
static Vars g;

/*------------ Static Vars method implementations reference 'g' ------------*/
void Vars::smethod0()
{
    cout << "smethod0() -> ";
    g.method0();
}

void Vars::smethod1(const LLSD& obj)
{
    cout << "smethod1(" << obj << ") -> ";
    g.method1(obj);
}

void Vars::smethodna(NPARAMSa)
{
    cout << "smethodna(...) -> ";
    g.methodna(NARGSa);
}

void Vars::smethodnb(NPARAMSb)
{
    cout << "smethodnb(...) -> ";
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
    cout << "free0() -> ";
    g.method0();
}

void free1(const LLSD& obj)
{
    cout << "free1(" << obj << ") -> ";
    g.method1(obj);
}

void freena(NPARAMSa)
{
    cout << "freena(...) -> ";
    g.methodna(NARGSa);
}

void freenb(NPARAMSb)
{
    cout << "freenb(...) -> ";
    g.methodnb(NARGSb);
}

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct lleventdispatcher_data
    {
        WrapLL_ERRS redirect;
        Dispatcher work;
        Vars v;
        std::string name, desc;
        typedef std::map<std::string, std::string> FuncMap;
        FuncMap funcs;
        // Required structure for Callables with requirements
        LLSD required;
        // Parameter names for freena(), freenb()
        LLSD params;
        // Full, partial defaults arrays for params for freena(), freenb()
        LLSD dft_array_full, dft_array_partial;
        // Start index of partial defaults arrays
        const LLSD::Integer partial_offset;
        // Full, partial defaults maps for params for freena(), freenb()
        LLSD dft_map_full, dft_map_partial;
        // Most of the above are indexed by "a" or "b". Useful to have an
        // array containing those strings for iterating.
        std::vector<LLSD::String> ab;

        lleventdispatcher_data():
            work("test dispatcher", "op"),
            // map {d=double, array=[3 elements]}
            required(LLSDMap("d", LLSD::Real(0))("array", LLSDArray(LLSD())(LLSD())(LLSD()))),
            // first several params are required, last couple optional
            partial_offset(3)
        {
            // This object is reconstructed for every test<n> method. But
            // clear global variables every time too.
            ::clear();

            const char* abs[] = { "a", "b" };
            ab.assign(boost::begin(abs), boost::end(abs));

            // Registration cases:
            // - (Callable | subclass const method | subclass non-const method |
            //   non-subclass method) (with | without) required
            // - (Free function | static method | non-static method), (no | arbitrary) params,
            //   array style
            // - (Free function | static method | non-static method), (no | arbitrary) params,
            //   map style, (empty | partial | full) (array | map) defaults
            // - Map-style errors:
            //   - (scalar | map) param names
            //   - defaults scalar
            //   - defaults array longer than params array
            //   - defaults map with plural unknown param names

            // I hate to have to write things twice, because of having to keep
            // them consistent. If we had variadic functions, addf() would be
            // a variadic method, capturing the name and desc and passing them
            // plus "everything else" to work.add(). If I could return a pair
            // and use that pair as the first two args to work.add(), I'd do
            // that. But the best I can do with present C++ is to set two
            // instance variables as a side effect of addf(), and pass those
            // variables to each work.add() call. :-P

            /*------------------------- Callables --------------------------*/

            // Arbitrary Callable with/out required params
            addf("free1", "free1");
            work.add(name, desc, free1);
            addf("free1_req", "free1");
            work.add(name, desc, free1, required);
            // Subclass non-const method with/out required params
            addf("Dmethod1", "method1");
            work.add(name, desc, &Dispatcher::method1);
            addf("Dmethod1_req", "method1");
            work.add(name, desc, &Dispatcher::method1, required);
            // Subclass const method with/out required params
            addf("Dcmethod1", "cmethod1");
            work.add(name, desc, &Dispatcher::cmethod1);
            addf("Dcmethod1_req", "cmethod1");
            work.add(name, desc, &Dispatcher::cmethod1, required);
            // Non-subclass method with/out required params
            addf("method1", "method1");
            work.add(name, desc, boost::bind(&Vars::method1, boost::ref(v), _1));
            addf("method1_req", "method1");
            work.add(name, desc, boost::bind(&Vars::method1, boost::ref(v), _1), required);

            /*--------------- Arbitrary params, array style ----------------*/

            // (Free function | static method) with (no | arbitrary) params, array style
            addf("free0_array", "free0");
            work.add(name, desc, free0);
            addf("freena_array", "freena");
            work.add(name, desc, freena);
            addf("freenb_array", "freenb");
            work.add(name, desc, freenb);
            addf("smethod0_array", "smethod0");
            work.add(name, desc, &Vars::smethod0);
            addf("smethodna_array", "smethodna");
            work.add(name, desc, &Vars::smethodna);
            addf("smethodnb_array", "smethodnb");
            work.add(name, desc, &Vars::smethodnb);
            // Non-static method with (no | arbitrary) params, array style
            addf("method0_array", "method0");
            work.add(name, desc, &Vars::method0, boost::lambda::var(v));
            addf("methodna_array", "methodna");
            work.add(name, desc, &Vars::methodna, boost::lambda::var(v));
            addf("methodnb_array", "methodnb");
            work.add(name, desc, &Vars::methodnb, boost::lambda::var(v));

            /*---------------- Arbitrary params, map style -----------------*/

            // We lay out each params list as an array, also each array of
            // default values we'll register. We'll zip these into
            // (param=value) maps. Why not define them as maps and just
            // extract the keys and values to arrays? Because that wouldn't
            // give us the right params-list order.

            // freena(), methodna(), cmethodna(), smethodna() all take same param list.
            // Same for freenb() et al.
            params = LLSDMap("a", LLSDArray("b")("i")("f")("d")("cp"))
                            ("b", LLSDArray("s")("uuid")("date")("uri")("bin"));
            cout << "params:\n" << params << "\nparams[\"a\"]:\n" << params["a"] << "\nparams[\"b\"]:\n" << params["b"] << std::endl;
            // default LLSD::Binary value   
            std::vector<U8> binary;
            for (size_t ix = 0, h = 0xaa; ix < 6; ++ix, h += 0x11)
            {
                binary.push_back(h);
            }
            // Full defaults arrays. We actually don't care what the LLUUID or
            // LLDate values are, as long as they're different from the
            // LLUUID() and LLDate() default values so inspect() will report
            // them.
            dft_array_full = LLSDMap("a", LLSDArray(true)(17)(3.14)(123456.78)("classic"))
                                    ("b", LLSDArray("string")
                                                   (LLUUID::generateNewID())
                                                   (LLDate::now())
                                                   (LLURI("http://www.ietf.org/rfc/rfc3986.txt"))
                                                   (binary));
            cout << "dft_array_full:\n" << dft_array_full << std::endl;
            // Partial defaults arrays.
            foreach(LLSD::String a, ab)
            {
                LLSD::Integer partition(std::min(partial_offset, dft_array_full[a].size()));
                dft_array_partial[a] =
                    llsd_copy_array(dft_array_full[a].beginArray() + partition,
                                    dft_array_full[a].endArray());
            }
            cout << "dft_array_partial:\n" << dft_array_partial << std::endl;

            foreach(LLSD::String a, ab)
            {
                // Generate full defaults maps by zipping (params, dft_array_full).
                dft_map_full[a] = zipmap(params[a], dft_array_full[a]);

                // Generate partial defaults map by zipping alternate entries from
                // (params, dft_array_full). Part of the point of using map-style
                // defaults is to allow any subset of the target function's
                // parameters to be optional, not just the rightmost.
                for (LLSD::Integer ix = 0, ixend = params[a].size(); ix < ixend; ix += 2)
                {
                    dft_map_partial[a][params[a][ix].asString()] = dft_array_full[a][ix];
                }
            }
            cout << "dft_map_full:\n" << dft_map_full << "\ndft_map_partial:\n" << dft_map_partial << '\n';

            // (Free function | static method) with (no | arbitrary) params,
            // map style, no (empty array) defaults
            addf("free0_map", "free0");
            work.add(name, desc, free0, LLSD::emptyArray());
            addf("smethod0_map", "smethod0");
            work.add(name, desc, &Vars::smethod0, LLSD::emptyArray());
            addf("freena_map_allreq", "freena");
            work.add(name, desc, freena, params["a"]);
            addf("freenb_map_allreq", "freenb");
            work.add(name, desc, freenb, params["b"]);
            addf("smethodna_map_allreq", "smethodna");
            work.add(name, desc, &Vars::smethodna, params["a"]);
            addf("smethodnb_map_allreq", "smethodnb");
            work.add(name, desc, &Vars::smethodnb, params["b"]);
            // Non-static method with (no | arbitrary) params, map style, no
            // (empty array) defaults
            addf("method0_map", "method0");
            work.add(name, desc, &Vars::method0, var(v), LLSD::emptyArray());
            addf("methodna_map_allreq", "methodna");
            work.add(name, desc, &Vars::methodna, var(v), params["a"]);
            addf("methodnb_map_allreq", "methodnb");
            work.add(name, desc, &Vars::methodnb, var(v), params["b"]);

            // Except for the "more (array | map) defaults than params" error
            // cases, tested separately below, the (partial | full)(array |
            // map) defaults cases don't apply to no-params functions/methods.
            // So eliminate free0, smethod0, method0 from the cases below.

            // (Free function | static method) with arbitrary params, map
            // style, partial (array | map) defaults
            addf("freena_map_leftreq", "freena");
            work.add(name, desc, freena, params["a"], dft_array_partial["a"]);
            addf("freenb_map_leftreq", "freenb");
            work.add(name, desc, freenb, params["b"], dft_array_partial["b"]);
            addf("smethodna_map_leftreq", "smethodna");
            work.add(name, desc, &Vars::smethodna, params["a"], dft_array_partial["a"]);
            addf("smethodnb_map_leftreq", "smethodnb");
            work.add(name, desc, &Vars::smethodnb, params["b"], dft_array_partial["b"]);
            addf("freena_map_skipreq", "freena");
            work.add(name, desc, freena, params["a"], dft_map_partial["a"]);
            addf("freenb_map_skipreq", "freenb");
            work.add(name, desc, freenb, params["b"], dft_map_partial["b"]);
            addf("smethodna_map_skipreq", "smethodna");
            work.add(name, desc, &Vars::smethodna, params["a"], dft_map_partial["a"]);
            addf("smethodnb_map_skipreq", "smethodnb");
            work.add(name, desc, &Vars::smethodnb, params["b"], dft_map_partial["b"]);
            // Non-static method with arbitrary params, map style, partial
            // (array | map) defaults
            addf("methodna_map_leftreq", "methodna");
            work.add(name, desc, &Vars::methodna, var(v), params["a"], dft_array_partial["a"]);
            addf("methodnb_map_leftreq", "methodnb");
            work.add(name, desc, &Vars::methodnb, var(v), params["b"], dft_array_partial["b"]);
            addf("methodna_map_skipreq", "methodna");
            work.add(name, desc, &Vars::methodna, var(v), params["a"], dft_map_partial["a"]);
            addf("methodnb_map_skipreq", "methodnb");
            work.add(name, desc, &Vars::methodnb, var(v), params["b"], dft_map_partial["b"]);

            // (Free function | static method) with arbitrary params, map
            // style, full (array | map) defaults
            addf("freena_map_adft", "freena");
            work.add(name, desc, freena, params["a"], dft_array_full["a"]);
            addf("freenb_map_adft", "freenb");
            work.add(name, desc, freenb, params["b"], dft_array_full["b"]);
            addf("smethodna_map_adft", "smethodna");
            work.add(name, desc, &Vars::smethodna, params["a"], dft_array_full["a"]);
            addf("smethodnb_map_adft", "smethodnb");
            work.add(name, desc, &Vars::smethodnb, params["b"], dft_array_full["b"]);
            addf("freena_map_mdft", "freena");
            work.add(name, desc, freena, params["a"], dft_map_full["a"]);
            addf("freenb_map_mdft", "freenb");
            work.add(name, desc, freenb, params["b"], dft_map_full["b"]);
            addf("smethodna_map_mdft", "smethodna");
            work.add(name, desc, &Vars::smethodna, params["a"], dft_map_full["a"]);
            addf("smethodnb_map_mdft", "smethodnb");
            work.add(name, desc, &Vars::smethodnb, params["b"], dft_map_full["b"]);
            // Non-static method with arbitrary params, map style, full
            // (array | map) defaults
            addf("methodna_map_adft", "methodna");
            work.add(name, desc, &Vars::methodna, var(v), params["a"], dft_array_full["a"]);
            addf("methodnb_map_adft", "methodnb");
            work.add(name, desc, &Vars::methodnb, var(v), params["b"], dft_array_full["b"]);
            addf("methodna_map_mdft", "methodna");
            work.add(name, desc, &Vars::methodna, var(v), params["a"], dft_map_full["a"]);
            addf("methodnb_map_mdft", "methodnb");
            work.add(name, desc, &Vars::methodnb, var(v), params["b"], dft_map_full["b"]);

            // All the above are expected to succeed, and are setup for the
            // tests to follow. Registration error cases are exercised as
            // tests rather than as test setup.
        }

        void addf(const std::string& n, const std::string& d)
        {
            // This method is to capture in our own FuncMap the name and
            // description of every registered function, for metadata query
            // testing.
            funcs[n] = d;
            // See constructor for rationale for setting these instance vars.
            this->name = n;
            this->desc = d;
        }

        void verify_funcs()
        {
            // Copy funcs to a temp map of same type.
            FuncMap forgotten(funcs.begin(), funcs.end());
            // LLEventDispatcher intentionally provides only const_iterator:
            // since dereferencing that iterator generates values on the fly,
            // it's meaningless to have a modifiable iterator. But since our
            // 'work' object isn't const, by default BOOST_FOREACH() wants to
            // use non-const iterators. Persuade it to use the const_iterator.
            foreach(LLEventDispatcher::NameDesc nd, const_cast<const Dispatcher&>(work))
            {
                FuncMap::iterator found = forgotten.find(nd.first);
                ensure(STRINGIZE("LLEventDispatcher records function '" << nd.first
                                 << "' we didn't enter"),
                       found != forgotten.end());
                ensure_equals(STRINGIZE("LLEventDispatcher desc '" << nd.second <<
                                        "' doesn't match what we entered: '" << found->second << "'"),
                              nd.second, found->second);
                // found in our map the name from LLEventDispatcher, good, erase
                // our map entry
                forgotten.erase(found);
            }
            if (! forgotten.empty())
            {
                std::ostringstream out;
                out << "LLEventDispatcher failed to report";
                const char* delim = ": ";
                foreach(const FuncMap::value_type& fme, forgotten)
                {
                    out << delim << fme.first;
                    delim = ", ";
                }
                ensure(out.str(), false);
            }
        }

        void ensure_has(const std::string& outer, const std::string& inner)
        {
            ensure(STRINGIZE("'" << outer << "' does not contain '" << inner << "'").c_str(),
                   outer.find(inner) != std::string::npos);
        }

        void call_exc(const std::string& func, const LLSD& args, const std::string& exc_frag)
        {
            std::string threw;
            try
            {
                work(func, args);
            }
            catch (const std::runtime_error& e)
            {
                cout << "*** " << e.what() << '\n';
                threw = e.what();
            }
            ensure_has(threw, exc_frag);
        }

        LLSD getMetadata(const std::string& name)
        {
            LLSD meta(work.getMetadata(name));
            ensure(STRINGIZE("No metadata for " << name), meta.isDefined());
            return meta;
        }

        // From two related LLSD arrays, e.g. a param-names array and a values
        // array, zip them together into an LLSD map.
        LLSD zipmap(const LLSD& keys, const LLSD& values)
        {
            LLSD map;
            for (LLSD::Integer i = 0, iend = keys.size(); i < iend; ++i)
            {
                // Have to select asString() since you can index an LLSD
                // object with either String or Integer.
                map[keys[i].asString()] = values[i];
            }
            return map;
        }

        // If I call this ensure_equals(), it blocks visibility of all other
        // ensure_equals() overloads. Normally I could say 'using
        // baseclass::ensure_equals;' and fix that, but I don't know what the
        // base class is!
        void ensure_llsd(const std::string& msg, const LLSD& actual, const LLSD& expected, U32 bits)
        {
            std::ostringstream out;
            if (! msg.empty())
            {
                out << msg << ": ";
            }
            out << "expected " << expected << ", actual " << actual;
            ensure(out.str(), llsd_equals(actual, expected, bits));
        }

        void ensure_llsd(const LLSD& actual, const LLSD& expected, U32 bits)
        {
            ensure_llsd("", actual, expected, bits);
        }
    };
    typedef test_group<lleventdispatcher_data> lleventdispatcher_group;
    typedef lleventdispatcher_group::object object;
    lleventdispatcher_group lleventdispatchergrp("lleventdispatcher");

    // Call cases:
    // - (try_call | call) (explicit name | event key) (real | bogus) name
    // - Callable with args that (do | do not) match required
    // - (Free function | non-static method), no args, (array | map) style
    // - (Free function | non-static method), arbitrary args, array style with
    //   (scalar | map | array (too short | too long | just right))
    //   [trap LL_WARNS for too-long case?]
    // - (Free function | non-static method), arbitrary args, map style with
    //   (scalar | array | map (all | too many | holes (with | without) defaults))
    // - const char* param gets ("" | NULL)

    // Query cases:
    // - Iterate over all (with | without) remove()
    // - getDispatchKey()
    // - Callable style (with | without) required
    // - (Free function | non-static method), array style, (no | arbitrary) params
    // - (Free function | non-static method), map style, (no | arbitrary) params,
    //   (empty | full | partial (array | map)) defaults

    template<> template<>
    void object::test<1>()
    {
        set_test_name("map-style registration with non-array params");
        // Pass "param names" as scalar or as map
        LLSD attempts(LLSDArray(17)(LLSDMap("pi", 3.14)("two", 2)));
        foreach(LLSD ae, inArray(attempts))
        {
            std::string threw;
            try
            {
                work.add("freena_err", "freena", freena, ae);
            }
            catch (const std::exception& e)
            {
                threw = e.what();
            }
            ensure_has(threw, "must be an array");
        }
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("map-style registration with badly-formed defaults");
        std::string threw;
        try
        {
            work.add("freena_err", "freena", freena, LLSDArray("a")("b"), 17);
        }
        catch (const std::exception& e)
        {
            threw = e.what();
        }
        ensure_has(threw, "must be a map or an array");
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("map-style registration with too many array defaults");
        std::string threw;
        try
        {
            work.add("freena_err", "freena", freena,
                     LLSDArray("a")("b"),
                     LLSDArray(17)(0.9)("gack"));
        }
        catch (const std::exception& e)
        {
            threw = e.what();
        }
        ensure_has(threw, "shorter than");
    }

    template<> template<>
    void object::test<4>()
    {
        set_test_name("map-style registration with too many map defaults");
        std::string threw;
        try
        {
            work.add("freena_err", "freena", freena,
                     LLSDArray("a")("b"),
                     LLSDMap("b", 17)("foo", 3.14)("bar", "sinister"));
        }
        catch (const std::exception& e)
        {
            threw = e.what();
        }
        ensure_has(threw, "nonexistent params");
        ensure_has(threw, "foo");
        ensure_has(threw, "bar");
    }

    template<> template<>
    void object::test<5>()
    {
        set_test_name("query all");
        verify_funcs();
    }

    template<> template<>
    void object::test<6>()
    {
        set_test_name("query all with remove()");
        ensure("remove('bogus') returned true", ! work.remove("bogus"));
        ensure("remove('real') returned false", work.remove("free1"));
        // Of course, remove that from 'funcs' too...
        funcs.erase("free1");
        verify_funcs();
    }

    template<> template<>
    void object::test<7>()
    {
        set_test_name("getDispatchKey()");
        ensure_equals(work.getDispatchKey(), "op");
    }

    template<> template<>
    void object::test<8>()
    {
        set_test_name("query Callables with/out required params");
        LLSD names(LLSDArray("free1")("Dmethod1")("Dcmethod1")("method1"));
        foreach(LLSD nm, inArray(names))
        {
            LLSD metadata(getMetadata(nm));
            ensure_equals("name mismatch", metadata["name"], nm);
            ensure_equals(metadata["desc"].asString(), funcs[nm]);
            ensure("should not have required structure", metadata["required"].isUndefined());
            ensure("should not have optional", metadata["optional"].isUndefined());

            std::string name_req(nm.asString() + "_req");
            metadata = getMetadata(name_req);
            ensure_equals(metadata["name"].asString(), name_req);
            ensure_equals(metadata["desc"].asString(), funcs[name_req]);
            ensure_equals("required mismatch", required, metadata["required"]);
            ensure("should not have optional", metadata["optional"].isUndefined());
        }
    }

    template<> template<>
    void object::test<9>()
    {
        set_test_name("query array-style functions/methods");
        // Associate each registered name with expected arity.
        LLSD expected(LLSDArray
                      (LLSDArray
                       (0)(LLSDArray("free0_array")("smethod0_array")("method0_array")))
                      (LLSDArray
                       (5)(LLSDArray("freena_array")("smethodna_array")("methodna_array")))
                      (LLSDArray
                       (5)(LLSDArray("freenb_array")("smethodnb_array")("methodnb_array"))));
        foreach(LLSD ae, inArray(expected))
        {
            LLSD::Integer arity(ae[0].asInteger());
            LLSD names(ae[1]);
            LLSD req(LLSD::emptyArray());
            if (arity)
                req[arity - 1] = LLSD();
            foreach(LLSD nm, inArray(names))
            {
                LLSD metadata(getMetadata(nm));
                ensure_equals("name mismatch", metadata["name"], nm);
                ensure_equals(metadata["desc"].asString(), funcs[nm]);
                ensure_equals(STRINGIZE("mismatched required for " << nm.asString()),
                              metadata["required"], req);
                ensure("should not have optional", metadata["optional"].isUndefined());
            }
        }
    }

    template<> template<>
    void object::test<10>()
    {
        set_test_name("query map-style no-params functions/methods");
        // - (Free function | non-static method), map style, no params (ergo
        //   no defaults)
        LLSD names(LLSDArray("free0_map")("smethod0_map")("method0_map"));
        foreach(LLSD nm, inArray(names))
        {
            LLSD metadata(getMetadata(nm));
            ensure_equals("name mismatch", metadata["name"], nm);
            ensure_equals(metadata["desc"].asString(), funcs[nm]);
            ensure("should not have required",
                   (metadata["required"].isUndefined() || metadata["required"].size() == 0));
            ensure("should not have optional", metadata["optional"].isUndefined());
        }
    }

    template<> template<>
    void object::test<11>()
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
        LLSD equivalences(LLSDArray
                          (LLSDArray("freena_map_adft")("freena_map_mdft"))
                          (LLSDArray("freenb_map_adft")("freenb_map_mdft"))
                          (LLSDArray("smethodna_map_adft")("smethodna_map_mdft"))
                          (LLSDArray("smethodnb_map_adft")("smethodnb_map_mdft"))
                          (LLSDArray("methodna_map_adft")("methodna_map_mdft"))
                          (LLSDArray("methodnb_map_adft")("methodnb_map_mdft")));
        foreach(LLSD eq, inArray(equivalences))
        {
            LLSD adft(eq[0]);
            LLSD mdft(eq[1]);
            // We can't just compare the results of the two getMetadata()
            // calls, because they contain ["name"], which are different. So
            // capture them, verify that each ["name"] is as expected, then
            // remove for comparing the rest.
            LLSD ameta(getMetadata(adft));
            LLSD mmeta(getMetadata(mdft));
            ensure_equals("adft name", adft, ameta["name"]);
            ensure_equals("mdft name", mdft, mmeta["name"]);
            ameta.erase("name");
            mmeta.erase("name");
            ensure_equals(STRINGIZE("metadata for " << adft.asString()
                                    << " vs. " << mdft.asString()),
                          ameta, mmeta);
        }
    }

    template<> template<>
    void object::test<12>()
    {
        set_test_name("query map-style arbitrary-params functions/methods");
        // - (Free function | non-static method), map style, arbitrary params,
        //   (empty | full | partial (array | map)) defaults

        // Generate maps containing all parameter names for cases in which all
        // params are required. Also maps containing left requirements for
        // partial defaults arrays. Also defaults maps from defaults arrays.
        LLSD allreq, leftreq, rightdft;
        foreach(LLSD::String a, ab)
        {
            // The map in which all params are required uses params[a] as
            // keys, with all isUndefined() as values. We can accomplish that
            // by passing zipmap() an empty values array.
            allreq[a] = zipmap(params[a], LLSD::emptyArray());
            // Same for leftreq, save that we use the subset of the params not
            // supplied by dft_array_partial[a].
            LLSD::Integer partition(params[a].size() - dft_array_partial[a].size());
            leftreq[a] = zipmap(llsd_copy_array(params[a].beginArray(),
                                                params[a].beginArray() + partition),
                                LLSD::emptyArray());
            // Generate map pairing dft_array_partial[a] values with their
            // param names.
            rightdft[a] = zipmap(llsd_copy_array(params[a].beginArray() + partition,
                                                 params[a].endArray()),
                                 dft_array_partial[a]);
        }
        cout << "allreq:\n" << allreq << "\nleftreq:\n" << leftreq << "\nrightdft:\n" << rightdft << std::endl;

        // Generate maps containing parameter names not provided by the
        // dft_map_partial maps.
        LLSD skipreq(allreq);
        foreach(LLSD::String a, ab)
        {
            foreach(const MapEntry& me, inMap(dft_map_partial[a]))
            {
                skipreq[a].erase(me.first);
            }
        }
        cout << "skipreq:\n" << skipreq << std::endl;

        LLSD groups(LLSDArray       // array of groups

                    (LLSDArray      // group
                     (LLSDArray("freena_map_allreq")("smethodna_map_allreq")("methodna_map_allreq"))
                     (LLSDArray(allreq["a"])(LLSD()))) // required, optional

                    (LLSDArray        // group
                     (LLSDArray("freenb_map_allreq")("smethodnb_map_allreq")("methodnb_map_allreq"))
                     (LLSDArray(allreq["b"])(LLSD()))) // required, optional

                    (LLSDArray        // group
                     (LLSDArray("freena_map_leftreq")("smethodna_map_leftreq")("methodna_map_leftreq"))
                     (LLSDArray(leftreq["a"])(rightdft["a"]))) // required, optional

                    (LLSDArray        // group
                     (LLSDArray("freenb_map_leftreq")("smethodnb_map_leftreq")("methodnb_map_leftreq"))
                     (LLSDArray(leftreq["b"])(rightdft["b"]))) // required, optional

                    (LLSDArray        // group
                     (LLSDArray("freena_map_skipreq")("smethodna_map_skipreq")("methodna_map_skipreq"))
                     (LLSDArray(skipreq["a"])(dft_map_partial["a"]))) // required, optional

                    (LLSDArray        // group
                     (LLSDArray("freenb_map_skipreq")("smethodnb_map_skipreq")("methodnb_map_skipreq"))
                     (LLSDArray(skipreq["b"])(dft_map_partial["b"]))) // required, optional

                    // We only need mention the full-map-defaults ("_mdft" suffix)
                    // registrations, having established their equivalence with the
                    // full-array-defaults ("_adft" suffix) registrations in another test.
                    (LLSDArray        // group
                     (LLSDArray("freena_map_mdft")("smethodna_map_mdft")("methodna_map_mdft"))
                     (LLSDArray(LLSD::emptyMap())(dft_map_full["a"]))) // required, optional

                    (LLSDArray        // group
                     (LLSDArray("freenb_map_mdft")("smethodnb_map_mdft")("methodnb_map_mdft"))
                     (LLSDArray(LLSD::emptyMap())(dft_map_full["b"])))); // required, optional

        foreach(LLSD grp, inArray(groups))
        {
            // Internal structure of each group in 'groups':
            LLSD names(grp[0]);
            LLSD required(grp[1][0]);
            LLSD optional(grp[1][1]);
            cout << "For " << names << ",\n" << "required:\n" << required << "\noptional:\n" << optional << std::endl;

            // Loop through 'names'
            foreach(LLSD nm, inArray(names))
            {
                LLSD metadata(getMetadata(nm));
                ensure_equals("name mismatch", metadata["name"], nm);
                ensure_equals(nm.asString(), metadata["desc"].asString(), funcs[nm]);
                ensure_equals(STRINGIZE(nm << " required mismatch"),
                              metadata["required"], required);
                ensure_equals(STRINGIZE(nm << " optional mismatch"),
                              metadata["optional"], optional);
            }
        }
    }

    template<> template<>
    void object::test<13>()
    {
        set_test_name("try_call()");
        ensure("try_call(bogus name, LLSD()) returned true", ! work.try_call("freek", LLSD()));
        ensure("try_call(bogus name) returned true", ! work.try_call(LLSDMap("op", "freek")));
        ensure("try_call(real name, LLSD()) returned false", work.try_call("free0_array", LLSD()));
        ensure("try_call(real name) returned false", work.try_call(LLSDMap("op", "free0_map")));
    }

    template<> template<>
    void object::test<14>()
    {
        set_test_name("call with bad name");
        call_exc("freek", LLSD(), "not found");
        // We don't have a comparable helper function for the one-arg
        // operator() method, and it's not worth building one just for this
        // case. Write it out.
        std::string threw;
        try
        {
            work(LLSDMap("op", "freek"));
        }
        catch (const std::runtime_error& e)
        {
            cout << "*** " << e.what() << "\n";
            threw = e.what();
        }
        ensure_has(threw, "bad");
        ensure_has(threw, "op");
        ensure_has(threw, "freek");
    }

    template<> template<>
    void object::test<15>()
    {
        set_test_name("call with event key");
        // We don't need a separate test for operator()(string, LLSD) with
        // valid name, because all the rest of the tests exercise that case.
        // The one we don't exercise elsewhere is operator()(LLSD) with valid
        // name, so here it is.
        work(LLSDMap("op", "free0_map"));
        ensure_equals(g.i, 17);
    }

    // Cannot be defined inside function body... remind me again why we use C++...  :-P
    struct CallablesTriple
    {
        std::string name, name_req;
        LLSD& llsd;
    };

    template<> template<>
    void object::test<16>()
    {
        set_test_name("call Callables");
        CallablesTriple tests[] =
        {
            { "free1",     "free1_req",     g.llsd },
            { "Dmethod1",  "Dmethod1_req",  work.llsd },
            { "Dcmethod1", "Dcmethod1_req", work.llsd },
            { "method1",   "method1_req",   v.llsd }
        };
        // Arbitrary LLSD value that we should be able to pass to Callables
        // without 'required', but should not be able to pass to Callables
        // with 'required'.
        LLSD answer(42);
        // LLSD value matching 'required' according to llsd_matches() rules.
        LLSD matching(LLSDMap("d", 3.14)("array", LLSDArray("answer")(true)(answer)));
        // Okay, walk through 'tests'.
        foreach(const CallablesTriple& tr, tests)
        {
            // Should be able to pass 'answer' to Callables registered
            // without 'required'.
            work(tr.name, answer);
            ensure_equals("answer mismatch", tr.llsd, answer);
            // Should NOT be able to pass 'answer' to Callables registered
            // with 'required'.
            call_exc(tr.name_req, answer, "bad request");
            // But SHOULD be able to pass 'matching' to Callables registered
            // with 'required'.
            work(tr.name_req, matching);
            ensure_equals("matching mismatch", tr.llsd, matching);
        }
    }

    template<> template<>
    void object::test<17>()
    {
        set_test_name("passing wrong args to (map | array)-style registrations");

        // Pass scalar/map to array-style functions, scalar/array to map-style
        // functions. As that validation happens well before we engage the
        // argument magic, it seems pointless to repeat this with every
        // variation: (free function | non-static method), (no | arbitrary)
        // args. We should only need to engage it for one map-style
        // registration and one array-style registration.
        std::string array_exc("needs an args array");
        call_exc("free0_array", 17, array_exc);
        call_exc("free0_array", LLSDMap("pi", 3.14), array_exc);

        std::string map_exc("needs a map");
        call_exc("free0_map", 17, map_exc);
        call_exc("free0_map", LLSDArray("a")("b"), map_exc);
    }

    struct FunctionsTriple
    {
        std::string name1, name2;
        Vars* vars;
    };

    template<> template<>
    void object::test<18>()
    {
        set_test_name("call no-args functions");
        FunctionsTriple tests[] =
        {
            { "free0_array",    "free0_map",    &g },
            { "smethod0_array", "smethod0_map", &g },
            { "method0_array",  "method0_map",  &v }
        };
        foreach(const FunctionsTriple& tr, tests)
        {
            // Both the global and stack Vars instances are automatically
            // cleared at the start of each test<n> method. But since we're
            // calling these things several different times in the same
            // test<n> method, manually reset the Vars between each.
            *tr.vars = Vars();
            ensure_equals(tr.vars->i, 0);
            // array-style call with empty array (or LLSD(), should be equivalent)
            work(tr.name1, LLSD());
            ensure_equals(tr.vars->i, 17);

            *tr.vars = Vars();
            // map-style call with empty map (or LLSD(), should be equivalent)
            work(tr.name2, LLSD());
            ensure_equals(tr.vars->i, 17);
        }
    }

    // Break out function to return this data because we use it in a couple
    // different tests.
    std::vector<FunctionsTriple> array_funcs(Vars& v)
    {
        FunctionsTriple tests[] =
        {
            { "freena_array",    "freenb_array",    &g },
            { "smethodna_array", "smethodnb_array", &g },
            { "methodna_array",  "methodnb_array",  &v }
        };
        return std::vector<FunctionsTriple>(boost::begin(tests), boost::end(tests));
    }

    template<> template<>
    void object::test<19>()
    {
        set_test_name("call array-style functions with too-short arrays");
        // Could have two different too-short arrays, one for *na and one for
        // *nb, but since they both take 5 params...
        LLSD tooshort(LLSDArray("this")("array")("too")("short"));
        foreach(const FunctionsTriple& tr, array_funcs(v))
        {
            call_exc(tr.name1, tooshort, "requires more arguments");
            call_exc(tr.name2, tooshort, "requires more arguments");
        }
    }

    template<> template<>
    void object::test<20>()
    {
        set_test_name("call array-style functions with (just right | too long) arrays");
        std::vector<U8> binary;
        for (size_t h(0x01), i(0); i < 5; h+= 0x22, ++i)
        {
            binary.push_back(h);
        }
        LLSD args(LLSDMap("a", LLSDArray(true)(17)(3.14)(123.456)("char*"))
                         ("b", LLSDArray("string")
                                        (LLUUID("01234567-89ab-cdef-0123-456789abcdef"))
                                        (LLDate("2011-02-03T15:07:00Z"))
                                        (LLURI("http://secondlife.com"))
                                        (binary)));
        LLSD argsplus(args);
        argsplus["a"].append("bogus");
        argsplus["b"].append("bogus");
        LLSD expect;
        foreach(LLSD::String a, ab)
        {
            expect[a] = zipmap(params[a], args[a]);
        }
        // Adjust expect["a"]["cp"] for special Vars::cp treatment.
        expect["a"]["cp"] = std::string("'") + expect["a"]["cp"].asString() + "'";
        cout << "expect: " << expect << '\n';

        // Use substantially the same logic for args and argsplus
        LLSD argsarrays(LLSDArray(args)(argsplus));
        // So i==0 selects 'args', i==1 selects argsplus
        for (LLSD::Integer i(0), iend(argsarrays.size()); i < iend; ++i)
        {
            foreach(const FunctionsTriple& tr, array_funcs(v))
            {
                // Get tr.name1 and tr.name2 into a map keyed by ["a"] and ["b"]
                LLSD funcs(LLSDMap("a", tr.name1)("b", tr.name2));

                // So now we can call tr.name1 (as funcs["a"]) with the "a"
                // params, etc.
                foreach(LLSD::String a, ab)
                {
                    // Reset the Vars instance before each call
                    *tr.vars = Vars();
                    work(funcs[a], argsarrays[i][a]);
                    ensure_llsd(STRINGIZE(funcs[a].asString() <<
                                          ": expect[\"" << a << "\"] mismatch"),
                                tr.vars->inspect(), expect[a], 7); // 7 bits ~= 2 decimal digits

                    // TODO: in the i==1 or argsplus case, intercept LL_WARNS
                    // output? Even without that, using argsplus verifies that
                    // passing too many args isn't fatal; it works -- but
                    // would be nice to notice the warning too.
                }
            }
        }
    }
} // namespace tut
