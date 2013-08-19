/**
 * @file llfloatergotoline.h
 * @author MartinRJ
 * @brief LLFloaterGotoLine class implementation
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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
#include "llfloatergotoline.h"
#include "llpreviewscript.h"
#include "llfloaterreg.h"
#include "lllineeditor.h"
#include "llviewertexteditor.h"
#include "llviewerwindow.h"

LLFloaterGotoLine* LLFloaterGotoLine::sInstance = NULL;

LLFloaterGotoLine::LLFloaterGotoLine(LLScriptEdCore* editor_core)
:       LLFloater(LLSD()),
        mGotoBox(NULL),
        mEditorCore(editor_core)
{
        buildFromFile("floater_goto_line.xml");

        sInstance = this;
        
        // find floater in which script panel is embedded
        LLView* viewp = (LLView*)editor_core;
        while(viewp)
        {
                LLFloater* floaterp = dynamic_cast<LLFloater*>(viewp);
                if (floaterp)
                {
                        floaterp->addDependentFloater(this);
                        break;
                }
                viewp = viewp->getParent();
        }
}

BOOL LLFloaterGotoLine::postBuild()
{
	mGotoBox = getChild<LLLineEditor>("goto_line");
	mGotoBox->setCommitCallback(boost::bind(&LLFloaterGotoLine::onGotoBoxCommit, this));
	mGotoBox->setCommitOnFocusLost(FALSE);
        getChild<LLLineEditor>("goto_line")->setPrevalidate(LLTextValidate::validateNonNegativeS32);
        childSetAction("goto_btn", onBtnGoto,this);
        setDefaultBtn("goto_btn");

        return TRUE;
}

//static 
void LLFloaterGotoLine::show(LLScriptEdCore* editor_core)
{
        if (sInstance && sInstance->mEditorCore && sInstance->mEditorCore != editor_core)
        {
                sInstance->closeFloater();
                delete sInstance;
        }

        if (!sInstance)
        {
                // sInstance will be assigned in the constructor.
                new LLFloaterGotoLine(editor_core);
        }

        sInstance->openFloater();
}

LLFloaterGotoLine::~LLFloaterGotoLine()
{
        sInstance = NULL;
}

// static 
void LLFloaterGotoLine::onBtnGoto(void *userdata)
{
        LLFloaterGotoLine* self = (LLFloaterGotoLine*)userdata;
        self->handleBtnGoto();
}

void LLFloaterGotoLine::handleBtnGoto()
{
        S32 row = 0;
        S32 column = 0;
        row = getChild<LLUICtrl>("goto_line")->getValue().asInteger();
        if (row >= 0)
        {
                if (mEditorCore && mEditorCore->mEditor)
                {
			mEditorCore->mEditor->deselect();
			mEditorCore->mEditor->setCursor(row, column);
			mEditorCore->mEditor->setFocus(TRUE);
                }
        }
}

bool LLFloaterGotoLine::hasAccelerators() const
{
        if (mEditorCore)
        {
                return mEditorCore->hasAccelerators();
        }
        return FALSE;
}

BOOL LLFloaterGotoLine::handleKeyHere(KEY key, MASK mask)
{
        if (mEditorCore)
        {
                return mEditorCore->handleKeyHere(key, mask);
        }

        return FALSE;
}

void LLFloaterGotoLine::onGotoBoxCommit()
{
        S32 row = 0;
        S32 column = 0;
        row = getChild<LLUICtrl>("goto_line")->getValue().asInteger();
        if (row >= 0)
        {
                if (mEditorCore && mEditorCore->mEditor)
                {
			mEditorCore->mEditor->setCursor(row, column);

			S32 rownew = 0;
			S32 columnnew = 0;
			mEditorCore->mEditor->getCurrentLineAndColumn( &rownew, &columnnew, FALSE );  // don't include wordwrap
			if (rownew == row && columnnew == column)
			{
			        mEditorCore->mEditor->deselect();
			        mEditorCore->mEditor->setFocus(TRUE);
			        sInstance->closeFloater();
			} //else do nothing (if the cursor-position didn't change)
                }
        }
}
