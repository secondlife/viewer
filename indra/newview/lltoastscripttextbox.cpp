/**
 * @file lltoastscripttextbox.cpp
 * @brief Panel for script llTextBox dialogs
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "lltoastscripttextbox.h"

#include "llfocusmgr.h"

#include "llbutton.h"
#include "lliconctrl.h"
#include "llinventoryfunctions.h"
#include "llnotifications.h"
#include "llviewertexteditor.h"

#include "lluiconstants.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "lltrans.h"
#include "llstyle.h"

#include "llglheaders.h"
#include "llagent.h"
#include "llavatariconctrl.h"
#include "llfloaterinventory.h"
#include "llinventorytype.h"

const S32 LLToastScriptTextbox::DEFAULT_MESSAGE_MAX_LINE_COUNT	= 7;

LLToastScriptTextbox::LLToastScriptTextbox(LLNotificationPtr& notification)
:	LLToastNotifyPanel(notification),
	mInventoryOffer(NULL)
{
	buildFromFile( "panel_notify_textbox.xml");

	const LLSD& payload = notification->getPayload();
	llwarns << "PAYLOAD " << payload << llendl;
	llwarns << "TYPE " << notification->getType() << llendl;
	llwarns << "MESSAGE " << notification->getMessage() << llendl;
	llwarns << "LABEL " << notification->getLabel() << llendl;
	llwarns << "URL " << notification->getURL() << llendl;

	/*
2010-09-29T12:24:44Z WARNING: LLToastScriptTextbox: PAYLOAD {'chat_channel':i-376,'object_id':ubb05bcf2-4eca-2203-13f4-b328411d344f,'sender':'216.82.20.80:13001'}
2010-09-29T12:24:44Z WARNING: LLToastScriptTextbox: TYPE notify
2010-09-29T12:24:44Z WARNING: LLToastScriptTextbox: MESSAGE Tofu Tester's 'lltextbox test'
Write something here...
2010-09-29T12:24:44Z WARNING: LLToastScriptTextbox: LABEL 
2010-09-29T12:24:44Z WARNING: LLToastScriptTextbox: URL 
*/

	//message subject
	//const std::string& subject = payload["subject"].asString();
	//message body
	const std::string& message = payload["message"].asString();

	std::string timeStr = "["+LLTrans::getString("UTCTimeWeek")+"],["
		+LLTrans::getString("UTCTimeDay")+"] ["
		+LLTrans::getString("UTCTimeMth")+"] ["
		+LLTrans::getString("UTCTimeYr")+"] ["
		+LLTrans::getString("UTCTimeHr")+"]:["
		+LLTrans::getString("UTCTimeMin")+"]:["
		+LLTrans::getString("UTCTimeSec")+"] ["
		+LLTrans::getString("UTCTimeTimezone")+"]";
	const LLDate timeStamp = notification->getDate();
	LLDate notice_date = timeStamp.notNull() ? timeStamp : LLDate::now();
	LLSD substitution;
	substitution["datetime"] = (S32) notice_date.secondsSinceEpoch();
	LLStringUtil::format(timeStr, substitution);

	LLViewerTextEditor* pMessageText = getChild<LLViewerTextEditor>("message");
	pMessageText->clear();

	LLStyle::Params style;

	LLFontGL* date_font = LLFontGL::getFontByName(getString("date_font"));
	if (date_font)
		style.font = date_font;
	pMessageText->appendText(timeStr + "\n", TRUE, style);
	
	style.font = pMessageText->getDefaultFont();
	pMessageText->appendText(message, TRUE, style);
	/*
	//attachment
	BOOL hasInventory = payload["inventory_offer"].isDefined();

	//attachment text
	LLTextBox * pAttachLink = getChild<LLTextBox>("attachment");
	//attachment icon
	LLIconCtrl* pAttachIcon = getChild<LLIconCtrl>("attachment_icon", TRUE);

	//If attachment is empty let it be invisible and not take place at the panel
	pAttachLink->setVisible(hasInventory);
	pAttachIcon->setVisible(hasInventory);
	if (hasInventory) {
		pAttachLink->setValue(payload["inventory_name"]);

		mInventoryOffer = new LLOfferInfo(payload["inventory_offer"]);
		getChild<LLTextBox>("attachment")->setClickedCallback(boost::bind(
				&LLToastScriptTextbox::onClickAttachment, this));

		LLUIImagePtr attachIconImg = LLInventoryIcon::getIcon(mInventoryOffer->mType,
												LLInventoryType::IT_TEXTURE);
		pAttachIcon->setValue(attachIconImg->getName());
	}
	*/
	//ok button
	LLButton* pOkBtn = getChild<LLButton>("btn_ok");
	pOkBtn->setClickedCallback((boost::bind(&LLToastScriptTextbox::onClickOk, this)));
	setDefaultBtn(pOkBtn);

	S32 maxLinesCount;
	std::istringstream ss( getString("message_max_lines_count") );
	if (!(ss >> maxLinesCount))
	{
		maxLinesCount = DEFAULT_MESSAGE_MAX_LINE_COUNT;
	}
	snapToMessageHeight(pMessageText, maxLinesCount);
}

// virtual
LLToastScriptTextbox::~LLToastScriptTextbox()
{
}

void LLToastScriptTextbox::close()
{
	// The group notice dialog may be an inventory offer.
	// If it has an inventory save button and that button is still enabled
	// Then we need to send the inventory declined message
	/*
	if(mInventoryOffer != NULL)
	{
		mInventoryOffer->forceResponse(IOR_DECLINE);
		mInventoryOffer = NULL;
	}
	*/
	die();
}

#include "lllslconstants.h"
void LLToastScriptTextbox::onClickOk()
{
	LLViewerTextEditor* pMessageText = getChild<LLViewerTextEditor>("message");

	if (pMessageText)
	{
		LLSD response = mNotification->getResponseTemplate();
		//response["OH MY GOD WHAT A HACK"] = "woot";
		response[TEXTBOX_MAGIC_TOKEN] = pMessageText->getText();
		mNotification->respond(response);
		close();
		llwarns << response << llendl;
	}
}
/*
void LLToastScriptTextbox::onClickAttachment()
{
	if (mInventoryOffer != NULL) {
		mInventoryOffer->forceResponse(IOR_ACCEPT);

		LLTextBox * pAttachLink = getChild<LLTextBox> ("attachment");
		static const LLUIColor textColor = LLUIColorTable::instance().getColor(
				"GroupNotifyDimmedTextColor");
		pAttachLink->setColor(textColor);

		LLIconCtrl* pAttachIcon =
				getChild<LLIconCtrl> ("attachment_icon", TRUE);
		pAttachIcon->setEnabled(FALSE);

		//if attachment isn't openable - notify about saving
		if (!isAttachmentOpenable(mInventoryOffer->mType)) {
			LLNotifications::instance().add("AttachmentSaved", LLSD(), LLSD());
		}

		mInventoryOffer = NULL;
	}
}
*/

 /*
//static
bool LLToastScriptTextbox::isAttachmentOpenable(LLAssetType::EType type)
{
	switch(type)
	{
	case LLAssetType::AT_LANDMARK:
	case LLAssetType::AT_NOTECARD:
	case LLAssetType::AT_IMAGE_JPEG:
	case LLAssetType::AT_IMAGE_TGA:
	case LLAssetType::AT_TEXTURE:
	case LLAssetType::AT_TEXTURE_TGA:
		return true;
	default:
		return false;
	}
}

 */
