/** 
 * @file inventory.cpp
 * @author Phoenix
 * @date 2005-11-15
 * @brief Functions for inventory test framework
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#include "linden_common.h"
#include "llsd.h"

#include "../llinventory.h"

#include "../test/lltut.h"


#if LL_WINDOWS
// disable unreachable code warnings
#pragma warning(disable: 4702)
#endif

LLPointer<LLInventoryItem> create_random_inventory_item()
{
	LLUUID item_id;
	item_id.generate();
	LLUUID parent_id;
	parent_id.generate();
	LLPermissions perm;
	LLUUID creator_id;
	creator_id.generate();
	LLUUID owner_id;
	owner_id.generate();
	LLUUID last_owner_id;
	last_owner_id.generate();
	LLUUID group_id;
	group_id.generate();
	perm.init(creator_id, owner_id, last_owner_id, group_id);
	perm.initMasks(PERM_ALL, PERM_ALL, PERM_COPY, PERM_COPY, PERM_MODIFY | PERM_COPY);
	LLUUID asset_id;
	asset_id.generate();
	S32 price = rand();
	LLSaleInfo sale_info(LLSaleInfo::FS_COPY, price);
	U32 flags = rand();
	S32 creation = time(NULL);

	LLPointer<LLInventoryItem> item = new LLInventoryItem(
		item_id,
		parent_id,
		perm,
		asset_id,
		LLAssetType::AT_OBJECT,
		LLInventoryType::IT_ATTACHMENT,
		std::string("Sample Object"),
		std::string("Used for Testing"),
		sale_info,
		flags,
		creation);
	return item;
}

LLPointer<LLInventoryCategory> create_random_inventory_cat()
{
	LLUUID item_id;
	item_id.generate();
	LLUUID parent_id;
	parent_id.generate();

	LLPointer<LLInventoryCategory> cat = new LLInventoryCategory(
		item_id,
		parent_id,
		LLFolderType::FT_NONE,
		std::string("Sample category"));
	return cat;
}

namespace tut
{
	struct inventory_data
	{
	};
	typedef test_group<inventory_data> inventory_test;
	typedef inventory_test::object inventory_object;
	tut::inventory_test inv("llinventory");

//***class LLInventoryType***//


	template<> template<>
	void inventory_object::test<1>()
	{
		LLInventoryType::EType retType =  LLInventoryType::lookup(std::string("sound"));
		ensure("1.LLInventoryType::lookup(char*) failed", retType == LLInventoryType::IT_SOUND);

		retType = LLInventoryType::lookup(std::string("snapshot"));
		ensure("2.LLInventoryType::lookup(char*) failed", retType == LLInventoryType::IT_SNAPSHOT);
	}

	template<> template<>
	void inventory_object::test<2>()
	{
		static std::string retType =  LLInventoryType::lookup(LLInventoryType::IT_CALLINGCARD);
		ensure("1.LLInventoryType::lookup(EType) failed", (retType == "callcard")); 

		retType = LLInventoryType::lookup(LLInventoryType::IT_LANDMARK);
		ensure("2.LLInventoryType::lookup(EType) failed", (retType == "landmark"));
		
	}

	template<> template<>
	void inventory_object::test<3>()
	{
		static std::string retType =  LLInventoryType::lookupHumanReadable(LLInventoryType::IT_CALLINGCARD);
		ensure("1.LLInventoryType::lookupHumanReadable(EType) failed", (retType == "calling card")); 

		retType =  LLInventoryType::lookupHumanReadable(LLInventoryType::IT_LANDMARK);
		ensure("2.LLInventoryType::lookupHumanReadable(EType) failed", (retType == "landmark")); 
	}

	template<> template<>
	void inventory_object::test<4>()
	{
		static LLInventoryType::EType retType =  LLInventoryType::defaultForAssetType(LLAssetType::AT_TEXTURE);
		ensure("1.LLInventoryType::defaultForAssetType(LLAssetType EType) failed", retType == LLInventoryType::IT_TEXTURE);

		retType =  LLInventoryType::defaultForAssetType(LLAssetType::AT_LANDMARK);
		ensure("2.LLInventoryType::defaultForAssetType(LLAssetType EType) failed", retType == LLInventoryType::IT_LANDMARK);
	}

//*****class LLInventoryItem*****//

	template<> template<>
	void inventory_object::test<5>()
	{
		LLPointer<LLInventoryItem> src = create_random_inventory_item();
		LLSD sd = ll_create_sd_from_inventory_item(src);
		//llinfos << "sd: " << *sd << llendl;
		LLPointer<LLInventoryItem> dst = new LLInventoryItem;
		bool successful_parse = dst->fromLLSD(sd);
		ensure_equals("0.LLInventoryItem::fromLLSD()", successful_parse, true);
		ensure_equals("1.item id::getUUID() failed", dst->getUUID(), src->getUUID());
		ensure_equals("2.parent::getParentUUID() failed", dst->getParentUUID(), src->getParentUUID());
		ensure_equals("3.name::getName() failed", dst->getName(), src->getName());
		ensure_equals("4.type::getType() failed", dst->getType(), src->getType());
		
		ensure_equals("5.permissions::getPermissions() failed", dst->getPermissions(), src->getPermissions());
		ensure_equals("6.description::getDescription() failed", dst->getDescription(), src->getDescription());
		ensure_equals("7.sale type::getSaleType() failed", dst->getSaleInfo().getSaleType(), src->getSaleInfo().getSaleType());
		ensure_equals("8.sale price::getSalePrice() failed", dst->getSaleInfo().getSalePrice(), src->getSaleInfo().getSalePrice());
		ensure_equals("9.asset id::getAssetUUID() failed", dst->getAssetUUID(), src->getAssetUUID());
		ensure_equals("10.inventory type::getInventoryType() failed", dst->getInventoryType(), src->getInventoryType());
		ensure_equals("11.flags::getFlags() failed", dst->getFlags(), src->getFlags());
		ensure_equals("12.creation::getCreationDate() failed", dst->getCreationDate(), src->getCreationDate());

		LLUUID new_item_id, new_parent_id;
		new_item_id.generate();
		src->setUUID(new_item_id);
		
		new_parent_id.generate();
		src->setParent(new_parent_id);
		
		std::string new_name = "LindenLab";
		src->rename(new_name);
		
		src->setType(LLAssetType::AT_SOUND);

		LLUUID new_asset_id;
		new_asset_id.generate();
		
		src->setAssetUUID(new_asset_id);
		std::string new_desc = "SecondLife Testing";
		src->setDescription(new_desc);
		
		S32 new_price = rand();
		LLSaleInfo new_sale_info(LLSaleInfo::FS_COPY, new_price);
		src->setSaleInfo(new_sale_info);

		U32 new_flags = rand();
		S32 new_creation = time(NULL);

		LLPermissions new_perm;

		LLUUID new_creator_id;
		new_creator_id.generate();
	
		LLUUID new_owner_id;
		new_owner_id.generate();

		LLUUID last_owner_id;
		last_owner_id.generate();

		LLUUID new_group_id;
		new_group_id.generate();
		new_perm.init(new_creator_id, new_owner_id, last_owner_id, new_group_id);
		new_perm.initMasks(PERM_ALL, PERM_ALL, PERM_COPY, PERM_COPY, PERM_MODIFY | PERM_COPY);
		src->setPermissions(new_perm);

		src->setInventoryType(LLInventoryType::IT_SOUND);
		src->setFlags(new_flags);
		src->setCreationDate(new_creation);

		sd = ll_create_sd_from_inventory_item(src);
		//llinfos << "sd: " << *sd << llendl;
		successful_parse = dst->fromLLSD(sd);
		ensure_equals("13.item id::getUUID() failed", dst->getUUID(), src->getUUID());
		ensure_equals("14.parent::getParentUUID() failed", dst->getParentUUID(), src->getParentUUID());
		ensure_equals("15.name::getName() failed", dst->getName(), src->getName());
		ensure_equals("16.type::getType() failed", dst->getType(), src->getType());
		
		ensure_equals("17.permissions::getPermissions() failed", dst->getPermissions(), src->getPermissions());
		ensure_equals("18.description::getDescription() failed", dst->getDescription(), src->getDescription());
		ensure_equals("19.sale type::getSaleType() failed type", dst->getSaleInfo().getSaleType(), src->getSaleInfo().getSaleType());
		ensure_equals("20.sale price::getSalePrice() failed price", dst->getSaleInfo().getSalePrice(), src->getSaleInfo().getSalePrice());
		ensure_equals("21.asset id::getAssetUUID() failed id", dst->getAssetUUID(), src->getAssetUUID());
		ensure_equals("22.inventory type::getInventoryType() failed type", dst->getInventoryType(), src->getInventoryType());
		ensure_equals("23.flags::getFlags() failed", dst->getFlags(), src->getFlags());
		ensure_equals("24.creation::getCreationDate() failed", dst->getCreationDate(), src->getCreationDate());
		
	}

	template<> template<>
	void inventory_object::test<6>()
	{
		LLPointer<LLInventoryItem> src = create_random_inventory_item();
		
		LLUUID new_item_id, new_parent_id;
		new_item_id.generate();
		src->setUUID(new_item_id);
		
		new_parent_id.generate();
		src->setParent(new_parent_id);
		
		std::string new_name = "LindenLab";
		src->rename(new_name);
		
		src->setType(LLAssetType::AT_SOUND);

		LLUUID new_asset_id;
		new_asset_id.generate();
		
		src->setAssetUUID(new_asset_id);
		std::string new_desc = "SecondLife Testing";
		src->setDescription(new_desc);
		
		S32 new_price = rand();
		LLSaleInfo new_sale_info(LLSaleInfo::FS_COPY, new_price);
		src->setSaleInfo(new_sale_info);

		U32 new_flags = rand();
		S32 new_creation = time(NULL);

		LLPermissions new_perm;

		LLUUID new_creator_id;
		new_creator_id.generate();
	
		LLUUID new_owner_id;
		new_owner_id.generate();

		LLUUID last_owner_id;
		last_owner_id.generate();

		LLUUID new_group_id;
		new_group_id.generate();
		new_perm.init(new_creator_id, new_owner_id, last_owner_id, new_group_id);
		new_perm.initMasks(PERM_ALL, PERM_ALL, PERM_COPY, PERM_COPY, PERM_MODIFY | PERM_COPY);
		src->setPermissions(new_perm);

		src->setInventoryType(LLInventoryType::IT_SOUND);
		src->setFlags(new_flags);
		src->setCreationDate(new_creation);

		// test a save/load cycle to LLSD and back again
		LLSD sd = ll_create_sd_from_inventory_item(src);
		LLPointer<LLInventoryItem> dst = new LLInventoryItem;
		bool successful_parse = dst->fromLLSD(sd);
		ensure_equals("0.LLInventoryItem::fromLLSD()", successful_parse, true);

		LLPointer<LLInventoryItem> src1 = create_random_inventory_item();
		src1->copyItem(src);
		
		ensure_equals("1.item id::getUUID() failed", dst->getUUID(), src1->getUUID());
		ensure_equals("2.parent::getParentUUID() failed", dst->getParentUUID(), src1->getParentUUID());
		ensure_equals("3.name::getName() failed", dst->getName(), src1->getName());
		ensure_equals("4.type::getType() failed", dst->getType(), src1->getType());
		
		ensure_equals("5.permissions::getPermissions() failed", dst->getPermissions(), src1->getPermissions());
		ensure_equals("6.description::getDescription() failed", dst->getDescription(), src1->getDescription());
		ensure_equals("7.sale type::getSaleType() failed type", dst->getSaleInfo().getSaleType(), src1->getSaleInfo().getSaleType());
		ensure_equals("8.sale price::getSalePrice() failed price", dst->getSaleInfo().getSalePrice(), src1->getSaleInfo().getSalePrice());
		ensure_equals("9.asset id::getAssetUUID() failed id", dst->getAssetUUID(), src1->getAssetUUID());
		ensure_equals("10.inventory type::getInventoryType() failed type", dst->getInventoryType(), src1->getInventoryType());
		ensure_equals("11.flags::getFlags() failed", dst->getFlags(), src1->getFlags());
		ensure_equals("12.creation::getCreationDate() failed", dst->getCreationDate(), src1->getCreationDate());

		// quick test to make sure generateUUID() really works
		src1->generateUUID();	
		ensure_not_equals("13.item id::generateUUID() failed", src->getUUID(), src1->getUUID());
	}

	template<> template<>
	void inventory_object::test<7>()
	{
		LLFILE* fp = LLFile::fopen("linden_file.dat","w+");
		if(!fp)
		{
			llerrs << "file could not be opened\n" << llendl;
			return;
		}
			
		LLPointer<LLInventoryItem> src1 = create_random_inventory_item();
		src1->exportFile(fp, TRUE);
		fclose(fp);

		LLPointer<LLInventoryItem> src2 = new LLInventoryItem();	
		fp = LLFile::fopen("linden_file.dat","r+");
		if(!fp)
		{
			llerrs << "file could not be opened\n" << llendl;
			return;
		}
		
		src2->importFile(fp);
		fclose(fp);
		
		ensure_equals("1.item id::getUUID() failed", src1->getUUID(), src2->getUUID());
		ensure_equals("2.parent::getParentUUID() failed", src1->getParentUUID(), src2->getParentUUID());
		ensure_equals("3.permissions::getPermissions() failed", src1->getPermissions(), src2->getPermissions());
		ensure_equals("4.sale price::getSalePrice() failed price", src1->getSaleInfo().getSalePrice(), src2->getSaleInfo().getSalePrice());
		ensure_equals("5.asset id::getAssetUUID() failed id", src1->getAssetUUID(), src2->getAssetUUID());
		ensure_equals("6.type::getType() failed", src1->getType(), src2->getType());
		ensure_equals("7.inventory type::getInventoryType() failed type", src1->getInventoryType(), src2->getInventoryType());
		ensure_equals("8.name::getName() failed", src1->getName(), src2->getName());
		ensure_equals("9.description::getDescription() failed", src1->getDescription(), src2->getDescription());				
		ensure_equals("10.creation::getCreationDate() failed", src1->getCreationDate(), src2->getCreationDate());
			
	}

	template<> template<>
	void inventory_object::test<8>()
	{
			
		LLPointer<LLInventoryItem> src1 = create_random_inventory_item();

		std::ostringstream ostream;
		src1->exportLegacyStream(ostream, TRUE);
		
		std::istringstream istream(ostream.str());
		LLPointer<LLInventoryItem> src2 = new LLInventoryItem();
		src2->importLegacyStream(istream);
					
		ensure_equals("1.item id::getUUID() failed", src1->getUUID(), src2->getUUID());
		ensure_equals("2.parent::getParentUUID() failed", src1->getParentUUID(), src2->getParentUUID());
		ensure_equals("3.permissions::getPermissions() failed", src1->getPermissions(), src2->getPermissions());
		ensure_equals("4.sale price::getSalePrice() failed price", src1->getSaleInfo().getSalePrice(), src2->getSaleInfo().getSalePrice());
		ensure_equals("5.asset id::getAssetUUID() failed id", src1->getAssetUUID(), src2->getAssetUUID());
		ensure_equals("6.type::getType() failed", src1->getType(), src2->getType());
		ensure_equals("7.inventory type::getInventoryType() failed type", src1->getInventoryType(), src2->getInventoryType());
		ensure_equals("8.name::getName() failed", src1->getName(), src2->getName());
		ensure_equals("9.description::getDescription() failed", src1->getDescription(), src2->getDescription());				
		ensure_equals("10.creation::getCreationDate() failed", src1->getCreationDate(), src2->getCreationDate());
		
		
	}

	template<> template<>
	void inventory_object::test<9>()
	{
		// Deleted LLInventoryItem::exportFileXML() and LLInventoryItem::importXML()
		// because I can't find any non-test code references to it. 2009-05-04 JC
	}
		
	template<> template<>
	void inventory_object::test<10>()
	{
		LLPointer<LLInventoryItem> src1 = create_random_inventory_item();
		U8* bin_bucket = new U8[300];
		S32 bin_bucket_size = src1->packBinaryBucket(bin_bucket, NULL);

		LLPointer<LLInventoryItem> src2 = new LLInventoryItem();
		src2->unpackBinaryBucket(bin_bucket, bin_bucket_size);

		ensure_equals("1.sale price::getSalePrice() failed price", src1->getSaleInfo().getSalePrice(), src2->getSaleInfo().getSalePrice());
		ensure_equals("2.sale type::getSaleType() failed type", src1->getSaleInfo().getSaleType(), src2->getSaleInfo().getSaleType());
		ensure_equals("3.type::getType() failed", src1->getType(), src2->getType());
		ensure_equals("4.inventory type::getInventoryType() failed type", src1->getInventoryType(), src2->getInventoryType());
		ensure_equals("5.name::getName() failed", src1->getName(), src2->getName());
		ensure_equals("6.description::getDescription() failed", src1->getDescription(), src2->getDescription());				
		ensure_equals("7.flags::getFlags() failed", src1->getFlags(), src2->getFlags());
	
	}
	
	template<> template<>
	void inventory_object::test<11>()
	{
		LLPointer<LLInventoryItem> src1 = create_random_inventory_item();
		LLSD retSd = src1->asLLSD();
		LLPointer<LLInventoryItem> src2 = new LLInventoryItem();
		src2->fromLLSD(retSd);

		ensure_equals("1.item id::getUUID() failed", src1->getUUID(), src2->getUUID());
		ensure_equals("2.parent::getParentUUID() failed", src1->getParentUUID(), src2->getParentUUID());
		ensure_equals("3.permissions::getPermissions() failed", src1->getPermissions(), src2->getPermissions());
		ensure_equals("4.asset id::getAssetUUID() failed id", src1->getAssetUUID(), src2->getAssetUUID());
		ensure_equals("5.type::getType() failed", src1->getType(), src2->getType());
		ensure_equals("6.inventory type::getInventoryType() failed type", src1->getInventoryType(), src2->getInventoryType());
		ensure_equals("7.flags::getFlags() failed", src1->getFlags(), src2->getFlags());
		ensure_equals("8.sale type::getSaleType() failed type", src1->getSaleInfo().getSaleType(), src2->getSaleInfo().getSaleType());
		ensure_equals("9.sale price::getSalePrice() failed price", src1->getSaleInfo().getSalePrice(), src2->getSaleInfo().getSalePrice());
		ensure_equals("10.name::getName() failed", src1->getName(), src2->getName());
		ensure_equals("11.description::getDescription() failed", src1->getDescription(), src2->getDescription());				
		ensure_equals("12.creation::getCreationDate() failed", src1->getCreationDate(), src2->getCreationDate());
	}	

//******class LLInventoryCategory*******//

	template<> template<>
	void inventory_object::test<12>()
	{
		LLPointer<LLInventoryCategory> src = create_random_inventory_cat();
		LLSD sd = ll_create_sd_from_inventory_category(src);
		LLPointer<LLInventoryCategory> dst = ll_create_category_from_sd(sd);
		
		ensure_equals("1.item id::getUUID() failed", dst->getUUID(), src->getUUID());
		ensure_equals("2.parent::getParentUUID() failed", dst->getParentUUID(), src->getParentUUID());
		ensure_equals("3.name::getName() failed", dst->getName(), src->getName());
		ensure_equals("4.type::getType() failed", dst->getType(), src->getType());
		ensure_equals("5.preferred type::getPreferredType() failed", dst->getPreferredType(), src->getPreferredType());

		src->setPreferredType( LLFolderType::FT_TEXTURE);
		sd = ll_create_sd_from_inventory_category(src);
		dst = ll_create_category_from_sd(sd);
		ensure_equals("6.preferred type::getPreferredType() failed", dst->getPreferredType(), src->getPreferredType());

		
	}

	template<> template<>
	void inventory_object::test<13>()
	{
		LLFILE* fp = LLFile::fopen("linden_file.dat","w");
		if(!fp)
		{
			llerrs << "file coudnt be opened\n" << llendl;
			return;
		}
			
		LLPointer<LLInventoryCategory> src1 = create_random_inventory_cat();
		src1->exportFile(fp, TRUE);
		fclose(fp);

		LLPointer<LLInventoryCategory> src2 = new LLInventoryCategory();	
		fp = LLFile::fopen("linden_file.dat","r");
		if(!fp)
		{
			llerrs << "file coudnt be opened\n" << llendl;
			return;
		}
		
		src2->importFile(fp);
		fclose(fp);

		ensure_equals("1.item id::getUUID() failed", src1->getUUID(), src2->getUUID());
		ensure_equals("2.parent::getParentUUID() failed", src1->getParentUUID(), src2->getParentUUID());
 		ensure_equals("3.type::getType() failed", src1->getType(), src2->getType());
		ensure_equals("4.preferred type::getPreferredType() failed", src1->getPreferredType(), src2->getPreferredType());
		ensure_equals("5.name::getName() failed", src1->getName(), src2->getName());
	}

	template<> template<>
	void inventory_object::test<14>()
	{
		LLPointer<LLInventoryCategory> src1 = create_random_inventory_cat();

		std::ostringstream ostream;
		src1->exportLegacyStream(ostream, TRUE);

		std::istringstream istream(ostream.str());
		LLPointer<LLInventoryCategory> src2 = new LLInventoryCategory();
		src2->importLegacyStream(istream);
					
		ensure_equals("1.item id::getUUID() failed", src1->getUUID(), src2->getUUID());
		ensure_equals("2.parent::getParentUUID() failed", src1->getParentUUID(), src2->getParentUUID());
		ensure_equals("3.type::getType() failed", src1->getType(), src2->getType());
		ensure_equals("4.preferred type::getPreferredType() failed", src1->getPreferredType(), src2->getPreferredType());
		ensure_equals("5.name::getName() failed", src1->getName(), src2->getName());
			
	}
}
