/**
 * @file llerror_test.cpp
 * @date   December 2006
 * @brief error unit tests
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include <vector>
#include <stdexcept>

#include "linden_common.h"

#include "../llerror.h"

#include "../llerrorcontrol.h"
#include "../llsd.h"

#include "../test/lltut.h"

enum LogFieldIndex
{
    TIME_FIELD,
    LEVEL_FIELD,
    TAGS_FIELD,
    LOCATION_FIELD,
    FUNCTION_FIELD,
    MSG_FIELD
};

static const char* FieldName[] =
{
    "TIME",
    "LEVEL",
    "TAGS",
    "LOCATION",
    "FUNCTION",
    "MSG"
};

namespace
{
#ifdef __clang__
#   pragma clang diagnostic ignored "-Wunused-function"
#endif
    void test_that_error_h_includes_enough_things_to_compile_a_message()
    {
        LL_INFOS() << "!" << LL_ENDL;
    }
}

namespace
{
    static bool fatalWasCalled = false;
    struct FatalWasCalled: public std::runtime_error
    {
        FatalWasCalled(const std::string& what): std::runtime_error(what) {}
    };
    void fatalCall(const std::string& msg) { throw FatalWasCalled(msg); }
}

// Because we use LLError::setFatalFunction(fatalCall), any LL_ERRS call we
// issue will throw FatalWasCalled. But we want the test program to continue.
// So instead of writing:
// LL_ERRS("tag") << "some message" << LL_ENDL;
// write:
// CATCH(LL_ERRS("tag"), "some message");
#define CATCH(logcall, expr)                    \
    try                                         \
    {                                           \
        logcall << expr << LL_ENDL;             \
    }                                           \
    catch (const FatalWasCalled&)               \
    {                                           \
        fatalWasCalled = true;                  \
    }

namespace tut
{
    class TestRecorder : public LLError::Recorder
    {
    public:
        TestRecorder()
            {
                showTime(false);
            }
        virtual ~TestRecorder()
            {}

        virtual void recordMessage(LLError::ELevel level,
                           const std::string& message)
        {
            mMessages.push_back(message);
        }

        int countMessages()         { return (int) mMessages.size(); }
        void clearMessages()        { mMessages.clear(); }

        std::string message(int n)
        {
            std::ostringstream test_name;
            test_name << "testing message " << n << ", not enough messages";

            tut::ensure(test_name.str(), n < countMessages());
            return mMessages[n];
        }

    private:
        typedef std::vector<std::string> MessageVector;
        MessageVector mMessages;
    };

    struct ErrorTestData
    {
        LLError::RecorderPtr mRecorder;
        LLError::SettingsStoragePtr mPriorErrorSettings;

        ErrorTestData():
            mRecorder(new TestRecorder())
        {
            fatalWasCalled = false;

            mPriorErrorSettings = LLError::saveAndResetSettings();
            LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
            LLError::setFatalFunction(fatalCall);
            LLError::addRecorder(mRecorder);
        }

        ~ErrorTestData()
        {
            LLError::removeRecorder(mRecorder);
            LLError::restoreSettings(mPriorErrorSettings);
        }

        int countMessages()
        {
            return std::dynamic_pointer_cast<TestRecorder>(mRecorder)->countMessages();
        }

        void clearMessages()
        {
            std::dynamic_pointer_cast<TestRecorder>(mRecorder)->clearMessages();
        }

        void setWantsTime(bool t)
            {
                std::dynamic_pointer_cast<TestRecorder>(mRecorder)->showTime(t);
            }

        void setWantsMultiline(bool t)
            {
                std::dynamic_pointer_cast<TestRecorder>(mRecorder)->showMultiline(t);
            }

        std::string message(int n)
        {
            return std::dynamic_pointer_cast<TestRecorder>(mRecorder)->message(n);
        }

        void ensure_message_count(int expectedCount)
        {
            ensure_equals("message count", countMessages(), expectedCount);
        }

        std::string message_field(int msgnum, LogFieldIndex fieldnum)
        {
            std::ostringstream test_name;
            test_name << "testing message " << msgnum << ", not enough messages";
            tut::ensure(test_name.str(), msgnum < countMessages());

            std::string msg(message(msgnum));

            std::string field_value;

            // find the start of the field; fields are separated by a single space
            size_t scan = 0;
            int on_field = 0;
            while ( scan < msg.length() && on_field < fieldnum )
            {
                // fields are delimited by one space
                if ( ' ' == msg[scan] )
                {
                    if ( on_field < FUNCTION_FIELD )
                    {
                        on_field++;
                    }
                    // except function, which may have embedded spaces so ends with " : "
                    else if (   ( on_field == FUNCTION_FIELD )
                             && ( ':' == msg[scan+1] && ' ' == msg[scan+2] )
                             )
                    {
                        on_field++;
                        scan +=2;
                    }
                }
                scan++;
            }
            size_t start_field = scan;
            size_t fieldlen = 0;
            if ( fieldnum < FUNCTION_FIELD )
            {
                fieldlen = msg.find(' ', start_field) - start_field;
            }
            else if ( fieldnum == FUNCTION_FIELD )
            {
                fieldlen = msg.find(" : ", start_field) - start_field;
            }
            else if ( MSG_FIELD == fieldnum ) // no delimiter, just everything to the end
            {
                fieldlen = msg.length() - start_field;
            }

            return msg.substr(start_field, fieldlen);
        }

        void ensure_message_field_equals(int msgnum, LogFieldIndex fieldnum, const std::string& expectedText)
         {
             std::ostringstream test_name;
             test_name << "testing message " << msgnum << " field " << FieldName[fieldnum] << "\n  message: \"" << message(msgnum) << "\"\n  ";

             ensure_equals(test_name.str(), message_field(msgnum, fieldnum), expectedText);
         }

        void ensure_message_does_not_contain(int n, const std::string& expectedText)
        {
            std::ostringstream test_name;
            test_name << "testing message " << n;

            ensure_does_not_contain(test_name.str(), message(n), expectedText);
        }
    };

    typedef test_group<ErrorTestData>   ErrorTestGroup;
    typedef ErrorTestGroup::object      ErrorTestObject;

    ErrorTestGroup errorTestGroup("error");

    template<> template<>
    void ErrorTestObject::test<1>()
        // basic test of output
    {
        LL_INFOS() << "test" << LL_ENDL;
        LL_INFOS() << "bob" << LL_ENDL;

        ensure_message_field_equals(0, MSG_FIELD, "test");
        ensure_message_field_equals(1, MSG_FIELD, "bob");
    }
}

namespace
{
    void writeSome()
    {
        LL_DEBUGS("WriteTag","AnotherTag") << "one" << LL_ENDL;
        LL_INFOS("WriteTag") << "two" << LL_ENDL;
        LL_WARNS("WriteTag") << "three" << LL_ENDL;
        CATCH(LL_ERRS("WriteTag"), "four");
    }
};

namespace tut
{
    template<> template<>
    void ErrorTestObject::test<2>()
        // messages are filtered based on default level
    {
        LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
        writeSome();
        ensure_message_field_equals(0, MSG_FIELD, "one");
        ensure_message_field_equals(0, LEVEL_FIELD, "DEBUG");
        ensure_message_field_equals(0, TAGS_FIELD, "#WriteTag#AnotherTag#");
        ensure_message_field_equals(1, MSG_FIELD, "two");
        ensure_message_field_equals(1, LEVEL_FIELD, "INFO");
        ensure_message_field_equals(1, TAGS_FIELD, "#WriteTag#");
        ensure_message_field_equals(2, MSG_FIELD, "three");
        ensure_message_field_equals(2, LEVEL_FIELD, "WARNING");
        ensure_message_field_equals(2, TAGS_FIELD, "#WriteTag#");
        ensure_message_field_equals(3, MSG_FIELD, "four");
        ensure_message_field_equals(3, LEVEL_FIELD, "ERROR");
        ensure_message_field_equals(3, TAGS_FIELD, "#WriteTag#");
        ensure_message_count(4);

        LLError::setDefaultLevel(LLError::LEVEL_INFO);
        writeSome();
        ensure_message_field_equals(4, MSG_FIELD, "two");
        ensure_message_field_equals(5, MSG_FIELD, "three");
        ensure_message_field_equals(6, MSG_FIELD, "four");
        ensure_message_count(7);

        LLError::setDefaultLevel(LLError::LEVEL_WARN);
        writeSome();
        ensure_message_field_equals(7, MSG_FIELD, "three");
        ensure_message_field_equals(8, MSG_FIELD, "four");
        ensure_message_count(9);

        LLError::setDefaultLevel(LLError::LEVEL_ERROR);
        writeSome();
        ensure_message_field_equals(9, MSG_FIELD, "four");
        ensure_message_count(10);

        LLError::setDefaultLevel(LLError::LEVEL_NONE);
        writeSome();
        ensure_message_count(10);
    }

    template<> template<>
    void ErrorTestObject::test<3>()
        // error type string in output
    {
        writeSome();
        ensure_message_field_equals(0, LEVEL_FIELD, "DEBUG");
        ensure_message_field_equals(1, LEVEL_FIELD, "INFO");
        ensure_message_field_equals(2, LEVEL_FIELD, "WARNING");
        ensure_message_field_equals(3, LEVEL_FIELD, "ERROR");
        ensure_message_count(4);
    }

    template<> template<>
    void ErrorTestObject::test<4>()
        // file abbreviation
    {
        std::string prev, abbreviateFile = __FILE__;
        do
        {
            prev = abbreviateFile;
            abbreviateFile = LLError::abbreviateFile(abbreviateFile);
            // __FILE__ is assumed to end with
            // indra/llcommon/tests/llerror_test.cpp. This test used to call
            // abbreviateFile() exactly once, then check below whether it
            // still contained the string 'indra'. That fails if the FIRST
            // part of the pathname also contains indra! Certain developer
            // machine images put local directory trees under
            // /ngi-persist/indra, which is where we observe the problem. So
            // now, keep calling abbreviateFile() until it returns its
            // argument unchanged, THEN check.
        } while (abbreviateFile != prev);

        ensure_ends_with("file name abbreviation",
            abbreviateFile,
            "llcommon/tests/llerror_test.cpp"
            );
        ensure_does_not_contain("file name abbreviation",
            abbreviateFile, "indra");

        std::string someFile =
#if LL_WINDOWS
            "C:/amy/bob/cam.cpp"
#else
            "/amy/bob/cam.cpp"
#endif
            ;
        std::string someAbbreviation = LLError::abbreviateFile(someFile);

        ensure_equals("non-indra file abbreviation",
            someAbbreviation, someFile);
    }
}

namespace
{
    std::string locationString(int line)
    {
        std::ostringstream location;
        location << LLError::abbreviateFile(__FILE__)
                 << "(" << line << ")";

        return location.str();
    }

    std::string writeReturningLocation()
    {
        LL_INFOS() << "apple" << LL_ENDL;   int this_line = __LINE__;
        return locationString(this_line);
    }

    void writeReturningLocationAndFunction(std::string& location, std::string& function)
    {
        LL_INFOS() << "apple" << LL_ENDL;   int this_line = __LINE__;
        location = locationString(this_line);
        function = __FUNCTION__;
    }

    std::string errorReturningLocation()
    {
        int this_line = __LINE__;   CATCH(LL_ERRS(), "die");
        return locationString(this_line);
    }
}

/* The following helper functions and class members all log a simple message
    from some particular function scope.  Each function takes a bool argument
    that indicates if it should log its own name or not (in the manner that
    existing log messages often do.)  The functions all return their C++
    name so that test can be substantial mechanized.
 */

std::string logFromGlobal(bool id)
{
    LL_INFOS() << (id ? "logFromGlobal: " : "") << "hi" << LL_ENDL;
    return "logFromGlobal";
}

static std::string logFromStatic(bool id)
{
    LL_INFOS() << (id ? "logFromStatic: " : "") << "hi" << LL_ENDL;
    return "logFromStatic";
}

namespace
{
    std::string logFromAnon(bool id)
    {
        LL_INFOS() << (id ? "logFromAnon: " : "") << "hi" << LL_ENDL;
        return "logFromAnon";
    }
}

namespace Foo {
    std::string logFromNamespace(bool id)
    {
        LL_INFOS() << (id ? "Foo::logFromNamespace: " : "") << "hi" << LL_ENDL;
        //return "Foo::logFromNamespace";
            // there is no standard way to get the namespace name, hence
            // we won't be testing for it
        return "logFromNamespace";
    }
}

namespace
{
    class ClassWithNoLogType {
    public:
        std::string logFromMember(bool id)
        {
            LL_INFOS() << (id ? "ClassWithNoLogType::logFromMember: " : "") << "hi" << LL_ENDL;
            return "ClassWithNoLogType::logFromMember";
        }
        static std::string logFromStatic(bool id)
        {
            LL_INFOS() << (id ? "ClassWithNoLogType::logFromStatic: " : "") << "hi" << LL_ENDL;
            return "ClassWithNoLogType::logFromStatic";
        }
    };

    class ClassWithLogType {
        LOG_CLASS(ClassWithLogType);
    public:
        std::string logFromMember(bool id)
        {
            LL_INFOS() << (id ? "ClassWithLogType::logFromMember: " : "") << "hi" << LL_ENDL;
            return "ClassWithLogType::logFromMember";
        }
        static std::string logFromStatic(bool id)
        {
            LL_INFOS() << (id ? "ClassWithLogType::logFromStatic: " : "") << "hi" << LL_ENDL;
            return "ClassWithLogType::logFromStatic";
        }
    };

    std::string logFromNamespace(bool id) { return Foo::logFromNamespace(id); }
    std::string logFromClassWithLogTypeMember(bool id) { ClassWithLogType c; return c.logFromMember(id); }
    std::string logFromClassWithLogTypeStatic(bool id) { return ClassWithLogType::logFromStatic(id); }

    void ensure_has(const std::string& message,
        const std::string& actual, const std::string& expected)
    {
        std::string::size_type n1 = actual.find(expected);
        if (n1 == std::string::npos)
        {
            std::stringstream ss;
            ss << message << ": " << "expected to find a copy of '" << expected
               << "' in actual '" << actual << "'";
            throw tut::failure(ss.str().c_str());
        }
    }

    typedef std::string (*LogFromFunction)(bool);
    void testLogName(LLError::RecorderPtr recorder, LogFromFunction f,
        const std::string& class_name = "")
    {
        std::dynamic_pointer_cast<tut::TestRecorder>(recorder)->clearMessages();
        std::string name = f(false);
        f(true);

        std::string messageWithoutName = std::dynamic_pointer_cast<tut::TestRecorder>(recorder)->message(0);
        std::string messageWithName = std::dynamic_pointer_cast<tut::TestRecorder>(recorder)->message(1);

        ensure_has(name + " logged without name",
            messageWithoutName, name);
        ensure_has(name + " logged with name",
            messageWithName, name);

        if (!class_name.empty())
        {
            ensure_has(name + "logged without name",
                messageWithoutName, class_name);
            ensure_has(name + "logged with name",
                messageWithName, class_name);
        }
    }
}

namespace
{
    void writeMsgNeedsEscaping()
    {
        LL_DEBUGS("WriteTag") << "backslash\\" << LL_ENDL;
        LL_INFOS("WriteTag") << "newline\nafternewline" << LL_ENDL;
        LL_WARNS("WriteTag") << "return\rafterreturn" << LL_ENDL;

        LL_DEBUGS("WriteTag") << "backslash\\backslash\\" << LL_ENDL;
        LL_INFOS("WriteTag") << "backslash\\newline\nanothernewline\nafternewline" << LL_ENDL;
        LL_WARNS("WriteTag") << "backslash\\returnnewline\r\n\\afterbackslash" << LL_ENDL;
    }
};

namespace tut
{
    template<> template<>
    void ErrorTestObject::test<5>()
        // backslash, return, and newline are not escaped with backslashes
    {
        LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
        setWantsMultiline(true);
        writeMsgNeedsEscaping(); // but should not be now
        ensure_message_field_equals(0, MSG_FIELD, "backslash\\");
        ensure_message_field_equals(1, MSG_FIELD, "newline\nafternewline");
        ensure_message_field_equals(2, MSG_FIELD, "return\rafterreturn");
        ensure_message_field_equals(3, MSG_FIELD, "backslash\\backslash\\");
        ensure_message_field_equals(4, MSG_FIELD, "backslash\\newline\nanothernewline\nafternewline");
        ensure_message_field_equals(5, MSG_FIELD, "backslash\\returnnewline\r\n\\afterbackslash");
        ensure_message_count(6);
    }
}

namespace tut
{
    template<> template<>
        //  class/function information in output
    void ErrorTestObject::test<6>()
    {
        testLogName(mRecorder, logFromGlobal);
        testLogName(mRecorder, logFromStatic);
        testLogName(mRecorder, logFromAnon);
        testLogName(mRecorder, logFromNamespace);
        testLogName(mRecorder, logFromClassWithLogTypeMember, "ClassWithLogType");
        testLogName(mRecorder, logFromClassWithLogTypeStatic, "ClassWithLogType");
    }
}

namespace
{
    std::string innerLogger()
    {
        LL_INFOS() << "inside" << LL_ENDL;
        return "moo";
    }

    std::string outerLogger()
    {
        LL_INFOS() << "outside(" << innerLogger() << ")" << LL_ENDL;
        return "bar";
    }

    class LogWhileLogging
    {
    public:
        void print(std::ostream& out) const
        {
            LL_INFOS() << "logging" << LL_ENDL;
            out << "baz";
        }
    };

    std::ostream& operator<<(std::ostream& out, const LogWhileLogging& l)
        { l.print(out); return out; }

    void metaLogger()
    {
        LogWhileLogging l;
        LL_INFOS() << "meta(" << l << ")" << LL_ENDL;
    }

}

namespace tut
{
    template<> template<>
        // handle nested logging
    void ErrorTestObject::test<7>()
    {
        outerLogger();
        ensure_message_field_equals(0, MSG_FIELD, "inside");
        ensure_message_field_equals(1, MSG_FIELD, "outside(moo)");
        ensure_message_count(2);

        metaLogger();
        ensure_message_field_equals(2, MSG_FIELD, "logging");
        ensure_message_field_equals(3, MSG_FIELD, "meta(baz)");
        ensure_message_count(4);
    }

    template<> template<>
        // special handling of LL_ERRS() calls
    void ErrorTestObject::test<8>()
    {
        std::string location = errorReturningLocation();

        ensure_message_field_equals(0, LOCATION_FIELD, location);
        ensure_message_field_equals(0, MSG_FIELD, "die");
        ensure_message_count(1);

        ensure("fatal callback called", fatalWasCalled);
    }
}

namespace
{
    std::string roswell()
    {
        return "1947-07-08T03:04:05Z";
    }

    void ufoSighting()
    {
        LL_INFOS() << "ufo" << LL_ENDL;
    }
}

namespace tut
{
    template<> template<>
        // time in output (for recorders that need it)
    void ErrorTestObject::test<9>()
    {
        LLError::setTimeFunction(roswell);

        setWantsTime(false);
        ufoSighting();
        ensure_message_field_equals(0, MSG_FIELD, "ufo");
        ensure_message_does_not_contain(0, roswell());

        setWantsTime(true);
        ufoSighting();
        ensure_message_field_equals(1, MSG_FIELD, "ufo");
        ensure_message_field_equals(1, TIME_FIELD, roswell());
    }

    template<> template<>
        // output order
    void ErrorTestObject::test<10>()
    {
        LLError::setTimeFunction(roswell);
        setWantsTime(true);

        std::string location,
                    function;
        writeReturningLocationAndFunction(location, function);

        ensure_equals("order is time level tags location function message",
                      message(0),
                      roswell() + " INFO " + "# " /* no tag */ + location + " " + function + " : " + "apple");
    }

    template<> template<>
        // multiple recorders
    void ErrorTestObject::test<11>()
    {
        LLError::RecorderPtr altRecorder(new TestRecorder());
        LLError::addRecorder(altRecorder);

        LL_INFOS() << "boo" << LL_ENDL;

        ensure_message_field_equals(0, MSG_FIELD, "boo");
        ensure_equals("alt recorder count", std::dynamic_pointer_cast<TestRecorder>(altRecorder)->countMessages(), 1);
        ensure_contains("alt recorder message 0", std::dynamic_pointer_cast<TestRecorder>(altRecorder)->message(0), "boo");

        LLError::setTimeFunction(roswell);

        LLError::RecorderPtr anotherRecorder(new TestRecorder());
        std::dynamic_pointer_cast<TestRecorder>(anotherRecorder)->showTime(true);
        LLError::addRecorder(anotherRecorder);

        LL_INFOS() << "baz" << LL_ENDL;

        std::string when = roswell();

        ensure_message_does_not_contain(1, when);
        ensure_equals("alt recorder count", std::dynamic_pointer_cast<TestRecorder>(altRecorder)->countMessages(), 2);
        ensure_does_not_contain("alt recorder message 1", std::dynamic_pointer_cast<TestRecorder>(altRecorder)->message(1), when);
        ensure_equals("another recorder count", std::dynamic_pointer_cast<TestRecorder>(anotherRecorder)->countMessages(), 1);
        ensure_contains("another recorder message 0", std::dynamic_pointer_cast<TestRecorder>(anotherRecorder)->message(0), when);

        LLError::removeRecorder(altRecorder);
        LLError::removeRecorder(anotherRecorder);
    }
}

class TestAlpha
{
    LOG_CLASS(TestAlpha);
public:
    static void doDebug()   { LL_DEBUGS() << "add dice" << LL_ENDL; }
    static void doInfo()    { LL_INFOS()  << "any idea" << LL_ENDL; }
    static void doWarn()    { LL_WARNS()  << "aim west" << LL_ENDL; }
    static void doError()   { CATCH(LL_ERRS(), "ate eels"); }
    static void doAll() { doDebug(); doInfo(); doWarn(); doError(); }
};

class TestBeta
{
    LOG_CLASS(TestBeta);
public:
    static void doDebug()   { LL_DEBUGS() << "bed down" << LL_ENDL; }
    static void doInfo()    { LL_INFOS()  << "buy iron" << LL_ENDL; }
    static void doWarn()    { LL_WARNS()  << "bad word" << LL_ENDL; }
    static void doError()   { CATCH(LL_ERRS(), "big easy"); }
    static void doAll() { doDebug(); doInfo(); doWarn(); doError(); }
};

namespace tut
{
    template<> template<>
        // filtering by class
    void ErrorTestObject::test<12>()
    {
        LLError::setDefaultLevel(LLError::LEVEL_WARN);
        LLError::setClassLevel("TestBeta", LLError::LEVEL_INFO);

        TestAlpha::doAll();
        TestBeta::doAll();

        ensure_message_field_equals(0, MSG_FIELD, "aim west");
        ensure_message_field_equals(1, MSG_FIELD, "ate eels");
        ensure_message_field_equals(2, MSG_FIELD, "buy iron");
        ensure_message_field_equals(3, MSG_FIELD, "bad word");
        ensure_message_field_equals(4, MSG_FIELD, "big easy");
        ensure_message_count(5);
    }

    template<> template<>
        // filtering by function, and that it will override class filtering
    void ErrorTestObject::test<13>()
    {
        LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
        LLError::setClassLevel("TestBeta", LLError::LEVEL_WARN);
        LLError::setFunctionLevel("TestBeta::doInfo", LLError::LEVEL_DEBUG);
        LLError::setFunctionLevel("TestBeta::doError", LLError::LEVEL_NONE);

        TestBeta::doAll();
        ensure_message_field_equals(0, MSG_FIELD, "buy iron");
        ensure_message_field_equals(1, MSG_FIELD, "bad word");
        ensure_message_count(2);
    }

    template<> template<>
        // filtering by file
        // and that it is overridden by both class and function filtering
    void ErrorTestObject::test<14>()
    {
        LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
        LLError::setFileLevel(LLError::abbreviateFile(__FILE__),
                                    LLError::LEVEL_WARN);
        LLError::setClassLevel("TestAlpha", LLError::LEVEL_INFO);
        LLError::setFunctionLevel("TestAlpha::doError",
                                    LLError::LEVEL_NONE);
        LLError::setFunctionLevel("TestBeta::doError",
                                    LLError::LEVEL_NONE);

        TestAlpha::doAll();
        TestBeta::doAll();
        ensure_message_field_equals(0, MSG_FIELD, "any idea");
        ensure_message_field_equals(1, MSG_FIELD, "aim west");
        ensure_message_field_equals(2, MSG_FIELD, "bad word");
        ensure_message_count(3);
    }

    template<> template<>
        // proper cached, efficient lookup of filtering
    void ErrorTestObject::test<15>()
    {
        LLError::setDefaultLevel(LLError::LEVEL_NONE);

        TestAlpha::doInfo();
        ensure_message_count(0);
        ensure_equals("first check", LLError::shouldLogCallCount(), 1);
        TestAlpha::doInfo();
        ensure_message_count(0);
        ensure_equals("second check", LLError::shouldLogCallCount(), 1);

        LLError::setClassLevel("TestAlpha", LLError::LEVEL_DEBUG);
        TestAlpha::doInfo();
        ensure_message_count(1);
        ensure_equals("third check", LLError::shouldLogCallCount(), 2);
        TestAlpha::doInfo();
        ensure_message_count(2);
        ensure_equals("fourth check", LLError::shouldLogCallCount(), 2);

        LLError::setClassLevel("TestAlpha", LLError::LEVEL_WARN);
        TestAlpha::doInfo();
        ensure_message_count(2);
        ensure_equals("fifth check", LLError::shouldLogCallCount(), 3);
        TestAlpha::doInfo();
        ensure_message_count(2);
        ensure_equals("sixth check", LLError::shouldLogCallCount(), 3);
    }

    template<> template<>
        // configuration from LLSD
    void ErrorTestObject::test<16>()
    {
        LLSD config;
        config["print-location"] = true;
        config["default-level"] = "DEBUG";

        LLSD set1;
        set1["level"] = "WARN";
        set1["files"][0] = LLError::abbreviateFile(__FILE__);

        LLSD set2;
        set2["level"] = "INFO";
        set2["classes"][0] = "TestAlpha";

        LLSD set3;
        set3["level"] = "NONE";
        set3["functions"][0] = "TestAlpha::doError";
        set3["functions"][1] = "TestBeta::doError";

        config["settings"][0] = set1;
        config["settings"][1] = set2;
        config["settings"][2] = set3;

        LLError::configure(config);

        TestAlpha::doAll();
        TestBeta::doAll();
        ensure_message_field_equals(0, MSG_FIELD, "any idea");
        ensure_message_field_equals(1, MSG_FIELD, "aim west");
        ensure_message_field_equals(2, MSG_FIELD, "bad word");
        ensure_message_count(3);

        // make sure reconfiguring works
        LLSD config2;
        config2["default-level"] = "WARN";

        LLError::configure(config2);

        TestAlpha::doAll();
        TestBeta::doAll();
        ensure_message_field_equals(3, MSG_FIELD, "aim west");
        ensure_message_field_equals(4, MSG_FIELD, "ate eels");
        ensure_message_field_equals(5, MSG_FIELD, "bad word");
        ensure_message_field_equals(6, MSG_FIELD, "big easy");
        ensure_message_count(7);
    }
}

namespace tut
{
    template<> template<>
    void ErrorTestObject::test<17>()
        // backslash, return, and newline are escaped with backslashes
    {
        LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
        writeMsgNeedsEscaping();
        ensure_message_field_equals(0, MSG_FIELD, "backslash\\\\");
        ensure_message_field_equals(1, MSG_FIELD, "newline\\nafternewline");
        ensure_message_field_equals(2, MSG_FIELD, "return\\rafterreturn");
        ensure_message_field_equals(3, MSG_FIELD, "backslash\\\\backslash\\\\");
        ensure_message_field_equals(4, MSG_FIELD, "backslash\\\\newline\\nanothernewline\\nafternewline");
        ensure_message_field_equals(5, MSG_FIELD, "backslash\\\\returnnewline\\r\\n\\\\afterbackslash");
        ensure_message_count(6);
    }
}

namespace
{
    std::string writeTagWithSpaceReturningLocation()
    {
        int this_line = __LINE__; CATCH(LL_DEBUGS("Write Tag"), "not allowed");
        return locationString(this_line);
    }
};

namespace tut
{
    template<> template<>
    void ErrorTestObject::test<18>()
        // space character is not allowed in a tag
    {
        LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
        fatalWasCalled = false;

        std::string location = writeTagWithSpaceReturningLocation();
        std::string expected = "Space is not allowed in a log tag at " + location;
        ensure_message_field_equals(0, LEVEL_FIELD, "ERROR");
        ensure_message_field_equals(0, MSG_FIELD, expected);
        ensure("fatal callback called", fatalWasCalled);
    }
}

/* Tests left:
    handling of classes without LOG_CLASS

    live update of filtering from file

    syslog recorder
    file recorder
    cerr/stderr recorder
    fixed buffer recorder
    windows recorder

    mutex use when logging (?)
    strange careful about to crash handling (?)
*/
