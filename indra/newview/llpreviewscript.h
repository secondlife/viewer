/** 
 * @file llpreviewscript.h
 * @brief LLPreviewScript class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#ifndef LL_LLPREVIEWSCRIPT_H
#define LL_LLPREVIEWSCRIPT_H

#include "lldarray.h"
#include "llpreview.h"
#include "lltabcontainer.h"
#include "llinventory.h"
#include "llcombobox.h"
#include "lliconctrl.h"
#include "llframetimer.h"


class LLMessageSystem;
class LLTextEditor;
class LLButton;
class LLCheckBoxCtrl;
class LLScrollListCtrl;
class LLViewerObject;
struct 	LLEntryAndEdCore;
class LLMenuBarGL;
class LLFloaterScriptSearch;
class LLKeywordToken;

// Inner, implementation class.  LLPreviewScript and LLLiveLSLEditor each own one of these.
class LLScriptEdCore : public LLPanel
{
	friend class LLPreviewScript;
	friend class LLPreviewLSL;
	friend class LLLiveLSLEditor;
	friend class LLFloaterScriptSearch;

public:
	LLScriptEdCore(
		const std::string& name,
		const LLRect& rect,
		const std::string& sample,
		const std::string& help,
		const LLHandle<LLFloater>& floater_handle,
		void (*load_callback)(void* userdata),
		void (*save_callback)(void* userdata, BOOL close_after_save),
		void (*search_replace_callback)(void* userdata),
		void* userdata,
		S32 bottom_pad = 0);	// pad below bottom row of buttons
	~LLScriptEdCore();
	
	void			initMenu();

	virtual void	draw();

	BOOL			canClose();

	static void		handleSaveChangesDialog(S32 option, void* userdata);
	static void		handleReloadFromServerDialog(S32 option, void* userdata);

	static void		onHelpWebDialog(S32 option, void* userdata);
	static void		onBtnHelp(void* userdata);
	static void		onBtnDynamicHelp(void* userdata);
	static void		onCheckLock(LLUICtrl*, void*);
	static void		onHelpComboCommit(LLUICtrl* ctrl, void* userdata);
	static void		onClickBack(void* userdata);
	static void		onClickForward(void* userdata);
	static void		onBtnInsertSample(void*);
	static void		onBtnInsertFunction(LLUICtrl*, void*);
	static void		doSave( void* userdata, BOOL close_after_save );
	static void		onBtnSave(void*);
	static void		onBtnUndoChanges(void*);
	static void		onSearchMenu(void* userdata);

	static void		onUndoMenu(void* userdata);
	static void		onRedoMenu(void* userdata);
	static void		onCutMenu(void* userdata);
	static void		onCopyMenu(void* userdata);
	static void		onPasteMenu(void* userdata);
	static void		onSelectAllMenu(void* userdata);
	static void		onDeselectMenu(void* userdata);

	static BOOL		enableUndoMenu(void* userdata);
	static BOOL		enableRedoMenu(void* userdata);
	static BOOL		enableCutMenu(void* userdata);
	static BOOL		enableCopyMenu(void* userdata);
	static BOOL		enablePasteMenu(void* userdata);
	static BOOL		enableSelectAllMenu(void* userdata);
	static BOOL		enableDeselectMenu(void* userdata);

	static BOOL		hasChanged(void* userdata);

	void selectFirstError();

	virtual BOOL handleKeyHere(KEY key, MASK mask);

protected:
	void deleteBridges();
	void setHelpPage(const LLString& help_string);
	void updateDynamicHelp(BOOL immediate = FALSE);
	void addHelpItemToHistory(const LLString& help_string);

	static void onErrorList(LLUICtrl*, void* user_data);

 	virtual const char *getTitleName() const { return "Script"; }

private:
	LLString		mSampleText;
	std::string		mHelpFile;
	LLTextEditor*	mEditor;
	void			(*mLoadCallback)(void* userdata);
	void			(*mSaveCallback)(void* userdata, BOOL close_after_save);
	void			(*mSearchReplaceCallback) (void* userdata);
	void*			mUserdata;
	LLComboBox		*mFunctions;
	BOOL			mForceClose;
	//LLPanel*		mGuiPanel;
	LLPanel*		mCodePanel;
	LLScrollListCtrl* mErrorList;
	LLDynamicArray<LLEntryAndEdCore*> mBridges;
	LLHandle<LLFloater>	mLiveHelpHandle;
	LLKeywordToken* mLastHelpToken;
	LLFrameTimer	mLiveHelpTimer;
	S32				mLiveHelpHistorySize;
};


// Used to view and edit a LSL from your inventory.
class LLPreviewLSL : public LLPreview
{
public:
	LLPreviewLSL(const std::string& name, const LLRect& rect, const std::string& title,
				 const LLUUID& item_uuid );
	virtual void callbackLSLCompileSucceeded();
	virtual void callbackLSLCompileFailed(const LLSD& compile_errors);

	/*virtual*/ void open();		/*Flawfinder: ignore*/

protected:
	virtual BOOL canClose();
	void closeIfNeeded();
	virtual void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	virtual void loadAsset();
	void saveIfNeeded();
	void uploadAssetViaCaps(const std::string& url,
							const std::string& filename, 
							const LLUUID& item_id);
	void uploadAssetLegacy(const std::string& filename,
							const LLUUID& item_id,
							const LLTransactionID& tid);

	static void onSearchReplace(void* userdata);
	static void onLoad(void* userdata);
	static void onSave(void* userdata, BOOL close_after_save);
	
	static void onLoadComplete(LLVFS *vfs, const LLUUID& uuid,
							   LLAssetType::EType type,
							   void* user_data, S32 status, LLExtStat ext_status);
	static void onSaveComplete(const LLUUID& uuid, void* user_data, S32 status, LLExtStat ext_status);
	static void onSaveBytecodeComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status);
	static LLPreviewLSL* getInstance(const LLUUID& uuid);

	static void* createScriptEdPanel(void* userdata);


protected:

 	virtual const char *getTitleName() const { return "Script"; }
	LLScriptEdCore* mScriptEd;
	// Can safely close only after both text and bytecode are uploaded
	S32 mPendingUploads;
};


// Used to view and edit an LSL that is attached to an object.
class LLLiveLSLEditor : public LLPreview
{
public: 
	LLLiveLSLEditor(const std::string& name, const LLRect& rect,
					const std::string& title,
					const LLUUID& object_id, const LLUUID& item_id);
	~LLLiveLSLEditor();


	static LLLiveLSLEditor* show(const LLUUID& item_id, const LLUUID& object_id);
	static void hide(const LLUUID& item_id, const LLUUID& object_id);
	static LLLiveLSLEditor* find(const LLUUID& item_id, const LLUUID& object_id);

	static void processScriptRunningReply(LLMessageSystem* msg, void**);

	virtual void callbackLSLCompileSucceeded(const LLUUID& task_id,
											const LLUUID& item_id,
											bool is_script_running);
	virtual void callbackLSLCompileFailed(const LLSD& compile_errors);

	// Overide LLPreview::open() to avoid calling loadAsset twice.
	/*virtual*/ void open();		/*Flawfinder: ignore*/

protected:
	virtual BOOL canClose();
	void closeIfNeeded();
	virtual void draw();
	virtual void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	virtual void loadAsset();
	void loadAsset(BOOL is_new);
	void saveIfNeeded();
	void uploadAssetViaCaps(const std::string& url,
							const std::string& filename, 
							const LLUUID& task_id,
							const LLUUID& item_id,
							BOOL is_running);
	void uploadAssetLegacy(const std::string& filename,
						   LLViewerObject* object,
						   const LLTransactionID& tid,
						   BOOL is_running);

	static void onSearchReplace(void* userdata);
	static void onLoad(void* userdata);
	static void onSave(void* userdata, BOOL close_after_save);

	static void onLoadComplete(LLVFS *vfs, const LLUUID& asset_uuid,
							   LLAssetType::EType type,
							   void* user_data, S32 status, LLExtStat ext_status);
	static void onSaveTextComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status);
	static void onSaveBytecodeComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status);
	static void onRunningCheckboxClicked(LLUICtrl*, void* userdata);
	static void onReset(void* userdata);

	void loadScriptText(const char* filename);
	void loadScriptText(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type);

	static void onErrorList(LLUICtrl*, void* user_data);

	static void* createScriptEdPanel(void* userdata);


protected:
	LLUUID mObjectID;
	LLUUID mItemID; // The inventory item this script is associated with
	BOOL mIsNew;
	LLScriptEdCore* mScriptEd;
	//LLUUID mTransmitID;
	LLCheckBoxCtrl	*mRunningCheckbox;
	BOOL mAskedForRunningInfo;
	BOOL mHaveRunningInfo;
	LLButton		*mResetButton;
	LLPointer<LLViewerInventoryItem> mItem;
	BOOL mCloseAfterSave;
	// need to save both text and script, so need to decide when done
	S32 mPendingUploads;

	static LLMap<LLUUID, LLLiveLSLEditor*> sInstances;
};

// name of help file for lsl
extern const char HELP_LSL[];

#endif  // LL_LLPREVIEWSCRIPT_H
