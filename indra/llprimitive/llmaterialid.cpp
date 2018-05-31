/** 
* @file llmaterialid.cpp
* @brief Implementation of llmaterialid
* @author Stinson@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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

#include "linden_common.h"

#include "llmaterialid.h"

#include <string>

#include "llformat.h"

const LLMaterialID LLMaterialID::null;

LLMaterialID::LLMaterialID()
{
	clear();
}

LLMaterialID::LLMaterialID(const LLSD& pMaterialID)
{
	llassert(pMaterialID.isBinary());
	parseFromBinary(pMaterialID.asBinary());
}

LLMaterialID::LLMaterialID(const LLSD::Binary& pMaterialID)
{
	parseFromBinary(pMaterialID);
}

LLMaterialID::LLMaterialID(const void* pMemory)
{
	set(pMemory);
}

LLMaterialID::LLMaterialID(const LLMaterialID& pOtherMaterialID)
{
	copyFromOtherMaterialID(pOtherMaterialID);
}

LLMaterialID::LLMaterialID(const LLUUID& lluid)
{
	set(lluid.mData);
}

LLMaterialID::~LLMaterialID()
{
}

bool LLMaterialID::operator == (const LLMaterialID& pOtherMaterialID) const
{
	return (compareToOtherMaterialID(pOtherMaterialID) == 0);
}

bool LLMaterialID::operator != (const LLMaterialID& pOtherMaterialID) const
{
	return (compareToOtherMaterialID(pOtherMaterialID) != 0);
}

bool LLMaterialID::operator < (const LLMaterialID& pOtherMaterialID) const
{
	return (compareToOtherMaterialID(pOtherMaterialID) < 0);
}

bool LLMaterialID::operator <= (const LLMaterialID& pOtherMaterialID) const
{
	return (compareToOtherMaterialID(pOtherMaterialID) <= 0);
}

bool LLMaterialID::operator > (const LLMaterialID& pOtherMaterialID) const
{
	return (compareToOtherMaterialID(pOtherMaterialID) > 0);
}

bool LLMaterialID::operator >= (const LLMaterialID& pOtherMaterialID) const
{
	return (compareToOtherMaterialID(pOtherMaterialID) >= 0);
}

LLMaterialID& LLMaterialID::operator = (const LLMaterialID& pOtherMaterialID)
{
	copyFromOtherMaterialID(pOtherMaterialID);
	return (*this);
}

bool LLMaterialID::isNull() const
{
	return (compareToOtherMaterialID(LLMaterialID::null) == 0);
}

const U8* LLMaterialID::get() const
{
	return mID;
}

void LLMaterialID::set(const void* pMemory)
{
	llassert(pMemory != NULL);

	// assumes that the required size of memory is available
	memcpy(mID, pMemory, MATERIAL_ID_SIZE * sizeof(U8));
}

void LLMaterialID::clear()
{
	memset(mID, 0, MATERIAL_ID_SIZE * sizeof(U8));
}

LLSD LLMaterialID::asLLSD() const
{
	LLSD::Binary materialIDBinary;

	materialIDBinary.resize(MATERIAL_ID_SIZE * sizeof(U8));
	memcpy(materialIDBinary.data(), mID, MATERIAL_ID_SIZE * sizeof(U8));

	LLSD materialID = materialIDBinary;
	return materialID;
}

std::string LLMaterialID::asString() const
{
	std::string materialIDString;
	for (unsigned int i = 0U; i < static_cast<unsigned int>(MATERIAL_ID_SIZE / sizeof(U32)); ++i)
	{
		if (i != 0U)
		{
			materialIDString += "-";
		}
		const U32 *value = reinterpret_cast<const U32*>(&get()[i * sizeof(U32)]);
		materialIDString += llformat("%08x", *value);
	}
	return materialIDString;
}

std::ostream& operator<<(std::ostream& s, const LLMaterialID &material_id)
{
	s << material_id.asString();
	return s;
}


void LLMaterialID::parseFromBinary (const LLSD::Binary& pMaterialID)
{
	llassert(pMaterialID.size() == (MATERIAL_ID_SIZE * sizeof(U8)));
	memcpy(mID, &pMaterialID[0], MATERIAL_ID_SIZE * sizeof(U8));
}

void LLMaterialID::copyFromOtherMaterialID(const LLMaterialID& pOtherMaterialID)
{
	memcpy(mID, pOtherMaterialID.get(), MATERIAL_ID_SIZE * sizeof(U8));
}

int LLMaterialID::compareToOtherMaterialID(const LLMaterialID& pOtherMaterialID) const
{
	int retVal = 0;

	for (unsigned int i = 0U; (retVal == 0) && (i < static_cast<unsigned int>(MATERIAL_ID_SIZE / sizeof(U32))); ++i)
	{
		const U32 *thisValue = reinterpret_cast<const U32*>(&get()[i * sizeof(U32)]);
		const U32 *otherValue = reinterpret_cast<const U32*>(&pOtherMaterialID.get()[i * sizeof(U32)]);
		retVal = ((*thisValue < *otherValue) ? -1 : ((*thisValue > *otherValue) ? 1 : 0));
	}

	return retVal;
}
