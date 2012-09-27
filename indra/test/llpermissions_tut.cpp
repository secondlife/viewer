/**
 * @file llpermissions_tut.cpp
 * @author Adroit
 * @date March 2007
 * @brief llpermissions test cases.
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
#include "message.h"
#include "llpermissions.h"


namespace tut
{
	struct permission
	{
	};
	typedef test_group<permission> permission_t;
	typedef permission_t::object permission_object_t;
	tut::permission_t tut_permission("permission");

	template<> template<>
	void permission_object_t::test<1>()
	{
		LLPermissions permissions;
		LLUUID	uuid = permissions.getCreator();
		LLUUID	uuid1 = permissions.getOwner(); 
		LLUUID	uuid2 = permissions.getGroup();
		LLUUID	uuid3 = permissions.getLastOwner(); 

		ensure("LLPermission Get Functions failed", (uuid == LLUUID::null && uuid1 == LLUUID::null && 
			uuid2 == LLUUID::null && uuid3 == LLUUID::null));
		ensure("LLPermission Get Functions failed", (permissions.getMaskBase() == PERM_ALL && permissions.getMaskOwner() == PERM_ALL && 
			permissions.getMaskGroup() == PERM_ALL && permissions.getMaskEveryone() == PERM_ALL && permissions.getMaskNextOwner() == PERM_ALL));
		ensure("Ownership functions failed", ((! permissions.isGroupOwned()) && (! permissions.isOwned())));
	}

	template<> template<>
	void permission_object_t::test<2>()
	{
		LLPermissions permissions;
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner("68edcf47-ccd7-45b8-9f90-1649d7f12806"); 
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		permissions.init(creator, owner, lastOwner, group);

		ensure_equals("init/getCreator():failed to return the creator ", creator, permissions.getCreator());	
		ensure_equals("init/getOwner():failed to return the owner ", owner, permissions.getOwner());
		ensure_equals("init/getLastOwner():failed to return the group ", lastOwner, permissions.getLastOwner());	
		ensure_equals("init/getGroup():failed to return the group ", group, permissions.getGroup());	
	}

	template<> template<>
	void permission_object_t::test<3>()
	{
		LLPermissions permissions;
		U32 base = PERM_ALL;
		U32 owner = PERM_ITEM_UNRESTRICTED; //PERM_ITEM_UNRESTRICTED = PERM_MODIFY | PERM_COPY | PERM_TRANSFER;
		U32 group = PERM_TRANSFER | PERM_MOVE | PERM_COPY|PERM_MODIFY;
		U32 everyone = PERM_TRANSFER | PERM_MOVE | PERM_MODIFY;
		U32 next = PERM_NONE;

		U32 fixedbase = base;
		U32 fixedowner = PERM_ITEM_UNRESTRICTED; //owner & fixedbase
		U32 fixedgroup = PERM_ITEM_UNRESTRICTED; // no PERM_MOVE as owner does not have that perm either
		U32 fixedeveryone = PERM_TRANSFER; // no PERM_MOVE. Everyone can never modify.
		U32 fixednext = PERM_NONE;

		permissions.initMasks(base, owner, everyone, group, next); // will fix perms if not allowed.
		ensure_equals("initMasks/getMaskBase():failed to return the MaskBase ", fixedbase, permissions.getMaskBase());	
		ensure_equals("initMasks/getMaskOwner():failed to return the MaskOwner ", fixedowner, permissions.getMaskOwner());	
		ensure_equals("initMasks/getMaskEveryone():failed to return the MaskGroup ", fixedgroup, permissions.getMaskGroup());	
		ensure_equals("initMasks/getMaskEveryone():failed to return the MaskEveryone ", fixedeveryone, permissions.getMaskEveryone());	
		ensure_equals("initMasks/getMaskNextOwner():failed to return the MaskNext ", fixednext, permissions.getMaskNextOwner());	

		// explictly set should maintain the values
		permissions.setMaskBase(base); //no fixing
		ensure_equals("setMaskBase/getMaskBase():failed to return the MaskBase ", base, permissions.getMaskBase());	

		permissions.setMaskOwner(owner);
		ensure_equals("setMaskOwner/getMaskOwner():failed to return the MaskOwner ", owner, permissions.getMaskOwner());	
		
		permissions.setMaskEveryone(everyone);
		ensure_equals("setMaskEveryone/getMaskEveryone():failed to return the MaskEveryone ", everyone, permissions.getMaskEveryone());	

		permissions.setMaskGroup(group);
		ensure_equals("setMaskGroup/getMaskEveryone():failed to return the MaskGroup ", group, permissions.getMaskGroup());	

		permissions.setMaskNext(next);
		ensure_equals("setMaskNext/getMaskNextOwner():failed to return the MaskNext ", next, permissions.getMaskNextOwner());	

		// further tests can be added to ensure perms for owner/group/everyone etc. get properly fixed. 
		// code however suggests that there is no explict check if the perms are correct and the user of this 
		// class is expected to know how to use them correctly. skipping further test cases for now for various
		// perm combinations.
	}

	template<> template<>
	void permission_object_t::test<4>()
	{
		LLPermissions perm,perm1;
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner("68edcf47-ccd7-45b8-9f90-1649d7f12806"); 
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		perm1.init(creator,owner,lastOwner,group);
		perm.set(perm1);
		ensure("set():failed to set ", (creator == perm.getCreator()) && (owner == perm.getOwner())&&
									(lastOwner == perm.getLastOwner())&& (group == perm.getGroup()));	
		}

	template<> template<>
	void permission_object_t::test<5>()
	{
		LLPermissions perm,perm1;
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner("68edcf47-ccd7-45b8-9f90-1649d7f12806"); 
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		perm1.init(creator,owner,lastOwner,group);

		U32 base = PERM_TRANSFER;
		U32 ownerp = PERM_TRANSFER;
		U32 groupp = PERM_TRANSFER;
		U32 everyone = PERM_TRANSFER;
		U32 next = PERM_NONE;

		perm1.initMasks(base, ownerp, everyone, groupp, next);

		base = PERM_ALL;
		ownerp = PERM_ITEM_UNRESTRICTED; //PERM_ITEM_UNRESTRICTED = PERM_MODIFY | PERM_COPY | PERM_TRANSFER;
		groupp = PERM_TRANSFER | PERM_COPY|PERM_MODIFY;
		everyone = PERM_TRANSFER;
		next = PERM_NONE;
		
		perm.init(creator,owner,lastOwner,group);
		perm.initMasks(base, ownerp, everyone, groupp, next); 

		// restrict permissions by accumulation
		perm.accumulate(perm1);

		U32 fixedbase = PERM_TRANSFER | PERM_MOVE;
		U32 fixedowner = PERM_TRANSFER;
		U32 fixedgroup = PERM_TRANSFER;
		U32 fixedeveryone = PERM_TRANSFER;
		U32 fixednext = PERM_NONE;

		ensure_equals("accumulate failed ", fixedbase, perm.getMaskBase());	
		ensure_equals("accumulate failed ", fixedowner, perm.getMaskOwner());	
		ensure_equals("accumulate failed ", fixedgroup, perm.getMaskGroup());	
		ensure_equals("accumulate failed ", fixedeveryone, perm.getMaskEveryone());	
		ensure_equals("accumulate failed ", fixednext, perm.getMaskNextOwner());	
	}

	template<> template<>
	void permission_object_t::test<6>()
	{
		LLPermissions perm;
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner("68edcf47-ccd7-45b8-9f90-1649d7f12806"); 
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		perm.init(creator,owner,lastOwner,group);
		ensure_equals("getSafeOwner:failed ", owner,perm.getSafeOwner());
		
		///NULL Owner
		perm.init(creator,LLUUID::null,lastOwner,group);
		ensure_equals("getSafeOwner:failed ", group, perm.getSafeOwner());
	}
	
		template<> template<>
	void permission_object_t::test<7>()
	{
		LLPermissions perm1;
		LLUUID uuid;
		BOOL is_group_owned = FALSE;
		ensure("1:getOwnership:failed ", ! perm1.getOwnership(uuid,is_group_owned));
		
		LLPermissions perm;
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner("68edcf47-ccd7-45b8-9f90-1649d7f12806"); 
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		perm.init(creator,owner,lastOwner,group);
		perm.getOwnership(uuid,is_group_owned);
		ensure("2:getOwnership:failed ", ((uuid == owner) && (! is_group_owned))); 

		perm.init(creator,LLUUID::null,lastOwner,group);
		perm.getOwnership(uuid,is_group_owned);
		ensure("3:getOwnership:failed ", ((uuid == group) && is_group_owned));
	}

	template<> template<>
	void permission_object_t::test<8>()
	{
		LLPermissions perm,perm1;
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner("68edcf47-ccd7-45b8-9f90-1649d7f12806"); 
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		perm.init(creator,owner,lastOwner,group);
		perm1.init(creator,owner,lastOwner,group);
		ensure_equals("getCRC32:failed ", perm.getCRC32(),perm1.getCRC32());
	}

	template<> template<>
	void permission_object_t::test<9>()
	{
		LLPermissions perm;
		LLUUID agent("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner("68edcf47-ccd7-45b8-9f90-1649d7f12806"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");
		bool is_atomic = true;
		ensure("setOwnerAndGroup():failed ", perm.setOwnerAndGroup(agent,owner,group,is_atomic));
		
		LLUUID owner2("68edcf47-ccd7-45b8-9f90-1649d7f12807"); 
		LLUUID group2("9c8eca51-53d5-42a7-bb58-cef070395db9");
		
		// cant change - agent need to be current owner
		ensure("setOwnerAndGroup():failed ", ! perm.setOwnerAndGroup(agent,owner2,group2,is_atomic));
		
		// should be able to change - agent and owner same as current owner
		ensure("setOwnerAndGroup():failed ", perm.setOwnerAndGroup(owner,owner,group2,is_atomic));
	}

	template<> template<>
	void permission_object_t::test<10>()
	{
		LLPermissions perm;
		LLUUID agent;
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");
		ensure("deedToGroup():failed ", perm.deedToGroup(agent,group));
	}
	template<> template<>
	void permission_object_t::test<11>()
	{
		LLPermissions perm;
		LLUUID agent;
		BOOL set = 1;
		U32 bits = PERM_TRANSFER | PERM_MODIFY;
		ensure("setBaseBits():failed ", perm.setBaseBits(agent, set, bits));
		ensure("setOwnerBits():failed ", perm.setOwnerBits(agent, set, bits));

		LLUUID agent1("9c8eca51-53d5-42a7-bb58-cef070395db8");
		ensure("setBaseBits():failed ", ! perm.setBaseBits(agent1, set, bits));
		ensure("setOwnerBits():failed ", ! perm.setOwnerBits(agent1, set, bits));
	}

	template<> template<>
	void permission_object_t::test<12>()
	{
		LLPermissions perm;
		LLUUID agent;
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		BOOL set = 1;
		U32 bits = 10;
		ensure("setGroupBits():failed ", perm.setGroupBits(agent,group, set, bits));
		ensure("setEveryoneBits():failed ", perm.setEveryoneBits(agent,group, set, bits));
		ensure("setNextOwnerBits():failed ", perm.setNextOwnerBits(agent,group, set, bits));

		LLUUID agent1("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");
		ensure("setGroupBits():failed ", ! perm.setGroupBits(agent1,group, set, bits));
		ensure("setEveryoneBits():failed ", ! perm.setEveryoneBits(agent1,group, set, bits));
		ensure("setNextOwnerBits():failed ", ! perm.setNextOwnerBits(agent1,group, set, bits));
	}

	template<> template<>
	void permission_object_t::test<13>()
	{
		LLPermissions perm;
		LLUUID agent;
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		U32 bits = 10;
		ensure("allowOperationBy():failed ", perm.allowOperationBy(bits,agent,group));

		LLUUID agent1("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner("68edcf47-ccd7-45b8-9f90-1649d7f12806"); 
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		perm.init(creator,owner,lastOwner,group);
		ensure("allowOperationBy():failed ", perm.allowOperationBy(bits,agent1,group));
	}

	template<> template<>
	void permission_object_t::test<14>()
	{
		LLPermissions perm;
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner;
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		perm.init(creator,owner,lastOwner,group);
		LLUUID agent;
		ensure("1:allowModifyBy():failed ", perm.allowModifyBy(agent));
		ensure("2:allowModifyBy():failed ", perm.allowModifyBy(agent,group));
				
		LLUUID agent1("9c8eca51-53d5-42a7-bb58-cef070395db8"); 
		ensure("3:allowModifyBy():failed ", perm.allowModifyBy(agent1));
		ensure("4:allowModifyBy():failed ", perm.allowModifyBy(agent1,group));
	}
	
	template<> template<>
	void permission_object_t::test<15>()
	{
		LLPermissions perm;
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner;
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		perm.init(creator,owner,lastOwner,group);
		LLUUID agent;
		ensure("1:allowCopyBy():failed ", perm.allowModifyBy(agent));
		ensure("2:allowCopyBy():failed ", perm.allowModifyBy(agent,group));
				
		LLUUID agent1("9c8eca51-53d5-42a7-bb58-cef070395db8"); 
		ensure("3:allowCopyBy():failed ", perm.allowCopyBy(agent1));
		ensure("4:allowCopyBy():failed ", perm.allowCopyBy(agent1,group));
	}

	template<> template<>
	void permission_object_t::test<16>()
	{
		LLPermissions perm;
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner;
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		perm.init(creator,owner,lastOwner,group);
		LLUUID agent;
		ensure("1:allowMoveBy():failed ", perm.allowMoveBy(agent));
		ensure("2:allowMoveBy():failed ", perm.allowMoveBy(agent,group));

		LLUUID agent1("9c8eca51-53d5-42a7-bb58-cef070395db8"); 
		ensure("3:allowMoveBy():failed ", perm.allowMoveBy(agent1));
		ensure("4:allowMoveBy():failed ", perm.allowMoveBy(agent1,group));
	}

	template<> template<>
	void permission_object_t::test<17>()
	{
		LLPermissions perm;
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner;
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		LLUUID agent;
		ensure("1:allowMoveBy():failed ", perm.allowTransferTo(agent));
		
		perm.init(creator,owner,lastOwner,group);
		ensure("2:allowMoveBy():failed ", perm.allowTransferTo(agent));
	}

	template<> template<>
	void permission_object_t::test<18>()
	{
		LLPermissions perm,perm1;
		ensure_equals("1:Operator==:failed ", perm, perm1);
		
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner("68edcf47-ccd7-45b8-9f90-1649d7f12806"); 
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		perm.init(creator,owner,lastOwner,group);
		perm = perm1;
		ensure_equals("2:Operator==:failed ", perm, perm1);
	}

	template<> template<>
	void permission_object_t::test<19>()
	{
		LLPermissions perm,perm1;
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner("68edcf47-ccd7-45b8-9f90-1649d7f12806"); 
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		perm.init(creator,owner,lastOwner,group);
		ensure_not_equals("2:Operator==:failed ", perm, perm1);
	}

	template<> template<>
	void permission_object_t::test<20>()
	{
		LLFILE* fp = LLFile::fopen("linden_file.dat","w+");
		if(!fp)
		{
			llerrs << "file couldn't be opened\n" << llendl;
			return;
		}
		LLPermissions perm,perm1;
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner("68edcf47-ccd7-45b8-9f90-1649d7f12806"); 
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		perm.init(creator,owner,lastOwner,group);
		
		U32 base = PERM_TRANSFER | PERM_COPY;
		U32 ownerp = PERM_TRANSFER;
		U32 groupp = PERM_TRANSFER;
		U32 everyone = PERM_TRANSFER;
		U32 next = PERM_NONE;

		perm.initMasks(base, ownerp, everyone, groupp, next);

		ensure("Permissions export failed", perm.exportFile(fp));
		fclose(fp);	
		fp = LLFile::fopen("linden_file.dat","r+");
		if(!fp)
		{
			llerrs << "file couldn't be opened\n" << llendl;
			return;
		}
		ensure("Permissions import failed", perm1.importFile(fp));
		fclose(fp);
		ensure_equals("exportFile()/importFile():failed to export and import the data ", perm1, perm);	
}

	template<> template<>
	void permission_object_t::test<21>()
	{
		LLPermissions perm,perm1;
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner("68edcf47-ccd7-45b8-9f90-1649d7f12806"); 
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		perm.init(creator,owner,lastOwner,group);

		U32 base = PERM_TRANSFER | PERM_COPY;
		U32 ownerp = PERM_TRANSFER;
		U32 groupp = PERM_TRANSFER;
		U32 everyone = PERM_TRANSFER;
		U32 next = PERM_NONE;

		perm.initMasks(base, ownerp, everyone, groupp, next);

		std::ostringstream ostream;
		perm.exportStream(ostream);
		std::istringstream istream(ostream.str());
		perm1.importStream(istream);

		ensure_equals("exportStream()/importStream():failed to export and import the data ", perm1, perm);
	}

	template<> template<>
	void permission_object_t::test<22>()
	{
		// Deleted LLPermissions::exportFileXML() and LLPermissions::importXML()
		// because I can't find any non-test code references to it. 2009-05-04 JC
	}

	template<> template<>
	void permission_object_t::test<23>()
	{
		LLPermissions perm,perm1;
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner("68edcf47-ccd7-45b8-9f90-1649d7f12806"); 
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		perm.init(creator,owner,lastOwner,group);
		std::ostringstream stream1, stream2;
		stream1 << perm;
		perm1.init(creator,owner,lastOwner,group);
		stream2 << perm1;
		ensure_equals("1:operator << failed", stream1.str(), stream2.str());
	}

	template<> template<>
	void permission_object_t::test<24>()
	{
		LLPermissions perm,perm1;
		LLUUID creator("abf0d56b-82e5-47a2-a8ad-74741bb2c29e");	
		LLUUID owner("68edcf47-ccd7-45b8-9f90-1649d7f12806"); 
		LLUUID lastOwner("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); 
		LLUUID group("9c8eca51-53d5-42a7-bb58-cef070395db8");		
		perm.init(creator,owner,lastOwner,group);
		
		U32 base = PERM_TRANSFER | PERM_COPY;
		U32 ownerp = PERM_TRANSFER;
		U32 groupp = PERM_TRANSFER;
		U32 everyone = PERM_TRANSFER;
		U32 next = PERM_NONE;

		perm.initMasks(base, ownerp, everyone, groupp, next);

		LLSD sd = ll_create_sd_from_permissions(perm);
		perm1 = ll_permissions_from_sd(sd);
		ensure_equals("ll_permissions_from_sd() and ll_create_sd_from_permissions()functions failed", perm, perm1);
	}

	template<> template<>
	void permission_object_t::test<25>()
	{
		LLAggregatePermissions AggrPermission;	
		LLAggregatePermissions AggrPermission1;	
		ensure_equals("getU8() function failed", AggrPermission.getU8(), 0);
		ensure("isEmpty() function failed", AggrPermission.isEmpty());
		AggrPermission.getValue(PERM_TRANSFER);
		ensure_equals("getValue() function failed", AggrPermission.getValue(PERM_TRANSFER), 0x00);

		AggrPermission.aggregate(PERM_ITEM_UNRESTRICTED);
		ensure("aggregate() function failed", ! AggrPermission.isEmpty());

		AggrPermission1.aggregate(AggrPermission);
		ensure("aggregate() function failed", ! AggrPermission1.isEmpty());

		std::ostringstream stream1;
		stream1 << AggrPermission;
		ensure_equals("operator<< failed", stream1.str(), "{PI_COPY=All, PI_MODIFY=All, PI_TRANSFER=All}");
	}
}
