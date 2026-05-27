/**
 * @file tut/tut.hpp
 * @brief Compatibility layer for legacy TUT-style tests, implemented locally
 *        while the viewer test harness moves to doctest.
 */

#ifndef LL_TEST_TUT_COMPAT_HPP
#define LL_TEST_TUT_COMPAT_HPP

#include "doctest/doctest.h"

#include <cmath>
#include <exception>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

namespace tut
{
    class failure : public std::exception
    {
    public:
        explicit failure(const std::string& message) : mMessage(message) {}
        explicit failure(const char* message) : mMessage(message ? message : "") {}
        const char* what() const noexcept override { return mMessage.c_str(); }

    private:
        std::string mMessage;
    };

    class skipped : public std::exception
    {
    public:
        explicit skipped(const std::string& message) : mMessage(message) {}
        const char* what() const noexcept override { return mMessage.c_str(); }

    private:
        std::string mMessage;
    };

    class no_such_test : public std::exception
    {
    public:
        const char* what() const noexcept override { return "no such test"; }
    };

    struct test_result
    {
        enum result_type
        {
            ok,
            fail,
            ex,
            warn,
            term,
            skip
        };

        std::string group;
        int test = 0;
        std::string name;
        result_type result = ok;
        std::string message;
        std::string exception_typeid;
    };

    class callback
    {
    public:
        virtual ~callback() = default;
        virtual void run_started() {}
        virtual void group_started(const std::string&) {}
        virtual void test_completed(const test_result&) {}
        virtual void group_completed(const std::string&) {}
        virtual void run_completed() {}
    };

    using groupnames = std::set<std::string>;

    namespace detail
    {
        inline std::string& current_test_name()
        {
            static thread_local std::string name;
            return name;
        }

        template <typename T>
        std::string stringify(const T& value)
        {
            std::ostringstream out;
            out << value;
            return out.str();
        }

        inline std::string stringify(const char* value)
        {
            return value ? std::string(value) : std::string("(null)");
        }

        inline std::string stringify(char* value)
        {
            return stringify(static_cast<const char*>(value));
        }
    }

    inline void set_test_name(const std::string& name)
    {
        detail::current_test_name() = name;
    }

    inline void fail(const std::string& message)
    {
        throw failure(message);
    }

    inline void fail(const char* message)
    {
        throw failure(message);
    }

    inline void skip(const std::string& message)
    {
        throw skipped(message);
    }

    inline void skip(const char* message)
    {
        throw skipped(message ? message : "");
    }

    template <typename T>
    void ensure(const T& condition)
    {
        if (!condition)
        {
            throw failure("condition failed");
        }
    }

    template <typename T>
    void ensure(const std::string& message, const T& condition)
    {
        if (!condition)
        {
            throw failure(message);
        }
    }

    template <typename T>
    void ensure(const char* message, const T& condition)
    {
        ensure(std::string(message ? message : ""), condition);
    }

    template <typename T>
    void ensure_not(const std::string& message, const T& condition)
    {
        ensure(message, !condition);
    }

    template <typename T>
    void ensure_not(const char* message, const T& condition)
    {
        ensure_not(std::string(message ? message : ""), condition);
    }

    template <typename Actual, typename Expected>
    void ensure_equals(const std::string& message, const Actual& actual, const Expected& expected)
    {
        if (!(actual == expected))
        {
            std::ostringstream out;
            out << message << (message.empty() ? "" : ": ")
                << "expected " << detail::stringify(expected)
                << " but got " << detail::stringify(actual);
            throw failure(out.str());
        }
    }

    template <typename Actual, typename Expected>
    void ensure_equals(const char* message, const Actual& actual, const Expected& expected)
    {
        ensure_equals(std::string(message ? message : ""), actual, expected);
    }

    inline void ensure_equals(const std::string& message, const char* actual, const char* expected)
    {
        ensure_equals(message, std::string(actual ? actual : ""), std::string(expected ? expected : ""));
    }

    inline void ensure_equals(const char* message, const char* actual, const char* expected)
    {
        ensure_equals(std::string(message ? message : ""), actual, expected);
    }

    template <typename Actual, typename Expected>
    void ensure_equals(const Actual& actual, const Expected& expected)
    {
        ensure_equals(std::string(), actual, expected);
    }

    inline void ensure_equals(const char* actual, const char* expected)
    {
        ensure_equals(std::string(), actual, expected);
    }

    template <typename Actual, typename Expected, typename Tolerance>
    void ensure_distance(const std::string& message, const Actual& actual, const Expected& expected, const Tolerance& tolerance)
    {
        if (std::fabs(actual - expected) > tolerance)
        {
            std::ostringstream out;
            out << message << (message.empty() ? "" : ": ")
                << "expected " << detail::stringify(expected)
                << " +/- " << detail::stringify(tolerance)
                << " but got " << detail::stringify(actual);
            throw failure(out.str());
        }
    }

    template <typename Actual, typename Expected, typename Tolerance>
    void ensure_distance(const char* message, const Actual& actual, const Expected& expected, const Tolerance& tolerance)
    {
        ensure_distance(std::string(message ? message : ""), actual, expected, tolerance);
    }

    template <typename Actual, typename Expected, typename Tolerance>
    void ensure_distance(const Actual& actual, const Expected& expected, const Tolerance& tolerance)
    {
        ensure_distance(std::string(), actual, expected, tolerance);
    }

    class test_group_base
    {
    public:
        virtual ~test_group_base() = default;
        virtual const std::string& name() const = 0;
        virtual int max_tests() const = 0;
        virtual test_result run_test(int test_number) = 0;
    };

    class test_runner
    {
    public:
        callback& get_callback() { return *mCallback; }
        void set_callback(callback* cb) { mCallback = cb ? cb : &mDefaultCallback; }

        void register_group(test_group_base* group)
        {
            mGroups[group->name()] = group;
        }

        groupnames list_groups() const
        {
            groupnames result;
            for (const auto& entry : mGroups)
            {
                result.insert(entry.first);
            }
            return result;
        }

        void run_tests()
        {
            mCallback->run_started();
            for (const auto& entry : mGroups)
            {
                run_group(*entry.second);
            }
            mCallback->run_completed();
        }

        void run_tests(const std::string& group_name)
        {
            mCallback->run_started();
            auto found = mGroups.find(group_name);
            if (found != mGroups.end())
            {
                run_group(*found->second);
            }
            mCallback->run_completed();
        }

    private:
        void run_group(test_group_base& group)
        {
            mCallback->group_started(group.name());
            for (int test_number = 1; test_number <= group.max_tests(); ++test_number)
            {
                test_result result = group.run_test(test_number);
                if (result.result != test_result::term)
                {
                    mCallback->test_completed(result);
                }
            }
            mCallback->group_completed(group.name());
        }

        callback mDefaultCallback;
        callback* mCallback = &mDefaultCallback;
        std::map<std::string, test_group_base*> mGroups;
    };

    class test_runner_singleton
    {
    public:
        test_runner& get()
        {
            static test_runner runner;
            return runner;
        }
    };

    template <typename Data, int MaxTests = 50>
    class test_group : public test_group_base
    {
    public:
        class object : public Data
        {
        public:
            template <int Test>
            void test()
            {
                throw no_such_test();
            }

            void set_test_name(const std::string& name)
            {
                tut::set_test_name(name);
            }
        };

        explicit test_group(const std::string& name) : mName(name)
        {
            runner.get().register_group(this);
        }

        const std::string& name() const override { return mName; }
        int max_tests() const override { return MaxTests; }

        test_result run_test(int test_number) override
        {
            test_result result;
            result.group = mName;
            result.test = test_number;
            detail::current_test_name().clear();

            try
            {
                run_test_number(test_number);
                result.result = test_result::ok;
            }
            catch (const no_such_test&)
            {
                result.result = test_result::term;
            }
            catch (const skipped& e)
            {
                result.result = test_result::skip;
                result.message = e.what();
            }
            catch (const failure& e)
            {
                result.result = test_result::fail;
                result.message = e.what();
            }
            catch (const std::exception& e)
            {
                result.result = test_result::ex;
                result.message = e.what();
                result.exception_typeid = typeid(e).name();
            }
            catch (...)
            {
                result.result = test_result::ex;
                result.message = "unknown exception";
                result.exception_typeid = "unknown";
            }

            result.name = detail::current_test_name();
            return result;
        }

    private:
        template <int Test>
        typename std::enable_if<(Test > MaxTests), void>::type run_test_number_impl(int)
        {
            throw no_such_test();
        }

        template <int Test>
        typename std::enable_if<(Test <= MaxTests), void>::type run_test_number_impl(int test_number)
        {
            if (Test == test_number)
            {
                object obj;
                obj.template test<Test>();
                return;
            }
            run_test_number_impl<Test + 1>(test_number);
        }

        void run_test_number(int test_number)
        {
            run_test_number_impl<1>(test_number);
        }

        std::string mName;
    };

    extern test_runner_singleton runner;
}

#endif
