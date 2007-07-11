/**
 * @file lldatapacker_tut.cpp
 * @date 2007-04
 * @brief LLDataPacker test cases.
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include <tut/tut.h>
#include "lltut.h"

#include "llapr.h"
#include "llversion.h"
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
		LLMessageSystemTestData()
		{
			static bool init = false;
			if(! init)
			{
				ll_init_apr();
				init_prehash_data();
				init = true;
			}

			// currently test disconnected message system
			start_messaging_system("notafile", 13035,
								   LL_VERSION_MAJOR,
								   LL_VERSION_MINOR,        
								   LL_VERSION_PATCH,        
								   FALSE,        
								   "notasharedsecret");
		}

		~LLMessageSystemTestData()
		{
			// not end_messaging_system()
			delete gMessageSystem;
			gMessageSystem = NULL;
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

