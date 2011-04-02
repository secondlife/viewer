/** 
 * @file llpreviewscript.h
 * @brief LLPreviewScript class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLPREVIEWSCRIPT_H
#define LL_LLPREVIEWSCRIPT_H

#include "lldarray.h"
#include "llpreview.h"
#include "lltabcontainer.h"
#include "llinventory.h"
#include "llcombobox.h"
#include "lliconctrl.h"
#include "llframetimer.h"

class LLLiveLSLFile;
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
class LLVFS;
class LLViewerInventoryItem;
class LLScriptEdContainer;

// Inner, implementation class.  LLPreviewScript and LLLiveLSLEditor each own one of these.
class LLScriptEdCore : public LLPanel
{
	friend class LLPreviewScript;
	friend class LLPreviewLSL;
	friend class LLLiveLSLEditor;
	friend class LLFloaterScriptSearch;
	friend class LLScriptEdContainer;

protected:
	// Supposed to be invoked only by the container.
	LLScriptEdCore(
		LLScriptEdContainer* container,
		const std::string& sample,
		const LLHandle<LLFloater>& floater_handle,
		void (*load_callback)(void* userdata),
		void (*save_callback)(void* userdata, BOOL close_after_save),
		void (*search_replace_callback)(void* userdata),
		void* userdata,
		S32 bottom_pad = 0);	// pad below bottom row of buttons
public:
	~LLScriptEdCore();
	
	void			initMenu();

	virtual void	draw();
	/*virtual*/	BOOL	postBuild();
	BOOL			canClose();
	void			setEnableEditing(bool enable);

	void            setScriptText(const std::string& text, BOOL is_valid);
	bool			loadScriptText(const std::string& filename);
	bool			writeToFile(const std::string& filename);
	void			sync();
	
	void			doSave( BOOL close_after_save );

	bool			handleSaveChangesDialog(const LLSD& notification, const LLSD& response);
	bool			handleReloadFromServerDialog(const LLSD& notification, const LLSD& response);

	void			openInExternalEditor();

	static void		onCheckLock(LLUICtrl*, void*);
	static void		onHelpComboCommit(LLUICtrl* ctrl, void* userdata);
	static void		onClickBack(void* userdata);
	static void		onClickForward(void* userdata);
	static void		onBtnInsertSample(void*);
	static void		onBtnInsertFunction(LLUICtrl*, void*);

	virtual bool	hasAccelerators() const { return true; }

private:
	void		onBtnHelp();
	void		onBtnDynamicHelp();
	void		onBtnUndoChanges();

	bool		hasChanged();

	void selectFirstError();

	virtual BOOL handleKeyHere(KEY key, MASK mask);
	
	void enableSave(BOOL b) {mEnableSave = b;}

protected:
	void deleteBridges();
	void setHelpPage(const std::string& help_string);
	void updateDynamicHelp(BOOL immediate = FALSE);
	void addHelpItemToHistory(const std::string& help_string);
	static void onErrorList(LLUICtrl*, void* user_data);

private:
	std::string		mSampleText;
	LLTextEditor*	mEditor;
	void			(*mLoadCallback)(void* userdata);
	void			(*mSaveCallback)(void* userdata, BOOL close_after_save);
	void			(*mSearchReplaceCallback) (void* userdata);
	void*			mUserdata;
	LLComboBox		*mFunctions;
	BOOL			mForceClose;
	LLPanel*		mCodePanel;
	LLScrollListCtrl* mErrorList;
	LLDynamicArray<LLEntryAndEdCore*> mBridges;
	LLHandle<LLFloater>	mLiveHelpHandle;
	LLKeywordToken* mLastHelpToken;
	LLFrameTimer	mLiveHelpTimer;
	S32				mLiveHelpHistorySize;
	BOOL			mEnableSave;
	BOOL			mHasScriptData;
	LLLiveLSLFile*	mLiveFile;

	LLScriptEdContainer* mContainer; // parent view
};

class LLScriptEdContainer : public LLPreview
{
	friend class LLScriptEdCore;

public:
	LLScriptEdContainer(const LLSD& key);

protected:
	std::string		getTmpFileName();
	bool			onExternalChange(const std::string& filename);
	virtual void	saveIfNeeded(bool sync = true) = 0;

	LLScriptEdCore*		mScriptEd;
};

// Used to view and edit a LSL from your inventory.
class LLPreviewLSL : public LLScriptEdContainer
{
public:
	LLPreviewLSL(const LLSD& key );
	virtual void callbackLSLCompileSucceeded();
	virtual void callbackLSLCompileFailed(const LLSD& compile_errors);

	/*virtual*/ BOOL postBuild();

protected:
	virtual BOOL canClose();
	void closeIfNeeded();

	virtual void loadAsset();
	/*virtual*/ void saveIfNeeded(bool sync = true);
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
	
protected:
	static void* createScriptEdPanel(void* userdata);


protected:

	// Can safely close only after both text and bytecode are uploaded
	S32 mPendingUploads;

};


// Used to view and edit an LSL that is attached to an object.
class LLLiveLSLEditor : public LLScriptEdContainer
{
	friend class LLLiveLSLFile;
public: 
	LLLiveLSLEditor(const LLSD& key);


	static void processScriptRunningReply(LLMessageSystem* msg, void**);

	virtual void callbackLSLCompileSucceeded(const LLUUID& task_id,
											const LLUUID& item_id,
											bool is_script_running);
	virtual void callbackLSLCompileFailed(const LLSD& compile_errors);

	/*virtual*/ BOOL postBuild();
	
	void setIsNew() { mIsNew = TRUE; }
	
private:
	virtual BOOL canClose();
	void closeIfNeeded();
	virtual void draw();

	virtual void loadAsset();
	void loadAsset(BOOL is_new);
	/*virtual*/ void saveIfNeeded(bool sync = true);
	void uploadAssetViaCaps(const std::string& url,
							const std::string& filename, 
							const LLUUID& task_id,
							const LLUUID& item_id,
							BOOL is_running);
	void uploadAssetLegacy(const std::string& filename,
						   LLViewerObject* object,
						   const LLTransactionID& tid,
						   BOOL is_running);
	BOOL monoChecked() const;


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

	void loadScriptText(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type);

	static void onErrorList(LLUICtrl*, void* user_data);

	static void* createScriptEdPanel(void* userdata);

	static void	onMonoCheckboxClicked(LLUICtrl*, void* userdata);

private:
	bool				mIsNew;
	//LLUUID mTransmitID;
	LLCheckBoxCtrl*		mRunningCheckbox;
	BOOL				mAskedForRunningInfo;
	BOOL				mHaveRunningInfo;
	LLButton*			mResetButton;
	LLPointer<LLViewerInventoryItem> mItem;
	BOOL				mCloseAfterSave;
	// need to save both text and script, so need to decide when done
	S32					mPendingUploads;

	BOOL getIsModifiable() const { return mIsModifiable; } // Evaluated on load assert
	
	LLCheckBoxCtrl*	mMonoCheckbox;
	BOOL mIsModifiable;
};

#endif  // LL_LLPREVIEWSCRIPT_H
