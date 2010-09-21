/**
 * @file   lldoubledispatch.h
 * @author Nat Goodspeed
 * @date   2008-11-11
 * @brief  function calls virtual on more than one parameter
 * 
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLDOUBLEDISPATCH_H)
#define LL_LLDOUBLEDISPATCH_H

#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>

/**
 * This class supports function calls which are virtual on the dynamic type of
 * more than one parameter. Specifically, we address a limited but useful
 * subset of that problem: function calls which accept two parameters, and
 * select which particular function to call depending on the dynamic type of
 * both.
 * 
 * Scott Meyers, in More Effective C++ (Item 31), talks about some of the perils
 * and pitfalls lurking down this pathway.  He discusses and dismisses the
 * straightforward approaches of using single-dispatch virtual functions twice,
 * and of using a family of single-dispatch virtual functions which each examine
 * RTTI for their other parameter.  He advocates using a registry in which you
 * look up the actual types of both parameters (he uses the classes' string names,
 * via typeid(param).name()) to obtain a pointer to a free (non-member) function
 * that will accept this pair of parameters.
 * 
 * He does point out that his approach doesn't handle inheritance.  If you have a
 * registry entry for SpaceShip, and you have in hand a MilitaryShip (subclass of
 * SpaceShip) and an Asteroid, you'd like to call the function appropriate for
 * SpaceShips and Asteroids -- but alas, his lookup fails because the class name
 * for your MilitaryShip subclass isn't in the registry.
 * 
 * This class extends his idea to build a registry whose entries can examine the
 * dynamic type of the parameter in a more flexible way -- using dynamic_cast<>
 * -- thereby supporting inheritance relationships.
 * 
 * Of course we must allow for the ambiguity this permits. We choose to use a
 * sequence container rather than a map, and require that the client code
 * specify the order in which dispatch-table entries should be searched. The
 * result resembles the semantics of the catch clauses for a try/catch block:
 * you must code catch clauses in decreasing order of specificity, because if
 * you catch ErrorBaseClass before you catch ErrorSubclass, then any
 * ErrorSubclass exceptions thrown by the protected code will always match
 * ErrorBaseClass, and you will never reach your catch(ErrorSubclass) clause.
 * 
 * So, in a similar way, if you have a specific routine to process
 * MilitaryShip and Asteroid, you'd better place that in the table @em before
 * your more general routine that processes SpaceShip and Asteroid, or else
 * the MilitaryShip variant will never be called.
 *
 * @todo This implementation doesn't attempt to deal with
 * <tt>const</tt>-correctness of arguments. Our container stores templated
 * objects into which the specific parameter types have been "frozen." But to
 * store all these in the same container, they are all instances of a base
 * class with a virtual invocation method. Naturally the base-class virtual
 * method definition cannot know the <tt>const</tt>-ness of the particular
 * types with which its template subclass is instantiated.
 *
 * One is tempted to suggest four different virtual methods, one for each
 * combination of @c const and non-<tt>const</tt> arguments. Then the client
 * will select the particular virtual method that best fits the
 * <tt>const</tt>-ness of the arguments in hand. The trouble with that idea is
 * that in order to instantiate the subclass instance, we must compile all
 * four of its virtual method overrides, which means we must be prepared to
 * pass all four combinations of @c const and non-<tt>const</tt> arguments to
 * the registered callable. That only works when the registered callable
 * accepts both parameters as @c const.
 *
 * Of course the virtual method overrides could use @c const_cast to force
 * them to compile correctly regardless of the <tt>const</tt>-ness of the
 * registered callable's parameter declarations. But if we're going to force
 * the issue with @c const_cast anyway, why bother with the four different
 * virtual methods? Why not just require canonical parameter
 * <tt>const</tt>-ness for any callables used with this mechanism?
 *
 * We therefore require that your callable accept both params as
 * non-<tt>const</tt>. (This is more general than @c const: you can perform @c
 * const operations on a non-<tt>const</tt> parameter, but you can't perform
 * non-<tt>const</tt> operations on a @c const parameter.)
 *
 * For ease of use, though, our invocation method accepts both params as @c
 * const. Again, you can pass a non-<tt>const</tt> object to a @c const param,
 * but not the other way around. We take care of the @c const_cast for you.
 */
// LLDoubleDispatch is a template because we have to assume that all parameter
// types are subclasses of some common base class -- but we don't have to
// impose that base class on client code.  Instead, we let IT tell US the
// common parameter base class.
template<class ReturnType, class ParamBaseType>
class LLDoubleDispatch
{
    typedef LLDoubleDispatch<ReturnType, ParamBaseType> self_type;

public:
    LLDoubleDispatch() {}

    /**
     * Call the first matching entry.  If there's no registered Functor
     * appropriate for this pair of parameter types, this call will do
     * @em nothing!  (If you want notification in this case, simply add a new
     * Functor for (ParamBaseType&, ParamBaseType&) at the end of the table.
     * The two base-class entries should match anything that isn't matched by
     * any more specific entry.)
     *
     * See note in class documentation about <tt>const</tt>-correctness.
     */
    inline
    ReturnType operator()(const ParamBaseType& param1, const ParamBaseType& param2) const;

    // Borrow a trick from Alexandrescu:  use a Type class to "wrap" a type
    // for purposes of passing the type itself into a template method.
    template<typename T>
    struct Type {};

    /**
     * Add a new Entry for a given @a Functor. As mentioned above, the order
     * in which you add these entries is very important.
     *
     * If you want symmetrical entries -- that is, if a B and an A can call
     * the same Functor as an A and a B -- then pass @c true for the last
     * parameter, and we'll add a (B, A) entry as well as an (A, B) entry. But
     * your @a Functor can still be written to expect exactly the pair of types
     * you've explicitly specified, because the Entry with the reversed params
     * will call a special thunk that swaps params before calling your @a
     * Functor.
     */
    template<typename Type1, typename Type2, class Functor>
    void add(const Type<Type1>& t1, const Type<Type2>& t2, Functor func, bool symmetrical=false)
    {
        insert(t1, t2, func);
        if (symmetrical)
        {
            // Use boost::bind() to construct a param-swapping thunk. Don't
            // forget to reverse the parameters too.
            insert(t2, t1, boost::bind(func, _2, _1));
        }
    }

    /**
     * Add a new Entry for a given @a Functor, explicitly passing instances of
     * the Functor's leaf param types to help us figure out where to insert.
     * Because it can use RTTI, this add() method isn't order-sensitive like
     * the other one.
     *
     * If you want symmetrical entries -- that is, if a B and an A can call
     * the same Functor as an A and a B -- then pass @c true for the last
     * parameter, and we'll add a (B, A) entry as well as an (A, B) entry. But
     * your @a Functor can still be written to expect exactly the pair of types
     * you've explicitly specified, because the Entry with the reversed params
     * will call a special thunk that swaps params before calling your @a
     * Functor.
     */
    template <typename Type1, typename Type2, class Functor>
    void add(const Type1& prototype1, const Type2& prototype2, Functor func, bool symmetrical=false)
    {
        // Because we expect our caller to pass leaf param types, we can just
        // perform an ordinary search to find the first matching iterator. If
        // we find an existing Entry that matches both params, either the
        // param types are the same, or the existing Entry uses the base class
        // for one or both params, and the new Entry must precede that. Assume
        // our client won't register two callables with exactly the SAME set
        // of types; in that case we'll insert the new one before any earlier
        // ones, meaning the last one registered will "win." Note that if
        // find() doesn't find any matching Entry, it will return end(),
        // meaning the new Entry will be last, which is fine.
        typename DispatchTable::iterator insertion = find(prototype1, prototype2);
        insert(Type<Type1>(), Type<Type2>(), func, insertion);
        if (symmetrical)
        {
            insert(Type<Type2>(), Type<Type1>(), boost::bind(func, _2, _1), insertion);
        }
    }

    /**
     * Add a new Entry for a given @a Functor, specifying the Functor's leaf
     * param types as explicit template arguments. This will instantiate
     * temporary objects of each of these types, which requires that they have
     * a lightweight default constructor.
     *
     * If you want symmetrical entries -- that is, if a B and an A can call
     * the same Functor as an A and a B -- then pass @c true for the last
     * parameter, and we'll add a (B, A) entry as well as an (A, B) entry. But
     * your @a Functor can still be written to expect exactly the pair of types
     * you've explicitly specified, because the Entry with the reversed params
     * will call a special thunk that swaps params before calling your @a
     * Functor.
     */
    template <typename Type1, typename Type2, class Functor>
    void add(Functor func, bool symmetrical=false)
    {
        // This is a convenience wrapper for the add() variant taking explicit
        // instances.
        add(Type1(), Type2(), func, symmetrical);
    }

private:
    /// This is the base class for each entry in our dispatch table.
    struct EntryBase
    {
        virtual ~EntryBase() {}
        virtual bool matches(const ParamBaseType& param1, const ParamBaseType& param2) const = 0;
        virtual ReturnType operator()(ParamBaseType& param1, ParamBaseType& param2) const = 0;
    };

    /// Here's the template subclass that actually creates each entry.
    template<typename Type1, typename Type2, class Functor>
    class Entry: public EntryBase
    {
    public:
        Entry(Functor func): mFunc(func) {}
        /// Is this entry appropriate for these arguments?
        virtual bool matches(const ParamBaseType& param1, const ParamBaseType& param2) const
        {
            return (dynamic_cast<const Type1*>(&param1) &&
                    dynamic_cast<const Type2*>(&param2));
        }
        /// invocation
        virtual ReturnType operator()(ParamBaseType& param1, ParamBaseType& param2) const
        {
            // We perform the downcast so callable can accept leaf param
            // types, instead of accepting ParamBaseType and downcasting
            // explicitly.
            return mFunc(dynamic_cast<Type1&>(param1), dynamic_cast<Type2&>(param2));
        }
    private:
        /// Bind whatever function or function object the instantiator passed.
        Functor mFunc;
    };

    /// shared_ptr manages Entry lifespan for us
    typedef boost::shared_ptr<EntryBase> EntryPtr;
    /// use a @c list to make it easy to insert
    typedef std::list<EntryPtr> DispatchTable;
    DispatchTable mDispatch;

    /// Look up the location of the first matching entry.
    typename DispatchTable::const_iterator find(const ParamBaseType& param1, const ParamBaseType& param2) const
    {
        // We assert that it's safe to call the non-const find() method on a
        // const LLDoubleDispatch instance. Cast away the const-ness of 'this'.
        return const_cast<self_type*>(this)->find(param1, param2);
    }

    /// Look up the location of the first matching entry.
    typename DispatchTable::iterator find(const ParamBaseType& param1, const ParamBaseType& param2)
    {
        return std::find_if(mDispatch.begin(), mDispatch.end(),
                            boost::bind(&EntryBase::matches, _1,
                                        boost::ref(param1), boost::ref(param2)));
    }

    /// Look up the first matching entry.
    EntryPtr lookup(const ParamBaseType& param1, const ParamBaseType& param2) const
    {
        typename DispatchTable::const_iterator found = find(param1, param2);            
        if (found != mDispatch.end())
        {
            // Dereferencing the list iterator gets us an EntryPtr
            return *found;
        }
        // not found
        return EntryPtr();
    }

    // Break out the actual insert operation so the public add() template
    // function can avoid calling itself recursively.  See add() comments.
    template<typename Type1, typename Type2, class Functor>
    void insert(const Type<Type1>& param1, const Type<Type2>& param2, Functor func)
    {
        insert(param1, param2, func, mDispatch.end());
    }

    // Break out the actual insert operation so the public add() template
    // function can avoid calling itself recursively.  See add() comments.
    template<typename Type1, typename Type2, class Functor>
    void insert(const Type<Type1>&, const Type<Type2>&, Functor func,
                typename DispatchTable::iterator where)
    {
        mDispatch.insert(where, EntryPtr(new Entry<Type1, Type2, Functor>(func)));
    }

    /// Don't implement the copy ctor.  Everyone will be happier if the
    /// LLDoubleDispatch object isn't copied.
    LLDoubleDispatch(const LLDoubleDispatch& src);
};

template <class ReturnType, class ParamBaseType>
ReturnType LLDoubleDispatch<ReturnType, ParamBaseType>::operator()(const ParamBaseType& param1,
                                                                   const ParamBaseType& param2) const
{
    EntryPtr found = lookup(param1, param2);
    if (found.get() == 0)
        return ReturnType();    // bogus return value

    // otherwise, call the Functor we found
    return (*found)(const_cast<ParamBaseType&>(param1), const_cast<ParamBaseType&>(param2));
}

#endif /* ! defined(LL_LLDOUBLEDISPATCH_H) */
