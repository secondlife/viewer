/**
 * @file lldatapacker_tut.cpp
 * @date 2007-04
 * @brief LLDataPacker test cases.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
		virtual void extendedResult(S32 code, const std::string& message, const LLSD& headers) { }
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

