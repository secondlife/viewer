/**
 * @file lldatapacker_tut.cpp
 * @date 2007-04
 * @brief LLDataPacker test cases.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include <tut/tut.hpp>
#include "linden_common.h"
#include "lltut.h"

#include "llapr.h"
#include "llmessageconfig.h"
#include "llsdserialize.h"
#include "llversionserver.h"
#include "message.h"
#include "message_prehash.h"

namespace
{
	struct Response : public LLHTTPNode::Response
	{
		virtual void result(const LLSD&) {}
		virtual void status(S32 code, const std::string& message) 
		{
			mStatus = code;
		}
		S32 mStatus;
	};
}

namespace tut
{	
	struct LLMessageSystemTestData 
	{
		std::string mTestConfigDir;
		std::string mSep;

		LLMessageSystemTestData()
		{
			static bool init = false;
			if(!init)
			{
				ll_init_apr();
				//init_prehash_data();
				init = true;
			}
			const F32 circuit_heartbeat_interval=5;
			const F32 circuit_timeout=100;


			// currently test disconnected message system
			start_messaging_system("notafile", 13035,
								   LL_VERSION_MAJOR,
								   LL_VERSION_MINOR,        
								   LL_VERSION_PATCH,        
								   FALSE,        
								   "notasharedsecret",
								   NULL,
								   false,
								   circuit_heartbeat_interval,
								   circuit_timeout
								   );
			// generate temp dir
			std::ostringstream ostr;
#if LL_WINDOWS
			mSep = "\\";
			ostr << "C:" << mSep;
#else
			mSep = "/";
			ostr << mSep << "tmp" << mSep;
#endif
			LLUUID random;
			random.generate();
			ostr << "message-test-" << random;
			mTestConfigDir = ostr.str();
			LLFile::mkdir(mTestConfigDir);
			writeConfigFile(LLSD());
			LLMessageConfig::initClass("simulator", ostr.str());
		}

		~LLMessageSystemTestData()
		{
			// not end_messaging_system()
			delete gMessageSystem;
			gMessageSystem = NULL;

			// rm contents of temp dir
			std::ostringstream ostr;
			ostr << mTestConfigDir << mSep << "message.xml";
			int rmfile = LLFile::remove(ostr.str());
			ensure_equals("rmfile value", rmfile, 0);

			// rm temp dir
			int rmdir = LLFile::rmdir(mTestConfigDir);
			ensure_equals("rmdir value", rmdir, 0);
		}

		void writeConfigFile(const LLSD& config)
		{
			std::ostringstream ostr;
			ostr << mTestConfigDir << mSep << "message.xml";
			llofstream file(ostr.str());
			if (file.is_open())
			{
				LLSDSerialize::toPrettyXML(config, file);
			}
			file.close();
		}
	};
	
	typedef test_group<LLMessageSystemTestData>	LLMessageSystemTestGroup;
	typedef LLMessageSystemTestGroup::object		LLMessageSystemTestObject;
	LLMessageSystemTestGroup messageTestGroup("LLMessageSystem");
	
	template<> template<>
	void LLMessageSystemTestObject::test<1>()
		// dispatch unknown message
	{
		const char* name = "notamessasge";
		const LLSD message;
		const LLPointer<Response> response = new Response();
		gMessageSystem->dispatch(name, message, response);
		ensure_equals(response->mStatus, 404);
	}
}

