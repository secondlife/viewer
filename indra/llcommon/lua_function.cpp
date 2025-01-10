/**
 * @file   lua_function.cpp
 * @author Nat Goodspeed
 * @date   2024-02-05
 * @brief  Implementation for lua_function.
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "lua_function.h"
// STL headers
// std headers
#include <algorithm>
#include <exception>
#include <iomanip>                  // std::quoted
#include <map>
#include <memory>                   // std::unique_ptr
#include <typeinfo>
#include <unordered_map>
// external library headers
// other Linden headers
#include "commoncontrol.h"
#include "fsyspath.h"
#include "hexdump.h"
#include "llcoros.h"
#include "lleventcoro.h"
#include "llsd.h"
#include "llsdutil.h"
#include "llstring.h"
#include "lualistener.h"
#include "stringize.h"

using namespace std::literals;      // e.g. std::string_view literals: "this"sv

const S32 INTERRUPTS_MAX_LIMIT = 100000;
const S32 INTERRUPTS_SUSPEND_LIMIT = 100;

#define lua_register(L, n, f) (lua_pushcfunction(L, (f), n), lua_setglobal(L, (n)))
#define lua_rawlen lua_objlen

int DistinctInt::mValues{0};

/*****************************************************************************
*   lluau namespace
*****************************************************************************/
namespace
{
    // can't specify free function free() as a unique_ptr deleter
    struct freer
    {
        void operator()(void* ptr){ free(ptr); }
    };
} // anonymous namespace

namespace lluau
{

int dostring(lua_State* L, const std::string& desc, const std::string& text,
             const std::vector<std::string>& args)
{
    // debug.traceback() + compiled chunk + args table + args... + slop
    lluau_checkstack(L, 1 + 1 + 1 + int(args.size()) + 2);
    auto r = loadstring(L, desc, text);
    if (r != LUA_OK)
        return r;

    // Push debug.traceback() onto the stack as lua_pcall()'s error
    // handler function. On error, lua_pcall() calls the specified error
    // handler function with the original error message; the message
    // returned by the error handler is then returned by lua_pcall().
    // Luau's debug.traceback() is called with a message to prepend to the
    // returned traceback string. Almost as if they'd been designed to
    // work together...
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    // ditch "debug"
    lua_remove(L, -2);
    // stack: compiled chunk, debug.traceback()
    lua_insert(L, -2);
    // stack: debug.traceback(), compiled chunk
    // capture absolute index of debug.traceback()
    int traceback = lua_absindex(L, -2);
    // remove it from stack on exit
    LuaRemover cleanup(L, traceback);

    // Originally we just pushed 'args' to the Lua stack before entering the
    // chunk. But that's awkward for the chunk: it must reference those
    // arguments as '...', using any of a number of tactics to move them to
    // named variables. This doesn't work as a Lua chunk expecting arguments:
    // function(a, b, c)
    //     -- ...
    // end
    // because that code only *defines* a function: the function's body isn't
    // entered by executing the chunk.
    //
    // Per https://www.lua.org/manual/5.1/manual.html#6 Lua Stand-alone, we
    // now also create a global table called 'arg' whose [0] is the script
    // name, ['n'] is the number of additional arguments and [1] through
    // [['n']] are the additional arguments. We diverge from that spec in not
    // creating any negative indices.
    //
    // Since the spec notes that the chunk can also reference args using
    // '...', we also leave them on the stack.

    // stack: debug.traceback(), compiled chunk
    // create arg table pre-sized to hold the args array, plus [0] and ['n']
    lua_createtable(L, narrow(args.size()), 2);
    // stack: debug.traceback(), compiled chunk, arg table
    int argi = lua_absindex(L, -1);
    lua_Integer i = 0;
    // store desc (e.g. script name) as arg[0]
    lua_pushinteger(L, i);
    lua_pushstdstring(L, desc);
    lua_rawset(L, argi);            // rawset() pops key and value
    // store args.size() as arg.n
    lua_pushinteger(L, narrow(args.size()));
    lua_setfield(L, argi, "n");    // setfield() pops value
    for (const auto& arg : args)
    {
        // push each arg in order
        lua_pushstdstring(L, arg);
        // push index
        lua_pushinteger(L, ++i);
        // duplicate arg[i] to store in arg table
        lua_pushvalue(L, -2);
        // stack: ..., arg[i], i, arg[i]
        lua_rawset(L, argi);
        // leave ..., arg[i] on stack
    }
    // stack: debug.traceback(), compiled chunk, arg, arg[1], arg[2], ...
    // duplicate the arg table to store it
    lua_pushvalue(L, argi);
    lua_setglobal(L, "arg");
    lua_remove(L, argi);
    // stack: debug.traceback(), compiled chunk, arg[1], arg[2], ...

    // It's important to pass LUA_MULTRET as the expected number of return
    // values: if we pass any fixed number, we discard any returned values
    // beyond that number.
    return lua_pcall(L, int(args.size()), LUA_MULTRET, traceback);
}

int loadstring(lua_State *L, const std::string &desc, const std::string &text)
{
    lluau_checkstack(L, 1);
    size_t bytecodeSize = 0;
    // The char* returned by luau_compile() must be freed by calling free().
    // Use unique_ptr so the memory will be freed even if luau_load() throws.
    std::unique_ptr<char[], freer> bytecode{
        luau_compile(text.data(), text.length(), nullptr, &bytecodeSize)};
    return luau_load(L, desc.data(), bytecode.get(), bytecodeSize, 0);
}

fsyspath source_path(lua_State* L)
{
    //Luau lua_Debug and lua_getinfo() are different compared to default Lua:
    //see https://github.com/luau-lang/luau/blob/80928acb92d1e4b6db16bada6d21b1fb6fa66265/VM/include/lua.h
    // In particular:
    // passing level=1 gets you info about the deepest function call
    // passing level=lua_stackdepth() gets you info about the topmost script
    // Empirically, lua_getinfo(level > 1) behaves strangely (including
    // crashing the program) unless you iterate from 1 to desired level.
    lua_Debug ar{};
    for (int i(0), depth(lua_stackdepth(L)); i <= depth; ++i)
    {
        lua_getinfo(L, i, "s", &ar);
    }
    return { ar.source };
}

} // namespace lluau

/*****************************************************************************
*   lua_destroyuserdata(), lua_destroybounduserdata() (see lua_emplace<T>())
*****************************************************************************/
int lua_destroyuserdata(lua_State* L)
{
    // stack: lua_emplace() userdata to be destroyed
    if (int tag;
        lua_isuserdata(L, -1) &&
        (tag = lua_userdatatag(L, -1)) != 0)
    {
        auto dtor = lua_getuserdatadtor(L, tag);
        // detach this userdata from the destructor with tag 'tag'
        lua_setuserdatatag(L, -1, 0);
        // now run the real destructor
        dtor(L, lua_touserdata(L, -1));
    }
    lua_settop(L, 0);
    return 0;
}

int lua_destroybounduserdata(lua_State *L)
{
    // called with no arguments -- push bound upvalue
    lluau_checkstack(L, 1);
    lua_pushvalue(L, lua_upvalueindex(1));
    return lua_destroyuserdata(L);
}

/*****************************************************************************
*   Lua <=> C++ conversions
*****************************************************************************/
std::string lua_tostdstring(lua_State* L, int index)
{
    lua_checkdelta(L);
    size_t len;
    const char* strval{ lua_tolstring(L, index, &len) };
    return { strval, len };
}

void lua_pushstdstring(lua_State* L, const std::string& str)
{
    lua_checkdelta(L, 1);
    lluau_checkstack(L, 1);
    lua_pushlstring(L, str.c_str(), str.length());
}

// By analogy with existing lua_tomumble() functions, return an LLSD object
// corresponding to the Lua object at stack index 'index' in state L.
// This function assumes that a Lua caller is fully aware that they're trying
// to call a viewer function. In other words, the caller must specifically
// construct Lua data convertible to LLSD.
//
// For proper error handling, we REQUIRE that the Lua runtime be compiled as
// C++ so errors are raised as C++ exceptions rather than as longjmp() calls:
// http://www.lua.org/manual/5.4/manual.html#4.4
// "Internally, Lua uses the C longjmp facility to handle errors. (Lua will
// use exceptions if you compile it as C++; search for LUAI_THROW in the
// source code for details.)"
// Some blocks within this function construct temporary C++ objects in the
// expectation that these objects will be properly destroyed even if code
// reached by that block raises a Lua error.
LLSD lua_tollsd(lua_State* L, int index)
{
    lua_checkdelta(L);
    switch (lua_type(L, index))
    {
    case LUA_TNONE:
        // Should LUA_TNONE be an error instead of returning isUndefined()?
    case LUA_TNIL:
        return {};

    case LUA_TBOOLEAN:
        return bool(lua_toboolean(L, index));

    case LUA_TNUMBER:
    {
        // Vanilla Lua supports lua_tointegerx(), which tells the caller
        // whether the number at the specified stack index is or is not an
        // integer. Apparently the function exists but does not work right in
        // Luau: it reports even non-integer numbers as integers.
        // Instead, check if integer truncation leaves the number intact.
        lua_Number  numval{ lua_tonumber(L, index) };
        lua_Integer intval{ narrow(numval) };
        if (lua_Number(intval) == numval)
        {
            return LLSD::Integer(intval);
        }
        else
        {
            return numval;
        }
    }

    case LUA_TSTRING:
        return lua_tostdstring(L, index);

    case LUA_TUSERDATA:
    {
        LLSD::Binary binary(lua_rawlen(L, index));
        std::memcpy(binary.data(), lua_touserdata(L, index), binary.size());
        return binary;
    }

    case LUA_TTABLE:
    {
        // A Lua table correctly constructed to convert to LLSD will have
        // either consecutive integer keys starting at 1, which we represent
        // as an LLSD array (with Lua key 1 at C++ index 0), or will have
        // all string keys.
        //
        // In the belief that Lua table traversal skips "holes," that is, it
        // doesn't report any key/value pair whose value is nil, we allow a
        // table with integer keys >= 1 but with "holes." This produces an
        // LLSD array with isUndefined() entries at unspecified keys. There
        // would be no other way for a Lua caller to construct an
        // isUndefined() LLSD array entry. However, to guard against crazy int
        // keys, we forbid gaps larger than a certain size: crazy int keys
        // could result in a crazy large contiguous LLSD array.
        //
        // Possible looseness could include:
        // - A mix of integer and string keys could produce an LLSD map in
        //   which the integer keys are converted to string. (Key conversion
        //   must be performed in C++, not Lua, to avoid confusing
        //   lua_next().)
        // - However, since in Lua t[0] and t["0"] are distinct table entries,
        //   do not consider converting numeric string keys to int to return
        //   an LLSD array.
        // But until we get more experience with actual Lua scripts in
        // practice, let's say that any deviation is a Lua coding error.
        // An important property of the strict definition above is that most
        // conforming data blobs can make a round trip across the language
        // boundary and still compare equal. A non-conforming data blob would
        // lose that property.
        // Known exceptions to round trip identity:
        // - Empty LLSD map and empty LLSD array convert to empty Lua table.
        //   But empty Lua table converts to isUndefined() LLSD object.
        // - LLSD::Real with integer value returns as LLSD::Integer.
        // - LLSD::UUID, LLSD::Date and LLSD::URI all convert to Lua string,
        //   and so return as LLSD::String.
        // - Lua does not store any table key whose value is nil. An LLSD
        //   array with isUndefined() entries produces a Lua table with
        //   "holes" in the int key sequence; this converts back to an LLSD
        //   array containing corresponding isUndefined() entries -- except
        //   when one or more of the final entries isUndefined(). These are
        //   simply dropped, producing a shorter LLSD array than the original.
        // - For the same reason, any keys in an LLSD map whose value
        //   isUndefined() are simply discarded in the converted Lua table.
        //   This converts back to an LLSD map lacking those keys.
        // - If it's important to preserve the original length of an LLSD
        //   array whose final entries are undefined, or the full set of keys
        //   for an LLSD map some of whose values are undefined, store an
        //   LLSD::emptyArray() or emptyMap() instead. These will be
        //   represented in Lua as empty table, which should convert back to
        //   undefined LLSD. Naturally, though, those won't survive a second
        //   round trip.

        // This is the most important of the lluau_checkstack() calls because a
        // deeply nested Lua structure will enter this case at each level, and
        // we'll need another 2 stack slots to traverse each nested table.
        lluau_checkstack(L, 2);
        // BEFORE we push nil to initialize the lua_next() traversal, convert
        // 'index' to absolute! Our caller might have passed a relative index;
        // we do, below: lua_tollsd(L, -1). If 'index' is -1, then when we
        // push nil, what we find at index -1 is nil, not the table!
        index = lua_absindex(L, index);
        lua_pushnil(L);             // first key
        if (! lua_next(L, index))
        {
            // it's a table, but the table is empty -- no idea if it should be
            // modeled as empty array or empty map -- return isUndefined(),
            // which can be consumed as either
            return {};
        }
        // key is at stack index -2, value at index -1
        // from here until lua_next() returns 0, have to lua_pop(2) if we
        // return early
        LuaPopper popper(L, 2);
        // Remember the type of the first key
        auto firstkeytype{ lua_type(L, -2) };
        switch (firstkeytype)
        {
        case LUA_TNUMBER:
        {
            // First Lua key is a number: try to convert table to LLSD array.
            // This is tricky because we don't know in advance the size of the
            // array. The Lua reference manual says that lua_rawlen() is the
            // same as the length operator '#'; but the length operator states
            // that it might stop at any "hole" in the subject table.
            // Moreover, the Lua next() function (and presumably lua_next())
            // traverses a table in unspecified order, even for numeric keys
            // (emphasized in the doc).
            // Make a preliminary pass over the whole table to validate and to
            // collect keys.
            std::vector<LLSD::Integer> keys;
            // Try to determine the length of the table. If the length
            // operator is truthful, avoid allocations while we grow the keys
            // vector. Even if it's not, we can still grow the vector, albeit
            // a little less efficiently.
            keys.reserve(lua_objlen(L, index));
            do
            {
                auto arraykeytype{ lua_type(L, -2) };
                switch (arraykeytype)
                {
                case LUA_TNUMBER:
                {
                    int isint;
                    lua_Integer intkey{ lua_tointegerx(L, -2, &isint) };
                    if (! isint)
                    {
                        // key isn't an integer - this doesn't fit our LLSD
                        // array constraints
                        return lluau::error(L, "Expected integer array key, got %f instead",
                                            lua_tonumber(L, -2));
                    }
                    if (intkey < 1)
                    {
                        return lluau::error(L, "array key %d out of bounds", int(intkey));
                    }

                    keys.push_back(LLSD::Integer(intkey));
                    break;
                }

                case LUA_TSTRING:
                    // break out strings specially to report the value
                    return lluau::error(L, "Cannot convert string array key '%s' to LLSD",
                                        lua_tostring(L, -2));

                default:
                    return lluau::error(L, "Cannot convert %s array key to LLSD",
                                        lua_typename(L, arraykeytype));
                }

                // remove value, keep key for next iteration
                lua_pop(L, 1);
            } while (lua_next(L, index) != 0);
            popper.disarm();
            // Table keys are all integers: are they reasonable integers?
            // Arbitrary max: may bite us, but more likely to protect us
            const size_t array_max{ 10000 };
            if (keys.size() > array_max)
            {
                return lluau::error(L, "Conversion from Lua to LLSD array limited to %d entries",
                                    int(array_max));
            }
            // We know the smallest key is >= 1. Check the largest. We also
            // know the vector is NOT empty, else we wouldn't have gotten here.
            std::sort(keys.begin(), keys.end());
            LLSD::Integer highkey = *keys.rbegin();
            if ((highkey - LLSD::Integer(keys.size())) > 100)
            {
                // Looks like we've gone beyond intentional array gaps into
                // crazy key territory.
                return lluau::error(L, "Gaps in Lua table too large for conversion to LLSD array");
            }
            // right away expand the result array to the size we'll need
            LLSD result{ LLSD::emptyArray() };
            result[highkey - 1] = LLSD();
            // Traverse the table again, and this time populate result array.
            lua_pushnil(L);         // first key
            while (lua_next(L, index))
            {
                // key at stack index -2, value at index -1
                // We've already validated lua_tointegerx() for each key.
                auto key{ lua_tointeger(L, -2) };
                // Don't forget to subtract 1 from Lua key for LLSD subscript!
                result[LLSD::Integer(key) - 1] = lua_tollsd(L, -1);
                // remove value, keep key for next iteration
                lua_pop(L, 1);
            }
            return result;
        }

        case LUA_TSTRING:
        {
            // First Lua key is a string: try to convert table to LLSD map
            LLSD result{ LLSD::emptyMap() };
            do
            {
                auto mapkeytype{ lua_type(L, -2) };
                if (mapkeytype != LUA_TSTRING)
                {
                    return lluau::error(L, "Cannot convert %s map key to LLSD",
                                        lua_typename(L, mapkeytype));
                }

                auto key{ lua_tostdstring(L, -2) };
                result[key] = lua_tollsd(L, -1);
                // remove value, keep key for next iteration
                lua_pop(L, 1);
            } while (lua_next(L, index) != 0);
            popper.disarm();
            return result;
        }

        default:
            // First Lua key isn't number or string: sorry
            return lluau::error(L, "Cannot convert %s table key to LLSD",
                                lua_typename(L, firstkeytype));
        }
    }

    default:
        // Other Lua entities (e.g. function, C function, light userdata,
        // thread, userdata) are not convertible to LLSD, indicating a coding
        // error in the caller.
        return lluau::error(L, "Cannot convert type %s to LLSD", luaL_typename(L, index));
    }
}

// By analogy with existing lua_pushmumble() functions, push onto state L's
// stack a Lua object corresponding to the passed LLSD object.
void lua_pushllsd(lua_State* L, const LLSD& data)
{
    lua_checkdelta(L, 1);
    // might need 2 slots for array or map
    lluau_checkstack(L, 2);
    switch (data.type())
    {
    case LLSD::TypeUndefined:
        lua_pushnil(L);
        break;

    case LLSD::TypeBoolean:
        lua_pushboolean(L, data.asBoolean());
        break;

    case LLSD::TypeInteger:
        lua_pushinteger(L, data.asInteger());
        break;

    case LLSD::TypeReal:
        lua_pushnumber(L, data.asReal());
        break;

    case LLSD::TypeBinary:
    {
        auto binary{ data.asBinary() };
        std::memcpy(lua_newuserdata(L, binary.size()),
                    binary.data(), binary.size());
        break;
    }

    case LLSD::TypeMap:
    {
        // push a new table with space for our non-array keys
        lua_createtable(L, 0, narrow(data.size()));
        for (const auto& pair: llsd::inMap(data))
        {
            // push value -- so now table is at -2, value at -1
            lua_pushllsd(L, pair.second);
            // pop value, assign to table[key]
            lua_setfield(L, -2, pair.first.c_str());
        }
        break;
    }

    case LLSD::TypeArray:
    {
        // push a new table with space for array entries
        lua_createtable(L, narrow(data.size()), 0);
        lua_Integer key{ 0 };
        for (const auto& item: llsd::inArray(data))
        {
            // push new array value: table at -2, value at -1
            lua_pushllsd(L, item);
            // pop value, assign table[key] = value
            lua_rawseti(L, -2, ++key);
        }
        break;
    }

    case LLSD::TypeString:
    case LLSD::TypeUUID:
    case LLSD::TypeDate:
    case LLSD::TypeURI:
    default:
    {
        lua_pushstdstring(L, data.asString());
        break;
    }
    }
}

/*****************************************************************************
*   LuaState class
*****************************************************************************/
namespace
{

// If we find we're running Lua scripts from more than one thread, sLuaStateMap
// should be thread_local. Until then, avoid the overhead.
using LuaStateMap = std::unordered_map<lua_State*, LuaState*>;
static LuaStateMap sLuaStateMap;

// replace table-at-index[name] with passed func,
// binding the original table-at-index[name] as func's upvalue
void replace_entry(lua_State* L, int index,
                   const std::string& name, lua_CFunction func);

// replacement next() function that understands setdtor() proxy args
int lua_proxydrill(lua_State* L);
// replacement pairs() function that supports __iter() metamethod
int lua_metapairs(lua_State* L);
// replacement ipairs() function that supports __index() metamethod
int lua_metaipairs(lua_State* L);
// helper for lua_metaipairs() (actual generator function)
int lua_metaipair(lua_State* L);

} // anonymous namespace

LuaState::LuaState()
{
    /*---------------------------- feature flag ----------------------------*/
    try
    {
        mFeature = LL::CommonControl::get("Global", "LuaFeature").asBoolean();
    }
    catch (const LL::CommonControl::NoListener&)
    {
        // If this program doesn't have an LLViewerControlListener,
        // it's probably a test program; go ahead.
        mFeature = true;
    }
    catch (const LL::CommonControl::ParamError&)
    {
        // We found LLViewerControlListener, but its settings do not include
        // "LuaFeature". Hmm, fishy: that feature flag was introduced at the
        // same time as this code.
        mFeature = false;
    }
    // None of the rest of this is necessary if we're not going to run anything.
    if (! mFeature)
    {
        mError = "Lua feature disabled";
        return;
    }
    /*---------------------------- feature flag ----------------------------*/

    mState = luaL_newstate();
    // Ensure that we can always find this LuaState instance, given the
    // lua_State we just created or any of its coroutines.
    sLuaStateMap.emplace(mState, this);
    luaL_openlibs(mState);
    // publish to this new lua_State all the LL entry points we defined using
    // the lua_function() macro
    LuaFunction::init(mState);
    // Try to make print() write to our log.
    lua_register(mState, "print", LuaFunction::get("print_info"));
    // We don't want to have to prefix require().
    lua_register(mState, "require", LuaFunction::get("require"));

    // Replace certain key global functions so they understand our
    // LL.setdtor() proxy objects.
    // (We could also do this for selected library functions as well,
    // e.g. the table, string, math libraries... let's see if needed.)
    replace_entry(mState, LUA_GLOBALSINDEX, "next", lua_proxydrill);
    // Replacing pairs() with lua_metapairs() makes global pairs() honor
    // objects with __iter() metamethods.
    replace_entry(mState, LUA_GLOBALSINDEX, "pairs", lua_metapairs);
    // Replacing ipairs() with lua_metaipairs() makes global ipairs() honor
    // objects with __index() metamethods -- as long as the object in question
    // has no array entries (int keys) of its own. (If it does, object[i] will
    // retrieve table[i] instead of calling __index(table, i).)
    replace_entry(mState, LUA_GLOBALSINDEX, "ipairs", lua_metaipairs);
}

namespace
{

void replace_entry(lua_State* L, int index,
                   const std::string& name, lua_CFunction func)
{
    index = lua_absindex(L, index);
    lua_checkdelta(L);
    // push the function's name string twice
    lua_pushlstring(L, name.data(), name.length());
    lua_pushvalue(L, -1);
    // stack: name, name
    // look up the existing table entry
    lua_rawget(L, index);
    // stack: name, original function
    // bind original function as the upvalue for func()
    lua_pushcclosure(L, func, (name + "()").c_str(), 1);
    // stack: name, func-with-bound-original
    // table[name] = func-with-bound-original
    lua_rawset(L, index);
}

int lua_metapairs(lua_State* L)
{
//  LuaLog debug(L, "lua_metapairs()");
    // pairs(obj): object is at index 1
    // How many args were we passed?
    int args = lua_gettop(L);
    // stack: obj, ...
    if (luaL_getmetafield(L, 1, "__iter"))
    {
        // stack: obj, ..., getmetatable(obj).__iter
    }
    else
    {
        // Push the original pairs() function, captured as our upvalue.
        lua_pushvalue(L, lua_upvalueindex(1));
        // stack: obj, ..., original pairs()
    }
    lua_insert(L, 1);
    // stack: (__iter() or pairs()), obj, ...
    // call whichever function(obj, ...) (args args, up to 3 return values)
    lua_call(L, args, LUA_MULTRET);
    // return as many values as the selected function returned
    return lua_gettop(L);
}

int lua_metaipairs(lua_State* L)
{
//  LuaLog debug(L, "lua_metaipairs()");
    // ipairs(obj): object is at index 1
    // How many args were we passed?
    int args = lua_gettop(L);
    // stack: obj, ...
    if (luaL_getmetafield(L, 1, "__index"))
    {
        // stack: obj, ..., getmetatable(obj).__index
        // discard __index and everything but obj:
        // we don't want to call __index(), just check its presence
        lua_settop(L, 1);
        // stack: obj
        lua_pushcfunction(L, lua_metaipair, "lua_metaipair");
        // stack: obj, lua_metaipair
        lua_insert(L, 1);
        // stack: lua_metaipair, obj
        // push explicit 0 so lua_metaipair need not special-case nil
        lua_pushinteger(L, 0);
        // stack: lua_metaipair, obj, 0
        return 3;
    }
    else                            // no __index() metamethod
    {
        // Although our lua_metaipair() function demonstrably works whether or
        // not our object has an __index() metamethod, the code below assumes
        // that the Lua engine may have a more efficient implementation for
        // built-in ipairs() than our lua_metaipair().
        // Push the original ipairs() function, captured as our upvalue.
        lua_pushvalue(L, lua_upvalueindex(1));
        // stack: obj, ..., original ipairs()
        // Shift the stack so the original function is first.
        lua_insert(L, 1);
        // stack: original ipairs(), obj, ...
        // Call original ipairs() with all original args, no error checking.
        // Don't truncate however many values that function returns.
        lua_call(L, args, LUA_MULTRET);
        // Return as many values as the original function returned.
        return lua_gettop(L);
    }
}

int lua_metaipair(lua_State* L)
{
//  LuaLog debug(L, "lua_metaipair()");
    // called with (obj, previous-index)
    // increment previous-index for this call
    lua_Integer i = luaL_optinteger(L, 2, 0) + 1;
    lua_pop(L, 1);
    // stack: obj
    lua_pushinteger(L, i);
    // stack: obj, i
    lua_pushvalue(L, -1);
    // stack: obj, i, i
    lua_insert(L, 1);
    // stack: i, obj, i
    lua_gettable(L, -2);
    // stack: i, obj, obj[i] (honoring __index())
    lua_remove(L, -2);
    // stack: i, obj[i]
    if (! lua_isnil(L, -1))
    {
        // great, obj[i] isn't nil: return (i, obj[i])
        return 2;
    }
    // obj[i] is nil. ipairs() is documented to stop at the first hole,
    // regardless of #obj. Clear the stack, i.e. return nil.
    lua_settop(L, 0);
    return 0;
}

} // anonymous namespace

int LuaState::push_debug_traceback()
{
    // Push debug.traceback() onto the stack as lua_pcall()'s error
    // handler function. On error, lua_pcall() calls the specified error
    // handler function with the original error message; the message
    // returned by the error handler is then returned by lua_pcall().
    // Luau's debug.traceback() is called with a message to prepend to the
    // returned traceback string. Almost as if they'd been designed to
    // work together...
    lua_getglobal(mState, "debug");
    if (!lua_istable(mState, -1))
    {
        lua_pop(mState, 1);
        LL_WARNS("Lua") << "'debug' table not found" << LL_ENDL;
        return 0;
    }
    lua_getfield(mState, -1, "traceback");
    if (!lua_isfunction(mState, -1))
    {
        lua_pop(mState, 2);
        LL_WARNS("Lua") << "'traceback' func not found" << LL_ENDL;
        return 0;
    }
    // ditch "debug"
    lua_remove(mState, -2);
    return lua_gettop(mState);
}

LuaState::~LuaState()
{
    // If we're unwinding the stack due to an exception, don't bother trying
    // to call any callbacks -- either Lua or C++.
    if (std::uncaught_exceptions() != 0)
        return;

    /*---------------------------- feature flag ----------------------------*/
    if (! mFeature)
        return;
    /*---------------------------- feature flag ----------------------------*/

    // We're just about to destroy this lua_State mState. Did this Lua chunk
    // register any atexit() functions?
    lluau_checkstack(mState, 3);
    // look up Registry.atexit
    lua_getfield(mState, LUA_REGISTRYINDEX, "atexit");
    // stack contains Registry.atexit
    if (lua_istable(mState, -1))
    {
        int atexit = lua_gettop(mState);

        // We happen to know that Registry.atexit is built by appending array
        // entries using table.insert(). That's important because it means
        // there are no holes, and therefore lua_objlen() should be correct.
        // That's important because we walk the atexit table backwards, to
        // destroy last the things we created (passed to LL.atexit()) first.
        int len(lua_objlen(mState, -1));
        LL_DEBUGS("Lua") << LLCoros::getName() << ": Registry.atexit is a table with "
                         << len << " entries" << LL_ENDL;

        // TODO: 'debug' global shouldn't be overwritten and should be accessible at this stage
        S32 debug_traceback_idx = push_debug_traceback();
        // if debug_traceback is true, stack now contains atexit, /debug.traceback()/
        // otherwise just atexit
        for (int i(len); i >= 1; --i)
        {
            lua_pushinteger(mState, i);
            // stack contains Registry.atexit, /debug.traceback()/, i
            lua_gettable(mState, atexit);
            // stack contains Registry.atexit, /debug.traceback()/, atexit[i]
            // Call atexit[i](), no args, no return values.
            // Use lua_pcall() because errors in any one atexit() function
            // shouldn't cancel the rest of them. Pass debug.traceback() as
            // the error handler function.
            LL_DEBUGS("Lua") << LLCoros::getName() << ": calling atexit(" << i << ")" << LL_ENDL;
            if (lua_pcall(mState, 0, 0, debug_traceback_idx) != LUA_OK)
            {
                auto error{ lua_tostdstring(mState, -1) };
                LL_WARNS("Lua") << LLCoros::getName() << ": atexit(" << i << ") error: " << error << LL_ENDL;
                // pop error message
                lua_pop(mState, 1);
            }
            LL_DEBUGS("Lua") << LLCoros::getName() << ": atexit(" << i << ") done" << LL_ENDL;
            // lua_pcall() has already popped atexit[i]:
            // stack contains atexit, debug.traceback()
        }
        // pop debug.traceback()
        if (debug_traceback_idx)
            lua_remove(mState, debug_traceback_idx);
    }
    // pop Registry.atexit (either table or nil)
    lua_pop(mState, 1);

    // with the demise of this LuaState, remove sLuaStateMap entry
    sLuaStateMap.erase(mState);

    lua_close(mState);
}

bool LuaState::checkLua(const std::string& desc, int r)
{
    if (r != LUA_OK)
    {
        mError = lua_tostring(mState, -1);
        lua_pop(mState, 1);

        LL_WARNS("Lua") << desc << ": " << mError << LL_ENDL;
        return false;
    }
    return true;
}

std::pair<int, LLSD> LuaState::expr(const std::string& desc, const std::string& text,
                                    const std::vector<std::string>& args)
{
    /*---------------------------- feature flag ----------------------------*/
    if (! mFeature)
    {
        // fake an error
        return { -1, stringize("Not running ", desc) };
    }
    /*---------------------------- feature flag ----------------------------*/

    set_interrupts_counter(0);

    lua_callbacks(mState)->interrupt = [](lua_State *L, int gc)
    {
        // skip if we're interrupting only for garbage collection
        if (gc >= 0)
            return;

        LLCoros::checkStop();
        LuaState::getParent(L).check_interrupts_counter();
    };

    LL_INFOS("Lua") << desc << " run" << LL_ENDL;
    if (! checkLua(desc, lluau::dostring(mState, desc, text, args)))
    {
        LL_WARNS("Lua") << desc << " error: " << mError << LL_ENDL;
        return { -1, mError };
    }

    // here we believe there was no error -- did the Lua fragment leave
    // anything on the stack?
    std::pair<int, LLSD> result{ lua_gettop(mState), {} };
    LL_INFOS("Lua") << desc << " done, " << result.first << " results." << LL_ENDL;
    if (result.first)
    {
        // aha, at least one entry on the stack!
        if (result.first == 1)
        {
            // Don't forget that lua_tollsd() can throw Lua errors.
            try
            {
                result.second = lua_tollsd(mState, 1);
            }
            catch (const std::exception& error)
            {
                LL_WARNS("Lua") << desc << " error converting result: " << error.what() << LL_ENDL;
                // lua_tollsd() is designed to be called from a lua_function(),
                // that is, from a C++ function called by Lua. In case of error,
                // it throws a Lua error to be caught by the Lua runtime. expr()
                // is a peculiar use case in which our C++ code is calling
                // lua_tollsd() after return from the Lua runtime. We must catch
                // the exception thrown for a Lua error, else it will propagate
                // out to the main coroutine and terminate the viewer -- but since
                // we instead of the Lua runtime catch it, our lua_State retains
                // its internal error status. Any subsequent lua_pcall() calls
                // with this lua_State will report error regardless of whether the
                // chunk runs successfully.
                return { -1, stringize(LLError::Log::classname(error), ": ", error.what()) };
            }
        }
        else
        {
            // multiple entries on the stack
            int index;
            try
            {
                for (index = 1; index <= result.first; ++index)
                {
                    result.second.append(lua_tollsd(mState, index));
                }
            }
            catch (const std::exception& error)
            {
                LL_WARNS("Lua") << desc << " error converting result " << index << ": "
                                << error.what() << LL_ENDL;
                // see above comments regarding lua_State's error status
                return { -1, stringize(LLError::Log::classname(error), ": ", error.what()) };
            }
        }
    }
    // pop everything
    lua_settop(mState, 0);
    return result;
}

// We think we don't need mFeature tests in the rest of these LuaState methods
// because, if expr() isn't running code, nobody should be calling any of them.

LuaListener& LuaState::obtainListener(lua_State* L)
{
    lluau_checkstack(L, 2);
    lua_getfield(L, LUA_REGISTRYINDEX, "LuaListener");
    // compare lua_type() because lua_isuserdata() also accepts light userdata
    if (lua_type(L, -1) != LUA_TUSERDATA)
    {
        llassert(lua_type(L, -1) == LUA_TNIL);
        lua_pop(L, 1);
        // push a userdata containing new LuaListener, binding L
        lua_emplace<LuaListener>(L, L);
        // duplicate the top stack entry so we can store one copy
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, "LuaListener");
    }
    // At this point, one way or the other, the stack top should be (a Lua
    // userdata containing) our LuaListener.
    LuaListener* listener{ lua_toclass<LuaListener>(L, -1) };
    // Since our LuaListener instance is stored in the Registry, it won't be
    // garbage collected: it will be destroyed only when lua_close() clears
    // out the Registry. That's why we dare pop the userdata value off the
    // stack while still depending on a pointer into its data.
    lua_pop(L, 1);
    return *listener;
}

LuaState& LuaState::getParent(lua_State* L)
{
    // Look up the LuaState instance associated with the *script*, not the
    // specific Lua *coroutine*. In other words, first find this lua_State's
    // main thread.
    auto found{ sLuaStateMap.find(lua_mainthread(L)) };
    // Our constructor creates the map entry, our destructor deletes it. As
    // long as the LuaState exists, we should be able to find it. And we
    // SHOULD only be talking to a lua_State managed by a LuaState instance.
    llassert(found != sLuaStateMap.end());
    return *found->second;
}

void LuaState::set_interrupts_counter(S32 counter)
{
    mInterrupts = counter;
}

void LuaState::check_interrupts_counter()
{
    // The official way to manage data associated with a lua_State is to store
    // it *as* Lua data within the lua_State. But this method is called by the
    // Lua engine via lua_callbacks(L)->interrupt, and empirically we've hit
    // mysterious Lua data stack overflows trying to use stack-based Lua data
    // access functions in that situation. It seems the Lua engine is capable
    // of interrupting itself at a moment when re-entry is not valid. So only
    // touch data in this LuaState.
    ++mInterrupts;
    if (mInterrupts > INTERRUPTS_MAX_LIMIT)
    {
        lluau::error(mState, "Possible infinite loop, terminated.");
    }
    else if (mInterrupts % INTERRUPTS_SUSPEND_LIMIT == 0)
    {
        LL_DEBUGS("Lua.suspend") << LLCoros::getName() << " suspending at "
                                 << mInterrupts << " interrupts" << LL_ENDL;
        llcoro::suspend();
    }
}

/*****************************************************************************
*   atexit()
*****************************************************************************/
lua_function(atexit, "atexit(function): "
             "register Lua function to be called at script termination")
{
    lua_checkdelta(L, -1);
    lluau_checkstack(L, 4);
    // look up the global name "table"
    lua_getglobal(L, "table");
    // stack contains function, table
    // look up table.insert
    lua_getfield(L, -1, "insert");
    // stack contains function, table, table.insert
    // ditch table
    lua_replace(L, -2);
    // stack contains function, table.insert
    // find or create the "atexit" table in the Registry
    luaL_newmetatable(L, "atexit");
    // stack contains function, table.insert, Registry.atexit
    // we were called with a Lua function to append to that Registry.atexit
    // table -- push function
    lua_pushvalue(L, 1);            // or -3
    // stack contains function, table.insert, Registry.atexit, function
    // call table.insert(Registry.atexit, function)
    // don't use pcall(): if there's an error, let it propagate
    lua_call(L, 2, 0);
    // stack contains function -- pop everything
    lua_settop(L, 0);
    return 0;
}

/*****************************************************************************
*   LuaPopper class
*****************************************************************************/
LuaPopper::~LuaPopper()
{
    // If we're unwinding the C++ stack due to an exception, don't pop!
    if (std::uncaught_exceptions() == 0 && mCount)
    {
        lua_pop(mState, mCount);
    }
}

/*****************************************************************************
*   LuaFunction class
*****************************************************************************/
LuaFunction::LuaFunction(std::string_view name, lua_CFunction function,
                         std::string_view helptext)
{
    const auto& [registry, lookup] = getState();
    registry.emplace(name, Registry::mapped_type{ function, helptext });
    lookup.emplace(function, name);
}

void LuaFunction::init(lua_State* L)
{
    const auto& [registry, lookup] = getRState();
    lluau_checkstack(L, 2);
    // create LL table --
    // it happens that we know exactly how many non-array members we want
    lua_createtable(L, 0, int(narrow(lookup.size())));
    int idx = lua_gettop(L);
    for (const auto& [name, pair]: registry)
    {
        const auto& [funcptr, helptext] = pair;
        // store funcptr in LL table with saved name
        lua_pushcfunction(L, funcptr, name.c_str());
        lua_setfield(L, idx, name.c_str());
    }
    // store LL in new lua_State's globals
    lua_setglobal(L, "LL");
}

lua_CFunction LuaFunction::get(const std::string& key)
{
    // use find() instead of subscripting to avoid creating an entry for
    // unknown key
    const auto& [registry, lookup] = getState();
    auto found{ registry.find(key) };
    return (found == registry.end())? nullptr : found->second.first;
}

std::pair<LuaFunction::Registry&, LuaFunction::Lookup&> LuaFunction::getState()
{
    // use function-local statics to ensure they're initialized
    static Registry registry;
    static Lookup lookup;
    return { registry, lookup };
}

/*****************************************************************************
*   source_path()
*****************************************************************************/
lua_function(source_path, "source_path(): return the source path of the running Lua script")
{
    lua_checkdelta(L, 1);
    lluau_checkstack(L, 1);
    lua_pushstdstring(L, lluau::source_path(L));
    return 1;
}

/*****************************************************************************
*   source_dir()
*****************************************************************************/
lua_function(source_dir, "source_dir(): return the source directory of the running Lua script")
{
    lua_checkdelta(L, 1);
    lluau_checkstack(L, 1);
    lua_pushstdstring(L, fsyspath(lluau::source_path(L).parent_path()));
    return 1;
}

/*****************************************************************************
*   abspath()
*****************************************************************************/
lua_function(abspath, "abspath(path): "
             "for given filesystem path relative to running script, return absolute path")
{
    lua_checkdelta(L);
    auto path{ lua_tostdstring(L, 1) };
    lua_pop(L, 1);
    lua_pushstdstring(L, fsyspath(lluau::source_path(L).parent_path() / path));
    return 1;
}

/*****************************************************************************
*   check_stop()
*****************************************************************************/
lua_function(check_stop, "check_stop(): ensure that a Lua script responds to viewer shutdown")
{
    lua_checkdelta(L);
    LLCoros::checkStop();
    return 0;
}

/*****************************************************************************
*   help()
*****************************************************************************/
lua_function(help,
             "help(): list viewer's Lua functions\n"
             "LL.help(function): show help string for specific function")
{
    auto& luapump{ LLEventPumps::instance().obtain("lua output") };
    const auto& [registry, lookup]{ LuaFunction::getRState() };
    if (! lua_gettop(L))
    {
        // no arguments passed: list all lua_functions
        for (const auto& [name, pair] : registry)
        {
            const auto& [fptr, helptext] = pair;
            luapump.post("LL." + helptext);
        }
    }
    else
    {
        // arguments passed: list each of the specified lua_functions
        for (int idx = 1, top = lua_gettop(L); idx <= top; ++idx)
        {
            std::string arg{ stringize("<unknown ", lua_typename(L, lua_type(L, idx)), ">") };
            if (lua_type(L, idx) == LUA_TSTRING)
            {
                arg = lua_tostdstring(L, idx);
                LLStringUtil::removePrefix(arg, "LL.");
            }
            else if (lua_type(L, idx) == LUA_TFUNCTION)
            {
                // Caller passed the actual function instead of its string
                // name. A Lua function is an anonymous callable object; it
                // has a name only by assigment. You can't ask Lua for a
                // function's name, which is why our constructor maintains a
                // reverse Lookup map.
                auto function{ lua_tocfunction(L, idx) };
                if (auto found = lookup.find(function); found != lookup.end())
                {
                    // okay, pass found name to lookup below
                    arg = found->second;
                }
            }

            if (auto found = registry.find(arg); found != registry.end())
            {
                luapump.post("LL." + found->second.second);
            }
            else
            {
                luapump.post(arg + ": NOT FOUND");
            }
        }
        // pop all arguments
        lua_settop(L, 0);
    }
    return 0;                       // void return
}

/*****************************************************************************
*   leaphelp()
*****************************************************************************/
lua_function(
    leaphelp,
    "leaphelp(): list viewer's LEAP APIs\n"
    "LL.leaphelp(api): show help for specific api string name")
{
    LLSD request;
    int top{ lua_gettop(L) };
    if (top)
    {
        request = llsd::map("op", "getAPI", "api", lua_tostdstring(L, 1));
    }
    else
    {
        request = llsd::map("op", "getAPIs");
    }
    // pop all args
    lua_settop(L, 0);

    auto& outpump{ LLEventPumps::instance().obtain("lua output") };
    auto& listener{ LuaState::obtainListener(L) };
    LLEventStream replyPump("leaphelp", true);
    // ask the LuaListener's LeapListener and suspend calling coroutine until reply
    auto reply{ llcoro::postAndSuspend(request, listener.getCommandName(), replyPump, "reply") };
    reply.erase("reqid");

    if (auto error = reply["error"]; error.isString())
    {
        outpump.post(error.asString());
        return 0;
    }

    if (top)
    {
        // caller wants a specific API
        outpump.post(stringize(reply["name"].asString(), ":\n", reply["desc"].asString()));
        for (const auto& opmap : llsd::inArray(reply["ops"]))
        {
            std::ostringstream reqstr;
            auto req{ opmap["required"] };
            if (req.isArray())
            {
                const char* sep = " (requires ";
                for (const auto& [reqkey, reqval] : llsd::inMap(req))
                {
                    reqstr << sep << reqkey;
                    sep = ", ";
                }
                reqstr << ")";
            }
            outpump.post(stringize("---- ", reply["key"].asString(), " == '",
                                   opmap["name"].asString(), "'", reqstr.str(), ":\n",
                                   opmap["desc"].asString()));
        }
    }
    else
    {
        // caller wants a list of APIs
        for (const auto& [name, data] : llsd::inMap(reply))
        {
            outpump.post(stringize("==== ", name, ":\n", data["desc"].asString()));
        }
    }
    return 0;                       // void return
}

/*****************************************************************************
*   setdtor
*****************************************************************************/
namespace {

// proxy userdata object returned by setdtor()
struct setdtor_refs
{
    lua_State* L;
    std::string desc;
    // You can't directly store a Lua object in a C++ object, but you can
    // create a Lua "reference" by storing the object in the Lua Registry and
    // capturing its Registry index.
    int objref;
    int dtorref;

    setdtor_refs(lua_State* L, const std::string& desc, int objref, int dtorref):
        L(L),
        desc(desc),
        objref(objref),
        dtorref(dtorref)
    {}
    setdtor_refs(const setdtor_refs&) = delete;
    setdtor_refs& operator=(const setdtor_refs&) = delete;
    ~setdtor_refs();

    static void push_metatable(lua_State* L);
    static std::string binop(const std::string& name, const std::string& op);
    static int meta__index(lua_State* L);
};

} // anonymous namespace

lua_function(
    setdtor,
    "setdtor(desc, obj, dtorfunc) => proxy object referencing obj and dtorfunc.\n"
    "When the returned proxy object is garbage-collected, or when the script\n"
    "ends, call dtorfunc(obj). String desc is logged in the error message, if any.\n"
    "Use the returned proxy object (or proxy._target) like obj.\n"
    "obj won't be destroyed as long as the proxy exists; it's the proxy object's\n"
    "lifespan that determines when dtorfunc(obj) will be called.")
{
    if (lua_gettop(L) != 3)
    {
        return lluau::error(L, "setdtor(desc, obj, dtor) requires exactly 3 arguments");
    }
    // called with (desc, obj, dtor), returns proxy object
    lua_checkdelta(L, -2);
//  lluau_checkstack(L, 0);         // might get up to 3 stack entries
    auto desc{ lua_tostdstring(L, 1) };
    // Get Lua "references" for each of the object and the dtor function.
    int objref = lua_ref(L, 2);
    int dtorref = lua_ref(L, 3);
    // Having captured each of our parameters, discard them.
    lua_settop(L, 0);
    // Push our setdtor_refs userdata. Not only do we want to push it on L's
    // stack, but setdtor_refs's constructor itself requires L.
    lua_emplace<setdtor_refs>(L, L, desc, objref, dtorref);
    // stack: proxy (i.e. setdtor_refs userdata)
    // have to set its metatable
    lua_getfield(L, LUA_REGISTRYINDEX, "setdtor_meta");
    // stack: proxy, setdtor_meta (which might be nil)
    if (lua_isnil(L, -1))
    {
        // discard nil
        lua_pop(L, 1);
        // compile and push our forwarding metatable
        setdtor_refs::push_metatable(L);
        // stack: proxy, metatable
        // duplicate metatable to save it
        lua_pushvalue(L, -1);
        // stack: proxy, metatable, metable
        // save metatable for future calls
        lua_setfield(L, LUA_REGISTRYINDEX, "setdtor_meta");
        // stack: proxy, metatable
    }
    // stack: proxy, metatable
    lua_setmetatable(L, -2);
    // stack: proxy
    // Because ~setdtor_refs() necessarily uses the Lua stack, the Registry et
    // al., we can't let a setdtor_refs instance be destroyed by lua_close():
    // the Lua environment will already be partially shut down. To destroy
    // this new setdtor_refs instance BEFORE lua_close(), bind it with
    // lua_destroybounduserdata() and register it with LL.atexit().
    // push (the entry point for) LL.atexit()
    lua_pushcfunction(L, atexit_luasub::call, "LL.atexit()");
    // stack: proxy, atexit()
    lua_pushvalue(L, -2);
    // stack: proxy, atexit(), proxy
    int tag = lua_userdatatag(L, -1);
    // We don't have a lookup table to get from an int Lua userdata tag to the
    // corresponding C++ typeinfo name string. We'll introduce one if we need
    // it for debugging. But for this particular call, we happen to know it's
    // always a setdtor_refs object.
    lua_pushcclosure(L, lua_destroybounduserdata,
                     stringize("lua_destroybounduserdata<", tag, ">()").c_str(),
                     1);
    // stack: proxy, atexit(), lua_destroybounduserdata
    // call atexit(): one argument, no results, let error propagate
    lua_call(L, 1, 0);
    // stack: proxy
    return 1;
}

namespace {

void setdtor_refs::push_metatable(lua_State* L)
{
    lua_checkdelta(L, 1);
    lluau_checkstack(L, 1);
    // Ideally we want a metatable that forwards every operation on our
    // setdtor_refs userdata proxy object to the original object. But the
    // published C API doesn't include (e.g.) arithmetic operations on Lua
    // objects, so in fact it's easier to express the desired metatable in Lua
    // than in C++. We could make setdtor() depend on an external Lua module,
    // but it seems less fragile to embed the Lua source code right here.
    static const std::string setdtor_meta = stringize(R"-(
    -- This metatable literal doesn't define __index() because that's
    -- implemented in C++. We cannot, in Lua, peek into the setdtor_refs
    -- userdata object to obtain objref, nor can we fetch Registry[objref].
    -- So our C++ __index() metamethod recognizes access to '_target' as a
    -- reference to Registry[objref].
    -- The rest are defined per https://www.lua.org/manual/5.1/manual.html#2.8.
    -- Luau supports destructors instead of __gc metamethod -- we rely on that!
    -- We don't set __mode because our proxy is not a table. Real references
    -- are stored in the wrapped table, so ITS __mode is what counts.
    -- Initial definition of meta omits binary metamethods so they can bind the
    -- metatable itself, as explained for binop() below.
    local meta = {
        __unm = function(arg)
            return -arg._target
        end,
        __len = function(arg)
            return #arg._target
        end,
        -- Comparison metamethods __eq(), __lt() and __le() are only called
        -- when both operands have the same metamethod. For our purposes, that
        -- means both operands are setdtor_refs userdata objects.
        __eq = function(lhs, rhs)
            return (lhs._target == rhs._target)
        end,
        __lt = function(lhs, rhs)
            return (lhs._target < rhs._target)
        end,
        __le = function(lhs, rhs)
            return (lhs._target <= rhs._target)
        end,
        __newindex = function(t, key, value)
            assert(key ~= '_target',
                   "Don't try to replace a setdtor() proxy's _target")
            t._target[key] = value
        end,
        __call = function(func, ...)
            return func._target(...)
        end,
        __tostring = function(arg)
            -- don't fret about arg._target's __tostring metamethod,
            -- if any, because built-in tostring() deals with that
            return tostring(arg._target)
        end,
        __iter = function(arg)
            local iter = (getmetatable(arg._target) or {}).__iter
            if iter then
                return iter(arg._target)
            else
                return next, arg._target
            end
        end
    }
)-",
    binop("add", "+"),
    binop("sub", "-"),
    binop("mul", "*"),
    binop("div", "/"),
    binop("idiv", "//"),
    binop("mod", "%"),
    binop("pow", "^"),
    binop("concat", ".."),
R"-(
    return meta
)-");
    // only needed for debugging binop()
//  LL_DEBUGS("Lua") << setdtor_meta << LL_ENDL;

    if (lluau::dostring(L, LL_PRETTY_FUNCTION, setdtor_meta) != LUA_OK)
    {
        // stack: error message string
        lua_error(L);
    }
    llassert(lua_gettop(L) > 0);
    llassert(lua_type(L, -1) == LUA_TTABLE);
    // stack: Lua metatable compiled from setdtor_meta source
    // Inject our C++ __index metamethod.
    lua_rawsetfield(L, -1, "__index"sv, &setdtor_refs::meta__index);
}

// In the definition of setdtor_meta above, binary arithmethic and
// concatenation metamethods are a little funny in that we don't know a
// priori which operand is the userdata with our metatable: the metamethod
// can be invoked either way. So every such metamethod must check, which
// leads to lots of redundancy. Hence this helper function. Call it a Lua
// macro.
std::string setdtor_refs::binop(const std::string& name, const std::string& op)
{
    return stringize(
        "    meta.__", name, " = function(lhs, rhs)\n"
        "        if getmetatable(lhs) == meta then\n"
        "            return lhs._target ", op, " rhs\n"
        "        else\n"
        "            return lhs ", op, " rhs._target\n"
        "        end\n"
        "    end\n");
}

// setdtor_refs __index() metamethod
int setdtor_refs::meta__index(lua_State* L)
{
    // called with (setdtor_refs userdata, key), returns retrieved object
    lua_checkdelta(L, -1);
    lluau_checkstack(L, 2);
    // stack: proxy, key
    // get ptr to the C++ struct data
    auto ptr = lua_toclass<setdtor_refs>(L, -2);
    // meta__index() should NEVER be called with anything but setdtor_refs!
    llassert(ptr);
    // push the wrapped object
    lua_getref(L, ptr->objref);
    // stack: proxy, key, _target
    // replace userdata with _target
    lua_replace(L, -3);
    // stack: _target, key
    // Duplicate key because lua_tostring() converts number to string:
    // if the key is (e.g.) 1, don't try to retrieve _target["1"]!
    lua_pushvalue(L, -1);
    // stack: _target, key, key
    // recognize the special _target field
    if (lua_tostdstring(L, -1) == "_target")
    {
        // okay, ditch both copies of "_target" string key
        lua_pop(L, 2);
        // stack: _target
    }
    else                            // any key but _target
    {
        // ditch stringized key
        lua_pop(L, 1);
        // stack: _target, key
        // replace key with _target[key], invoking metamethod if any
        lua_gettable(L, -2);
        // stack: _target, _target[key]
        // discard _target
        lua_remove(L, -2);
        // stack: _target[key]
    }
    return 1;
}

// replacement for global next():
// its lua_upvalueindex(1) is the original function it's replacing
int lua_proxydrill(lua_State* L)
{
    // Accept however many arguments the original function normally accepts.
    // If our first arg is a userdata, check if it's a setdtor_refs proxy.
    // Drill through as many levels of proxy wrapper as needed.
    while (const setdtor_refs* ptr = lua_toclass<setdtor_refs>(L, 1))
    {
        // push original object
        lua_getref(L, ptr->objref);
        // replace first argument with that
        lua_replace(L, 1);
    }
    // We've reached a first argument that's not a setdtor() proxy.
    // How many arguments were we passed, anyway?
    int args = lua_gettop(L);
    // Push the original function, captured as our upvalue.
    lua_pushvalue(L, lua_upvalueindex(1));
    // Shift the stack so the original function is first.
    lua_insert(L, 1);
    // Call the original function with all original args, no error checking.
    // Don't truncate however many values that function returns.
    lua_call(L, args, LUA_MULTRET);
    // Return as many values as the original function returned.
    return lua_gettop(L);
}

// When Lua destroys a setdtor_refs userdata object, either from garbage
// collection or from LL.atexit(lua_destroybounduserdata), it's time to keep
// its promise to call the specified Lua destructor function with the
// specified Lua object. Of course we must also delete the captured
// "references" to both objects.
setdtor_refs::~setdtor_refs()
{
    lua_checkdelta(L);
    lluau_checkstack(L, 2);
    // push Registry[dtorref]
    lua_getref(L, dtorref);
    // push Registry[objref]
    lua_getref(L, objref);
    // free Registry[dtorref]
    lua_unref(L, dtorref);
    // free Registry[objref]
    lua_unref(L, objref);
    // call dtor(obj): one arg, no result, no error function
    int rc = lua_pcall(L, 1, 0, 0);
    if (rc != LUA_OK)
    {
        // TODO: we don't really want to propagate the error here.
        // If this setdtor_refs instance is being destroyed by
        // LL.atexit(), we want to continue cleanup. If it's being
        // garbage-collected, the call is completely unpredictable from
        // the consuming script's point of view. But what to do about this
        // error?? For now, just log it.
        LL_WARNS("Lua") << LLCoros::getName()
                        << ": setdtor(" << std::quoted(desc) << ") error: "
                        << lua_tostring(L, -1) << LL_ENDL;
        lua_pop(L, 1);
    }
}

} // anonymous namespace

/*****************************************************************************
*   lua_what
*****************************************************************************/
std::ostream& operator<<(std::ostream& out, const lua_what& self)
{
    switch (lua_type(self.L, self.index))
    {
    case LUA_TNONE:
        // distinguish acceptable but non-valid index
        out << "none";
        break;

    case LUA_TNIL:
        out << "nil";
        break;

    case LUA_TBOOLEAN:
    {
        auto oldflags { out.flags() };
        out << std::boolalpha << lua_toboolean(self.L, self.index);
        out.flags(oldflags);
        break;
    }

    case LUA_TNUMBER:
        out << lua_tonumber(self.L, self.index);
        break;

    case LUA_TSTRING:
        out << std::quoted(lua_tostdstring(self.L, self.index));
        break;

    case LUA_TUSERDATA:
    {
        const S32 maxlen = 20;
        S32 binlen{ lua_rawlen(self.L, self.index) };
        LLSD::Binary binary(std::min(maxlen, binlen));
        std::memcpy(binary.data(), lua_touserdata(self.L, self.index), binary.size());
        out << LL::hexdump(binary);
        if (binlen > maxlen)
        {
            out << "...(" << (binlen - maxlen) << " more)";
        }
        break;
    }

    case LUA_TLIGHTUSERDATA:
        out << lua_touserdata(self.L, self.index);
        break;

    case LUA_TFUNCTION:
    {
        // Try for the function's name, at the cost of a few more stack
        // entries.
        lua_checkdelta(self.L);
        lluau_checkstack(self.L, 3);
        lua_getglobal(self.L, "debug");
        // stack: ..., debug
        lua_getfield(self.L, -1, "info");
        // stack: ..., debug, debug.info
        lua_remove(self.L, -2);
        // stack: ..., debug.info
        lua_pushvalue(self.L, self.index);
        // stack: ..., debug.info, this function
        lua_pushstring(self.L, "n");
        // stack: ..., debug.info, this function, "n"
        // 2 arguments, 1 return value (or error message), no error handler
        lua_pcall(self.L, 2, 1, 0);
        // stack: ..., function name (or error) from debug.info()
        out << "function " << lua_tostdstring(self.L, -1);
        lua_pop(self.L, 1);
        // stack: ...
        break;
    }

    default:
        // anything else, don't bother trying to report value, just type
        out << lua_typename(self.L, lua_type(self.L, self.index));
        break;
    }
    return out;
}

/*****************************************************************************
*   lua_stack
*****************************************************************************/
std::ostream& operator<<(std::ostream& out, const lua_stack& self)
{
    out << "stack: [";
    const char* sep = "";
    for (int index = 1; index <= lua_gettop(self.L); ++index)
    {
        out << sep << lua_what(self.L, index);
        sep = ", ";
    }
    out << ']';
    return out;
}

/*****************************************************************************
*   LuaStackDelta
*****************************************************************************/
LuaStackDelta::LuaStackDelta(lua_State* L, const std::string& where, int delta):
    L(L),
    mWhere(where),
    mDepth(lua_gettop(L)),
    mDelta(delta)
{}

LuaStackDelta::~LuaStackDelta()
{
    auto depth{ lua_gettop(L) };
    // If we're unwinding the stack due to an exception, then of course we
    // can't expect the logic in the block containing this LuaStackDelta
    // instance to keep its contract wrt the Lua data stack.
    if (std::uncaught_exceptions() == 0 && mDepth + mDelta != depth)
    {
        LL_ERRS("Lua") << LLCoros::getName() << ": " << mWhere
                       << ": Lua stack went from " << mDepth << " to " << depth;
        if (mDelta)
        {
            LL_CONT << ", rather than expected " << (mDepth + mDelta) << " (" << mDelta << ")";
        }
        LL_ENDL;
    }
}
