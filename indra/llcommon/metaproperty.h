/** 
 * @file metaproperty.h
 * @author Babbage
 * @date 2006-05-15
 * @brief Reflective meta information describing a property of a class.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_METAPROPERTY_H
#define LL_METAPROPERTY_H

#include "stdtypes.h"
#include "llsd.h"
#include "reflective.h"

class LLMetaClass;
class LLReflective;
class LL_COMMON_API LLMetaProperty
{
public:
	LLMetaProperty(const std::string& name, const LLMetaClass& object_class);
  virtual ~LLMetaProperty();
  
  // Get property name.
  const std::string& getName() const {return mName;}

  // Get value of this property.
  virtual const LLReflective* get(const LLReflective* object) const = 0;
  
  // Set value of this property.
  // virtual void set(LLReflective* object, const LLReflective* value) = 0;
  
  // Get value of this property as LLSD. Default returns undefined LLSD.
  virtual LLSD getLLSD(const LLReflective* object) const = 0;
  
  // Get the MetaClass of legal values of this property.
  // const LLMetaClass& getValueMetaClass();
  
  // Get the meta class that this property is a member of.
  const LLMetaClass& getObjectMetaClass() const;

protected:

  // Check object is instance of object class, throw exception if not.
  void checkObjectClass(const LLReflective* object) const;

private:

	std::string mName;
	const LLMetaClass& mObjectClass;
};

#endif // LL_METAPROPERTY_H
