/** 
 * @file 
 * @brief 
 *
 * $LicenseInfo:firstyear=2008&license=internal$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * 
 * The following source code is PROPRIETARY AND CONFIDENTIAL. Use of
 * this source code is governed by the Linden Lab Source Code Disclosure
 * Agreement ("Agreement") previously entered between you and Linden
 * Lab. By accessing, using, copying, modifying or distributing this
 * software, you acknowledge that you have been informed of your
 * obligations under the Agreement and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

/* Macro Definitions */
#ifndef LL_LLTESTMESSAGESENDER_H
#define LL_LLTESTMESSAGESENDER_H


#include "linden_common.h"
#include "llmessagesenderinterface.h"
#include <vector>



class LLTestMessageSender : public LLMessageSenderInterface
{
public:
	virtual ~LLTestMessageSender();
	virtual S32 sendMessage(const LLHost& host, LLStoredMessagePtr message);

	std::vector<LLHost> mSendHosts;
	std::vector<LLStoredMessagePtr> mSendMessages;
};



#endif //LL_LLTESTMESSAGESENDER_H

