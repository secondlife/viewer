/** 
 * @file lluploaddialog.cpp
 * @brief LLUploadDialog class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#include "lluploaddialog.h"
#include "llviewerwindow.h"
#include "llfontgl.h"
#include "llresmgr.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "llkeyboard.h"
#include "llfocusmgr.h"
#include "llviewercontrol.h"

// static
LLUploadDialog*	LLUploadDialog::sDialog = NULL;

// static
LLUploadDialog* LLUploadDialog::modalUploadDialog(const std::string& msg)
{
	// Note: object adds, removes, and destroys itself.
	return new LLUploadDialog(msg);
}

// static
void LLUploadDialog::modalUploadFinished()
{
	// Note: object adds, removes, and destroys itself.
	delete LLUploadDialog::sDialog;
	LLUploadDialog::sDialog = NULL;
}

////////////////////////////////////////////////////////////
// Private methods

LLUploadDialog::LLUploadDialog( const std::string& msg)
	:
	LLPanel( "Uploading...", LLRect(0,100,100,0) )  // dummy rect.  Will reshape below.
{
	setBackgroundVisible( TRUE );

	if( LLUploadDialog::sDialog )
	{
		delete LLUploadDialog::sDialog;
	}
	LLUploadDialog::sDialog = this;

	const LLFontGL* font = gResMgr->getRes( LLFONT_SANSSERIF );
	LLRect msg_rect;
	for (int line_num=0; line_num<16; ++line_num)
	{
		mLabelBox[line_num] = new LLTextBox( "Filename", msg_rect, "", font );
		addChild(mLabelBox[line_num]);
	}

	setMessage(msg);

	// The dialog view is a root view
	gViewerWindow->setTopCtrl( this );
}

void LLUploadDialog::setMessage( const std::string& msg)
{
	const LLFontGL* font = gResMgr->getRes( LLFONT_SANSSERIF );

	const S32 VPAD = 16;
	const S32 HPAD = 25;

	// Make the text boxes a little wider than the text
	const S32 TEXT_PAD = 8;

	// Split message into lines, separated by '\n'
	S32 max_msg_width = 0;
	LLDoubleLinkedList<LLString> msg_lines;

	S32 size = msg.size() + 1;// + strlen("Uploading...\n\n");
	char* temp_msg = new char[size];
	
	//strcpy(temp_msg,"Uploading...\n\n");
	if (temp_msg == NULL)
	{
		llerrs << "Memory Allocation Failed" << llendl;
		return;
	}
	
	strcpy( temp_msg, msg.c_str());		/* Flawfinder: ignore */
	char* token = strtok( temp_msg, "\n" );
	while( token )
	{
		S32 cur_width = S32(font->getWidth(token) + 0.99f) + TEXT_PAD;
		max_msg_width = llmax( max_msg_width, cur_width );
		msg_lines.addDataAtEnd( new LLString( token ) );
		token = strtok( NULL, "\n" );
	}
	delete[] temp_msg;


	S32 line_height = S32( font->getLineHeight() + 0.99f );
	S32 dialog_width = max_msg_width + 2 * HPAD;
	S32 dialog_height = line_height * msg_lines.getLength() + 2 * VPAD;

	reshape( dialog_width, dialog_height, FALSE );

	// Message
	S32 msg_x = (mRect.getWidth() - max_msg_width) / 2;
	S32 msg_y = mRect.getHeight() - VPAD - line_height;
	int line_num;
	for (line_num=0; line_num<16; ++line_num)
	{
		mLabelBox[line_num]->setVisible(FALSE);
	}
	line_num = 0;
	for( LLString* cur_line = msg_lines.getFirstData(); cur_line; cur_line = msg_lines.getNextData() )
	{
		LLRect msg_rect;
		msg_rect.setOriginAndSize( msg_x, msg_y, max_msg_width, line_height );
		mLabelBox[line_num]->setRect(msg_rect);
		mLabelBox[line_num]->setText(*cur_line);
		mLabelBox[line_num]->setColor( gColors.getColor( "LabelTextColor" ) );
		mLabelBox[line_num]->setVisible(TRUE);
		msg_y -= line_height;
		++line_num;
	}
	msg_lines.deleteAllData();

	centerDialog();
}

LLUploadDialog::~LLUploadDialog()
{
	gFocusMgr.releaseFocusIfNeeded( this );

//    LLFilePicker::instance().reset();


	LLUploadDialog::sDialog = NULL;
}

void LLUploadDialog::centerDialog()
{
	LLRect window_rect = gViewerWindow->getRootView()->getRect();

	S32 dialog_left = window_rect.mLeft + (window_rect.getWidth() - mRect.getWidth()) / 2;
	S32 dialog_bottom = window_rect.mBottom + (window_rect.getHeight() - mRect.getHeight()) / 2;

	translate( dialog_left - mRect.mLeft, dialog_bottom - mRect.mBottom );
}

