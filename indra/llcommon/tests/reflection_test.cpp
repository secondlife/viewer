/** 
 * @file reflection_test.cpp
 * @date   May 2006
 * @brief Reflection unit tests.
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

#include "../linden_common.h"
#include "../reflective.h"
#include "../metaclasst.h"
#include "../metapropertyt.h"
#include "../stdtypes.h"

#include "../test/lltut.h"

namespace tut
{
  class TestAggregatedData : public LLReflective
  {
  public:
	TestAggregatedData() {;}
	virtual const LLMetaClass& getMetaClass() const;
  
  private:
  };
  
  class TestReflectionData : public LLReflective
  {
  public:
	TestReflectionData() : mInt(42), mString("foo"), mNullPtr(NULL), mPtr(new TestAggregatedData()), mRef(*(new TestAggregatedData)) {;}
	virtual ~TestReflectionData() {delete mPtr;}
	virtual const LLMetaClass& getMetaClass() const;
	
	static U32 getPropertyCount() {return 5;}
	
  private:
  
	friend class LLMetaClassT<TestReflectionData>;
    S32 mInt;
	std::string mString;
	TestAggregatedData* mNullPtr;
	TestAggregatedData* mPtr;
	TestAggregatedData mObj;
	TestAggregatedData& mRef;
  };
}

template <>
void LLMetaClassT<tut::TestReflectionData>::reflectProperties(LLMetaClass& meta_class)
{
	reflectProperty(meta_class, "mInt", &tut::TestReflectionData::mInt);
	reflectProperty(meta_class, "mString", &tut::TestReflectionData::mString);
	reflectPtrProperty(meta_class, "mNullPtr", &tut::TestReflectionData::mNullPtr);
	reflectPtrProperty(meta_class, "mPtr", &tut::TestReflectionData::mPtr);
	reflectProperty(meta_class, "mObj", &tut::TestReflectionData::mObj);
	//reflectProperty(meta_class, "mRef", &tut::TestReflectionData::mRef); // AARGH!
}

namespace tut
{
	// virtual
	const LLMetaClass& TestReflectionData::getMetaClass() const
	{
	   return LLMetaClassT<TestReflectionData>::instance();
    }
	
	const LLMetaClass& TestAggregatedData::getMetaClass() const
	{
	   return LLMetaClassT<TestAggregatedData>::instance();
    }
}

namespace tut
{
  typedef tut::test_group<TestReflectionData> TestReflectionGroup;
  typedef TestReflectionGroup::object TestReflectionObject;
  TestReflectionGroup gTestReflectionGroup("reflection");

  template<> template<>
  void TestReflectionObject::test<1>()
  {
	// Check properties can be found.
    const LLMetaClass& meta_class = LLMetaClassT<TestReflectionData>::instance();
	const LLMetaProperty* null = NULL;
	ensure_not_equals(meta_class.findProperty("mInt"), null);
	ensure_not_equals(meta_class.findProperty("mString"), null);
  }
  
  template<> template<>
  void TestReflectionObject::test<2>()
  {
	// Check non-existent property cannot be found.
    const LLMetaClass& meta_class = LLMetaClassT<TestReflectionData>::instance();
	const LLMetaProperty* null = NULL;
	ensure_equals(meta_class.findProperty("foo"), null);
  }
  
  template<> template<>
  void TestReflectionObject::test<3>()
  {
	// Check integer property has correct value.	
    const LLMetaClass& meta_class = LLMetaClassT<TestReflectionData>::instance();
	ensure_equals(meta_class.findProperty("mInt")->getLLSD(this).asInteger(), 42);
  }
  
  template<> template<>
  void TestReflectionObject::test<4>()
  {
	// Check string property has correct value.	
    const LLMetaClass& meta_class = LLMetaClassT<TestReflectionData>::instance();
	ensure_equals(meta_class.findProperty("mString")->getLLSD(this).asString(), std::string("foo"));
  }
  
  template<> template<>
  void TestReflectionObject::test<5>()
  {
	// Check NULL reference property has correct value.
	const LLMetaClass& meta_class = LLMetaClassT<TestReflectionData>::instance();
	const LLReflective* null = NULL;
	ensure_equals(meta_class.findProperty("mNullPtr")->get(this), null);
  }
  
  template<> template<>
  void TestReflectionObject::test<6>()
  {
	// Check reference property has correct value.
	const LLMetaClass& meta_class = LLMetaClassT<TestReflectionData>::instance();
	const LLReflective* null = NULL;
	const LLReflective* ref = meta_class.findProperty("mPtr")->get(this);
	ensure_not_equals(ref, null);
  }
  
  template<> template<>
  void TestReflectionObject::test<7>()
  {
	// Check reflective property has correct value.
	const LLMetaClass& meta_class = LLMetaClassT<TestReflectionData>::instance();
	const LLReflective* null = NULL;
	const LLReflective* ref = meta_class.findProperty("mObj")->get(this);
	ensure_not_equals(ref, null);
  }

  template<> template<>
  void TestReflectionObject::test<8>()
  {
	// Check property count.
    const LLMetaClass& meta_class = LLMetaClassT<TestReflectionData>::instance();
	ensure_equals(meta_class.getPropertyCount(), TestReflectionData::getPropertyCount());
  }
  
  template<> template<>
  void TestReflectionObject::test<9>()
  {
	// Check property iteration.
    const LLMetaClass& meta_class = LLMetaClassT<TestReflectionData>::instance();
	U32 count = 0;
	LLMetaClass::PropertyIterator iter;
	for(iter = meta_class.beginProperties(); iter != meta_class.endProperties(); ++iter)
	{
		++count;
	}
	ensure_equals(count, TestReflectionData::getPropertyCount());
  }
  
  template<> template<>
  void TestReflectionObject::test<10>()
  {
	// Check meta classes of different types do not compare equal.
	const LLMetaClass* reflection_data_meta_class = &(LLMetaClassT<TestReflectionData>::instance());
	const LLMetaClass* aggregated_data_meta_class = &(LLMetaClassT<TestAggregatedData>::instance());
	ensure_not_equals(reflection_data_meta_class, aggregated_data_meta_class);
  }
  
  template<> template<>
  void TestReflectionObject::test<11>()
  {
	// Check class cast checks.
	const LLMetaClass& meta_class = LLMetaClassT<TestReflectionData>::instance();
	TestAggregatedData* aggregated_data = new TestAggregatedData();
	LLMetaClass::PropertyIterator iter;
	U32 exception_count = 0;
	for(iter = meta_class.beginProperties(); iter != meta_class.endProperties(); ++iter)
	{
		try
		{
			const LLMetaProperty* property = (*iter).second;
			const LLReflective* reflective = property->get(aggregated_data); // Wrong reflective type, should throw exception.

			// useless op to get rid of compiler warning.
			reflective = reflective;
		}
		catch(...)
		{
			++exception_count;
		}
	}
	ensure_equals(exception_count, getPropertyCount());
	
  }
}
