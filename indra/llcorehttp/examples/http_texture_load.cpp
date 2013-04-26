/**
 * @file http_texture_load.cpp
 * @brief Texture download example for core-http library
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <map>
#if !defined(WIN32)
#include <pthread.h>
#endif

#include "linden_common.h"

#include "httpcommon.h"
#include "httprequest.h"
#include "httphandler.h"
#include "httpresponse.h"
#include "httpheaders.h"
#include "bufferarray.h"
#include "_mutex.h"

#include <curl/curl.h>
#include <openssl/crypto.h>

#include "lltimer.h"


void init_curl();
void term_curl();
unsigned long ssl_thread_id_callback(void);
void ssl_locking_callback(int mode, int type, const char * file, int line);
void usage(std::ostream & out);

// Default command line settings
static int concurrency_limit(40);
static char url_format[1024] = "http://example.com/some/path?texture_id=%s.texture";

#if defined(WIN32)

#define	strncpy(_a, _b, _c)   strncpy_s(_a, _b, _c)
#define strtok_r(_a, _b, _c)		strtok_s(_a, _b, _c)

int getopt(int argc, char * const argv[], const char *optstring);
char *optarg(NULL);
int optind(1);

#endif


// Mostly just a container for the texture IDs and fetch
// parameters....
class WorkingSet : public LLCore::HttpHandler
{
public:
	WorkingSet();
	~WorkingSet();

	bool reload(LLCore::HttpRequest *);
	
	virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response);

	void loadTextureUuids(FILE * in);
	
public:
	struct Spec
	{
		std::string		mUuid;
		int				mOffset;
		int				mLength;
	};
	typedef std::set<LLCore::HttpHandle> handle_set_t;
	typedef std::vector<Spec> texture_list_t;
	
public:
	bool						mVerbose;
	bool						mRandomRange;
	int							mMaxConcurrency;
	handle_set_t				mHandles;
	int							mRemaining;
	int							mLimit;
	int							mAt;
	std::string					mUrl;
	texture_list_t				mTextures;
	int							mErrorsApi;
	int							mErrorsHttp;
	int							mErrorsHttp404;
	int							mErrorsHttp416;
	int							mErrorsHttp500;
	int							mErrorsHttp503;
	int							mSuccesses;
	long						mByteCount;
	LLCore::HttpHeaders *		mHeaders;
};


// Gather process information while we run.  Process
// size, cpu consumed, wallclock time.

class Metrics
{
public:
	class MetricsImpl;
	
public:
	Metrics();
	~Metrics();

	void init();
	void sample();
	void term();

protected:
	MetricsImpl *		mImpl;

public:
	U64					mMaxVSZ;
	U64					mMinVSZ;
	U64					mStartWallTime;
	U64					mEndWallTime;
	U64					mStartUTime;
	U64					mEndUTime;
	U64					mStartSTime;
	U64					mEndSTime;
};


//
//
//
int main(int argc, char** argv)
{
	LLCore::HttpStatus status;
	bool do_random(false);
	bool do_verbose(false);
	
	int option(-1);
	while (-1 != (option = getopt(argc, argv, "u:c:h?Rv")))
	{
		switch (option)
		{
		case 'u':
			strncpy(url_format, optarg, sizeof(url_format));
			url_format[sizeof(url_format) - 1] = '\0';
			break;

		case 'c':
		    {
				unsigned long value;
				char * end;

				value = strtoul(optarg, &end, 10);
				if (value < 1 || value > 100 || *end != '\0')
				{
					usage(std::cerr);
					return 1;
				}
				concurrency_limit = value;
			}
			break;

		case 'R':
			do_random = true;
			break;

		case 'v':
			do_verbose = true;
			break;
			
		case 'h':
		case '?':
			usage(std::cout);
			return 0;
		}
	}

	if ((optind + 1) != argc)
	{
		usage(std::cerr);
		return 1;
	}

	FILE * uuids(fopen(argv[optind], "r"));
	if (! uuids)
	{
		const char * errstr(strerror(errno));
		
		std::cerr << "Couldn't open UUID file '" << argv[optind] << "'.  Reason:  "
				  << errstr << std::endl;
		return 1;
	}
	
	// Initialization
	init_curl();
	LLCore::HttpRequest::createService();
	LLCore::HttpRequest::setPolicyClassOption(LLCore::HttpRequest::DEFAULT_POLICY_ID,
											  LLCore::HttpRequest::CP_CONNECTION_LIMIT,
											  concurrency_limit);
	LLCore::HttpRequest::startThread();
	
	// Get service point
	LLCore::HttpRequest * hr = new LLCore::HttpRequest();

	// Get a handler/working set
	WorkingSet ws;

	// Fill the working set with work
	ws.mUrl = url_format;
	ws.loadTextureUuids(uuids);
	ws.mRandomRange = do_random;
	ws.mVerbose = do_verbose;
	ws.mMaxConcurrency = 100;
	
	if (! ws.mTextures.size())
	{
		std::cerr << "No UUIDs found in file '" << argv[optind] << "'." << std::endl;
		return 1;
	}

	// Setup metrics
	Metrics metrics;
	metrics.init();
	
	// Run it
	int passes(0);
	while (! ws.reload(hr))
	{
		hr->update(5000000);
		ms_sleep(2);
		if (0 == (++passes % 200))
		{
			metrics.sample();
		}
	}
	metrics.sample();
	metrics.term();

	// Report
	std::cout << "HTTP errors: " << ws.mErrorsHttp << "  API errors:  " << ws.mErrorsApi
			  << "  Successes:  " << ws.mSuccesses << "  Byte count:  " << ws.mByteCount
			  << std::endl;
	std::cout << "HTTP 404 errors: " << ws.mErrorsHttp404 << "  HTTP 416 errors: " << ws.mErrorsHttp416
			  << "  HTTP 500 errors:  " << ws.mErrorsHttp500 << "  HTTP 503 errors: " << ws.mErrorsHttp503 
			  << std::endl;
	std::cout << "User CPU: " << (metrics.mEndUTime - metrics.mStartUTime)
			  << " uS  System CPU: " << (metrics.mEndSTime - metrics.mStartSTime)
			  << " uS  Wall Time: "  << (metrics.mEndWallTime - metrics.mStartWallTime)
			  << " uS  Maximum VSZ: " << metrics.mMaxVSZ
			  << " Bytes  Minimum VSZ: " << metrics.mMinVSZ << " Bytes"
			  << std::endl;

	// Clean up
	hr->requestStopThread(NULL);
	ms_sleep(1000);
	delete hr;
	LLCore::HttpRequest::destroyService();
	term_curl();
	
    return 0;
}


void usage(std::ostream & out)
{
	out << "\n"
		"usage:\thttp_texture_load [options]  uuid_file\n"
		"\n"
		"This is a standalone program to drive the New Platform HTTP Library.\n"
		"The program is supplied with a file of texture UUIDs, one per line\n"
		"These are fetched sequentially using a pool of concurrent connection\n"
		"until all are fetched.  The default URL format is only useful from\n"
		"within Linden Lab but this can be overriden with a printf-style\n"
		"URL formatting string on the command line.\n"
		"\n"
		"Options:\n"
		"\n"
		" -u <url_format>       printf-style format string for URL generation\n"
		"                       Default:  " << url_format << "\n"
		" -R                    Issue GETs with random Range: headers\n"
		" -c <limit>            Maximum request concurrency.  Range:  [1..100]\n"
		"                       Default:  " << concurrency_limit << "\n"
		" -v                    Verbose mode.  Issue some chatter while running\n"
		" -h                    print this help\n"
		"\n"
		<< std::endl;
}


WorkingSet::WorkingSet()
	: LLCore::HttpHandler(),
	  mVerbose(false),
	  mRandomRange(false),
	  mRemaining(200),
	  mLimit(200),
	  mAt(0),
	  mErrorsApi(0),
	  mErrorsHttp(0),
	  mErrorsHttp404(0),
	  mErrorsHttp416(0),
	  mErrorsHttp500(0),
	  mErrorsHttp503(0),
	  mSuccesses(0),
	  mByteCount(0L)
{
	mTextures.reserve(30000);

	mHeaders = new LLCore::HttpHeaders;
	mHeaders->mHeaders.push_back("Accept: image/x-j2c");
}


WorkingSet::~WorkingSet()
{
	if (mHeaders)
	{
		mHeaders->release();
		mHeaders = NULL;
	}
}


bool WorkingSet::reload(LLCore::HttpRequest * hr)
{
	int to_do((std::min)(mRemaining, mMaxConcurrency - int(mHandles.size())));

	for (int i(0); i < to_do; ++i)
	{
		char buffer[1024];
#if	defined(WIN32)
		_snprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, mUrl.c_str(), mTextures[mAt].mUuid.c_str());
#else
		snprintf(buffer, sizeof(buffer), mUrl.c_str(), mTextures[mAt].mUuid.c_str());
#endif
		int offset(mRandomRange ? ((unsigned long) rand()) % 1000000UL : mTextures[mAt].mOffset);
		int length(mRandomRange ? ((unsigned long) rand()) % 1000000UL : mTextures[mAt].mLength);

		LLCore::HttpHandle handle;
		if (offset || length)
		{
			handle = hr->requestGetByteRange(0, 0, buffer, offset, length, NULL, mHeaders, this);
		}
		else
		{
			handle = hr->requestGet(0, 0, buffer, NULL, mHeaders, this);
		}
		if (! handle)
		{
			// Fatal.  Couldn't queue up something.
			std::cerr << "Failed to queue work to HTTP Service.  Reason:  "
					  << hr->getStatus().toString() << std::endl;
			exit(1);
		}
		else
		{
			mHandles.insert(handle);
		}
		mAt++;
		mRemaining--;

		if (mVerbose)
		{
			static int count(0);
			++count;
			if (0 == (count %5))
				std::cout << "Queued " << count << std::endl;
		}
	}

	// Are we done?
	return (! mRemaining) && mHandles.empty();
}
	

void WorkingSet::onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response)
{
	handle_set_t::iterator it(mHandles.find(handle));
	if (mHandles.end() == it)
	{
		// Wha?
		std::cerr << "Failed to find handle in request list.  Fatal." << std::endl;
		exit(1);
	}
	else
	{
		LLCore::HttpStatus status(response->getStatus());
		if (status)
		{
			// More success
			LLCore::BufferArray * data(response->getBody());
			mByteCount += data->size();
			++mSuccesses;
		}
		else
		{
			// Something in this library or libcurl
			if (status.isHttpStatus())
			{
				static const LLCore::HttpStatus hs404(404);
				static const LLCore::HttpStatus hs416(416);
				static const LLCore::HttpStatus hs500(500);
				static const LLCore::HttpStatus hs503(503);
				
				++mErrorsHttp;
				if (hs404 == status)
				{
					++mErrorsHttp404;
				}
				else if (hs416 == status)
				{
					++mErrorsHttp416;
				}
				else if (hs500 == status)
				{
					++mErrorsHttp500;
				}
				else if (hs503 == status)
				{
					++mErrorsHttp503;
				}
			}
			else
			{
				++mErrorsApi;
			}
		}
		mHandles.erase(it);
	}

	if (mVerbose)
	{
		static int count(0);
		++count;
		if (0 == (count %5))
			std::cout << "Handled " << count << std::endl;
	}
}


void WorkingSet::loadTextureUuids(FILE * in)
{
	char buffer[1024];

	while (fgets(buffer, sizeof(buffer), in))
	{
		WorkingSet::Spec texture;
		char * state(NULL);
		char * token = strtok_r(buffer, " \t\n,", &state);
		if (token && 36 == strlen(token))
		{
			// Close enough for this function
			texture.mUuid = token;
			texture.mOffset = 0;
			texture.mLength = 0;
			token = strtok_r(buffer, " \t\n,", &state);
			if (token)
			{
				int offset(atoi(token));
				token = strtok_r(buffer, " \t\n,", &state);
				if (token)
				{
					int length(atoi(token));
					texture.mOffset = offset;
					texture.mLength = length;
				}
			}
			mTextures.push_back(texture);
		}
	}
	mRemaining = mLimit = mTextures.size();
}


int ssl_mutex_count(0);
LLCoreInt::HttpMutex ** ssl_mutex_list = NULL;

void init_curl()
{
	curl_global_init(CURL_GLOBAL_ALL);

	ssl_mutex_count = CRYPTO_num_locks();
	if (ssl_mutex_count > 0)
	{
		ssl_mutex_list = new LLCoreInt::HttpMutex * [ssl_mutex_count];
		
		for (int i(0); i < ssl_mutex_count; ++i)
		{
			ssl_mutex_list[i] = new LLCoreInt::HttpMutex;
		}

		CRYPTO_set_locking_callback(ssl_locking_callback);
		CRYPTO_set_id_callback(ssl_thread_id_callback);
	}
}


void term_curl()
{
	CRYPTO_set_locking_callback(NULL);
	for (int i(0); i < ssl_mutex_count; ++i)
	{
		delete ssl_mutex_list[i];
	}
	delete [] ssl_mutex_list;
}


unsigned long ssl_thread_id_callback(void)
{
#if defined(WIN32)
	return (unsigned long) GetCurrentThread();
#else
	return (unsigned long) pthread_self();
#endif
}


void ssl_locking_callback(int mode, int type, const char * /* file */, int /* line */)
{
	if (type >= 0 && type < ssl_mutex_count)
	{
		if (mode & CRYPTO_LOCK)
		{
			ssl_mutex_list[type]->lock();
		}
		else
		{
			ssl_mutex_list[type]->unlock();
		}
	}
}


#if defined(WIN32)

// Very much a subset of posix functionality.  Don't push
// it too hard...
int getopt(int argc, char * const argv[], const char *optstring)
{
	static int pos(0);
	while (optind < argc)
	{
		if (pos == 0)
		{
			if (argv[optind][0] != '-')
				return -1;
			pos = 1;
		}
		if (! argv[optind][pos])
		{
			++optind;
			pos = 0;
			continue;
		}
		const char * thing(strchr(optstring, argv[optind][pos]));
		if (! thing)
		{
			++optind;
			return -1;
		}
		if (thing[1] == ':')
		{
			optarg = argv[++optind];
			++optind;
			pos = 0;
		}
		else
		{
			optarg = NULL;
			++pos;
		}
		return *thing;
	}
	return -1;
}

#endif



#if LL_WINDOWS

#define	PSAPI_VERSION	1
#include "windows.h"
#include "psapi.h"

class Metrics::MetricsImpl
{
public:
	MetricsImpl()
		{}

	~MetricsImpl()
		{}

	void init(Metrics * metrics)
		{
			HANDLE self(GetCurrentProcess());		// Does not have to be closed
			FILETIME ft_dummy, ft_system, ft_user;
			GetProcessTimes(self, &ft_dummy, &ft_dummy, &ft_system, &ft_user);
			ULARGE_INTEGER uli;
			uli.u.LowPart = ft_system.dwLowDateTime;
			uli.u.HighPart = ft_system.dwHighDateTime;
			metrics->mStartSTime = uli.QuadPart / U64L(10);		// Convert to uS
			uli.u.LowPart = ft_user.dwLowDateTime;
			uli.u.HighPart = ft_user.dwHighDateTime;
			metrics->mStartUTime = uli.QuadPart / U64L(10);
			metrics->mStartWallTime = totalTime();
		}

	void sample(Metrics * metrics)
		{
			PROCESS_MEMORY_COUNTERS_EX	counters;

			GetProcessMemoryInfo(GetCurrentProcess(),
								 (PROCESS_MEMORY_COUNTERS *) &counters,
								 sizeof(counters));
			// Okay, PrivateUsage isn't truly VSZ but it will be
			// a good tracker for leaks and fragmentation.  Work on
			// a better estimator later...
			SIZE_T vsz(counters.PrivateUsage);
			metrics->mMaxVSZ = (std::max)(metrics->mMaxVSZ, U64(vsz));
			metrics->mMinVSZ = (std::min)(metrics->mMinVSZ, U64(vsz));
		}

	void term(Metrics * metrics)
		{
			HANDLE self(GetCurrentProcess());		// Does not have to be closed
			FILETIME ft_dummy, ft_system, ft_user;
			GetProcessTimes(self, &ft_dummy, &ft_dummy, &ft_system, &ft_user);
			ULARGE_INTEGER uli;
			uli.u.LowPart = ft_system.dwLowDateTime;
			uli.u.HighPart = ft_system.dwHighDateTime;
			metrics->mEndSTime = uli.QuadPart / U64L(10);
			uli.u.LowPart = ft_user.dwLowDateTime;
			uli.u.HighPart = ft_user.dwHighDateTime;
			metrics->mEndUTime = uli.QuadPart / U64L(10);
			metrics->mEndWallTime = totalTime();
		}

protected:
};

#elif LL_DARWIN

#include <sys/resource.h>
#include <mach/mach.h>
	
class Metrics::MetricsImpl
{
public:
	MetricsImpl()
		{}

	~MetricsImpl()
		{}

	void init(Metrics * metrics)
		{
			U64 utime, stime;

			if (getTimes(&utime, &stime))
			{
				metrics->mStartSTime = stime;
				metrics->mStartUTime = utime;
			}
			metrics->mStartWallTime = totalTime();
			sample(metrics);
		}

	void sample(Metrics * metrics)
		{
			U64 vsz;
			
			if (getVM(&vsz))
			{
				metrics->mMaxVSZ = (std::max)(metrics->mMaxVSZ, vsz);
				metrics->mMinVSZ = (std::min)(metrics->mMinVSZ, vsz);
			}
		}

	void term(Metrics * metrics)
		{
			U64 utime, stime;

			if (getTimes(&utime, &stime))
			{
				metrics->mEndSTime = stime;
				metrics->mEndUTime = utime;
			}
			metrics->mEndWallTime = totalTime();
		}

protected:
	bool getVM(U64 * vsz)
		{
			task_basic_info				task_info_block;
			mach_msg_type_number_t		task_info_count(TASK_BASIC_INFO_COUNT);

			if (KERN_SUCCESS != task_info(mach_task_self(),
										  TASK_BASIC_INFO,
										  (task_info_t) &task_info_block,
										  &task_info_count))
			{
				return false;
			}
			* vsz = task_info_block.virtual_size;
			return true;
		}

	bool getTimes(U64 * utime, U64 * stime)
		{
			struct rusage usage;

			if (getrusage(RUSAGE_SELF, &usage))
			{
				return false;
			}
			* utime = U64(usage.ru_utime.tv_sec) * U64L(1000000) + usage.ru_utime.tv_usec;
			* stime = U64(usage.ru_stime.tv_sec) * U64L(1000000) + usage.ru_stime.tv_usec;
			return true;
		}
	
};

#else

class Metrics::MetricsImpl
{
public:
	MetricsImpl()
		: mProcFS(NULL),
		  mUsecsPerTick(U64L(0))
		{}

	
	~MetricsImpl()
		{
			if (mProcFS)
			{
				fclose(mProcFS);
				mProcFS = NULL;
			}
		}

	void init(Metrics * metrics)
		{
			if (! mProcFS)
			{
				mProcFS = fopen("/proc/self/stat", "r");
				if (! mProcFS)
				{
					const int errnum(errno);
					LL_ERRS("Main") << "Error opening proc fs:  " << strerror(errnum) << LL_ENDL;
				}
			}

			long ticks_per_sec(sysconf(_SC_CLK_TCK));
			mUsecsPerTick = U64L(1000000) / ticks_per_sec;
			U64 usecs_per_sec(mUsecsPerTick * ticks_per_sec);
			if (900000 > usecs_per_sec || 1100000 < usecs_per_sec)
			{
				LL_ERRS("Main") << "Resolution problems using uSecs for ticks" << LL_ENDL;
			}

			U64 utime, stime;
			if (scanProcFS(&utime, &stime, NULL))
			{
				metrics->mStartSTime = stime;
				metrics->mStartUTime = utime;
			}
			metrics->mStartWallTime = totalTime();

			sample(metrics);
		}


	void sample(Metrics * metrics)
		{
			U64 vsz;
			if (scanProcFS(NULL, NULL, &vsz))
			{
				metrics->mMaxVSZ = (std::max)(metrics->mMaxVSZ, vsz);
				metrics->mMinVSZ = (std::min)(metrics->mMinVSZ, vsz);
			}
		}


	void term(Metrics * metrics)
		{
			U64 utime, stime;
			if (scanProcFS(&utime, &stime, NULL))
			{
				metrics->mEndSTime = stime;
				metrics->mEndUTime = utime;
			}
			metrics->mEndWallTime = totalTime();

			sample(metrics);
			
			if (mProcFS)
			{
				fclose(mProcFS);
				mProcFS = NULL;
			}
		}
	
protected:
	bool scanProcFS(U64 * utime, U64 * stime, U64 * vsz)
		{
			if (mProcFS)
			{
				int i_dummy;
				unsigned int ui_dummy;
				unsigned long ul_dummy, user_ticks, sys_ticks, vsize;
				long l_dummy, rss;
				unsigned long long ull_dummy;
				char c_dummy;
				
				char buffer[256];
				
				static const char * format("%d %*s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %llu %lu %ld");

				fseek(mProcFS, 0L, SEEK_SET);
				size_t len = fread(buffer, 1, sizeof(buffer) - 1, mProcFS);
				if (! len)
				{
					return false;
				}
				buffer[len] = '\0';
				if (23 == sscanf(buffer, format,
								 &i_dummy,				// pid
								 // &s_dummy,			// command name
								 &c_dummy,				// state
								 &i_dummy,				// ppid
								 &i_dummy,				// pgrp
								 &i_dummy,				// session
								 &i_dummy,				// terminal
								 &i_dummy,				// terminal group id
								 &ui_dummy,				// flags
								 &ul_dummy,				// minor faults
								 &ul_dummy,				// minor faults in children
								 &ul_dummy,				// major faults
								 &ul_dummy,				// major faults in children
								 &user_ticks,
								 &sys_ticks,
								 &l_dummy,				// cutime
								 &l_dummy,				// cstime
								 &l_dummy,				// process priority
								 &l_dummy,				// nice value
								 &l_dummy,				// thread count
								 &l_dummy,				// time to SIGALRM
								 &ull_dummy,			// start time
								 &vsize,
								 &rss))
				{
					// Looks like we understand the line
					if (utime)
					{
						*utime = user_ticks * mUsecsPerTick;
					}

					if (stime)
					{
						*stime = sys_ticks * mUsecsPerTick;
					}
					
					if (vsz)
					{
						*vsz = vsize;
					}
					return true;
				}
			}
			return false;
		}
	
protected:
	FILE *		mProcFS;
	U64			mUsecsPerTick;
	
};


#endif  // LL_WINDOWS

Metrics::Metrics()
	: mMaxVSZ(U64(0)),
	  mMinVSZ(U64L(0xffffffffffffffff)),
	  mStartWallTime(U64(0)),
	  mEndWallTime(U64(0)),
	  mStartUTime(U64(0)),
	  mEndUTime(U64(0)),
	  mStartSTime(U64(0)),
	  mEndSTime(U64(0))
{
	mImpl = new MetricsImpl();
}


Metrics::~Metrics()
{
	delete mImpl;
	mImpl = NULL;
}


void Metrics::init()
{
	mImpl->init(this);
}


void Metrics::sample()
{
	mImpl->sample(this);
}


void Metrics::term()
{
	mImpl->term(this);
}

	
