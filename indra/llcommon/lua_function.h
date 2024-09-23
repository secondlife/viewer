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
#include "llsd.h"
#include "scriptcommand.h"
#include "stringize.h"
#include <exception>                // std::uncaught_exceptions()
#include <memory>                   // std::shared_ptr
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>                  // std::pair
#include <vector>

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
    // terminated char arrays. Any args are pushed to the Lua stack before
    // calling the Lua chunk in text.
    int dostring(lua_State* L, const std::string& desc, const std::string& text,
                 const std::vector<std::string>& args={});
    int loadstring(lua_State* L, const std::string& desc, const std::string& text);

    fsyspath source_path(lua_State* L);
} // namespace lluau

// must be a macro because LL_PRETTY_FUNCTION is context-sensitive
#define lluau_checkstack(L, n) luaL_checkstack((L), (n), LL_PRETTY_FUNCTION)

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
    LuaState();

    LuaState(const LuaState&) = delete;
    LuaState& operator=(const LuaState&) = delete;

    ~LuaState();

    bool checkLua(const std::string& desc, int r);

    // expr() is for when we want to capture any results left on the stack
    // by a Lua expression, possibly including multiple return values.
    // Pass:
    // desc = description used for logging et al.
    // text = Lua chunk to execute, e.g. contents of a script file
    // args = arguments, if any, to pass to script file
    // Returns:
    // int <  0 means error, and LLSD::asString() is the error message.
    // int == 0 with LLSD::isUndefined() means the Lua expression returned no
    //          results.
    // int == 1 means the Lua expression returned one result.
    // int >  1 with LLSD::isArray() means the Lua expression returned
    //          multiple results, represented as the entries of the array.
    std::pair<int, LLSD> expr(const std::string& desc, const std::string& text,
                              const std::vector<std::string>& args={});

    operator lua_State*() const { return mState; }

    // Find or create LuaListener for this LuaState.
    LuaListener& obtainListener() { return obtainListener(mState); }
    // Find or create LuaListener for passed lua_State.
    static LuaListener& obtainListener(lua_State* L);

    // Given lua_State* L, return the LuaState object managing (the main Lua
    // thread for) L.
    static LuaState& getParent(lua_State* L);

    void set_interrupts_counter(S32 counter);
    void check_interrupts_counter();

private:
    /*---------------------------- feature flag ----------------------------*/
    bool mFeature{ false };
    /*---------------------------- feature flag ----------------------------*/
    lua_State* mState{ nullptr };
    std::string mError;
    S32 mInterrupts{ 0 };
};

/*****************************************************************************
*   LuaPopper
*****************************************************************************/
/**
 * LuaPopper is an RAII class whose role is to pop some number of entries
 * from the Lua stack if the calling function exits early.
 */
class LuaPopper
{
public:
    LuaPopper(lua_State* L, int count):
        mState(L),
        mCount(count)
    {}

    LuaPopper(const LuaPopper&) = delete;
    LuaPopper& operator=(const LuaPopper&) = delete;

    ~LuaPopper();

    void disarm() { set(0); }
    void set(int count) { mCount = count; }

private:
    lua_State* mState;
    int mCount;
};

/*****************************************************************************
*   LuaRemover
*****************************************************************************/
/**
 * Remove a particular stack index on exit from enclosing scope.
 * If you pass a negative index (meaning relative to the current stack top),
 * converts to an absolute index. The point of LuaRemover is to remove the
 * entry at the specified index regardless of subsequent pushes to the stack.
 */
class LuaRemover
{
public:
    LuaRemover(lua_State* L, int index):
        mState(L),
        mIndex(lua_absindex(L, index))
    {}
    LuaRemover(const LuaRemover&) = delete;
    LuaRemover& operator=(const LuaRemover&) = delete;
    ~LuaRemover()
    {
        // If we're unwinding the C++ stack due to an exception, don't mess
        // with the Lua stack!
        if (std::uncaught_exceptions() == 0)
            lua_remove(mState, mIndex);
    }

private:
    lua_State* mState;
    int mIndex;
};

/*****************************************************************************
*   LuaStackDelta
*****************************************************************************/
/**
 * Instantiate LuaStackDelta in a block to compare the Lua data stack depth on
 * entry (LuaStackDelta construction) and exit. Optionally, pass the expected
 * depth increment. (But be aware that LuaStackDelta cannot observe the effect
 * of a LuaPopper or LuaRemover declared previously in the same block.)
 */
class LuaStackDelta
{
public:
    LuaStackDelta(lua_State* L, const std::string& where, int delta=0);
    LuaStackDelta(const LuaStackDelta&) = delete;
    LuaStackDelta& operator=(const LuaStackDelta&) = delete;

    ~LuaStackDelta();

private:
    lua_State* L;
    std::string mWhere;
    int mDepth, mDelta;
};

#define lua_checkdelta(L, ...) LuaStackDelta delta(L, LL_PRETTY_FUNCTION, ##__VA_ARGS__)

/*****************************************************************************
*   lua_push() wrappers for generic code
*****************************************************************************/
inline
void lua_push(lua_State* L, bool b)
{
    lua_pushboolean(L, int(b));
}

inline
void lua_push(lua_State* L, lua_CFunction fn)
{
    lua_pushcfunction(L, fn, "");
}

inline
void lua_push(lua_State* L, lua_Integer n)
{
    lua_pushinteger(L, n);
}

inline
void lua_push(lua_State* L, void* p)
{
    lua_pushlightuserdata(L, p);
}

inline
void lua_push(lua_State* L, const LLSD& data)
{
    lua_pushllsd(L, data);
}

inline
void lua_push(lua_State* L, const char* s, size_t len)
{
    lua_pushlstring(L, s, len);
}

inline
void lua_push(lua_State* L)
{
    lua_pushnil(L);
}

inline
void lua_push(lua_State* L, lua_Number n)
{
    lua_pushnumber(L, n);
}

inline
void lua_push(lua_State* L, const std::string& s)
{
    lua_pushstdstring(L, s);
}

inline
void lua_push(lua_State* L, const char* s)
{
    lua_pushstring(L, s);
}

/*****************************************************************************
*   lua_to() wrappers for generic code
*****************************************************************************/
template <typename T>
auto lua_to(lua_State* L, int index);

template <>
inline
auto lua_to<bool>(lua_State* L, int index)
{
    return lua_toboolean(L, index);
}

template <>
inline
auto lua_to<lua_CFunction>(lua_State* L, int index)
{
    return lua_tocfunction(L, index);
}

template <>
inline
auto lua_to<lua_Integer>(lua_State* L, int index)
{
    return lua_tointeger(L, index);
}

template <>
inline
auto lua_to<LLSD>(lua_State* L, int index)
{
    return lua_tollsd(L, index);
}

template <>
inline
auto lua_to<lua_Number>(lua_State* L, int index)
{
    return lua_tonumber(L, index);
}

template <>
inline
auto lua_to<std::string>(lua_State* L, int index)
{
    return lua_tostdstring(L, index);
}

template <>
inline
auto lua_to<void*>(lua_State* L, int index)
{
    return lua_touserdata(L, index);
}

/*****************************************************************************
*   field operations
*****************************************************************************/
// return to C++, from table at index, the value of field k
template <typename T>
auto lua_getfieldv(lua_State* L, int index, const char* k)
{
    lua_checkdelta(L);
    lluau_checkstack(L, 1);
    lua_getfield(L, index, k);
    LuaPopper pop(L, 1);
    return lua_to<T>(L, -1);
}

// set in table at index, as field k, the specified C++ value
template <typename T>
auto lua_setfieldv(lua_State* L, int index, const char* k, const T& value)
{
    index = lua_absindex(L, index);
    lua_checkdelta(L);
    lluau_checkstack(L, 1);
    lua_push(L, value);
    lua_setfield(L, index, k);
}

// return to C++, from table at index, the value of field k (without metamethods)
template <typename T>
auto lua_rawgetfield(lua_State* L, int index, std::string_view k)
{
    index = lua_absindex(L, index);
    lua_checkdelta(L);
    lluau_checkstack(L, 1);
    lua_pushlstring(L, k.data(), k.length());
    lua_rawget(L, index);
    LuaPopper pop(L, 1);
    return lua_to<T>(L, -1);
}

// set in table at index, as field k, the specified C++ value (without metamethods)
template <typename T>
void lua_rawsetfield(lua_State* L, int index, std::string_view k, const T& value)
{
    index = lua_absindex(L, index);
    lua_checkdelta(L);
    lluau_checkstack(L, 2);
    lua_pushlstring(L, k.data(), k.length());
    lua_push(L, value);
    lua_rawset(L, index);
}

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
    LuaFunction(std::string_view name, lua_CFunction function,
                std::string_view helptext);

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
// Every instance of DistinctInt has a different int value, barring int
// wraparound.
class DistinctInt
{
public:
    DistinctInt(): mValue(++mValues) {}
    int get() const { return mValue; }
    operator int() const { return mValue; }
private:
    static int mValues;
    int mValue;
};

namespace {

template <typename T>
struct TypeTag
{
    // For   (std::is_same<T, U>), &TypeTag<T>::value == &TypeTag<U>::value.
    // For (! std::is_same<T, U>), &TypeTag<T>::value != &TypeTag<U>::value.
    // And every distinct instance of DistinctInt has a distinct value.
    // Therefore, TypeTag<T>::value is an int uniquely associated with each
    // distinct T.
    static DistinctInt value;
};

template <typename T>
DistinctInt TypeTag<T>::value;

} // anonymous namespace

/**
 * On the stack belonging to the passed lua_State, push a Lua userdata object
 * containing a newly-constructed C++ object T(args...). The userdata has a
 * Luau destructor guaranteeing that the new T instance is destroyed when the
 * userdata is garbage-collected, no later than when the LuaState is
 * destroyed. It may be destroyed explicitly by calling lua_destroyuserdata().
 *
 * Usage:
 * lua_emplace<T>(L, T constructor args...);
 * // L's Lua stack top is now a userdata containing T
 */
template <class T, typename... ARGS>
void lua_emplace(lua_State* L, ARGS&&... args)
{
    lua_checkdelta(L, 1);
    lluau_checkstack(L, 1);
    int tag{ TypeTag<T>::value };
    if (! lua_getuserdatadtor(L, tag))
    {
        // We haven't yet told THIS lua_State the destructor to use for this tag.
        lua_setuserdatadtor(
            L, tag,
            [](lua_State*, void* ptr)
            {
                // destroy the contained T instance
                static_cast<T*>(ptr)->~T();
            });
    }
    auto ptr = lua_newuserdatatagged(L, sizeof(T), tag);
    // stack is uninitialized userdata
    // For now, assume (but verify) that lua_newuserdata() returns a
    // conservatively-aligned ptr. If that turns out not to be the case, we
    // might have to discard the new userdata, overallocate its successor and
    // perform manual alignment -- but only if we must.
    llassert((uintptr_t(ptr) % alignof(T)) == 0);
    // Construct our T there using placement new
    new (ptr) T(std::forward<ARGS>(args)...);
    // stack is now initialized userdata containing our T instance -- return
    // that
}

/**
 * If the value at the passed acceptable index is a full userdata created by
 * lua_emplace<T>(), return a pointer to the contained T instance. Otherwise
 * (index is not a full userdata; userdata is not of type T) return nullptr.
 */
template <class T>
T* lua_toclass(lua_State* L, int index)
{
    lua_checkdelta(L);
    // get void* pointer to userdata (if that's what it is)
    void* ptr{ lua_touserdatatagged(L, index, TypeTag<T>::value) };
    // Derive the T* from ptr. If in future lua_emplace() must manually
    // align our T* within the Lua-provided void*, adjust accordingly.
    return static_cast<T*>(ptr);
}

/**
 * Call lua_destroyuserdata() with the doomed userdata on the stack top.
 * It must have been created by lua_emplace().
 */
int lua_destroyuserdata(lua_State* L);

/**
 * Call lua_pushcclosure(L, lua_destroybounduserdata, 1) with the target
 * userdata on the stack top. When the resulting C closure is called with no
 * arguments, the bound userdata is destroyed by lua_destroyuserdata().
 */
int lua_destroybounduserdata(lua_State *L);

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
        LL_DEBUGS("Lua") << mBlock << ' ';
        stream_to(LL_CONT, std::forward<ARGS>(args)...);
        LL_ENDL;
    }

private:
    lua_State* L;
    const std::string mBlock;
};

#endif /* ! defined(LL_LUA_FUNCTION_H) */
