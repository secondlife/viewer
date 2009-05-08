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

class LLViewerInventoryItem;

enum	EWearableType  // If you change this, update LLWearable::getTypeName(), getTypeLabel(), and LLVOAvatar::getTEWearableType()
{
	WT_SHAPE	= 0,
	WT_SKIN		= 1,
	WT_HAIR		= 2,
	WT_EYES		= 3,
	WT_SHIRT	= 4,
	WT_PANTS	= 5,
	WT_SHOES	= 6,
	WT_SOCKS	= 7,
	WT_JACKET	= 8,
	WT_GLOVES	= 9,
	WT_UNDERSHIRT = 10,
	WT_UNDERPANTS = 11,
	WT_SKIRT	= 12,
	WT_COUNT	= 13,
	WT_INVALID	= 255
};

class LLWearable
{
	friend class LLWearableList;
public:
	~LLWearable();

	const LLAssetID&		getID() const { return mAssetID; }
	const LLTransactionID&		getTransactionID() const { return mTransactionID; }

	BOOL				isDirty();
	BOOL				isOldVersion();

	void				writeToAvatar( BOOL set_by_user );
	void				readFromAvatar();
	void				removeFromAvatar( BOOL set_by_user )	{ LLWearable::removeFromAvatar( mType, set_by_user ); }
	static void			removeFromAvatar( EWearableType type, BOOL set_by_user ); 

	BOOL				exportFile(LLFILE* file);
	BOOL				importFile(LLFILE* file);

	static void			initClass ();
	EWearableType		getType() const							{ return mType; }
	void				setType( EWearableType type )			{ mType = type; }

	void				setName( const std::string& name )			{ mName = name; }
	const std::string&	getName() const								{ return mName; }

	void				setDescription( const std::string& desc )	{ mDescription = desc; }
	const std::string&	getDescription() const						{ return mDescription; }

	void				setPermissions( const LLPermissions& p ) { mPermissions = p; }
	const LLPermissions& getPermissions() const						{ return mPermissions; }

	void				setSaleInfo( const LLSaleInfo& info )	{ mSaleInfo = info; }
	const LLSaleInfo&	getSaleInfo() const							{ return mSaleInfo; }

	const std::string&	getTypeLabel() const					{ return LLWearable::sTypeLabel[ mType ]; }
	const std::string&	getTypeName() const						{ return LLWearable::sTypeName[ mType ]; }

	void				setParamsToDefaults();
	void				setTexturesToDefaults();

	LLAssetType::EType	getAssetType() const					{ return LLWearable::typeToAssetType( mType ); }

	static EWearableType typeNameToType( const std::string& type_name );
	static const std::string& typeToTypeName( EWearableType type )	{ return LLWearable::sTypeName[llmin(type,WT_COUNT)]; }
	static const std::string& typeToTypeLabel( EWearableType type )	{ return LLWearable::sTypeLabel[llmin(type,WT_COUNT)]; }
	static LLAssetType::EType typeToAssetType( EWearableType wearable_type );

	void				saveNewAsset();
	static void			onSaveNewAssetComplete( const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status );

	BOOL				isMatchedToInventoryItem( LLViewerInventoryItem* item );

	void				copyDataFrom( LLWearable* src );

	static void			setCurrentDefinitionVersion( S32 version ) { LLWearable::sCurrentDefinitionVersion = version; }

	friend std::ostream& operator<<(std::ostream &s, const LLWearable &w);

private:
	// Private constructor used by LLWearableList
	LLWearable(const LLTransactionID& transactionID);
	LLWearable(const LLAssetID& assetID);

	static S32			sCurrentDefinitionVersion;	// Depends on the current state of the avatar_lad.xml.
	S32					mDefinitionVersion;			// Depends on the state of the avatar_lad.xml when this asset was created.
	std::string			mName;
	std::string			mDescription;
	LLPermissions		mPermissions;
	LLSaleInfo			mSaleInfo;
	LLAssetID mAssetID;
	LLTransactionID		mTransactionID;
	EWearableType		mType;

	typedef std::map<S32, F32> param_map_t;
	param_map_t mVisualParamMap;	// maps visual param id to weight
	typedef std::map<S32, LLUUID> te_map_t;
	te_map_t mTEMap;				// maps TE to Image ID

	static const std::string sTypeName[ WT_COUNT+1 ];
	static std::string sTypeLabel[ WT_COUNT+1 ];
};

#endif  // LL_LLWEARABLE_H
