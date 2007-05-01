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

class LLMessageConfig
{
public:
	static void initClass(const std::string& server_name,
						  const std::string& config_dir);
		// force loading of config file during startup process
		// so it can be used for startup features

	static bool isServerDefaultBuilderLLSD();
	static bool isServerDefaultBuilderTemplate();

	// For individual messages
	static bool isMessageBuiltLLSD(const std::string& msg_name);
	static bool isMessageBuiltTemplate(const std::string& msg_name);
	static bool isMessageTrusted(const std::string& msg_name);
	static bool isValidUntrustedMessage(const std::string& msg_name);
};
#endif // LL_MESSAGECONFIG_H
