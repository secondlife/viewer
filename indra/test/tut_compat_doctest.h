#pragma once

/**
 * @file tut_compat_doctest.h
 * @brief Lightweight compatibility layer allowing generated TUT-style tests
 *        to build on top of doctest.
 *
 * This header is intended for auto-generated sources only. It maps the
 * most common TUT primitives to doctest while providing safe fallbacks.
 * Unsupported constructs should be replaced by the generator with explicit
 * DOCTEST_FAIL markers.
 */

#include "doctest.h"
#include "ll_doctest_helpers.h"

#include <cmath>
#include <sstream>
#include <string>

namespace tut_compat
{
inline void ensure(bool condition)
{
    CHECK(condition);
}

inline void ensure(const char* message, bool condition)
{
    CHECK_MESSAGE(condition, message);
}

template <typename Left, typename Right>
inline void ensure_equals(const Left& lhs, const Right& rhs)
{
    CHECK(lhs == rhs);
}

template <typename Left, typename Right>
inline void ensure_equals(const char* message, const Left& lhs, const Right& rhs)
{
    CHECK_MESSAGE(lhs == rhs, message);
}

template <typename Expr>
inline void ensure_not(const Expr& value)
{
    CHECK_FALSE(value);
}

template <typename Expr>
inline void ensure_not(const char* message, const Expr& value)
{
    CHECK_MESSAGE(!value, message);
}

template <typename Func>
inline void ensure_throws(Func&& fn)
{
    CHECK_THROWS(fn());
}

template <typename Func, typename Exception>
inline void ensure_throws(const char* message, Func&& fn, Exception)
{
    (void)message;
    CHECK_THROWS_AS(fn(), Exception);
}

inline void set_test_name(const char* name)
{
    INFO("test name: " << (name ? name : "<null>"));
}

inline void skip(const char* reason)
{
    INFO("skip requested: " << (reason ? reason : "<unspecified>"));
    DOCTEST_FAIL("TODO: original test requested skip");
}
} // namespace tut_compat

#define TUT_ENSURE(...) ::tut_compat::ensure(__VA_ARGS__)
#define TUT_ENSURE_EQ(...) ::tut_compat::ensure_equals(__VA_ARGS__)
#define TUT_ENSURE_NOT(...) ::tut_compat::ensure_not(__VA_ARGS__)
#define TUT_ENSURE_THROWS(expr) ::tut_compat::ensure_throws([&]() { expr; })
#define TUT_CHECK_MSG(cond, msg) CHECK_MESSAGE((cond), (msg))
#define TUT_SUITE(name) TEST_SUITE(name)
#define TUT_CASE(name) TEST_CASE(name)
#define TUT_SET_TEST_NAME(name) ::tut_compat::set_test_name(name)
#define TUT_SKIP(reason) ::tut_compat::skip(reason)
