/**
 * @file   classic_callback.h
 * @author Nat Goodspeed
 * @date   2016-06-21
 * @brief  ClassicCallback and HeapClassicCallback
 *
 * This header file addresses the problem of passing a method on a C++ object
 * to an API that requires a classic-C function pointer. Typically such a
 * callback API accepts a void* pointer along with the function pointer, and
 * the function pointer signature accepts a void* parameter. The API passes
 * the caller's pointer value into the callback function so it can find its
 * data. In C++, there are a few ways to deal with this case:
 *
 * - Use a static method with correct signature. If you don't need access to a
 *   specific instance, that works fine.
 * - Store the object statically (or store a static pointer to a non-static
 *   instance). As long as you only care about one instance, that works, but
 *   starts to get a little icky. As soon as there's more than one pertinent
 *   instance, fight valiantly against the temptation to stuff the instance
 *   pointer into a static pointer variable "just for a moment."
 * - Code a static trampoline callback function that accepts the void* user
 *   data pointer, casts it to the appropriate class type and calls the actual
 *   method on that class.
 *
 * ClassicCallback encapsulates the last. You need only construct a
 * ClassicCallback instance somewhere that will survive until the callback is
 * called, binding the target C++ callable. You then call its get_callback()
 * and get_userdata() methods to pass an appropriate classic-C function
 * pointer and void* user data pointer, respectively, to the old-style
 * callback API. get_callback() synthesizes a static trampoline function
 * that casts the user data pointer and calls the bound C++ callable.
 *
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 * Copyright (c) 2016, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_CLASSIC_CALLBACK_H)
#define LL_CLASSIC_CALLBACK_H

#include <tuple>
#include <type_traits>              // std::is_same

/*****************************************************************************
*   Helpers
*****************************************************************************/

// find a type in a parameter pack: http://stackoverflow.com/q/17844867/5533635
// usage: index_of<0, sought_t, PackName...>::value
template <int idx, typename sought, typename candidate, typename ...rest>
struct index_of
{
  static constexpr int const value =
      std::is_same<sought, candidate>::value ?
          idx : index_of<idx + 1, sought, rest...>::value;
};

// recursion tail
template <int idx, typename sought, typename candidate>
struct index_of<idx, sought, candidate>
{
  static constexpr int const value =
      std::is_same<sought, candidate>::value ? idx : -1;
};

/*****************************************************************************
*   ClassicCallback
*****************************************************************************/
/**
 * Instantiate ClassicCallback in whatever storage will persist long enough
 * for the callback to be called. It holds a modern C++ callable, providing a
 * static function pointer and a USERDATA (default void*) capable of being
 * passed through a classic-C callback API. When the static function is called
 * with that USERDATA pointer, ClassicCallback forwards the call to the bound
 * C++ callable.
 *
 * Usage:
 * @code
 * // callback signature required by the API of interest
 * typedef void (*callback_t)(int, const char*, void*, double);
 * // old-style API that accepts a classic-C callback function pointer
 * void oldAPI(callback_t callback, void* userdata);
 * // but I want to pass a lambda that references data local to my function!
 * // (We don't need to name the void* parameter in the C++ callable;
 * // ClassicCallback already used it to locate the lambda instance.)
 * auto ccb{
 *     makeClassicCallback<callback_t>(
 *         [=](int n, const char* s, void*, double f){ ... }) };
 * oldAPI(ccb.get_callback(), ccb.get_userdata());
 * // If the passed callback is called before oldAPI() returns, we can now
 * // safely destroy ccb. If the callback might be called later, consider
 * // HeapClassicCallback instead.
 * @endcode
 *
 * If you have a callable object in hand, and you want to pass that to
 * ClassicCallback, you may either consume it by passing std::move(object), or
 * explicitly specify a reference to that object type as the CALLABLE template
 * parameter:
 * @code
 * CallableObject obj;
 * ClassicCallback<callback_t, void*, CallableObject&> ccb{obj};
 * @endcode
 */
// CALLABLE should either be deduced, e.g. by makeClassicCallback(), or
// specified explicitly. Its default type is meaningless, coded only so we can
// provide a useful default for USERDATA.
template <typename SIGNATURE, typename USERDATA=void*, typename CALLABLE=void(*)()>
class ClassicCallback
{
    typedef ClassicCallback<SIGNATURE, USERDATA, CALLABLE> self_t;

public:
    /// ClassicCallback binds any modern C++ callable.
    ClassicCallback(CALLABLE&& callable):
        mCallable(std::forward<CALLABLE>(callable))
    {}

    /**
     * ClassicCallback must not itself be copied or moved! Once you've passed
     * get_userdata() to some API, this object MUST remain at that address.
     */
    // However, makeClassicCallback() is useful for deducing the CALLABLE
    // type, which means we MUST be able to return one to construct into
    // caller's instance (move ctor). Possible defense: bool 'referenced' data
    // member set by get_userdata(), with an llassert_always(! referenced)
    // check in the move constructor.
    ClassicCallback(ClassicCallback const&) = delete;
    ClassicCallback(ClassicCallback&&) = default; // delete;
    ClassicCallback& operator=(ClassicCallback const&) = delete;
    ClassicCallback& operator=(ClassicCallback&&) = delete;

    /// Call get_callback() to get the necessary function pointer.
    SIGNATURE get_callback() const
    {
        // This declaration is where the compiler instantiates the correct
        // signature for the call() function template.
        SIGNATURE callback = call;
        return callback;
    }

    /// Call get_userdata() to get the opaque USERDATA pointer to pass
    /// through the classic-C callback API.
    USERDATA get_userdata() const
    {
        // The USERDATA userdata is of course a pointer to this object.
        return static_cast<USERDATA>(const_cast<self_t*>(this));
    }

protected:
    /**
     * This call() method accepts one or more callback arguments. It assumes
     * the first USERDATA parameter is the userdata.
     */
    // Note that we're not literally using C++ perfect forwarding here -- it
    // doesn't work to specify (Args&&... args). But that's okay because we're
    // dealing with a classic-C callback! It's not going to pass any move-only
    // types.
    template <typename... Args>
    static auto call(Args... args)
    {
        auto userdata = extract_userdata(std::forward<Args>(args)...);
        // cast the userdata param to 'this' and call mCallable
        return static_cast<self_t*>(userdata)->
            mCallable(std::forward<Args>(args)...);
    }

    template <typename... Args>
    static USERDATA extract_userdata(Args... args)
    {
        // Search for the first USERDATA parameter type, then extract that pointer.
        // extract value from parameter pack: http://stackoverflow.com/a/24710433/5533635
        return std::get<index_of<0, USERDATA, Args...>::value>(std::forward_as_tuple(args...));
    }

    CALLABLE mCallable;
};

/**
 * Usage:
 * @code
 * auto ccb{ makeClassicCallback<classic_callback_signature>(actual_callback) };
 * @endcode
 */
template <typename SIGNATURE, typename USERDATA=void*, typename CALLABLE=void(*)()>
auto makeClassicCallback(CALLABLE&& callable)
{
    return std::move(ClassicCallback<SIGNATURE, USERDATA, CALLABLE>
                     (std::forward<CALLABLE>(callable)));
}

/*****************************************************************************
*   HeapClassicCallback
*****************************************************************************/
/**
 * HeapClassicCallback is like ClassicCallback, with this exception: it MUST
 * be allocated on the heap because, once the callback has been called, it
 * deletes itself. This addresses the problem of a callback whose lifespan
 * must persist beyond the scope in which the callback API is engaged -- but
 * naturally this callback must be called exactly ONCE.
 *
 * Usage:
 * @code
 * // callback signature required by the API of interest
 * typedef void (*callback_t)(int, const char*, void*, double);
 * // here's the old-style API
 * void oldAPI(callback_t callback, void* userdata);
 * // want to call someObjPtr->method() when oldAPI() fires the callback,
 * // sometime in the future after the enclosing function has returned
 * auto ccb{
 *     makeHeapClassicCallback<callback_t>(
 *         [someObjPtr](int n, const char* s, void*, double f)
 *         { someObjPtr->method(); }) };
 * oldAPI(ccb.get_callback(), ccb.get_userdata());
 * // We don't need a smart pointer for ccb, because it will be deleted once
 * // oldAPI() calls the bound lambda. HeapClassicCallback is for when the
 * // callback will be called exactly once. If the classic API might call the
 * // passed callback more than once -- or might never call it at all --
 * // manually construct a ClassicCallback on the heap and manage its lifespan
 * // explicitly.
 * @endcode
 */
template <typename SIGNATURE, typename USERDATA=void*, typename CALLABLE=void(*)()>
class HeapClassicCallback: public ClassicCallback<SIGNATURE, USERDATA, CALLABLE>
{
    typedef ClassicCallback<SIGNATURE, USERDATA, CALLABLE> super;
    typedef HeapClassicCallback<SIGNATURE, USERDATA, CALLABLE> self_t;

    // This destructor is intentionally private to prevent allocation anywhere
    // but the heap. (The Design and Evolution of C++, section 11.4.2: Control
    // of Allocation)
    ~HeapClassicCallback() {}

public:
    HeapClassicCallback(CALLABLE&& callable):
        super(std::forward<CALLABLE>(callable))
    {}

    // makeHeapClassicCallback() only needs to return a pointer -- not an
    // instance -- so we can lock down our move constructor too.
    HeapClassicCallback(HeapClassicCallback&&) = delete;

    /// Replicate get_callback() from the base class because we must
    /// instantiate OUR call() function template.
    SIGNATURE get_callback() const
    {
        // This declaration is where the compiler instantiates the correct
        // signature for the call() function template.
        SIGNATURE callback = call;
        return callback;
    }

    /// Replicate get_userdata() from the base class because our call()
    /// method must be able to reconstitute a pointer to this subclass.
    USERDATA get_userdata() const
    {
        // The USERDATA userdata is of course a pointer to this object.
        return static_cast<const USERDATA>(const_cast<self_t*>(this));
    }

private:
    // call() uses a helper class to delete the HeapClassicCallback when done,
    // for two reasons. Most importantly, this deletes even if the callback
    // throws an exception. But also, call() must directly return the callback
    // result for return-type deduction.
    struct Destroyer
    {
        Destroyer(self_t* p): mPtr(p) {}
        ~Destroyer() { delete mPtr; }

        self_t* mPtr;
    };

    template <typename... Args>
    static auto call(Args... args)
    {
        // extract userdata at this level too
        USERDATA userdata = super::extract_userdata(std::forward<Args>(args)...);
        // arrange to delete it when we leave by whatever means
        Destroyer destroy(static_cast<self_t*>(userdata));

        return super::call(std::forward<Args>(args)...);
    }
};

template <typename SIGNATURE, typename USERDATA=void*, typename CALLABLE=void(*)()>
auto makeHeapClassicCallback(CALLABLE&& callable)
{
    return new HeapClassicCallback<SIGNATURE, USERDATA, CALLABLE>
        (std::forward<CALLABLE>(callable));
}

#endif /* ! defined(LL_CLASSIC_CALLBACK_H) */
