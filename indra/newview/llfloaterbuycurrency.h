/** 
 * @file llfloaterbuycurrency.h
 * @brief LLFloaterBuyCurrency class definition
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERBUYCURRENCY_H
#define LL_LLFLOATERBUYCURRENCY_H

#include "stdtypes.h"

class LLFloaterBuyCurrency
{
public:
	static void buyCurrency();
	static void buyCurrency(const std::string& name, S32 price);
		/* name should be a noun phrase of the object or service being bought:
				"That object costs"
				"Trying to give"
				"Uploading costs"
			a space and the price will be appended
		*/
};


#endif
