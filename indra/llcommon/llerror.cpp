/** 
 * @file llerror.cpp
 * @date   December 2006
 * @brief error message system
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

#include "linden_common.h"

#include "llerror.h"
#include "llerrorcontrol.h"

#include <cctype>
#ifdef __GNUC__
# include <cxxabi.h>
#endif // __GNUC__
#include <sstream>
#if !LL_WINDOWS
# include <syslog.h>
# include <unistd.h>
#endif // !LL_WINDOWS
#include <vector>

#include "llapp.h"
#include "llapr.h"
#include "llfile.h"
#include "lllivefile.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llstl.h"
#include "lltimer.h"

namespace {
#if !LL_WINDOWS
	class RecordToSyslog : public LLError::Recorder
	{
	public:
		RecordToSyslog(const std::string& identity)
			: mIdentity(identity)
		{
			openlog(mIdentity.c_str(), LOG_CONS|LOG_PID, LOG_LOCAL0);
				// we need to set the string from a local copy of the string
				// since apparanetly openlog expects the const char* to remain
				// valid even after it returns (presumably until closelog)
		}
		
		~RecordToSyslog()
		{
			closelog();
		}
		
		virtual void recordMessage(LLError::ELevel level,
									const std::string& message)
		{
			int syslogPriority = LOG_CRIT;
			switch (level) {
				case LLError::LEVEL_DEBUG:	syslogPriority = LOG_DEBUG;	break;
				case LLError::LEVEL_INFO:	syslogPriority = LOG_INFO;	break;
				case LLError::LEVEL_WARN:	syslogPriority = LOG_WARNING; break;
				case LLError::LEVEL_ERROR:	syslogPriority = LOG_CRIT;	break;
				default:					syslogPriority = LOG_CRIT;
			}
			
			syslog(syslogPriority, "%s", message.c_str());
		}
	private:
		std::string mIdentity;
	};
#endif

	class RecordToFile : public LLError::Recorder
	{
	public:
		RecordToFile(const std::string& filename)
		{
			mFile.open(filename, llofstream::out | llofstream::app);
			if (!mFile)
			{
				llinfos << "Error setting log file to " << filename << llendl;
			}
		}
		
		~RecordToFile()
		{
			mFile.close();
		}
		
		bool okay() { return mFile; }
		
		virtual bool wantsTime() { return true; }
		
		virtual void recordMessage(LLError::ELevel level,
									const std::string& message)
		{
			mFile << message << std::endl;
			// mFile.flush();
				// *FIX: should we do this? 
		}
	
	private:
		llofstream mFile;
	};
	
	
	class RecordToStderr : public LLError::Recorder
	{
	public:
		RecordToStderr(bool timestamp) : mTimestamp(timestamp), mUseANSI(ANSI_PROBE) { }

		virtual bool wantsTime() { return mTimestamp; }
		
		virtual void recordMessage(LLError::ELevel level,
					   const std::string& message)
		{
			if (ANSI_PROBE == mUseANSI)
				mUseANSI = (checkANSI() ? ANSI_YES : ANSI_NO);

			if (ANSI_YES == mUseANSI)
			{
				// Default all message levels to bold so we can distinguish our own messages from those dumped by subprocesses and libraries.
				colorANSI("1"); // bold
				switch (level) {
				case LLError::LEVEL_ERROR:
					colorANSI("31"); // red
					break;
				case LLError::LEVEL_WARN:
					colorANSI("34"); // blue
					break;
				case LLError::LEVEL_DEBUG:
					colorANSI("35"); // magenta
					break;
				default:
					break;
				}
			}
			fprintf(stderr, "%s\n", message.c_str());
			if (ANSI_YES == mUseANSI) colorANSI("0"); // reset
		}
	
	private:
		bool mTimestamp;
		enum ANSIState {ANSI_PROBE, ANSI_YES, ANSI_NO};
		ANSIState mUseANSI;
		void colorANSI(const std::string color)
		{
			// ANSI color code escape sequence
			fprintf(stderr, "\033[%sm", color.c_str() );
		};
		bool checkANSI(void)
		{
#if LL_LINUX || LL_DARWIN
			// Check whether it's okay to use ANSI; if stderr is
			// a tty then we assume yes.  Can be turned off with
			// the LL_NO_ANSI_COLOR env var.
			return (0 != isatty(2)) &&
				(NULL == getenv("LL_NO_ANSI_COLOR"));
#endif // LL_LINUX
			return false;
		};
	};

	class RecordToFixedBuffer : public LLError::Recorder
	{
	public:
		RecordToFixedBuffer(LLLineBuffer* buffer) : mBuffer(buffer) { }
		
		virtual void recordMessage(LLError::ELevel level,
								   const std::string& message)
		{
			mBuffer->addLine(message);
		}
	
	private:
		LLLineBuffer* mBuffer;
	};

#if LL_WINDOWS
	class RecordToWinDebug: public LLError::Recorder
	{
	public:
		virtual void recordMessage(LLError::ELevel level,
								   const std::string& message)
		{
			llutf16string utf16str =
				wstring_to_utf16str(utf8str_to_wstring(message));
			utf16str += '\n';
			OutputDebugString(utf16str.c_str());
		}
	};
#endif
}


namespace
{
	std::string className(const std::type_info& type)
	{
#ifdef __GNUC__
		// GCC: type_info::name() returns a mangled class name, must demangle

		static size_t abi_name_len = 100;
		static char* abi_name_buf = (char*)malloc(abi_name_len);
			// warning: above is voodoo inferred from the GCC manual,
			// do NOT change

		int status;
			// We don't use status, and shouldn't have to pass apointer to it
			// but gcc 3.3 libstc++'s implementation of demangling is broken
			// and fails without.
			
		char* name = abi::__cxa_demangle(type.name(),
										abi_name_buf, &abi_name_len, &status);
			// this call can realloc the abi_name_buf pointer (!)

		return name ? name : type.name();

#elif LL_WINDOWS
		// DevStudio: type_info::name() includes the text "class " at the start

		static const std::string class_prefix = "class ";

		std::string name = type.name();
		std::string::size_type p = name.find(class_prefix);
		if (p == std::string::npos)
		{
			return name;
		}

		return name.substr(p + class_prefix.size());

#else		
		return type.name();
#endif
	}

	std::string functionName(const std::string& preprocessor_name)
	{
#if LL_WINDOWS
		// DevStudio: the __FUNCTION__ macro string includes
		// the type and/or namespace prefixes

		std::string::size_type p = preprocessor_name.rfind(':');
		if (p == std::string::npos)
		{
			return preprocessor_name;
		}
		return preprocessor_name.substr(p + 1);

#else
		return preprocessor_name;
#endif
	}


	class LogControlFile : public LLLiveFile
	{
		LOG_CLASS(LogControlFile);
	
	public:
		static LogControlFile& fromDirectory(const std::string& dir);
		
		virtual bool loadFile();
		
	private:
		LogControlFile(const std::string &filename)
			: LLLiveFile(filename)
			{ }
	};

	LogControlFile& LogControlFile::fromDirectory(const std::string& dir)
	{
		std::string dirBase = dir + "/";
			// NB: We have no abstraction in llcommon  for the "proper"
			// delimiter but it turns out that "/" works on all three platforms
			
		std::string file = dirBase + "logcontrol-dev.xml";
		
		llstat stat_info;
		if (LLFile::stat(file, &stat_info)) {
			// NB: stat returns non-zero if it can't read the file, for example
			// if it doesn't exist.  LLFile has no better abstraction for 
			// testing for file existence.
			
			file = dirBase + "logcontrol.xml";
		}
		return * new LogControlFile(file);
			// NB: This instance is never freed
	}
	
	bool LogControlFile::loadFile()
	{
		LLSD configuration;

		{
			llifstream file(filename());
			if (file.is_open())
			{
				LLSDSerialize::fromXML(configuration, file);
			}

			if (configuration.isUndefined())
			{
				llwarns << filename() << " missing, ill-formed,"
							" or simply undefined; not changing configuration"
						<< llendl;
				return false;
			}
		}
		
		LLError::configure(configuration);
		llinfos << "logging reconfigured from " << filename() << llendl;
		return true;
	}


	typedef std::map<std::string, LLError::ELevel> LevelMap;
	typedef std::vector<LLError::Recorder*> Recorders;
	typedef std::vector<LLError::CallSite*> CallSiteVector;

	class Globals
	{
	public:
		std::ostringstream messageStream;
		bool messageStreamInUse;

		void addCallSite(LLError::CallSite&);
		void invalidateCallSites();
		
		static Globals& get();
			// return the one instance of the globals

	private:
		CallSiteVector callSites;

		Globals()
			:	messageStreamInUse(false)
			{ }
		
	};

	void Globals::addCallSite(LLError::CallSite& site)
	{
		callSites.push_back(&site);
	}
	
	void Globals::invalidateCallSites()
	{
		for (CallSiteVector::const_iterator i = callSites.begin();
			 i != callSites.end();
			 ++i)
		{
			(*i)->invalidate();
		}
		
		callSites.clear();
	}

	Globals& Globals::get()
	{
		/* This pattern, of returning a reference to a static function
		   variable, is to ensure that this global is constructed before
		   it is used, no matter what the global initialization sequence
		   is.
		   See C++ FAQ Lite, sections 10.12 through 10.14
		*/
		static Globals* globals = new Globals;		
		return *globals;
	}
}

namespace LLError
{
	class Settings
	{
	public:
		bool printLocation;

		LLError::ELevel defaultLevel;
		
		LevelMap functionLevelMap;
		LevelMap classLevelMap;
		LevelMap fileLevelMap;
		LevelMap tagLevelMap;
		std::map<std::string, unsigned int> uniqueLogMessages;
		
		LLError::FatalFunction crashFunction;
		LLError::TimeFunction timeFunction;
		
		Recorders recorders;
		Recorder* fileRecorder;
		Recorder* fixedBufferRecorder;
		std::string fileRecorderFileName;
		
		int shouldLogCallCounter;
		
		static Settings& get();
	
		static void reset();
		static Settings* saveAndReset();
		static void restore(Settings*);
		
	private:
		Settings()
			:	printLocation(false),
				defaultLevel(LLError::LEVEL_DEBUG),
				crashFunction(),
				timeFunction(NULL),
				fileRecorder(NULL),
				fixedBufferRecorder(NULL),
				shouldLogCallCounter(0)
			{ }
		
		~Settings()
		{
			for_each(recorders.begin(), recorders.end(),
					 DeletePointer());
		}
		
		static Settings*& getPtr();
	};
	
	Settings& Settings::get()
	{
		Settings* p = getPtr();
		if (!p)
		{
			reset();
			p = getPtr();
		}
		return *p;
	}
	
	void Settings::reset()
	{
		Globals::get().invalidateCallSites();
		
		Settings*& p = getPtr();
		delete p;
		p = new Settings();
	}
	
	Settings* Settings::saveAndReset()
	{
		Globals::get().invalidateCallSites();
		
		Settings*& p = getPtr();
		Settings* originalSettings = p;
		p = new Settings();
		return originalSettings;
	}
	
	void Settings::restore(Settings* originalSettings)
	{
		Globals::get().invalidateCallSites();
		
		Settings*& p = getPtr();
		delete p;
		p = originalSettings;
	}
	
	Settings*& Settings::getPtr()
	{
		static Settings* currentSettings = NULL;
		return currentSettings;
	}
}

namespace LLError
{
	CallSite::CallSite(ELevel level,
					const char* file,
					int line,
					const std::type_info& class_info, 
					const char* function, 
					const char* broadTag, 
					const char* narrowTag,
					bool printOnce)
		: mLevel(level), mFile(file), mLine(line),
		  mClassInfo(class_info), mFunction(function),
		  mCached(false), mShouldLog(false), 
		  mBroadTag(broadTag), mNarrowTag(narrowTag), mPrintOnce(printOnce)
		{ }


	void CallSite::invalidate()
		{ mCached = false; }
}

namespace
{
	bool shouldLogToStderr()
	{
#if LL_DARWIN
		// On Mac OS X, stderr from apps launched from the Finder goes to the
		// console log.  It's generally considered bad form to spam too much
		// there.
		
		// If stdin is a tty, assume the user launched from the command line and
		// therefore wants to see stderr.  Otherwise, assume we've been launched
		// from the finder and shouldn't spam stderr.
		return isatty(0);
#else
		return true;
#endif
	}
	
	bool stderrLogWantsTime()
	{
#if LL_WINDOWS
		return false;
#else
		return true;
#endif
	}
	
	
	void commonInit(const std::string& dir)
	{
		LLError::Settings::reset();
		
		LLError::setDefaultLevel(LLError::LEVEL_INFO);
		LLError::setFatalFunction(LLError::crashAndLoop);
		LLError::setTimeFunction(LLError::utcTime);

		if (shouldLogToStderr())
		{
			LLError::addRecorder(new RecordToStderr(stderrLogWantsTime()));
		}
		
#if LL_WINDOWS
		LLError::addRecorder(new RecordToWinDebug);
#endif

		LogControlFile& e = LogControlFile::fromDirectory(dir);

		// NOTE: We want to explicitly load the file before we add it to the event timer
		// that checks for changes to the file.  Else, we're not actually loading the file yet,
		// and most of the initialization happens without any attention being paid to the
		// log control file.  Not to mention that when it finally gets checked later,
		// all log statements that have been evaluated already become dirty and need to be
		// evaluated for printing again.  So, make sure to call checkAndReload()
		// before addToEventTimer().
		e.checkAndReload();
		e.addToEventTimer();
	}
}

namespace LLError
{
	void initForServer(const std::string& identity)
	{
		std::string dir = "/opt/linden/etc";
		if (LLApp::instance())
		{
			dir = LLApp::instance()->getOption("configdir").asString();
		}
		commonInit(dir);
#if !LL_WINDOWS
		addRecorder(new RecordToSyslog(identity));
#endif
	}

	void initForApplication(const std::string& dir)
	{
		commonInit(dir);
	}

	void setPrintLocation(bool print)
	{
		Settings& s = Settings::get();
		s.printLocation = print;
	}

	void setFatalFunction(const FatalFunction& f)
	{
		Settings& s = Settings::get();
		s.crashFunction = f;
	}

    FatalFunction getFatalFunction()
    {
        Settings& s = Settings::get();
        return s.crashFunction;
    }

	void setTimeFunction(TimeFunction f)
	{
		Settings& s = Settings::get();
		s.timeFunction = f;
	}

	void setDefaultLevel(ELevel level)
	{
		Globals& g = Globals::get();
		Settings& s = Settings::get();
		g.invalidateCallSites();
		s.defaultLevel = level;
	}

	ELevel getDefaultLevel()
	{
		Settings& s = Settings::get();
		return s.defaultLevel;
	}

	void setFunctionLevel(const std::string& function_name, ELevel level)
	{
		Globals& g = Globals::get();
		Settings& s = Settings::get();
		g.invalidateCallSites();
		s.functionLevelMap[function_name] = level;
	}

	void setClassLevel(const std::string& class_name, ELevel level)
	{
		Globals& g = Globals::get();
		Settings& s = Settings::get();
		g.invalidateCallSites();
		s.classLevelMap[class_name] = level;
	}

	void setFileLevel(const std::string& file_name, ELevel level)
	{
		Globals& g = Globals::get();
		Settings& s = Settings::get();
		g.invalidateCallSites();
		s.fileLevelMap[file_name] = level;
	}

	void setTagLevel(const std::string& tag_name, ELevel level)
	{
		Globals& g = Globals::get();
		Settings& s = Settings::get();
		g.invalidateCallSites();
		s.tagLevelMap[tag_name] = level;
	}

	LLError::ELevel decodeLevel(std::string name)
	{
		static LevelMap level_names;
		if (level_names.empty())
		{
			level_names["ALL"]		= LLError::LEVEL_ALL;
			level_names["DEBUG"]	= LLError::LEVEL_DEBUG;
			level_names["INFO"]		= LLError::LEVEL_INFO;
			level_names["WARN"]		= LLError::LEVEL_WARN;
			level_names["ERROR"]	= LLError::LEVEL_ERROR;
			level_names["NONE"]		= LLError::LEVEL_NONE;
		}
		
		std::transform(name.begin(), name.end(), name.begin(), toupper);
		
		LevelMap::const_iterator i = level_names.find(name);
		if (i == level_names.end())
		{
			llwarns << "unrecognized logging level: '" << name << "'" << llendl;
			return LLError::LEVEL_INFO;
		}
		
		return i->second;
	}
}

namespace {
	void setLevels(LevelMap& map, const LLSD& list, LLError::ELevel level)
	{
		LLSD::array_const_iterator i, end;
		for (i = list.beginArray(), end = list.endArray(); i != end; ++i)
		{
			map[*i] = level;
		}
	}
}

namespace LLError
{
	void configure(const LLSD& config)
	{
		Globals& g = Globals::get();
		Settings& s = Settings::get();
		
		g.invalidateCallSites();
		s.functionLevelMap.clear();
		s.classLevelMap.clear();
		s.fileLevelMap.clear();
		s.tagLevelMap.clear();
		s.uniqueLogMessages.clear();
		
		setPrintLocation(config["print-location"]);
		setDefaultLevel(decodeLevel(config["default-level"]));
		
		LLSD sets = config["settings"];
		LLSD::array_const_iterator a, end;
		for (a = sets.beginArray(), end = sets.endArray(); a != end; ++a)
		{
			const LLSD& entry = *a;
			
			ELevel level = decodeLevel(entry["level"]);
			
			setLevels(s.functionLevelMap,	entry["functions"],	level);
			setLevels(s.classLevelMap,		entry["classes"],	level);
			setLevels(s.fileLevelMap,		entry["files"],		level);
			setLevels(s.tagLevelMap,		entry["tags"],		level);
		}
	}
}


namespace LLError
{
	Recorder::~Recorder()
		{ }

	// virtual
	bool Recorder::wantsTime()
		{ return false; }



	void addRecorder(Recorder* recorder)
	{
		if (recorder == NULL)
		{
			return;
		}
		Settings& s = Settings::get();
		s.recorders.push_back(recorder);
	}

	void removeRecorder(Recorder* recorder)
	{
		if (recorder == NULL)
		{
			return;
		}
		Settings& s = Settings::get();
		s.recorders.erase(
			std::remove(s.recorders.begin(), s.recorders.end(), recorder),
			s.recorders.end());
	}
}

namespace LLError
{
	void logToFile(const std::string& file_name)
	{
		LLError::Settings& s = LLError::Settings::get();

		removeRecorder(s.fileRecorder);
		delete s.fileRecorder;
		s.fileRecorder = NULL;
		s.fileRecorderFileName.clear();
		
		if (file_name.empty())
		{
			return;
		}
		
		RecordToFile* f = new RecordToFile(file_name);
		if (!f->okay())
		{
			delete f;
			return;
		}

		s.fileRecorderFileName = file_name;
		s.fileRecorder = f;
		addRecorder(f);
	}
	
	void logToFixedBuffer(LLLineBuffer* fixedBuffer)
	{
		LLError::Settings& s = LLError::Settings::get();

		removeRecorder(s.fixedBufferRecorder);
		delete s.fixedBufferRecorder;
		s.fixedBufferRecorder = NULL;
		
		if (!fixedBuffer)
		{
			return;
		}
		
		s.fixedBufferRecorder = new RecordToFixedBuffer(fixedBuffer);
		addRecorder(s.fixedBufferRecorder);
	}

	std::string logFileName()
	{
		LLError::Settings& s = LLError::Settings::get();
		return s.fileRecorderFileName;
	}
}

namespace
{
	void writeToRecorders(LLError::ELevel level, const std::string& message)
	{
		LLError::Settings& s = LLError::Settings::get();
	
		std::string messageWithTime;
		
		for (Recorders::const_iterator i = s.recorders.begin();
			i != s.recorders.end();
			++i)
		{
			LLError::Recorder* r = *i;
			
			if (r->wantsTime()  &&  s.timeFunction != NULL)
			{
				if (messageWithTime.empty())
				{
					messageWithTime = s.timeFunction() + " " + message;
				}
				
				r->recordMessage(level, messageWithTime);
			}
			else
			{
				r->recordMessage(level, message);
			}
		}
	}
}


/*
Recorder formats:

$type = "ERROR" | "WARNING" | "ALERT" | "INFO" | "DEBUG"
$loc = "$file($line)"
$msg = "$loc : " if FATAL or printing loc
		"" otherwise
$msg += "$type: "
$msg += contents of stringstream

$time = "%Y-%m-%dT%H:%M:%SZ" if UTC
	 or "%Y-%m-%dT%H:%M:%S %Z" if local

syslog:	"$msg"
file: "$time $msg\n"
stderr: "$time $msg\n" except on windows, "$msg\n"
fixedbuf: "$msg"
winddebug: "$msg\n"

Note: if FATAL, an additional line gets logged first, with $msg set to
	"$loc : error"
	
You get:
	llfoo.cpp(42) : error
	llfoo.cpp(42) : ERROR: something
	
*/

namespace {
	bool checkLevelMap(const LevelMap& map, const std::string& key,
						LLError::ELevel& level)
	{
		LevelMap::const_iterator i = map.find(key);
		if (i == map.end())
		{
			return false;
		}
		
			level = i->second;
		return true;
	}
	
	class LogLock
	{
	public:
		LogLock();
		~LogLock();
		bool ok() const { return mOK; }
	private:
		bool mLocked;
		bool mOK;
	};
	
	LogLock::LogLock()
		: mLocked(false), mOK(false)
	{
		if (!gLogMutexp)
		{
			mOK = true;
			return;
		}
		
		const int MAX_RETRIES = 5;
		for (int attempts = 0; attempts < MAX_RETRIES; ++attempts)
		{
			apr_status_t s = apr_thread_mutex_trylock(gLogMutexp);
			if (!APR_STATUS_IS_EBUSY(s))
			{
				mLocked = true;
				mOK = true;
				return;
			}

			ms_sleep(1);
			//apr_thread_yield();
				// Just yielding won't necessarily work, I had problems with
				// this on Linux - doug 12/02/04
		}

		// We're hosed, we can't get the mutex.  Blah.
		std::cerr << "LogLock::LogLock: failed to get mutex for log"
					<< std::endl;
	}
	
	LogLock::~LogLock()
	{
		if (mLocked)
		{
			apr_thread_mutex_unlock(gLogMutexp);
		}
	}
}

namespace LLError
{
	bool Log::shouldLog(CallSite& site)
	{
		LogLock lock;
		if (!lock.ok())
		{
			return false;
		}
		
		Globals& g = Globals::get();
		Settings& s = Settings::get();
		
		s.shouldLogCallCounter += 1;
		
		std::string class_name = className(site.mClassInfo);
		std::string function_name = functionName(site.mFunction);
#if LL_LINUX
		// gross, but typeid comparison seems to always fail here with gcc4.1
		if (0 != strcmp(site.mClassInfo.name(), typeid(NoClassInfo).name()))
#else
		if (site.mClassInfo != typeid(NoClassInfo))
#endif // LL_LINUX
		{
			function_name = class_name + "::" + function_name;
		}

		ELevel compareLevel = s.defaultLevel;

		// The most specific match found will be used as the log level,
		// since the computation short circuits.
		// So, in increasing order of importance:
		// Default < Broad Tag < File < Class < Function < Narrow Tag
		((site.mNarrowTag != NULL) ? checkLevelMap(s.tagLevelMap, site.mNarrowTag, compareLevel) : false)
		|| checkLevelMap(s.functionLevelMap, function_name, compareLevel)
		|| checkLevelMap(s.classLevelMap, class_name, compareLevel)
		|| checkLevelMap(s.fileLevelMap, abbreviateFile(site.mFile), compareLevel)
		|| ((site.mBroadTag != NULL) ? checkLevelMap(s.tagLevelMap, site.mBroadTag, compareLevel) : false);

		site.mCached = true;
		g.addCallSite(site);
		return site.mShouldLog = site.mLevel >= compareLevel;
	}


	std::ostringstream* Log::out()
	{
		LogLock lock;
		if (lock.ok())
		{
			Globals& g = Globals::get();

			if (!g.messageStreamInUse)
			{
				g.messageStreamInUse = true;
				return &g.messageStream;
			}
		}
		
		return new std::ostringstream;
	}
	
	void Log::flush(std::ostringstream* out, char* message)
    {
       LogLock lock;
       if (!lock.ok())
       {
           return;
       }
       
	   if(strlen(out->str().c_str()) < 128)
	   {
		   strcpy(message, out->str().c_str());
	   }
	   else
	   {
		   strncpy(message, out->str().c_str(), 127);
		   message[127] = '\0' ;
	   }
	   
	   Globals& g = Globals::get();
       if (out == &g.messageStream)
       {
           g.messageStream.clear();
           g.messageStream.str("");
           g.messageStreamInUse = false;
       }
       else
       {
           delete out;
       }
	   return ;
    }

	void Log::flush(std::ostringstream* out, const CallSite& site)
	{
		LogLock lock;
		if (!lock.ok())
		{
			return;
		}
		
		Globals& g = Globals::get();
		Settings& s = Settings::get();

		std::string message = out->str();
		if (out == &g.messageStream)
		{
			g.messageStream.clear();
			g.messageStream.str("");
			g.messageStreamInUse = false;
		}
		else
		{
			delete out;
		}

		if (site.mLevel == LEVEL_ERROR)
		{
			std::ostringstream fatalMessage;
			fatalMessage << abbreviateFile(site.mFile)
						<< "(" << site.mLine << ") : error";
			
			writeToRecorders(site.mLevel, fatalMessage.str());
		}
		
		
		std::ostringstream prefix;

		switch (site.mLevel)
		{
			case LEVEL_DEBUG:		prefix << "DEBUG: ";	break;
			case LEVEL_INFO:		prefix << "INFO: ";		break;
			case LEVEL_WARN:		prefix << "WARNING: ";	break;
			case LEVEL_ERROR:		prefix << "ERROR: ";	break;
			default:				prefix << "XXX: ";		break;
		};
		
		if (s.printLocation)
		{
			prefix << abbreviateFile(site.mFile)
					<< "(" << site.mLine << ") : ";
		}
		
	#if LL_WINDOWS
		// DevStudio: __FUNCTION__ already includes the full class name
	#else
                #if LL_LINUX
		// gross, but typeid comparison seems to always fail here with gcc4.1
		if (0 != strcmp(site.mClassInfo.name(), typeid(NoClassInfo).name()))
                #else
		if (site.mClassInfo != typeid(NoClassInfo))
                #endif // LL_LINUX
		{
			prefix << className(site.mClassInfo) << "::";
		}
	#endif
		prefix << site.mFunction << ": ";

		if (site.mPrintOnce)
		{
			std::map<std::string, unsigned int>::iterator messageIter = s.uniqueLogMessages.find(message);
			if (messageIter != s.uniqueLogMessages.end())
			{
				messageIter->second++;
				unsigned int num_messages = messageIter->second;
				if (num_messages == 10 || num_messages == 50 || (num_messages % 100) == 0)
				{
					prefix << "ONCE (" << num_messages << "th time seen): ";
				} 
				else
				{
					return;
				}
			}
			else 
			{
				prefix << "ONCE: ";
				s.uniqueLogMessages[message] = 1;
			}
		}
		
		prefix << message;
		message = prefix.str();
		
		writeToRecorders(site.mLevel, message);
		
		if (site.mLevel == LEVEL_ERROR  &&  s.crashFunction)
		{
			s.crashFunction(message);
		}
	}
}




namespace LLError
{
	Settings* saveAndResetSettings()
	{
		return Settings::saveAndReset();
	}
	
	void restoreSettings(Settings* s)
	{
		return Settings::restore(s);
	}

	std::string removePrefix(std::string& s, const std::string& p)
	{
		std::string::size_type where = s.find(p);
		if (where == std::string::npos)
		{
			return s;
		}
		
		return std::string(s, where + p.size());
	}
	
	void replaceChar(std::string& s, char old, char replacement)
	{
		std::string::size_type i = 0;
		std::string::size_type len = s.length();
		for ( ; i < len; i++ )
		{
			if (s[i] == old)
			{
				s[i] = replacement;
			}
		}
	}

	std::string abbreviateFile(const std::string& filePath)
	{
		std::string f = filePath;
#if LL_WINDOWS
		replaceChar(f, '\\', '/');
#endif
		static std::string indra_prefix = "indra/";
		f = removePrefix(f, indra_prefix);

#if LL_DARWIN
		static std::string newview_prefix = "newview/../";
		f = removePrefix(f, newview_prefix);
#endif

		return f;
	}

	int shouldLogCallCount()
	{
		Settings& s = Settings::get();
		return s.shouldLogCallCounter;
	}

#if LL_WINDOWS
		// VC80 was optimizing the error away.
		#pragma optimize("", off)
#endif
	void crashAndLoop(const std::string& message)
	{
		// Now, we go kaboom!
		int* make_me_crash = NULL;

		*make_me_crash = 0;

		while(true)
		{
			// Loop forever, in case the crash didn't work?
		}
		
		// this is an attempt to let Coverity and other semantic scanners know that this function won't be returning ever.
		exit(EXIT_FAILURE);
	}
#if LL_WINDOWS
		#pragma optimize("", on)
#endif

	std::string utcTime()
	{
		time_t now = time(NULL);
		const size_t BUF_SIZE = 64;
		char time_str[BUF_SIZE];	/* Flawfinder: ignore */
		
		int chars = strftime(time_str, BUF_SIZE, 
								  "%Y-%m-%dT%H:%M:%SZ",
								  gmtime(&now));

		return chars ? time_str : "time error";
	}
}

namespace LLError
{     
	char** LLCallStacks::sBuffer = NULL ;
	S32    LLCallStacks::sIndex  = 0 ;

#define SINGLE_THREADED 1

	class CallStacksLogLock
	{
	public:
		CallStacksLogLock();
		~CallStacksLogLock();

#if SINGLE_THREADED
		bool ok() const { return true; }
#else
		bool ok() const { return mOK; }
	private:
		bool mLocked;
		bool mOK;
#endif
	};
	
#if SINGLE_THREADED
	CallStacksLogLock::CallStacksLogLock()
	{
	}
	CallStacksLogLock::~CallStacksLogLock()
	{
	}
#else
	CallStacksLogLock::CallStacksLogLock()
		: mLocked(false), mOK(false)
	{
		if (!gCallStacksLogMutexp)
		{
			mOK = true;
			return;
		}
		
		const int MAX_RETRIES = 5;
		for (int attempts = 0; attempts < MAX_RETRIES; ++attempts)
		{
			apr_status_t s = apr_thread_mutex_trylock(gCallStacksLogMutexp);
			if (!APR_STATUS_IS_EBUSY(s))
			{
				mLocked = true;
				mOK = true;
				return;
			}

			ms_sleep(1);
		}

		// We're hosed, we can't get the mutex.  Blah.
		std::cerr << "CallStacksLogLock::CallStacksLogLock: failed to get mutex for log"
					<< std::endl;
	}
	
	CallStacksLogLock::~CallStacksLogLock()
	{
		if (mLocked)
		{
			apr_thread_mutex_unlock(gCallStacksLogMutexp);
		}
	}
#endif

	//static
   void LLCallStacks::push(const char* function, const int line)
   {
	   CallStacksLogLock lock;
       if (!lock.ok())
       {
           return;
       }

	   if(!sBuffer)
	   {
		   sBuffer = new char*[512] ;
		   sBuffer[0] = new char[512 * 128] ;
		   for(S32 i = 1 ; i < 512 ; i++)
		   {
			   sBuffer[i] = sBuffer[i-1] + 128 ;
		   }
		   sIndex = 0 ;
	   }

	   if(sIndex > 511)
	   {
		   clear() ;
	   }

	   strcpy(sBuffer[sIndex], function) ;
	   sprintf(sBuffer[sIndex] + strlen(function), " line: %d ", line) ;
	   sIndex++ ;

	   return ;
   }

	//static
   std::ostringstream* LLCallStacks::insert(const char* function, const int line)
   {
       std::ostringstream* _out = LLError::Log::out();
	   *_out << function << " line " << line << " " ;
             
	   return _out ;
   }

   //static
   void LLCallStacks::end(std::ostringstream* _out)
   {
	   CallStacksLogLock lock;
       if (!lock.ok())
       {
           return;
       }

	   if(!sBuffer)
	   {
		   sBuffer = new char*[512] ;
		   sBuffer[0] = new char[512 * 128] ;
		   for(S32 i = 1 ; i < 512 ; i++)
		   {
			   sBuffer[i] = sBuffer[i-1] + 128 ;
		   }
		   sIndex = 0 ;
	   }

	   if(sIndex > 511)
	   {
		   clear() ;
	   }

	   LLError::Log::flush(_out, sBuffer[sIndex++]) ;	   
   }

   //static
   void LLCallStacks::print()
   {
	   CallStacksLogLock lock;
       if (!lock.ok())
       {
           return;
       }

       if(sIndex > 0)
       {
           llinfos << " ************* PRINT OUT LL CALL STACKS ************* " << llendl ;
           while(sIndex > 0)
           {                  
			   sIndex-- ;
               llinfos << sBuffer[sIndex] << llendl ;
           }
           llinfos << " *************** END OF LL CALL STACKS *************** " << llendl ;
       }

	   if(sBuffer)
	   {
		   delete[] sBuffer[0] ;
		   delete[] sBuffer ;
		   sBuffer = NULL ;
	   }
   }

   //static
   void LLCallStacks::clear()
   {
       sIndex = 0 ;
   }
}

