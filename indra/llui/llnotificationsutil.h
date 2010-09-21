/**
 * @file llnotificationsutil.h
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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
#ifndef LLNOTIFICATIONSUTIL_H
#define LLNOTIFICATIONSUTIL_H

// The vast majority of clients of the notifications system just want to add 
// a notification to the screen, so define this lightweight public interface
// to avoid including the heavyweight llnotifications.h

#include "llnotificationptr.h"

#include <boost/function.hpp>

class LLSD;

namespace LLNotificationsUtil
{
	LLNotificationPtr add(const std::string& name);
	
	LLNotificationPtr add(const std::string& name, 
						  const LLSD& substitutions);
	
	LLNotificationPtr add(const std::string& name, 
						  const LLSD& substitutions, 
						  const LLSD& payload);
	
	LLNotificationPtr add(const std::string& name, 
						  const LLSD& substitutions, 
						  const LLSD& payload, 
						  const std::string& functor_name);

	LLNotificationPtr add(const std::string& name, 
						  const LLSD& substitutions, 
						  const LLSD& payload, 
						  boost::function<void (const LLSD&, const LLSD&)> functor);
	
	S32 getSelectedOption(const LLSD& notification, const LLSD& response);

	void cancel(LLNotificationPtr pNotif);

	LLNotificationPtr find(LLUUID uuid);
}

#endif
