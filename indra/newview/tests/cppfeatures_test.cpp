/**
 * @file cppfeatures_test
 * @author Vir
 * @date 2021-03
 * @brief cpp features
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2021, Linden Research, Inc.
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

// Tests related to newer C++ features, for verifying support across compilers and platforms

#include "linden_common.h"
#include "../test/lltut.h"

namespace tut
{

struct cpp_features_test {};
typedef test_group<cpp_features_test> cpp_features_test_t;
typedef cpp_features_test_t::object cpp_features_test_object_t;
tut::cpp_features_test_t tut_cpp_features_test("LLCPPFeatures");

// bracket initializers
// Can initialize containers or values using curly brackets
template<> template<>
void cpp_features_test_object_t::test<1>()
{
    S32 explicit_val{3};
    ensure(explicit_val==3);

    S32 default_val{};
    ensure(default_val==0);
    
    std::vector<S32> fibs{1,1,2,3,5};
    ensure(fibs[4]==5);
}

// auto
//
// https://en.cppreference.com/w/cpp/language/auto
// 
// Can use auto in place of a more complex type specification, if the compiler can infer the type
template<> template<>
void cpp_features_test_object_t::test<2>()
{
    std::vector<S32> numbers{3,6,9};

    // auto element
    auto& aval = numbers[1];
    ensure("auto element", aval==6);

    // auto iterator (non-const)
    auto it = numbers.rbegin();
    *it += 1;
    S32 val = *it;
    ensure("auto iterator", val==10);
}

// range for
//
// https://en.cppreference.com/w/cpp/language/range-for
//
// Can iterate over containers without explicit iterator
template<> template<>
void cpp_features_test_object_t::test<3>()
{

    // Traditional iterator for with container
    //
    // Problems:
    // * Have to create a new variable for the iterator, which is unrelated to the problem you're trying to solve.
    // * Redundant and somewhat fragile. Have to make sure begin() and end() are both from the right container.
    std::vector<S32> numbers{3,6,9};
    for (auto it = numbers.begin(); it != numbers.end(); ++it)
    {
        auto& n = *it;
        n *= 2;
    }
    ensure("iterator for vector", numbers[2]==18);

    // Range for with container
    //
    // Under the hood, this is doing the same thing as the traditional
    // for loop above. Still uses begin() and end() but you don't have
    // to access them directly.
    std::vector<S32> numbersb{3,6,9};
    for (auto& n: numbersb)
    {
        n *= 2;
    }
    ensure("range for vector", numbersb[2]==18);

    // Range for over a C-style array.
    //
    // This is handy because the language determines the range automatically.
    // Getting this right manually is a little trickier.
    S32 pows[] = {1,2,4,8,16};
    S32 sum{};
    for (const auto& v: pows)
    {
        sum += v;
    }
    ensure("for C-array", sum==31);
}

// override specifier
//
// https://en.cppreference.com/w/cpp/language/override
//
// Specify that a particular class function is an override of a virtual function.
// Benefits:
// * Makes code somewhat easier to read by showing intent.
// * Prevents mistakes where you think something is an override but it doesn't actually match the declaration in the parent class.
// Drawbacks:
// * Some compilers require that any class using override must use it consistently for all functions. 
//   This makes switching a class to use override a lot more work. 

class Foo
{
public:
    virtual bool is_happy() const = 0;
};

class Bar: public Foo
{
public:
    bool is_happy() const override { return true; } 
    // Override would fail: non-const declaration doesn't match parent 
    // bool is_happy() override { return true; } 
    // Override would fail: wrong name
    // bool is_happx() override { return true; } 
};

template<> template<>
void cpp_features_test_object_t::test<4>()
{
    Bar b;
    ensure("override", b.is_happy());
}

// final
//
// https://en.cppreference.com/w/cpp/language/final: "Specifies that a
// virtual function cannot be overridden in a derived class or that a
// class cannot be inherited from."

class Vehicle
{
public:
    virtual bool has_wheels() const = 0;
};

class WheeledVehicle: public Vehicle
{
public:
    virtual bool has_wheels() const final override { return true; }
};

class Bicycle: public WheeledVehicle
{
public:
    // Error: can't override final version in WheeledVehicle 
    // virtual bool has_wheels() override const { return true; }
};

template<> template<>
void cpp_features_test_object_t::test<5>()
{
    Bicycle bi;
    ensure("final", bi.has_wheels());
}

// deleted function declaration
//
// https://en.cppreference.com/w/cpp/language/function#Deleted_functions
//
// Typical case: copy constructor doesn't make sense for a particular class, so you want to make
// sure the no one tries to copy-construct an instance of the class, and that the
// compiler won't generate a copy constructor for  you automatically.
// Traditional fix is to declare a
// copy constructor but never implement it, giving you a link-time error if anyone tries to use it.
// Now you can explicitly declare a function to be deleted, which has at least two advantages over
// the old way:
// * Makes the intention clear
// * Creates an error sooner, at compile time

class DoNotCopy
{
public:
    DoNotCopy() {}
    DoNotCopy(const DoNotCopy& ref) = delete;
};

template<> template<>
void cpp_features_test_object_t::test<6>()
{
    DoNotCopy nc; // OK, default constructor
    //DoNotCopy nc2(nc); // No, can't copy
    //DoNotCopy nc3 = nc; // No, this also calls copy constructor (even though it looks like an assignment)
}

// defaulted function declaration
//
// https://en.cppreference.com/w/cpp/language/function#Function_definition
//
// What about the complementary case to the deleted function declaration, where you want a copy constructor
// and are happy with the default implementation the compiler will make (memberwise copy).
// Now you can explicitly declare that too.
// Usage: I guess it makes the intent clearer, but otherwise not obviously useful.
class DefaultCopyOK
{
public:
    DefaultCopyOK(): mVal(123) {}
    DefaultCopyOK(const DefaultCopyOK&) = default;
    S32 val() const { return mVal; }
private:
    S32 mVal;
};

template<> template<>
void cpp_features_test_object_t::test<7>()
{
    DefaultCopyOK d; // OK
    DefaultCopyOK d2(d); // OK
    DefaultCopyOK d3 = d; // OK
    ensure("default copy d", d.val()==123);
    ensure("default copy d2", d.val()==d2.val());
    ensure("default copy d3", d.val()==d3.val());
}

// initialize class members inline
//
// https://en.cppreference.com/w/cpp/language/data_members#Member_initialization
//
// Default class member values can be set where they are declared, using either brackets or =

// It is preferred to skip creating a constructor if all the work can be done by inline initialization:
// http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines.html#c45-dont-define-a-default-constructor-that-only-initializes-data-members-use-in-class-member-initializers-instead
//
class InitInline
{
public:
    S32 mFoo{10};
};

class InitInlineWithConstructor
{
public:
    // Here mFoo is not specified, so you will get the default value of 10.
    // mBar is specified, so 25 will override the default value.
    InitInlineWithConstructor():
        mBar(25)
    {}

    // Default values set using two different styles, same effect.
    S32 mFoo{10};
    S32 mBar = 20;
};

template<> template<>
void cpp_features_test_object_t::test<8>()
{
    InitInline ii;
    ensure("init member inline 1", ii.mFoo==10);

    InitInlineWithConstructor iici;
    ensure("init member inline 2", iici.mFoo=10);
    ensure("init member inline 3", iici.mBar==25);
}

// constexpr
//
// https://en.cppreference.com/w/cpp/language/constexpr
//
// Various things can be computed at compile time, and flagged as constexpr.
constexpr S32 compute2() { return 2; }

constexpr S32 ce_factorial(S32 n)
{
    if (n<=0)
    {
        return 1;
    }
    else
    {
        return n*ce_factorial(n-1);
    }
}

template<> template<>
void cpp_features_test_object_t::test<9>()
{
    S32 val = compute2();
    ensure("constexpr 1", val==2);

    // Compile-time factorial. You used to need complex templates to do something this useless.
    S32 fac5 = ce_factorial(5);
    ensure("constexpr 2", fac5==120);
}

// static assert
//
// https://en.cppreference.com/w/cpp/language/static_assert
//
// You can add asserts to be checked at compile time. The thing to be checked must be a constexpr.
// There are two forms:
// * static_assert(expr);
// * static_assert(expr, message);
//
// Currently only the 2-parameter form works on windows. The 1-parameter form needs a flag we don't set.

template<> template<>
void cpp_features_test_object_t::test<10>()
{
    // static_assert(ce_factorial(6)==720); No, needs a flag we don't currently set.
    static_assert(ce_factorial(6)==720, "bad factorial"); // OK
}

// type aliases
//
// https://en.cppreference.com/w/cpp/language/type_alias
// 
// You can use the "using" statement to create simpler templates that
// are aliases for more complex ones. "Template typedef"

// This makes stringmap<T> an alias for std::map<std::string, T>
template<typename T>
using stringmap = std::map<std::string, T>;

template<> template<>
void cpp_features_test_object_t::test<11>()
{
    stringmap<S32> name_counts{ {"alice", 3}, {"bob", 2} };
    ensure("type alias", name_counts["bob"]==2);
}

// Other possibilities:

// nullptr

// class enums

// std::unique_ptr and make_unique

// std::shared_ptr and make_shared

// lambdas

// perfect forwarding

// variadic templates

// std::thread

// std::mutex

// thread_local

// rvalue reference &&

// move semantics

// std::move

// string_view

} // namespace tut
