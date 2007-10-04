/** 
 * @file llwearable.h
 * @brief LLWearable class header file
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

#ifndef LL_LLWEARABLE_H
#define LL_LLWEARABLE_H

#include "lluuid.h"
#include "llptrskipmap.h"
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
public:
	LLWearable(const LLTransactionID& transactionID);
	LLWearable(const LLAssetID& assetID);
	~LLWearable();

	const LLAssetID&		getID() { return mAssetID; }
	const LLTransactionID&		getTransactionID() { return mTransactionID; }

	BOOL				readData( const char *buffer );
	BOOL				isDirty();
	BOOL				isOldVersion();

	void				writeToAvatar( BOOL set_by_user );
	void				readFromAvatar();
	void				removeFromAvatar( BOOL set_by_user )	{ LLWearable::removeFromAvatar( mType, set_by_user ); }
	static void			removeFromAvatar( EWearableType type, BOOL set_by_user ); 

	BOOL				exportFile(FILE* file);
	BOOL				importFile(FILE* file);

	EWearableType		getType() const							{ return mType; }
	void				setType( EWearableType type )			{ mType = type; }

	void				setName( const std::string& name )				{ mName = name; }
	const std::string&	getName()								{ return mName; }

	void				setDescription( const std::string& desc )		{ mDescription = desc; }
	const std::string&	getDescription()						{ return mDescription; }

	void				setPermissions( const LLPermissions& p ) { mPermissions = p; }
	const LLPermissions& getPermissions()						{ return mPermissions; }

	void				setSaleInfo( const LLSaleInfo& info )	{ mSaleInfo = info; }
	const LLSaleInfo&	getSaleInfo()							{ return mSaleInfo; }

	const char*			getTypeLabel() const					{ return LLWearable::sTypeLabel[ mType ]; }
	const char*			getTypeName() const						{ return LLWearable::sTypeName[ mType ]; }

	void				setParamsToDefaults();
	void				setTexturesToDefaults();

	LLAssetType::EType	getAssetType() const					{ return LLWearable::typeToAssetType( mType ); }

	static EWearableType typeNameToType( const LLString& type_name );
	static const char*	typeToTypeName( EWearableType type )	{ return (type<WT_COUNT) ? LLWearable::sTypeName[type] : "invalid"; }
	static const char*	typeToTypeLabel( EWearableType type )	{ return (type<WT_COUNT) ? LLWearable::sTypeLabel[type] : "invalid"; }
	static LLAssetType::EType typeToAssetType( EWearableType wearable_type );

	void				saveNewAsset();
	static void			onSaveNewAssetComplete( const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status );

	BOOL				isMatchedToInventoryItem( LLViewerInventoryItem* item );

	void				copyDataFrom( LLWearable* src );

	static void			setCurrentDefinitionVersion( S32 version ) { LLWearable::sCurrentDefinitionVersion = version; }

	void				dump();

private:
	static S32			sCurrentDefinitionVersion;	// Depends on the current state of the avatar_lad.xml.
	S32					mDefinitionVersion;			// Depends on the state of the avatar_lad.xml when this asset was created.
	LLString			mName;
	LLString			mDescription;
	LLPermissions		mPermissions;
	LLSaleInfo			mSaleInfo;
	LLAssetID mAssetID;
	LLTransactionID		mTransactionID;
	EWearableType		mType;

	LLPtrSkipMap<S32, F32*>	mVisualParamMap;	// maps visual param id to weight
	LLPtrSkipMap<S32, LLUUID*>	mTEMap;				// maps TE to Image ID

	static const char* sTypeName[ WT_COUNT ];
	static const char* sTypeLabel[ WT_COUNT ];
};

#endif  // LL_LLWEARABLE_H
