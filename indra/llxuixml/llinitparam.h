/** 
f * @file llinitparam.h
 * @brief parameter block abstraction for creating complex objects and 
 * parsing construction parameters from xml and LLSD
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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

#ifndef LL_LLPARAM_H
#define LL_LLPARAM_H

#include <vector>

#include <stddef.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include "llregistry.h"
#include "llmemory.h"


namespace LLInitParam
{

	template <typename T, bool IS_BOOST_FUNCTION = boost::is_convertible<T, boost::function_base>::value >
    struct ParamCompare 
	{
    	static bool equals(const T &a, const T &b)
		{
			return a == b;
		}
    };
    
	// boost function types are not comparable
	template<typename T>
	struct ParamCompare<T, true>
	{
		static bool equals(const T&a, const T &b)
		{
			return false;
		}
	};

	// default constructor adaptor for InitParam Values
	// constructs default instances of the given type, returned by const reference
	template <typename T>
	struct DefaultInitializer
	{
		typedef const T&			T_const_ref;
		// return reference to a single default instance of T
		// built-in types will be initialized to zero, default constructor otherwise
		static T_const_ref get() { static T t = T(); return t; } 
	};

	// helper functions and classes
	typedef ptrdiff_t param_handle_t;

	template <typename T>
	class TypeValues
	{
	public:
		// empty default implemenation of key cache
		class KeyCache
		{
		public:
			void setKey(const std::string& key) {}
			std::string getKey() const { return ""; }
			void clearKey(){}
		};

		static bool get(const std::string& name, T& value)
		{
			return false;
		}

		static bool empty()
		{
			return true;
		}

		static std::vector<std::string>* getPossibleValues() { return NULL; }
	};

	template <typename T, typename DERIVED_TYPE = TypeValues<T> >
	class TypeValuesHelper
	:	public LLRegistrySingleton<std::string, T, DERIVED_TYPE >
	{
		typedef LLRegistrySingleton<std::string, T, DERIVED_TYPE>	super_t;
		typedef LLSingleton<DERIVED_TYPE>							singleton_t;
	public:

		//TODO: cache key by index to save on param block size
		class KeyCache
		{
		public:
			void setKey(const std::string& key) 
			{
				mKey = key; 
			}

			void clearKey()
			{
				mKey = "";
			}

			std::string getKey() const
			{ 
				return mKey; 
			}

		private:
			std::string mKey;
		};

		static bool get(const std::string& name, T& value)
		{
			if (!singleton_t::instance().exists(name)) return false;

			value = *singleton_t::instance().getValue(name);
			return true;
		}

		static bool empty()
		{
			return singleton_t::instance().LLRegistry<std::string, T>::empty();
		}
	
		//override this to add name value pairs
		static void declareValues() {}
	
		void initSingleton()
		{
			DERIVED_TYPE::declareValues();
		}

		static const std::vector<std::string>* getPossibleValues() 
		{ 
			// in order to return a pointer to a member, we lazily
			// evaluate the result and store it in mValues here
			if (singleton_t::instance().mValues.empty())
			{
				typename super_t::Registrar::registry_map_t::const_iterator it;
				for (it = super_t::defaultRegistrar().beginItems(); it != super_t::defaultRegistrar().endItems(); ++it)
				{
					singleton_t::instance().mValues.push_back(it->first);
				}
			}
			return &singleton_t::instance().mValues; 
		}


	protected:
		static void declare(const std::string& name, const T& value)
		{
			super_t::defaultRegistrar().add(name, value);
		}

	private:
		std::vector<std::string> mValues;
	};

	class Parser
	{
		LOG_CLASS(Parser);

	public:
		
		struct CompareTypeID
		{
			bool operator()(const std::type_info* lhs, const std::type_info* rhs) const
			{
				return lhs->before(*rhs);
			}
		};

		typedef std::vector<std::pair<std::string, S32> >			name_stack_t;
		typedef std::pair<name_stack_t::const_iterator, name_stack_t::const_iterator>	name_stack_range_t;
		typedef std::vector<std::string>							possible_values_t;

		typedef boost::function<bool (void*)>															parser_read_func_t;
		typedef boost::function<bool (const void*, const name_stack_t&)>								parser_write_func_t;
		typedef boost::function<void (const name_stack_t&, S32, S32, const possible_values_t*)>	parser_inspect_func_t;

		typedef std::map<const std::type_info*, parser_read_func_t, CompareTypeID>		parser_read_func_map_t;
		typedef std::map<const std::type_info*, parser_write_func_t, CompareTypeID>		parser_write_func_map_t;
		typedef std::map<const std::type_info*, parser_inspect_func_t, CompareTypeID>	parser_inspect_func_map_t;

		Parser()
		:	mParseSilently(false),
			mParseGeneration(0)
		{}
		virtual ~Parser();

		template <typename T> bool readValue(T& param)
	    {
		    parser_read_func_map_t::iterator found_it = mParserReadFuncs.find(&typeid(T));
		    if (found_it != mParserReadFuncs.end())
		    {
			    return found_it->second((void*)&param);
		    }
		    return false;
	    }

		template <typename T> bool writeValue(const T& param, const name_stack_t& name_stack)
		{
		    parser_write_func_map_t::iterator found_it = mParserWriteFuncs.find(&typeid(T));
		    if (found_it != mParserWriteFuncs.end())
		    {
			    return found_it->second((const void*)&param, name_stack);
		    }
		    return false;
		}

		// dispatch inspection to registered inspection functions, for each parameter in a param block
		template <typename T> bool inspectValue(const name_stack_t& name_stack, S32 min_count, S32 max_count, const possible_values_t* possible_values)
		{
		    parser_inspect_func_map_t::iterator found_it = mParserInspectFuncs.find(&typeid(T));
		    if (found_it != mParserInspectFuncs.end())
		    {
			    found_it->second(name_stack, min_count, max_count, possible_values);
				return true;
		    }
			return false;
		}

		virtual std::string getCurrentElementName() = 0;
		virtual void parserWarning(const std::string& message);
		virtual void parserError(const std::string& message);
		void setParseSilently(bool silent) { mParseSilently = silent; }
		bool getParseSilently() { return mParseSilently; }

		S32 getParseGeneration() { return mParseGeneration; }
		S32 newParseGeneration() { return ++mParseGeneration; }


	protected:
		template <typename T>
		void registerParserFuncs(parser_read_func_t read_func, parser_write_func_t write_func)
		{
			mParserReadFuncs.insert(std::make_pair(&typeid(T), read_func));
			mParserWriteFuncs.insert(std::make_pair(&typeid(T), write_func));
		}

		template <typename T>
		void registerInspectFunc(parser_inspect_func_t inspect_func)
		{
			mParserInspectFuncs.insert(std::make_pair(&typeid(T), inspect_func));
		}

		bool				mParseSilently;

	private:
		parser_read_func_map_t		mParserReadFuncs;
		parser_write_func_map_t		mParserWriteFuncs;
		parser_inspect_func_map_t	mParserInspectFuncs;
		S32	mParseGeneration;
	};

	class BaseBlock;

	class Param
	{
	public:
		// public to allow choice blocks to clear provided flag on stale choices
		void setProvided(bool is_provided) { mIsProvided = is_provided; }

	protected:
		bool getProvided() const { return mIsProvided; }

		Param(class BaseBlock* enclosing_block);

		// store pointer to enclosing block as offset to reduce space and allow for quick copying
		BaseBlock& enclosingBlock() const
		{ 
			const U8* my_addr = reinterpret_cast<const U8*>(this);
			// get address of enclosing BLOCK class using stored offset to enclosing BaseBlock class
			return *const_cast<BaseBlock*>(
							reinterpret_cast<const BaseBlock*>(my_addr - (ptrdiff_t)(S32)mEnclosingBlockOffset));
		}

	private:
		friend class BaseBlock;

		bool		mIsProvided;
		U16			mEnclosingBlockOffset;
	};

	// various callbacks and constraints associated with an individual param
	struct ParamDescriptor
	{
	public:
		typedef bool(*merge_func_t)(Param&, const Param&, bool);
		typedef bool(*deserialize_func_t)(Param&, Parser&, const Parser::name_stack_range_t&, S32);
		typedef void(*serialize_func_t)(const Param&, Parser&, Parser::name_stack_t&, const Param* diff_param);
		typedef void(*inspect_func_t)(const Param&, Parser&, Parser::name_stack_t&, S32 min_count, S32 max_count);
		typedef bool(*validation_func_t)(const Param*);

		ParamDescriptor(param_handle_t p, 
				merge_func_t merge_func, 
				deserialize_func_t deserialize_func, 
				serialize_func_t serialize_func,
				validation_func_t validation_func,
				inspect_func_t inspect_func,
				S32 min_count,
				S32 max_count)
		:	mParamHandle(p),
			mMergeFunc(merge_func),
			mDeserializeFunc(deserialize_func),
			mSerializeFunc(serialize_func),
			mValidationFunc(validation_func),
			mInspectFunc(inspect_func),
			mMinCount(min_count),
			mMaxCount(max_count),
			mGeneration(0),
			mNumRefs(0)
		{}

		ParamDescriptor()
		:	mParamHandle(0),
			mMergeFunc(NULL),
			mDeserializeFunc(NULL),
			mSerializeFunc(NULL),
			mValidationFunc(NULL),
			mInspectFunc(NULL),
			mMinCount(0),
			mMaxCount(0),
			mGeneration(0),
			mNumRefs(0)
		{}

		param_handle_t		mParamHandle;
	
		merge_func_t		mMergeFunc;
		deserialize_func_t	mDeserializeFunc;
		serialize_func_t	mSerializeFunc;
		inspect_func_t		mInspectFunc;
		validation_func_t	mValidationFunc;
		S32					mMinCount;
		S32					mMaxCount;
		S32					mGeneration;
		S32					mNumRefs;
	};

	// each derived Block class keeps a static data structure maintaining offsets to various params
	class BlockDescriptor
	{
	public:
		BlockDescriptor()
		:	mMaxParamOffset(0),
			mInitializationState(UNINITIALIZED),
			mCurrentBlockPtr(NULL)
		{}

		typedef enum e_initialization_state
		{
			UNINITIALIZED,
			INITIALIZING,
			INITIALIZED
		} EInitializationState;

		void aggregateBlockData(BlockDescriptor& src_block_data);

	public:
		typedef std::map<const std::string, ParamDescriptor*> param_map_t; // references param descriptors stored in mAllParams
		typedef std::vector<ParamDescriptor*> param_list_t; 

		typedef std::list<ParamDescriptor> all_params_list_t;// references param descriptors stored in mAllParams
		typedef std::vector<std::pair<param_handle_t, ParamDescriptor::validation_func_t> > param_validation_list_t;

		param_map_t						mNamedParams;			// parameters with associated names
		param_map_t						mSynonyms;				// parameters with alternate names
		param_list_t					mUnnamedParams;			// parameters with_out_ associated names
		param_validation_list_t			mValidationList;		// parameters that must be validated
		all_params_list_t				mAllParams;				// all parameters, owns descriptors

		size_t					mMaxParamOffset;

		EInitializationState	mInitializationState;	// whether or not static block data has been initialized
		class BaseBlock*		mCurrentBlockPtr;		// pointer to block currently being constructed
	};

	class BaseBlock
	{
	public:
		// "Multiple" constraint types
		struct AnyAmount
		{
			static U32 minCount() { return 0; }
			static U32 maxCount() { return U32_MAX; }
		};

		template<U32 MIN_AMOUNT>
		struct AtLeast
		{
			static U32 minCount() { return MIN_AMOUNT; }
			static U32 maxCount() { return U32_MAX; }
		};

		template<U32 MAX_AMOUNT>
		struct AtMost
		{
			static U32 minCount() { return 0; }
			static U32 maxCount() { return MAX_AMOUNT; }
		};

		template<U32 MIN_AMOUNT, U32 MAX_AMOUNT>
		struct Between
		{
			static U32 minCount() { return MIN_AMOUNT; }
			static U32 maxCount() { return MAX_AMOUNT; }
		};

		template<U32 EXACT_COUNT>
		struct Exactly
		{
			static U32 minCount() { return EXACT_COUNT; }
			static U32 maxCount() { return EXACT_COUNT; }
		};

		// this typedef identifies derived classes as being blocks
		typedef void baseblock_base_class_t;
		LOG_CLASS(BaseBlock);
		friend class Param;

		BaseBlock();
		virtual ~BaseBlock();
		bool submitValue(const Parser::name_stack_t& name_stack, Parser& p, bool silent=false);

		param_handle_t getHandleFromParam(const Param* param) const;
		bool validateBlock(bool silent = false) const;

		Param* getParamFromHandle(const param_handle_t param_handle)
		{
			if (param_handle == 0) return NULL;
			U8* baseblock_address = reinterpret_cast<U8*>(this);
			return reinterpret_cast<Param*>(baseblock_address + param_handle);
		}

		const Param* getParamFromHandle(const param_handle_t param_handle) const
		{
			const U8* baseblock_address = reinterpret_cast<const U8*>(this);
			return reinterpret_cast<const Param*>(baseblock_address + param_handle);
		}

		void addSynonym(Param& param, const std::string& synonym);

		// Blocks can override this to do custom tracking of changes
		virtual void setLastChangedParam(const Param& last_param, bool user_provided);

		S32 getLastChangeVersion() const { return mChangeVersion; }
		bool isDefault() const { return mChangeVersion == 0; }

		bool deserializeBlock(Parser& p, Parser::name_stack_range_t name_stack);
		bool serializeBlock(Parser& p, Parser::name_stack_t name_stack = Parser::name_stack_t(), const BaseBlock* diff_block = NULL) const;
		virtual bool inspectBlock(Parser& p, Parser::name_stack_t name_stack = Parser::name_stack_t()) const;

		const BlockDescriptor& getBlockDescriptor() const { return *mBlockDescriptor; }
		BlockDescriptor& getBlockDescriptor() { return *mBlockDescriptor; }

		// take all provided params from other and apply to self
		bool overwriteFrom(const BaseBlock& other)
		{
			return false;
		}

		// take all provided params that are not already provided, and apply to self
		bool fillFrom(const BaseBlock& other)
		{
			return false;
		}

		static void addParam(BlockDescriptor& block_data, const ParamDescriptor& param, const char* name);
	protected:
		void init(BlockDescriptor& descriptor, BlockDescriptor& base_descriptor, size_t block_size);


		// take all provided params from other and apply to self
		bool overwriteFromImpl(BlockDescriptor& block_data, const BaseBlock& other);

		// take all provided params that are not already provided, and apply to self
		bool fillFromImpl(BlockDescriptor& block_data, const BaseBlock& other);

		// can be updated in getters
		mutable S32				mChangeVersion;

		BlockDescriptor*		mBlockDescriptor;	// most derived block descriptor

		static BlockDescriptor& blockDescriptor()
		{
			static BlockDescriptor sBlockDescriptor;
			return sBlockDescriptor;
		}

	private:
		const std::string& getParamName(const BlockDescriptor& block_data, const Param* paramp) const;
		ParamDescriptor* findParamDescriptor(param_handle_t handle);
	};


	template<typename T>
	struct ParamIterator
	{
		typedef typename std::vector<T>::const_iterator		const_iterator;
		typedef typename std::vector<T>::iterator			iterator;
	};

	// these templates allow us to distinguish between template parameters
	// that derive from BaseBlock and those that don't
	// this is supposedly faster than boost::is_convertible and its ilk
	template<typename T, typename Void = void>
	struct IsBaseBlock
	{
		static const bool value = false;
	};

	template<typename T>
	struct IsBaseBlock<T, typename T::baseblock_base_class_t>
	{
		static const bool value = true;
	};

	// specialize for custom parsing/decomposition of specific classes
	// e.g. TypedParam<LLRect> has left, top, right, bottom, etc...
	template<typename	T,
			typename	NAME_VALUE_LOOKUP = TypeValues<T>,
			bool		HAS_MULTIPLE_VALUES = false,
			bool		VALUE_IS_BLOCK = IsBaseBlock<T>::value>
	class TypedParam 
	:	public Param
	{
	public:
		typedef const T&																	value_const_ref_t;
		typedef value_const_ref_t															value_assignment_t;
		typedef typename NAME_VALUE_LOOKUP::KeyCache										key_cache_t;
		typedef	TypedParam<T, NAME_VALUE_LOOKUP, HAS_MULTIPLE_VALUES, VALUE_IS_BLOCK>		self_t;

		TypedParam(BlockDescriptor& block_descriptor, const char* name, value_assignment_t value, ParamDescriptor::validation_func_t validate_func, S32 min_count, S32 max_count) 
		:	Param(block_descriptor.mCurrentBlockPtr)
		{
			if (block_descriptor.mInitializationState == BlockDescriptor::INITIALIZING)
			{
				ParamDescriptor param_descriptor(block_descriptor.mCurrentBlockPtr->getHandleFromParam(this),
												&mergeWith,
												&deserializeParam,
												&serializeParam,
												validate_func,
												&inspectParam,
												min_count, max_count);
				BaseBlock::addParam(block_descriptor, param_descriptor, name);
			}

			mData.mValue = value;
		} 

		bool isProvided() const { return Param::getProvided(); }

		static bool deserializeParam(Param& param, Parser& parser, const Parser::name_stack_range_t& name_stack, S32 generation) 
		{ 
			self_t& typed_param = static_cast<self_t&>(param);
			// no further names in stack, attempt to parse value now
			if (name_stack.first == name_stack.second)
			{
				if (parser.readValue<T>(typed_param.mData.mValue))
				{
					typed_param.setProvided(true);
					typed_param.enclosingBlock().setLastChangedParam(param, true);
					return true;
				}
				
				// try to parse a known named value
				if(!NAME_VALUE_LOOKUP::empty())
				{
					// try to parse a known named value
					std::string name;
					if (parser.readValue<std::string>(name))
					{
						// try to parse a per type named value
						if (NAME_VALUE_LOOKUP::get(name, typed_param.mData.mValue))
						{
							typed_param.mData.setKey(name);
							typed_param.setProvided(true);
							typed_param.enclosingBlock().setLastChangedParam(param, true);
							return true;
						}

					}
				}
			}
			return false;
		}

		static void serializeParam(const Param& param, Parser& parser, Parser::name_stack_t& name_stack, const Param* diff_param)
		{
			const self_t& typed_param = static_cast<const self_t&>(param);
			if (!typed_param.isProvided()) return;

			if (!name_stack.empty())
			{
				name_stack.back().second = parser.newParseGeneration();
			}

			std::string key = typed_param.mData.getKey();

			// first try to write out name of name/value pair

			if (!key.empty())
			{
				if (!diff_param || !ParamCompare<std::string>::equals(static_cast<const self_t*>(diff_param)->mData.getKey(), key))
				{
					if (!parser.writeValue<std::string>(key, name_stack))
					{
						return;
					}
				}
			}
			// then try to serialize value directly
			else if (!diff_param || !ParamCompare<T>::equals(typed_param.get(), static_cast<const self_t*>(diff_param)->get()))					{
				if (!parser.writeValue<T>(typed_param.mData.mValue, name_stack)) 
				{
					return;
				}
			}
		}

		static void inspectParam(const Param& param, Parser& parser, Parser::name_stack_t& name_stack, S32 min_count, S32 max_count)
		{
			// tell parser about our actual type
			parser.inspectValue<T>(name_stack, min_count, max_count, NULL);
			// then tell it about string-based alternatives ("red", "blue", etc. for LLColor4)
			if (NAME_VALUE_LOOKUP::getPossibleValues())
			{
				parser.inspectValue<std::string>(name_stack, min_count, max_count, NAME_VALUE_LOOKUP::getPossibleValues());
			}
		}

		void set(value_assignment_t val, bool flag_as_provided = true)
		{
			mData.mValue = val;
			mData.clearKey();
			setProvided(flag_as_provided);
			Param::enclosingBlock().setLastChangedParam(*this, flag_as_provided);
		}

		void setIfNotProvided(value_assignment_t val, bool flag_as_provided = true)
		{
			if (!isProvided())
			{
				set(val, flag_as_provided);
			}
		}

		// implicit conversion
		operator value_assignment_t() const { return get(); } 
		// explicit conversion
		value_assignment_t operator()() const { return get(); } 

	protected:
		value_assignment_t get() const
		{
			return mData.mValue;
		}

		static bool mergeWith(Param& dst, const Param& src, bool overwrite)
		{
			const self_t& src_typed_param = static_cast<const self_t&>(src);
			self_t& dst_typed_param = static_cast<self_t&>(dst);
			if (src_typed_param.isProvided()
				&& (overwrite || !dst_typed_param.isProvided()))
			{
				dst_typed_param.mData.clearKey();
				dst_typed_param = src_typed_param;
				return true;
			}
			return false;
		}

		struct Data : public key_cache_t
		{
			T mValue;
		};

		Data		mData;
	};

	// parameter that is a block
	template <typename T, typename NAME_VALUE_LOOKUP>
	class TypedParam<T, NAME_VALUE_LOOKUP, false, true> 
	:	public T,
		public Param
	{
	public:
		typedef const T											value_const_t;
		typedef T												value_t;
		typedef value_const_t&									value_const_ref_t;
		typedef value_const_ref_t								value_assignment_t;
		typedef typename NAME_VALUE_LOOKUP::KeyCache			key_cache_t;
		typedef TypedParam<T, NAME_VALUE_LOOKUP, false, true>	self_t;

		TypedParam(BlockDescriptor& block_descriptor, const char* name, value_assignment_t value, ParamDescriptor::validation_func_t validate_func, S32 min_count, S32 max_count)
		:	Param(block_descriptor.mCurrentBlockPtr),
			T(value)
		{
			if (block_descriptor.mInitializationState == BlockDescriptor::INITIALIZING)
			{
				ParamDescriptor param_descriptor(block_descriptor.mCurrentBlockPtr->getHandleFromParam(this),
												&mergeWith,
												&deserializeParam,
												&serializeParam,
												validate_func, 
												&inspectParam,
												min_count, max_count);
				BaseBlock::addParam(block_descriptor, param_descriptor, name);
			}
		}

		static bool deserializeParam(Param& param, Parser& parser, const Parser::name_stack_range_t& name_stack, S32 generation) 
		{ 
			self_t& typed_param = static_cast<self_t&>(param);
			// attempt to parse block...
			if(typed_param.deserializeBlock(parser, name_stack))
			{
				typed_param.enclosingBlock().setLastChangedParam(param, true);
				return true;
			}

			if(!NAME_VALUE_LOOKUP::empty())
			{
				// try to parse a known named value
				std::string name;
				if (parser.readValue<std::string>(name))
				{
					// try to parse a per type named value
					if (NAME_VALUE_LOOKUP::get(name, typed_param))
					{
						typed_param.enclosingBlock().setLastChangedParam(param, true);
						typed_param.mData.setKey(name);
						typed_param.mData.mKeyVersion = typed_param.getLastChangeVersion();
						return true;
					}

				}
			}
			return false;
		}

		static void serializeParam(const Param& param, Parser& parser, Parser::name_stack_t& name_stack, const Param* diff_param)
		{
			const self_t& typed_param = static_cast<const self_t&>(param);
			if (!name_stack.empty())
			{
				name_stack.back().second = parser.newParseGeneration();
			}

			std::string key = typed_param.mData.getKey();
			if (!key.empty() && typed_param.mData.mKeyVersion == typed_param.getLastChangeVersion())
			{
				if (!parser.writeValue<std::string>(key, name_stack))
				{
					return;
				}
			}
			else
			{
				typed_param.serializeBlock(parser, name_stack, static_cast<const self_t*>(diff_param));
			}
		}

		static void inspectParam(const Param& param, Parser& parser, Parser::name_stack_t& name_stack, S32 min_count, S32 max_count)
		{
			// I am a param that is also a block, so just recurse into my contents
			const self_t& typed_param = static_cast<const self_t&>(param);
			typed_param.inspectBlock(parser, name_stack);
		}

		// a param-that-is-a-block is provided when the user has set one of its child params
		// *and* the block as a whole validates
		bool isProvided() const 
		{ 
			// only validate block when it hasn't already passed validation and user has supplied *some* value
			if (Param::getProvided() && mData.mValidatedVersion < T::getLastChangeVersion())
			{
				// a sub-block is "provided" when it has been filled in enough to be valid
				mData.mValidated = T::validateBlock(true);
				mData.mValidatedVersion = T::getLastChangeVersion();
			}
			return Param::getProvided() && mData.mValidated;
		}

		// assign block contents to this param-that-is-a-block
		void set(value_assignment_t val, bool flag_as_provided = true)
		{
			value_t::operator=(val);
			mData.clearKey();
			// force revalidation of block by clearing known provided version
			// next call to isProvided() will update provision status based on validity
			mData.mValidatedVersion = 0;
			setProvided(flag_as_provided);
			Param::enclosingBlock().setLastChangedParam(*this, flag_as_provided);
		}

		void setIfNotProvided(value_assignment_t val, bool flag_as_provided = true)
		{
			if (!isProvided())
			{
				set(val, flag_as_provided);
			}
		}

		// propagate changed status up to enclosing block
		/*virtual*/ void setLastChangedParam(const Param& last_param, bool user_provided)
		{ 
			T::setLastChangedParam(last_param, user_provided);
			Param::enclosingBlock().setLastChangedParam(*this, user_provided);
			if (user_provided)
			{
				// a child param has been explicitly changed
				// so *some* aspect of this block is now provided
				setProvided(true);
			}
		}

		// implicit conversion
		operator value_assignment_t() const { return get(); } 
		// explicit conversion
		value_assignment_t operator()() const { return get(); } 

	protected:
		value_assignment_t get() const
		{
			return *this;
		}

		static bool mergeWith(Param& dst, const Param& src, bool overwrite)
		{
			const self_t& src_typed_param = static_cast<const self_t&>(src);
			self_t& dst_typed_param = static_cast<self_t&>(dst);
			if (overwrite)
			{
				if (dst_typed_param.T::overwriteFrom(src_typed_param))
				{
					dst_typed_param.mData.clearKey();
					return true;
				}
			}
			else
			{
				if (dst_typed_param.T::fillFrom(src_typed_param))
				{			
					dst_typed_param.mData.clearKey();
					return true;
				}
			}
			return false;
		}

		struct Data : public key_cache_t
		{
			S32 			mKeyVersion;
			mutable S32 	mValidatedVersion;
			mutable bool 	mValidated; // lazy validation flag

			Data() 
			:	mKeyVersion(0),
				mValidatedVersion(0),
				mValidated(false)
			{}
		};
		Data	mData;
	};

	// container of non-block parameters
	template <typename VALUE_TYPE, typename NAME_VALUE_LOOKUP>
	class TypedParam<VALUE_TYPE, NAME_VALUE_LOOKUP, true, false> 
	:	public Param
	{
	public:
		typedef TypedParam<VALUE_TYPE, NAME_VALUE_LOOKUP, true, false>		self_t;
		typedef typename std::vector<VALUE_TYPE>							container_t;
		typedef const container_t&											value_assignment_t;

		typedef VALUE_TYPE													value_t;
		typedef value_t&													value_ref_t;
		typedef const value_t&												value_const_ref_t;
		
		typedef typename NAME_VALUE_LOOKUP::KeyCache						key_cache_t;

		TypedParam(BlockDescriptor& block_descriptor, const char* name, value_assignment_t value, ParamDescriptor::validation_func_t validate_func, S32 min_count, S32 max_count) 
		:	Param(block_descriptor.mCurrentBlockPtr),
			mValues(value)
		{
			mCachedKeys.resize(mValues.size());
			if (block_descriptor.mInitializationState == BlockDescriptor::INITIALIZING)
			{
				ParamDescriptor param_descriptor(block_descriptor.mCurrentBlockPtr->getHandleFromParam(this),
												&mergeWith,
												&deserializeParam,
												&serializeParam,
												validate_func,
												&inspectParam,
												min_count, max_count);
				BaseBlock::addParam(block_descriptor, param_descriptor, name);
			}
		} 

		bool isProvided() const { return Param::getProvided(); }

		static bool deserializeParam(Param& param, Parser& parser, const Parser::name_stack_range_t& name_stack, S32 generation) 
		{ 
			self_t& typed_param = static_cast<self_t&>(param);
			value_t value;
			// no further names in stack, attempt to parse value now
			if (name_stack.first == name_stack.second)
			{
				// attempt to read value directly
				if (parser.readValue<value_t>(value))
				{
					typed_param.mValues.push_back(value);
					// save an empty name/value key as a placeholder
					typed_param.mCachedKeys.push_back(key_cache_t());
					typed_param.enclosingBlock().setLastChangedParam(param, true);
					typed_param.setProvided(true);
					return true;
				}
				
				// try to parse a known named value
				if(!NAME_VALUE_LOOKUP::empty())
				{
					// try to parse a known named value
					std::string name;
					if (parser.readValue<std::string>(name))
					{
						// try to parse a per type named value
						if (NAME_VALUE_LOOKUP::get(name, typed_param.mValues))
						{
							typed_param.mValues.push_back(value);
							typed_param.mCachedKeys.push_back(key_cache_t());
							typed_param.mCachedKeys.back().setKey(name);
							typed_param.enclosingBlock().setLastChangedParam(param, true);
							typed_param.setProvided(true);
							return true;
						}

					}
				}
			}
			return false;
		}

		static void serializeParam(const Param& param, Parser& parser, Parser::name_stack_t& name_stack, const Param* diff_param)
		{
			const self_t& typed_param = static_cast<const self_t&>(param);
			if (!typed_param.isProvided() || name_stack.empty()) return;

			typename container_t::const_iterator it = typed_param.mValues.begin();
			for (typename std::vector<key_cache_t>::const_iterator key_it = typed_param.mCachedKeys.begin();
				it != typed_param.mValues.end();
				++key_it, ++it)
			{
				std::string key = key_it->get();
				name_stack.back().second = parser.newParseGeneration();

				if(!key.empty())
				{
					if(!parser.writeValue<std::string>(key, name_stack))
					{
						return;
					}
				}
				// not parse via name values, write out value directly
				else if (!parser.writeValue<VALUE_TYPE>(*it, name_stack))
				{
					return;
				}
			}
		}

		static void inspectParam(const Param& param, Parser& parser, Parser::name_stack_t& name_stack, S32 min_count, S32 max_count)
		{
			parser.inspectValue<VALUE_TYPE>(name_stack, min_count, max_count, NULL);
			if (NAME_VALUE_LOOKUP::getPossibleValues())
			{
				parser.inspectValue<std::string>(name_stack, min_count, max_count, NAME_VALUE_LOOKUP::getPossibleValues());
			}
		}

		void set(value_assignment_t val, bool flag_as_provided = true)
		{
			mValues = val;
			mCachedKeys.clear();
			mCachedKeys.resize(mValues.size());
			setProvided(flag_as_provided);
			Param::enclosingBlock().setLastChangedParam(*this, flag_as_provided);
		}


		void setIfNotProvided(value_assignment_t val, bool flag_as_provided = true)
		{
			if (!isProvided())
			{
				set(val, flag_as_provided);
			}
		}

		value_ref_t add()
		{
			mValues.push_back(value_t());
			mCachedKeys.push_back(key_cache_t());
			setProvided(true);
			return mValues.back();
		}

		void add(value_const_ref_t item)
		{
			mValues.push_back(item);
			mCachedKeys.push_back(key_cache_t());
			setProvided(true);
		}

		// implicit conversion
		operator value_assignment_t() const { return self_t::get(); } 
		// explicit conversion
		value_assignment_t operator()() const { return get(); } 

		U32 numValidElements() const
		{
			return mValues.size();
		}

	protected:
		value_assignment_t get() const
		{
			return mValues;
		}

		static bool mergeWith(Param& dst, const Param& src, bool overwrite)
		{
			const self_t& src_typed_param = static_cast<const self_t&>(src);
			self_t& dst_typed_param = static_cast<self_t&>(dst);

			if (src_typed_param.isProvided()
				&& (overwrite || !isProvided()))
			{
				dst_typed_param = src_typed_param;
				return true;
			}
			return false;
		}

		container_t		mValues;
		std::vector<key_cache_t>	mCachedKeys;
	};

	// container of block parameters
	template <typename VALUE_TYPE, typename NAME_VALUE_LOOKUP>
	class TypedParam<VALUE_TYPE, NAME_VALUE_LOOKUP, true, true> 
	:	public Param
	{
	public:
		typedef TypedParam<VALUE_TYPE, NAME_VALUE_LOOKUP, true, true>	self_t;
		typedef typename std::vector<VALUE_TYPE>						container_t;
		typedef const container_t&										value_assignment_t;

		typedef VALUE_TYPE												value_t;
		typedef value_t&												value_ref_t;
		typedef const value_t&											value_const_ref_t;

		typedef typename NAME_VALUE_LOOKUP::KeyCache					key_cache_t;

		TypedParam(BlockDescriptor& block_descriptor, const char* name, value_assignment_t value, ParamDescriptor::validation_func_t validate_func, S32 min_count, S32 max_count) 
		:	Param(block_descriptor.mCurrentBlockPtr),
			mValues(value),
			mLastParamGeneration(0)
		{
			mCachedKeys.resize(mValues.size());
			if (block_descriptor.mInitializationState == BlockDescriptor::INITIALIZING)
			{
				ParamDescriptor param_descriptor(block_descriptor.mCurrentBlockPtr->getHandleFromParam(this),
												&mergeWith,
												&deserializeParam,
												&serializeParam,
												validate_func,
												&inspectParam,
												min_count, max_count);
				BaseBlock::addParam(block_descriptor, param_descriptor, name);
			}
		} 

		bool isProvided() const { return Param::getProvided(); }

		value_ref_t operator[](S32 index) { return mValues[index]; }
		value_const_ref_t operator[](S32 index) const { return mValues[index]; }

		static bool deserializeParam(Param& param, Parser& parser, const Parser::name_stack_range_t& name_stack, S32 generation) 
		{ 
			self_t& typed_param = static_cast<self_t&>(param);
			if (generation != typed_param.mLastParamGeneration || typed_param.mValues.empty())
			{
				typed_param.mValues.push_back(value_t());
				typed_param.mCachedKeys.push_back(Data());
				typed_param.enclosingBlock().setLastChangedParam(param, true);
				typed_param.mLastParamGeneration = generation;
			}

			value_t& value = typed_param.mValues.back();

			// attempt to parse block...
			if(value.deserializeBlock(parser, name_stack))
			{
				typed_param.setProvided(true);
				return true;
			}

			if(!NAME_VALUE_LOOKUP::empty())
			{
				// try to parse a known named value
				std::string name;
				if (parser.readValue<std::string>(name))
				{
					// try to parse a per type named value
					if (NAME_VALUE_LOOKUP::get(name, value))
					{
						typed_param.mCachedKeys.back().setKey(name);
						typed_param.mCachedKeys.back().mKeyVersion = value.getLastChangeVersion();
						typed_param.enclosingBlock().setLastChangedParam(param, true);
						typed_param.setProvided(true);
						return true;
					}

				}
			}

			return false;
		}

		static void serializeParam(const Param& param, Parser& parser, Parser::name_stack_t& name_stack, const Param* diff_param)
		{
			const self_t& typed_param = static_cast<const self_t&>(param);
			if (!typed_param.isProvided() || name_stack.empty()) return;

			typename container_t::const_iterator it = typed_param.mValues.begin();
			for (typename std::vector<Data>::const_iterator key_it = typed_param.mCachedKeys.begin();
				it != typed_param.mValues.end();
				++key_it, ++it)
			{
				name_stack.back().second = parser.newParseGeneration();

				std::string key = key_it->getKey();
				if (!key.empty() && key_it->mKeyVersion == it->getLastChangeVersion())
				{
					if(!parser.writeValue<std::string>(key, name_stack))
					{
						return;
					}
				}
				// Not parsed via named values, write out value directly
				// NOTE: currently we don't worry about removing default values in Multiple
				else if (!it->serializeBlock(parser, name_stack, NULL))
				{
					return;
				}
			}
		}

		static void inspectParam(const Param& param, Parser& parser, Parser::name_stack_t& name_stack, S32 min_count, S32 max_count)
		{
			// I am a vector of blocks, so describe my contents recursively
			value_t().inspectBlock(parser, name_stack);
		}

		void set(value_assignment_t val, bool flag_as_provided = true)
		{
			mValues = val;
			mCachedKeys.clear();
			mCachedKeys.resize(mValues.size());
			setProvided(flag_as_provided);
			Param::enclosingBlock().setLastChangedParam(*this, flag_as_provided);
		}

		void setIfNotProvided(value_assignment_t val, bool flag_as_provided = true)
		{
			if (!isProvided())
			{
				set(val, flag_as_provided);
			}
		}

		value_ref_t add()
		{
			mValues.push_back(value_t());
			mCachedKeys.push_back(Data());
			setProvided(true);
			return mValues.back();
		}

		void add(value_const_ref_t item)
		{
			mValues.push_back(item);
			mCachedKeys.push_back(Data());
			setProvided(true);
		}

		// implicit conversion
		operator value_assignment_t() const { return self_t::get(); } 
		// explicit conversion
		value_assignment_t operator()() const { return get(); } 

		U32 numValidElements() const
		{
			U32 count = 0;
			for (typename container_t::const_iterator it = mValues.begin();
				it != mValues.end();
				++it)
			{
				if(it->validateBlock(true)) count++;
			}
			return count;
		}

	protected:
		value_assignment_t get() const
		{
			return mValues;
		}

		static bool mergeWith(Param& dst, const Param& src, bool overwrite)
		{
			const self_t& src_typed_param = static_cast<const self_t&>(src);
			self_t& dst_typed_param = static_cast<self_t&>(dst);

			if (src_typed_param.isProvided()
				&& (overwrite || !dst_typed_param.isProvided()))
			{
				dst_typed_param = src_typed_param;
				return true;
			}
			return false;
		}

		struct Data : public key_cache_t
		{
			S32 mKeyVersion;	// version of block for which key was last valid

			Data() : mKeyVersion(0) {}
		};

		container_t			mValues;
		std::vector<Data>	mCachedKeys;

		S32			mLastParamGeneration;
	};

	template <typename DERIVED_BLOCK>
	class Choice : public BaseBlock
	{
		typedef Choice<DERIVED_BLOCK> self_t;
		typedef Choice<DERIVED_BLOCK> enclosing_block_t;
		
		LOG_CLASS(self_t);
	public:
		// take all provided params from other and apply to self
		bool overwriteFrom(const self_t& other)
		{
			mCurChoice = other.mCurChoice;
			return BaseBlock::overwriteFromImpl(blockDescriptor(), other);
		}

		// take all provided params that are not already provided, and apply to self
		bool fillFrom(const self_t& other)
		{
			return false;
		}

		// clear out old choice when param has changed
		/*virtual*/ void setLastChangedParam(const Param& last_param, bool user_provided)
		{ 
			param_handle_t changed_param_handle = BaseBlock::getHandleFromParam(&last_param);
			// if we have a new choice...
			if (changed_param_handle != mCurChoice)
			{
				// clear provided flag on previous choice
				Param* previous_choice = BaseBlock::getParamFromHandle(mCurChoice);
				if (previous_choice) 
				{
					previous_choice->setProvided(false);
				}
				mCurChoice = changed_param_handle;
			}
			BaseBlock::setLastChangedParam(last_param, user_provided);
		}

	protected:
		Choice()
		:	mCurChoice(0)
		{
			BaseBlock::init(blockDescriptor(), BaseBlock::blockDescriptor(), sizeof(DERIVED_BLOCK));
		}

		// Alternatives are mutually exclusive wrt other Alternatives in the same block.  
		// One alternative in a block will always have isChosen() == true.
		// At most one alternative in a block will have isProvided() == true.
		template <typename T, typename NAME_VALUE_LOOKUP = TypeValues<T> >
		class Alternative : public TypedParam<T, NAME_VALUE_LOOKUP, false>
		{
		public:
			friend class Choice<DERIVED_BLOCK>;

			typedef Alternative<T, NAME_VALUE_LOOKUP>									self_t;
			typedef TypedParam<T, NAME_VALUE_LOOKUP, false, IsBaseBlock<T>::value>		super_t;
			typedef typename super_t::value_assignment_t								value_assignment_t;

			explicit Alternative(const char* name, value_assignment_t val = DefaultInitializer<T>::get())
			:	super_t(DERIVED_BLOCK::blockDescriptor(), name, val, NULL, 0, 1),
				mOriginalValue(val)
			{
				// assign initial choice to first declared option
				DERIVED_BLOCK* blockp = ((DERIVED_BLOCK*)DERIVED_BLOCK::blockDescriptor().mCurrentBlockPtr);
				if (DERIVED_BLOCK::blockDescriptor().mInitializationState == BlockDescriptor::INITIALIZING
					&& blockp->mCurChoice == 0)
				{
					blockp->mCurChoice = Param::enclosingBlock().getHandleFromParam(this);
				}
			}

			Alternative& operator=(value_assignment_t val)
			{
				super_t::set(val);
				return *this;
			}

			void operator()(typename super_t::value_assignment_t val) 
			{ 
				super_t::set(val);
			}

			operator value_assignment_t() const 
			{ 
				if (static_cast<enclosing_block_t&>(Param::enclosingBlock()).getCurrentChoice() == this)
				{
					return super_t::get(); 
				}
				return mOriginalValue;
			} 

			value_assignment_t operator()() const 
			{ 
				if (static_cast<enclosing_block_t&>(Param::enclosingBlock()).getCurrentChoice() == this)
				{
					return super_t::get(); 
				}
				return mOriginalValue;
			} 

			bool isChosen() const
			{
				return static_cast<enclosing_block_t&>(Param::enclosingBlock()).getCurrentChoice() == this;
			}
		
		private:
			T			mOriginalValue;
		};

	protected:
		static BlockDescriptor& blockDescriptor()
		{
			static BlockDescriptor sBlockDescriptor;
			return sBlockDescriptor;
		}

	private:
		param_handle_t	mCurChoice;

		const Param* getCurrentChoice() const
		{
			return BaseBlock::getParamFromHandle(mCurChoice);
		}
	};

	template <typename DERIVED_BLOCK, typename BASE_BLOCK = BaseBlock>
	class Block 
	:	public BASE_BLOCK
	{
		typedef Block<DERIVED_BLOCK, BASE_BLOCK> self_t;
		typedef Block<DERIVED_BLOCK, BASE_BLOCK> block_t;

	public:
		typedef BASE_BLOCK base_block_t;

		// take all provided params from other and apply to self
		bool overwriteFrom(const self_t& other)
		{
			return BaseBlock::overwriteFromImpl(blockDescriptor(), other);
		}

		// take all provided params that are not already provided, and apply to self
		bool fillFrom(const self_t& other)
		{
			return BaseBlock::fillFromImpl(blockDescriptor(), other);
		}
	protected:
		Block()
		{
			//#pragma message("Parsing LLInitParam::Block")
			BaseBlock::init(blockDescriptor(), BASE_BLOCK::blockDescriptor(), sizeof(DERIVED_BLOCK));
		}

		//
		// Nested classes for declaring parameters
		//
		template <typename T, typename NAME_VALUE_LOOKUP = TypeValues<T> >
		class Optional : public TypedParam<T, NAME_VALUE_LOOKUP, false>
		{
		public:
			typedef TypedParam<T, NAME_VALUE_LOOKUP, false, IsBaseBlock<T>::value>		super_t;
			typedef typename super_t::value_assignment_t								value_assignment_t;

			explicit Optional(const char* name = "", value_assignment_t val = DefaultInitializer<T>::get())
			:	super_t(DERIVED_BLOCK::blockDescriptor(), name, val, NULL, 0, 1)
			{
				//#pragma message("Parsing LLInitParam::Block::Optional")
			}

			Optional& operator=(value_assignment_t val)
			{
				set(val);
				return *this;
			}

			DERIVED_BLOCK& operator()(typename super_t::value_assignment_t val)
			{
				super_t::set(val);
				return static_cast<DERIVED_BLOCK&>(Param::enclosingBlock());
			}
			using super_t::operator();
		};

		template <typename T, typename NAME_VALUE_LOOKUP = TypeValues<T> >
		class Mandatory : public TypedParam<T, NAME_VALUE_LOOKUP, false>
		{
		public:
			typedef TypedParam<T, NAME_VALUE_LOOKUP, false, IsBaseBlock<T>::value>		super_t;
			typedef Mandatory<T, NAME_VALUE_LOOKUP>										self_t;
			typedef typename super_t::value_assignment_t								value_assignment_t;

			// mandatory parameters require a name to be parseable
			explicit Mandatory(const char* name = "", value_assignment_t val = DefaultInitializer<T>::get())
			:	super_t(DERIVED_BLOCK::blockDescriptor(), name, val, &validate, 1, 1)
			{}

			Mandatory& operator=(value_assignment_t val)
			{
				set(val);
				return *this;
			}

			DERIVED_BLOCK& operator()(typename super_t::value_assignment_t val)
			{
				super_t::set(val);
				return static_cast<DERIVED_BLOCK&>(Param::enclosingBlock());
			}
			using super_t::operator();

			static bool validate(const Param* p)
			{
				// valid only if provided
				return static_cast<const self_t*>(p)->isProvided();
			}

		};

		template <typename T, typename RANGE = BaseBlock::AnyAmount, typename NAME_VALUE_LOOKUP = TypeValues<T> >
		class Multiple : public TypedParam<T, NAME_VALUE_LOOKUP, true>
		{
		public:
			typedef TypedParam<T, NAME_VALUE_LOOKUP, true, IsBaseBlock<T>::value>	super_t;
			typedef Multiple<T, RANGE, NAME_VALUE_LOOKUP>							self_t;
			typedef typename super_t::container_t									container_t;
			typedef typename super_t::value_assignment_t							value_assignment_t;
			typedef typename container_t::iterator									iterator;
			typedef typename container_t::const_iterator							const_iterator;

			explicit Multiple(const char* name = "", value_assignment_t val = DefaultInitializer<container_t>::get())
			:	super_t(DERIVED_BLOCK::blockDescriptor(), name, val, &validate, RANGE::minCount(), RANGE::maxCount())
			{}

			using super_t::operator();

			Multiple& operator=(value_assignment_t val)
			{
				set(val);
				return *this;
			}
			
			DERIVED_BLOCK& operator()(typename super_t::value_assignment_t val)
			{
				super_t::set(val);
				return static_cast<DERIVED_BLOCK&>(Param::enclosingBlock());
			}

			static bool validate(const Param* paramp) 
			{
				U32 num_valid = ((super_t*)paramp)->numValidElements();
				return RANGE::minCount() <= num_valid && num_valid <= RANGE::maxCount();
			}
		};

		class Deprecated : public Param
		{
		public:
			explicit Deprecated(const char* name)
			:	Param(DERIVED_BLOCK::blockDescriptor().mCurrentBlockPtr)
			{
				BlockDescriptor& block_descriptor = DERIVED_BLOCK::blockDescriptor();
				if (block_descriptor.mInitializationState == BlockDescriptor::INITIALIZING)
				{
					ParamDescriptor param_descriptor(block_descriptor.mCurrentBlockPtr->getHandleFromParam(this),
													NULL,
													&deserializeParam,
													NULL,
													NULL,
													NULL, 
													0, S32_MAX);
					BaseBlock::addParam(block_descriptor, param_descriptor, name);
				}
			}
			
			static bool deserializeParam(Param& param, Parser& parser, const Parser::name_stack_range_t& name_stack, S32 generation)
			{
				if (name_stack.first == name_stack.second)
				{
					//std::string message = llformat("Deprecated value %s ignored", getName().c_str());
					//parser.parserWarning(message);
					return true;
				}

				return false;
			}
		};

		typedef Deprecated Ignored;

	protected:
		static BlockDescriptor& blockDescriptor()
		{
			static BlockDescriptor sBlockDescriptor;
			return sBlockDescriptor;
		}
	};
	
	template<typename T, typename DERIVED = TypedParam<T> >
	class BlockValue
	:	public Block<TypedParam<T, TypeValues<T>, false> >,
		public Param
	{
	public:
		typedef BlockValue<T>										self_t;
		typedef Block<TypedParam<T, TypeValues<T>, false> >			block_t;
		typedef const T&											value_const_ref_t;
		typedef value_const_ref_t									value_assignment_t;
		typedef typename TypeValues<T>::KeyCache					key_cache_t;

		BlockValue(BlockDescriptor& block_descriptor, const char* name, value_assignment_t value, ParamDescriptor::validation_func_t validate_func, S32 min_count, S32 max_count)
		:	Param(block_descriptor.mCurrentBlockPtr),
			mData(value)
		{
			if (block_descriptor.mInitializationState == BlockDescriptor::INITIALIZING)
			{
				ParamDescriptor param_descriptor(block_descriptor.mCurrentBlockPtr->getHandleFromParam(this),
												&mergeWith,
												&deserializeParam,
												&serializeParam,
												validate_func,
												&inspectParam,
												min_count, max_count);
				BaseBlock::addParam(block_descriptor, param_descriptor, name);
			}
		}

		// implicit conversion
		operator value_assignment_t() const { return get(); } 
		// explicit conversion
		value_assignment_t operator()() const { return get(); } 

		static bool deserializeParam(Param& param, Parser& parser, const Parser::name_stack_range_t& name_stack, S32 generation)
		{
			self_t& typed_param = static_cast<self_t&>(param);
			// type to apply parse direct value T
			if (name_stack.first == name_stack.second)
			{
				if(parser.readValue<T>(typed_param.mData.mValue))
				{
					typed_param.enclosingBlock().setLastChangedParam(param, true);
					typed_param.setProvided(true);
					typed_param.mData.mLastParamVersion = typed_param.BaseBlock::getLastChangeVersion();
					return true;
				}

				if(!TypeValues<T>::empty())
				{
					// try to parse a known named value
					std::string name;
					if (parser.readValue<std::string>(name))
					{
						// try to parse a per type named value
						if (TypeValues<T>::get(name, typed_param.mData.mValue))
						{
							typed_param.mData.setKey(name);
							typed_param.enclosingBlock().setLastChangedParam(param, true);
							typed_param.setProvided(true);
							typed_param.mData.mLastParamVersion = typed_param.BaseBlock::getLastChangeVersion();
							return true;
						}
					}
				}
			}

			// fall back on parsing block components for T
			// if we deserialized at least one component...
			if (typed_param.BaseBlock::deserializeBlock(parser, name_stack))
			{
				// ...our block is provided, and considered changed
				typed_param.enclosingBlock().setLastChangedParam(param, true);
				typed_param.setProvided(true);
				return true;
			}
			return false;
		}

		static void serializeParam(const Param& param, Parser& parser, Parser::name_stack_t& name_stack, const Param* diff_param)
		{
			const self_t& typed_param = static_cast<const self_t&>(param);
			
			if (!typed_param.isProvided()) return;
			
			std::string key = typed_param.mData.getKey();

			// first try to write out name of name/value pair
			if (!key.empty())
			{
				if (!diff_param || !ParamCompare<std::string>::equals(static_cast<const self_t*>(diff_param)->mData.getKey(), key))
				{
					if (!parser.writeValue<std::string>(key, name_stack))
					{
						return;
					}
				}
			}
			// then try to serialize value directly
			else if (!diff_param || !ParamCompare<T>::equals(typed_param.get(), (static_cast<const self_t*>(diff_param))->get()))	
            {
				
				if (parser.writeValue<T>(typed_param.mData.mValue, name_stack)) 
				{
					return;
				}

				//RN: *always* serialize provided components of BlockValue (don't pass diff_param on),
				// since these tend to be viewed as the constructor arguments for the value T.  It seems
				// cleaner to treat the uniqueness of a BlockValue according to the generated value, and
				// not the individual components.  This way <color red="0" green="1" blue="0"/> will not
				// be exported as <color green="1"/>, since it was probably the intent of the user to 
				// be specific about the RGB color values.  This also fixes an issue where we distinguish
				// between rect.left not being provided and rect.left being explicitly set to 0 (same as default)
				typed_param.BaseBlock::serializeBlock(parser, name_stack, NULL);
			}
		}

		static void inspectParam(const Param& param, Parser& parser, Parser::name_stack_t& name_stack, S32 min_count, S32 max_count)
		{
			// first, inspect with actual type...
			parser.inspectValue<T>(name_stack, min_count, max_count, NULL);
			if (TypeValues<T>::getPossibleValues())
			{
				//...then inspect with possible string values...
				parser.inspectValue<std::string>(name_stack, min_count, max_count, TypeValues<T>::getPossibleValues());
			}
			// then recursively inspect contents...
			const self_t& typed_param = static_cast<const self_t&>(param);
			typed_param.inspectBlock(parser, name_stack);
		}


		bool isProvided() const 
		{
			// either param value provided directly or block is sufficiently filled in
			// if cached value is stale, regenerate from params
			if (Param::getProvided() && mData.mLastParamVersion < BaseBlock::getLastChangeVersion())
			{
				if (block_t::validateBlock(true))
				{
					static_cast<const DERIVED*>(this)->setValueFromBlock();
					// clear stale keyword associated with old value
					mData.clearKey();
					mData.mLastParamVersion = BaseBlock::getLastChangeVersion();
					return true;
				}
				else
				{
					//block value incomplete, so not considered provided
					// will attempt to revalidate on next call to isProvided()
					return false;  
				}
			}
			// either no data provided, or we have a valid value in hand
			return Param::getProvided();
		}

		void set(value_assignment_t val, bool flag_as_provided = true)
		{
			Param::enclosingBlock().setLastChangedParam(*this, flag_as_provided);
			
			// set param version number to be up to date, so we ignore block contents
			mData.mLastParamVersion = BaseBlock::getLastChangeVersion();

			mData.mValue = val;
			mData.clearKey();
			setProvided(flag_as_provided);
			static_cast<DERIVED*>(this)->setBlockFromValue();
		}

		void setIfNotProvided(value_assignment_t val, bool flag_as_provided = true)
		{
			// don't override any user provided value
			if (!isProvided())
			{
				set(val, flag_as_provided);
			}
		}

 		// propagate change status up to enclosing block
		/*virtual*/ void setLastChangedParam(const Param& last_param, bool user_provided)
		{ 
			BaseBlock::setLastChangedParam(last_param, user_provided);
			Param::enclosingBlock().setLastChangedParam(*this, user_provided);
			if (user_provided)
			{
				setProvided(true);  // some component provided
			}
		}

	protected:
		value_assignment_t get() const
		{
			// if some parameters were provided, issue warnings on invalid blocks
			if (Param::getProvided() && (mData.mLastParamVersion < BaseBlock::getLastChangeVersion()))
			{
				// go ahead and issue warnings at this point if any param is invalid
				if(block_t::validateBlock(false))
				{
					static_cast<const DERIVED*>(this)->setValueFromBlock();
					mData.clearKey();
					mData.mLastParamVersion = BaseBlock::getLastChangeVersion();
				}
			}

			return mData.mValue;
		}

		// mutable to allow lazy updates on get
		struct Data : public key_cache_t
		{
			Data(const T& value) 
			:	mValue(value),
				mLastParamVersion(0)
			{}

			T		mValue;
			S32		mLastParamVersion;
		};

		mutable Data		mData;

	private:
		static bool mergeWith(Param& dst, const Param& src, bool overwrite)
		{
			const self_t& src_param = static_cast<const self_t&>(src);
			self_t& dst_typed_param = static_cast<self_t&>(dst);

			if (src_param.isProvided()
				&& (overwrite || !dst_typed_param.isProvided()))
			{
				// assign individual parameters
				if (overwrite)
				{
					dst_typed_param.BaseBlock::overwriteFromImpl(block_t::blockDescriptor(), src_param);
				}
				else
				{
					dst_typed_param.BaseBlock::fillFromImpl(block_t::blockDescriptor(), src_param);
				}
				// then copy actual value
				dst_typed_param.mData.mValue = src_param.get();
				dst_typed_param.mData.clearKey();
				dst_typed_param.setProvided(true);
				return true;
			}
			return false;
		}
	};

	template<> 
	struct ParamCompare<LLSD, false>
	{
		static bool equals(const LLSD &a, const LLSD &b);
	};
}

#endif // LL_LLPARAM_H
