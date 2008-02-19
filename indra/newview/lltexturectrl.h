/** 
 * @file lltexturectrl.h
 * @author Richard Nelson, James Cook
 * @brief LLTextureCtrl class header file including related functions
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

#ifndef LL_LLTEXTURECTRL_H
#define LL_LLTEXTURECTRL_H

#include "llcoord.h"
#include "llfloater.h"
#include "llstring.h"
#include "lluictrl.h"
#include "llpermissionsflags.h"

class LLButton;
class LLFloaterTexturePicker;
class LLInventoryItem;
class LLTextBox;
class LLViewBorder;
class LLViewerImage;

// used for setting drag & drop callbacks.
typedef BOOL (*drag_n_drop_callback)(LLUICtrl*, LLInventoryItem*, void*);

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
	LLTextureCtrl(
		const std::string& name, const LLRect& rect,
		const std::string& label,
		const LLUUID& image_id,
		const LLUUID& default_image_id, 
		const std::string& default_image_name );
	virtual ~LLTextureCtrl();

	// LLView interface
	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_TEXTURE_PICKER; }
	virtual LLString getWidgetTag() const { return LL_TEXTURE_CTRL_TAG; }
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask,
						BOOL drop, EDragAndDropType cargo_type, void *cargo_data,
						EAcceptance *accept,
						LLString& tooltip_msg);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent);

	virtual void	draw();
	virtual void	setVisible( BOOL visible );
	virtual void	setEnabled( BOOL enabled );

	virtual BOOL	isDirty() const;
	virtual void	resetDirty();

	void			setValid(BOOL valid);

	// LLUICtrl interface
	virtual void	clear();

	// Takes a UUID, wraps get/setImageAssetID
	virtual void	setValue( LLSD value );
	virtual LLSD	getValue() const;

	// LLTextureCtrl interface
	void			showPicker(BOOL take_focus);
	void			setLabel(const LLString& label);
	const LLString&	getLabel() const							{ return mLabel; }

	void			setAllowNoTexture( BOOL b )					{ mAllowNoTexture = b; }
	bool			getAllowNoTexture() const					{ return mAllowNoTexture; }

	const LLUUID&	getImageItemID() { return mImageItemID; }

	void			setImageAssetID(const LLUUID &image_asset_id);
	const LLUUID&	getImageAssetID() const						{ return mImageAssetID; }

	void			setDefaultImageAssetID( const LLUUID& id )	{ mDefaultImageAssetID = id; }

	const LLString&	getDefaultImageName() const					{ return mDefaultImageName; }
	const LLUUID&	getDefaultImageAssetID() const				{ return mDefaultImageAssetID; }

	void			setCaption(const LLString& caption);
	void			setCanApplyImmediately(BOOL b);

	void			setImmediateFilterPermMask(PermissionMask mask)
					{ mImmediateFilterPermMask = mask; }
	void			setNonImmediateFilterPermMask(PermissionMask mask)
					{ mNonImmediateFilterPermMask = mask; }
	PermissionMask	getImmediateFilterPermMask() { return mImmediateFilterPermMask; }
	PermissionMask	getNonImmediateFilterPermMask() { return mNonImmediateFilterPermMask; }

	void			closeFloater();

	void			onFloaterClose();
	void			onFloaterCommit(ETexturePickOp op);

	// This call is returned when a drag is detected. Your callback
	// should return TRUE if the drag is acceptable.
	void setDragCallback(drag_n_drop_callback cb)	{ mDragCallback = cb; }

	// This callback is called when the drop happens. Return TRUE if
	// the drop happened - resulting in an on commit callback, but not
	// necessariliy any other change.
	void setDropCallback(drag_n_drop_callback cb)	{ mDropCallback = cb; }

	void setOnCancelCallback(LLUICtrlCallback cb)	{ mOnCancelCallback = cb; }
	
	void setOnSelectCallback(LLUICtrlCallback cb)	{ mOnSelectCallback = cb; }

private:
	BOOL allowDrop(LLInventoryItem* item);
	BOOL doDrop(LLInventoryItem* item);

private:
	drag_n_drop_callback	 mDragCallback;
	drag_n_drop_callback	 mDropCallback;
	LLUICtrlCallback		 mOnCancelCallback;
	LLUICtrlCallback		 mOnSelectCallback;
	LLPointer<LLViewerImage> mTexturep;
	LLColor4				 mBorderColor;
	LLUUID					 mImageItemID;
	LLUUID					 mImageAssetID;
	LLUUID					 mDefaultImageAssetID;
	LLString				 mDefaultImageName;
	LLHandle<LLFloater>			 mFloaterHandle;
	LLTextBox*				 mTentativeLabel;
	LLTextBox*				 mCaption;
	LLString				 mLabel;
	BOOL					 mAllowNoTexture; // If true, the user can select "none" as an option
	LLCoordGL				 mLastFloaterLeftTop;
	PermissionMask			 mImmediateFilterPermMask;
	PermissionMask			 mNonImmediateFilterPermMask;
	BOOL					 mCanApplyImmediately;
	BOOL					 mNeedsRawImageData;
	LLViewBorder*			 mBorder;
	BOOL					 mValid;
	BOOL					 mDirty;
};

// XUI HACK: When floaters converted, switch this file to lltexturepicker.h/cpp
// and class to LLTexturePicker
#define LLTexturePicker LLTextureCtrl

#endif  // LL_LLTEXTURECTRL_H
