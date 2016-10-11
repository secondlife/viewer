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
#include "llsingleton.h"
#include "llstl.h"
#include "lltimer.h"

namespace {
#if LL_WINDOWS
	void debugger_print(const std::string& s)
	{
		// Be careful when calling OutputDebugString as it throws DBG_PRINTEXCEPTION_C 
		// which works just fine under the windows debugger, but can cause users who
		// have enabled SEHOP exception chain validation to crash due to interactions
		// between the Win 32-bit exception handling and boost coroutine fiber stacks. BUG-2707
		//
		if (IsDebuggerPresent())
		{
			// Need UTF16 for Unicode OutputDebugString
			//
			if (s.size())
			{
				OutputDebugString(utf8str_to_utf16str(s).c_str());
				OutputDebugString(TEXT("\n"));
			}
		}
	}
#else
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
			mFile.open(filename.c_str(), std::ios_base::out | std::ios_base::app);
			if (!mFile)
			{
				LL_INFOS() << "Error setting log file to " << filename << LL_ENDL;
			}
			mWantsTime = true;
            mWantsTags = true;
		}
		
		~RecordToFile()
		{
			mFile.close();
		}
		
		bool okay() { return mFile.good(); }
		
		virtual void recordMessage(LLError::ELevel level,
									const std::string& message)
		{
			mFile << message << std::endl;
		}
	
	private:
		llofstream mFile;
	};
	
	
	class RecordToStderr : public LLError::Recorder
	{
	public:
		RecordToStderr(bool timestamp) : mUseANSI(ANSI_PROBE) 
		{
			mWantsTime = timestamp;
		}
		
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
		enum ANSIState 
		{
			ANSI_PROBE, 
			ANSI_YES, 
			ANSI_NO
		}					mUseANSI;

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
		RecordToWinDebug()
		{}

		virtual void recordMessage(LLError::ELevel level,
								   const std::string& message)
		{
			debugger_print(message);
		}
	};
#endif
}


namespace
{
	std::string className(const std::type_info& type)
	{
#ifdef __GNUC__
		// GCC: type_info::name() returns a mangled class name,st demangle

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
			llifstream file(filename().c_str());
			if (file.is_open())
			{
				LLSDSerialize::fromXML(configuration, file);
			}

			if (configuration.isUndefined())
			{
				LL_WARNS() << filename() << " missing, ill-formed,"
							" or simply undefined; not changing configuration"
						<< LL_ENDL;
				return false;
			}
		}
		
		LLError::configure(configuration);
		LL_INFOS() << "logging reconfigured from " << filename() << LL_ENDL;
		return true;
	}


	typedef std::map<std::string, LLError::ELevel> LevelMap;
	typedef std::vector<LLError::RecorderPtr> Recorders;
	typedef std::vector<LLError::CallSite*> CallSiteVector;

	class Globals : public LLSingleton<Globals>
	{
	public:
		Globals();

		std::ostringstream messageStream;
		bool messageStreamInUse;

		void addCallSite(LLError::CallSite&);
		void invalidateCallSites();

	private:
		CallSiteVector callSites;
	};

	Globals::Globals()
		: messageStream(),
		messageStreamInUse(false),
		callSites()
	{
	}

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
}

namespace LLError
{
	class SettingsConfig : public LLRefCount
	{
		friend class Settings;

	public:
		virtual ~SettingsConfig();

		bool                                mPrintLocation;

		LLError::ELevel                     mDefaultLevel;
		
		LevelMap                            mFunctionLevelMap;
		LevelMap                            mClassLevelMap;
		LevelMap                            mFileLevelMap;
		LevelMap                            mTagLevelMap;
		std::map<std::string, unsigned int> mUniqueLogMessages;
		
		LLError::FatalFunction              mCrashFunction;
		LLError::TimeFunction               mTimeFunction;
		
		Recorders                           mRecorders;
		RecorderPtr                         mFileRecorder;
		RecorderPtr                         mFixedBufferRecorder;
		std::string                         mFileRecorderFileName;
		
		int									mShouldLogCallCounter;
		
	private:
		SettingsConfig();
	};

	typedef LLPointer<SettingsConfig> SettingsConfigPtr;

	class Settings : public LLSingleton<Settings>
	{
	public:
		Settings();

		SettingsConfigPtr getSettingsConfig();
	
		void reset();
		SettingsStoragePtr saveAndReset();
		void restore(SettingsStoragePtr pSettingsStorage);
		
	private:
		SettingsConfigPtr mSettingsConfig;
	};
	
	SettingsConfig::SettingsConfig()
		: LLRefCount(),
		mPrintLocation(false),
		mDefaultLevel(LLError::LEVEL_DEBUG),
		mFunctionLevelMap(),
		mClassLevelMap(),
		mFileLevelMap(),
		mTagLevelMap(),
		mUniqueLogMessages(),
		mCrashFunction(NULL),
		mTimeFunction(NULL),
		mRecorders(),
		mFileRecorder(),
		mFixedBufferRecorder(),
		mFileRecorderFileName(),
		mShouldLogCallCounter(0)
	{
	}

	SettingsConfig::~SettingsConfig()
	{
		mRecorders.clear();
	}

	Settings::Settings()
		: LLSingleton<Settings>(),
		mSettingsConfig(new SettingsConfig())
	{
	}

	SettingsConfigPtr Settings::getSettingsConfig()
	{
		return mSettingsConfig;
	}
	
	void Settings::reset()
	{
		Globals::getInstance()->invalidateCallSites();
		mSettingsConfig = new SettingsConfig();
	}
	
	SettingsStoragePtr Settings::saveAndReset()
	{
		SettingsStoragePtr oldSettingsConfig(mSettingsConfig.get());
		reset();
		return oldSettingsConfig;
	}
	
	void Settings::restore(SettingsStoragePtr pSettingsStorage)
	{
		Globals::getInstance()->invalidateCallSites();
		SettingsConfigPtr newSettingsConfig(dynamic_cast<SettingsConfig *>(pSettingsStorage.get()));
		mSettingsConfig = newSettingsConfig;
	}
}

namespace LLError
{
	CallSite::CallSite(ELevel level,
					const char* file,
					int line,
					const std::type_info& class_info, 
					const char* function, 
					bool printOnce,
					const char** tags, 
					size_t tag_count)
	:	mLevel(level), 
		mFile(file), 
		mLine(line),
		mClassInfo(class_info), 
		mFunction(function),
		mCached(false), 
		mShouldLog(false), 
		mPrintOnce(printOnce),
		mTags(new const char* [tag_count]),
		mTagCount(tag_count)
	{
		for (int i = 0; i < tag_count; i++)
		{
			mTags[i] = tags[i];
		}

		switch (mLevel)
		{
		case LEVEL_DEBUG:		mLevelString = "DEBUG:";	break;
		case LEVEL_INFO:		mLevelString = "INFO:";		break;
		case LEVEL_WARN:		mLevelString = "WARNING:";	break;
		case LEVEL_ERROR:		mLevelString = "ERROR:";	break;
		default:				mLevelString = "XXX:";		break;
		};

		mLocationString = llformat("%s(%d) :", abbreviateFile(mFile).c_str(), mLine);
#if LL_WINDOWS
		// DevStudio: __FUNCTION__ already includes the full class name
#else
#if LL_LINUX
		// gross, but typeid comparison seems to always fail here with gcc4.1
		if (0 != strcmp(mClassInfo.name(), typeid(NoClassInfo).name()))
#else
		if (mClassInfo != typeid(NoClassInfo))
#endif // LL_LINUX
		{
			mFunctionString = className(mClassInfo) + "::";
		}
#endif
		mFunctionString += std::string(mFunction) + ":";
		for (size_t i = 0; i < mTagCount; i++)
		{
			mTagString += std::string("#") + mTags[i] + ((i == mTagCount - 1) ? "" : ",");
		}
	}

	CallSite::~CallSite()
	{
		delete []mTags;
	}

	void CallSite::invalidate()
	{
		mCached = false; 
	}
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
	
	
	void commonInit(const std::string& dir, bool log_to_stderr = true)
	{
		LLError::Settings::getInstance()->reset();
		
		LLError::setDefaultLevel(LLError::LEVEL_INFO);
		LLError::setFatalFunction(LLError::crashAndLoop);
		LLError::setTimeFunction(LLError::utcTime);

		// log_to_stderr is only false in the unit and integration tests to keep builds quieter
		if (log_to_stderr && shouldLogToStderr())
		{
			LLError::RecorderPtr recordToStdErr(new RecordToStderr(stderrLogWantsTime()));
			LLError::addRecorder(recordToStdErr);
		}
		
#if LL_WINDOWS
		LLError::RecorderPtr recordToWinDebug(new RecordToWinDebug());
		LLError::addRecorder(recordToWinDebug);
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
		LLError::RecorderPtr recordToSyslog(new RecordToSyslog(identity));
		addRecorder(recordToSyslog);
#endif
	}

	void initForApplication(const std::string& dir, bool log_to_stderr)
	{
		commonInit(dir, log_to_stderr);
	}

	void setPrintLocation(bool print)
	{
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();
		s->mPrintLocation = print;
	}

	void setFatalFunction(const FatalFunction& f)
	{
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();
		s->mCrashFunction = f;
	}

    FatalFunction getFatalFunction()
    {
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();
        return s->mCrashFunction;
    }

	void setTimeFunction(TimeFunction f)
	{
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();
		s->mTimeFunction = f;
	}

	void setDefaultLevel(ELevel level)
	{
		Globals::getInstance()->invalidateCallSites();
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();
		s->mDefaultLevel = level;
	}

	ELevel getDefaultLevel()
	{
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();
		return s->mDefaultLevel;
	}

	void setFunctionLevel(const std::string& function_name, ELevel level)
	{
		Globals::getInstance()->invalidateCallSites();
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();
		s->mFunctionLevelMap[function_name] = level;
	}

	void setClassLevel(const std::string& class_name, ELevel level)
	{
		Globals::getInstance()->invalidateCallSites();
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();
		s->mClassLevelMap[class_name] = level;
	}

	void setFileLevel(const std::string& file_name, ELevel level)
	{
		Globals::getInstance()->invalidateCallSites();
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();
		s->mFileLevelMap[file_name] = level;
	}

	void setTagLevel(const std::string& tag_name, ELevel level)
	{
		Globals::getInstance()->invalidateCallSites();
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();
		s->mTagLevelMap[tag_name] = level;
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
			LL_WARNS() << "unrecognized logging level: '" << name << "'" << LL_ENDL;
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
		Globals::getInstance()->invalidateCallSites();
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();
		
		s->mFunctionLevelMap.clear();
		s->mClassLevelMap.clear();
		s->mFileLevelMap.clear();
		s->mTagLevelMap.clear();
		s->mUniqueLogMessages.clear();
		
		setPrintLocation(config["print-location"]);
		setDefaultLevel(decodeLevel(config["default-level"]));
		
		LLSD sets = config["settings"];
		LLSD::array_const_iterator a, end;
		for (a = sets.beginArray(), end = sets.endArray(); a != end; ++a)
		{
			const LLSD& entry = *a;
			
			ELevel level = decodeLevel(entry["level"]);
			
			setLevels(s->mFunctionLevelMap,	entry["functions"],	level);
			setLevels(s->mClassLevelMap,	entry["classes"],	level);
			setLevels(s->mFileLevelMap,		entry["files"],		level);
			setLevels(s->mTagLevelMap,		entry["tags"],		level);
		}
	}
}


namespace LLError
{
	Recorder::Recorder()
	:	mWantsTime(false),
		mWantsTags(false),
		mWantsLevel(true),
		mWantsLocation(false),
		mWantsFunctionName(true)
	{
	}

	Recorder::~Recorder()
	{
	}

	bool Recorder::wantsTime()
	{ 
		return mWantsTime; 
	}

	// virtual
	bool Recorder::wantsTags()
	{
		return mWantsTags;
	}

	// virtual 
	bool Recorder::wantsLevel() 
	{ 
		return mWantsLevel;
	}

	// virtual 
	bool Recorder::wantsLocation() 
	{ 
		return mWantsLocation;
	}

	// virtual 
	bool Recorder::wantsFunctionName() 
	{ 
		return mWantsFunctionName;
	}

	void addRecorder(RecorderPtr recorder)
	{
		if (!recorder)
		{
			return;
		}
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();
		s->mRecorders.push_back(recorder);
	}

	void removeRecorder(RecorderPtr recorder)
	{
		if (!recorder)
		{
			return;
		}
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();
		s->mRecorders.erase(std::remove(s->mRecorders.begin(), s->mRecorders.end(), recorder),
							s->mRecorders.end());
	}
}

namespace LLError
{
	void logToFile(const std::string& file_name)
	{
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();

		removeRecorder(s->mFileRecorder);
		s->mFileRecorder.reset();
		s->mFileRecorderFileName.clear();
		
		if (file_name.empty())
		{
			return;
		}
		
		RecorderPtr recordToFile(new RecordToFile(file_name));
		if (boost::dynamic_pointer_cast<RecordToFile>(recordToFile)->okay())
		{
			s->mFileRecorderFileName = file_name;
			s->mFileRecorder = recordToFile;
			addRecorder(recordToFile);
		}
	}
	
	void logToFixedBuffer(LLLineBuffer* fixedBuffer)
	{
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();

		removeRecorder(s->mFixedBufferRecorder);
		s->mFixedBufferRecorder.reset();
		
		if (!fixedBuffer)
		{
			return;
		}
		
		RecorderPtr recordToFixedBuffer(new RecordToFixedBuffer(fixedBuffer));
		s->mFixedBufferRecorder = recordToFixedBuffer;
		addRecorder(recordToFixedBuffer);
	}

	std::string logFileName()
	{
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();
		return s->mFileRecorderFileName;
	}
}

namespace
{
	void writeToRecorders(const LLError::CallSite& site, const std::string& message, bool show_location = true, bool show_time = true, bool show_tags = true, bool show_level = true, bool show_function = true)
	{
		LLError::ELevel level = site.mLevel;
		LLError::SettingsConfigPtr s = LLError::Settings::getInstance()->getSettingsConfig();
	
		for (Recorders::const_iterator i = s->mRecorders.begin();
			i != s->mRecorders.end();
			++i)
		{
			LLError::RecorderPtr r = *i;
			
			std::ostringstream message_stream;

			if (show_time && r->wantsTime() && s->mTimeFunction != NULL)
			{
				message_stream << s->mTimeFunction() << " ";
			}

			if (show_level && r->wantsLevel())
            {
				message_stream << site.mLevelString << " ";
            }
				
			if (show_tags && r->wantsTags())
			{
				message_stream << site.mTagString;
			}

            if (show_location && (r->wantsLocation() || level == LLError::LEVEL_ERROR || s->mPrintLocation))
            {
                message_stream << site.mLocationString << " ";
            }

			if (show_function && r->wantsFunctionName())
			{
				message_stream << site.mFunctionString << " ";
			}

			message_stream << message;

			r->recordMessage(level, message_stream.str());
		}
	}
}

namespace {
	bool checkLevelMap(const LevelMap& map, const std::string& key,
						LLError::ELevel& level)
	{
		bool stop_checking;
		LevelMap::const_iterator i = map.find(key);
		if (i == map.end())
		{
			return stop_checking = false;
		}
		
			level = i->second;
		return stop_checking = true;
	}
	
	bool checkLevelMap(	const LevelMap& map, 
						const char *const * keys, 
						size_t count,
						LLError::ELevel& level)
	{
		bool found_level = false;

		LLError::ELevel tag_level = LLError::LEVEL_NONE;

		for (size_t i = 0; i < count; i++)
		{
			LevelMap::const_iterator it = map.find(keys[i]);
			if (it != map.end())
			{
				found_level = true;
				tag_level = llmin(tag_level, it->second);
			}
		}

		if (found_level)
		{
			level = tag_level;
		}
		return found_level;
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
		
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();
		
		s->mShouldLogCallCounter++;
		
		const std::string& class_name = className(site.mClassInfo);
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

		ELevel compareLevel = s->mDefaultLevel;

		// The most specific match found will be used as the log level,
		// since the computation short circuits.
		// So, in increasing order of importance:
		// Default < Tags < File < Class < Function
		checkLevelMap(s->mFunctionLevelMap, function_name, compareLevel)
		|| checkLevelMap(s->mClassLevelMap, class_name, compareLevel)
		|| checkLevelMap(s->mFileLevelMap, abbreviateFile(site.mFile), compareLevel)
		|| (site.mTagCount > 0
			? checkLevelMap(s->mTagLevelMap, site.mTags, site.mTagCount, compareLevel) 
			: false);

		site.mCached = true;
		Globals::getInstance()->addCallSite(site);
		return site.mShouldLog = site.mLevel >= compareLevel;
	}


	std::ostringstream* Log::out()
	{
		LogLock lock;
		if (lock.ok())
		{
			Globals* g = Globals::getInstance();

			if (!g->messageStreamInUse)
			{
				g->messageStreamInUse = true;
				return &g->messageStream;
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
	   
	   Globals* g = Globals::getInstance();
       if (out == &g->messageStream)
       {
           g->messageStream.clear();
           g->messageStream.str("");
           g->messageStreamInUse = false;
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
		
		Globals* g = Globals::getInstance();
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();

		std::string message = out->str();
		if (out == &g->messageStream)
		{
			g->messageStream.clear();
			g->messageStream.str("");
			g->messageStreamInUse = false;
		}
		else
		{
			delete out;
		}

		if (site.mLevel == LEVEL_ERROR)
		{
			writeToRecorders(site, "error", true, true, true, false, false);
		}
		
		std::ostringstream message_stream;

		if (site.mPrintOnce)
		{
			std::map<std::string, unsigned int>::iterator messageIter = s->mUniqueLogMessages.find(message);
			if (messageIter != s->mUniqueLogMessages.end())
			{
				messageIter->second++;
				unsigned int num_messages = messageIter->second;
				if (num_messages == 10 || num_messages == 50 || (num_messages % 100) == 0)
				{
					message_stream << "ONCE (" << num_messages << "th time seen): ";
				} 
				else
				{
					return;
				}
			}
			else 
			{
				message_stream << "ONCE: ";
				s->mUniqueLogMessages[message] = 1;
			}
		}
		
		message_stream << message;
		
		writeToRecorders(site, message_stream.str());
		
		if (site.mLevel == LEVEL_ERROR  &&  s->mCrashFunction)
		{
			s->mCrashFunction(message_stream.str());
		}
	}
}

namespace LLError
{
	SettingsStoragePtr saveAndResetSettings()
	{
		return Settings::getInstance()->saveAndReset();
	}
	
	void restoreSettings(SettingsStoragePtr pSettingsStorage)
	{
		return Settings::getInstance()->restore(pSettingsStorage);
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
		SettingsConfigPtr s = Settings::getInstance()->getSettingsConfig();
		return s->mShouldLogCallCounter;
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
   void LLCallStacks::allocateStackBuffer()
   {
	   if(sBuffer == NULL)
	   {
		   sBuffer = new char*[512] ;
		   sBuffer[0] = new char[512 * 128] ;
		   for(S32 i = 1 ; i < 512 ; i++)
		   {
			   sBuffer[i] = sBuffer[i-1] + 128 ;
		   }
		   sIndex = 0 ;
	   }
   }

   void LLCallStacks::freeStackBuffer()
   {
	   if(sBuffer != NULL)
	   {
		   delete [] sBuffer[0] ;
		   delete [] sBuffer ;
		   sBuffer = NULL ;
	   }
   }

   //static
   void LLCallStacks::push(const char* function, const int line)
   {
	   CallStacksLogLock lock;
       if (!lock.ok())
       {
           return;
       }

	   if(sBuffer == NULL)
	   {
		   allocateStackBuffer();
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

	   if(sBuffer == NULL)
	   {
		   allocateStackBuffer();
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
           LL_INFOS() << " ************* PRINT OUT LL CALL STACKS ************* " << LL_ENDL;
           while(sIndex > 0)
           {                  
			   sIndex-- ;
               LL_INFOS() << sBuffer[sIndex] << LL_ENDL;
           }
           LL_INFOS() << " *************** END OF LL CALL STACKS *************** " << LL_ENDL;
       }

	   if(sBuffer != NULL)
	   {
		   freeStackBuffer();
	   }
   }

   //static
   void LLCallStacks::clear()
   {
       sIndex = 0 ;
   }

   //static
   void LLCallStacks::cleanup()
   {
	   freeStackBuffer();
   }
}

