/** 
 * @file llconfirmationmanager.cpp
 * @brief LLConfirmationManager class implementation
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	S32 option, const LLString& text, void* data)
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
	LLString::format_map_t args;
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
										  LLString::format_map_t(),
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

