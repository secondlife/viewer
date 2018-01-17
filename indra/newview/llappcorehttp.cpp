/**
 * @file llappcorehttp.cpp
 * @brief 
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2014, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llappcorehttp.h"

#include "llappviewer.h"
#include "llviewercontrol.h"
#include "llexception.h"
#include "stringize.h"

#include <openssl/x509_vfy.h>
#include <openssl/ssl.h>
#include "llsecapi.h"
#include <curl/curl.h>

#include "llcorehttputil.h"
#include "httpstats.h"

// Here is where we begin to get our connection usage under control.
// This establishes llcorehttp policy classes that, among other
// things, limit the maximum number of connections to outside
// services.  Each of the entries below maps to a policy class and
// has a limit, sometimes configurable, of how many connections can
// be open at a time.

const F64 LLAppCoreHttp::MAX_THREAD_WAIT_TIME(10.0);
const long LLAppCoreHttp::PIPELINING_DEPTH(5L);

//  Default and dynamic values for classes
static const struct
{
	U32							mDefault;
	U32							mMin;
	U32							mMax;
	U32							mRate;
	bool						mPipelined;
	std::string					mKey;
	const char *				mUsage;
} init_data[LLAppCoreHttp::AP_COUNT] =
{
	{ // AP_DEFAULT
		8,		8,		8,		0,		false,
		"",
		"other"
	},
	{ // AP_TEXTURE
		8,		1,		12,		0,		true,
		"TextureFetchConcurrency",
		"texture fetch"
	},
	{ // AP_MESH1
		32,		1,		128,	0,		false,
		"MeshMaxConcurrentRequests",
		"mesh fetch"
	},
	{ // AP_MESH2
		8,		1,		32,		0,		true,	
		"Mesh2MaxConcurrentRequests",
		"mesh2 fetch"
	},
	{ // AP_LARGE_MESH
		2,		1,		8,		0,		false,
		"",
		"large mesh fetch"
	},
	{ // AP_UPLOADS 
		2,		1,		8,		0,		false,
		"",
		"asset upload"
	},
	{ // AP_LONG_POLL
		32,		32,		32,		0,		false,
		"",
		"long poll"
	},
	{ // AP_INVENTORY
		4,		1,		4,		0,		false,
		"",
		"inventory"
	},
	{ // AP_MATERIALS
		2,		1,		8,		0,		false,
		"RenderMaterials",
		"material manager requests"
	},
	{ // AP_AGENT
		2,		1,		32,		0,		false,
		"Agent",
		"Agent requests"
	}
};

static void setting_changed();


LLAppCoreHttp::HttpClass::HttpClass()
	: mPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID),
	  mConnLimit(0U),
	  mPipelined(false)
{}


LLAppCoreHttp::LLAppCoreHttp()
	: mRequest(NULL),
	  mStopHandle(LLCORE_HTTP_HANDLE_INVALID),
	  mStopRequested(0.0),
	  mStopped(false),
	  mPipelined(true)
{}


LLAppCoreHttp::~LLAppCoreHttp()
{
	delete mRequest;
	mRequest = NULL;
}


void LLAppCoreHttp::init()
{
    LLCoreHttpUtil::setPropertyMethods(
        boost::bind(&LLControlGroup::getBOOL, boost::ref(gSavedSettings), _1),
        boost::bind(&LLControlGroup::declareBOOL, boost::ref(gSavedSettings), _1, _2, _3, LLControlVariable::PERSIST_NONDFT));

    LLCore::LLHttp::initialize();

	LLCore::HttpStatus status = LLCore::HttpRequest::createService();
	if (! status)
	{
		LL_ERRS("Init") << "Failed to initialize HTTP services.  Reason:  " << status.toString()
						<< LL_ENDL;
	}

	// Point to our certs or SSH/https: will fail on connect
	status = LLCore::HttpRequest::setStaticPolicyOption(LLCore::HttpRequest::PO_CA_FILE,
														LLCore::HttpRequest::GLOBAL_POLICY_ID,
														gDirUtilp->getCAFile(), NULL);
	if (! status)
	{
		LL_ERRS("Init") << "Failed to set CA File for HTTP services.  Reason:  " << status.toString()
						<< LL_ENDL;
	}

	// Establish HTTP Proxy, if desired.
	status = LLCore::HttpRequest::setStaticPolicyOption(LLCore::HttpRequest::PO_LLPROXY,
														LLCore::HttpRequest::GLOBAL_POLICY_ID,
														1, NULL);
	if (! status)
	{
		LL_WARNS("Init") << "Failed to set HTTP proxy for HTTP services.  Reason:  " << status.toString()
						 << LL_ENDL;
	}

	// Set up SSL Verification call back.
	status = LLCore::HttpRequest::setStaticPolicyOption(LLCore::HttpRequest::PO_SSL_VERIFY_CALLBACK,
														LLCore::HttpRequest::GLOBAL_POLICY_ID,
														sslVerify, NULL);
	if (!status)
	{
		LL_WARNS("Init") << "Failed to set SSL Verification.  Reason:  " << status.toString() << LL_ENDL;
	}

	// Tracing levels for library & libcurl (note that 2 & 3 are beyond spammy):
	// 0 - None
	// 1 - Basic start, stop simple transitions
	// 2 - libcurl CURLOPT_VERBOSE mode with brief lines
	// 3 - with partial data content
	static const std::string http_trace("QAModeHttpTrace");
	if (gSavedSettings.controlExists(http_trace))
	{
		long trace_level(0L);
		trace_level = long(gSavedSettings.getU32(http_trace));
		status = LLCore::HttpRequest::setStaticPolicyOption(LLCore::HttpRequest::PO_TRACE,
															LLCore::HttpRequest::GLOBAL_POLICY_ID,
															trace_level, NULL);
	}
	
	// Setup default policy and constrain if directed to
	mHttpClasses[AP_DEFAULT].mPolicy = LLCore::HttpRequest::DEFAULT_POLICY_ID;

	// Setup additional policies based on table and some special rules
	llassert(LL_ARRAY_SIZE(init_data) == AP_COUNT);
	for (int i(0); i < LL_ARRAY_SIZE(init_data); ++i)
	{
		const EAppPolicy app_policy(static_cast<EAppPolicy>(i));

		if (AP_DEFAULT == app_policy)
		{
			// Pre-created
			continue;
		}

		mHttpClasses[app_policy].mPolicy = LLCore::HttpRequest::createPolicyClass();
		// We have run out of available HTTP policies. Adjust HTTP_POLICY_CLASS_LIMIT in _httpinternal.h
		llassert(mHttpClasses[app_policy].mPolicy != LLCore::HttpRequest::INVALID_POLICY_ID);
		if (! mHttpClasses[app_policy].mPolicy)
		{
			// Use default policy (but don't accidentally modify default)
			LL_WARNS("Init") << "Failed to create HTTP policy class for " << init_data[i].mUsage
							 << ".  Using default policy."
							 << LL_ENDL;
			mHttpClasses[app_policy].mPolicy = mHttpClasses[AP_DEFAULT].mPolicy;
			continue;
		}
	}

	// Need a request object to handle dynamic options before setting them
	mRequest = new LLCore::HttpRequest;

	// Apply initial settings
	refreshSettings(true);
	
	// Kick the thread
	status = LLCore::HttpRequest::startThread();
	if (! status)
	{
		LL_ERRS("Init") << "Failed to start HTTP servicing thread.  Reason:  " << status.toString()
						<< LL_ENDL;
	}

	// Signal for global pipelining preference from settings
	static const std::string http_pipelining("HttpPipelining");
	if (gSavedSettings.controlExists(http_pipelining))
	{
		LLPointer<LLControlVariable> cntrl_ptr = gSavedSettings.getControl(http_pipelining);
		if (cntrl_ptr.isNull())
		{
			LL_WARNS("Init") << "Unable to set signal on global setting '" << http_pipelining
							 << "'" << LL_ENDL;
		}
		else
		{
			mPipelinedSignal = cntrl_ptr->getCommitSignal()->connect(boost::bind(&setting_changed));
		}
	}

	// Register signals for settings and state changes
	for (int i(0); i < LL_ARRAY_SIZE(init_data); ++i)
	{
		const EAppPolicy app_policy(static_cast<EAppPolicy>(i));

		if (! init_data[i].mKey.empty() && gSavedSettings.controlExists(init_data[i].mKey))
		{
			LLPointer<LLControlVariable> cntrl_ptr = gSavedSettings.getControl(init_data[i].mKey);
			if (cntrl_ptr.isNull())
			{
				LL_WARNS("Init") << "Unable to set signal on global setting '" << init_data[i].mKey
								 << "'" << LL_ENDL;
			}
			else
			{
				mHttpClasses[app_policy].mSettingsSignal = cntrl_ptr->getCommitSignal()->connect(boost::bind(&setting_changed));
			}
		}
	}
}


void setting_changed()
{
	LLAppViewer::instance()->getAppCoreHttp().refreshSettings(false);
}

namespace
{
    // The NoOpDeletor is used when wrapping LLAppCoreHttp in a smart pointer below for
    // passage into the LLCore::Http libararies.  When the smart pointer is destroyed, 
    // no action will be taken since we do not in this case want the entire LLAppCoreHttp object
    // to be destroyed at the end of the call.
    // 
    // *NOTE$: Yes! It is "Deletor" 
    // http://english.stackexchange.com/questions/4733/what-s-the-rule-for-adding-er-vs-or-when-nouning-a-verb
    // "delete" derives from Latin "deletus"
    void NoOpDeletor(LLCore::HttpHandler *)
    { /*NoOp*/ }
}

void LLAppCoreHttp::requestStop()
{
	llassert_always(mRequest);

	mStopHandle = mRequest->requestStopThread(LLCore::HttpHandler::ptr_t(this, NoOpDeletor));
	if (LLCORE_HTTP_HANDLE_INVALID != mStopHandle)
	{
		mStopRequested = LLTimer::getTotalSeconds();
	}
}


void LLAppCoreHttp::cleanup()
{
    LLCore::HTTPStats::instance().dumpStats();

	if (LLCORE_HTTP_HANDLE_INVALID == mStopHandle)
	{
		// Should have been started already...
		requestStop();
	}
	
	if (LLCORE_HTTP_HANDLE_INVALID == mStopHandle)
	{
		LL_WARNS("Cleanup") << "Attempting to cleanup HTTP services without thread shutdown"
							<< LL_ENDL;
	}
	else
	{
		while (! mStopped && LLTimer::getTotalSeconds() < (mStopRequested + MAX_THREAD_WAIT_TIME))
		{
			mRequest->update(200000);
			ms_sleep(50);
		}
		if (! mStopped)
		{
			LL_WARNS("Cleanup") << "Attempting to cleanup HTTP services with thread shutdown incomplete"
								<< LL_ENDL;
		}
	}

	for (int i(0); i < LL_ARRAY_SIZE(mHttpClasses); ++i)
	{
		mHttpClasses[i].mSettingsSignal.disconnect();
	}
	mPipelinedSignal.disconnect();
	
	delete mRequest;
	mRequest = NULL;

	LLCore::HttpStatus status = LLCore::HttpRequest::destroyService();
	if (! status)
	{
		LL_WARNS("Cleanup") << "Failed to shutdown HTTP services, continuing.  Reason:  "
							<< status.toString()
							<< LL_ENDL;
	}
}


void LLAppCoreHttp::refreshSettings(bool initial)
{
	LLCore::HttpStatus status;

	// Global pipelining setting
	bool pipeline_changed(false);
	static const std::string http_pipelining("HttpPipelining");
	if (gSavedSettings.controlExists(http_pipelining))
	{
		// Default to true (in ctor) if absent.
		bool pipelined(gSavedSettings.getBOOL(http_pipelining));
		if (pipelined != mPipelined)
		{
			mPipelined = pipelined;
			pipeline_changed = true;
		}
        LL_INFOS("Init") << "HTTP Pipelining " << (mPipelined ? "enabled" : "disabled") << "!" << LL_ENDL;
	}
	
	for (int i(0); i < LL_ARRAY_SIZE(init_data); ++i)
	{
		const EAppPolicy app_policy(static_cast<EAppPolicy>(i));

		if (initial)
		{
			// Init-time only settings, can use the static setters here

			if (init_data[i].mRate)
			{
				// Set any desired throttle
				status = LLCore::HttpRequest::setStaticPolicyOption(LLCore::HttpRequest::PO_THROTTLE_RATE,
																	mHttpClasses[app_policy].mPolicy,
																	init_data[i].mRate,
																	NULL);
				if (! status)
				{
					LL_WARNS("Init") << "Unable to set " << init_data[i].mUsage
									 << " throttle rate.  Reason:  " << status.toString()
									 << LL_ENDL;
				}
			}

		}

		// Init- or run-time settings.  Must use the queued request API.

		// Pipelining changes
		if (initial || pipeline_changed)
		{
			const bool to_pipeline(mPipelined && init_data[i].mPipelined);
			if (to_pipeline != mHttpClasses[app_policy].mPipelined)
			{
				// Pipeline election changing, set dynamic option via request

				LLCore::HttpHandle handle;
				const long new_depth(to_pipeline ? PIPELINING_DEPTH : 0);
				
				handle = mRequest->setPolicyOption(LLCore::HttpRequest::PO_PIPELINING_DEPTH,
												   mHttpClasses[app_policy].mPolicy,
												   new_depth,
                                                   LLCore::HttpHandler::ptr_t());
				if (LLCORE_HTTP_HANDLE_INVALID == handle)
				{
					status = mRequest->getStatus();
					LL_WARNS("Init") << "Unable to set " << init_data[i].mUsage
									 << " pipelining.  Reason:  " << status.toString()
									 << LL_ENDL;
				}
				else
				{
					LL_DEBUGS("Init") << "Changed " << init_data[i].mUsage
									  << " pipelining.  New value:  " << new_depth
									  << LL_ENDL;
					mHttpClasses[app_policy].mPipelined = to_pipeline;
				}
			}
		}
		
		// Get target connection concurrency value
		U32 setting(init_data[i].mDefault);
		if (! init_data[i].mKey.empty() && gSavedSettings.controlExists(init_data[i].mKey))
		{
			U32 new_setting(gSavedSettings.getU32(init_data[i].mKey));
			if (new_setting)
			{
				// Treat zero settings as an ask for default
				setting = llclamp(new_setting, init_data[i].mMin, init_data[i].mMax);
			}
		}

		if (initial || setting != mHttpClasses[app_policy].mConnLimit || pipeline_changed)
		{
			// Set it and report.  Strategies depend on pipelining:
			//
			// No Pipelining.  Llcorehttp manages connections itself based
			// on the PO_CONNECTION_LIMIT setting.  Set both limits to the
			// same value for logical consistency.  In the future, may
			// hand over connection management to libcurl after the
			// connection cache has been better vetted.
			//
			// Pipelining.  Libcurl is allowed to manage connections to a
			// great degree.  Steady state will connection limit based on
			// the per-host setting.  Transitions (region crossings, new
			// avatars, etc.) can request additional outbound connections
			// to other servers via 2X total connection limit.
			//
			LLCore::HttpHandle handle;
			handle = mRequest->setPolicyOption(LLCore::HttpRequest::PO_CONNECTION_LIMIT,
											   mHttpClasses[app_policy].mPolicy,
											   (mHttpClasses[app_policy].mPipelined ? 2 * setting : setting),
                                               LLCore::HttpHandler::ptr_t());
			if (LLCORE_HTTP_HANDLE_INVALID == handle)
			{
				status = mRequest->getStatus();
				LL_WARNS("Init") << "Unable to set " << init_data[i].mUsage
								 << " concurrency.  Reason:  " << status.toString()
								 << LL_ENDL;
			}
			else
			{
				handle = mRequest->setPolicyOption(LLCore::HttpRequest::PO_PER_HOST_CONNECTION_LIMIT,
												   mHttpClasses[app_policy].mPolicy,
												   setting,
                                                   LLCore::HttpHandler::ptr_t());
				if (LLCORE_HTTP_HANDLE_INVALID == handle)
				{
					status = mRequest->getStatus();
					LL_WARNS("Init") << "Unable to set " << init_data[i].mUsage
									 << " per-host concurrency.  Reason:  " << status.toString()
									 << LL_ENDL;
				}
				else
				{
					LL_DEBUGS("Init") << "Changed " << init_data[i].mUsage
									  << " concurrency.  New value:  " << setting
									  << LL_ENDL;
					mHttpClasses[app_policy].mConnLimit = setting;
					if (initial && setting != init_data[i].mDefault)
					{
						LL_INFOS("Init") << "Application settings overriding default " << init_data[i].mUsage
										 << " concurrency.  New value:  " << setting
										 << LL_ENDL;
					}
				}
			}
		}
	}
}

LLCore::HttpStatus LLAppCoreHttp::sslVerify(const std::string &url, 
	const LLCore::HttpHandler::ptr_t &handler, void *appdata)
{
	X509_STORE_CTX *ctx = static_cast<X509_STORE_CTX *>(appdata);
	LLCore::HttpStatus result;
	LLPointer<LLCertificateStore> store = gSecAPIHandler->getCertificateStore("");
	LLPointer<LLCertificateChain> chain = gSecAPIHandler->getCertificateChain(ctx);
	LLSD validation_params = LLSD::emptyMap();
	LLURI uri(url);

	validation_params[CERT_HOSTNAME] = uri.hostName();

	// *TODO: In the case of an exception while validating the cert, we need a way
	// to pass the offending(?) cert back out. *Rider*

	try
	{
		// don't validate hostname.  Let libcurl do it instead.  That way, it'll handle redirects
		store->validate(VALIDATION_POLICY_SSL & (~VALIDATION_POLICY_HOSTNAME), chain, validation_params);
	}
	catch (LLCertValidationTrustException &cert_exception)
	{
		// this exception is is handled differently than the general cert
		// exceptions, as we allow the user to actually add the certificate
		// for trust.
		// therefore we pass back a different error code
		// NOTE: We're currently 'wired' to pass around CURL error codes.  This is
		// somewhat clumsy, as we may run into errors that do not map directly to curl
		// error codes.  Should be refactored with login refactoring, perhaps.
		result = LLCore::HttpStatus(LLCore::HttpStatus::EXT_CURL_EASY, CURLE_SSL_CACERT);
		result.setMessage(cert_exception.what());
		LLPointer<LLCertificate> cert = cert_exception.getCert();
		cert->ref(); // adding an extra ref here
		result.setErrorData(cert.get());
		// We should probably have a more generic way of passing information
		// back to the error handlers.
	}
	catch (LLCertException &cert_exception)
	{
		result = LLCore::HttpStatus(LLCore::HttpStatus::EXT_CURL_EASY, CURLE_SSL_PEER_CERTIFICATE);
		result.setMessage(cert_exception.what());
		LLPointer<LLCertificate> cert = cert_exception.getCert();
		cert->ref(); // adding an extra ref here
		result.setErrorData(cert.get());
	}
	catch (...)
	{
		LOG_UNHANDLED_EXCEPTION(STRINGIZE("('" << url << "')"));
		// any other odd error, we just handle as a connect error.
		result = LLCore::HttpStatus(LLCore::HttpStatus::EXT_CURL_EASY, CURLE_SSL_CONNECT_ERROR);
	}

	return result;
}




void LLAppCoreHttp::onCompleted(LLCore::HttpHandle, LLCore::HttpResponse *)
{
	mStopped = true;
}
