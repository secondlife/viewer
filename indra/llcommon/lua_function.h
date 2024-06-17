/**
 * @file   lua_function.h
 * @author Nat Goodspeed
 * @date   2024-02-05
 * @brief  Definitions useful for coding a new Luau entry point into C++
 * 
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LUA_FUNCTION_H)
#define LL_LUA_FUNCTION_H

#include "luau/luacode.h"
#include "luau/lua.h"
#include "luau/luaconf.h"
#include "luau/lualib.h"
#include "fsyspath.h"
#include "llerror.h"
#include "stringize.h"
#include <exception>                // std::uncaught_exceptions()
#include <memory>                   // std::shared_ptr
#include <optional>
#include <typeinfo>
#include <utility>                  // std::pair

class LuaListener;

/*****************************************************************************
*   lluau namespace utility functions
*****************************************************************************/
namespace lluau
{
    // luau defines luaL_error() as void, but we want to use the Lua idiom of
    // 'return error(...)'. Wrap luaL_error() in an int function.
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#endif // __clang__
    template<typename... Args>
    int error(lua_State* L, const char* format, Args&&... args)
    {
        luaL_error(L, format, std::forward<Args>(args)...);
#ifndef LL_MSVC
        return 0;
#endif
    }
#if __clang__
#pragma clang diagnostic pop
#endif // __clang__

    // luau removed lua_dostring(), but since we perform the equivalent luau
    // sequence in multiple places, encapsulate it. desc and text are strings
    // rather than string_views because dostring() needs pointers to nul-
    // terminated char arrays.
    int dostring(lua_State* L, const std::string& desc, const std::string& text);
    int loadstring(lua_State* L, const std::string& desc, const std::string& text);

    fsyspath source_path(lua_State* L);

    void set_interrupts_counter(lua_State *L, S32 counter);
    void check_interrupts_counter(lua_State* L);
} // namespace lluau

std::string lua_tostdstring(lua_State* L, int index);
void lua_pushstdstring(lua_State* L, const std::string& str);
LLSD lua_tollsd(lua_State* L, int index);
void lua_pushllsd(lua_State* L, const LLSD& data);

/*****************************************************************************
*   LuaState
*****************************************************************************/
/**
 * RAII class to manage the lifespan of a lua_State
 */
class LuaState
{
public:
    typedef std::function<void(std::string msg)> script_finished_fn;

    LuaState(script_finished_fn cb={});

    LuaState(const LuaState&) = delete;
    LuaState& operator=(const LuaState&) = delete;

    ~LuaState();

    void initLuaState();

    bool checkLua(const std::string& desc, int r);

    // expr() is for when we want to capture any results left on the stack
    // by a Lua expression, possibly including multiple return values.
    // int <  0 means error, and LLSD::asString() is the error message.
    // int == 0 with LLSD::isUndefined() means the Lua expression returned no
    //          results.
    // int == 1 means the Lua expression returned one result.
    // int >  1 with LLSD::isArray() means the Lua expression returned
    //          multiple results, represented as the entries of the array.
    std::pair<int, LLSD> expr(const std::string& desc, const std::string& text);

    operator lua_State*() const { return mState; }

    // Find or create LuaListener for this LuaState.
    LuaListener& obtainListener() { return obtainListener(mState); }
    // Find or create LuaListener for passed lua_State.
    static LuaListener& obtainListener(lua_State* L);

private:
    script_finished_fn mCallback;
    lua_State* mState;
    std::string mError;
};

/*****************************************************************************
*   LuaPopper
*****************************************************************************/
/**
 * LuaPopper is an RAII struct whose role is to pop some number of entries
 * from the Lua stack if the calling function exits early.
 */
struct LuaPopper
{
    LuaPopper(lua_State* L, int count):
        mState(L),
        mCount(count)
    {}

    LuaPopper(const LuaPopper&) = delete;
    LuaPopper& operator=(const LuaPopper&) = delete;

    ~LuaPopper();

    void disarm() { set(0); }
    void set(int count) { mCount = count; }

    lua_State* mState;
    int mCount;
};

/*****************************************************************************
*   lua_function (and helper class LuaFunction)
*****************************************************************************/
/**
 * LuaFunction is a base class containing a static registry of its static
 * subclass call() methods. call() is NOT virtual: instead, each subclass
 * constructor passes a pointer to its distinct call() method to the base-
 * class constructor, along with a name by which to register that method.
 *
 * The init() method walks the registry and registers each such name with the
 * passed lua_State.
 */
class LuaFunction
{
public:
    LuaFunction(const std::string_view& name, lua_CFunction function,
                const std::string_view& helptext);

    static void init(lua_State* L);

    static lua_CFunction get(const std::string& key);

protected:
    using Registry = std::map<std::string, std::pair<lua_CFunction, std::string>>;
    using Lookup = std::map<lua_CFunction, std::string>;
    static std::pair<const Registry&, const Lookup&> getRState() { return getState(); }

private:
    static std::pair<Registry&, Lookup&> getState();
};

/**
 * lua_function(name, helptext) is a macro to facilitate defining C++ functions
 * available to Lua. It defines a subclass of LuaFunction and declares a
 * static instance of that subclass, thereby forcing the compiler to call its
 * constructor at module initialization time. The constructor passes the
 * stringized instance name to its LuaFunction base-class constructor, along
 * with a pointer to the static subclass call() method. It then emits the
 * call() method definition header, to be followed by a method body enclosed
 * in curly braces as usual.
 */
#define lua_function(name, helptext)                        \
static struct name##_luasub : public LuaFunction            \
{                                                           \
    name##_luasub(): LuaFunction(#name, &call, helptext) {} \
    static int call(lua_State* L);                          \
} name##_lua;                                               \
int name##_luasub::call(lua_State* L)
// {
//     ... supply method body here, referencing 'L' ...
// }

/*****************************************************************************
*   lua_emplace<T>(), lua_toclass<T>()
*****************************************************************************/
namespace {

// this closure function retrieves its bound argument to pass to
// lua_emplace_gc<T>()
template <class T>
int lua_emplace_call_gc(lua_State* L);
// this will be the function called by the new userdata's metatable's __gc()
template <class T>
int lua_emplace_gc(lua_State* L);
// name by which we'll store the new userdata's metatable in the Registry
template <class T>
std::string lua_emplace_metaname(const std::string& Tname = LLError::Log::classname<T>());

} // anonymous namespace

/**
 * On the stack belonging to the passed lua_State, push a Lua userdata object
 * with a newly-constructed C++ object std::optional<T>(args...). The new
 * userdata has a metadata table with a __gc() function to ensure that when
 * the userdata instance is garbage-collected, ~T() is called.
 *
 * We wrap the userdata object as std::optional<T> so we can explicitly
 * destroy the contained T, and detect that we've done so.
 *
 * Usage:
 * lua_emplace<T>(L, T constructor args...);
 */
template <class T, typename... ARGS>
void lua_emplace(lua_State* L, ARGS&&... args)
{
    using optT = std::optional<T>;
    luaL_checkstack(L, 3, nullptr);
    auto ptr = lua_newuserdata(L, sizeof(optT));
    // stack is uninitialized userdata
    // For now, assume (but verify) that lua_newuserdata() returns a
    // conservatively-aligned ptr. If that turns out not to be the case, we
    // might have to discard the new userdata, overallocate its successor and
    // perform manual alignment -- but only if we must.
    llassert((uintptr_t(ptr) % alignof(optT)) == 0);
    // Construct our T there using placement new
    new (ptr) optT(std::in_place, std::forward<ARGS>(args)...);
    // stack is now initialized userdata containing our T instance

    // Find or create the metatable shared by all userdata instances holding
    // C++ type T. We want it to be shared across instances, but it must be
    // type-specific because its __gc field is lua_emplace_gc<T>.
    auto Tname{ LLError::Log::classname<T>() };
    auto metaname{ lua_emplace_metaname<T>(Tname) };
    if (luaL_newmetatable(L, metaname.c_str()))
    {
        // just created it: populate it
        auto gcname{ stringize("lua_emplace_gc<", Tname, ">") };
        lua_pushcfunction(L, lua_emplace_gc<T>, gcname.c_str());
        // stack is userdata, metatable, lua_emplace_gc<T>
        lua_setfield(L, -2, "__gc");
    }
    // stack is userdata, metatable
    lua_setmetatable(L, -2);
    // Stack is now userdata, initialized with T(args),
    // with metatable.__gc pointing to lua_emplace_gc<T>.
    // But wait, there's more! Use our atexit() function to ensure that this
    // C++ object is eventually cleaned up even if the garbage collector never
    // gets around to it.
    lua_getglobal(L, "LL");
    // stack contains userdata, LL
    lua_getfield(L, -1, "atexit");
    // stack contains userdata, LL, LL.atexit
    // duplicate userdata
    lua_pushvalue(L, -3);
    // stack contains userdata, LL, LL.atexit, userdata
    // push a closure binding (lua_emplace_call_gc<T>, userdata)
    auto callgcname{ stringize("lua_emplace_call_gc<", Tname, ">") };
    lua_pushcclosure(L, lua_emplace_call_gc<T>, callgcname.c_str(), 1);
    // stack contains userdata, LL, LL.atexit, closure
    // Call LL.atexit(closure)
    lua_call(L, 1, 0);
    // stack contains userdata, LL
    lua_pop(L, 1);
    // stack contains userdata -- return that
}

namespace {

// passed to LL.atexit(closure(lua_emplace_call_gc<T>, userdata));
// retrieves bound userdata to pass to lua_emplace_gc<T>()
template <class T>
int lua_emplace_call_gc(lua_State* L)
{
    luaL_checkstack(L, 1, nullptr);
    // retrieve the first (only) bound upvalue and push to stack top as the
    // argument for lua_emplace_gc<T>()
    lua_pushvalue(L, lua_upvalueindex(1));
    return lua_emplace_gc<T>(L);
}

// set as metatable(userdata).__gc to be called by the garbage collector
template <class T>
int lua_emplace_gc(lua_State* L)
{
    using optT = std::optional<T>;
    // We're called with userdata on the stack holding an instance of type T.
    auto ptr = lua_touserdata(L, -1);
    llassert(ptr);
    // Destroy the T object contained in optT at the void* address ptr. If
    // in future lua_emplace() must manually align our optT* within the
    // Lua-provided void*, derive optT* from ptr.
    static_cast<optT*>(ptr)->reset();
    // pop the userdata
    lua_pop(L, 1);
    return 0;
}

template <class T>
std::string lua_emplace_metaname(const std::string& Tname)
{
    return stringize("lua_emplace_", Tname, "_meta");
}

} // anonymous namespace

/**
 * If the value at the passed acceptable index is a full userdata created by
 * lua_emplace<T>() -- that is, the userdata contains a non-empty
 * std::optional<T> -- return a pointer to the contained T instance. Otherwise
 * (index is not a full userdata; userdata is not of type std::optional<T>;
 * std::optional<T> is empty) return nullptr.
 */
template <class T>
T* lua_toclass(lua_State* L, int index)
{
    using optT = std::optional<T>;
    luaL_checkstack(L, 2, nullptr);
    // get void* pointer to userdata (if that's what it is)
    auto ptr{ lua_touserdata(L, index) };
    if (! ptr)
        return nullptr;
    // push the metatable for this userdata, if any
    if (! lua_getmetatable(L, index))
        return nullptr;
    // now push the metatable created by lua_emplace<T>()
    auto metaname{ lua_emplace_metaname<T>() };
    luaL_getmetatable(L, metaname.c_str());
    auto equal{ lua_equal(L, -1, -2) };
    // Having compared the userdata's metatable with the one set by
    // lua_emplace<T>(), we no longer need either metatable on the stack.
    lua_pop(L, 2);
    if (! equal)
        return nullptr;
    // Derive the optT* from ptr. If in future lua_emplace() must manually
    // align our optT* within the Lua-provided void*, adjust accordingly.
    optT* tptr(static_cast<optT*>(ptr));
    // make sure our optT isn't empty
    if (! *tptr)
        return nullptr;
    // looks like we still have a non-empty optT: return the *address* of the
    // value() reference
    return &tptr->value();
}

/*****************************************************************************
*   lua_what()
*****************************************************************************/
// Usage:  std::cout << lua_what(L, stackindex) << ...;
// Reports on the Lua value found at the passed stackindex.
// If cast to std::string, returns the corresponding string value.
class lua_what
{
public:
    lua_what(lua_State* state, int idx):
        L(state),
        index(idx)
    {}

    friend std::ostream& operator<<(std::ostream& out, const lua_what& self);

    operator std::string() const { return stringize(*this); }

private:
    lua_State* L;
    int index;
};

/*****************************************************************************
*   lua_stack()
*****************************************************************************/
// Usage:  std::cout << lua_stack(L) << ...;
// Reports on the contents of the Lua stack.
// If cast to std::string, returns the corresponding string value.
class lua_stack
{
public:
    lua_stack(lua_State* state):
        L(state)
    {}

    friend std::ostream& operator<<(std::ostream& out, const lua_stack& self);

    operator std::string() const { return stringize(*this); }

private:
    lua_State* L;
};

/*****************************************************************************
*   LuaLog
*****************************************************************************/
// adapted from indra/test/debug.h
// can't generalize Debug::operator() target because it's a variadic template
class LuaLog
{
public:
    template <typename... ARGS>
    LuaLog(lua_State* L, ARGS&&... args):
        L(L),
        mBlock(stringize(std::forward<ARGS>(args)...))
    {
        (*this)("entry ", lua_stack(L));
    }

    // non-copyable
    LuaLog(const LuaLog&) = delete;
    LuaLog& operator=(const LuaLog&) = delete;

    ~LuaLog()
    {
        auto exceptional{ std::uncaught_exceptions()? "exceptional " : "" };
        (*this)(exceptional, "exit ", lua_stack(L));
    }

    template <typename... ARGS>
    void operator()(ARGS&&... args)
    {
        LL_INFOS("Lua") << mBlock << ' ';
        stream_to(LL_CONT, std::forward<ARGS>(args)...);
        LL_ENDL;
    }

private:
    lua_State* L;
    const std::string mBlock;
};

#endif /* ! defined(LL_LUA_FUNCTION_H) */
