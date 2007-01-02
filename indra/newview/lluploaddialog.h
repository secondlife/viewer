/** 
 * @file lluploaddialog.h
 * @brief LLUploadDialog class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_UPLOADDIALOG_H
#define LL_UPLOADDIALOG_H

#include "llpanel.h"
#include "lltextbox.h"
			
class LLUploadDialog : public LLPanel
{
public:
	// Use this function to open a modal dialog and display it until the user presses the "close" button.
	static LLUploadDialog*	modalUploadDialog(const std::string& msg);		// Message to display
	static void				modalUploadFinished();		// Message to display

	static bool				modalUploadIsFinished() { return (sDialog == NULL); }

	void setMessage( const std::string& msg );

private:
	LLUploadDialog( const std::string& msg);
	virtual ~LLUploadDialog();	// No you can't kill it.  It can only kill itself.

	void			centerDialog();

	LLTextBox* mLabelBox[16];

private:
	static LLUploadDialog*	sDialog;  // Hidden singleton instance, created and destroyed as needed.
};

#endif  // LL_UPLOADDIALOG_H
