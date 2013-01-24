/** 
 * @file LLSaleInfo_tut.cpp
 * @author Adroit
 * @date 2007-03
 * @brief Test cases of llsaleinfo.h
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include <tut/tut.hpp>
#include "linden_common.h"
#include "lltut.h"
#include "llsaleinfo.h"

namespace tut
{
	struct llsaleinfo_tut
	{
	};
	typedef test_group<llsaleinfo_tut> llsaleinfo_tut_t;
	typedef llsaleinfo_tut_t::object llsaleinfo_test_t;
	tut::llsaleinfo_tut_t tut_llsaleinfo_test("llsaleinfo");

	template<> template<>
	void llsaleinfo_test_t::test<1>()
	{
		//test case for getSaleType(), getSalePrice(), getCRC32() fn.
		//test case for setSaleType(), setSalePrice() fn.

		S32 sale_price = 10000;
		LLSaleInfo llsaleinfo(LLSaleInfo::FS_COPY, sale_price);
		const char* sale= "copy";

		LLSD llsd_obj1 = ll_create_sd_from_sale_info(llsaleinfo);
		LLSaleInfo saleinfo1 = ll_sale_info_from_sd(llsd_obj1);
		
		ensure("1. The getSaleType() fn failed", LLSaleInfo::FS_COPY == llsaleinfo.getSaleType());
		ensure("2. LLSaleInfo::isForSale() fn failed", TRUE == llsaleinfo.isForSale());
		ensure("3. The getSalePrice() fn failed", sale_price == llsaleinfo.getSalePrice());
		ensure("4. The getCRC32() fn failed", 235833404 == llsaleinfo.getCRC32());
		ensure("5. LLSaleInfo::lookup(const char* name) fn failed", LLSaleInfo::FS_COPY == llsaleinfo.lookup(sale));
		ensure_equals("6. ll_create_sd_from_sale_info() fn failed", llsaleinfo.getSalePrice(), saleinfo1.getSalePrice());
		ensure_equals("7. ll_create_sd_from_sale_info() fn failed", llsaleinfo.getSaleType(), saleinfo1.getSaleType());

		llsaleinfo.setSalePrice(10000000);
		llsaleinfo.setSaleType(LLSaleInfo::FS_ORIGINAL);
		sale = "cntn";
		llsd_obj1 = ll_create_sd_from_sale_info(llsaleinfo);
		saleinfo1 = ll_sale_info_from_sd(llsd_obj1);

		ensure("8. The getSaleType() and setSaleType() fn failed", LLSaleInfo::FS_ORIGINAL == llsaleinfo.getSaleType());
		ensure("9. LLSaleInfo::isForSale() fn failed", TRUE == llsaleinfo.isForSale());
		ensure("10. The getSalePrice() fn failed", 10000000 == llsaleinfo.getSalePrice());
		ensure("11. The getCRC32() fn failed", 127911702 == llsaleinfo.getCRC32());
		ensure("12. LLSaleInfo::lookup(const char* name) fn failed", LLSaleInfo::FS_CONTENTS == llsaleinfo.lookup(sale));
		ensure_equals("13. ll_create_sd_from_sale_info() fn failed", llsaleinfo.getSalePrice(), saleinfo1.getSalePrice());
		ensure_equals("14. ll_create_sd_from_sale_info() fn failed", llsaleinfo.getSaleType(), saleinfo1.getSaleType());

		llsaleinfo.setSalePrice(55000550);
		llsaleinfo.setSaleType(LLSaleInfo::FS_CONTENTS);
		sale = "orig";
		llsd_obj1 = ll_create_sd_from_sale_info(llsaleinfo);
		saleinfo1 = ll_sale_info_from_sd(llsd_obj1);

		ensure("15. The getSaleType() and setSaleType() fn failed", LLSaleInfo::FS_CONTENTS == llsaleinfo.getSaleType());
		ensure("16. LLSaleInfo::isForSale() fn failed", TRUE == llsaleinfo.isForSale());
		ensure("17. The getSalePrice() fn failed", 55000550 == llsaleinfo.getSalePrice());
		ensure("18. The getCRC32() fn failed", 408735656 == llsaleinfo.getCRC32());
		ensure("19. LLSaleInfo::lookup(const char* name) fn failed", LLSaleInfo::FS_ORIGINAL == llsaleinfo.lookup(sale));
		ensure_equals("20. ll_create_sd_from_sale_info() fn failed", llsaleinfo.getSalePrice(), saleinfo1.getSalePrice());
		ensure_equals("21. ll_create_sd_from_sale_info() fn failed", llsaleinfo.getSaleType(), saleinfo1.getSaleType());

		llsaleinfo.setSalePrice(-6432);
		llsaleinfo.setSaleType(LLSaleInfo::FS_NOT);
		sale = "not";
		llsd_obj1 = ll_create_sd_from_sale_info(llsaleinfo);
		saleinfo1 = ll_sale_info_from_sd(llsd_obj1);

		ensure("22. The getSaleType() and setSaleType() fn failed", LLSaleInfo::FS_NOT == llsaleinfo.getSaleType());
		ensure("23. LLSaleInfo::isForSale() fn failed", FALSE == llsaleinfo.isForSale());
		ensure("24. The getSalePrice() fn failed", 0 == llsaleinfo.getSalePrice());
		ensure("25. The getCRC32() fn failed", 0 == llsaleinfo.getCRC32());
		ensure("26. LLSaleInfo::lookup(const char* name) fn failed", LLSaleInfo::FS_NOT == llsaleinfo.lookup(sale));
		ensure_equals("27. ll_create_sd_from_sale_info() fn failed", llsaleinfo.getSalePrice(), saleinfo1.getSalePrice());
		ensure_equals("28. ll_create_sd_from_sale_info() fn failed", llsaleinfo.getSaleType(), saleinfo1.getSaleType());
	}

	template<> template<>
	void llsaleinfo_test_t::test<2>()
	{

		LLFILE* fp = LLFile::fopen("linden_file.dat","w+");
		if(!fp)
		{
			llerrs << "file could not be opened\n" << llendl;
			return;
		}
			
		S32 sale_price = 43500;
		LLSaleInfo llsaleinfo(LLSaleInfo::FS_COPY, sale_price);
		
		llsaleinfo.exportFile(fp);
		fclose(fp);

		LLSaleInfo llsaleinfo1;
		U32 perm_mask;
		BOOL has_perm_mask;
		fp = LLFile::fopen("linden_file.dat","r");
		
		if(!fp)
		{
			llerrs << "file coudnt be opened\n" << llendl;
			return;
		}
		
		llsaleinfo1.importFile(fp, has_perm_mask, perm_mask);
		fclose(fp);
		
		ensure("importFile() fn failed ", llsaleinfo.getSaleType() == llsaleinfo1.getSaleType() &&
								     llsaleinfo.getSalePrice() == llsaleinfo1.getSalePrice());				
	}

	template<> template<>
	void llsaleinfo_test_t::test<3>()
	{
		S32 sale_price = 525452;
		LLSaleInfo llsaleinfo(LLSaleInfo::FS_ORIGINAL, sale_price);
		
		std::ostringstream ostream;
		llsaleinfo.exportStream(ostream);
		
		std::istringstream istream(ostream.str());
		LLSaleInfo llsaleinfo1;
		U32 perm_mask = 0;
		BOOL has_perm_mask = FALSE;
		llsaleinfo1.importStream(istream, has_perm_mask, perm_mask);
					
		ensure("importStream() fn failed ",
			llsaleinfo.getSalePrice() == llsaleinfo1.getSalePrice() &&
			llsaleinfo.getSaleType() == llsaleinfo1.getSaleType());
	}

	template<> template<>
	void llsaleinfo_test_t::test<4>()
	{
		// Deleted LLSaleInfo::exportFileXML() and LLSaleInfo::importXML()
		// because I can't find any non-test code references to it. 2009-05-04 JC
	}

	template<> template<>
	void llsaleinfo_test_t::test<5>()
	{	
		S32 sale_price = 99000;
		LLSaleInfo saleinfo(LLSaleInfo::FS_ORIGINAL, sale_price);
		
		LLSD sd_result = saleinfo.asLLSD();
		
		U32 perm_mask = 0 ;
		BOOL has_perm_mask = FALSE;

		LLSaleInfo saleinfo1;
		saleinfo1.fromLLSD( sd_result, has_perm_mask, perm_mask);	

		ensure_equals("asLLSD and fromLLSD failed", saleinfo.getSalePrice(), saleinfo1.getSalePrice());
		ensure_equals("asLLSD and fromLLSD failed", saleinfo.getSaleType(), saleinfo1.getSaleType());
	}

	//static EForSale lookup(const char* name) fn test
	template<> template<>
	void llsaleinfo_test_t::test<6>()
	{
		S32 sale_price = 233223;
		LLSaleInfo::EForSale ret_type = LLSaleInfo::lookup("orig");
		
		ensure_equals("lookup(const char* name) fn failed", ret_type, LLSaleInfo::FS_ORIGINAL);

		LLSaleInfo saleinfo(LLSaleInfo::FS_COPY, sale_price);
		const char* result = LLSaleInfo::lookup(LLSaleInfo::FS_COPY);
		ensure("char* lookup(EForSale type) fn failed", 0 == strcmp("copy", result));
	}

	//void LLSaleInfo::accumulate(const LLSaleInfo& sale_info) fn test
	template<> template<>
	void llsaleinfo_test_t::test<7>()
	{
		S32 sale_price = 20;
		LLSaleInfo saleinfo(LLSaleInfo::FS_COPY, sale_price);
		LLSaleInfo saleinfo1(LLSaleInfo::FS_COPY, sale_price);
		saleinfo1.accumulate(saleinfo);
		ensure_equals("LLSaleInfo::accumulate(const LLSaleInfo& sale_info) fn failed", saleinfo1.getSalePrice(), 40);
				
	}

	// test cases of bool operator==(const LLSaleInfo &rhs) fn
	// test case of bool operator!=(const LLSaleInfo &rhs) fn
	template<> template<>
	void llsaleinfo_test_t::test<8>()
	{
		S32 sale_price = 55000;
		LLSaleInfo saleinfo(LLSaleInfo::FS_ORIGINAL, sale_price);
		LLSaleInfo saleinfoequal(LLSaleInfo::FS_ORIGINAL, sale_price);
		LLSaleInfo saleinfonotequal(LLSaleInfo::FS_ORIGINAL, sale_price*2);
		
		ensure("operator == fn. failed", true == (saleinfo == saleinfoequal));
		ensure("operator != fn. failed", true == (saleinfo != saleinfonotequal));
	}			

	template<> template<>
	void llsaleinfo_test_t::test<9>()
	{

		//TBD: void LLSaleInfo::packMessage(LLMessageSystem* msg) const
		//TBD: void LLSaleInfo::unpackMessage(LLMessageSystem* msg, const char* block)
		//TBD: void LLSaleInfo::unpackMultiMessage(LLMessageSystem* msg, const char* block, S32 block_num)
	}

}
