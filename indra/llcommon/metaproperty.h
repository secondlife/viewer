/** 
 * @file metaproperty.h
 * @author Babbage
 * @date 2006-05-15
 * @brief Reflective meta information describing a property of a class.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
