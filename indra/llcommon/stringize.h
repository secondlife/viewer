/**
 * @file   stringize.h
 * @author Nat Goodspeed
 * @date   2008-12-17
 * @brief  stringize(item) template function and STRINGIZE(expression) macro
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#if ! defined(LL_STRINGIZE_H)
#define LL_STRINGIZE_H

#include <sstream>
#include "llstring.h"
#include <boost/call_traits.hpp>

/**
 * stream_to(std::ostream&, items, ...) streams each item in the parameter list
 * to the passed std::ostream using the insertion operator <<. This can be
 * used, for instance, to make a simple print() function, e.g.:
 *
 * @code
 * template <typename... Items>
 * void print(Items&&... items)
 * {
 *     stream_to(std::cout, std::forward<Items>(items)...);
 * }
 * @endcode
 */
// recursion tail
template <typename CHARTYPE>
void stream_to(std::basic_ostream<CHARTYPE>& out) {}
// stream one or more items
template <typename CHARTYPE, typename T, typename... Items>
void stream_to(std::basic_ostream<CHARTYPE>& out, T&& item, Items&&... items)
{
    out << std::forward<T>(item);
    stream_to(out, std::forward<Items>(items)...);
}

// why we use function overloads, not function template specializations:
// http://www.gotw.ca/publications/mill17.htm

/**
 * gstringize(item, ...) encapsulates an idiom we use constantly, using
 * operator<<(std::ostringstream&, TYPE) followed by std::ostringstream::str()
 * or their wstring equivalents to render a string expressing one or more items.
 */
// two or more args - the case of a single argument is handled separately
template <typename CHARTYPE, typename T0, typename T1, typename... Items>
auto gstringize(T0&& item0, T1&& item1, Items&&... items)
{
    std::basic_ostringstream<CHARTYPE> out;
    stream_to(out, std::forward<T0>(item0), std::forward<T1>(item1),
              std::forward<Items>(items)...);
    return out.str();
}

// generic single argument: stream to out, as above
template <typename CHARTYPE, typename T>
struct gstringize_impl
{
    auto operator()(typename boost::call_traits<T>::param_type arg)
    {
        std::basic_ostringstream<CHARTYPE> out;
        out << arg;
        return out.str();
    }
};

// partially specialize for a single STRING argument -
// note that ll_convert<T>(T) already handles the trivial case
template <typename OUTCHAR, typename INCHAR>
struct gstringize_impl<OUTCHAR, std::basic_string<INCHAR>>
{
    auto operator()(const std::basic_string<INCHAR>& arg)
    {
        return ll_convert<std::basic_string<OUTCHAR>>(arg);
    }
};

// partially specialize for a single CHARTYPE* argument -
// since it's not a basic_string and we do want to optimize this common case
template <typename OUTCHAR, typename INCHAR>
struct gstringize_impl<OUTCHAR, INCHAR*>
{
    auto operator()(const INCHAR* arg)
    {
        return ll_convert<std::basic_string<OUTCHAR>>(arg);
    }
};

// gstringize(single argument)
template <typename CHARTYPE, typename T>
auto gstringize(T&& item)
{
    // use decay<T> so we don't require separate specializations
    // for T, const T, T&, const T& ...
    return gstringize_impl<CHARTYPE, std::decay_t<T>>()(std::forward<T>(item));
}

/**
 * Specialization of gstringize for std::string return types
 */
template <typename... Items>
auto stringize(Items&&... items)
{
    return gstringize<char>(std::forward<Items>(items)...);
}

/**
 * Specialization of gstringize for std::wstring return types
 */
template <typename... Items>
auto wstringize(Items&&... items)
{
    return gstringize<wchar_t>(std::forward<Items>(items)...);
}

/**
 * stringize_f(functor)
 */
template <typename CHARTYPE, typename Functor>
std::basic_string<CHARTYPE> stringize_f(Functor const & f)
{
    std::basic_ostringstream<CHARTYPE> out;
    f(out);
    return out.str();
}

/**
 * STRINGIZE(item1 << item2 << item3 ...) effectively expands to the
 * following:
 * @code
 * std::ostringstream out;
 * out << item1 << item2 << item3 ... ;
 * return out.str();
 * @endcode
 */
#define STRINGIZE(EXPRESSION) (stringize_f<char>([&](std::ostream& out){ out << EXPRESSION; }))

/**
 * WSTRINGIZE() is the wstring equivalent of STRINGIZE()
 */
#define WSTRINGIZE(EXPRESSION) (stringize_f<wchar_t>([&](std::wostream& out){ out << EXPRESSION; }))

/**
 * destringize(str)
 * defined for symmetry with stringize
 * @NOTE - this has distinct behavior from boost::lexical_cast<T> regarding
 * leading/trailing whitespace and handling of bad_lexical_cast exceptions
 * @NOTE - no need for dewstringize(), since passing std::wstring will Do The
 * Right Thing
 */
template <typename T, typename CHARTYPE>
T destringize(std::basic_string<CHARTYPE> const & str)
{
    T val;
    std::basic_istringstream<CHARTYPE> in(str);
    in >> val;
    return val;
}

/**
 * destringize_f(str, functor)
 */
template <typename CHARTYPE, typename Functor>
void destringize_f(std::basic_string<CHARTYPE> const & str, Functor const & f)
{
    std::basic_istringstream<CHARTYPE> in(str);
    f(in);
}

/**
 * DESTRINGIZE(str, item1 >> item2 >> item3 ...) effectively expands to the
 * following:
 * @code
 * std::istringstream in(str);
 * in >> item1 >> item2 >> item3 ... ;
 * @endcode
 */
#define DESTRINGIZE(STR, EXPRESSION) (destringize_f((STR), [&](auto& in){in >> EXPRESSION;}))
// legacy name, just use DESTRINGIZE() going forward
#define DEWSTRINGIZE(STR, EXPRESSION) DESTRINGIZE(STR, EXPRESSION)

#endif /* ! defined(LL_STRINGIZE_H) */
