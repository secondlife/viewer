/** 
 * @file llerrorcontrol.h
 * @date   December 2006
 * @brief error message system control
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#ifndef LL_LLERRORCONTROL_H
#define LL_LLERRORCONTROL_H

#include "llerror.h"
#include "boost/function.hpp"
#include <string>

class LLSD;

/*
	This is the part of the LLError namespace that manages the messages
	produced by the logging.  The logging support is defined in llerror.h.
	Most files do not need to include this.
	
	These implementations are in llerror.cpp.
*/

// Line buffer interface
class LLLineBuffer
{
public:
	LLLineBuffer() {};
	virtual ~LLLineBuffer() {};

	virtual void clear() = 0; // Clear the buffer, and reset it.

	virtual void addLine(const std::string& utf8line) = 0;
};


namespace LLError
{
	LL_COMMON_API void initForServer(const std::string& identity);
		// resets all logging settings to defaults needed by server processes
		// logs to stderr, syslog, and windows debug log
		// the identity string is used for in the syslog

	LL_COMMON_API void initForApplication(const std::string& dir);
		// resets all logging settings to defaults needed by applicaitons
		// logs to stderr and windows debug log
		// sets up log configuration from the file logcontrol.xml in dir


	/*
		Settings that control what is logged.
		Setting a level means log messages at that level or above.
	*/
	
	LL_COMMON_API void setPrintLocation(bool);
	LL_COMMON_API void setDefaultLevel(LLError::ELevel);
	LL_COMMON_API void setFunctionLevel(const std::string& function_name, LLError::ELevel);
	LL_COMMON_API void setClassLevel(const std::string& class_name, LLError::ELevel);
	LL_COMMON_API void setFileLevel(const std::string& file_name, LLError::ELevel);
	void setTagLevel(const std::string& file_name, LLError::ELevel);
	
	LL_COMMON_API void configure(const LLSD&);
		// the LLSD can configure all of the settings
		// usually read automatically from the live errorlog.xml file


	/*
		Control functions.
	*/

	typedef boost::function<void(const std::string&)> FatalFunction;
	LL_COMMON_API void crashAndLoop(const std::string& message);
		// Default fatal function: access null pointer and loops forever

	LL_COMMON_API void setFatalFunction(const FatalFunction&);
		// The fatal function will be called when an message of LEVEL_ERROR
		// is logged.  Note: supressing a LEVEL_ERROR message from being logged
		// (by, for example, setting a class level to LEVEL_NONE), will keep
		// the that message from causing the fatal funciton to be invoked.

    LL_COMMON_API FatalFunction getFatalFunction();
        // Retrieve the previously-set FatalFunction

    /// temporarily override the FatalFunction for the duration of a
    /// particular scope, e.g. for unit tests
    class LL_COMMON_API OverrideFatalFunction
    {
    public:
        OverrideFatalFunction(const FatalFunction& func):
            mPrev(getFatalFunction())
        {
            setFatalFunction(func);
        }
        ~OverrideFatalFunction()
        {
            setFatalFunction(mPrev);
        }

    private:
        FatalFunction mPrev;
    };

	typedef std::string (*TimeFunction)();
	LL_COMMON_API std::string utcTime();
	
	LL_COMMON_API void setTimeFunction(TimeFunction);
		// The function is use to return the current time, formatted for
		// display by those error recorders that want the time included.



	class LL_COMMON_API Recorder
	{
		// An object that handles the actual output or error messages.
	public:
		virtual ~Recorder();
		
		virtual void recordMessage(LLError::ELevel, const std::string& message) = 0;
			// use the level for better display, not for filtering
			
		virtual bool wantsTime(); // default returns false
			// override and return true if the recorder wants the time string
			// included in the text of the message
	};
	
	LL_COMMON_API void addRecorder(Recorder*);
	LL_COMMON_API void removeRecorder(Recorder*);
		// each error message is passed to each recorder via recordMessage()
	
	LL_COMMON_API void logToFile(const std::string& filename);
	LL_COMMON_API void logToFixedBuffer(LLLineBuffer*);
		// Utilities to add recorders for logging to a file or a fixed buffer
		// A second call to the same function will remove the logger added
		// with the first.
		// Passing the empty string or NULL to just removes any prior.
	LL_COMMON_API std::string logFileName();
		// returns name of current logging file, empty string if none


	/*
		Utilities for use by the unit tests of LLError itself.
	*/

	class Settings;
	LL_COMMON_API Settings* saveAndResetSettings();
	LL_COMMON_API void restoreSettings(Settings *);
		
	LL_COMMON_API std::string abbreviateFile(const std::string& filePath);
	LL_COMMON_API int shouldLogCallCount();
	
};

#endif // LL_LLERRORCONTROL_H

