#pragma once

/**
 * @file ll_doctest_helpers.h
 * @brief Reusable assertion helpers for doctest-based unit tests.
 *
 * Usage guidelines:
 * - Prefer `LL_CHECK_MSG` when you want to attach a readable failure message to a boolean expression.
 *   Example:
 *     LL_CHECK_MSG(result.success(), "login should succeed with valid credentials");
 *
 * - Use `LL_CHECK_APPROX` for floating-point comparisons that require a tolerance.
 *   Example:
 *     LL_CHECK_APPROX(actual_value, expected_value, 1.0e-5);
 *
 * - Employ `LL_CHECK_EQ_RANGE` when validating contiguous buffers or array contents.
 *   It will emit the first mismatching index to ease debugging.
 *   Example:
 *     LL_CHECK_EQ_RANGE(buffer_a.data(), buffer_b.data(), buffer_a.size());
 */

#include "doctest.h"

#include <cstddef>
#include <sstream>

namespace ll::test::detail
{
template <typename TLeft, typename TRight>
inline void check_range_equal(
    const TLeft* lhs,
    const TRight* rhs,
    std::size_t length,
    const char* lhs_expr,
    const char* rhs_expr)
{
    bool match = true;
    std::size_t mismatch_index = 0;

    for (std::size_t index = 0; index < length; ++index)
    {
        if (!(lhs[index] == rhs[index]))
        {
            match = false;
            mismatch_index = index;
            break;
        }
    }

    if (!match)
    {
        std::ostringstream description;
        description << lhs_expr << "[" << mismatch_index << "] ("
                    << lhs[mismatch_index] << ") differs from "
                    << rhs_expr << "[" << mismatch_index << "] ("
                    << rhs[mismatch_index] << ")";
        INFO("range length: " << length);
        INFO("mismatch index: " << mismatch_index);
        CHECK_MESSAGE(false, description.str());
        return;
    }

    CHECK(match);
}
} // namespace ll::test::detail

#define LL_CHECK_APPROX(actual, expected, epsilon) \
    CHECK((actual) == doctest::Approx(expected).epsilon(epsilon))

#define LL_CHECK_EQ_RANGE(ptrA, ptrB, len)                                                                  \
    do                                                                                                      \
    {                                                                                                       \
        const auto* _ll_ptrA = (ptrA);                                                                      \
        const auto* _ll_ptrB = (ptrB);                                                                      \
        const std::size_t _ll_length = static_cast<std::size_t>(len);                                       \
        ::ll::test::detail::check_range_equal(_ll_ptrA, _ll_ptrB, _ll_length, #ptrA, #ptrB);                \
    } while (false)

#define LL_CHECK_MSG(condition, message) CHECK_MESSAGE((condition), (message))
