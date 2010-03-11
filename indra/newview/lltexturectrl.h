/** 
 * @file lltexturectrl.h
 * @author Richard Nelson, James Cook
 * @brief LLTextureCtrl class header file including related functions
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLTEXTURECTRL_H
#define LL_LLTEXTURECTRL_H

#include "llcoord.h"
#include "llfloater.h"
#include "llstring.h"
#include "lluictrl.h"
#include "llpermissionsflags.h"
#include "lltextbox.h" // for params
#include "llviewborder.h" // for params

class LLButton;
class LLFloaterTexturePicker;
class LLInventoryItem;
class LLViewerFetchedTexture;

// used for setting drag & drop callbacks.
typedef boost::function<BOOL (LLUICtrl*, LLInventoryItem*)> drag_n_drop_callback;


//////////////////////////////////////////////////////////////////////////////////////////
// LLTextureCtrl


class LLTextureCtrl
: public LLUICtrl
{
public:
	typedef enum e_texture_pick_op
	{
		TEXTURE_CHANGE,
		TEXTURE_SELECT,
		TEXTURE_CANCEL
	} ETexturePickOp;

public:
	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<LLUUID>		image_id;
		Optional<LLUUID>		default_image_id;
		Optional<std::string>	default_image_name;
		Optional<bool>			allow_no_texture;
		Optional<bool>			can_apply_immediately;
		Optional<bool>			no_commit_on_selection; // alternative mode: commit occurs and the widget gets dirty
														// only on DnD or when OK is pressed in the picker
		Optional<S32>			label_width;
		Optional<LLUIColor>		border_color;
		
		Optional<LLTextBox::Params>	multiselect_text,
									caption_text;

		Optional<LLViewBorder::Params> border;

		Params()
		:	image_id("image"),
			default_image_id("default_image_id"),
			default_image_name("default_image_name"),
			allow_no_texture("allow_no_texture"),
			can_apply_immediately("can_apply_immediately"),
			no_commit_on_selection("no_commit_on_selection", false),
		    label_width("label_width", -1),
			border_color("border_color"),
			multiselect_text("multiselect_text"),
			caption_text("caption_text"),
			border("border")
		{
			name = "texture picker";
			mouse_opaque(true);
			follows.flags(FOLLOWS_LEFT | FOLLOWS_TOP);
		}
	};
protected:
	LLTextureCtrl(const Params&);
	friend class LLUICtrlFactory;
public:
	virtual ~LLTextureCtrl();

	// LLView interface

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask,
						BOOL drop, EDragAndDropType cargo_type, void *cargo_data,
						EAcceptance *accept,
						std::string& tooltip_msg);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleUnicodeCharHere(llwchar uni_char);

	virtual void	draw();
	virtual void	setVisible( BOOL visible );
	virtual void	setEnabled( BOOL enabled );

	void			setValid(BOOL valid);

	// LLUICtrl interface
	virtual void	clear();

	// Takes a UUID, wraps get/setImageAssetID
	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const;

	// LLTextureCtrl interface
	void			showPicker(BOOL take_focus);
	void			setLabel(const std::string& label);
	void			setLabelWidth(S32 label_width) {mLabelWidth =label_width;}	
	const std::string&	getLabel() const							{ return mLabel; }

	void			setAllowNoTexture( BOOL b )					{ mAllowNoTexture = b; }
	bool			getAllowNoTexture() const					{ return mAllowNoTexture; }

	const LLUUID&	getImageItemID() { return mImageItemID; }

	void			setImageAssetID(const LLUUID &image_asset_id);
	const LLUUID&	getImageAssetID() const						{ return mImageAssetID; }

	void			setDefaultImageAssetID( const LLUUID& id )	{ mDefaultImageAssetID = id; }
	const LLUUID&	getDefaultImageAssetID() const { return mDefaultImageAssetID; }

	const std::string&	getDefaultImageName() const					{ return mDefaultImageName; }

	void			setFallbackImageName( const std::string& name ) { mFallbackImageName = name; }			
	const std::string& 	getFallbackImageName() const { return mFallbackImageName; }	   

	void			setCaption(const std::string& caption);
	void			setCanApplyImmediately(BOOL b);

	void			setImmediateFilterPermMask(PermissionMask mask)
					{ mImmediateFilterPermMask = mask; }
	void			setNonImmediateFilterPermMask(PermissionMask mask)
					{ mNonImmediateFilterPermMask = mask; }
	PermissionMask	getImmediateFilterPermMask() { return mImmediateFilterPermMask; }
	PermissionMask	getNonImmediateFilterPermMask() { return mNonImmediateFilterPermMask; }

	void			closeDependentFloater();

	void			onFloaterClose();
	void			onFloaterCommit(ETexturePickOp op);

	// This call is returned when a drag is detected. Your callback
	// should return TRUE if the drag is acceptable.
	void setDragCallback(drag_n_drop_callback cb)	{ mDragCallback = cb; }

	// This callback is called when the drop happens. Return TRUE if
	// the drop happened - resulting in an on commit callback, but not
	// necessariliy any other change.
	void setDropCallback(drag_n_drop_callback cb)	{ mDropCallback = cb; }
	
	void setOnCancelCallback(commit_callback_t cb)	{ mOnCancelCallback = cb; }
	
	void setOnSelectCallback(commit_callback_t cb)	{ mOnSelectCallback = cb; }

	void setShowLoadingPlaceholder(BOOL showLoadingPlaceholder);

	LLViewerFetchedTexture* getTexture() { return mTexturep; }

private:
	BOOL allowDrop(LLInventoryItem* item);
	BOOL doDrop(LLInventoryItem* item);

private:
	drag_n_drop_callback	 mDragCallback;
	drag_n_drop_callback	 mDropCallback;
	commit_callback_t		 mOnCancelCallback;
	commit_callback_t		 mOnSelectCallback;
	LLPointer<LLViewerFetchedTexture> mTexturep;
	LLUIColor				 mBorderColor;
	LLUUID					 mImageItemID;
	LLUUID					 mImageAssetID;
	LLUUID					 mDefaultImageAssetID;
	std::string				 mFallbackImageName;
	std::string				 mDefaultImageName;
	LLHandle<LLFloater>			 mFloaterHandle;
	LLTextBox*				 mTentativeLabel;
	LLTextBox*				 mCaption;
	std::string				 mLabel;
	BOOL					 mAllowNoTexture; // If true, the user can select "none" as an option
	PermissionMask			 mImmediateFilterPermMask;
	PermissionMask			 mNonImmediateFilterPermMask;
	BOOL					 mCanApplyImmediately;
	BOOL					 mCommitOnSelection;
	BOOL					 mNeedsRawImageData;
	LLViewBorder*			 mBorder;
	BOOL					 mValid;
	BOOL					 mShowLoadingPlaceholder;
	std::string				 mLoadingPlaceholderString;
	S32						 mLabelWidth;
};

// XUI HACK: When floaters converted, switch this file to lltexturepicker.h/cpp
// and class to LLTexturePicker
#define LLTexturePicker LLTextureCtrl

#endif  // LL_LLTEXTURECTRL_H
