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
#include "llexception.h"
#include "llsdutil.h"
#include "stringize.h"
#include <iomanip>                  // std::quoted()
#include <memory>                   // std::auto_ptr

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
class LL_COMMON_API LLEventDispatcher::LLSDArgsMapper
{
public:
    /// Accept description of function: function name, param names, param
    /// default values
    LLSDArgsMapper(LLEventDispatcher* parent, const std::string& function,
                   const LLSD& names, const LLSD& defaults);

    /// Given arguments map, return LLSD::Array of parameter values, or
    /// trigger error.
    LLSD map(const LLSD& argsmap) const;

private:
    static std::string formatlist(const LLSD&);
    template <typename... ARGS>
    [[noreturn]] void callFail(ARGS&&... args) const;

    // store a plain dumb back-pointer because we don't have to manage the
    // parent LLEventDispatcher's lifespan
    LLEventDispatcher* _parent;
    // The function-name string is purely descriptive. We want error messages
    // to be able to indicate which function's LLSDArgsMapper has the problem.
    std::string _function;
    // Store the names array pretty much as given.
    LLSD _names;
    // Though we're handed an array of name strings, it's more useful to us to
    // store it as a map from name string to position index. Of course that's
    // easy to generate from the incoming names array, but why do it more than
    // once?
    typedef std::map<LLSD::String, size_t> IndexMap;
    IndexMap _indexes;
    // Generated array of default values, aligned with the array of param names.
    LLSD _defaults;
    // Indicate whether we have a default value for each param.
    typedef std::vector<char> FilledVector;
    FilledVector _has_dft;
};

LLEventDispatcher::LLSDArgsMapper::LLSDArgsMapper(LLEventDispatcher* parent,
                                                  const std::string& function,
                                                  const LLSD& names,
                                                  const LLSD& defaults):
    _parent(parent),
    _function(function),
    _names(names),
    _has_dft(names.size())
{
    if (! (_names.isUndefined() || _names.isArray()))
    {
        callFail(" names must be an array, not ", names);
    }
    auto nparams(_names.size());
    // From _names generate _indexes.
    for (size_t ni = 0, nend = _names.size(); ni < nend; ++ni)
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
        auto ndefaults = defaults.size();
        // defaults is a (possibly empty) array. Right-align it with names.
        if (ndefaults > nparams)
        {
            callFail(" names array ", names, " shorter than defaults array ", defaults);
        }

        // Offset by which we slide defaults array right to right-align with
        // _names array
        auto offset = nparams - ndefaults;
        // Fill rightmost _defaults entries from defaults, and mark them as
        // filled
        for (size_t i = 0, iend = ndefaults; i < iend; ++i)
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

            auto pos = ixit->second;
            // Store default value at that position in the _defaults array.
            _defaults[pos] = mi->second;
            // Don't forget to record the fact that we've filled this
            // position.
            _has_dft[pos] = 1;
        }
        if (bogus.size())
        {
            callFail(" defaults specified for nonexistent params ", formatlist(bogus));
        }
    }
    else
    {
        callFail(" defaults must be a map or an array, not ", defaults);
    }
}

LLSD LLEventDispatcher::LLSDArgsMapper::map(const LLSD& argsmap) const
{
    if (! (argsmap.isUndefined() || argsmap.isMap() || argsmap.isArray()))
    {
        callFail(" map() needs a map or array, not ", argsmap);
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

    if (argsmap.isArray())
    {
        // Fill args from array. If there are too many args in passed array,
        // ignore the rest.
        auto size(argsmap.size());
        if (size > args.size())
        {
            // We don't just use std::min() because we want to sneak in this
            // warning if caller passes too many args.
            LL_WARNS("LLSDArgsMapper") << _function << " needs " << args.size()
                                       << " params, ignoring last " << (size - args.size())
                                       << " of passed " << size << ": " << argsmap << LL_ENDL;
            size = args.size();
        }
        for (LLSD::Integer i(0); i < size; ++i)
        {
            // Copy the actual argument from argsmap
            args[i] = argsmap[i];
            // Note that it's been filled
            filled[i] = 1;
        }
    }
    else
    {
        // argsmap is in fact a map. Walk the map.
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
            auto pos = ixit->second;
            // Store the value at that position in the args array.
            args[pos] = mi->second;
            // Don't forget to record the fact that we've filled this
            // position.
            filled[pos] = 1;
        }
    }

    // Fill any remaining holes from _defaults.
    LLSD unfilled(LLSD::emptyArray());
    for (size_t i = 0, iend = args.size(); i < iend; ++i)
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
        callFail(" missing required arguments ", formatlist(unfilled), " from ", argsmap);
    }

    // done
    return args;
}

std::string LLEventDispatcher::LLSDArgsMapper::formatlist(const LLSD& list)
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

template <typename... ARGS>
[[noreturn]] void LLEventDispatcher::LLSDArgsMapper::callFail(ARGS&&... args) const
{
    _parent->callFail<LLEventDispatcher::DispatchError>
        (_function, std::forward<ARGS>(args)...);
}

/*****************************************************************************
*   LLEventDispatcher
*****************************************************************************/
LLEventDispatcher::LLEventDispatcher(const std::string& desc, const std::string& key):
    LLEventDispatcher(desc, key, "args")
{}

LLEventDispatcher::LLEventDispatcher(const std::string& desc, const std::string& key,
                                     const std::string& argskey):
    mDesc(desc),
    mKey(key),
    mArgskey(argskey)
{}

LLEventDispatcher::~LLEventDispatcher()
{
}

LLEventDispatcher::DispatchEntry::DispatchEntry(LLEventDispatcher* parent, const std::string& desc):
    mParent(parent),
    mDesc(desc)
{}

/**
 * DispatchEntry subclass used for callables accepting(const LLSD&)
 */
struct LLEventDispatcher::LLSDDispatchEntry: public LLEventDispatcher::DispatchEntry
{
    LLSDDispatchEntry(LLEventDispatcher* parent, const std::string& desc,
                      const Callable& func, const LLSD& required):
        DispatchEntry(parent, desc),
        mFunc(func),
        mRequired(required)
    {}

    Callable mFunc;
    LLSD mRequired;

    LLSD call(const std::string& desc, const LLSD& event, bool, const std::string&) const override
    {
        // Validate the syntax of the event itself.
        std::string mismatch(llsd_matches(mRequired, event));
        if (! mismatch.empty())
        {
            callFail(desc, ": bad request: ", mismatch);
        }
        // Event syntax looks good, go for it!
        return mFunc(event);
    }

    LLSD getMetadata() const override
    {
        return llsd::map("required", mRequired);
    }
};

/**
 * DispatchEntry subclass for passing LLSD to functions accepting
 * arbitrary argument types (convertible via LLSDParam)
 */
struct LLEventDispatcher::ParamsDispatchEntry: public LLEventDispatcher::DispatchEntry
{
    ParamsDispatchEntry(LLEventDispatcher* parent, const std::string& name,
                        const std::string& desc, const invoker_function& func):
        DispatchEntry(parent, desc),
        mName(name),
        mInvoker(func)
    {}

    std::string mName;
    invoker_function mInvoker;

    LLSD call(const std::string&, const LLSD& event, bool, const std::string&) const override
    {
        try
        {
            return mInvoker(event);
        }
        catch (const LL::apply_error& err)
        {
            // could hit runtime errors with LL::apply()
            callFail(err.what());
        }
    }
};

/**
 * DispatchEntry subclass for dispatching LLSD::Array to functions accepting
 * arbitrary argument types (convertible via LLSDParam)
 */
struct LLEventDispatcher::ArrayParamsDispatchEntry: public LLEventDispatcher::ParamsDispatchEntry
{
    ArrayParamsDispatchEntry(LLEventDispatcher* parent, const std::string& name,
                             const std::string& desc, const invoker_function& func,
                             LLSD::Integer arity):
        ParamsDispatchEntry(parent, name, desc, func),
        mArity(arity)
    {}

    LLSD::Integer mArity;

    LLSD call(const std::string& desc, const LLSD& event, bool fromMap, const std::string& argskey) const override
    {
//      std::string context { stringize(desc, "(", event, ") with argskey ", std::quoted(argskey), ": ") };
        // Whether we try to extract arguments from 'event' depends on whether
        // the LLEventDispatcher consumer called one of the (name, event)
        // methods (! fromMap) or one of the (event) methods (fromMap). If we
        // were called with (name, event), the passed event must itself be
        // suitable to pass to the registered callable, no args extraction
        // required or even attempted. Only if called with plain (event) do we
        // consider extracting args from that event. Initially assume 'event'
        // itself contains the arguments.
        LLSD args{ event };
        if (fromMap)
        {
            if (! mArity)
            {
                // When the target function is nullary, and we're called from
                // an (event) method, just ignore the rest of the map entries.
                args.clear();
            }
            else
            {
                // We only require/retrieve argskey if the target function
                // isn't nullary. For all others, since we require an LLSD
                // array, we must have an argskey.
                if (argskey.empty())
                {
                    callFail("LLEventDispatcher has no args key");
                }
                if ((! event.has(argskey)))
                {
                    callFail("missing required key ", std::quoted(argskey));
                }
                args = event[argskey];
            }
        }
        return ParamsDispatchEntry::call(desc, args, fromMap, argskey);
    }

    LLSD getMetadata() const override
    {
        LLSD array(LLSD::emptyArray());
        // Resize to number of arguments required
        if (mArity)
            array[mArity - 1] = LLSD();
        llassert_always(array.size() == mArity);
        return llsd::map("required", array);
    }
};

/**
 * DispatchEntry subclass for dispatching LLSD::Map to functions accepting
 * arbitrary argument types (convertible via LLSDParam)
 */
struct LLEventDispatcher::MapParamsDispatchEntry: public LLEventDispatcher::ParamsDispatchEntry
{
    MapParamsDispatchEntry(LLEventDispatcher* parent, const std::string& name,
                           const std::string& desc, const invoker_function& func,
                           const LLSD& params, const LLSD& defaults):
        ParamsDispatchEntry(parent, name, desc, func),
        mMapper(parent, name, params, defaults),
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
            auto offset = params.size() - defaults.size();
            // Now the name of every defaults[i] is at params[i + offset].
            for (size_t i(0), iend(defaults.size()); i < iend; ++i)
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

    LLSD call(const std::string& desc, const LLSD& event, bool fromMap, const std::string& argskey) const override
    {
        // by default, pass the whole event as the arguments map
        LLSD args{ event };
        // Were we called by one of the (event) methods (instead of the (name,
        // event) methods), do we have an argskey, and does the incoming event
        // have that key?
        if (fromMap && (! argskey.empty()) && event.has(argskey))
        {
            // if so, extract the value of argskey from the incoming event,
            // and use that as the arguments map
            args = event[argskey];
        }
        // Now convert args from LLSD map to LLSD array using mMapper, then
        // pass to base-class call() method.
        return ParamsDispatchEntry::call(desc, mMapper.map(args), fromMap, argskey);
    }

    LLSD getMetadata() const override
    {
        return llsd::map("required", mRequired, "optional", mOptional);
    }
};

void LLEventDispatcher::addArrayParamsDispatchEntry(const std::string& name,
                                                    const std::string& desc,
                                                    const invoker_function& invoker,
                                                    LLSD::Integer arity)
{
    mDispatch.emplace(
        name,
        new ArrayParamsDispatchEntry(this, "", desc, invoker, arity));
}

void LLEventDispatcher::addMapParamsDispatchEntry(const std::string& name,
                                                  const std::string& desc,
                                                  const invoker_function& invoker,
                                                  const LLSD& params,
                                                  const LLSD& defaults)
{
    // Pass instance info as well as this entry name for error messages.
    mDispatch.emplace(
        name,
        new MapParamsDispatchEntry(this, "", desc, invoker, params, defaults));
}

/// Register a callable by name
void LLEventDispatcher::addLLSD(const std::string& name, const std::string& desc,
                                const Callable& callable, const LLSD& required)
{
    mDispatch.emplace(name, new LLSDDispatchEntry(this, desc, callable, required));
}

void LLEventDispatcher::addFail(const std::string& name, const char* classname) const
{
    LL_ERRS("LLEventDispatcher") << "LLEventDispatcher(" << mDesc << ")::add(" << name
                                 << "): " << LLError::Log::demangle(classname)
                                 << " is not a subclass of LLEventDispatcher"
                                 << LL_ENDL;
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

/// Call a registered callable with an explicitly-specified name. It is an
/// error if no such callable exists.
LLSD LLEventDispatcher::operator()(const std::string& name, const LLSD& event) const
{
    return try_call(std::string(), name, event);
}

bool LLEventDispatcher::try_call(const std::string& name, const LLSD& event) const
{
    try
    {
        try_call(std::string(), name, event);
        return true;
    }
    // Note that we don't catch the generic DispatchError, only the specific
    // DispatchMissing. try_call() only promises to return false if the
    // specified callable name isn't found -- not for general errors.
    catch (const DispatchMissing&)
    {
        return false;
    }
}

/// Extract the @a key value from the incoming @a event, and call the callable
/// whose name is specified by that map @a key. It is an error if no such
/// callable exists.
LLSD LLEventDispatcher::operator()(const LLSD& event) const
{
    return try_call(mKey, event[mKey], event);
}

bool LLEventDispatcher::try_call(const LLSD& event) const
{
    try
    {
        try_call(mKey, event[mKey], event);
        return true;
    }
    catch (const DispatchMissing&)
    {
        return false;
    }
}

LLSD LLEventDispatcher::try_call(const std::string& key, const std::string& name,
                                 const LLSD& event) const
{
    if (name.empty())
    {
        if (key.empty())
        {
            callFail<DispatchError>("attempting to call with no name");
        }
        else
        {
            callFail<DispatchError>("no ", key);
        }
    }

    DispatchMap::const_iterator found = mDispatch.find(name);
    if (found == mDispatch.end())
    {
        // Here we were passed a non-empty name, but there's no registered
        // callable with that name. This is the one case in which we throw
        // DispatchMissing instead of the generic DispatchError.
        // Distinguish the public method by which our caller reached here:
        // key.empty() means the name was passed explicitly, non-empty means
        // we extracted the name from the incoming event using that key.
        if (key.empty())
        {
            callFail<DispatchMissing>(std::quoted(name), " not found");
        }
        else
        {
            callFail<DispatchMissing>("bad ", key, " value ", std::quoted(name));
        }
    }

    // Found the name, so it's plausible to even attempt the call.
    const char* delim = (key.empty()? "" : "=");
    // append either "[key=name]" or just "[name]"
    SetState transient(this, '[', key, delim, name, ']');
    return found->second->call("", event, (! key.empty()), mArgskey);
}

template <typename EXCEPTION, typename... ARGS>
//static
[[noreturn]] void LLEventDispatcher::sCallFail(ARGS&&... args)
{
    auto error = stringize(std::forward<ARGS>(args)...);
    LL_WARNS("LLEventDispatcher") << error << LL_ENDL;
    LLTHROW(EXCEPTION(error));
}

template <typename EXCEPTION, typename... ARGS>
[[noreturn]] void LLEventDispatcher::callFail(ARGS&&... args) const
{
    // Describe this instance in addition to the error itself.
    sCallFail<EXCEPTION>(*this, ": ", std::forward<ARGS>(args)...);
}

LLSD LLEventDispatcher::getMetadata(const std::string& name) const
{
    DispatchMap::const_iterator found = mDispatch.find(name);
    if (found == mDispatch.end())
    {
        return LLSD();
    }
    LLSD meta{ found->second->getMetadata() };
    meta["name"] = name;
    meta["desc"] = found->second->mDesc;
    return meta;
}

std::ostream& operator<<(std::ostream& out, const LLEventDispatcher& self)
{
    // If we're a subclass of LLEventDispatcher, e.g. LLEventAPI, report that.
    // Also report whatever transient state is active.
    return out << LLError::Log::classname(self) << '(' << self.mDesc << ')'
               << self.getState();
}

std::string LLEventDispatcher::getState() const
{
    // default value of fiber_specific_ptr is nullptr, and ~SetState() reverts
    // to that; infer empty string
    if (! mState.get())
        return {};
    else
        return *mState;
}

bool LLEventDispatcher::setState(SetState&, const std::string& state) const
{
    // If SetState is instantiated at multiple levels of function call, ignore
    // the lower-level call because the outer call presumably provides more
    // context.
    if (mState.get())
        return false;

    // Pass us empty string (a la ~SetState()) to reset to nullptr, else take
    // a heap copy of the passed state string so we can delete it on
    // subsequent reset().
    mState.reset(state.empty()? nullptr : new std::string(state));
    return true;
}

/*****************************************************************************
*   LLDispatchListener
*****************************************************************************/
std::string LLDispatchListener::mReplyKey{ "reply" };

bool LLDispatchListener::process(const LLSD& event) const
{
    // Decide what to do based on the incoming value of the specified dispatch
    // key.
    LLSD name{ event[getDispatchKey()] };
    if (name.isMap())
    {
        call_map(name, event);
    }
    else if (name.isArray())
    {
        call_array(name, event);
    }
    else
    {
        call_one(name, event);
    }
    return false;
}

void LLDispatchListener::call_one(const LLSD& name, const LLSD& event) const
{
    LLSD result;
    try
    {
        result = (*this)(event);
    }
    catch (const DispatchError& err)
    {
        if (! event.has(mReplyKey))
        {
            // Without a reply key, let the exception propagate.
            throw;
        }

        // Here there was an error and the incoming event has mReplyKey. Reply
        // with a map containing an "error" key explaining the problem.
        return reply(llsd::map("error", err.what()), event);
    }

    // We seem to have gotten a valid result. But we don't know whether the
    // registered callable is void or non-void. If it's void,
    // LLEventDispatcher returned isUndefined(). Otherwise, try to send it
    // back to our invoker.
    if (result.isDefined())
    {
        if (! result.isMap())
        {
            // wrap the result in a map as the "data" key
            result = llsd::map("data", result);
        }
        reply(result, event);
    }
}

void LLDispatchListener::call_map(const LLSD& reqmap, const LLSD& event) const
{
    // LLSD map containing returned values
    LLSD result;
    // cache dispatch key
    std::string key{ getDispatchKey() };
    // collect any error messages here
    std::ostringstream errors;
    const char* delim = "";

    for (const auto& pair : llsd::inMap(reqmap))
    {
        const LLSD::String& name{ pair.first };
        const LLSD& args{ pair.second };
        try
        {
            // in case of errors, tell user the dispatch key, the fact that
            // we're processing a request map and the current key in that map
            SetState(this, '[', key, '[', name, "]]");
            // With this form, capture return value even if undefined:
            // presence of the key in the response map can be used to detect
            // which request keys succeeded.
            result[name] = (*this)(name, args);
        }
        catch (const std::exception& err)
        {
            // Catch not only DispatchError, but any C++ exception thrown by
            // the target callable. Collect exception name and message in
            // 'errors'.
            errors << delim << LLError::Log::classname(err) << ": " << err.what();
            delim = "\n";
        }
    }

    // so, were there any errors?
    std::string error = errors.str();
    if (! error.empty())
    {
        if (! event.has(mReplyKey))
        {
            // can't send reply, throw
            sCallFail<DispatchError>(error);
        }
        else
        {
            // reply key present
            result["error"] = error;
        }
    }

    reply(result, event);
}

void LLDispatchListener::call_array(const LLSD& reqarray, const LLSD& event) const
{
    // LLSD array containing returned values
    LLSD results;
    // cache the dispatch key
    std::string key{ getDispatchKey() };
    // arguments array, if present -- const because, if it's shorter than
    // reqarray, we don't want to grow it
    const LLSD argsarray{ event[getArgsKey()] };
    // error message, if any
    std::string error;

    // classic index loop because we need the index
    for (size_t i = 0, size = reqarray.size(); i < size; ++i)
    {
        const auto& reqentry{ reqarray[i] };
        std::string name;
        LLSD args;
        if (reqentry.isString())
        {
            name = reqentry.asString();
            args = argsarray[i];
        }
        else if (reqentry.isArray() && reqentry.size() == 2 && reqentry[0].isString())
        {
            name = reqentry[0].asString();
            args = reqentry[1];
        }
        else
        {
            // reqentry isn't in either of the documented forms
            error = stringize(*this, ": ", getDispatchKey(), '[', i, "] ",
                              reqentry, " unsupported");
            break;
        }

        // reqentry is one of the valid forms, got name and args
        try
        {
            // in case of errors, tell user the dispatch key, the fact that
            // we're processing a request array, the current entry in that
            // array and the corresponding callable name
            SetState(this, '[', key, '[', i, "]=", name, ']');
            // With this form, capture return value even if undefined
            results.append((*this)(name, args));
        }
        catch (const std::exception& err)
        {
            // Catch not only DispatchError, but any C++ exception thrown by
            // the target callable. Report the exception class as well as the
            // error string.
            error = stringize(LLError::Log::classname(err), ": ", err.what());
            break;
        }
    }

    LLSD result;
    // was there an error?
    if (! error.empty())
    {
        if (! event.has(mReplyKey))
        {
            // can't send reply, throw
            sCallFail<DispatchError>(error);
        }
        else
        {
            // reply key present
            result["error"] = error;
        }
    }

    // wrap the results array as response map "data" key, as promised
    if (results.isDefined())
    {
        result["data"] = results;
    }

    reply(result, event);
}

void LLDispatchListener::reply(const LLSD& reply, const LLSD& request) const
{
    // Call sendReply() unconditionally: sendReply() itself tests whether the
    // specified reply key is present in the incoming request, and does
    // nothing if there's no such key.
    sendReply(reply, request, mReplyKey);
}
