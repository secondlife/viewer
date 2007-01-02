/** 
 * @file llclassifiedinfo.cpp
 * @brief LLClassifiedInfo class definition
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"
#include "llclassifiedinfo.h"

#include "viewer.h"	// for gPacificDaylightTime
#include "lluuid.h"

LLClassifiedInfo::cat_map LLClassifiedInfo::sCategories;

// static
void LLClassifiedInfo::loadCategories(LLUserAuth::options_t classified_options)
{
	LLUserAuth::options_t::iterator resp_it;
	for (resp_it = classified_options.begin(); 
		 resp_it != classified_options.end(); 
		 ++resp_it)
	{
		const LLUserAuth::response_t& response = *resp_it;

		LLUserAuth::response_t::const_iterator option_it;

		S32 cat_id = 0;
		option_it = response.find("category_id");
		if (option_it != response.end())
		{
			cat_id = atoi(option_it->second.c_str());
		}
		else
		{
			continue;
		}

		// Add the category id/name pair
		option_it = response.find("category_name");
		if (option_it != response.end())
		{
			LLClassifiedInfo::sCategories[cat_id] = option_it->second;
		}

	}

}
