/** 
 * @file llmessageconfig.h
 * @brief Live file handling for messaging
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2007, Linden Research, Inc.
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

#ifndef LL_MESSAGECONFIG_H
#define LL_MESSAGECONFIG_H

#include <string>
#include "llsd.h"

class LLSD;

class LLMessageConfig
{
public:
	enum Flavor { NO_FLAVOR=0, LLSD_FLAVOR=1, TEMPLATE_FLAVOR=2 };
	enum SenderTrust { NOT_SET=0, UNTRUSTED=1, TRUSTED=2 };
		
	static void initClass(const std::string& server_name,
						  const std::string& config_dir);
	static void useConfig(const LLSD& config);

	static Flavor getServerDefaultFlavor();
	 static S32 getMaxQueuedEvents();

	// For individual messages
	static Flavor getMessageFlavor(const std::string& msg_name);
	static SenderTrust getSenderTrustedness(const std::string& msg_name);
	static bool isValidMessage(const std::string& msg_name);
	static bool onlySendLatest(const std::string& msg_name);
	static bool isCapBanned(const std::string& cap_name);
	static LLSD getConfigForMessage(const std::string& msg_name);
};
#endif // LL_MESSAGECONFIG_H
