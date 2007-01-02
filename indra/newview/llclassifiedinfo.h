/** 
 * @file llclassifiedinfo.h
 * @brief LLClassifiedInfo class definition
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLCLASSIFIEDINFO_H
#define LL_LLCLASSIFIEDINFO_H

#include <map>

#include "v3dmath.h"
#include "lluuid.h"
#include "lluserauth.h"

class LLMessageSystem;

class LLClassifiedInfo
{
public:
	LLClassifiedInfo() {}

	static void loadCategories(LLUserAuth::options_t event_options);

	typedef std::map<U32, std::string> cat_map;
	static	cat_map sCategories;
};

#endif // LL_LLCLASSIFIEDINFO_H
