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

#include "linden_common.h"

#include "../llerror.h"

#include "../llerrorcontrol.h"
#include "../llsd.h"

#include "../test/lltut.h"

namespace
{
	void test_that_error_h_includes_enough_things_to_compile_a_message()
	{
		llinfos << "!" << llendl;
	}
}

namespace
{
	static bool fatalWasCalled;
	void fatalCall(const std::string&) { fatalWasCalled = true; }
}

namespace tut
{
	class TestRecorder : public LLError::Recorder
	{
	public:
		TestRecorder() : mWantsTime(false) { }
		~TestRecorder() { LLError::removeRecorder(this); }

		void recordMessage(LLError::ELevel level,
						   const std::string& message)
		{
			mMessages.push_back(message);
		}

		int countMessages()			{ return (int) mMessages.size(); }
		void clearMessages()		{ mMessages.clear(); }

		void setWantsTime(bool t)	{ mWantsTime = t; }
		bool wantsTime()			{ return mWantsTime; }

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

		bool mWantsTime;
	};

	struct ErrorTestData
	{
		// addRecorder() expects to be able to later delete the passed
		// Recorder*. Even though removeRecorder() reclaims ownership, passing
		// a pointer to a data member rather than a heap Recorder subclass
		// instance would just be Wrong.
		TestRecorder* mRecorder;
		LLError::Settings* mPriorErrorSettings;

		ErrorTestData():
			mRecorder(new TestRecorder)
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
			delete mRecorder;
			LLError::restoreSettings(mPriorErrorSettings);
		}

		void ensure_message_count(int expectedCount)
		{
			ensure_equals("message count", mRecorder->countMessages(), expectedCount);
		}

		void ensure_message_contains(int n, const std::string& expectedText)
		{
			std::ostringstream test_name;
			test_name << "testing message " << n;

			ensure_contains(test_name.str(), mRecorder->message(n), expectedText);
		}

		void ensure_message_does_not_contain(int n, const std::string& expectedText)
		{
			std::ostringstream test_name;
			test_name << "testing message " << n;

			ensure_does_not_contain(test_name.str(), mRecorder->message(n), expectedText);
		}
	};

	typedef test_group<ErrorTestData>	ErrorTestGroup;
	typedef ErrorTestGroup::object		ErrorTestObject;

	ErrorTestGroup errorTestGroup("error");

	template<> template<>
	void ErrorTestObject::test<1>()
		// basic test of output
	{
		llinfos << "test" << llendl;
		llinfos << "bob" << llendl;

		ensure_message_contains(0, "test");
		ensure_message_contains(1, "bob");
	}
}

namespace
{
	void writeSome()
	{
		lldebugs << "one" << llendl;
		llinfos << "two" << llendl;
		llwarns << "three" << llendl;
		llerrs << "four" << llendl;
			// fatal messages write out and addtional "error" message
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
		ensure_message_contains(0, "one");
		ensure_message_contains(1, "two");
		ensure_message_contains(2, "three");
		ensure_message_contains(3, "error");
		ensure_message_contains(4, "four");
		ensure_message_count(5);

		LLError::setDefaultLevel(LLError::LEVEL_INFO);
		writeSome();
		ensure_message_contains(5, "two");
		ensure_message_contains(6, "three");
		ensure_message_contains(7, "error");
		ensure_message_contains(8, "four");
		ensure_message_count(9);

		LLError::setDefaultLevel(LLError::LEVEL_WARN);
		writeSome();
		ensure_message_contains(9, "three");
		ensure_message_contains(10, "error");
		ensure_message_contains(11, "four");
		ensure_message_count(12);

		LLError::setDefaultLevel(LLError::LEVEL_ERROR);
		writeSome();
		ensure_message_contains(12, "error");
		ensure_message_contains(13, "four");
		ensure_message_count(14);

		LLError::setDefaultLevel(LLError::LEVEL_NONE);
		writeSome();
		ensure_message_count(14);
	}

	template<> template<>
	void ErrorTestObject::test<3>()
		// error type string in output
	{
		writeSome();
		ensure_message_contains(0, "DEBUG: ");
		ensure_message_contains(1, "INFO: ");
		ensure_message_contains(2, "WARNING: ");
		ensure_message_does_not_contain(3, "ERROR");
		ensure_message_contains(4, "ERROR: ");
		ensure_message_count(5);
	}

	template<> template<>
	void ErrorTestObject::test<4>()
		// file abbreviation
	{
		std::string thisFile = __FILE__;
		std::string abbreviateFile = LLError::abbreviateFile(thisFile);

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
				 << "(" << line << ") : ";

		return location.str();
	}

	std::string writeReturningLocation()
	{
		llinfos << "apple" << llendl;	int this_line = __LINE__;
		return locationString(this_line);
	}

	std::string writeReturningLocationAndFunction()
	{
		llinfos << "apple" << llendl;	int this_line = __LINE__;
		return locationString(this_line) + __FUNCTION__;
	}

	std::string errorReturningLocation()
	{
		llerrs << "die" << llendl;	int this_line = __LINE__;
		return locationString(this_line);
	}
}

namespace tut
{
	template<> template<>
	void ErrorTestObject::test<5>()
		// file and line information in log messages
	{
		std::string location = writeReturningLocation();
			// expecting default to not print location information

		LLError::setPrintLocation(true);
		writeReturningLocation();

		LLError::setPrintLocation(false);
		writeReturningLocation();

		ensure_message_does_not_contain(0, location);
		ensure_message_contains(1, location);
		ensure_message_does_not_contain(2, location);
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
	llinfos << (id ? "logFromGlobal: " : "") << "hi" << llendl;
	return "logFromGlobal";
}

static std::string logFromStatic(bool id)
{
	llinfos << (id ? "logFromStatic: " : "") << "hi" << llendl;
	return "logFromStatic";
}

namespace
{
	std::string logFromAnon(bool id)
	{
		llinfos << (id ? "logFromAnon: " : "") << "hi" << llendl;
		return "logFromAnon";
	}
}

namespace Foo {
	std::string logFromNamespace(bool id)
	{
		llinfos << (id ? "Foo::logFromNamespace: " : "") << "hi" << llendl;
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
			llinfos << (id ? "ClassWithNoLogType::logFromMember: " : "") << "hi" << llendl;
			return "ClassWithNoLogType::logFromMember";
		}
		static std::string logFromStatic(bool id)
		{
			llinfos << (id ? "ClassWithNoLogType::logFromStatic: " : "") << "hi" << llendl;
			return "ClassWithNoLogType::logFromStatic";
		}
	};

	class ClassWithLogType {
		LOG_CLASS(ClassWithLogType);
	public:
		std::string logFromMember(bool id)
		{
			llinfos << (id ? "ClassWithLogType::logFromMember: " : "") << "hi" << llendl;
			return "ClassWithLogType::logFromMember";
		}
		static std::string logFromStatic(bool id)
		{
			llinfos << (id ? "ClassWithLogType::logFromStatic: " : "") << "hi" << llendl;
			return "ClassWithLogType::logFromStatic";
		}
	};

	std::string logFromNamespace(bool id) { return Foo::logFromNamespace(id); }
	std::string logFromClassWithNoLogTypeMember(bool id) { ClassWithNoLogType c; return c.logFromMember(id); }
	std::string logFromClassWithNoLogTypeStatic(bool id) { return ClassWithNoLogType::logFromStatic(id); }
	std::string logFromClassWithLogTypeMember(bool id) { ClassWithLogType c; return c.logFromMember(id); }
	std::string logFromClassWithLogTypeStatic(bool id) { return ClassWithLogType::logFromStatic(id); }

	void ensure_has(const std::string& message,
		const std::string& actual, const std::string& expected)
	{
		std::string::size_type n1 = actual.find(expected);
		if (n1 == std::string::npos)
		{
			std::stringstream ss;
			ss << message << ": " << "expected to find a copy of " << expected
				<< " in actual " << actual;
			throw tut::failure(ss.str().c_str());
		}
	}

	typedef std::string (*LogFromFunction)(bool);
	void testLogName(tut::TestRecorder* recorder, LogFromFunction f,
		const std::string& class_name = "")
	{
		recorder->clearMessages();
		std::string name = f(false);
		f(true);

		std::string messageWithoutName = recorder->message(0);
		std::string messageWithName = recorder->message(1);

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

namespace tut
{
	template<> template<>
		// 	class/function information in output
	void ErrorTestObject::test<6>()
	{
		testLogName(mRecorder, logFromGlobal);
		testLogName(mRecorder, logFromStatic);
		testLogName(mRecorder, logFromAnon);
		testLogName(mRecorder, logFromNamespace);
		//testLogName(mRecorder, logFromClassWithNoLogTypeMember, "ClassWithNoLogType");
		//testLogName(mRecorder, logFromClassWithNoLogTypeStatic, "ClassWithNoLogType");
			// XXX: figure out what the exepcted response is for these
		testLogName(mRecorder, logFromClassWithLogTypeMember, "ClassWithLogType");
		testLogName(mRecorder, logFromClassWithLogTypeStatic, "ClassWithLogType");
	}
}

namespace
{
	std::string innerLogger()
	{
		llinfos << "inside" << llendl;
		return "moo";
	}

	std::string outerLogger()
	{
		llinfos << "outside(" << innerLogger() << ")" << llendl;
		return "bar";
	}

	void uberLogger()
	{
		llinfos << "uber(" << outerLogger() << "," << innerLogger() << ")" << llendl;
	}

	class LogWhileLogging
	{
	public:
		void print(std::ostream& out) const
		{
			llinfos << "logging" << llendl;
			out << "baz";
		}
	};

	std::ostream& operator<<(std::ostream& out, const LogWhileLogging& l)
		{ l.print(out); return out; }

	void metaLogger()
	{
		LogWhileLogging l;
		llinfos << "meta(" << l << ")" << llendl;
	}

}

namespace tut
{
	template<> template<>
		// handle nested logging
	void ErrorTestObject::test<7>()
	{
		outerLogger();
		ensure_message_contains(0, "inside");
		ensure_message_contains(1, "outside(moo)");
		ensure_message_count(2);

		uberLogger();
		ensure_message_contains(2, "inside");
		ensure_message_contains(3, "inside");
		ensure_message_contains(4, "outside(moo)");
		ensure_message_contains(5, "uber(bar,moo)");
		ensure_message_count(6);

		metaLogger();
		ensure_message_contains(6, "logging");
		ensure_message_contains(7, "meta(baz)");
		ensure_message_count(8);
	}

	template<> template<>
		// special handling of llerrs calls
	void ErrorTestObject::test<8>()
	{
		LLError::setPrintLocation(false);
		std::string location = errorReturningLocation();

		ensure_message_contains(0, location + "error");
		ensure_message_contains(1, "die");
		ensure_message_count(2);

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
		llinfos << "ufo" << llendl;
	}
}

namespace tut
{
	template<> template<>
		// time in output (for recorders that need it)
	void ErrorTestObject::test<9>()
	{
		LLError::setTimeFunction(roswell);

		mRecorder->setWantsTime(false);
		ufoSighting();
		ensure_message_contains(0, "ufo");
		ensure_message_does_not_contain(0, roswell());

		mRecorder->setWantsTime(true);
		ufoSighting();
		ensure_message_contains(1, "ufo");
		ensure_message_contains(1, roswell());
	}

	template<> template<>
		// output order
	void ErrorTestObject::test<10>()
	{
		LLError::setPrintLocation(true);
		LLError::setTimeFunction(roswell);
		mRecorder->setWantsTime(true);
		std::string locationAndFunction = writeReturningLocationAndFunction();

		ensure_equals("order is time type location function message",
			mRecorder->message(0),
			roswell() + " INFO: " + locationAndFunction + ": apple");
	}

	template<> template<>
		// multiple recorders
	void ErrorTestObject::test<11>()
	{
		TestRecorder* altRecorder(new TestRecorder);
		LLError::addRecorder(altRecorder);

		llinfos << "boo" << llendl;

		ensure_message_contains(0, "boo");
		ensure_equals("alt recorder count", altRecorder->countMessages(), 1);
		ensure_contains("alt recorder message 0", altRecorder->message(0), "boo");

		LLError::setTimeFunction(roswell);

		TestRecorder* anotherRecorder(new TestRecorder);
		anotherRecorder->setWantsTime(true);
		LLError::addRecorder(anotherRecorder);

		llinfos << "baz" << llendl;

		std::string when = roswell();

		ensure_message_does_not_contain(1, when);
		ensure_equals("alt recorder count", altRecorder->countMessages(), 2);
		ensure_does_not_contain("alt recorder message 1", altRecorder->message(1), when);
		ensure_equals("another recorder count", anotherRecorder->countMessages(), 1);
		ensure_contains("another recorder message 0", anotherRecorder->message(0), when);
	}
}

class TestAlpha
{
	LOG_CLASS(TestAlpha);
public:
	static void doDebug()	{ lldebugs << "add dice" << llendl; }
	static void doInfo()	{ llinfos  << "any idea" << llendl; }
	static void doWarn()	{ llwarns  << "aim west" << llendl; }
	static void doError()	{ llerrs   << "ate eels" << llendl; }
	static void doAll() { doDebug(); doInfo(); doWarn(); doError(); }
};

class TestBeta
{
	LOG_CLASS(TestBeta);
public:
	static void doDebug()	{ lldebugs << "bed down" << llendl; }
	static void doInfo()	{ llinfos  << "buy iron" << llendl; }
	static void doWarn()	{ llwarns  << "bad word" << llendl; }
	static void doError()	{ llerrs   << "big easy" << llendl; }
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

		ensure_message_contains(0, "aim west");
		ensure_message_contains(1, "error");
		ensure_message_contains(2, "ate eels");
		ensure_message_contains(3, "buy iron");
		ensure_message_contains(4, "bad word");
		ensure_message_contains(5, "error");
		ensure_message_contains(6, "big easy");
		ensure_message_count(7);
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
		ensure_message_contains(0, "buy iron");
		ensure_message_contains(1, "bad word");
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
		ensure_message_contains(0, "any idea");
		ensure_message_contains(1, "aim west");
		ensure_message_contains(2, "bad word");
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
		std::string this_file = LLError::abbreviateFile(__FILE__);
		LLSD config;
		config["print-location"] = true;
		config["default-level"] = "DEBUG";

		LLSD set1;
		set1["level"] = "WARN";
		set1["files"][0] = this_file;

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
		ensure_message_contains(0, "any idea");
		ensure_message_contains(0, this_file);
		ensure_message_contains(1, "aim west");
		ensure_message_contains(2, "bad word");
		ensure_message_count(3);

		// make sure reconfiguring works
		LLSD config2;
		config2["default-level"] = "WARN";

		LLError::configure(config2);

		TestAlpha::doAll();
		TestBeta::doAll();
		ensure_message_contains(3, "aim west");
		ensure_message_does_not_contain(3, this_file);
		ensure_message_contains(4, "error");
		ensure_message_contains(5, "ate eels");
		ensure_message_contains(6, "bad word");
		ensure_message_contains(7, "error");
		ensure_message_contains(8, "big easy");
		ensure_message_count(9);
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
