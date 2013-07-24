/** 
 * @file lluploaddialog.cpp
 * @brief LLUploadDialog class implementation
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

#include "lluploaddialog.h"
#include "llviewerwindow.h"
#include "llfontgl.h"
#include "llresmgr.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "llkeyboard.h"
#include "llfocusmgr.h"
#include "llviewercontrol.h"
#include "llrootview.h"

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
  : LLPanel()
{
	setBackgroundVisible( TRUE );

	if( LLUploadDialog::sDialog )
	{
		delete LLUploadDialog::sDialog;
	}
	LLUploadDialog::sDialog = this;

	const LLFontGL* font = LLFontGL::getFontSansSerif();
	LLRect msg_rect;
	for (int line_num=0; line_num<16; ++line_num)
	{
		LLTextBox::Params params;
		params.name("Filename");
		params.rect(msg_rect);
		params.initial_value("Filename");
		params.font(font);
		mLabelBox[line_num] = LLUICtrlFactory::create<LLTextBox> (params);
		addChild(mLabelBox[line_num]);
	}

	setMessage(msg);

	// The dialog view is a root view
	gViewerWindow->addPopup(this);
}

void LLUploadDialog::setMessage( const std::string& msg)
{
	const LLFontGL* font = LLFontGL::getFontSansSerif();

	const S32 VPAD = 16;
	const S32 HPAD = 25;

	// Make the text boxes a little wider than the text
	const S32 TEXT_PAD = 8;

	// Split message into lines, separated by '\n'
	S32 max_msg_width = 0;
	std::list<std::string> msg_lines;

	S32 size = msg.size() + 1;
	std::vector<char> temp_msg(size); // non-const copy to make strtok happy
	strcpy( &temp_msg[0], msg.c_str());
	char* token = strtok( &temp_msg[0], "\n" );
	while( token )
	{
		std::string tokstr(token);
		S32 cur_width = S32(font->getWidth(tokstr) + 0.99f) + TEXT_PAD;
		max_msg_width = llmax( max_msg_width, cur_width );
		msg_lines.push_back( tokstr );
		token = strtok( NULL, "\n" );
	}

	S32 line_height = font->getLineHeight();
	S32 dialog_width = max_msg_width + 2 * HPAD;
	S32 dialog_height = line_height * msg_lines.size() + 2 * VPAD;

	reshape( dialog_width, dialog_height, FALSE );

	// Message
	S32 msg_x = (getRect().getWidth() - max_msg_width) / 2;
	S32 msg_y = getRect().getHeight() - VPAD - line_height;
	int line_num;
	for (line_num=0; line_num<16; ++line_num)
	{
		mLabelBox[line_num]->setVisible(FALSE);
	}
	line_num = 0;
	for (std::list<std::string>::iterator iter = msg_lines.begin();
		 iter != msg_lines.end(); ++iter)
	{
		std::string& cur_line = *iter;
		LLRect msg_rect;
		msg_rect.setOriginAndSize( msg_x, msg_y, max_msg_width, line_height );
		mLabelBox[line_num]->setRect(msg_rect);
		mLabelBox[line_num]->setText(cur_line);
		mLabelBox[line_num]->setColor( LLUIColorTable::instance().getColor( "LabelTextColor" ) );
		mLabelBox[line_num]->setVisible(TRUE);
		msg_y -= line_height;
		++line_num;
	}

	centerWithin(gViewerWindow->getRootView()->getRect());
}

LLUploadDialog::~LLUploadDialog()
{
	gFocusMgr.releaseFocusIfNeeded( this );

//    LLFilePicker::instance().reset();


	LLUploadDialog::sDialog = NULL;
}



