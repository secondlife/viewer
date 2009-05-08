/**
 * @file lltemplatemessagedispatcher.h
 * @brief LLTemplateMessageDispatcher class
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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


#include "lltemplatemessagedispatcher.h"

#include "llhttpnode.h"
#include "llhost.h"
#include "message.h"
#include "llsd.h"
#include "lltemplatemessagereader.h"


LLTemplateMessageDispatcher::LLTemplateMessageDispatcher(LLTemplateMessageReader &template_message_reader) :
	mTemplateMessageReader(template_message_reader)
{
}

void LLTemplateMessageDispatcher::dispatch(const std::string& msg_name,
										   const LLSD& message,
										   LLHTTPNode::ResponsePtr responsep)
{
	std::vector<U8> data = message["body"]["binary-template-data"].asBinary();
	U32 size = data.size();
	if(size == 0)
	{
		return;
	}

	LLHost host;
	host = gMessageSystem->getSender();

	bool validate_message = mTemplateMessageReader.validateMessage(&(data[0]), data.size(), host, true);

	if (validate_message)
	{
		mTemplateMessageReader.readMessage(&(data[0]),host);
	} 
	else 
	{
		gMessageSystem->clearReceiveState();
	}
}

