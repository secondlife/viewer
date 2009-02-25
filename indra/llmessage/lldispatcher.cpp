/** 
 * @file lldispatcher.cpp
 * @brief Implementation of the dispatcher object.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "linden_common.h"

#include "lldispatcher.h"

#include <algorithm>
#include "llstl.h"
#include "message.h"

///----------------------------------------------------------------------------
/// Class lldispatcher
///----------------------------------------------------------------------------


LLDispatcher::LLDispatcher()
{
}

LLDispatcher::~LLDispatcher()
{
}

bool LLDispatcher::isHandlerPresent(const key_t& name) const
{
	if(mHandlers.find(name) != mHandlers.end())
	{
		return true;
	}
	return false;
}

void LLDispatcher::copyAllHandlerNames(keys_t& names) const
{
	// copy the names onto the vector we are given
	std::transform(
		mHandlers.begin(),
		mHandlers.end(),
		std::back_insert_iterator<keys_t>(names),
		llselect1st<dispatch_map_t::value_type>());
}

bool LLDispatcher::dispatch(
	const key_t& name,
	const LLUUID& invoice,
	const sparam_t& strings) const
{
	dispatch_map_t::const_iterator it = mHandlers.find(name);
	if(it != mHandlers.end())
	{
		LLDispatchHandler* func = (*it).second;
		return (*func)(this, name, invoice, strings);
	}
	llwarns << "Unable to find handler for Generic message: " << name << llendl;
	return false;
}

LLDispatchHandler* LLDispatcher::addHandler(
	const key_t& name, LLDispatchHandler* func)
{
	dispatch_map_t::iterator it = mHandlers.find(name);
	LLDispatchHandler* old_handler = NULL;
	if(it != mHandlers.end())
	{
		old_handler = (*it).second;
		mHandlers.erase(it);
	}
	if(func)
	{
		// only non-null handlers so that we don't have to worry about
		// it later.
		mHandlers.insert(dispatch_map_t::value_type(name, func));
	}
	return old_handler;
}

// static
bool LLDispatcher::unpackMessage(
		LLMessageSystem* msg,
		LLDispatcher::key_t& method,
		LLUUID& invoice,
		LLDispatcher::sparam_t& parameters)
{
	msg->getStringFast(_PREHASH_MethodData, _PREHASH_Method, method);
	msg->getUUIDFast(_PREHASH_MethodData, _PREHASH_Invoice, invoice);
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_ParamList);
	for (S32 i = 0; i < count; ++i)
	{
		std::string parameter;
		msg->getStringFast(_PREHASH_ParamList,_PREHASH_Parameter, parameter, i);
		parameters.push_back(parameter);
	}
	return true;
}
