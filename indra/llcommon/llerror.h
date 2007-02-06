/** 
 * @file llerror.h
 * @date   December 2006
 * @brief error message system
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLERROR_H
#define LL_LLERROR_H

#include <sstream>
#include <typeinfo>

#include "llerrorlegacy.h"


/* Error Logging Facility

	Information for most users:
	
	Code can log messages with constuctions like this:
	
		llinfos << "request to fizzbip agent " << agent_id
			<< " denied due to timeout" << llendl;
		
	Messages can be logged to one of four increasing levels of concern,
	using one of four "streams":

		lldebugs	- debug messages that are normally supressed
		llinfos		- informational messages that are normall shown
		llwarns		- warning messages that singal a problem
		llerrs		- error messages that are major, unrecoverable failures
		
	The later (llerrs) automatically crashes the process after the message
	is logged.
	
	Note that these "streams" are actually #define magic.  Rules for use:
		* they cannot be used as normal streams, only to start a message
		* messages written to them MUST be terminated with llendl
		* between the opening and closing, the << operator is indeed
		  writing onto a std::ostream, so all conversions and stream
		  formating are available
	
	These messages are automatically logged with function name, and (if enabled)
	file and line of the message.  (Note: Existing messages that already include
	the function name don't get name printed twice.)
	
	If you have a class, adding LOG_CLASS line to the declaration will cause
	all messages emitted from member functions (normal and static) to be tagged
	with the proper class name as well as the function name:
	
		class LLFoo
		{
			LOG_CLASS(LLFoo);
		public:
			...
		};
	
		void LLFoo::doSomething(int i)
		{
			if (i > 100)
			{
				llwanrs << "called with a big value for i: " << i << llendl; 
			}
			...
		}
	
	will result in messages like:
	
		WARN: LLFoo::doSomething: called with a big value for i: 283
	
	Which messages are logged and which are supressed can be controled at run
	time from the live file logcontrol.xml based on function, class and/or 
	source file.  See etc/logcontrol-dev.xml for details.
	
	Lastly, logging is now very efficient in both compiled code and execution
	when skipped.  There is no need to wrap messages, even debugging ones, in
	#ifdef _DEBUG constructs.  lldebugs messages are compiled into all builds,
	even release.  Which means you can use them to help debug even when deployed
	to a real grid.
*/

namespace LLError
{
	enum ELevel
	{
		LEVEL_ALL = 0,
			// used to indicate that all messagess should be logged
			
		LEVEL_DEBUG = 0,
		LEVEL_INFO = 1,
		LEVEL_WARN = 2,
		LEVEL_ERROR = 3,	// used to be called FATAL
		
		LEVEL_NONE = 4
			// not really a level
			// used to indicate that no messages should be logged
	};
	
	/*	Macro support
		The classes CallSite and Log are used by the logging macros below.
		They are not intended for general use.
	*/
	
	class CallSite;
	
	class Log
	{
	public:
		static bool shouldLog(CallSite&);
		static std::ostringstream* out();
		static void flush(std::ostringstream*, const CallSite&);
	};
	
	class CallSite
	{
		// Represents a specific place in the code where a message is logged
		// This is public because it is used by the macros below.  It is not
		// intended for public use.
	public:
		CallSite(ELevel, const char* file, int line,
				const std::type_info& class_info, const char* function);
						
		bool shouldLog()
			{ return mCached ? mShouldLog : Log::shouldLog(*this); }
			// this member function needs to be in-line for efficiency
		
		void invalidate();
		
	private:
		// these describe the call site and never change
		const ELevel			mLevel;
		const char* const		mFile;
		const int				mLine;
		const std::type_info&	mClassInfo;
		const char* const		mFunction;
		
		// these implement a cache of the call to shouldLog()
		bool mCached;
		bool mShouldLog;
		
		friend class Log;
	};
	
	
	class End { };
	inline std::ostream& operator<<(std::ostream& s, const End&)
		{ return s; }
		// used to indicate the end of a message
		
	class NoClassInfo { };
		// used to indicate no class info known for logging
}



/*
	Class type information for logging
 */

#define LOG_CLASS(s)	typedef s _LL_CLASS_TO_LOG
	// Declares class to tag logged messages with.
	// See top of file for example of how to use this
	
typedef LLError::NoClassInfo _LL_CLASS_TO_LOG;
	// Outside a class declartion, or in class without LOG_CLASS(), this
	// typedef causes the messages to not be associated with any class.





/*
	Error Logging Macros
	See top of file for common usage.	
*/

#define lllog(level) \
	{ \
		static LLError::CallSite _site( \
			level, __FILE__, __LINE__, typeid(_LL_CLASS_TO_LOG), __FUNCTION__);\
		if (_site.shouldLog()) \
		{ \
			std::ostringstream* _out = LLError::Log::out(); \
			(*_out)
	
#define llendl \
			LLError::End(); \
			LLError::Log::flush(_out, _site); \
		} \
	}

#define llinfos		lllog(LLError::LEVEL_INFO)
#define lldebugs	lllog(LLError::LEVEL_DEBUG)
#define llwarns		lllog(LLError::LEVEL_WARN)
#define llerrs		lllog(LLError::LEVEL_ERROR)

#define llcont		(*_out)
	/*
		Use this construct if you need to do computation in the middle of a
		message:
		
			llinfos << "the agent " << agend_id;
			switch (f)
			{
				case FOP_SHRUGS:	llcont << "shrugs";				break;
				case FOP_TAPS:		llcont << "points at " << who;	break;
				case FOP_SAYS:		llcont << "says " << message;	break;
			}
			llcont << " for " << t << " seconds" << llendl;
		
		Such computation is done iff the message will be logged.
	*/


#endif // LL_LLERROR_H
