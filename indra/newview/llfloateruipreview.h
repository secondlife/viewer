/**
 * @file llfloateruipreview.h
 * @brief Tool for previewing and editing floaters
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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

// Tool for previewing floaters and panels for localization and UI design purposes.
// See: https://wiki.lindenlab.com/wiki/GUI_Preview_And_Localization_Tools
// See: https://jira.lindenlab.com/browse/DEV-16869

#ifndef LL_LLUIPREVIEW_H
#define LL_LLUIPREVIEW_H

#include "llfloater.h"			// superclass
#include "llscrollcontainer.h"	// scroll container for overlapping elements
#include "lllivefile.h"					// live file poll/stat/reload
#include <list>
#include <map>

// Forward declarations to avoid header dependencies
class LLEventTimer;
class LLColor;
class LLScrollListCtrl;
class LLComboBox;
class LLButton;
class LLLineEditor;
class LLXmlTreeNode;
class LLFloaterUIPreview;
class LLFadeEventTimer;

class LLLocalizationResetForcer;
class LLGUIPreviewLiveFile;
class LLFadeEventTimer;
class LLPreviewedFloater;

// Implementation of custom overlapping element display panel
class LLOverlapPanel : public LLPanel
{
public:
	struct Params : public LLInitParam::Block<Params, LLPanel::Params>
	{
		Params() {}
	};
	LLOverlapPanel(Params p = Params()) : LLPanel(p),
		mSpacing(10),
		// mClickedElement(NULL),
		mLastClickedElement(NULL)
	{
		mOriginalWidth = getRect().getWidth();
		mOriginalHeight = getRect().getHeight();
	}
	virtual void draw();
	
	typedef std::map<LLView*, std::list<LLView*> >	OverlapMap;
	OverlapMap mOverlapMap;						// map, of XUI element to a list of XUI elements it overlaps
	
	// LLView *mClickedElement;
	LLView *mLastClickedElement;
	int mOriginalWidth, mOriginalHeight, mSpacing;
};


class LLFloaterUIPreview : public LLFloater
{
public:
	// Setup
	LLFloaterUIPreview(const LLSD& key);
	virtual ~LLFloaterUIPreview();

	std::string getLocStr(S32 ID);							// fetches the localization string based on what is selected in the drop-down menu
	void displayFloater(BOOL click, S32 ID, bool save = false);			// needs to be public so live file can call it when it finds an update

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onClose(bool app_quitting);

	void refreshList();										// refresh list (empty it out and fill it up from scratch)
	void addFloaterEntry(const std::string& path);			// add a single file's entry to the list of floaters
	
	static BOOL containerType(LLView* viewp);				// check if the element is a container type and tree traverses need to look at its children
	
public:	
	LLPreviewedFloater*			mDisplayedFloater;			// the floater which is currently being displayed
	LLPreviewedFloater*			mDisplayedFloater_2;			// the floater which is currently being displayed
	LLGUIPreviewLiveFile*		mLiveFile;					// live file for checking for updates to the currently-displayed XML file
	LLOverlapPanel*				mOverlapPanel;				// custom overlapping elements panel
	// BOOL						mHighlightingDiffs;			// bool for whether localization diffs are being highlighted or not
	BOOL						mHighlightingOverlaps;		// bool for whether overlapping elements are being highlighted

	// typedef std::map<std::string,std::pair<std::list<std::string>,std::list<std::string> > > DiffMap; // this version copies the lists etc., and thus is bad memory-wise
	typedef std::list<std::string> StringList;
	typedef boost::shared_ptr<StringList> StringListPtr;
	typedef std::map<std::string, std::pair<StringListPtr,StringListPtr> > DiffMap;
	DiffMap mDiffsMap;							// map, of filename to pair of list of changed element paths and list of errors

private:
	// XUI elements for this floater
	LLScrollListCtrl*			mFileList;							// scroll list control for file list
	LLLineEditor*				mEditorPathTextBox;					// text field for path to editor executable
	LLLineEditor*				mEditorArgsTextBox;					// text field for arguments to editor executable
	LLLineEditor*				mDiffPathTextBox;					// text field for path to diff file
	LLButton*					mDisplayFloaterBtn;					// button to display primary floater
	LLButton*					mDisplayFloaterBtn_2;				// button to display secondary floater
	LLButton*					mEditFloaterBtn;					// button to edit floater
	LLButton*					mExecutableBrowseButton;			// button to browse for executable
	LLButton*					mCloseOtherButton;					// button to close primary displayed floater
	LLButton*					mCloseOtherButton_2;				// button to close secondary displayed floater
	LLButton*					mDiffBrowseButton;					// button to browse for diff file
	LLButton*					mToggleHighlightButton;				// button to toggle highlight of files/elements with diffs
	LLButton*					mToggleOverlapButton;				// button to togle overlap panel/highlighting
	LLComboBox*					mLanguageSelection;					// combo box for primary language selection
	LLComboBox*					mLanguageSelection_2;				// combo box for secondary language selection
	LLScrollContainer*			mOverlapScrollView;					// overlapping elements scroll container
	S32							mLastDisplayedX, mLastDisplayedY;	// stored position of last floater so the new one opens up in the same place
	std::string 				mDelim;								// the OS-specific delimiter character (/ or \) (*TODO: this shouldn't be needed, right?)

	std::string					mSavedEditorPath;					// stored editor path so closing this floater doesn't reset it
	std::string					mSavedEditorArgs;					// stored editor args so closing this floater doesn't reset it
	std::string					mSavedDiffPath;						// stored diff file path so closing this floater doesn't reset it

	// Internal functionality
	static void popupAndPrintWarning(std::string& warning);			// pop up a warning
	std::string getLocalizedDirectory();							// build and return the path to the XUI directory for the currently-selected localization
	void scanDiffFile(LLXmlTreeNode* file_node);					// scan a given XML node for diff entries and highlight them in its associated file
	void highlightChangedElements();								// look up the list of elements to highlight and highlight them in the current floater
	void highlightChangedFiles();									// look up the list of changed files to highlight and highlight them in the scroll list
	void findOverlapsInChildren(LLView* parent);					// fill the map below with element overlap information
	static BOOL overlapIgnorable(LLView* viewp);					// check it the element can be ignored for overlap/localization purposes

	// check if two elements overlap using their rectangles
	// used instead of llrect functions because by adding a few pixels of leeway I can cut down drastically on the number of overlaps
	BOOL elementOverlap(LLView* view1, LLView* view2);

	// Button/drop-down action listeners (self explanatory)
	void onClickDisplayFloater(S32 id);
	void onClickSaveFloater(S32 id);
	void onClickSaveAll(S32 id);
	void onClickEditFloater();
	void onClickBrowseForEditor();
	void onClickBrowseForDiffs();
	void onClickToggleDiffHighlighting();
	void onClickToggleOverlapping();
	void onClickCloseDisplayedFloater(S32 id);
	void onLanguageComboSelect(LLUICtrl* ctrl);
	void onClickExportSchema();
};
#endif // LL_LLUIPREVIEW_H

