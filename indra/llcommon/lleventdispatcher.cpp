/**
 * @file   lleventdispatcher.cpp
 * @author Nat Goodspeed
 * @date   2009-06-18
 * @brief  Implementation for lleventdispatcher.
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

#if LL_WINDOWS
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif

// Precompiled header
#include "linden_common.h"
// associated header
#include "lleventdispatcher.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llevents.h"
#include "llerror.h"
#include "llsdutil.h"
#include "stringize.h"
#include <memory>                   // std::auto_ptr

/*****************************************************************************
*   LLSDArgsSource
*****************************************************************************/
/**
 * Store an LLSD array, producing its elements one at a time. Die with LL_ERRS
 * if the consumer requests more elements than the array contains.
 */
class LL_COMMON_API LLSDArgsSource
{
public:
    LLSDArgsSource(const std::string function, const LLSD& args);
    ~LLSDArgsSource();

    LLSD next();

    void done() const;

private:
    std::string _function;
    LLSD _args;
    LLSD::Integer _index;
};

LLSDArgsSource::LLSDArgsSource(const std::string function, const LLSD& args):
    _function(function),
    _args(args),
    _index(0)
{
    if (! (_args.isUndefined() || _args.isArray()))
    {
        LL_ERRS("LLSDArgsSource") << _function << " needs an args array instead of "
                                  << _args << LL_ENDL;
    }
}

LLSDArgsSource::~LLSDArgsSource()
{
    done();
}

LLSD LLSDArgsSource::next()
{
    if (_index >= _args.size())
    {
        LL_ERRS("LLSDArgsSource") << _function << " requires more arguments than the "
                                  << _args.size() << " provided: " << _args << LL_ENDL;
    }
    return _args[_index++];
}

void LLSDArgsSource::done() const
{
    if (_index < _args.size())
    {
        LL_WARNS("LLSDArgsSource") << _function << " only consumed " << _index
                                   << " of the " << _args.size() << " arguments provided: "
                                   << _args << LL_ENDL;
    }
}

/*****************************************************************************
*   LLSDArgsMapper
*****************************************************************************/
/**
 * From a formal parameters description and a map of arguments, construct an
 * arguments array.
 *
 * That is, given:
 * - an LLSD array of length n containing parameter-name strings,
 *   corresponding to the arguments of a function of interest
 * - an LLSD collection specifying default parameter values, either:
 *   - an LLSD array of length m <= n, matching the rightmost m params, or
 *   - an LLSD map explicitly stating default name=value pairs
 * - an LLSD map of parameter names and actual values for a particular
 *   function call
 * construct an LLSD array of actual argument values for this function call.
 *
 * The parameter-names array and the defaults collection describe the function
 * being called. The map might vary with every call, providing argument values
 * for the described parameters.
 *
 * The array of parameter names must match the number of parameters expected
 * by the function of interest.
 *
 * If you pass a map of default parameter values, it provides default values
 * as you might expect. It is an error to specify a default value for a name
 * not listed in the parameters array.
 *
 * If you pass an array of default parameter values, it is mapped to the
 * rightmost m of the n parameter names. It is an error if the default-values
 * array is longer than the parameter-names array. Consider the following
 * parameter names: ["a", "b", "c", "d"].
 *
 * - An empty array of default values (or an isUndefined() value) asserts that
 *   every one of the above parameter names is required.
 * - An array of four default values [1, 2, 3, 4] asserts that every one of
 *   the above parameters is optional. If the current parameter map is empty,
 *   they will be passed to the function as [1, 2, 3, 4].
 * - An array of two default values [11, 12] asserts that parameters "a" and
 *   "b" are required, while "c" and "d" are optional, having default values
 *   "c"=11 and "d"=12.
 *
 * The arguments array is constructed as follows:
 *
 * - Arguments-map keys not found in the parameter-names array are ignored.
 * - Entries from the map provide values for an improper subset of the
 *   parameters named in the parameter-names array. This results in a
 *   tentative values array with "holes." (size of map) + (number of holes) =
 *   (size of names array)
 * - Holes are filled with the default values.
 * - Any remaining holes constitute an error.
 */
class LL_COMMON_API LLSDArgsMapper
{
public:
    /// Accept description of function: function name, param names, param
    /// default values
    LLSDArgsMapper(const std::string& function, const LLSD& names, const LLSD& defaults);

    /// Given arguments map, return LLSD::Array of parameter values, or LL_ERRS.
    LLSD map(const LLSD& argsmap) const;

private:
    static std::string formatlist(const LLSD&);

    // The function-name string is purely descriptive. We want error messages
    // to be able to indicate which function's LLSDArgsMapper has the problem.
    std::string _function;
    // Store the names array pretty much as given.
    LLSD _names;
    // Though we're handed an array of name strings, it's more useful to us to
    // store it as a map from name string to position index. Of course that's
    // easy to generate from the incoming names array, but why do it more than
    // once?
    typedef std::map<LLSD::String, LLSD::Integer> IndexMap;
    IndexMap _indexes;
    // Generated array of default values, aligned with the array of param names.
    LLSD _defaults;
    // Indicate whether we have a default value for each param.
    typedef std::vector<char> FilledVector;
    FilledVector _has_dft;
};

LLSDArgsMapper::LLSDArgsMapper(const std::string& function,
                               const LLSD& names, const LLSD& defaults):
    _function(function),
    _names(names),
    _has_dft(names.size())
{
    if (! (_names.isUndefined() || _names.isArray()))
    {
        LL_ERRS("LLSDArgsMapper") << function << " names must be an array, not " << names << LL_ENDL;
    }
    LLSD::Integer nparams(_names.size());
    // From _names generate _indexes.
    for (LLSD::Integer ni = 0, nend = _names.size(); ni < nend; ++ni)
    {
        _indexes[_names[ni]] = ni;
    }

    // Presize _defaults() array so we don't have to resize it more than once.
    // All entries are initialized to LLSD(); but since _has_dft is still all
    // 0, they're all "holes" for now.
    if (nparams)
    {
        _defaults[nparams - 1] = LLSD();
    }

    if (defaults.isUndefined() || defaults.isArray())
    {
        LLSD::Integer ndefaults = defaults.size();
        // defaults is a (possibly empty) array. Right-align it with names.
        if (ndefaults > nparams)
        {
            LL_ERRS("LLSDArgsMapper") << function << " names array " << names
                                      << " shorter than defaults array " << defaults << LL_ENDL;
        }

        // Offset by which we slide defaults array right to right-align with
        // _names array
        LLSD::Integer offset = nparams - ndefaults;
        // Fill rightmost _defaults entries from defaults, and mark them as
        // filled
        for (LLSD::Integer i = 0, iend = ndefaults; i < iend; ++i)
        {
            _defaults[i + offset] = defaults[i];
            _has_dft[i + offset] = 1;
        }
    }
    else if (defaults.isMap())
    {
        // defaults is a map. Use it to populate the _defaults array.
        LLSD bogus;
        for (LLSD::map_const_iterator mi(defaults.beginMap()), mend(defaults.endMap());
             mi != mend; ++mi)
        {
            IndexMap::const_iterator ixit(_indexes.find(mi->first));
            if (ixit == _indexes.end())
            {
                bogus.append(mi->first);
                continue;
            }

            LLSD::Integer pos = ixit->second;
            // Store default value at that position in the _defaults array.
            _defaults[pos] = mi->second;
            // Don't forget to record the fact that we've filled this
            // position.
            _has_dft[pos] = 1;
        }
        if (bogus.size())
        {
            LL_ERRS("LLSDArgsMapper") << function << " defaults specified for nonexistent params "
                                      << formatlist(bogus) << LL_ENDL;
        }
    }
    else
    {
        LL_ERRS("LLSDArgsMapper") << function << " defaults must be a map or an array, not "
                                  << defaults << LL_ENDL;
    }
}

LLSD LLSDArgsMapper::map(const LLSD& argsmap) const
{
    if (! (argsmap.isUndefined() || argsmap.isMap()))
    {
        LL_ERRS("LLSDArgsMapper") << _function << " map() needs a map, not " << argsmap << LL_ENDL;
    }
    // Initialize the args array. Indexing a non-const LLSD array grows it
    // to appropriate size, but we don't want to resize this one on each
    // new operation. Just make it as big as we need before we start
    // stuffing values into it.
    LLSD args(LLSD::emptyArray());
    if (_defaults.size() == 0)
    {
        // If this function requires no arguments, fast exit. (Don't try to
        // assign to args[-1].)
        return args;
    }
    args[_defaults.size() - 1] = LLSD();

    // Get a vector of chars to indicate holes. It's tempting to just scan
    // for LLSD::isUndefined() values after filling the args array from
    // the map, but it's plausible for caller to explicitly pass
    // isUndefined() as the value of some parameter name. That's legal
    // since isUndefined() has well-defined conversions (default value)
    // for LLSD data types. So use a whole separate array for detecting
    // holes. (Avoid std::vector<bool> which is known to be odd -- can we
    // iterate?)
    FilledVector filled(args.size());
    // Walk the map.
    for (LLSD::map_const_iterator mi(argsmap.beginMap()), mend(argsmap.endMap());
         mi != mend; ++mi)
    {
        // mi->first is a parameter-name string, with mi->second its
        // value. Look up the name's position index in _indexes.
        IndexMap::const_iterator ixit(_indexes.find(mi->first));
        if (ixit == _indexes.end())
        {
            // Allow for a map containing more params than were passed in
            // our names array. Caller typically receives a map containing
            // the function name, cruft such as reqid, etc. Ignore keys
            // not defined in _indexes.
            LL_DEBUGS("LLSDArgsMapper") << _function << " ignoring "
                                        << mi->first << "=" << mi->second << LL_ENDL;
            continue;
        }
        LLSD::Integer pos = ixit->second;
        // Store the value at that position in the args array.
        args[pos] = mi->second;
        // Don't forget to record the fact that we've filled this
        // position.
        filled[pos] = 1;
    }
    // Fill any remaining holes from _defaults.
    LLSD unfilled(LLSD::emptyArray());
    for (LLSD::Integer i = 0, iend = args.size(); i < iend; ++i)
    {
        if (! filled[i])
        {
            // If there's no default value for this parameter, that's an
            // error.
            if (! _has_dft[i])
            {
                unfilled.append(_names[i]);
            }
            else
            {
                args[i] = _defaults[i];
            }
        }
    }
    // If any required args -- args without defaults -- were left unfilled
    // by argsmap, that's a problem.
    if (unfilled.size())
    {
        LL_ERRS("LLSDArgsMapper") << _function << " missing required arguments "
                                  << formatlist(unfilled) << " from " << argsmap << LL_ENDL;
    }

    // done
    return args;
}

std::string LLSDArgsMapper::formatlist(const LLSD& list)
{
    std::ostringstream out;
    const char* delim = "";
    for (LLSD::array_const_iterator li(list.beginArray()), lend(list.endArray());
         li != lend; ++li)
    {
        out << delim << li->asString();
        delim = ", ";
    }
    return out.str();
}

LLEventDispatcher::LLEventDispatcher(const std::string& desc, const std::string& key):
    mDesc(desc),
    mKey(key)
{
}

LLEventDispatcher::~LLEventDispatcher()
{
}

/**
 * DispatchEntry subclass used for callables accepting(const LLSD&)
 */
struct LLEventDispatcher::LLSDDispatchEntry: public LLEventDispatcher::DispatchEntry
{
    LLSDDispatchEntry(const std::string& desc, const Callable& func, const LLSD& required):
        DispatchEntry(desc),
        mFunc(func),
        mRequired(required)
    {}

    Callable mFunc;
    LLSD mRequired;

    virtual void call(const std::string& desc, const LLSD& event) const
    {
        // Validate the syntax of the event itself.
        std::string mismatch(llsd_matches(mRequired, event));
        if (! mismatch.empty())
        {
            LL_ERRS("LLEventDispatcher") << desc << ": bad request: " << mismatch << LL_ENDL;
        }
        // Event syntax looks good, go for it!
        mFunc(event);
    }

    virtual LLSD addMetadata(LLSD meta) const
    {
        meta["required"] = mRequired;
        return meta;
    }
};

/**
 * DispatchEntry subclass for passing LLSD to functions accepting
 * arbitrary argument types (convertible via LLSDParam)
 */
struct LLEventDispatcher::ParamsDispatchEntry: public LLEventDispatcher::DispatchEntry
{
    ParamsDispatchEntry(const std::string& desc, const invoker_function& func):
        DispatchEntry(desc),
        mInvoker(func)
    {}

    invoker_function mInvoker;

    virtual void call(const std::string& desc, const LLSD& event) const
    {
        LLSDArgsSource src(desc, event);
        mInvoker(boost::bind(&LLSDArgsSource::next, boost::ref(src)));
    }
};

/**
 * DispatchEntry subclass for dispatching LLSD::Array to functions accepting
 * arbitrary argument types (convertible via LLSDParam)
 */
struct LLEventDispatcher::ArrayParamsDispatchEntry: public LLEventDispatcher::ParamsDispatchEntry
{
    ArrayParamsDispatchEntry(const std::string& desc, const invoker_function& func,
                             LLSD::Integer arity):
        ParamsDispatchEntry(desc, func),
        mArity(arity)
    {}

    LLSD::Integer mArity;

    virtual LLSD addMetadata(LLSD meta) const
    {
        LLSD array(LLSD::emptyArray());
        // Resize to number of arguments required
        array[mArity - 1] = LLSD();
        llassert_always(array.size() == mArity);
        meta["required"] = array;
        return meta;
    }
};

/**
 * DispatchEntry subclass for dispatching LLSD::Map to functions accepting
 * arbitrary argument types (convertible via LLSDParam)
 */
struct LLEventDispatcher::MapParamsDispatchEntry: public LLEventDispatcher::ParamsDispatchEntry
{
    MapParamsDispatchEntry(const std::string& name, const std::string& desc,
                           const invoker_function& func,
                           const LLSD& params, const LLSD& defaults):
        ParamsDispatchEntry(desc, func),
        mMapper(name, params, defaults),
        mRequired(LLSD::emptyMap())
    {
        // Build the set of all param keys, then delete the ones that are
        // optional. What's left are the ones that are required.
        for (LLSD::array_const_iterator pi(params.beginArray()), pend(params.endArray());
             pi != pend; ++pi)
        {
            mRequired[pi->asString()] = LLSD();
        }

        if (defaults.isArray() || defaults.isUndefined())
        {
            // Right-align the params and defaults arrays.
            LLSD::Integer offset = params.size() - defaults.size();
            // Now the name of every defaults[i] is at params[i + offset].
            for (LLSD::Integer i(0), iend(defaults.size()); i < iend; ++i)
            {
                // Erase this optional param from mRequired.
                mRequired.erase(params[i + offset].asString());
                // Instead, make an entry in mOptional with the default
                // param's name and value.
                mOptional[params[i + offset].asString()] = defaults[i];
            }
        }
        else if (defaults.isMap())
        {
            // if defaults is already a map, then it's already in the form we
            // intend to deliver in metadata
            mOptional = defaults;
            // Just delete from mRequired every key appearing in mOptional.
            for (LLSD::map_const_iterator mi(mOptional.beginMap()), mend(mOptional.endMap());
                 mi != mend; ++mi)
            {
                mRequired.erase(mi->first);
            }
        }
    }

    LLSDArgsMapper mMapper;
    LLSD mRequired;
    LLSD mOptional;

    virtual void call(const std::string& desc, const LLSD& event) const
    {
        // Just convert from LLSD::Map to LLSD::Array using mMapper, then pass
        // to base-class call() method.
        ParamsDispatchEntry::call(desc, mMapper.map(event));
    }

    virtual LLSD addMetadata(LLSD meta) const
    {
        meta["required"] = mRequired;
        meta["optional"] = mOptional;
        return meta;
    }
};

void LLEventDispatcher::addArrayParamsDispatchEntry(const std::string& name,
                                                    const std::string& desc,
                                                    const invoker_function& invoker,
                                                    LLSD::Integer arity)
{
    // Peculiar to me that boost::ptr_map() accepts std::auto_ptr but not dumb ptr
    mDispatch.insert(name, std::auto_ptr<DispatchEntry>(
                         new ArrayParamsDispatchEntry(desc, invoker, arity)));
}

void LLEventDispatcher::addMapParamsDispatchEntry(const std::string& name,
                                                  const std::string& desc,
                                                  const invoker_function& invoker,
                                                  const LLSD& params,
                                                  const LLSD& defaults)
{
    mDispatch.insert(name, std::auto_ptr<DispatchEntry>(
                         new MapParamsDispatchEntry(name, desc, invoker, params, defaults)));
}

/// Register a callable by name
void LLEventDispatcher::add(const std::string& name, const std::string& desc,
                            const Callable& callable, const LLSD& required)
{
    mDispatch.insert(name, std::auto_ptr<DispatchEntry>(
                         new LLSDDispatchEntry(desc, callable, required)));
}

void LLEventDispatcher::addFail(const std::string& name, const std::string& classname) const
{
    LL_ERRS("LLEventDispatcher") << "LLEventDispatcher(" << mDesc << ")::add(" << name
                                 << "): " << classname << " is not a subclass "
                                 << "of LLEventDispatcher" << LL_ENDL;
}

/// Unregister a callable
bool LLEventDispatcher::remove(const std::string& name)
{
    DispatchMap::iterator found = mDispatch.find(name);
    if (found == mDispatch.end())
    {
        return false;
    }
    mDispatch.erase(found);
    return true;
}

/// Call a registered callable with an explicitly-specified name. If no
/// such callable exists, die with LL_ERRS.
void LLEventDispatcher::operator()(const std::string& name, const LLSD& event) const
{
    if (! try_call(name, event))
    {
        LL_ERRS("LLEventDispatcher") << "LLEventDispatcher(" << mDesc << "): '" << name
                                     << "' not found" << LL_ENDL;
    }
}

/// Extract the @a key value from the incoming @a event, and call the
/// callable whose name is specified by that map @a key. If no such
/// callable exists, die with LL_ERRS.
void LLEventDispatcher::operator()(const LLSD& event) const
{
    // This could/should be implemented in terms of the two-arg overload.
    // However -- we can produce a more informative error message.
    std::string name(event[mKey]);
    if (! try_call(name, event))
    {
        LL_ERRS("LLEventDispatcher") << "LLEventDispatcher(" << mDesc << "): bad " << mKey
                                     << " value '" << name << "'" << LL_ENDL;
    }
}

bool LLEventDispatcher::try_call(const LLSD& event) const
{
    return try_call(event[mKey], event);
}

bool LLEventDispatcher::try_call(const std::string& name, const LLSD& event) const
{
    DispatchMap::const_iterator found = mDispatch.find(name);
    if (found == mDispatch.end())
    {
        return false;
    }
    // Found the name, so it's plausible to even attempt the call.
    found->second->call(STRINGIZE("LLEventDispatcher(" << mDesc << ") calling '" << name << "'"),
                        event);
    return true;                    // tell caller we were able to call
}

LLSD LLEventDispatcher::getMetadata(const std::string& name) const
{
    DispatchMap::const_iterator found = mDispatch.find(name);
    if (found == mDispatch.end())
    {
        return LLSD();
    }
    LLSD meta;
    meta["name"] = name;
    meta["desc"] = found->second->mDesc;
    return found->second->addMetadata(meta);
}

LLDispatchListener::LLDispatchListener(const std::string& pumpname, const std::string& key):
    LLEventDispatcher(pumpname, key),
    mPump(pumpname, true),          // allow tweaking for uniqueness
    mBoundListener(mPump.listen("self", boost::bind(&LLDispatchListener::process, this, _1)))
{
}

bool LLDispatchListener::process(const LLSD& event)
{
    (*this)(event);
    return false;
}

LLEventDispatcher::DispatchEntry::DispatchEntry(const std::string& desc):
    mDesc(desc)
{}

