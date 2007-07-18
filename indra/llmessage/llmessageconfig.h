/** 
 * @file llmessageconfig.h
 * @brief Live file handling for messaging
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
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

	// For individual messages
	static Flavor getMessageFlavor(const std::string& msg_name);
	static SenderTrust getSenderTrustedness(const std::string& msg_name);
	static bool isValidMessage(const std::string& msg_name);
	static bool isCapBanned(const std::string& cap_name);
	static LLSD getConfigForMessage(const std::string& msg_name);
};
#endif // LL_MESSAGECONFIG_H
