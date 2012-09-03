/** 
 * @file llviewerwearable.h
 * @brief LLViewerWearable class header file
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

#ifndef LL_VIEWER_WEARABLE_H
#define LL_VIEWER_WEARABLE_H

#include "llwearable.h"
#include "llvoavatardefines.h"

class LLViewerWearable : public LLWearable
{
	friend class LLWearableList;

	//--------------------------------------------------------------------
	// Constructors and destructors
	//--------------------------------------------------------------------
private:
	// Private constructors used by LLViewerWearableList
	LLViewerWearable(const LLTransactionID& transactionID);
	LLViewerWearable(const LLAssetID& assetID);
public:
	virtual ~LLViewerWearable();

	//--------------------------------------------------------------------
	// Accessors
	//--------------------------------------------------------------------
public:
	const LLUUID&				getItemID() const { return mItemID; }
	const LLAssetID&			getAssetID() const { return mAssetID; }
	const LLTransactionID&		getTransactionID() const { return mTransactionID; }
	void						setItemID(const LLUUID& item_id);

public:

	BOOL				isDirty() const;
	BOOL				isOldVersion() const;

	/*virtual*/ void	writeToAvatar();
	void				removeFromAvatar( BOOL upload_bake )	{ LLViewerWearable::removeFromAvatar( mType, upload_bake ); }
	static void			removeFromAvatar( LLWearableType::EType type, BOOL upload_bake ); 

	/*virtual*/ BOOL	exportFile(LLFILE* file) const;
	/*virtual*/ EImportResult	importFile(LLFILE* file);
	
	void				setParamsToDefaults();
	void				setTexturesToDefaults();

	static const LLUUID			getDefaultTextureImageID(LLVOAvatarDefines::ETextureIndex index);


	void				saveNewAsset() const;
	static void			onSaveNewAssetComplete( const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status );

	void				copyDataFrom(const LLViewerWearable* src);

	friend std::ostream& operator<<(std::ostream &s, const LLViewerWearable &w);

	/*virtual*/ LLLocalTextureObject* getLocalTextureObject(S32 index);
	const LLLocalTextureObject* getLocalTextureObject(S32 index) const;
	std::vector<LLLocalTextureObject*> getLocalTextureListSeq();
	void				setLocalTextureObject(S32 index, LLLocalTextureObject &lto);
	void				setVisualParams();

	void				revertValues();
	void				saveValues();
	void				pullCrossWearableValues();		

	BOOL				isOnTop() const;

	// Something happened that requires the wearable's label to be updated (e.g. worn/unworn).
	void				setLabelUpdated() const;

	// the wearable was worn. make sure the name of the wearable object matches the LLViewerInventoryItem,
	// not the wearable asset itself.
	void				refreshName();

protected:
	typedef std::map<S32, LLLocalTextureObject*> te_map_t;

	void 				createLayers(S32 te);
	/*virtual*/void 	createVisualParams();

	void				syncImages(te_map_t &src, te_map_t &dst);
	void				destroyTextures();	
	LLAssetID			mAssetID;
	LLTransactionID		mTransactionID;

	te_map_t mTEMap;				// maps TE to LocalTextureObject
	te_map_t mSavedTEMap;			// last saved version of TEMap
	LLUUID				mItemID;  // ID of the inventory item in the agent's inventory	
};


#endif  // LL_VIEWER_WEARABLE_H

