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

#include "llextendedstatus.h"
//#include "lluuid.h"
//#include "llstring.h"
#include "llpermissions.h"
#include "llsaleinfo.h"
//#include "llassetstorage.h"
#include "llwearabletype.h"
//#include "llfile.h"
#include "lllocaltextureobject.h"

class LLVisualParam;
class LLTexGlobalColorInfo;
class LLTexGlobalColor;

class LLWearable
{
	//--------------------------------------------------------------------
	// Constructors and destructors
	//--------------------------------------------------------------------
public:
	virtual ~LLWearable();

	//--------------------------------------------------------------------
	// Accessors
	//--------------------------------------------------------------------
public:
	LLWearableType::EType		getType() const	{ return mType; }
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
	S32							getDefinitionVersion() const { return mDefinitionVersion; }
	void						setDefinitionVersion( S32 new_version ) { mDefinitionVersion = new_version; }

public:
	typedef std::vector<LLVisualParam*> visual_param_vec_t;

	enum EImportResult
	{
		FAILURE = 0,
		SUCCESS,
		BAD_HEADER
	};
	virtual BOOL				exportFile(LLFILE* file) const;
	virtual EImportResult		importFile(LLFILE* file);

	virtual LLLocalTextureObject* getLocalTextureObject(S32 index) = 0;
	virtual void	writeToAvatar() = 0;


	static void			setCurrentDefinitionVersion( S32 version ) { LLWearable::sCurrentDefinitionVersion = version; }

	void				addVisualParam(LLVisualParam *param);
	void 				setVisualParamWeight(S32 index, F32 value, BOOL upload_bake);
	F32					getVisualParamWeight(S32 index) const;
	LLVisualParam*		getVisualParam(S32 index) const;
	void				getVisualParams(visual_param_vec_t &list);
	void				animateParams(F32 delta, BOOL upload_bake);

	LLColor4			getClothesColor(S32 te) const;
	void 				setClothesColor( S32 te, const LLColor4& new_color, BOOL upload_bake );

	typedef std::map<S32, LLUUID> texture_id_map_t;
	const texture_id_map_t& getTextureIDMap() const { return mTextureIDMap; }

protected:
	virtual void 	createVisualParams() = 0;

	static S32			sCurrentDefinitionVersion;	// Depends on the current state of the avatar_lad.xml.
	S32					mDefinitionVersion;			// Depends on the state of the avatar_lad.xml when this asset was created.
	std::string			mName;
	std::string			mDescription;
	LLPermissions		mPermissions;
	LLSaleInfo			mSaleInfo;
	LLWearableType::EType		mType;

	typedef std::map<S32, F32> param_map_t;
	param_map_t mSavedVisualParamMap; // last saved version of visual params

	typedef std::map<S32, LLVisualParam *>    visual_param_index_map_t;
	visual_param_index_map_t mVisualParamIndexMap;

	// *TODO: Lazy mutable.  Find a better way?
	mutable texture_id_map_t mTextureIDMap;
};

#endif  // LL_LLWEARABLE_H
