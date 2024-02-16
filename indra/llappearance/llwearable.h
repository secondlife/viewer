/** 
 * @file llwearable.h
 * @brief LLWearable class header file
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

#ifndef LL_LLWEARABLE_H
#define LL_LLWEARABLE_H

#include "llavatarappearancedefines.h"
#include "llpermissions.h"
#include "llsaleinfo.h"
#include "llwearabletype.h"

class LLVisualParam;
class LLTexGlobalColorInfo;
class LLTexGlobalColor;
class LLLocalTextureObject;
class LLAvatarAppearance;

// Abstract class.
class LLWearable
{
	//--------------------------------------------------------------------
	// Constructors and destructors
	//--------------------------------------------------------------------
public:
	LLWearable();
	virtual ~LLWearable();

	//--------------------------------------------------------------------
	// Accessors
	//--------------------------------------------------------------------
public:
	LLWearableType::EType		getType() const	{ return mType; }
	void						setType(LLWearableType::EType type, LLAvatarAppearance *avatarp);
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
	S32							getDefinitionVersion() const { return mDefinitionVersion; }
	void						setDefinitionVersion( S32 new_version ) { mDefinitionVersion = new_version; }
	static S32					getCurrentDefinitionVersion() { return LLWearable::sCurrentDefinitionVersion; }

public:
	typedef std::vector<LLVisualParam*> visual_param_vec_t;

	virtual void	writeToAvatar(LLAvatarAppearance* avatarp);

	enum EImportResult
	{
		FAILURE = 0,
		SUCCESS,
		BAD_HEADER
	};
	bool				exportFile(const std::string& filename) const;
	EImportResult		importFile(const std::string& filename, LLAvatarAppearance* avatarp );
	virtual bool				exportStream( std::ostream& output_stream ) const;
	virtual EImportResult		importStream( std::istream& input_stream, LLAvatarAppearance* avatarp );

	static void			setCurrentDefinitionVersion( S32 version ) { LLWearable::sCurrentDefinitionVersion = version; }
	virtual LLUUID		getDefaultTextureImageID(LLAvatarAppearanceDefines::ETextureIndex index) const = 0;

	LLLocalTextureObject* getLocalTextureObject(S32 index);
	const LLLocalTextureObject* getLocalTextureObject(S32 index) const;
	std::vector<LLLocalTextureObject*> getLocalTextureListSeq();

	void				setLocalTextureObject(S32 index, LLLocalTextureObject &lto);
	void				addVisualParam(LLVisualParam *param);
	void 				setVisualParamWeight(S32 index, F32 value);
	F32					getVisualParamWeight(S32 index) const;
	LLVisualParam*		getVisualParam(S32 index) const;
	void				getVisualParams(visual_param_vec_t &list);
	void				animateParams(F32 delta);

	LLColor4			getClothesColor(S32 te) const;
	void 				setClothesColor( S32 te, const LLColor4& new_color);

	virtual void		revertValues();
	virtual void		saveValues();

	// Something happened that requires the wearable to be updated (e.g. worn/unworn).
	virtual void		setUpdated() const = 0;

	typedef std::map<S32, LLVisualParam *>    visual_param_index_map_t;
	visual_param_index_map_t mVisualParamIndexMap;

protected:
	typedef std::map<S32, LLLocalTextureObject*> te_map_t;
	void				syncImages(te_map_t &src, te_map_t &dst);
	void				destroyTextures();
	void			 	createVisualParams(LLAvatarAppearance *avatarp);
	void 				createLayers(S32 te, LLAvatarAppearance *avatarp);
	bool				getNextPopulatedLine(std::istream& input_stream, char* buffer, U32 buffer_size);

	static S32			sCurrentDefinitionVersion;	// Depends on the current state of the avatar_lad.xml.
	S32					mDefinitionVersion;			// Depends on the state of the avatar_lad.xml when this asset was created.
	std::string			mName;
	std::string			mDescription;
	LLPermissions		mPermissions;
	LLSaleInfo			mSaleInfo;
	LLWearableType::EType		mType;

	typedef std::map<S32, F32> param_map_t;
	param_map_t mSavedVisualParamMap; // last saved version of visual params

	te_map_t mTEMap;				// maps TE to LocalTextureObject
	te_map_t mSavedTEMap;			// last saved version of TEMap
};

#endif  // LL_LLWEARABLE_H
