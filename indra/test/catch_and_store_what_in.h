/**
 * @file   catch_and_store_what_in.h
 * @author Nat Goodspeed
 * @date   2012-02-15
 * @brief  catch_what() template function, CATCH_AND_STORE_WHAT_IN() macro
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_CATCH_AND_STORE_WHAT_IN_H)
#define LL_CATCH_AND_STORE_WHAT_IN_H

/**
 * In the brave new world of lambdas, we can use a nicer C++ idiom for testing
 * exceptions than CATCH_AND_STORE_WHAT_IN() below, e.g.:
 *
 * @code
 * std::string threw = catch_what<std::runtime_error>(
 *     [](){ throw std::runtime_error("badness"); });
 * ensure_equals(threw, "badness");
 * @endcode
 */
template <typename EXCEPTION, typename FUNC>
std::string catch_what(FUNC func)
{
    try
    {
        func();
        return {};
    }
    catch (const EXCEPTION& err)
    {
        return err.what();
    }
}

/**
 * Idiom useful for test programs: catch an expected exception, store its
 * what() string in a specified std::string variable. From there the caller
 * can do things like:
 * @code
 * ensure("expected exception not thrown", ! string.empty());
 * @endcode
 * or
 * @code
 * ensure_contains("exception doesn't mention blah", string, "blah");
 * @endcode
 * etc.
 *
 * The trouble is that when linking to a dynamic libllcommon.so on Linux, we
 * generally fail to catch the specific exception. Oddly, we can catch it as
 * std::runtime_error and validate its typeid().name(), so we do -- but that's
 * a lot of boilerplate per test. Encapsulate with this macro. Usage:
 *
 * @code
 * std::string threw;
 * try
 * {
 *     some_call_that_should_throw_Foo();
 * }
 * CATCH_AND_STORE_WHAT_IN(threw, Foo)
 * ensure("some_call_that_should_throw_Foo() didn't throw", ! threw.empty());
 * @endcode
 */
#define CATCH_AND_STORE_WHAT_IN(THREW, EXCEPTION)   \
catch (const EXCEPTION& ex)                         \
{                                                   \
    (THREW) = ex.what();                            \
}                                                   \
CATCH_MISSED_LINUX_EXCEPTION(THREW, EXCEPTION)

#ifndef LL_LINUX
#define CATCH_MISSED_LINUX_EXCEPTION(THREW, EXCEPTION)                  \
    /* only needed on Linux */
#else // LL_LINUX

#define CATCH_MISSED_LINUX_EXCEPTION(THREW, EXCEPTION)                  \
catch (const std::runtime_error& ex)                                    \
{                                                                       \
    /* This clause is needed on Linux, on the viewer side, because  */  \
    /* the exception isn't caught by catch (const EXCEPTION&).      */  \
    /* But if the expected exception was thrown, allow the test to  */  \
    /* succeed anyway. Not sure how else to handle this odd case.   */  \
    if (std::string(typeid(ex).name()) == typeid(EXCEPTION).name())     \
    {                                                                   \
        /* std::cerr << "Caught " << typeid(ex).name() */               \
        /*           << " with Linux workaround" << std::endl; */       \
        (THREW) = ex.what();                                            \
        /*std::cout << ex.what() << std::endl;*/                        \
    }                                                                   \
    else                                                                \
    {                                                                   \
        /* We don't even recognize this exception. Let it propagate */  \
        /* out to TUT to fail the test.                             */  \
        throw;                                                          \
    }                                                                   \
}                                                                       \
catch (...)                                                             \
{                                                                       \
    std::cerr << "Failed to catch expected exception "                  \
              << #EXCEPTION << "!" << std::endl;                        \
    /* This indicates a problem in the test that should be addressed. */ \
    throw;                                                              \
}

#endif // LL_LINUX

#endif /* ! defined(LL_CATCH_AND_STORE_WHAT_IN_H) */
