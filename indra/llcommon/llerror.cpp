/** 
 * @file llerror.cpp
 * @date   December 2006
 * @brief error message system
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llerror.h"
#include "llerrorcontrol.h"

#include "llapp.h"
#include "llapr.h"
extern apr_thread_mutex_t *gLogMutexp;
#include "llfile.h"
#include "llfixedbuffer.h"
#include "lllivefile.h"
#include "llsd.h"
#include "llsdserialize.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#if !LL_WINDOWS
#include <stdio.h>
#include <syslog.h>
#endif
#include <time.h>
#if LL_WINDOWS
#include <windows.h>
#endif
#include <vector>


#ifdef __GNUC__
#include <cxxabi.h>
#endif

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
			mFile.open(filename.c_str(), llofstream::out | llofstream::app);
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
		RecordToStderr(bool timestamp) : mTimestamp(timestamp) { }

		virtual bool wantsTime() { return mTimestamp; }
		
		virtual void recordMessage(LLError::ELevel level,
									const std::string& message)
		{
			fprintf(stderr, "%s\n", message.c_str());
		}
	
	private:
		bool mTimestamp;
	};

	class RecordToFixedBuffer : public LLError::Recorder
	{
	public:
		RecordToFixedBuffer(LLFixedBuffer& buffer) : mBuffer(buffer) { }
		
		virtual void recordMessage(LLError::ELevel level,
									const std::string& message)
		{
			mBuffer.addLine(message.c_str());
		}
	
	private:
		LLFixedBuffer& mBuffer;
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
		static LogControlFile *fromDirectory(const std::string& dir);
		
		virtual void loadFile();
		
	private:
		LogControlFile(const std::string &filename)
			: LLLiveFile(filename)
			{ }
	};

	LogControlFile *LogControlFile::fromDirectory(const std::string& dir)
	{
		std::string dirBase = dir + "/";
			// NB: We have no abstraction in llcommon  for the "proper"
			// delimiter but it turns out that "/" works on all three platforms
			
		std::string file = dirBase + "logcontrol-dev.xml";
		
		llstat stat_info;
		if (LLFile::stat(file.c_str(), &stat_info)) {
			// NB: stat returns non-zero if it can't read the file, for example
			// if it doesn't exist.  LLFile has no better abstraction for 
			// testing for file existence.
			
			file = dirBase + "logcontrol.xml";
		}
		return new LogControlFile(file);
	}
	
	void LogControlFile::loadFile()
	{
		LLSD configuration;

		{
			llifstream file(filename().c_str());
			if (file.is_open())
			{
				LLSDSerialize::fromXML(configuration, file);
			}

			if (configuration.isUndefined())
			{
				llwarns << filename() << " missing, ill-formed,"
							" or simply undefined; not changing configuration"
						<< llendl;
				return;
			}
		}
		
		LLError::configure(configuration);
		llinfos << "logging reconfigured from " << filename() << llendl;
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

		static void Globals::cleanup();

	private:
		CallSiteVector callSites;
		static Globals *sGlobals;

		Globals()
			:	messageStreamInUse(false)
			{ }
		
	};

	Globals *Globals::sGlobals;

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
		   it is used, no matter what the global initializeation sequence
		   is.
		   See C++ FAQ Lite, sections 10.12 through 10.14
		*/
		if (sGlobals == NULL) {
			sGlobals = new Globals;
		}

		return *sGlobals;
	}

	void Globals::cleanup()
	{
		delete sGlobals;
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
		
		LLError::FatalFunction crashFunction;
		LLError::TimeFunction timeFunction;
		
		Recorders recorders;
		Recorder* fileRecorder;
		Recorder* fixedBufferRecorder;
		std::string fileRecorderFileName;
		
		int shouldLogCallCounter;
		
		static Settings& get();
		static void cleanup();
	
		static void reset();
		static Settings* saveAndReset();
		static void restore(Settings*);
		
	private:
		Settings()
			:	printLocation(false),
				defaultLevel(LLError::LEVEL_DEBUG),
				crashFunction(NULL),
				timeFunction(NULL),
				fileRecorder(NULL),
				fixedBufferRecorder(NULL),
				shouldLogCallCounter(0)
			{ }
		
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
	
	void Settings::cleanup()
	{
		Settings*& ptr = getPtr();
		if (ptr)
		{
			delete ptr;
			ptr = NULL;
		}
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
					const char* file, int line,
					const std::type_info& class_info, const char* function)
		: mLevel(level), mFile(file), mLine(line),
		  mClassInfo(class_info), mFunction(function),
		  mCached(false), mShouldLog(false)
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
	
	
	static LogControlFile *gLogControlFile;

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

		gLogControlFile = LogControlFile::fromDirectory(dir);
		gLogControlFile->addToEventTimer();
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

	void cleanupLogging(void)
	{
		delete gLogControlFile;
		gLogControlFile = NULL;

		Settings::cleanup();
		Globals::cleanup();
	}
	
	void setPrintLocation(bool print)
	{
		Settings& s = Settings::get();
		s.printLocation = print;
	}

	void setFatalFunction(FatalFunction f)
	{
		Settings& s = Settings::get();
		s.crashFunction = f;
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
}

namespace {
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
	
	void logToFixedBuffer(LLFixedBuffer* fixedBuffer)
	{
		LLError::Settings& s = LLError::Settings::get();

		removeRecorder(s.fixedBufferRecorder);
		delete s.fixedBufferRecorder;
		s.fixedBufferRecorder = NULL;
		
		if (!fixedBuffer)
		{
			return;
		}
		
		s.fixedBufferRecorder = new RecordToFixedBuffer(*fixedBuffer);
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
		if (site.mClassInfo != typeid(NoClassInfo))
		{
			function_name = class_name + "::" + function_name;
		}

		ELevel compareLevel = s.defaultLevel;

		checkLevelMap(s.functionLevelMap, function_name, compareLevel)
		|| checkLevelMap(s.classLevelMap, class_name, compareLevel)
		|| checkLevelMap(s.fileLevelMap, abbreviateFile(site.mFile), compareLevel);

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
		
		if (message.find(functionName(site.mFunction)) == std::string::npos)
		{
	#if LL_WINDOWS
			// DevStudio: __FUNCTION__ already includes the full class name
	#else
			if (site.mClassInfo != typeid(NoClassInfo))
			{
				prefix << className(site.mClassInfo) << "::";
			}
	#endif
			prefix << site.mFunction << ": ";
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

	void crashAndLoop(const std::string& message)
	{
		// Now, we go kaboom!
		int* crash = NULL;

		*crash = 0;

		while(true)
		{
			// Loop forever, in case the crash didn't work?
		}
	}

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

