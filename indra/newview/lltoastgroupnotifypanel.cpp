/**
 * @file lltoastgroupnotifypanel.cpp
 * @brief Panel for group notify toasts.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "lltoastgroupnotifypanel.h"

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

const S32 LLToastGroupNotifyPanel::DEFAULT_MESSAGE_MAX_LINE_COUNT	= 7;

LLToastGroupNotifyPanel::LLToastGroupNotifyPanel(LLNotificationPtr& notification)
:	LLToastPanel(notification),
	mInventoryOffer(NULL)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_group_notify.xml");
	const LLSD& payload = notification->getPayload();
	LLGroupData groupData;
	if (!gAgent.getGroupData(payload["group_id"].asUUID(),groupData))
	{
		llwarns << "Group notice for unkown group: " << payload["group_id"].asUUID() << llendl;
	}

	//group icon
	LLIconCtrl* pGroupIcon = getChild<LLIconCtrl>("group_icon", TRUE);
	pGroupIcon->setValue(groupData.mInsigniaID);

	//header title
	const std::string& from_name = payload["sender_name"].asString();
	std::stringstream from;
	from << from_name << "/" << groupData.mName;
	LLTextBox* pTitleText = getChild<LLTextBox>("title");
	pTitleText->setValue(from.str());

	//message subject
	const std::string& subject = payload["subject"].asString();
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
	LLFontGL* subject_font = LLFontGL::getFontByName(getString("subject_font"));
	if (subject_font) 
		style.font = subject_font;
	pMessageText->appendText(subject, FALSE, style);

	LLFontGL* date_font = LLFontGL::getFontByName(getString("date_font"));
	if (date_font)
		style.font = date_font;
	pMessageText->appendText(timeStr + "\n", TRUE, style);
	
	style.font = pMessageText->getDefaultFont();
	pMessageText->appendText(message, TRUE, style);

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
		childSetActionTextbox("attachment", boost::bind(
				&LLToastGroupNotifyPanel::onClickAttachment, this));

		LLUIImagePtr attachIconImg = get_item_icon(mInventoryOffer->mType,
												LLInventoryType::IT_TEXTURE,
												0, FALSE);
		pAttachIcon->setValue(attachIconImg->getName());
	}

	//ok button
	LLButton* pOkBtn = getChild<LLButton>("btn_ok");
	pOkBtn->setClickedCallback((boost::bind(&LLToastGroupNotifyPanel::onClickOk, this)));
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
LLToastGroupNotifyPanel::~LLToastGroupNotifyPanel()
{
}

void LLToastGroupNotifyPanel::close()
{
	// The group notice dialog may be an inventory offer.
	// If it has an inventory save button and that button is still enabled
	// Then we need to send the inventory declined message
	if(mInventoryOffer != NULL)
	{
		mInventoryOffer->forceResponse(IOR_DECLINE);
		mInventoryOffer = NULL;
	}

	die();
}

void LLToastGroupNotifyPanel::onClickOk()
{
	LLSD response = mNotification->getResponseTemplate();
	mNotification->respond(response);
	close();
}

void LLToastGroupNotifyPanel::onClickAttachment()
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

//static
bool LLToastGroupNotifyPanel::isAttachmentOpenable(LLAssetType::EType type)
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

