/** 
 * @file llerror.h
 * @date   December 2006
 * @brief error message system
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLERROR_H
#define LL_LLERROR_H

#include <sstream>
#include <typeinfo>

#include "llerrorlegacy.h"
#include "stdtypes.h"


/* Error Logging Facility

	Information for most users:
	
	Code can log messages with constuctions like this:
	
		LL_INFOS("StringTag") << "request to fizzbip agent " << agent_id
			<< " denied due to timeout" << LL_ENDL;
		
	Messages can be logged to one of four increasing levels of concern,
	using one of four "streams":

		LL_DEBUGS("StringTag")	- debug messages that are normally supressed
		LL_INFOS("StringTag")	- informational messages that are normall shown
		LL_WARNS("StringTag")	- warning messages that singal a problem
		LL_ERRS("StringTag")	- error messages that are major, unrecoverable failures
		
	The later (LL_ERRS("StringTag")) automatically crashes the process after the message
	is logged.
	
	Note that these "streams" are actually #define magic.  Rules for use:
		* they cannot be used as normal streams, only to start a message
		* messages written to them MUST be terminated with LL_ENDL
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
				LL_WARNS("FooBarTag") << "called with a big value for i: " << i << LL_ENDL; 
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
	#ifdef _DEBUG constructs.  LL_DEBUGS("StringTag") messages are compiled into all builds,
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
	
	class LL_COMMON_API Log
	{
	public:
		static bool shouldLog(CallSite&);
		static std::ostringstream* out();
		static void flush(std::ostringstream* out, char* message)  ;
		static void flush(std::ostringstream*, const CallSite&);
	};
	
	class LL_COMMON_API CallSite
	{
		// Represents a specific place in the code where a message is logged
		// This is public because it is used by the macros below.  It is not
		// intended for public use.
	public:
		CallSite(ELevel, const char* file, int line,
				const std::type_info& class_info, const char* function, const char* broadTag, const char* narrowTag, bool printOnce);
						
		bool shouldLog()
			{ return mCached ? mShouldLog : Log::shouldLog(*this); }
			// this member function needs to be in-line for efficiency
		
		void invalidate();
		
	private:
		// these describe the call site and never change
		const ELevel			mLevel;
		const char* const		mFile;
		const int			mLine;
		const std::type_info&   	mClassInfo;
		const char* const		mFunction;
		const char* const		mBroadTag;
		const char* const		mNarrowTag;
		const bool			mPrintOnce;
		
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

   //LLCallStacks keeps track of call stacks and output the call stacks to log file
   //when LLAppViewer::handleViewerCrash() is triggered.
   //
   //Note: to be simple, efficient and necessary to keep track of correct call stacks, 
	//LLCallStacks is designed not to be thread-safe.
   //so try not to use it in multiple parallel threads at same time.
   //Used in a single thread at a time is fine.
   class LL_COMMON_API LLCallStacks
   {
   private:
       static char**  sBuffer ;
	   static S32     sIndex ;
          
   public:   
	   static void push(const char* function, const int line) ;
	   static std::ostringstream* insert(const char* function, const int line) ;
       static void print() ;
       static void clear() ;
	   static void end(std::ostringstream* _out) ;
   }; 
}

//this is cheaper than llcallstacks if no need to output other variables to call stacks. 
#define llpushcallstacks LLError::LLCallStacks::push(__FUNCTION__, __LINE__)
#define llcallstacks \
	{\
       std::ostringstream* _out = LLError::LLCallStacks::insert(__FUNCTION__, __LINE__) ; \
       (*_out)
#define llcallstacksendl \
		LLError::End(); \
		LLError::LLCallStacks::end(_out) ; \
	}
#define llclearcallstacks LLError::LLCallStacks::clear()
#define llprintcallstacks LLError::LLCallStacks::print() 

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

#define lllog(level, broadTag, narrowTag, once) \
	do { \
		static LLError::CallSite _site( \
			level, __FILE__, __LINE__, typeid(_LL_CLASS_TO_LOG), __FUNCTION__, broadTag, narrowTag, once);\
		if (LL_UNLIKELY(_site.shouldLog()))			\
		{ \
			std::ostringstream* _out = LLError::Log::out(); \
			(*_out)

// DEPRECATED: Don't call directly, use LL_ENDL instead, which actually looks like a macro
#define llendl \
			LLError::End(); \
			LLError::Log::flush(_out, _site); \
		} \
	} while(0)

// DEPRECATED: Use the new macros that allow tags and *look* like macros.
#define lldebugs	lllog(LLError::LEVEL_DEBUG, NULL, NULL, false)
#define llinfos		lllog(LLError::LEVEL_INFO, NULL, NULL, false)
#define llwarns		lllog(LLError::LEVEL_WARN, NULL, NULL, false)
#define llerrs		lllog(LLError::LEVEL_ERROR, NULL, NULL, false)
#define llcont		(*_out)

// NEW Macros for debugging, allow the passing of a string tag

// One Tag
#define LL_DEBUGS(broadTag)	lllog(LLError::LEVEL_DEBUG, broadTag, NULL, false)
#define LL_INFOS(broadTag)	lllog(LLError::LEVEL_INFO, broadTag, NULL, false)
#define LL_WARNS(broadTag)	lllog(LLError::LEVEL_WARN, broadTag, NULL, false)
#define LL_ERRS(broadTag)	lllog(LLError::LEVEL_ERROR, broadTag, NULL, false)
// Two Tags
#define LL_DEBUGS2(broadTag, narrowTag)	lllog(LLError::LEVEL_DEBUG, broadTag, narrowTag, false)
#define LL_INFOS2(broadTag, narrowTag)	lllog(LLError::LEVEL_INFO, broadTag, narrowTag, false)
#define LL_WARNS2(broadTag, narrowTag)	lllog(LLError::LEVEL_WARN, broadTag, narrowTag, false)
#define LL_ERRS2(broadTag, narrowTag)	lllog(LLError::LEVEL_ERROR, broadTag, narrowTag, false)

// Only print the log message once (good for warnings or infos that would otherwise
// spam the log file over and over, such as tighter loops).
#define LL_DEBUGS_ONCE(broadTag)	lllog(LLError::LEVEL_DEBUG, broadTag, NULL, true)
#define LL_INFOS_ONCE(broadTag)	lllog(LLError::LEVEL_INFO, broadTag, NULL, true)
#define LL_WARNS_ONCE(broadTag)	lllog(LLError::LEVEL_WARN, broadTag, NULL, true)
#define LL_DEBUGS2_ONCE(broadTag, narrowTag)	lllog(LLError::LEVEL_DEBUG, broadTag, narrowTag, true)
#define LL_INFOS2_ONCE(broadTag, narrowTag)	lllog(LLError::LEVEL_INFO, broadTag, narrowTag, true)
#define LL_WARNS2_ONCE(broadTag, narrowTag)	lllog(LLError::LEVEL_WARN, broadTag, narrowTag, true)

#define LL_ENDL llendl
#define LL_CONT	(*_out)

	/*
		Use this construct if you need to do computation in the middle of a
		message:
		
			LL_INFOS("AgentGesture") << "the agent " << agend_id;
			switch (f)
			{
				case FOP_SHRUGS:	LL_CONT << "shrugs";				break;
				case FOP_TAPS:		LL_CONT << "points at " << who;	break;
				case FOP_SAYS:		LL_CONT << "says " << message;	break;
			}
			LL_CONT << " for " << t << " seconds" << LL_ENDL;
		
		Such computation is done iff the message will be logged.
	*/


#endif // LL_LLERROR_H
