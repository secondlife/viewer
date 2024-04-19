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
#include "stringize.h"
#include <exception>                // std::uncaught_exceptions()
#include <memory>                   // std::shared_ptr
#include <utility>                  // std::pair

class LuaListener;

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

    // Return LuaListener for this LuaState if we already have one, else empty
    // shared_ptr.
    std::shared_ptr<LuaListener> getListener() { return getListener(mState); }
    // Find or create LuaListener for this LuaState, returning its ptr_t.
    std::shared_ptr<LuaListener> obtainListener() { return obtainListener(mState); }
    // Return LuaListener for passed lua_State if we already have one, else
    // empty shared_ptr.
    static std::shared_ptr<LuaListener> getListener(lua_State* L);
    // Find or create LuaListener for passed lua_State, returning its ptr_t.
    static std::shared_ptr<LuaListener> obtainListener(lua_State* L);

private:
    script_finished_fn mCallback;
    lua_State* mState;
    std::string mError;
};

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
