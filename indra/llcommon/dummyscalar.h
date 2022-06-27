/**
 * @file   dummyscalar.h
 * @author Nat Goodspeed
 * @date   2022-06-24
 * @brief  Drop-in replacement for statistics-gathering scalar types
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Copyright (c) 2022, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_DUMMYSCALAR_H)
#define LL_DUMMYSCALAR_H

#include "stdtypes.h"
#include <iostream>

namespace LL
{

    /**
     * DummyScalar is a template class that presents the API of a numeric
     * scalar, but stores nothing and performs no operations.
     *
     * Certain classes are coded to track internal usage statistics. When
     * those classes can be used across threads, the statistics must be
     * thread-safe: e.g. std::atomic, LLScalarCond or defended with an
     * explicit mutex.
     *
     * In the spirit of only paying for what you use, though, we want to allow
     * compiling out certain statistics tracking altogether unless there's an
     * actual consumer. But wrapping every reference to any such variable in
     * #ifdef is not only tedious but ugly: it makes it harder to read the
     * business logic, making maintenance a bit more expensive and a bit more
     * fragile.
     *
     * #if logic can be used to replace the declaration of a statistics
     * variable with DummyScalar, allowing references in code to remain as-is.
     */
    template <typename SCALAR=U32, SCALAR DEFAULT=SCALAR()>
    struct DummyScalar
    {
        DummyScalar(SCALAR init=DEFAULT) {}
        operator SCALAR()  const { return DEFAULT; }
        SCALAR operator+() const { return DEFAULT; }
        SCALAR operator-() const { return DEFAULT; }
        SCALAR operator~() const { return DEFAULT; }

        operator bool() const  { return bool(DEFAULT); }
        bool operator!() const { return ! bool(*this); }

        bool operator==(SCALAR other) const { return (other == DEFAULT); }
        bool operator!=(SCALAR other) const { return ! (*this == other); }
        bool operator< (SCALAR other) const { return false; }
        bool operator<=(SCALAR other) const { return false; }
        bool operator> (SCALAR other) const { return false; }
        bool operator>=(SCALAR other) const { return false; }

        SCALAR operator=(SCALAR other) { return DEFAULT; }
        SCALAR operator++()    { return DEFAULT; } // pre-increment
        SCALAR operator++(int) { return DEFAULT; } // post-increment
        SCALAR operator--()    { return DEFAULT; } // pre-decrement
        SCALAR operator--(int) { return DEFAULT; } // post-decrement

        SCALAR operator+(SCALAR other) const { return DEFAULT; }
        SCALAR operator-(SCALAR other) const { return DEFAULT; }
        SCALAR operator*(SCALAR other) const { return DEFAULT; }
        SCALAR operator/(SCALAR other) const { return DEFAULT; }
        SCALAR operator%(SCALAR other) const { return DEFAULT; }
        SCALAR operator&(SCALAR other) const { return DEFAULT; }
        SCALAR operator|(SCALAR other) const { return DEFAULT; }
        SCALAR operator^(SCALAR other) const { return DEFAULT; }
        SCALAR operator<<(SCALAR other) const { return DEFAULT; }
        SCALAR operator>>(SCALAR other) const { return DEFAULT; }
        SCALAR operator&&(SCALAR other) const { return DEFAULT; }
        SCALAR operator||(SCALAR other) const { return DEFAULT; }

        SCALAR operator+=(SCALAR other) { return DEFAULT; }
        SCALAR operator-=(SCALAR other) { return DEFAULT; }
        SCALAR operator*=(SCALAR other) { return DEFAULT; }
        SCALAR operator/=(SCALAR other) { return DEFAULT; }
        SCALAR operator%=(SCALAR other) { return DEFAULT; }
        SCALAR operator&=(SCALAR other) { return DEFAULT; }
        SCALAR operator|=(SCALAR other) { return DEFAULT; }
        SCALAR operator^=(SCALAR other) { return DEFAULT; }
        SCALAR operator<<=(SCALAR other) { return DEFAULT; }
        SCALAR operator>>=(SCALAR other) { return DEFAULT; }

        friend std::ostream& operator<<(std::ostream& out, const DummyScalar& self)
        {
            return out << DEFAULT;
        }
    };

    // A little less ugly than writing DummyScalar<>
    using DummyCount = DummyScalar<>;

} // namespace LL

#endif /* ! defined(LL_DUMMYSCALAR_H) */
