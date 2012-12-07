/** 
 * @file lluuidhashmap_tut.cpp
 * @author Adroit
 * @date 2007-02
 * @brief Test cases for LLUUIDHashMap
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
#include "lluuidhashmap.h"
#include "llsdserialize.h"
#include "lldir.h"
#include "stringize.h"
#include <iostream>
#include <fstream>

namespace tut
{
	class UUIDTableEntry
	{
	public:
		UUIDTableEntry()
		{
			mID.setNull();
			mValue = 0;
		}
		
		UUIDTableEntry(const LLUUID& id, U32 value)
		{
			mID = id;
			mValue = value;
		}

		~UUIDTableEntry(){};

		static BOOL uuidEq(const LLUUID &uuid, const UUIDTableEntry &id_pair)
		{
			if (uuid == id_pair.mID)
			{
				return TRUE;
			}
			return FALSE;
		}

		const LLUUID& getID() { return mID; }
		const U32& getValue() { return mValue; }

	protected:
		LLUUID	mID;
		U32  mValue;
	};

	struct hashmap_test
	{
	};

	typedef test_group<hashmap_test> hash_index_t;
	typedef hash_index_t::object hash_index_object_t;
	tut::hash_index_t tut_hash_index("hashmap_test");

	// stress test
	template<> template<>
	void hash_index_object_t::test<1>()
	{
		set_test_name("stress test");
		// As of 2012-10-10, I (nat) have observed sporadic failures of this
		// test: "set/get did not work." The trouble is that since test data
		// are randomly generated with every run, it is impossible to debug a
		// test failure. One is left with the uneasy suspicion that
		// LLUUID::generate() can sometimes produce duplicates even within the
		// moderately small number requested here. Since rerunning the test
		// generally allows it to pass, it's too easy to shrug and forget it.
		// The following code is intended to support reproducing such test
		// failures. The idea is that, on test failure, we save the generated
		// data to a canonical filename in a temp directory. Then on every
		// subsequent run, we check for that filename. If it exists, we reload
		// that specific data rather than generating fresh data -- which
		// should presumably reproduce the same test failure. But we inform
		// the user that to resume normal (random) test runs, s/he need only
		// delete that file. And since it's in a temp directory, sooner or
		// later the system will clean it up anyway.
		const char* tempvar = "TEMP";
		const char* tempdir = getenv(tempvar); // Windows convention
		if (! tempdir)
		{
			tempvar = "TMPDIR";
			tempdir = getenv(tempvar); // Mac convention
		}
		if (! tempdir)
		{
			// reset tempvar to the first var we check; it's just a
			// recommendation
			tempvar = "TEMP";
			tempdir = "/tmp";		// Posix in general
		}
		std::string savefile(gDirUtilp->add(tempdir, "lluuidhashmap_tut.save.txt"));
		const int numElementsToCheck = 32*256*32;
		std::vector<LLUUID> idList;
		if (gDirUtilp->fileExists(savefile))
		{
			// We have saved data from a previous failed run. Reload that data.
			std::ifstream inf(savefile.c_str());
			if (! inf.is_open())
			{
				fail(STRINGIZE("Although save file '" << savefile << "' exists, it cannot be opened"));
			}
			std::string item;
			while (std::getline(inf, item))
			{
				idList.push_back(LLUUID(item));
			}
			std::cout << "Reloaded " << idList.size() << " items from '" << savefile << "'";
			if (idList.size() != numElementsToCheck)
			{
				std::cout << " (expected " << numElementsToCheck << ")";
			}
			std::cout << " -- delete this file to generate new data" << std::endl;
		}
		else
		{
			// savefile does not exist (normal case): regenerate idList from
			// scratch.
			for (int i = 0; i < numElementsToCheck; ++i)
			{
				LLUUID id;
				id.generate();
				idList.push_back(id);
			}
		}

		LLUUIDHashMap<UUIDTableEntry, 32>	hashTable(UUIDTableEntry::uuidEq, UUIDTableEntry());
		int i;
		
		for (i = 0; i < idList.size(); ++i)
		{
			UUIDTableEntry entry(idList[i], i);
			hashTable.set(idList[i], entry);
		}

		try
		{
			for (i = 0; i < idList.size(); i++)
			{
				LLUUID idToCheck = idList[i];
				UUIDTableEntry entryToCheck = hashTable.get(idToCheck);
				ensure_equals(STRINGIZE("set/get ID (entry " << i << ")").c_str(),
							  entryToCheck.getID(), idToCheck);
				ensure_equals(STRINGIZE("set/get value (ID " << idToCheck << ")").c_str(),
							  entryToCheck.getValue(), (size_t)i);
			}

			for (i = 0; i < idList.size(); i++)
			{
				LLUUID idToCheck = idList[i];
				if (i % 2 != 0)
				{
					hashTable.remove(idToCheck);
				}
			}

			for (i = 0; i < idList.size(); i++)
			{
				LLUUID idToCheck = idList[i];
				ensure("remove or check did not work", (i % 2 == 0 && hashTable.check(idToCheck)) || (i % 2 != 0 && !hashTable.check(idToCheck)));
			}
		}
		catch (const failure&)
		{
			// One of the above tests failed. Try to save idList to repro with
			// a later run.
			std::ofstream outf(savefile.c_str());
			if (! outf.is_open())
			{
				// Sigh, don't use fail() here because we want to preserve
				// the original test failure.
				std::cout << "Cannot open file '" << savefile
						  << "' to save data -- check and fix " << tempvar << std::endl;
			}
			else
			{
				// outf.is_open()
				for (int i = 0; i < idList.size(); ++i)
				{
					outf << idList[i] << std::endl;
				}
				std::cout << "Saved " << idList.size() << " entries to '" << savefile
						  << "' -- rerun test to debug with these" << std::endl;
			}
			// re-raise the same exception -- we WANT this test failure to
			// be reported! We just needed to save the data on the way out.
			throw;
		}
	}

	// test removing all but one element. 
	template<> template<>
	void hash_index_object_t::test<2>()
	{
		LLUUIDHashMap<UUIDTableEntry, 2>	hashTable(UUIDTableEntry::uuidEq, UUIDTableEntry());
		const int numElementsToCheck = 5;
		std::vector<LLUUID> idList(numElementsToCheck*10);
		int i;
		
		for (i = 0; i < numElementsToCheck; i++)
		{
			LLUUID id;
			id.generate();
			UUIDTableEntry entry(id, i);
			hashTable.set(id, entry);
			idList[i] = id;
		}

		ensure("getLength failed", hashTable.getLength() == numElementsToCheck);

		// remove all but the last element
		for (i = 0; i < numElementsToCheck-1; i++)
		{
			LLUUID idToCheck = idList[i];
			hashTable.remove(idToCheck);
		}

		// there should only be one element left now.
		ensure("getLength failed", hashTable.getLength() == 1);

		for (i = 0; i < numElementsToCheck; i++)
		{
			LLUUID idToCheck = idList[i];
			if (i != numElementsToCheck - 1)
			{
				ensure("remove did not work", hashTable.check(idToCheck)  == FALSE);
			}
			else
			{
				UUIDTableEntry entryToCheck = hashTable.get(idToCheck);
				ensure("remove did not work", entryToCheck.getID() == idToCheck && entryToCheck.getValue() == (size_t)i);
			}
		}
	}

	// test overriding of value already set. 
	template<> template<>
	void hash_index_object_t::test<3>()
	{
		LLUUIDHashMap<UUIDTableEntry, 5>	hashTable(UUIDTableEntry::uuidEq, UUIDTableEntry());
		const int numElementsToCheck = 10;
		std::vector<LLUUID> idList(numElementsToCheck);
		int i;
		
		for (i = 0; i < numElementsToCheck; i++)
		{
			LLUUID id;
			id.generate();
			UUIDTableEntry entry(id, i);
			hashTable.set(id, entry);
			idList[i] = id;
		}

		for (i = 0; i < numElementsToCheck; i++)
		{
			LLUUID id = idList[i];
			// set new entry with value = i+numElementsToCheck
			UUIDTableEntry entry(id, i+numElementsToCheck);
			hashTable.set(id, entry);
		}

		for (i = 0; i < numElementsToCheck; i++)
		{
			LLUUID idToCheck = idList[i];
			UUIDTableEntry entryToCheck = hashTable.get(idToCheck);
			ensure("set/get did not work", entryToCheck.getID() == idToCheck && entryToCheck.getValue() == (size_t)(i+numElementsToCheck));
		}
	}

	// test removeAll() 
	template<> template<>
	void hash_index_object_t::test<4>()
	{
		LLUUIDHashMap<UUIDTableEntry, 5>	hashTable(UUIDTableEntry::uuidEq, UUIDTableEntry());
		const int numElementsToCheck = 10;
		std::vector<LLUUID> idList(numElementsToCheck);
		int i;
		
		for (i = 0; i < numElementsToCheck; i++)
		{
			LLUUID id;
			id.generate();
			UUIDTableEntry entry(id, i);
			hashTable.set(id, entry);
			idList[i] = id;
		}

		hashTable.removeAll();
		ensure("removeAll failed", hashTable.getLength() == 0);
	}


	// test sparse map - force it by creating 256 entries that fall into 256 different nodes 
	template<> template<>
	void hash_index_object_t::test<5>()
	{
		LLUUIDHashMap<UUIDTableEntry, 2>	hashTable(UUIDTableEntry::uuidEq, UUIDTableEntry());
		const int numElementsToCheck = 256;
		std::vector<LLUUID> idList(numElementsToCheck);
		int i;
		
		for (i = 0; i < numElementsToCheck; i++)
		{
			LLUUID id;
			id.generate();
			// LLUUIDHashMap uses mData[0] to pick the bucket
			// overwrite mData[0] so that it ranges from 0 to 255
			id.mData[0] = i; 
			UUIDTableEntry entry(id, i);
			hashTable.set(id, entry);
			idList[i] = id;
		}

		for (i = 0; i < numElementsToCheck; i++)
		{
			LLUUID idToCheck = idList[i];
			UUIDTableEntry entryToCheck = hashTable.get(idToCheck);
			ensure("set/get did not work for sparse map", entryToCheck.getID() == idToCheck && entryToCheck.getValue() == (size_t)i);
		}

		for (i = 0; i < numElementsToCheck; i++)
		{
			LLUUID idToCheck = idList[i];
			if (i % 2 != 0)
			{
				hashTable.remove(idToCheck);
			}
		}

		for (i = 0; i < numElementsToCheck; i++)
		{
			LLUUID idToCheck = idList[i];
			ensure("remove or check did not work for sparse map", (i % 2 == 0 && hashTable.check(idToCheck)) || (i % 2 != 0 && !hashTable.check(idToCheck)));
		}
	}

	// iterator
	template<> template<>
	void hash_index_object_t::test<6>()
	{
		LLUUIDHashMap<UUIDTableEntry, 2>	hashTable(UUIDTableEntry::uuidEq, UUIDTableEntry());
		LLUUIDHashMapIter<UUIDTableEntry, 2> hashIter(&hashTable);
		const int numElementsToCheck = 256;
		std::vector<LLUUID> idList(numElementsToCheck);
		int i;
		
		for (i = 0; i < numElementsToCheck; i++)
		{
			LLUUID id;
			id.generate();
			// LLUUIDHashMap uses mData[0] to pick the bucket
			// overwrite mData[0] so that it ranges from 0 to 255
			// to create a sparse map
			id.mData[0] = i; 
			UUIDTableEntry entry(id, i);
			hashTable.set(id, entry);
			idList[i] = id;
		}

		hashIter.first();
		int numElementsIterated = 0;
		while(!hashIter.done())
		{
			numElementsIterated++;
			UUIDTableEntry tableEntry = *hashIter;
			LLUUID id = tableEntry.getID();
			hashIter.next();
			ensure("Iteration failed for sparse map", tableEntry.getValue() < (size_t)numElementsToCheck && idList[tableEntry.getValue()] ==  tableEntry.getID());
		}

		ensure("iteration count failed", numElementsIterated == numElementsToCheck);
	}

	// remove after middle of iteration
	template<> template<>
	void hash_index_object_t::test<7>()
	{
		LLUUIDHashMap<UUIDTableEntry, 2>	hashTable(UUIDTableEntry::uuidEq, UUIDTableEntry());
		LLUUIDHashMapIter<UUIDTableEntry, 2> hashIter(&hashTable);
		const int numElementsToCheck = 256;
		std::vector<LLUUID> idList(numElementsToCheck);
		int i;
		
		LLUUID uuidtoSearch;
		for (i = 0; i < numElementsToCheck; i++)
		{
			LLUUID id;
			id.generate();
			// LLUUIDHashMap uses mData[0] to pick the bucket
			// overwrite mData[0] so that it ranges from 0 to 255
			// to create a sparse map
			id.mData[0] = i; 
			UUIDTableEntry entry(id, i);
			hashTable.set(id, entry);
			idList[i] = id;

			// pick uuid somewhere in the middle
			if (i == 5)
			{
				uuidtoSearch = id;
			}
		}

		hashIter.first();
		int numElementsIterated = 0;
		while(!hashIter.done())
		{
			numElementsIterated++;
			UUIDTableEntry tableEntry = *hashIter;
			LLUUID id = tableEntry.getID();
			if (uuidtoSearch == id)
			{
				break;
			}
			hashIter.next();
		}

		// current iterator implementation will not allow any remove operations
		// until ALL elements have been iterated over. this seems to be 
		// an unnecessary restriction. Iterator should have a method to
		// reset() its state so that further operations (inckuding remove)
		// can be performed on the HashMap without having to iterate thru 
		// all the remaining nodes. 
		
//		 hashIter.reset();
//		 hashTable.remove(uuidtoSearch);
//		 ensure("remove after iteration reset failed", hashTable.check(uuidtoSearch) == FALSE);
	}
}
