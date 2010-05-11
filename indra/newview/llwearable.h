/** 
 * @file llwearable.h
 * @brief LLWearable class header file
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

#ifndef LL_LLWEARABLE_H
#define LL_LLWEARABLE_H

#include "lluuid.h"
#include "llstring.h"
#include "llpermissions.h"
#include "llsaleinfo.h"
#include "llassetstorage.h"
#include "llwearabledictionary.h"
#include "llfile.h"
#include "lllocaltextureobject.h"

class LLViewerInventoryItem;
class LLVisualParam;
class LLTexGlobalColorInfo;
class LLTexGlobalColor;

class LLWearable
{
	friend class LLWearableList;

	//--------------------------------------------------------------------
	// Constructors and destructors
	//--------------------------------------------------------------------
private:
	// Private constructors used by LLWearableList
	LLWearable(const LLTransactionID& transactionID);
	LLWearable(const LLAssetID& assetID);
public:
	virtual ~LLWearable();

	//--------------------------------------------------------------------
	// Accessors
	//--------------------------------------------------------------------
public:
	const LLUUID&				getItemID() const;
	const LLAssetID&			getAssetID() const { return mAssetID; }
	const LLTransactionID&		getTransactionID() const { return mTransactionID; }
	LLWearableType::EType				getType() const	{ return mType; }
	void						setType(LLWearableType::EType type);
	const std::string&			getName() const	{ return mName; }
	void						setName(const std::string& name) { mName = name; }
	const std::string&			getDescription() const { return mDescription; }
	void						setDescription(const std::string& desc)	{ mDescription = desc; }
	const LLPermissions& 		getPermissions() const { return mPermissions; }
	void						setPermissions(const LLPermissions& p) { mPermissions = p; }
	const LLSaleInfo&			getSaleInfo() const	{ return mSaleInfo; }
	void						setSaleInfo(const LLSaleInfo& info)	{ mSaleInfo = info; }
	const std::string&			getTypeLabel() const;
	const std::string&			getTypeName() const;
	LLAssetType::EType			getAssetType() const;
	LLLocalTextureObject*		getLocalTextureObject(S32 index) const;
	S32							getDefinitionVersion() const { return mDefinitionVersion; }
	void						setDefinitionVersion( S32 new_version ) { mDefinitionVersion = new_version; }

public:
	typedef std::vector<LLVisualParam*> visual_param_vec_t;

	BOOL				isDirty() const;
	BOOL				isOldVersion() const;

	void				writeToAvatar();
	void				removeFromAvatar( BOOL upload_bake )	{ LLWearable::removeFromAvatar( mType, upload_bake ); }
	static void			removeFromAvatar( LLWearableType::EType type, BOOL upload_bake ); 

	BOOL				exportFile(LLFILE* file) const;
	BOOL				importFile(LLFILE* file);
	
	void				setParamsToDefaults();
	void				setTexturesToDefaults();

	void				saveNewAsset() const;
	static void			onSaveNewAssetComplete( const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status );

	void				copyDataFrom(const LLWearable* src);

	static void			setCurrentDefinitionVersion( S32 version ) { LLWearable::sCurrentDefinitionVersion = version; }

	friend std::ostream& operator<<(std::ostream &s, const LLWearable &w);
	void				setItemID(const LLUUID& item_id);

	LLLocalTextureObject* getLocalTextureObject(S32 index);
	const LLLocalTextureObject* getConstLocalTextureObject(S32 index) const;

	void				setLocalTextureObject(S32 index, LLLocalTextureObject &lto);
	void				addVisualParam(LLVisualParam *param);
	void				setVisualParams();
	void 				setVisualParamWeight(S32 index, F32 value, BOOL upload_bake);
	F32					getVisualParamWeight(S32 index) const;
	LLVisualParam*		getVisualParam(S32 index) const;
	void				getVisualParams(visual_param_vec_t &list);
	void				animateParams(F32 delta, BOOL upload_bake);

	LLColor4			getClothesColor(S32 te) const;
	void 				setClothesColor( S32 te, const LLColor4& new_color, BOOL upload_bake );

	void				revertValues();
	void				saveValues();
	void				pullCrossWearableValues();		

	BOOL				isOnTop() const;

	// Something happened that requires the wearable's label to be updated (e.g. worn/unworn).
	void				setLabelUpdated() const;

	// the wearable was worn. make sure the name of the wearable object matches the LLViewerInventoryItem,
	// not the wearable asset itself.
	void				refreshName();

private:
	typedef std::map<S32, LLLocalTextureObject*> te_map_t;
	typedef std::map<S32, LLVisualParam *>    visual_param_index_map_t;

	void 				createLayers(S32 te);
	void 				createVisualParams();
	void				syncImages(te_map_t &src, te_map_t &dst);
	void				destroyTextures();	

	static S32			sCurrentDefinitionVersion;	// Depends on the current state of the avatar_lad.xml.
	S32					mDefinitionVersion;			// Depends on the state of the avatar_lad.xml when this asset was created.
	std::string			mName;
	std::string			mDescription;
	LLPermissions		mPermissions;
	LLSaleInfo			mSaleInfo;
	LLAssetID mAssetID;
	LLTransactionID		mTransactionID;
	LLWearableType::EType		mType;

	typedef std::map<S32, F32> param_map_t;
	param_map_t mSavedVisualParamMap; // last saved version of visual params

	visual_param_index_map_t mVisualParamIndexMap;

	te_map_t mTEMap;				// maps TE to LocalTextureObject
	te_map_t mSavedTEMap;			// last saved version of TEMap
	LLUUID				mItemID;  // ID of the inventory item in the agent's inventory
};

#endif  // LL_LLWEARABLE_H
