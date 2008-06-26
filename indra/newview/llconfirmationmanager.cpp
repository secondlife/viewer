/** 
 * @file llconfirmationmanager.cpp
 * @brief LLConfirmationManager class implementation
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llconfirmationmanager.h"

#include "lluictrlfactory.h"

// viewer includes
#include "llviewerwindow.h"
#include "lllineeditor.h"
#include "llstring.h"

LLConfirmationManager::ListenerBase::~ListenerBase()
{
}


static void onConfirmAlert(S32 option, void* data)
{
	LLConfirmationManager::ListenerBase* listener
		= (LLConfirmationManager::ListenerBase*)data;
		
	if (option == 0)
	{
		listener->confirmed("");
	}
	
	delete listener;
}

static void onConfirmAlertPassword(
	S32 option, const std::string& text, void* data)
{
	LLConfirmationManager::ListenerBase* listener
		= (LLConfirmationManager::ListenerBase*)data;
		
	if (option == 0)
	{
		listener->confirmed(text);
	}
	
	delete listener;
}

 
void LLConfirmationManager::confirm(Type type,
	const std::string& action,
	ListenerBase* listener)
{
	LLStringUtil::format_map_t args;
	args["[ACTION]"] = action;

	switch (type)
	{
		case TYPE_CLICK:
		  gViewerWindow->alertXml("ConfirmPurchase", args,
								  onConfirmAlert, listener);
		  break;

		case TYPE_PASSWORD:
		  gViewerWindow->alertXmlEditText("ConfirmPurchasePassword", args,
										  NULL, NULL,
										  onConfirmAlertPassword, listener,
										  LLStringUtil::format_map_t(),
										  TRUE);
		  break;
		case TYPE_NONE:
		default:
		  listener->confirmed("");
		  break;
	}
}


void LLConfirmationManager::confirm(
	const std::string& type,
	const std::string& action,
	ListenerBase* listener)
{
	Type decodedType = TYPE_NONE;
	
	if (type == "click")
	{
		decodedType = TYPE_CLICK;
	}
	else if (type == "password")
	{
		decodedType = TYPE_PASSWORD;
	}
	
	confirm(decodedType, action, listener);
}

