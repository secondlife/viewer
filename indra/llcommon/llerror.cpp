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
#include "llsdutil.h"

#include <cctype>
#ifdef __GNUC__
# include <cxxabi.h>
#endif // __GNUC__
#include <sstream>
#if !LL_WINDOWS
# include <syslog.h>
# include <unistd.h>
# include <sys/stat.h>
#else
# include <io.h>
#endif // !LL_WINDOWS
#include <vector>
#include "string.h"

#include "llapp.h"
#include "llapr.h"
#include "llfile.h"
#include "lllivefile.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llsingleton.h"
#include "llstl.h"
#include "lltimer.h"

// On Mac, got:
// #error "Boost.Stacktrace requires `_Unwind_Backtrace` function. Define
// `_GNU_SOURCE` macro or `BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED` if
// _Unwind_Backtrace is available without `_GNU_SOURCE`."
#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#include <boost/stacktrace.hpp>

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

        virtual bool enabled() override
        {
            return LLError::getEnabledLogTypesMask() & 0x01;
        }
        
		virtual void recordMessage(LLError::ELevel level,
									const std::string& message) override
		{
            LL_PROFILE_ZONE_SCOPED_CATEGORY_LOGGING
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
		RecordToFile(const std::string& filename):
			mName(filename)
		{
			mFile.open(filename.c_str(), std::ios_base::out | std::ios_base::app);
			if (!mFile)
			{
				LL_INFOS() << "Error setting log file to " << filename << LL_ENDL;
			}
			else
			{
				if (!LLError::getAlwaysFlush())
				{
					mFile.sync_with_stdio(false);
				}
			}
		}

		~RecordToFile()
		{
			mFile.close();
		}

        virtual bool enabled() override
        {
#ifdef LL_RELEASE_FOR_DOWNLOAD
            return 1;
#else
            return LLError::getEnabledLogTypesMask() & 0x02;
#endif
        }
        
        bool okay() const { return mFile.good(); }

        std::string getFilename() const { return mName; }

        virtual void recordMessage(LLError::ELevel level,
                                    const std::string& message) override
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_LOGGING
            if (LLError::getAlwaysFlush())
            {
                mFile << message << std::endl;
            }
            else
            {
                mFile << message << "\n";
            }
        }

	private:
		const std::string mName;
		llofstream mFile;
	};
	
	
	class RecordToStderr : public LLError::Recorder
	{
	public:
		RecordToStderr(bool timestamp) : mUseANSI(checkANSI()) 
		{
            this->showMultiline(true);
		}
		
        virtual bool enabled() override
        {
            return LLError::getEnabledLogTypesMask() & 0x04;
        }
        
        LL_FORCE_INLINE std::string createBoldANSI()
        {
            std::string ansi_code;
            ansi_code += '\033';
            ansi_code += "[";
            ansi_code += "1";
            ansi_code += "m";

            return ansi_code;
        }

        LL_FORCE_INLINE std::string createResetANSI()
        {
            std::string ansi_code;
            ansi_code += '\033';
            ansi_code += "[";
            ansi_code += "0";
            ansi_code += "m";

            return ansi_code;
        }

        LL_FORCE_INLINE std::string createANSI(const std::string& color)
        {
            std::string ansi_code;
            ansi_code  += '\033';
            ansi_code  += "[";
            ansi_code += "38;5;";
            ansi_code  += color;
            ansi_code += "m";

            return ansi_code;
        }

		virtual void recordMessage(LLError::ELevel level,
					   const std::string& message) override
		{
            LL_PROFILE_ZONE_SCOPED_CATEGORY_LOGGING
            // The default colors for error, warn and debug are now a bit more pastel
            // and easier to read on the default (black) terminal background but you 
            // now have the option to set the color of each via an environment variables:
            // LL_ANSI_ERROR_COLOR_CODE (default is red)
            // LL_ANSI_WARN_COLOR_CODE  (default is blue)
            // LL_ANSI_DEBUG_COLOR_CODE (default is magenta)
            // The list of color codes can be found in many places but I used this page:
            // https://www.lihaoyi.com/post/BuildyourownCommandLinewithANSIescapecodes.html#256-colors
            // (Note: you may need to restart Visual Studio to pick environment changes)
            char* val = nullptr;
            std::string s_ansi_error_code = "160";
            if ((val = getenv("LL_ANSI_ERROR_COLOR_CODE")) != nullptr) s_ansi_error_code = std::string(val);
            std::string s_ansi_warn_code = "33";
            if ((val = getenv("LL_ANSI_WARN_COLOR_CODE")) != nullptr) s_ansi_warn_code = std::string(val);
            std::string s_ansi_debug_code = "177";
            if ((val = getenv("LL_ANSI_DEBUG_COLOR_CODE")) != nullptr) s_ansi_debug_code = std::string(val);

            static std::string s_ansi_error = createANSI(s_ansi_error_code); // default is red
            static std::string s_ansi_warn  = createANSI(s_ansi_warn_code); // default is blue
            static std::string s_ansi_debug = createANSI(s_ansi_debug_code); // default is magenta

			if (mUseANSI)
			{
                writeANSI((level == LLError::LEVEL_ERROR) ? s_ansi_error :
                          (level == LLError::LEVEL_WARN)  ? s_ansi_warn :
                                                            s_ansi_debug, message);
			}
            else
            {
                LL_PROFILE_ZONE_NAMED("fprintf");
                 fprintf(stderr, "%s\n", message.c_str());
            }
		}
	
	private:
		bool mUseANSI;

        LL_FORCE_INLINE void writeANSI(const std::string& ansi_code, const std::string& message)
		{
            LL_PROFILE_ZONE_SCOPED_CATEGORY_LOGGING
            static std::string s_ansi_bold = createBoldANSI();  // bold text
            static std::string s_ansi_reset = createResetANSI();  // reset
			// ANSI color code escape sequence, message, and reset in one fprintf call
            // Default all message levels to bold so we can distinguish our own messages from those dumped by subprocesses and libraries.
			fprintf(stderr, "%s%s\n%s", ansi_code.c_str(), message.c_str(), s_ansi_reset.c_str() );
		}

		static bool checkANSI(void)
		{
			// Check whether it's okay to use ANSI; if stderr is
			// a tty then we assume yes.  Can be turned off with
			// the LL_NO_ANSI_COLOR env var.
			return (0 != isatty(2)) &&
				(NULL == getenv("LL_NO_ANSI_COLOR"));
		}
	};

	class RecordToFixedBuffer : public LLError::Recorder
	{
	public:
		RecordToFixedBuffer(LLLineBuffer* buffer)
            : mBuffer(buffer)
            {
                this->showMultiline(true);
                this->showTags(false);
                this->showLocation(false);
            }
		
        virtual bool enabled() override
        {
            return LLError::getEnabledLogTypesMask() & 0x08;
        }
        
		virtual void recordMessage(LLError::ELevel level,
								   const std::string& message) override
		{
            LL_PROFILE_ZONE_SCOPED_CATEGORY_LOGGING
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
		{
            this->showMultiline(true);
            this->showTags(false);
            this->showLocation(false);
        }

        virtual bool enabled() override
        {
            return LLError::getEnabledLogTypesMask() & 0x10;
        }
        
		virtual void recordMessage(LLError::ELevel level,
								   const std::string& message) override
		{
            LL_PROFILE_ZONE_SCOPED_CATEGORY_LOGGING
			debugger_print(message);
		}
	};
#endif
}


namespace
{
	std::string className(const std::type_info& type)
	{
		return LLError::Log::demangle(type.name());
	}
} // anonymous

namespace LLError
{
	std::string Log::demangle(const char* mangled)
	{
#ifdef __GNUC__
		// GCC: type_info::name() returns a mangled class name,st demangle
		// passing nullptr, 0 forces allocation of a unique buffer we can free
		// fixing MAINT-8724 on OSX 10.14
		int status = -1;
		char* name = abi::__cxa_demangle(mangled, nullptr, 0, &status);
		std::string result(name ? name : mangled);
		free(name);
		return result;

#elif LL_WINDOWS
		// Visual Studio: type_info::name() includes the text "class " at the start
		std::string name = mangled;
		for (const auto& prefix : std::vector<std::string>{ "class ", "struct " })
		{
			if (0 == name.compare(0, prefix.length(), prefix))
			{
				return name.substr(prefix.length());
			}
		}
		// huh, that's odd, we should see one or the other prefix -- but don't
		// try to log unless logging is already initialized
		// in Python, " or ".join(vector) -- but in C++, a PITB
		LL_DEBUGS() << "Did not see 'class' or 'struct' prefix on '"
			<< name << "'" << LL_ENDL;
		return name;

#else  // neither GCC nor Visual Studio
		return mangled;
#endif
	}
} // LLError

namespace
{
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
		static LogControlFile& fromDirectory(const std::string& user_dir, const std::string& app_dir);
		
		virtual bool loadFile();
		
	private:
		LogControlFile(const std::string &filename)
			: LLLiveFile(filename)
			{ }
	};

	LogControlFile& LogControlFile::fromDirectory(const std::string& user_dir, const std::string& app_dir)
	{
        // NB: We have no abstraction in llcommon  for the "proper"
        // delimiter but it turns out that "/" works on all three platforms
			
		std::string file = user_dir + "/logcontrol-dev.xml";
		
		llstat stat_info;
		if (LLFile::stat(file, &stat_info)) {
			// NB: stat returns non-zero if it can't read the file, for example
			// if it doesn't exist.  LLFile has no better abstraction for 
			// testing for file existence.
			
			file = app_dir + "/logcontrol.xml";
		}
		return * new LogControlFile(file);
			// NB: This instance is never freed
	}
	
	bool LogControlFile::loadFile()
	{
		LLSD configuration;

		{
			llifstream file(filename().c_str());
			if (!file.is_open())
			{
				LL_WARNS() << filename() << " failed to open file; not changing configuration" << LL_ENDL;
				return false;
			}

			if (LLSDSerialize::fromXML(configuration, file) == LLSDParser::PARSE_FAILURE)
			{
				LL_WARNS() << filename() << " parcing error; not changing configuration" << LL_ENDL;
				return false;
			}

			if (! configuration || !configuration.isMap())
			{
				LL_WARNS() << filename() << " missing, ill-formed, or simply undefined"
							" content; not changing configuration"
						<< LL_ENDL;
				return false;
			}
		}
		
		LLError::configure(configuration);
		LL_INFOS("LogControlFile") << "logging reconfigured from " << filename() << LL_ENDL;
		return true;
	}


	typedef std::map<std::string, LLError::ELevel> LevelMap;
	typedef std::vector<LLError::RecorderPtr> Recorders;
	typedef std::vector<LLError::CallSite*> CallSiteVector;

    class SettingsConfig : public LLRefCount
    {
        friend class Globals;

    public:
        virtual ~SettingsConfig();

        LLError::ELevel                     mDefaultLevel;

        bool 								mLogAlwaysFlush;

        U32 								mEnabledLogTypesMask;

        LevelMap                            mFunctionLevelMap;
        LevelMap                            mClassLevelMap;
        LevelMap                            mFileLevelMap;
        LevelMap                            mTagLevelMap;
        std::map<std::string, unsigned int> mUniqueLogMessages;

        LLError::FatalFunction              mCrashFunction;
        LLError::TimeFunction               mTimeFunction;

        Recorders                           mRecorders;
        LLMutex                             mRecorderMutex;

        int                                 mShouldLogCallCounter;

    private:
        SettingsConfig();
    };

    typedef LLPointer<SettingsConfig> SettingsConfigPtr;

    SettingsConfig::SettingsConfig()
        : LLRefCount(),
        mDefaultLevel(LLError::LEVEL_DEBUG),
        mLogAlwaysFlush(true),
        mEnabledLogTypesMask(255),
        mFunctionLevelMap(),
        mClassLevelMap(),
        mFileLevelMap(),
        mTagLevelMap(),
        mUniqueLogMessages(),
        mCrashFunction(NULL),
        mTimeFunction(NULL),
        mRecorders(),
        mRecorderMutex(),
        mShouldLogCallCounter(0)
    {
    }

    SettingsConfig::~SettingsConfig()
    {
        mRecorders.clear();
    }

	class Globals
	{
    public:
        static Globals* getInstance();
    protected:
		Globals();
	public:
		std::string mFatalMessage;

		void addCallSite(LLError::CallSite&);
		void invalidateCallSites();

        SettingsConfigPtr getSettingsConfig();

        void resetSettingsConfig();
        LLError::SettingsStoragePtr saveAndResetSettingsConfig();
        void restore(LLError::SettingsStoragePtr pSettingsStorage);
	private:
		CallSiteVector callSites;
        SettingsConfigPtr mSettingsConfig;
	};

	Globals::Globals()
		:
		callSites(),
        mSettingsConfig(new SettingsConfig())
	{
	}


    Globals* Globals::getInstance()
    {
        // According to C++11 Function-Local Initialization
        // of static variables is supposed to be thread safe
        // without risk of deadlocks.
        static Globals inst;

        return &inst;
    }

	void Globals::addCallSite(LLError::CallSite& site)
	{
		callSites.push_back(&site);
	}
	
	void Globals::invalidateCallSites()
	{
		for (LLError::CallSite* site : callSites)
		{
            site->invalidate();
		}
		
		callSites.clear();
	}

    SettingsConfigPtr Globals::getSettingsConfig()
    {
        return mSettingsConfig;
    }

    void Globals::resetSettingsConfig()
    {
        invalidateCallSites();
        mSettingsConfig = new SettingsConfig();
    }

    LLError::SettingsStoragePtr Globals::saveAndResetSettingsConfig()
    {
        LLError::SettingsStoragePtr oldSettingsConfig(mSettingsConfig.get());
        resetSettingsConfig();
        return oldSettingsConfig;
    }

    void Globals::restore(LLError::SettingsStoragePtr pSettingsStorage)
    {
        invalidateCallSites();
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
		switch (mLevel)
		{
        case LEVEL_DEBUG: mLevelString = "DEBUG";   break;
        case LEVEL_INFO:  mLevelString = "INFO";    break;
        case LEVEL_WARN:  mLevelString = "WARNING"; break;
        case LEVEL_ERROR: mLevelString = "ERROR";   break;
        default:          mLevelString = "XXX";     break;
		};

		mLocationString = llformat("%s(%d)", abbreviateFile(mFile).c_str(), mLine);
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
		mFunctionString += std::string(mFunction);

		for (int i = 0; i < tag_count; i++)
		{
            if (strchr(tags[i], ' '))
            {
                LL_ERRS() << "Space is not allowed in a log tag at " << mLocationString << LL_ENDL;
            }
			mTags[i] = tags[i];
		}

        mTagString.append("#");
        // always construct a tag sequence; will be just a single # if no tag
		for (size_t i = 0; i < mTagCount; i++)
		{
			mTagString.append(mTags[i]);
            mTagString.append("#");
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
        // On macOS, stderr from apps launched from the Finder goes to the
        // console log.  It's generally considered bad form to spam too much
        // there. That scenario can be detected by noticing that stderr is a
        // character device (S_IFCHR).

        // If stderr is a tty or a pipe, assume the user launched from the
        // command line or debugger and therefore wants to see stderr.
        if (isatty(STDERR_FILENO))
            return true;
        // not a tty, but might still be a pipe -- check
        struct stat st;
        if (fstat(STDERR_FILENO, &st) < 0)
        {
            // capture errno right away, before engaging any other operations
            auto errno_save = errno;
            // this gets called during log-system setup -- can't log yet!
            std::cerr << "shouldLogToStderr: fstat(" << STDERR_FILENO << ") failed, errno "
                      << errno_save << std::endl;
            // if we can't tell, err on the safe side and don't write stderr
            return false;
        }

        // fstat() worked: return true only if stderr is a pipe
        return ((st.st_mode & S_IFMT) == S_IFIFO);
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
	
	
	void commonInit(const std::string& user_dir, const std::string& app_dir, bool log_to_stderr = true)
	{
		Globals::getInstance()->resetSettingsConfig();

		LLError::setDefaultLevel(LLError::LEVEL_INFO);
		LLError::setAlwaysFlush(true);
		LLError::setEnabledLogTypesMask(0xFFFFFFFF);
		LLError::setTimeFunction(LLError::utcTime);

		// log_to_stderr is only false in the unit and integration tests to keep builds quieter
		if (log_to_stderr && shouldLogToStderr())
		{
			LLError::logToStderr();
		}

#if LL_WINDOWS
		LLError::RecorderPtr recordToWinDebug(new RecordToWinDebug());
		LLError::addRecorder(recordToWinDebug);
#endif

		LogControlFile& e = LogControlFile::fromDirectory(user_dir, app_dir);

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
	void initForApplication(const std::string& user_dir, const std::string& app_dir, bool log_to_stderr)
	{
		commonInit(user_dir, app_dir, log_to_stderr);
	}

	void setFatalFunction(const FatalFunction& f)
	{
		SettingsConfigPtr s = Globals::getInstance()->getSettingsConfig();
		s->mCrashFunction = f;
	}

	FatalFunction getFatalFunction()
	{
		SettingsConfigPtr s = Globals::getInstance()->getSettingsConfig();
		return s->mCrashFunction;
	}

	std::string getFatalMessage()
	{
		return Globals::getInstance()->mFatalMessage;
	}

	void setTimeFunction(TimeFunction f)
	{
		SettingsConfigPtr s = Globals::getInstance()->getSettingsConfig();
		s->mTimeFunction = f;
	}

	void setDefaultLevel(ELevel level)
	{
		Globals *g = Globals::getInstance();
		g->invalidateCallSites();
		SettingsConfigPtr s = g->getSettingsConfig();
		s->mDefaultLevel = level;
	}

	ELevel getDefaultLevel()
	{
		SettingsConfigPtr s = Globals::getInstance()->getSettingsConfig();
		return s->mDefaultLevel;
	}

	void setAlwaysFlush(bool flush)
	{
		SettingsConfigPtr s = Globals::getInstance()->getSettingsConfig();
		s->mLogAlwaysFlush = flush;
	}

	bool getAlwaysFlush()
	{
		SettingsConfigPtr s = Globals::getInstance()->getSettingsConfig();
		return s->mLogAlwaysFlush;
	}

	void setEnabledLogTypesMask(U32 mask)
	{
		SettingsConfigPtr s = Globals::getInstance()->getSettingsConfig();
		s->mEnabledLogTypesMask = mask;
	}

	U32 getEnabledLogTypesMask()
	{
		SettingsConfigPtr s = Globals::getInstance()->getSettingsConfig();
		return s->mEnabledLogTypesMask;
	}

	void setFunctionLevel(const std::string& function_name, ELevel level)
	{
		Globals *g = Globals::getInstance();
		g->invalidateCallSites();
		SettingsConfigPtr s = g->getSettingsConfig();
		s->mFunctionLevelMap[function_name] = level;
	}

	void setClassLevel(const std::string& class_name, ELevel level)
	{
		Globals *g = Globals::getInstance();
		g->invalidateCallSites();
		SettingsConfigPtr s = g->getSettingsConfig();
		s->mClassLevelMap[class_name] = level;
	}

	void setFileLevel(const std::string& file_name, ELevel level)
	{
		Globals *g = Globals::getInstance();
		g->invalidateCallSites();
		SettingsConfigPtr s = g->getSettingsConfig();
		s->mFileLevelMap[file_name] = level;
	}

	void setTagLevel(const std::string& tag_name, ELevel level)
	{
		Globals *g = Globals::getInstance();
		g->invalidateCallSites();
		SettingsConfigPtr s = g->getSettingsConfig();
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
		Globals *g = Globals::getInstance();
		g->invalidateCallSites();
		SettingsConfigPtr s = g->getSettingsConfig();
		
		s->mFunctionLevelMap.clear();
		s->mClassLevelMap.clear();
		s->mFileLevelMap.clear();
		s->mTagLevelMap.clear();
		s->mUniqueLogMessages.clear();
		
		setDefaultLevel(decodeLevel(config["default-level"]));
        if (config.has("log-always-flush"))
        {
            setAlwaysFlush(config["log-always-flush"]);
        }
        if (config.has("enabled-log-types-mask"))
        {
            setEnabledLogTypesMask(config["enabled-log-types-mask"].asInteger());
        }
        
        if (config.has("settings") && config["settings"].isArray())
        {
            LLSD sets = config["settings"];
            LLSD::array_const_iterator a, end;
            for (a = sets.beginArray(), end = sets.endArray(); a != end; ++a)
            {
                const LLSD& entry = *a;
                if (entry.isMap() && entry.size() != 0)
                {
                    ELevel level = decodeLevel(entry["level"]);

                    setLevels(s->mFunctionLevelMap, entry["functions"], level);
                    setLevels(s->mClassLevelMap, entry["classes"], level);
                    setLevels(s->mFileLevelMap, entry["files"], level);
                    setLevels(s->mTagLevelMap, entry["tags"], level);
                }
            }
        }
	}
}


namespace LLError
{
	Recorder::Recorder()
    	: mWantsTime(true)
        , mWantsTags(true)
        , mWantsLevel(true)
        , mWantsLocation(true)
        , mWantsFunctionName(true)
        , mWantsMultiline(false)
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

	// virtual 
	bool Recorder::wantsMultiline() 
	{ 
		return mWantsMultiline;
	}

    void Recorder::showTime(bool show)
    {
        mWantsTime = show;
    }
    
    void Recorder::showTags(bool show)
    {
        mWantsTags = show;
    }

    void Recorder::showLevel(bool show)
    {
        mWantsLevel = show;
    }

    void Recorder::showLocation(bool show)
    {
        mWantsLocation = show;
    }

    void Recorder::showFunctionName(bool show)
    {
        mWantsFunctionName = show;
    }

    void Recorder::showMultiline(bool show)
    {
        mWantsMultiline = show;
    }

	void addRecorder(RecorderPtr recorder)
	{
		if (!recorder)
		{
			return;
		}
		SettingsConfigPtr s = Globals::getInstance()->getSettingsConfig();
		LLMutexLock lock(&s->mRecorderMutex);
		s->mRecorders.push_back(recorder);
	}

	void removeRecorder(RecorderPtr recorder)
	{
		if (!recorder)
		{
			return;
		}
		SettingsConfigPtr s = Globals::getInstance()->getSettingsConfig();
		LLMutexLock lock(&s->mRecorderMutex);
		s->mRecorders.erase(std::remove(s->mRecorders.begin(), s->mRecorders.end(), recorder),
							s->mRecorders.end());
	}

    // Find an entry in SettingsConfig::mRecorders whose RecorderPtr points to
    // a Recorder subclass of type RECORDER. Return, not a RecorderPtr (which
    // points to the Recorder base class), but a shared_ptr<RECORDER> which
    // specifically points to the concrete RECORDER subclass instance, along
    // with a Recorders::iterator indicating the position of that entry in
    // mRecorders. The shared_ptr might be empty (operator!() returns true) if
    // there was no such RECORDER subclass instance in mRecorders.
    //
    // NOTE!!! Requires external mutex lock!!!
    template <typename RECORDER>
    std::pair<boost::shared_ptr<RECORDER>, Recorders::iterator>
    findRecorderPos(SettingsConfigPtr &s)
    {
        // Since we promise to return an iterator, use a classic iterator
        // loop.
        auto end{s->mRecorders.end()};
        for (Recorders::iterator it{s->mRecorders.begin()}; it != end; ++it)
        {
            // *it is a RecorderPtr, a shared_ptr<Recorder>. Use a
            // dynamic_pointer_cast to try to downcast to test if it's also a
            // shared_ptr<RECORDER>.
            auto ptr = boost::dynamic_pointer_cast<RECORDER>(*it);
            if (ptr)
            {
                // found the entry we want
                return { ptr, it };
            }
        }
        // dropped out of the loop without finding any such entry -- instead
        // of default-constructing Recorders::iterator (which might or might
        // not be valid), return a value that is valid but not dereferenceable.
        return { {}, end };
    }

    // Find an entry in SettingsConfig::mRecorders whose RecorderPtr points to
    // a Recorder subclass of type RECORDER. Return, not a RecorderPtr (which
    // points to the Recorder base class), but a shared_ptr<RECORDER> which
    // specifically points to the concrete RECORDER subclass instance. The
    // shared_ptr might be empty (operator!() returns true) if there was no
    // such RECORDER subclass instance in mRecorders.
    template <typename RECORDER>
    boost::shared_ptr<RECORDER> findRecorder()
    {
        SettingsConfigPtr s = Globals::getInstance()->getSettingsConfig();
        LLMutexLock lock(&s->mRecorderMutex);
        return findRecorderPos<RECORDER>(s).first;
    }

    // Remove an entry from SettingsConfig::mRecorders whose RecorderPtr
    // points to a Recorder subclass of type RECORDER. Return true if there
    // was one and we removed it, false if there wasn't one to start with.
    template <typename RECORDER>
    bool removeRecorder()
    {
        SettingsConfigPtr s = Globals::getInstance()->getSettingsConfig();
        LLMutexLock lock(&s->mRecorderMutex);
        auto found = findRecorderPos<RECORDER>(s);
        if (found.first)
        {
            s->mRecorders.erase(found.second);
        }
        return bool(found.first);
    }
}

namespace LLError
{
	void logToFile(const std::string& file_name)
	{
		// remove any previous Recorder filling this role
		removeRecorder<RecordToFile>();

		if (!file_name.empty())
		{
			boost::shared_ptr<RecordToFile> recordToFile(new RecordToFile(file_name));
			if (recordToFile->okay())
			{
				addRecorder(recordToFile);
			}
		}
	}

	std::string logFileName()
	{
		auto found = findRecorder<RecordToFile>();
		return found? found->getFilename() : std::string();
	}

    void logToStderr()
    {
        if (! findRecorder<RecordToStderr>())
        {
            RecorderPtr recordToStdErr(new RecordToStderr(stderrLogWantsTime()));
            addRecorder(recordToStdErr);
        }
    }

	void logToFixedBuffer(LLLineBuffer* fixedBuffer)
	{
		// remove any previous Recorder filling this role
		removeRecorder<RecordToFixedBuffer>();

		if (fixedBuffer)
		{
			RecorderPtr recordToFixedBuffer(new RecordToFixedBuffer(fixedBuffer));
			addRecorder(recordToFixedBuffer);
		}
	}
}

namespace
{
    std::string escapedMessageLines(const std::string& message)
    {
        std::ostringstream out;
        size_t written_out = 0;
        size_t all_content = message.length();
        size_t escape_char_index; // always relative to start of message
        // Use find_first_of to find the next character in message that needs escaping
        for ( escape_char_index = message.find_first_of("\\\n\r");
              escape_char_index != std::string::npos && written_out < all_content;
              // record what we've written this iteration, scan for next char that needs escaping
              written_out = escape_char_index + 1, escape_char_index = message.find_first_of("\\\n\r", written_out)
             )
        {
            // found a character that needs escaping, so write up to that with the escape prefix
            // note that escape_char_index is relative to the start, not to the written_out offset
            out << message.substr(written_out, escape_char_index - written_out) << '\\';

            // write out the appropriate second character in the escape sequence
            char found = message[escape_char_index];
            switch ( found )
            {
            case '\\':
                out << '\\';
                break;
            case '\n':
                out << 'n';
                break;
            case '\r':
                out << 'r';
                break;
            }
        }

        if ( written_out < all_content ) // if the loop above didn't write everything
        {
            // write whatever was left
            out << message.substr(written_out, std::string::npos);
        }
        return out.str();
    }

	void writeToRecorders(const LLError::CallSite& site, const std::string& message)
	{
        LL_PROFILE_ZONE_SCOPED_CATEGORY_LOGGING
		LLError::ELevel level = site.mLevel;
		SettingsConfigPtr s = Globals::getInstance()->getSettingsConfig();

        std::string escaped_message;

        LLMutexLock lock(&s->mRecorderMutex);
		for (LLError::RecorderPtr& r : s->mRecorders)
		{
            if (!r->enabled())
            {
                continue;
            }
            
			std::ostringstream message_stream;

			if (r->wantsTime() && s->mTimeFunction != NULL)
			{
				message_stream << s->mTimeFunction();
			}
            message_stream << " ";
            
			if (r->wantsLevel())
            {
				message_stream << site.mLevelString;
            }
            message_stream << " ";
				
			if (r->wantsTags())
			{
				message_stream << site.mTagString;
			}
            message_stream << " ";

            if (r->wantsLocation() || level == LLError::LEVEL_ERROR)
            {
                message_stream << site.mLocationString;
            }
            message_stream << " ";

			if (r->wantsFunctionName())
			{
				message_stream << site.mFunctionString;
			}
            message_stream << " : ";

            if (r->wantsMultiline())
            {
                message_stream << message;
            }
            else
            {
                if (escaped_message.empty())
                {
                    escaped_message = escapedMessageLines(message);
                }
                message_stream << escaped_message;
            }

			r->recordMessage(level, message_stream.str());
		}
	}
}

namespace {
	// We need a couple different mutexes, but we want to use the same mechanism
	// for both. Make getMutex() a template function with different instances
	// for different MutexDiscriminator values.
	enum MutexDiscriminator
	{
		LOG_MUTEX,
		STACKS_MUTEX
	};
	// Some logging calls happen very early in processing -- so early that our
	// module-static variables aren't yet initialized. getMutex() wraps a
	// function-static LLMutex so that early calls can still have a valid
	// LLMutex instance.
	template <MutexDiscriminator MTX>
	LLMutex* getMutex()
	{
		// guaranteed to be initialized the first time control reaches here
		static LLMutex sMutex;
		return &sMutex;
	}

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
}

namespace LLError
{

	bool Log::shouldLog(CallSite& site)
	{
        LL_PROFILE_ZONE_SCOPED_CATEGORY_LOGGING
		LLMutexTrylock lock(getMutex<LOG_MUTEX>(), 5);
		if (!lock.isLocked())
		{
			return false;
		}

		Globals *g = Globals::getInstance();
		SettingsConfigPtr s = g->getSettingsConfig();
		
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
		g->addCallSite(site);
		return site.mShouldLog = site.mLevel >= compareLevel;
	}


	void Log::flush(const std::ostringstream& out, const CallSite& site)
	{
        LL_PROFILE_ZONE_SCOPED_CATEGORY_LOGGING
		LLMutexTrylock lock(getMutex<LOG_MUTEX>(),5);
		if (!lock.isLocked())
		{
			return;
		}

		Globals* g = Globals::getInstance();
		SettingsConfigPtr s = g->getSettingsConfig();

		std::string message = out.str();

		if (site.mPrintOnce)
		{
			std::ostringstream message_stream;

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
			message_stream << message;
			message = message_stream.str();
		}
		
		writeToRecorders(site, message);

		if (site.mLevel == LEVEL_ERROR)
		{
			g->mFatalMessage = message;
            if (s->mCrashFunction)
            {
                s->mCrashFunction(message);
            }
		}
	}
}

namespace LLError
{
	SettingsStoragePtr saveAndResetSettings()
	{
		return Globals::getInstance()->saveAndResetSettingsConfig();
	}
	
	void restoreSettings(SettingsStoragePtr pSettingsStorage)
	{
		return Globals::getInstance()->restore(pSettingsStorage);
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
		SettingsConfigPtr s = Globals::getInstance()->getSettingsConfig();
		return s->mShouldLogCallCounter;
	}

	std::string utcTime()
	{
		time_t now = time(NULL);
		const size_t BUF_SIZE = 64;
		char time_str[BUF_SIZE];	/* Flawfinder: ignore */
		
		auto chars = strftime(time_str, BUF_SIZE, 
								  "%Y-%m-%dT%H:%M:%SZ",
								  gmtime(&now));

		return chars ? time_str : "time error";
	}
}

namespace LLError
{     
    LLCallStacks::StringVector LLCallStacks::sBuffer ;

    //static
    void LLCallStacks::push(const char* function, const int line)
    {
        LLMutexTrylock lock(getMutex<STACKS_MUTEX>(), 5);
        if (!lock.isLocked())
        {
            return;
        }

        if(sBuffer.size() > 511)
        {
            clear() ;
        }

        std::ostringstream out;
        insert(out, function, line);
        sBuffer.push_back(out.str());
    }

    //static
    void LLCallStacks::insert(std::ostream& out, const char* function, const int line)
    {
        out << function << " line " << line << " " ;
    }

    //static
    void LLCallStacks::end(const std::ostringstream& out)
    {
        LLMutexTrylock lock(getMutex<STACKS_MUTEX>(), 5);
        if (!lock.isLocked())
        {
            return;
        }

        if(sBuffer.size() > 511)
        {
            clear() ;
        }

        sBuffer.push_back(out.str());
    }

    //static
    void LLCallStacks::print()
    {
        LLMutexTrylock lock(getMutex<STACKS_MUTEX>(), 5);
        if (!lock.isLocked())
        {
            return;
        }

        if(! sBuffer.empty())
        {
            LL_INFOS() << " ************* PRINT OUT LL CALL STACKS ************* " << LL_ENDL;
            for (StringVector::const_reverse_iterator ri(sBuffer.rbegin()), re(sBuffer.rend());
                 ri != re; ++ri)
            {                  
                LL_INFOS() << (*ri) << LL_ENDL;
            }
            LL_INFOS() << " *************** END OF LL CALL STACKS *************** " << LL_ENDL;
        }

        cleanup();
    }

    //static
    void LLCallStacks::clear()
    {
        sBuffer.clear();
    }

    //static
    void LLCallStacks::cleanup()
    {
        clear();
    }

    std::ostream& operator<<(std::ostream& out, const LLStacktrace&)
    {
        return out << boost::stacktrace::stacktrace();
    }

    // LLOutOfMemoryWarning
    std::string LLUserWarningMsg::sLocalizedOutOfMemoryTitle;
    std::string LLUserWarningMsg::sLocalizedOutOfMemoryWarning;
    LLUserWarningMsg::Handler LLUserWarningMsg::sHandler;

    void LLUserWarningMsg::show(const std::string& message)
    {
        if (sHandler)
        {
            sHandler(std::string(), message);
        }
    }

    void LLUserWarningMsg::showOutOfMemory()
    {
        if (sHandler && !sLocalizedOutOfMemoryTitle.empty())
        {
            sHandler(sLocalizedOutOfMemoryTitle, sLocalizedOutOfMemoryWarning);
        }
    }

    void LLUserWarningMsg::showMissingFiles()
    {
        // Files Are missing, likely can't localize.
        const std::string error_string =
            "Second Life viewer couldn't access some of the files it needs and will be closed."
            "\n\nPlease reinstall viewer from  https://secondlife.com/support/downloads/ and "
            "contact https://support.secondlife.com if issue persists after reinstall.";
        sHandler("Missing Files", error_string);
    }

    void LLUserWarningMsg::setHandler(const LLUserWarningMsg::Handler &handler)
    {
        sHandler = handler;
    }

    void LLUserWarningMsg::setOutOfMemoryStrings(const std::string& title, const std::string& message)
    {
        sLocalizedOutOfMemoryTitle = title;
        sLocalizedOutOfMemoryWarning = message;
    }
}

void crashdriver(void (*callback)(int*))
{
    // The LLERROR_CRASH macro used to have inline code of the form:
    //int* make_me_crash = NULL;
    //*make_me_crash = 0;

    // But compilers are getting smart enough to recognize that, so we must
    // assign to an address supplied by a separate source file. We could do
    // the assignment here in crashdriver() -- but then BugSplat would group
    // all LL_ERRS() crashes as the fault of this one function, instead of
    // identifying the specific LL_ERRS() source line. So instead, do the
    // assignment in a lambda in the caller's source. We just provide the
    // nullptr target.
    callback(nullptr);
}
