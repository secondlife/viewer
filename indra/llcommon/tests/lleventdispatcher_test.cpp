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
namespace tut
{
    void ensure_has(const std::string& outer, const std::string& inner)
    {
        ensure(stringize("'", outer, "' does not contain '", inner, "'"),
               outer.find(inner) != std::string::npos);
    }

    template <typename CALLABLE>
    std::string call_exc(CALLABLE&& func, const std::string& exc_frag)
    {
        std::string what =
            catch_what<LLEventDispatcher::DispatchError>(std::forward<CALLABLE>(func));
        ensure_has(what, exc_frag);
        return what;
    }

    template <typename CALLABLE>
    void call_logerr(CALLABLE&& func, const std::string& frag)
    {
        CaptureLog capture;
        // the error should be logged; we just need to stop the exception
        // propagating
        catch_what<LLEventDispatcher::DispatchError>(std::forward<CALLABLE>(func));
        capture.messageWith(frag);
    }

    struct lleventdispatcher_data
    {
        Debug debug{"test"};
        WrapLLErrs redirect;
        Dispatcher work;
        Vars v;
        std::string name, desc;
        // Capture our own copy of all registered functions' descriptions
        typedef std::map<std::string, std::string> DescMap;
        DescMap descs;
        // Capture the Vars instance on which we expect each function to operate
        typedef std::map<std::string, Vars*> VarsMap;
        VarsMap funcvars;
        // Required structure for Callables with requirements
        LLSD required;
        // Parameter names for freena(), freenb()
        LLSD params;
        // Full, partial defaults arrays for params for freena(), freenb()
        LLSD dft_array_full, dft_array_partial;
        // Start index of partial defaults arrays
        const size_t partial_offset;
        // Full, partial defaults maps for params for freena(), freenb()
        LLSD dft_map_full, dft_map_partial;
        // Most of the above are indexed by "a" or "b". Useful to have an
        // array containing those strings for iterating.
        std::vector<LLSD::String> ab;

        lleventdispatcher_data():
            work("test dispatcher", "op"),
            // map {d=double, array=[3 elements]}
            required(LLSDMap("d", LLSD::Real(0))("array", llsd::array(LLSD(), LLSD(), LLSD()))),
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
            addf("free1", "free1", &g);
            work.add(name, desc, free1);
            addf("free1_req", "free1", &g);
            work.add(name, desc, free1, required);
            // Subclass non-const method with/out required params
            addf("Dmethod1", "method1", NULL);
            work.add(name, desc, &Dispatcher::method1);
            addf("Dmethod1_req", "method1", NULL);
            work.add(name, desc, &Dispatcher::method1, required);
            // Subclass const method with/out required params
            addf("Dcmethod1", "cmethod1", NULL);
            work.add(name, desc, &Dispatcher::cmethod1);
            addf("Dcmethod1_req", "cmethod1", NULL);
            work.add(name, desc, &Dispatcher::cmethod1, required);
            // Non-subclass method with/out required params
            addf("method1", "method1", &v);
            work.add(name, desc, [this](const LLSD& args){ return v.method1(args); });
            addf("method1_req", "method1", &v);
            work.add(name, desc, [this](const LLSD& args){ return v.method1(args); }, required);

            /*--------------- Arbitrary params, array style ----------------*/

            // (Free function | static method) with (no | arbitrary) params, array style
            addf("free0_array", "free0", &g);
            work.add(name, desc, free0);
            addf("freena_array", "freena", &g);
            work.add(name, desc, freena);
            addf("freenb_array", "freenb", &g);
            work.add(name, desc, freenb);
            addf("smethod0_array", "smethod0", &g);
            work.add(name, desc, &Vars::smethod0);
            addf("smethodna_array", "smethodna", &g);
            work.add(name, desc, &Vars::smethodna);
            addf("smethodnb_array", "smethodnb", &g);
            work.add(name, desc, &Vars::smethodnb);
            // Non-static method with (no | arbitrary) params, array style
            addf("method0_array", "method0", &v);
            work.add(name, desc, &Vars::method0, boost::lambda::var(v));
            addf("methodna_array", "methodna", &v);
            work.add(name, desc, &Vars::methodna, boost::lambda::var(v));
            addf("methodnb_array", "methodnb", &v);
            work.add(name, desc, &Vars::methodnb, boost::lambda::var(v));

            /*---------------- Arbitrary params, map style -----------------*/

            // We lay out each params list as an array, also each array of
            // default values we'll register. We'll zip these into
            // (param=value) maps. Why not define them as maps and just
            // extract the keys and values to arrays? Because that wouldn't
            // give us the right params-list order.

            // freena(), methodna(), cmethodna(), smethodna() all take same param list.
            // Same for freenb() et al.
            params = LLSDMap("a", llsd::array("b", "i", "f", "d", "cp"))
                            ("b", llsd::array("s", "uuid", "date", "uri", "bin"));
            debug("params:\n",
                  params, "\n"
                  "params[\"a\"]:\n",
                  params["a"], "\n"
                  "params[\"b\"]:\n",
                  params["b"]);
            // default LLSD::Binary value
            std::vector<U8> binary;
            for (size_t ix = 0, h = 0xaa; ix < 6; ++ix, h += 0x11)
            {
                binary.push_back((U8)h);
            }
            // Full defaults arrays. We actually don't care what the LLUUID or
            // LLDate values are, as long as they're different from the
            // LLUUID() and LLDate() default values so inspect() will report
            // them.
            dft_array_full = LLSDMap("a", llsd::array(true, 17, 3.14, 123456.78, "classic"))
                                    ("b", llsd::array("string",
                                                      LLUUID::generateNewID(),
                                                      LLDate::now(),
                                                      LLURI("http://www.ietf.org/rfc/rfc3986.txt"),
                                                      binary));
            debug("dft_array_full:\n",
                  dft_array_full);
            // Partial defaults arrays.
            for (LLSD::String a: ab)
            {
                LLSD::Integer partition(std::min(partial_offset, dft_array_full[a].size()));
                dft_array_partial[a] =
                    llsd_copy_array(dft_array_full[a].beginArray() + partition,
                                    dft_array_full[a].endArray());
            }
            debug("dft_array_partial:\n",
                  dft_array_partial);

            for(LLSD::String a: ab)
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
            debug("dft_map_full:\n",
                  dft_map_full, "\n"
                  "dft_map_partial:\n",
                  dft_map_partial);

            // (Free function | static method) with (no | arbitrary) params,
            // map style, no (empty array) defaults
            addf("free0_map", "free0", &g);
            work.add(name, desc, free0, LLSD::emptyArray());
            addf("smethod0_map", "smethod0", &g);
            work.add(name, desc, &Vars::smethod0, LLSD::emptyArray());
            addf("freena_map_allreq", "freena", &g);
            work.add(name, desc, freena, params["a"]);
            addf("freenb_map_allreq", "freenb", &g);
            work.add(name, desc, freenb, params["b"]);
            addf("smethodna_map_allreq", "smethodna", &g);
            work.add(name, desc, &Vars::smethodna, params["a"]);
            addf("smethodnb_map_allreq", "smethodnb", &g);
            work.add(name, desc, &Vars::smethodnb, params["b"]);
            // Non-static method with (no | arbitrary) params, map style, no
            // (empty array) defaults
            addf("method0_map", "method0", &v);
            work.add(name, desc, &Vars::method0, var(v), LLSD::emptyArray());
            addf("methodna_map_allreq", "methodna", &v);
            work.add(name, desc, &Vars::methodna, var(v), params["a"]);
            addf("methodnb_map_allreq", "methodnb", &v);
            work.add(name, desc, &Vars::methodnb, var(v), params["b"]);

            // Except for the "more (array | map) defaults than params" error
            // cases, tested separately below, the (partial | full)(array |
            // map) defaults cases don't apply to no-params functions/methods.
            // So eliminate free0, smethod0, method0 from the cases below.

            // (Free function | static method) with arbitrary params, map
            // style, partial (array | map) defaults
            addf("freena_map_leftreq", "freena", &g);
            work.add(name, desc, freena, params["a"], dft_array_partial["a"]);
            addf("freenb_map_leftreq", "freenb", &g);
            work.add(name, desc, freenb, params["b"], dft_array_partial["b"]);
            addf("smethodna_map_leftreq", "smethodna", &g);
            work.add(name, desc, &Vars::smethodna, params["a"], dft_array_partial["a"]);
            addf("smethodnb_map_leftreq", "smethodnb", &g);
            work.add(name, desc, &Vars::smethodnb, params["b"], dft_array_partial["b"]);
            addf("freena_map_skipreq", "freena", &g);
            work.add(name, desc, freena, params["a"], dft_map_partial["a"]);
            addf("freenb_map_skipreq", "freenb", &g);
            work.add(name, desc, freenb, params["b"], dft_map_partial["b"]);
            addf("smethodna_map_skipreq", "smethodna", &g);
            work.add(name, desc, &Vars::smethodna, params["a"], dft_map_partial["a"]);
            addf("smethodnb_map_skipreq", "smethodnb", &g);
            work.add(name, desc, &Vars::smethodnb, params["b"], dft_map_partial["b"]);
            // Non-static method with arbitrary params, map style, partial
            // (array | map) defaults
            addf("methodna_map_leftreq", "methodna", &v);
            work.add(name, desc, &Vars::methodna, var(v), params["a"], dft_array_partial["a"]);
            addf("methodnb_map_leftreq", "methodnb", &v);
            work.add(name, desc, &Vars::methodnb, var(v), params["b"], dft_array_partial["b"]);
            addf("methodna_map_skipreq", "methodna", &v);
            work.add(name, desc, &Vars::methodna, var(v), params["a"], dft_map_partial["a"]);
            addf("methodnb_map_skipreq", "methodnb", &v);
            work.add(name, desc, &Vars::methodnb, var(v), params["b"], dft_map_partial["b"]);

            // (Free function | static method) with arbitrary params, map
            // style, full (array | map) defaults
            addf("freena_map_adft", "freena", &g);
            work.add(name, desc, freena, params["a"], dft_array_full["a"]);
            addf("freenb_map_adft", "freenb", &g);
            work.add(name, desc, freenb, params["b"], dft_array_full["b"]);
            addf("smethodna_map_adft", "smethodna", &g);
            work.add(name, desc, &Vars::smethodna, params["a"], dft_array_full["a"]);
            addf("smethodnb_map_adft", "smethodnb", &g);
            work.add(name, desc, &Vars::smethodnb, params["b"], dft_array_full["b"]);
            addf("freena_map_mdft", "freena", &g);
            work.add(name, desc, freena, params["a"], dft_map_full["a"]);
            addf("freenb_map_mdft", "freenb", &g);
            work.add(name, desc, freenb, params["b"], dft_map_full["b"]);
            addf("smethodna_map_mdft", "smethodna", &g);
            work.add(name, desc, &Vars::smethodna, params["a"], dft_map_full["a"]);
            addf("smethodnb_map_mdft", "smethodnb", &g);
            work.add(name, desc, &Vars::smethodnb, params["b"], dft_map_full["b"]);
            // Non-static method with arbitrary params, map style, full
            // (array | map) defaults
            addf("methodna_map_adft", "methodna", &v);
            work.add(name, desc, &Vars::methodna, var(v), params["a"], dft_array_full["a"]);
            addf("methodnb_map_adft", "methodnb", &v);
            work.add(name, desc, &Vars::methodnb, var(v), params["b"], dft_array_full["b"]);
            addf("methodna_map_mdft", "methodna", &v);
            work.add(name, desc, &Vars::methodna, var(v), params["a"], dft_map_full["a"]);
            addf("methodnb_map_mdft", "methodnb", &v);
            work.add(name, desc, &Vars::methodnb, var(v), params["b"], dft_map_full["b"]);

            // All the above are expected to succeed, and are setup for the
            // tests to follow. Registration error cases are exercised as
            // tests rather than as test setup.
        }

        void addf(const std::string& n, const std::string& d, Vars* v)
        {
            debug("addf('", n, "', '", d, "')");
            // This method is to capture in our own DescMap the name and
            // description of every registered function, for metadata query
            // testing.
            descs[n] = d;
            // Also capture the Vars instance on which each function should operate.
            funcvars[n] = v;
            // See constructor for rationale for setting these instance vars.
            this->name = n;
            this->desc = d;
        }

        void verify_descs()
        {
            // Copy descs to a temp map of same type.
            DescMap forgotten(descs.begin(), descs.end());
            for (LLEventDispatcher::NameDesc nd: work)
            {
                DescMap::iterator found = forgotten.find(nd.first);
                ensure(stringize("LLEventDispatcher records function '", nd.first,
                                 "' we didn't enter"),
                       found != forgotten.end());
                ensure_equals(stringize("LLEventDispatcher desc '", nd.second,
                                        "' doesn't match what we entered: '", found->second, "'"),
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
                for (const DescMap::value_type& fme: forgotten)
                {
                    out << delim << fme.first;
                    delim = ", ";
                }
                throw failure(out.str());
            }
        }

        Vars* varsfor(const std::string& name)
        {
            VarsMap::const_iterator found = funcvars.find(name);
            ensure(stringize("No Vars* for ", name), found != funcvars.end());
            ensure(stringize("NULL Vars* for ", name), found->second);
            return found->second;
        }

        std::string call_exc(const std::string& func, const LLSD& args, const std::string& exc_frag)
        {
            return tut::call_exc(
                [this, func, args]()
                {
                    if (func.empty())
                    {
                        work(args);
                    }
                    else
                    {
                        work(func, args);
                    }
                },
                exc_frag);
        }

        void call_logerr(const std::string& func, const LLSD& args, const std::string& frag)
        {
            tut::call_logerr([this, func, args](){ work(func, args); }, frag);
        }

        LLSD getMetadata(const std::string& name)
        {
            LLSD meta(work.getMetadata(name));
            ensure(stringize("No metadata for ", name), meta.isDefined());
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
    // - (Free function | non-static method), arbitrary args,
    //   (array style with (scalar | map) | map style with scalar)
    // - (Free function | non-static method), arbitrary args, array style with
    //   array (too short | too long | just right)
    //   [trap LL_WARNS for too-long case?]
    // - (Free function | non-static method), arbitrary args, map style with
    //   (array | map) (all | too many | holes (with | without) defaults)
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
        LLSD attempts(llsd::array(17, LLSDMap("pi", 3.14)("two", 2)));
        for (LLSD ae: inArray(attempts))
        {
            std::string threw = catch_what<std::exception>([this, &ae](){
                    work.add("freena_err", "freena", freena, ae);
                });
            ensure_has(threw, "must be an array");
        }
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("map-style registration with badly-formed defaults");
        std::string threw = catch_what<std::exception>([this](){
                work.add("freena_err", "freena", freena, llsd::array("a", "b"), 17);
            });
        ensure_has(threw, "must be a map or an array");
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("map-style registration with too many array defaults");
        std::string threw = catch_what<std::exception>([this](){
                work.add("freena_err", "freena", freena,
                         llsd::array("a", "b"),
                         llsd::array(17, 0.9, "gack"));
            });
        ensure_has(threw, "shorter than");
    }

    template<> template<>
    void object::test<4>()
    {
        set_test_name("map-style registration with too many map defaults");
        std::string threw = catch_what<std::exception>([this](){
                work.add("freena_err", "freena", freena,
                         llsd::array("a", "b"),
                         LLSDMap("b", 17)("foo", 3.14)("bar", "sinister"));
            });
        ensure_has(threw, "nonexistent params");
        ensure_has(threw, "foo");
        ensure_has(threw, "bar");
    }

    template<> template<>
    void object::test<5>()
    {
        set_test_name("query all");
        verify_descs();
    }

    template<> template<>
    void object::test<6>()
    {
        set_test_name("query all with remove()");
        ensure("remove('bogus') returned true", ! work.remove("bogus"));
        ensure("remove('real') returned false", work.remove("free1"));
        // Of course, remove that from 'descs' too...
        descs.erase("free1");
        verify_descs();
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
        LLSD names(llsd::array("free1", "Dmethod1", "Dcmethod1", "method1"));
        for (LLSD nm: inArray(names))
        {
            LLSD metadata(getMetadata(nm));
            ensure_equals("name mismatch", metadata["name"], nm);
            ensure_equals(metadata["desc"].asString(), descs[nm]);
            ensure("should not have required structure", metadata["required"].isUndefined());
            ensure("should not have optional", metadata["optional"].isUndefined());

            std::string name_req(nm.asString() + "_req");
            metadata = getMetadata(name_req);
            ensure_equals(metadata["name"].asString(), name_req);
            ensure_equals(metadata["desc"].asString(), descs[name_req]);
            ensure_equals("required mismatch", required, metadata["required"]);
            ensure("should not have optional", metadata["optional"].isUndefined());
        }
    }

    template<> template<>
    void object::test<9>()
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
                ensure_equals("name mismatch", metadata["name"], nm);
                ensure_equals(metadata["desc"].asString(), descs[nm]);
                ensure_equals(stringize("mismatched required for ", nm.asString()),
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
        LLSD names(llsd::array("free0_map", "smethod0_map", "method0_map"));
        for (LLSD nm: inArray(names))
        {
            LLSD metadata(getMetadata(nm));
            ensure_equals("name mismatch", metadata["name"], nm);
            ensure_equals(metadata["desc"].asString(), descs[nm]);
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
            ensure_equals("adft name", adft, ameta["name"]);
            ensure_equals("mdft name", mdft, mmeta["name"]);
            ameta.erase("name");
            mmeta.erase("name");
            ensure_equals(stringize("metadata for ", adft.asString(),
                                    " vs. ", mdft.asString()),
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
        for (LLSD::String a: ab)
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
        debug("allreq:\n",
              allreq, "\n"
              "leftreq:\n",
              leftreq, "\n"
              "rightdft:\n",
              rightdft);

        // Generate maps containing parameter names not provided by the
        // dft_map_partial maps.
        LLSD skipreq(allreq);
        for (LLSD::String a: ab)
        {
            for (const MapEntry& me: inMap(dft_map_partial[a]))
            {
                skipreq[a].erase(me.first);
            }
        }
        debug("skipreq:\n",
              skipreq);

        LLSD groups(llsd::array       // array of groups

                    (llsd::array      // group
                     (llsd::array("freena_map_allreq", "smethodna_map_allreq", "methodna_map_allreq"),
                      llsd::array(allreq["a"], LLSD())),  // required, optional

                     llsd::array        // group
                     (llsd::array("freenb_map_allreq", "smethodnb_map_allreq", "methodnb_map_allreq"),
                      llsd::array(allreq["b"], LLSD())),  // required, optional

                     llsd::array        // group
                     (llsd::array("freena_map_leftreq", "smethodna_map_leftreq", "methodna_map_leftreq"),
                      llsd::array(leftreq["a"], rightdft["a"])),  // required, optional

                     llsd::array        // group
                     (llsd::array("freenb_map_leftreq", "smethodnb_map_leftreq", "methodnb_map_leftreq"),
                      llsd::array(leftreq["b"], rightdft["b"])),  // required, optional

                     llsd::array        // group
                     (llsd::array("freena_map_skipreq", "smethodna_map_skipreq", "methodna_map_skipreq"),
                      llsd::array(skipreq["a"], dft_map_partial["a"])),  // required, optional

                     llsd::array        // group
                     (llsd::array("freenb_map_skipreq", "smethodnb_map_skipreq", "methodnb_map_skipreq"),
                      llsd::array(skipreq["b"], dft_map_partial["b"])),  // required, optional

                     // We only need mention the full-map-defaults ("_mdft" suffix)
                     // registrations, having established their equivalence with the
                     // full-array-defaults ("_adft" suffix) registrations in another test.
                     llsd::array        // group
                     (llsd::array("freena_map_mdft", "smethodna_map_mdft", "methodna_map_mdft"),
                      llsd::array(LLSD::emptyMap(), dft_map_full["a"])),  // required, optional

                     llsd::array        // group
                     (llsd::array("freenb_map_mdft", "smethodnb_map_mdft", "methodnb_map_mdft"),
                      llsd::array(LLSD::emptyMap(), dft_map_full["b"])))); // required, optional

        for (LLSD grp: inArray(groups))
        {
            // Internal structure of each group in 'groups':
            LLSD names(grp[0]);
            LLSD required(grp[1][0]);
            LLSD optional(grp[1][1]);
            debug("For ", names, ",\n",
                  "required:\n",
                  required, "\n"
                  "optional:\n",
                  optional);

            // Loop through 'names'
            for (LLSD nm: inArray(names))
            {
                LLSD metadata(getMetadata(nm));
                ensure_equals("name mismatch", metadata["name"], nm);
                ensure_equals(nm.asString(), metadata["desc"].asString(), descs[nm]);
                ensure_equals(stringize(nm, " required mismatch"),
                              metadata["required"], required);
                ensure_equals(stringize(nm, " optional mismatch"),
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
        std::string threw = call_exc("", LLSDMap("op", "freek"), "bad");
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
        LLSD matching(LLSDMap("d", 3.14)("array", llsd::array("answer", true, answer)));
        // Okay, walk through 'tests'.
        for (const CallablesTriple& tr: tests)
        {
            // Should be able to pass 'answer' to Callables registered
            // without 'required'.
            work(tr.name, answer);
            ensure_equals("answer mismatch", tr.llsd, answer);
            // Should NOT be able to pass 'answer' to Callables registered
            // with 'required'.
            call_logerr(tr.name_req, answer, "bad request");
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

    template<> template<>
    void object::test<18>()
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
    }

    // Break out this data because we use it in a couple different tests.
    LLSD array_funcs(llsd::array
                     (LLSDMap("a", "freena_array")   ("b", "freenb_array"),
                      LLSDMap("a", "smethodna_array")("b", "smethodnb_array"),
                      LLSDMap("a", "methodna_array") ("b", "methodnb_array")));

    template<> template<>
    void object::test<19>()
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
            }
        }
    }

    template<> template<>
    void object::test<20>()
    {
        set_test_name("call array-style functions with right-size arrays");
        std::vector<U8> binary;
        for (size_t h(0x01), i(0); i < 5; h+= 0x22, ++i)
        {
            binary.push_back((U8)h);
        }
        LLSD args(LLSDMap("a", llsd::array(true, 17, 3.14, 123.456, "char*"))
                         ("b", llsd::array("string",
                                           LLUUID("01234567-89ab-cdef-0123-456789abcdef"),
                                           LLDate("2011-02-03T15:07:00Z"),
                                           LLURI("http://secondlife.com"),
                                           binary)));
        LLSD expect;
        for (LLSD::String a: ab)
        {
            expect[a] = zipmap(params[a], args[a]);
        }
        // Adjust expect["a"]["cp"] for special Vars::cp treatment.
        expect["a"]["cp"] = stringize("'", expect["a"]["cp"].asString(), "'");
        debug("expect: ", expect);

        for (const LLSD& funcsab: inArray(array_funcs))
        {
            for (LLSD::String a: ab)
            {
                // Reset the Vars instance before each call
                Vars* vars(varsfor(funcsab[a]));
                *vars = Vars();
                work(funcsab[a], args[a]);
                ensure_llsd(stringize(funcsab[a].asString(), ": expect[\"", a, "\"] mismatch"),
                            vars->inspect(), expect[a], 7); // 7 bits ~= 2 decimal digits
            }
        }
    }

    template<> template<>
    void object::test<21>()
    {
        set_test_name("verify that passing LLSD() to const char* sends NULL");

        ensure_equals("Vars::cp init", v.cp, "");
        work("methodna_map_mdft", LLSDMap("cp", LLSD()));
        ensure_equals("passing LLSD()", v.cp, "NULL");
        work("methodna_map_mdft", LLSDMap("cp", ""));
        ensure_equals("passing \"\"", v.cp, "''");
        work("methodna_map_mdft", LLSDMap("cp", "non-NULL"));
        ensure_equals("passing \"non-NULL\"", v.cp, "'non-NULL'");
    }

    template<> template<>
    void object::test<22>()
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
        debug("array_full: ", array_full, "\n"
              "array_overfull: ", array_overfull);
        // We rather hope that LLDate::now() will generate a timestamp
        // distinct from the one it generated in the constructor, moments ago.
        ensure_not_equals("Timestamps too close",
                          array_full["b"][2].asDate(), dft_array_full["b"][2].asDate());
        // We /insist/ that LLUUID::generateNewID() do so.
        ensure_not_equals("UUID collision",
                          array_full["b"][1].asUUID(), dft_array_full["b"][1].asUUID());
        LLSD map_full, map_overfull;
        for (LLSD::String a: ab)
        {
            map_full[a] = zipmap(params[a], array_full[a]);
            map_overfull[a] = map_full[a];
            map_overfull[a]["extra"] = "ignore";
        }
        debug("map_full: ", map_full, "\n"
              "map_overfull: ", map_overfull);
        LLSD expect(map_full);
        // Twiddle the const char* param.
        expect["a"]["cp"] = std::string("'") + expect["a"]["cp"].asString() + "'";
        // Another adjustment. For each data type, we're trying to distinguish
        // three values: the Vars member's initial value (member wasn't
        // stored; control never reached the set function), the registered
        // default param value from dft_array_full, and the array_full value
        // in this test. But bool can only distinguish two values. In this
        // case, we want to differentiate the local array_full value from the
        // dft_array_full value, so we use 'false'. However, that means
        // Vars::inspect() doesn't differentiate it from the initial value,
        // so won't bother returning it. Predict that behavior to match the
        // LLSD values.
        expect["a"].erase("b");
        debug("expect: ", expect);
        // For this test, calling functions registered with different sets of
        // parameter defaults should make NO DIFFERENCE WHATSOEVER. Every call
        // should pass all params.
        LLSD names(LLSDMap
                   ("a", llsd::array
                    ("freena_map_allreq",  "smethodna_map_allreq",  "methodna_map_allreq",
                     "freena_map_leftreq", "smethodna_map_leftreq", "methodna_map_leftreq",
                     "freena_map_skipreq", "smethodna_map_skipreq", "methodna_map_skipreq",
                     "freena_map_adft",    "smethodna_map_adft",    "methodna_map_adft",
                     "freena_map_mdft",    "smethodna_map_mdft",    "methodna_map_mdft"))
                   ("b", llsd::array
                    ("freenb_map_allreq",  "smethodnb_map_allreq",  "methodnb_map_allreq",
                     "freenb_map_leftreq", "smethodnb_map_leftreq", "methodnb_map_leftreq",
                     "freenb_map_skipreq", "smethodnb_map_skipreq", "methodnb_map_skipreq",
                     "freenb_map_adft",    "smethodnb_map_adft",    "methodnb_map_adft",
                     "freenb_map_mdft",    "smethodnb_map_mdft",    "methodnb_map_mdft")));
        // Treat (full | overfull) (array | map) the same.
        LLSD argssets(llsd::array(array_full, array_overfull, map_full, map_overfull));
        for (const LLSD& args: inArray(argssets))
        {
            for (LLSD::String a: ab)
            {
                for (LLSD::String name: inArray(names[a]))
                {
                    // Reset the Vars instance
                    Vars* vars(varsfor(name));
                    *vars = Vars();
                    work(name, args[a]);
                    ensure_llsd(stringize(name, ": expect[\"", a, "\"] mismatch"),
                                vars->inspect(), expect[a], 7); // 7 bits, 2 decimal digits
                    // intercept LL_WARNS for the two overfull cases?
                }
            }
        }
    }

    struct DispatchResult: public LLDispatchListener
    {
        using DR = DispatchResult;

        DispatchResult(): LLDispatchListener("results", "op")
        {
            add("strfunc",   "return string",       &DR::strfunc);
            add("voidfunc",  "void function",       &DR::voidfunc);
            add("emptyfunc", "return empty LLSD",   &DR::emptyfunc);
            add("intfunc",   "return Integer LLSD", &DR::intfunc);
            add("llsdfunc",  "return passed LLSD",  &DR::llsdfunc);
            add("mapfunc",   "return map LLSD",     &DR::mapfunc);
            add("arrayfunc", "return array LLSD",   &DR::arrayfunc);
        }

        std::string strfunc(const std::string& str) const { return "got " + str; }
        void voidfunc()                  const {}
        LLSD emptyfunc()                 const { return {}; }
        int  intfunc(int i)              const { return -i; }
        LLSD llsdfunc(const LLSD& event) const
        {
            LLSD result{ event };
            result["with"] = "string";
            return result;
        }
        LLSD mapfunc(int i, const std::string& str) const
        {
            return llsd::map("i", intfunc(i), "str", strfunc(str));
        }
        LLSD arrayfunc(int i, const std::string& str) const
        {
            return llsd::array(intfunc(i), strfunc(str));
        }
    };

    template<> template<>
    void object::test<23>()
    {
        set_test_name("string result");
        DispatchResult service;
        LLSD result{ service("strfunc", "a string") };
        ensure_equals("strfunc() mismatch", result.asString(), "got a string");
    }

    template<> template<>
    void object::test<24>()
    {
        set_test_name("void result");
        DispatchResult service;
        LLSD result{ service("voidfunc", LLSD()) };
        ensure("voidfunc() returned defined", result.isUndefined());
    }

    template<> template<>
    void object::test<25>()
    {
        set_test_name("Integer result");
        DispatchResult service;
        LLSD result{ service("intfunc", -17) };
        ensure_equals("intfunc() mismatch", result.asInteger(), 17);
    }

    template<> template<>
    void object::test<26>()
    {
        set_test_name("LLSD echo");
        DispatchResult service;
        LLSD result{ service("llsdfunc", llsd::map("op", "llsdfunc", "reqid", 17)) };
        ensure_equals("llsdfunc() mismatch", result,
                      llsd::map("op", "llsdfunc", "reqid", 17, "with", "string"));
    }

    template<> template<>
    void object::test<27>()
    {
        set_test_name("map LLSD result");
        DispatchResult service;
        LLSD result{ service("mapfunc", llsd::array(-12, "value")) };
        ensure_equals("mapfunc() mismatch", result, llsd::map("i", 12, "str", "got value"));
    }

    template<> template<>
    void object::test<28>()
    {
        set_test_name("array LLSD result");
        DispatchResult service;
        LLSD result{ service("arrayfunc", llsd::array(-8, "word")) };
        ensure_equals("arrayfunc() mismatch", result, llsd::array(8, "got word"));
    }

    template<> template<>
    void object::test<29>()
    {
        set_test_name("listener error, no reply");
        DispatchResult service;
        tut::call_exc(
            [&service]()
            { service.post(llsd::map("op", "nosuchfunc", "reqid", 17)); },
            "nosuchfunc");
    }

    template<> template<>
    void object::test<30>()
    {
        set_test_name("listener error with reply");
        DispatchResult service;
        LLCaptureListener<LLSD> result;
        service.post(llsd::map("op", "nosuchfunc", "reqid", 17, "reply", result.getName()));
        LLSD reply{ result.get() };
        ensure("no reply", reply.isDefined());
        ensure_equals("reqid not echoed", reply["reqid"].asInteger(), 17);
        ensure_has(reply["error"].asString(), "nosuchfunc");
    }

    template<> template<>
    void object::test<31>()
    {
        set_test_name("listener call to void function");
        DispatchResult service;
        LLCaptureListener<LLSD> result;
        result.set("non-empty");
        for (const auto& func: StringVec{ "voidfunc", "emptyfunc" })
        {
            service.post(llsd::map(
                             "op", func,
                             "reqid", 17,
                             "reply", result.getName()));
            ensure_equals("reply from " + func, result.get().asString(), "non-empty");
        }
    }

    template<> template<>
    void object::test<32>()
    {
        set_test_name("listener call to string function");
        DispatchResult service;
        LLCaptureListener<LLSD> result;
        service.post(llsd::map(
                         "op", "strfunc",
                         "args", llsd::array("a string"),
                         "reqid", 17,
                         "reply", result.getName()));
        LLSD reply{ result.get() };
        ensure_equals("reqid not echoed", reply["reqid"].asInteger(), 17);
        ensure_equals("bad reply from strfunc", reply["data"].asString(), "got a string");
    }

    template<> template<>
    void object::test<33>()
    {
        set_test_name("listener call to map function");
        DispatchResult service;
        LLCaptureListener<LLSD> result;
        service.post(llsd::map(
                         "op", "mapfunc",
                         "args", llsd::array(-7, "value"),
                         "reqid", 17,
                         "reply", result.getName()));
        LLSD reply{ result.get() };
        ensure_equals("reqid not echoed", reply["reqid"].asInteger(), 17);
        ensure_equals("bad i from mapfunc", reply["i"].asInteger(), 7);
        ensure_equals("bad str from mapfunc", reply["str"], "got value");
    }

    template<> template<>
    void object::test<34>()
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
        LLSD reply{ result.get() };
        ensure_equals("reqid not echoed", reply["reqid"].asInteger(), 17);
        reply.erase("reqid");
        ensure_equals(
            "bad map batch",
            reply,
            llsd::map(
                "strfunc", "got some string",
                "intfunc", -2,
                "voidfunc", LLSD(),
                "arrayfunc", llsd::array(5, "got other string")));
    }

    template<> template<>
    void object::test<35>()
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
        LLSD reply{ result.get() };
        ensure_equals("reqid not echoed", reply["reqid"].asInteger(), 17);
        reply.erase("reqid");
        auto error{ reply["error"].asString() };
        reply.erase("error");
        ensure_has(error, "badfunc");
        ensure_has(error, "missing");
        ensure_equals(
            "bad partial batch",
            reply,
            llsd::map(
                "strfunc", "got some string",
                "intfunc", -2,
                "voidfunc", LLSD(),
                "arrayfunc", llsd::array(5, "got other string")));
    }

    template<> template<>
    void object::test<36>()
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
            },
            "badfunc");
        ensure_has(error, "missing");
    }

    template<> template<>
    void object::test<37>()
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
        LLSD reply{ result.get() };
        ensure_equals("reqid not echoed", reply["reqid"].asInteger(), 17);
        reply.erase("reqid");
        ensure_equals(
            "bad array batch",
            reply,
            llsd::map(
                "data", llsd::array(
                    "got some string",
                    -2,
                    llsd::array(5, "got other string"),
                    LLSD())));
    }

    template<> template<>
    void object::test<38>()
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
        LLSD reply{ result.get() };
        ensure_equals("reqid not echoed", reply["reqid"].asInteger(), 17);
        reply.erase("reqid");
        auto error{ reply["error"] };
        reply.erase("error");
        ensure_has(error, "[1]");
        ensure_has(error, "unsupported");
        ensure_equals("bad array batch", reply,
                      llsd::map("data", llsd::array("got some string")));
    }

    template<> template<>
    void object::test<39>()
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
            },
            "[1]");
        ensure_has(error, "unsupported");
    }
} // namespace tut
