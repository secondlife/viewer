/** 
 * @file llinitparam.h
 * @brief parameter block abstraction for creating complex objects and 
 * parsing construction parameters from xml and LLSD
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#ifndef LL_LLPARAM_H
#define LL_LLPARAM_H

#include <vector>
#include <boost/function.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

#include "llerror.h"

namespace LLTypeTags
{
	template <typename INNER_TYPE, int _SORT_ORDER>
	struct TypeTagBase
	{
		typedef void		is_tag_t;
		typedef INNER_TYPE	inner_t;
		static const int	SORT_ORDER=_SORT_ORDER;
	};

	template <int VAL1, int VAL2>
	struct GreaterThan
	{
		static const bool value = VAL1 > VAL2;
	};

	template<typename ITEM, typename REST, bool NEEDS_SWAP = GreaterThan<ITEM::SORT_ORDER, REST::SORT_ORDER>::value >
	struct Swap
	{
		typedef typename ITEM::template Cons<REST>::value_t value_t;
	};

	template<typename ITEM, typename REST>
	struct Swap<ITEM, REST, true>
	{
		typedef typename REST::template Cons<Swap<ITEM, typename REST::inner_t>::value_t>::value_t value_t;
	};

	template<typename T, typename SORTABLE = void>
	struct IsSortable
	{
		static const bool value = false;
	};

	template<typename T>
	struct IsSortable<T, typename T::is_tag_t>
	{
		static const bool value = true;
	};

	template<typename ITEM, typename REST, bool IS_REST_SORTABLE = IsSortable<REST>::value>
	struct InsertInto
	{
		typedef typename ITEM::template Cons<REST>::value_t value_t;
	};

	template<typename ITEM, typename REST>
	struct InsertInto <ITEM, REST, true>
	{
		typedef typename Swap<ITEM, REST>::value_t value_t;
	};

	template<typename T, bool SORTABLE = IsSortable<T>::value>
	struct Sorted
	{
		typedef T value_t;
	};

	template<typename T>
	struct Sorted <T, true>
	{
		typedef typename InsertInto<T, typename Sorted<typename T::inner_t>::value_t>::value_t value_t;
	};
}

namespace LLInitParam
{
	// used to indicate no matching value to a given name when parsing
	struct Flag{};

	template<typename T> const T& defaultValue() { static T value; return value; }

	// wraps comparison operator between any 2 values of the same type
	// specialize to handle cases where equality isn't defined well, or at all
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

	template<> 
	struct ParamCompare<LLSD, false>
	{
		static bool equals(const LLSD &a, const LLSD &b) { return false; }
	};

	template<>
	struct ParamCompare<Flag, false>
	{
		static bool equals(const Flag& a, const Flag& b) { return false; }
	};


	// helper functions and classes
	typedef ptrdiff_t param_handle_t;
	struct IS_A_BLOCK {};
	struct NOT_BLOCK {};

	// these templates allow us to distinguish between template parameters
	// that derive from BaseBlock and those that don't
	template<typename T, typename BLOCK_IDENTIFIER = void>
	struct IsBlock
	{
		typedef NOT_BLOCK value_t;
	};

	template<typename T>
	struct IsBlock<T, typename T::baseblock_base_class_t>
	{
		typedef IS_A_BLOCK value_t;
	};

	// ParamValue class directly manages the wrapped value
	// by holding on to a copy (scalar params)
	// or deriving from it (blocks)
	// has specializations for custom value behavior
	// and "tag" values like Lazy and Atomic
	template<typename T, typename VALUE_IS_BLOCK = typename IsBlock<T>::value_t>
	class ParamValue
	{
		typedef ParamValue<T, VALUE_IS_BLOCK>	self_t;

	public:
		typedef T	default_value_t;
		typedef T	value_t;

		ParamValue(): mValue() {}
		ParamValue(const default_value_t& other) : mValue(other) {}

		void setValue(const value_t& val)
		{
			mValue = val;
		}

		const value_t& getValue() const
		{
			return mValue;
		}

		T& getValue()
		{
			return mValue;
		}

	protected:
		T mValue;
	};

	template<typename T>
	class ParamValue<T, IS_A_BLOCK> 
	:	public T
	{
		typedef ParamValue<T, IS_A_BLOCK>	self_t;
	public:
		typedef T	default_value_t;
		typedef T	value_t;

		ParamValue() 
		:	T(),
			mValidated(false)
		{}

		ParamValue(const default_value_t& other)
			:	T(other),
			mValidated(false)
		{}

		void setValue(const value_t& val)
		{
			*this = val;
		}

		const value_t& getValue() const
		{
			return *this;
		}

		T& getValue()
		{
			return *this;
		}

	protected:
		mutable bool 	mValidated; // lazy validation flag
	};


	// empty default implementation of key cache
	// leverages empty base class optimization
	template <typename T>
	class TypeValues
	:	public ParamValue<typename LLTypeTags::Sorted<T>::value_t>
	{
	private:
		struct Inaccessable{};
	public:
		typedef std::map<std::string, T> value_name_map_t;
		typedef Inaccessable name_t;
		typedef TypeValues<T> type_value_t;
		typedef ParamValue<typename LLTypeTags::Sorted<T>::value_t>	param_value_t;
		typedef typename param_value_t::value_t	value_t;

		TypeValues(const typename param_value_t::value_t& val)
		:	param_value_t(val)
		{}

		void setValueName(const std::string& key) {}
		std::string getValueName() const { return ""; }
		std::string calcValueName(const value_t& value) const { return ""; }
		void clearValueName() const {}

		static bool getValueFromName(const std::string& name, value_t& value)
		{
			return false;
		}

		static bool valueNamesExist()
		{
			return false;
		}

		static std::vector<std::string>* getPossibleValues()
		{
			return NULL;
		}

		void assignNamedValue(const Inaccessable& name)
		{}

		operator const value_t&() const
		{
			return param_value_t::getValue();
		}

		const value_t& operator()() const
		{
			return param_value_t::getValue();
		}

		static value_name_map_t* getValueNames() {return NULL;}
	};

	// helper class to implement name value lookups
	// and caching of last used name
	template <typename T, typename DERIVED_TYPE = TypeValues<T>, bool IS_SPECIALIZED = true >
	class TypeValuesHelper
	:	public ParamValue<typename LLTypeTags::Sorted<T>::value_t>
	{
		typedef TypeValuesHelper<T, DERIVED_TYPE, IS_SPECIALIZED> self_t;
	public:
		typedef typename std::map<std::string, T> value_name_map_t;
		typedef std::string name_t;
		typedef self_t type_value_t;
		typedef ParamValue<typename LLTypeTags::Sorted<T>::value_t> param_value_t;
		typedef typename param_value_t::value_t	value_t;

		TypeValuesHelper(const typename param_value_t::value_t& val)
		:	param_value_t(val)
		{}

		//TODO: cache key by index to save on param block size
		void setValueName(const std::string& value_name) 
		{
			mValueName = value_name; 
		}

		std::string getValueName() const
		{ 
			return mValueName; 
		}

		std::string calcValueName(const value_t& value) const
		{
			value_name_map_t* map = getValueNames();
			for (typename value_name_map_t::iterator it = map->begin(), end_it = map->end();
				it != end_it;
				++it)
			{
				if (ParamCompare<T>::equals(it->second, value))
				{
					return it->first;
				}
			}

			return "";
		}

		void clearValueName() const
		{
			mValueName.clear();
		}

		static bool getValueFromName(const std::string& name, value_t& value)
		{
			value_name_map_t* map = getValueNames();
			typename value_name_map_t::iterator found_it = map->find(name);
			if (found_it == map->end()) return false;

			value = found_it->second;
			return true;
		}

		static bool valueNamesExist()
		{
			return !getValueNames()->empty();
		}
	
		static value_name_map_t* getValueNames()
		{
			static value_name_map_t sMap;
			static bool sInitialized = false;

			if (!sInitialized)
			{
				sInitialized = true;
				DERIVED_TYPE::declareValues();
			}
			return &sMap;
		}

		static std::vector<std::string>* getPossibleValues()
		{
			static std::vector<std::string> sValues;

			value_name_map_t* map = getValueNames();
			for (typename value_name_map_t::iterator it = map->begin(), end_it = map->end();
				 it != end_it;
				 ++it)
			{
				sValues.push_back(it->first);
			}
			return &sValues;
		}

		static void declare(const std::string& name, const value_t& value)
		{
			(*getValueNames())[name] = value;
		}

		void operator ()(const std::string& name)
		{
			*this = name;
		}

		void assignNamedValue(const std::string& name)
		{
			if (getValueFromName(name, param_value_t::getValue()))
			{
				setValueName(name);
			}
		}

		operator const value_t&() const
		{
			return param_value_t::getValue();
		}

		const value_t& operator()() const
		{
			return param_value_t::getValue();
		}

	protected:
		static void getName(const std::string& name, const value_t& value)
		{}

		mutable std::string	mValueName;
	};

	// string types can support custom named values, but need
	// to disambiguate in code between a string that is a named value
	// and a string that is a name
	template <typename DERIVED_TYPE>
	class TypeValuesHelper<std::string, DERIVED_TYPE, true>
	:	public TypeValuesHelper<std::string, DERIVED_TYPE, false>
	{
	public:
		typedef TypeValuesHelper<std::string, DERIVED_TYPE, true> self_t;
		typedef TypeValuesHelper<std::string, DERIVED_TYPE, false> base_t;
		typedef std::string value_t;
		typedef std::string name_t;
		typedef self_t type_value_t;

		TypeValuesHelper(const std::string& val)
		:	TypeValuesHelper(val)
		{}

		void operator ()(const std::string& name)
		{
			*this = name;
		}

		self_t& operator =(const std::string& name)
		{
			if (base_t::getValueFromName(name, ParamValue<std::string>::getValue()))
			{
				base_t::setValueName(name);
			}
			else
			{
				ParamValue<std::string>::setValue(name);
			}
			return *this;
		}
		
		operator const value_t&() const
		{
			return ParamValue<std::string>::getValue();
		}

		const value_t& operator()() const
		{
			return ParamValue<std::string>::getValue();
		}

	};

	// parser base class with mechanisms for registering readers/writers/inspectors of different types
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

		typedef std::vector<std::pair<std::string, bool> >					name_stack_t;
		typedef std::pair<name_stack_t::iterator, name_stack_t::iterator>	name_stack_range_t;
		typedef std::vector<std::string>									possible_values_t;

		typedef bool (*parser_read_func_t)(Parser& parser, void* output);
		typedef bool (*parser_write_func_t)(Parser& parser, const void*, name_stack_t&);
		typedef boost::function<void (name_stack_t&, S32, S32, const possible_values_t*)>	parser_inspect_func_t;

		typedef std::map<const std::type_info*, parser_read_func_t, CompareTypeID>		parser_read_func_map_t;
		typedef std::map<const std::type_info*, parser_write_func_t, CompareTypeID>		parser_write_func_map_t;
		typedef std::map<const std::type_info*, parser_inspect_func_t, CompareTypeID>	parser_inspect_func_map_t;

		Parser(parser_read_func_map_t& read_map, parser_write_func_map_t& write_map, parser_inspect_func_map_t& inspect_map)
		:	mParseSilently(false),
			mParserReadFuncs(&read_map),
			mParserWriteFuncs(&write_map),
			mParserInspectFuncs(&inspect_map)
		{}
		virtual ~Parser();

		template <typename T> bool readValue(T& param)
	    {
		    parser_read_func_map_t::iterator found_it = mParserReadFuncs->find(&typeid(T));
		    if (found_it != mParserReadFuncs->end())
		    {
			    return found_it->second(*this, (void*)&param);
		    }
		    return false;
	    }

		template <typename T> bool writeValue(const T& param, name_stack_t& name_stack)
		{
		    parser_write_func_map_t::iterator found_it = mParserWriteFuncs->find(&typeid(T));
		    if (found_it != mParserWriteFuncs->end())
		    {
			    return found_it->second(*this, (const void*)&param, name_stack);
		    }
		    return false;
		}

		// dispatch inspection to registered inspection functions, for each parameter in a param block
		template <typename T> bool inspectValue(name_stack_t& name_stack, S32 min_count, S32 max_count, const possible_values_t* possible_values)
		{
		    parser_inspect_func_map_t::iterator found_it = mParserInspectFuncs->find(&typeid(T));
		    if (found_it != mParserInspectFuncs->end())
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

	protected:
		template <typename T>
		void registerParserFuncs(parser_read_func_t read_func, parser_write_func_t write_func = NULL)
		{
			mParserReadFuncs->insert(std::make_pair(&typeid(T), read_func));
			mParserWriteFuncs->insert(std::make_pair(&typeid(T), write_func));
		}

		template <typename T>
		void registerInspectFunc(parser_inspect_func_t inspect_func)
		{
			mParserInspectFuncs->insert(std::make_pair(&typeid(T), inspect_func));
		}

		bool				mParseSilently;

	private:
		parser_read_func_map_t*		mParserReadFuncs;
		parser_write_func_map_t*	mParserWriteFuncs;
		parser_inspect_func_map_t*	mParserInspectFuncs;
	};

	class Param;

	// various callbacks and constraints associated with an individual param
	struct ParamDescriptor
	{
		struct UserData
		{
			virtual ~UserData() {}
		};

		typedef bool(*merge_func_t)(Param&, const Param&, bool);
		typedef bool(*deserialize_func_t)(Param&, Parser&, const Parser::name_stack_range_t&, bool);
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
						S32 max_count);

		ParamDescriptor();
		~ParamDescriptor();

		param_handle_t		mParamHandle;
		merge_func_t		mMergeFunc;
		deserialize_func_t	mDeserializeFunc;
		serialize_func_t	mSerializeFunc;
		inspect_func_t		mInspectFunc;
		validation_func_t	mValidationFunc;
		S32					mMinCount;
		S32					mMaxCount;
		S32					mNumRefs;
		UserData*			mUserData;
	};

	typedef boost::shared_ptr<ParamDescriptor> ParamDescriptorPtr;

	// each derived Block class keeps a static data structure maintaining offsets to various params
	class BlockDescriptor
	{
	public:
		BlockDescriptor();

		typedef enum e_initialization_state
		{
			UNINITIALIZED,
			INITIALIZING,
			INITIALIZED
		} EInitializationState;

		void aggregateBlockData(BlockDescriptor& src_block_data);
		void addParam(ParamDescriptorPtr param, const char* name);

		typedef boost::unordered_map<const std::string, ParamDescriptorPtr>						param_map_t; 
		typedef std::vector<ParamDescriptorPtr>													param_list_t; 
		typedef std::list<ParamDescriptorPtr>													all_params_list_t;
		typedef std::vector<std::pair<param_handle_t, ParamDescriptor::validation_func_t> >		param_validation_list_t;

		param_map_t						mNamedParams;			// parameters with associated names
		param_list_t					mUnnamedParams;			// parameters with_out_ associated names
		param_validation_list_t			mValidationList;		// parameters that must be validated
		all_params_list_t				mAllParams;				// all parameters, owns descriptors
		size_t							mMaxParamOffset;
		EInitializationState			mInitializationState;	// whether or not static block data has been initialized
		class BaseBlock*				mCurrentBlockPtr;		// pointer to block currently being constructed
	};

		//TODO: implement in terms of owned_ptr
		template<typename T>
	class LazyValue
		{
		public:
		LazyValue()
				: mPtr(NULL)
			{}

		~LazyValue()
			{
				delete mPtr;
			}

		LazyValue(const T& value)
			{
			mPtr = new T(value);
		}

		LazyValue(const LazyValue& other)
		:	mPtr(NULL)
				{
			*this = other;
				}

		LazyValue& operator = (const LazyValue& other)
		{
			if (!other.mPtr)
				{
				delete mPtr;
					mPtr = NULL;
				}
			else
			{
				if (!mPtr)
				{
					mPtr = new T(*other.mPtr);
				}
				else
				{
					*mPtr = *(other.mPtr);
				}
				}
				return *this;
			}

		bool operator==(const LazyValue& other) const
		{
			if (empty() || other.empty()) return false;
			return *mPtr == *other.mPtr;
		}

			bool empty() const
			{
				return mPtr == NULL;
			}

			void set(const T& other)
			{
			if (!mPtr)
			{
				mPtr = new T(other);
			}
			else
			{
				*mPtr = other;
			}
		}

			const T& get() const
			{
			return *ensureInstance();
			}

			T& get()
			{
			return *ensureInstance();
		}

		operator const T&() const
		{ 
			return get(); 
			}

		private:
			// lazily allocate an instance of T
			T* ensureInstance() const
			{
				if (mPtr == NULL)
				{
					mPtr = new T();
				}
				return mPtr;
			}

		private:

			mutable T* mPtr;
		};

	// root class of all parameter blocks

	class BaseBlock
	{
	public:
		// lift block tags into baseblock namespace so derived classes do not need to qualify them
		typedef LLInitParam::IS_A_BLOCK IS_A_BLOCK;
		typedef LLInitParam::NOT_BLOCK NOT_A_BLOCK;

		template<typename T>
		struct Sequential : public LLTypeTags::TypeTagBase<T, 2>
		{
			template <typename S> struct Cons { typedef Sequential<ParamValue<S> > value_t; };
			template <typename S> struct Cons<Sequential<S> > { typedef Sequential<S> value_t; };
		};

		template<typename T>
		struct Atomic : public LLTypeTags::TypeTagBase<T, 1>
		{
			template <typename S> struct Cons { typedef Atomic<ParamValue<S> > value_t; };
			template <typename S> struct Cons<Atomic<S> > { typedef Atomic<S> value_t; };
		};

		template<typename T, typename BLOCK_T = typename IsBlock<T>::value_t >
		struct Lazy : public LLTypeTags::TypeTagBase<T, 0>
		{
			template <typename S> struct Cons
			{
				typedef Lazy<ParamValue<S, BLOCK_T>, BLOCK_T> value_t;
			};
			template <typename S> struct Cons<Lazy<S, IS_A_BLOCK> >
			{
				typedef Lazy<S, IS_A_BLOCK> value_t;
			};
			template <typename S> struct Cons<Lazy<S, NOT_A_BLOCK> >
			{
				typedef Lazy<S, BLOCK_T> value_t;
			};
		};

		// "Multiple" constraint types, put here in root class to avoid ambiguity during use
		struct AnyAmount
		{
			enum { minCount = 0 };
			enum { maxCount = U32_MAX };
		};

		template<U32 MIN_AMOUNT>
		struct AtLeast
		{
			enum { minCount = MIN_AMOUNT };
			enum { maxCount = U32_MAX };
		};

		template<U32 MAX_AMOUNT>
		struct AtMost
		{
			enum { minCount = 0 };
			enum { maxCount = MAX_AMOUNT };
		};

		template<U32 MIN_AMOUNT, U32 MAX_AMOUNT>
		struct Between
		{
			enum { minCount = MIN_AMOUNT };
			enum { maxCount = MAX_AMOUNT };
		};

		template<U32 EXACT_COUNT>
		struct Exactly
		{
			enum { minCount = EXACT_COUNT };
			enum { maxCount = EXACT_COUNT };
		};

		// this typedef identifies derived classes as being blocks
		typedef void baseblock_base_class_t;
		LOG_CLASS(BaseBlock);
		friend class Param;

		virtual ~BaseBlock() {}
		bool submitValue(Parser::name_stack_t& name_stack, Parser& p, bool silent=false);

		param_handle_t getHandleFromParam(const Param* param) const;
		bool validateBlock(bool emit_errors = true) const;

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
		virtual void paramChanged(const Param& changed_param, bool user_provided) {}

		bool deserializeBlock(Parser& p, Parser::name_stack_range_t name_stack_range, bool new_name);
		void serializeBlock(Parser& p, Parser::name_stack_t& name_stack, const BaseBlock* diff_block = NULL) const;
		bool inspectBlock(Parser& p, Parser::name_stack_t name_stack = Parser::name_stack_t(), S32 min_count = 0, S32 max_count = S32_MAX) const;

		virtual const BlockDescriptor& mostDerivedBlockDescriptor() const { return getBlockDescriptor(); }
		virtual BlockDescriptor& mostDerivedBlockDescriptor() { return getBlockDescriptor(); }

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

		ParamDescriptorPtr findParamDescriptor(const Param& param);

		// take all provided params from other and apply to self
		bool mergeBlock(BlockDescriptor& block_data, const BaseBlock& other, bool overwrite);

		static BlockDescriptor& getBlockDescriptor()
		{
			static BlockDescriptor sBlockDescriptor;
			return sBlockDescriptor;
		}

	protected:
		void init(BlockDescriptor& descriptor, BlockDescriptor& base_descriptor, size_t block_size);


		bool mergeBlockParam(bool source_provided, bool dst_provided, BlockDescriptor& block_data, const BaseBlock& source, bool overwrite)
		{
			return mergeBlock(block_data, source, overwrite);
		}

	private:
		const std::string& getParamName(const BlockDescriptor& block_data, const Param* paramp) const;
	};

	class Param
	{
	public:
		void setProvided(bool is_provided = true)
		{
			mIsProvided = is_provided;
			enclosingBlock().paramChanged(*this, is_provided);
		}

		Param& operator =(const Param& other)
		{
			mIsProvided = other.mIsProvided;
			// don't change mEnclosingblockoffset
			return *this;
		}
	protected:

		bool anyProvided() const { return mIsProvided; }

		Param(BaseBlock* enclosing_block);

		// store pointer to enclosing block as offset to reduce space and allow for quick copying
		BaseBlock& enclosingBlock() const
		{ 
			const U8* my_addr = reinterpret_cast<const U8*>(this);
			// get address of enclosing BLOCK class using stored offset to enclosing BaseBlock class
			return *const_cast<BaseBlock*>
				(reinterpret_cast<const BaseBlock*>
					(my_addr - (ptrdiff_t)getEnclosingBlockOffset()));
		}

		U32 getEnclosingBlockOffset() const
		{
			return ((U32)mEnclosingBlockOffsetHigh << 16) | (U32)mEnclosingBlockOffsetLow;
		}

	private:
		friend class BaseBlock;

		//24 bits for member offset field and 1 bit for provided flag
		U16		mEnclosingBlockOffsetLow;
		U8		mEnclosingBlockOffsetHigh:7;
		U8		mIsProvided:1;

	};

	template<typename T, typename NAME_VALUE_LOOKUP = TypeValues<T> >
	struct ParamIterator
	{
		typedef typename std::vector<typename NAME_VALUE_LOOKUP::type_value_t >::const_iterator	const_iterator;
		typedef typename std::vector<typename NAME_VALUE_LOOKUP::type_value_t >::iterator			iterator;
	};

	// wrapper for parameter with a known type
	// specialized to handle 4 cases:
	// simple "scalar" value
	// parameter that is itself a block
	// multiple scalar values, stored in a vector
	// multiple blocks, stored in a vector
	template<typename	T,
			typename	NAME_VALUE_LOOKUP = TypeValues<T>,
			bool		HAS_MULTIPLE_VALUES = false,
			typename	VALUE_IS_BLOCK = typename IsBlock<ParamValue<typename LLTypeTags::Sorted<T>::value_t> >::value_t>
	class TypedParam 
	:	public Param, 
		public NAME_VALUE_LOOKUP::type_value_t
	{
	protected:
		typedef	TypedParam<T, NAME_VALUE_LOOKUP, HAS_MULTIPLE_VALUES, VALUE_IS_BLOCK>	self_t;
		typedef ParamValue<typename LLTypeTags::Sorted<T>::value_t>						param_value_t;
		typedef typename param_value_t::default_value_t									default_value_t;
		typedef typename NAME_VALUE_LOOKUP::type_value_t								named_value_t;
	public:
		typedef typename param_value_t::value_t											value_t;

		using named_value_t::operator();

		TypedParam(BlockDescriptor& block_descriptor, const char* name, const default_value_t& value, ParamDescriptor::validation_func_t validate_func, S32 min_count, S32 max_count)
		:	Param(block_descriptor.mCurrentBlockPtr),
			named_value_t(value)
		{
			if (LL_UNLIKELY(block_descriptor.mInitializationState == BlockDescriptor::INITIALIZING))
			{
				init(block_descriptor, validate_func, min_count, max_count, name);
			}
		} 

		bool isProvided() const { return Param::anyProvided(); }

		static bool deserializeParam(Param& param, Parser& parser, const Parser::name_stack_range_t& name_stack_range, bool new_name)
		{ 
			self_t& typed_param = static_cast<self_t&>(param);
			// no further names in stack, attempt to parse value now
			if (name_stack_range.first == name_stack_range.second)
			{
				if (parser.readValue(typed_param.getValue()))
				{
					typed_param.clearValueName();
					typed_param.setProvided();
					return true;
				}
				
				// try to parse a known named value
				if(named_value_t::valueNamesExist())
				{
					// try to parse a known named value
					std::string name;
					if (parser.readValue(name))
					{
						// try to parse a per type named value

						if (named_value_t::getValueFromName(name, typed_param.getValue()))
						{
							typed_param.setValueName(name);
							typed_param.setProvided();
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
				name_stack.back().second = true;
			}

			std::string key = typed_param.getValueName();

			// first try to write out name of name/value pair

			if (!key.empty())
			{
				if (!diff_param || !ParamCompare<std::string>::equals(static_cast<const self_t*>(diff_param)->getValueName(), key))
				{
					parser.writeValue(key, name_stack);
				}
			}
			// then try to serialize value directly
			else if (!diff_param || !ParamCompare<T>::equals(typed_param.getValue(), static_cast<const self_t*>(diff_param)->getValue()))
			{
				if (!parser.writeValue(typed_param.getValue(), name_stack)) 
				{
					std::string calculated_key = typed_param.calcValueName(typed_param.getValue());
					if (calculated_key.size() 
						&& (!diff_param 
							|| !ParamCompare<std::string>::equals(static_cast<const self_t*>(diff_param)->getValueName(), calculated_key)))
					{
						parser.writeValue(calculated_key, name_stack);
					}
				}
			}
		}

		static void inspectParam(const Param& param, Parser& parser, Parser::name_stack_t& name_stack, S32 min_count, S32 max_count)
		{
			// tell parser about our actual type
			parser.inspectValue<T>(name_stack, min_count, max_count, NULL);
			// then tell it about string-based alternatives ("red", "blue", etc. for LLColor4)
			if (named_value_t::getPossibleValues())
			{
				parser.inspectValue<std::string>(name_stack, min_count, max_count, named_value_t::getPossibleValues());
			}
		}

		void set(const value_t& val, bool flag_as_provided = true)
		{
			named_value_t::clearValueName();
			setValue(val);
			setProvided(flag_as_provided);
		}

		self_t& operator =(const typename named_value_t::name_t& name)
		{
			named_value_t::assignNamedValue(name);
			return *this;
		}

	protected:

		self_t& operator =(const self_t& other)
		{
			param_value_t::operator =(other);
			Param::operator =(other);
			return *this;
		}

		static bool mergeWith(Param& dst, const Param& src, bool overwrite)
		{
			const self_t& src_typed_param = static_cast<const self_t&>(src);
			self_t& dst_typed_param = static_cast<self_t&>(dst);

			if (src_typed_param.isProvided()
				&& (overwrite || !dst_typed_param.isProvided()))
			{
				dst_typed_param.set(src_typed_param.getValue());
				return true;
			}
			return false;
		}
	private:
		void init( BlockDescriptor &block_descriptor, ParamDescriptor::validation_func_t validate_func, S32 min_count, S32 max_count, const char* name ) 
		{
			ParamDescriptorPtr param_descriptor = ParamDescriptorPtr(new ParamDescriptor(
				block_descriptor.mCurrentBlockPtr->getHandleFromParam(this),
				&mergeWith,
				&deserializeParam,
				&serializeParam,
				validate_func,
				&inspectParam,
				min_count, max_count));
			block_descriptor.addParam(param_descriptor, name);
		}
	};

	// parameter that is a block
	template <typename T, typename NAME_VALUE_LOOKUP>
	class TypedParam<T, NAME_VALUE_LOOKUP, false, IS_A_BLOCK> 
	:	public Param,
		public NAME_VALUE_LOOKUP::type_value_t
	{
	protected:
		typedef ParamValue<typename LLTypeTags::Sorted<T>::value_t>	param_value_t;
		typedef typename param_value_t::default_value_t				default_value_t;
		typedef TypedParam<T, NAME_VALUE_LOOKUP, false, IS_A_BLOCK>	self_t;
		typedef typename NAME_VALUE_LOOKUP::type_value_t			named_value_t;
	public:
		using named_value_t::operator();
		typedef typename param_value_t::value_t						value_t;

		TypedParam(BlockDescriptor& block_descriptor, const char* name, const default_value_t& value, ParamDescriptor::validation_func_t validate_func, S32 min_count, S32 max_count)
		:	Param(block_descriptor.mCurrentBlockPtr),
			named_value_t(value)
		{
			if (LL_UNLIKELY(block_descriptor.mInitializationState == BlockDescriptor::INITIALIZING))
			{
				init(block_descriptor, validate_func, min_count, max_count, name);
			}
		}

		static bool deserializeParam(Param& param, Parser& parser, const Parser::name_stack_range_t& name_stack_range, bool new_name)
		{ 
			self_t& typed_param = static_cast<self_t&>(param);
			// attempt to parse block...
			if(typed_param.deserializeBlock(parser, name_stack_range, new_name))
			{
				typed_param.clearValueName();
				typed_param.setProvided();
				return true;
			}

			if(named_value_t::valueNamesExist())
			{
				// try to parse a known named value
				std::string name;
				if (parser.readValue(name))
				{
					// try to parse a per type named value
					if (named_value_t::getValueFromName(name, typed_param.getValue()))
					{
						typed_param.setValueName(name);
						typed_param.setProvided();
						return true;
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
				name_stack.back().second = true;
			}

			std::string key = typed_param.getValueName();
			if (!key.empty())
			{
				if (!diff_param || !ParamCompare<std::string>::equals(static_cast<const self_t*>(diff_param)->getValueName(), key))
				{
					parser.writeValue(key, name_stack);
				}
			}
			else
			{
				typed_param.serializeBlock(parser, name_stack, static_cast<const self_t*>(diff_param));
			}
		}

		static void inspectParam(const Param& param, Parser& parser, Parser::name_stack_t& name_stack, S32 min_count, S32 max_count)
		{
			const self_t& typed_param = static_cast<const self_t&>(param);

			// tell parser about our actual type
			parser.inspectValue<value_t>(name_stack, min_count, max_count, NULL);
			// then tell it about string-based alternatives ("red", "blue", etc. for LLColor4)
			if (named_value_t::getPossibleValues())
			{
				parser.inspectValue<std::string>(name_stack, min_count, max_count, named_value_t::getPossibleValues());
			}

			typed_param.inspectBlock(parser, name_stack, min_count, max_count);
		}

		// a param-that-is-a-block is provided when the user has set one of its child params
		// *and* the block as a whole validates
		bool isProvided() const 
		{ 
			// only validate block when it hasn't already passed validation with current data
			if (Param::anyProvided() && !param_value_t::mValidated)
			{
				// a sub-block is "provided" when it has been filled in enough to be valid
				param_value_t::mValidated = param_value_t::validateBlock(false);
			}
			return Param::anyProvided() && param_value_t::mValidated;
		}

		// assign block contents to this param-that-is-a-block
		void set(const value_t& val, bool flag_as_provided = true)
		{
			setValue(val);
			named_value_t::clearValueName();
			// force revalidation of block
			// next call to isProvided() will update provision status based on validity
			param_value_t::mValidated = false;
			setProvided(flag_as_provided);
		}

		self_t& operator =(const typename named_value_t::name_t& name)
		{
			named_value_t::assignNamedValue(name);
			return *this;
		}

		// propagate changed status up to enclosing block
		/*virtual*/ void paramChanged(const Param& changed_param, bool user_provided)
		{ 
			param_value_t::paramChanged(changed_param, user_provided);

			if (user_provided)
			{
				// a child param has been explicitly changed
				// so *some* aspect of this block is now provided
				param_value_t::mValidated = false;
				setProvided();
				named_value_t::clearValueName();
			}
			else
			{
				Param::enclosingBlock().paramChanged(*this, user_provided);
			}
		}

	protected:

		self_t& operator =(const self_t& other)
		{
			param_value_t::operator =(other);
			Param::operator =(other);
			return *this;
		}

		static bool mergeWith(Param& dst, const Param& src, bool overwrite)
		{
			const self_t& src_typed_param = static_cast<const self_t&>(src);
			self_t& dst_typed_param = static_cast<self_t&>(dst);

			if (src_typed_param.anyProvided())
			{
				if (dst_typed_param.mergeBlockParam(src_typed_param.isProvided(), dst_typed_param.isProvided(), param_value_t::getBlockDescriptor(), src_typed_param, overwrite))
				{
					dst_typed_param.clearValueName();
					dst_typed_param.setProvided(true);
					return true;
				}
			}
			return false;
		}

	private:
		void init( BlockDescriptor &block_descriptor, ParamDescriptor::validation_func_t validate_func, S32 min_count, S32 max_count, const char* name ) 
		{
			ParamDescriptorPtr param_descriptor = ParamDescriptorPtr(new ParamDescriptor(
				block_descriptor.mCurrentBlockPtr->getHandleFromParam(this),
				&mergeWith,
				&deserializeParam,
				&serializeParam,
				validate_func, 
				&inspectParam,
				min_count, max_count));
			block_descriptor.addParam(param_descriptor, name);
		}
	};

	// container of non-block parameters
	template <typename VALUE_TYPE, typename NAME_VALUE_LOOKUP>
	class TypedParam<VALUE_TYPE, NAME_VALUE_LOOKUP, true, NOT_BLOCK> 
	:	public Param
	{
	protected:
		typedef TypedParam<VALUE_TYPE, NAME_VALUE_LOOKUP, true, NOT_BLOCK>		self_t;
		typedef ParamValue<typename LLTypeTags::Sorted<VALUE_TYPE>::value_t>	param_value_t;
		typedef typename std::vector<typename NAME_VALUE_LOOKUP::type_value_t>	container_t;
		typedef container_t														default_value_t;
		typedef typename NAME_VALUE_LOOKUP::type_value_t						named_value_t;
		
	public:
		typedef typename param_value_t::value_t								value_t;
		
		TypedParam(BlockDescriptor& block_descriptor, const char* name, const default_value_t& value, ParamDescriptor::validation_func_t validate_func, S32 min_count, S32 max_count)
		:	Param(block_descriptor.mCurrentBlockPtr)
		{
			std::copy(value.begin(), value.end(), std::back_inserter(mValues));

			if (LL_UNLIKELY(block_descriptor.mInitializationState == BlockDescriptor::INITIALIZING))
			{
				init(block_descriptor, validate_func, min_count, max_count, name);

			}
		} 

		bool isProvided() const { return Param::anyProvided(); }

		static bool deserializeParam(Param& param, Parser& parser, const Parser::name_stack_range_t& name_stack_range, bool new_name)
		{ 
			self_t& typed_param = static_cast<self_t&>(param);
			value_t value;
			// no further names in stack, attempt to parse value now
			if (name_stack_range.first == name_stack_range.second)
			{
				// attempt to read value directly
				if (parser.readValue(value))
				{
					typed_param.add(value);
					return true;
				}
				
				// try to parse a known named value
				if(named_value_t::valueNamesExist())
				{
					// try to parse a known named value
					std::string name;
					if (parser.readValue(name))
					{
						// try to parse a per type named value
						if (named_value_t::getValueFromName(name, value))
						{
							typed_param.add(value);
							typed_param.mValues.back().setValueName(name);
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

			for (const_iterator it = typed_param.mValues.begin(), end_it = typed_param.mValues.end();
				it != end_it;
				++it)
			{
				std::string key = it->getValueName();
				name_stack.back().second = true;

				if(key.empty())
				// not parsed via name values, write out value directly
				{
					bool value_written = parser.writeValue(*it, name_stack);
					if (!value_written)
					{
						std::string calculated_key = it->calcValueName(it->getValue());
						if (!parser.writeValue(calculated_key, name_stack))
						{
							break;
						}
					}
				}
				else 
				{
					if(!parser.writeValue(key, name_stack))
					{
						break;
					}
				}
			}
		}

		static void inspectParam(const Param& param, Parser& parser, Parser::name_stack_t& name_stack, S32 min_count, S32 max_count)
		{
			parser.inspectValue<VALUE_TYPE>(name_stack, min_count, max_count, NULL);
			if (named_value_t::getPossibleValues())
			{
				parser.inspectValue<std::string>(name_stack, min_count, max_count, named_value_t::getPossibleValues());
			}
		}

		void set(const container_t& val, bool flag_as_provided = true)
		{
			mValues = val;
			setProvided(flag_as_provided);
		}

		param_value_t& add()
		{
			mValues.push_back(value_t());
			Param::setProvided();
			return mValues.back();
		}

		self_t& add(const value_t& item)
		{
			mValues.push_back(item);
			setProvided();
			return *this;
		}

		self_t& add(const typename named_value_t::name_t& name)
		{
			value_t value;

			// try to parse a per type named value
			if (named_value_t::getValueFromName(name, value))
			{
				add(value);
				mValues.back().setValueName(name);
			}

			return *this;
		}

		// implicit conversion
		operator const container_t&() const { return mValues; } 
		// explicit conversion		
		const container_t& operator()() const { return mValues; }

		typedef typename container_t::iterator iterator;
		typedef typename container_t::const_iterator const_iterator;
		iterator begin() { return mValues.begin(); }
		iterator end() { return mValues.end(); }
		const_iterator begin() const { return mValues.begin(); }
		const_iterator end() const { return mValues.end(); }
		bool empty() const { return mValues.empty(); }
		size_t size() const { return mValues.size(); }

		U32 numValidElements() const
		{
			return mValues.size();
		}

	protected:
		static bool mergeWith(Param& dst, const Param& src, bool overwrite)
		{
			const self_t& src_typed_param = static_cast<const self_t&>(src);
			self_t& dst_typed_param = static_cast<self_t&>(dst);

			if (overwrite)
			{
				std::copy(src_typed_param.begin(), src_typed_param.end(), std::back_inserter(dst_typed_param.mValues));
			}
			else
			{
				container_t new_values(src_typed_param.mValues);
				std::copy(dst_typed_param.begin(), dst_typed_param.end(), std::back_inserter(new_values));
				std::swap(dst_typed_param.mValues, new_values);
			}

			if (src_typed_param.begin() != src_typed_param.end())
			{
				dst_typed_param.setProvided();
			}
			return true;
		}

		container_t		mValues;

	private:
		void init( BlockDescriptor &block_descriptor, ParamDescriptor::validation_func_t validate_func, S32 min_count, S32 max_count, const char* name ) 
		{
			ParamDescriptorPtr param_descriptor = ParamDescriptorPtr(new ParamDescriptor(
				block_descriptor.mCurrentBlockPtr->getHandleFromParam(this),
				&mergeWith,
				&deserializeParam,
				&serializeParam,
				validate_func,
				&inspectParam,
				min_count, max_count));
			block_descriptor.addParam(param_descriptor, name);
		}
	};

	// container of block parameters
	template <typename VALUE_TYPE, typename NAME_VALUE_LOOKUP>
	class TypedParam<VALUE_TYPE, NAME_VALUE_LOOKUP, true, IS_A_BLOCK> 
	:	public Param
	{
	protected:
		typedef TypedParam<VALUE_TYPE, NAME_VALUE_LOOKUP, true, IS_A_BLOCK>		self_t;
		typedef ParamValue<typename LLTypeTags::Sorted<VALUE_TYPE>::value_t>	param_value_t;
		typedef typename std::vector<typename NAME_VALUE_LOOKUP::type_value_t>	container_t;
		typedef typename NAME_VALUE_LOOKUP::type_value_t						named_value_t;
		typedef container_t														default_value_t;
		typedef typename container_t::iterator									iterator;
		typedef typename container_t::const_iterator							const_iterator;
	public:
		typedef typename param_value_t::value_t							value_t;

		TypedParam(BlockDescriptor& block_descriptor, const char* name, const default_value_t& value, ParamDescriptor::validation_func_t validate_func, S32 min_count, S32 max_count)
		:	Param(block_descriptor.mCurrentBlockPtr)
		{
			std::copy(value.begin(), value.end(), back_inserter(mValues));

			if (LL_UNLIKELY(block_descriptor.mInitializationState == BlockDescriptor::INITIALIZING))
			{
				init(block_descriptor, validate_func, min_count, max_count, name);
			}
		} 

		bool isProvided() const { return Param::anyProvided(); }

		static bool deserializeParam(Param& param, Parser& parser, const Parser::name_stack_range_t& name_stack_range, bool new_name) 
		{ 
			self_t& typed_param = static_cast<self_t&>(param);
			bool new_value = false;

			if (new_name || typed_param.mValues.empty())
			{
				new_value = true;
				typed_param.mValues.push_back(value_t());
			}

			param_value_t& value = typed_param.mValues.back();

			// attempt to parse block...
			if(value.deserializeBlock(parser, name_stack_range, new_name))
			{
				typed_param.setProvided();
				return true;
			}
			else if(named_value_t::valueNamesExist())
			{
				// try to parse a known named value
				std::string name;
				if (parser.readValue(name))
				{
					// try to parse a per type named value
					if (named_value_t::getValueFromName(name, value.getValue()))
					{
						typed_param.mValues.back().setValueName(name);
						typed_param.setProvided();
						return true;
					}

				}
			}

			if (new_value)
			{	// failed to parse new value, pop it off
				typed_param.mValues.pop_back();
			}

			return false;
		}

		static void serializeParam(const Param& param, Parser& parser, Parser::name_stack_t& name_stack, const Param* diff_param)
		{
			const self_t& typed_param = static_cast<const self_t&>(param);
			if (!typed_param.isProvided() || name_stack.empty()) return;

			for (const_iterator it = typed_param.mValues.begin(), end_it = typed_param.mValues.end();
				it != end_it;
				++it)
			{
				name_stack.back().second = true;

				std::string key = it->getValueName();
				if (!key.empty())
				{
					parser.writeValue(key, name_stack);
				}
				// Not parsed via named values, write out value directly
				// NOTE: currently we don't worry about removing default values in Multiple
				else 
				{
					it->serializeBlock(parser, name_stack, NULL);
				}
			}
		}

		static void inspectParam(const Param& param, Parser& parser, Parser::name_stack_t& name_stack, S32 min_count, S32 max_count)
		{
			const param_value_t& value_param = param_value_t(value_t());

			// tell parser about our actual type
			parser.inspectValue<value_t>(name_stack, min_count, max_count, NULL);
			// then tell it about string-based alternatives ("red", "blue", etc. for LLColor4)
			if (named_value_t::getPossibleValues())
			{
				parser.inspectValue<std::string>(name_stack, min_count, max_count, named_value_t::getPossibleValues());
		}

			value_param.inspectBlock(parser, name_stack, min_count, max_count);
		}

		void set(const container_t& val, bool flag_as_provided = true)
		{
			mValues = val;
			setProvided(flag_as_provided);
		}

		param_value_t& add()
		{
			mValues.push_back(value_t());
			setProvided();
			return mValues.back();
		}

		self_t& add(const value_t& item)
		{
			mValues.push_back(item);
			setProvided();
			return *this;
		}

		self_t& add(const typename named_value_t::name_t& name)
		{
			value_t value;

			// try to parse a per type named value
			if (named_value_t::getValueFromName(name, value))
			{
				add(value);
				mValues.back().setValueName(name);
			}
			return *this;
		}

		// implicit conversion
		operator const container_t&() const { return mValues; } 
		// explicit conversion
		const container_t& operator()() const { return mValues; }

		iterator begin() { return mValues.begin(); }
		iterator end() { return mValues.end(); }
		const_iterator begin() const { return mValues.begin(); }
		const_iterator end() const { return mValues.end(); }
		bool empty() const { return mValues.empty(); }
		size_t size() const { return mValues.size(); }

		U32 numValidElements() const
		{
			U32 count = 0;
			for (const_iterator it = mValues.begin(), end_it = mValues.end();
				it != end_it;
				++it)
			{
				if(it->validateBlock(false)) count++;
			}
			return count;
		}

	protected:

		static bool mergeWith(Param& dst, const Param& src, bool overwrite)
		{
			const self_t& src_typed_param = static_cast<const self_t&>(src);
			self_t& dst_typed_param = static_cast<self_t&>(dst);

			if (overwrite)
			{
				std::copy(src_typed_param.begin(), src_typed_param.end(), std::back_inserter(dst_typed_param.mValues));
			}
			else
			{
				container_t new_values(src_typed_param.mValues);
				std::copy(dst_typed_param.begin(), dst_typed_param.end(), std::back_inserter(new_values));
				std::swap(dst_typed_param.mValues, new_values);
			}

			if (src_typed_param.begin() != src_typed_param.end())
			{
				dst_typed_param.setProvided();
			}

			return true;
		}

		container_t			mValues;

	private:
		void init( BlockDescriptor &block_descriptor, ParamDescriptor::validation_func_t validate_func, S32 min_count, S32 max_count, const char* name ) 
		{
			ParamDescriptorPtr param_descriptor = ParamDescriptorPtr(new ParamDescriptor(
				block_descriptor.mCurrentBlockPtr->getHandleFromParam(this),
				&mergeWith,
				&deserializeParam,
				&serializeParam,
				validate_func,
				&inspectParam,
				min_count, max_count));
			block_descriptor.addParam(param_descriptor, name);
		}
	};

	template <typename DERIVED_BLOCK, typename BASE_BLOCK = BaseBlock>
	class ChoiceBlock : public BASE_BLOCK
	{
		typedef ChoiceBlock<DERIVED_BLOCK, BASE_BLOCK>	self_t;
		typedef ChoiceBlock<DERIVED_BLOCK, BASE_BLOCK>	enclosing_block_t;
		typedef BASE_BLOCK								base_block_t;
		
		LOG_CLASS(self_t);
	public:
		// take all provided params from other and apply to self
		bool overwriteFrom(const self_t& other)
		{
			return static_cast<DERIVED_BLOCK*>(this)->mergeBlock(getBlockDescriptor(), other, true);
		}

		// take all provided params that are not already provided, and apply to self
		bool fillFrom(const self_t& other)
		{
			return static_cast<DERIVED_BLOCK*>(this)->mergeBlock(getBlockDescriptor(), other, false);
		}

		bool mergeBlockParam(bool source_provided, bool dest_provided, BlockDescriptor& block_data, const self_t& source, bool overwrite)
		{
			bool source_override = source_provided && (overwrite || !dest_provided);

			if (source_override || source.mCurChoice == mCurChoice)
			{
				return mergeBlock(block_data, source, overwrite);
			}
			return false;
		}

		// merge with other block
		bool mergeBlock(BlockDescriptor& block_data, const self_t& other, bool overwrite)
		{
			mCurChoice = other.mCurChoice;
			return base_block_t::mergeBlock(getBlockDescriptor(), other, overwrite);
		}

		// clear out old choice when param has changed
		/*virtual*/ void paramChanged(const Param& changed_param, bool user_provided)
		{ 
			param_handle_t changed_param_handle = base_block_t::getHandleFromParam(&changed_param);
			// if we have a new choice...
			if (changed_param_handle != mCurChoice)
			{
				// clear provided flag on previous choice
				Param* previous_choice = base_block_t::getParamFromHandle(mCurChoice);
				if (previous_choice) 
				{
					previous_choice->setProvided(false);
				}
				mCurChoice = changed_param_handle;
			}
			base_block_t::paramChanged(changed_param, user_provided);
		}

		virtual const BlockDescriptor& mostDerivedBlockDescriptor() const { return getBlockDescriptor(); }
		virtual BlockDescriptor& mostDerivedBlockDescriptor() { return getBlockDescriptor(); }

	protected:
		ChoiceBlock()
		:	mCurChoice(0)
		{
			BaseBlock::init(getBlockDescriptor(), base_block_t::getBlockDescriptor(), sizeof(DERIVED_BLOCK));
		}

		// Alternatives are mutually exclusive wrt other Alternatives in the same block.  
		// One alternative in a block will always have isChosen() == true.
		// At most one alternative in a block will have isProvided() == true.
		template <typename T, typename NAME_VALUE_LOOKUP = typename TypeValues<T>::type_value_t >
		class Alternative : public TypedParam<T, NAME_VALUE_LOOKUP, false>
		{
			typedef TypedParam<T, NAME_VALUE_LOOKUP, false>	super_t;
			typedef typename super_t::value_t				value_t;
			typedef typename super_t::default_value_t		default_value_t;

		public:
			friend class ChoiceBlock<DERIVED_BLOCK>;

			using super_t::operator =;

			explicit Alternative(const char* name = "", const default_value_t& val = defaultValue<default_value_t>())
			:	super_t(DERIVED_BLOCK::getBlockDescriptor(), name, val, NULL, 0, 1),
				mOriginalValue(val)
			{
				// assign initial choice to first declared option
				DERIVED_BLOCK* blockp = ((DERIVED_BLOCK*)DERIVED_BLOCK::getBlockDescriptor().mCurrentBlockPtr);
				if (LL_UNLIKELY(DERIVED_BLOCK::getBlockDescriptor().mInitializationState == BlockDescriptor::INITIALIZING))
				{
					if(blockp->mCurChoice == 0)
					{
						blockp->mCurChoice = Param::enclosingBlock().getHandleFromParam(this);
					}
				}
			}

			void choose()
			{
				static_cast<enclosing_block_t&>(Param::enclosingBlock()).paramChanged(*this, true);
			}

			void chooseAs(const value_t& val)
			{
				super_t::set(val);
			}

			void operator =(const value_t& val)
			{
				super_t::set(val);
			}

			void operator()(const value_t& val) 
			{ 
				super_t::set(val);
			}

			operator const value_t&() const 
			{
				return (*this)();
			} 

			const value_t& operator()() const 
			{ 
				if (static_cast<enclosing_block_t&>(Param::enclosingBlock()).getCurrentChoice() == this)
				{
					return super_t::getValue(); 
				}
				return mOriginalValue;
			} 

			bool isChosen() const
			{
				return static_cast<enclosing_block_t&>(Param::enclosingBlock()).getCurrentChoice() == this;
			}
		
		private:
			default_value_t mOriginalValue;
		};

	public:
		static BlockDescriptor& getBlockDescriptor()
		{
			static BlockDescriptor sBlockDescriptor;
			return sBlockDescriptor;
		}

	private:
		param_handle_t	mCurChoice;

		const Param* getCurrentChoice() const
		{
			return base_block_t::getParamFromHandle(mCurChoice);
		}
	};

	template <typename DERIVED_BLOCK, typename BASE_BLOCK = BaseBlock>
	class Block 
	:	public BASE_BLOCK
	{
		typedef Block<DERIVED_BLOCK, BASE_BLOCK>	self_t;

	protected:
		typedef Block<DERIVED_BLOCK, BASE_BLOCK>	block_t;

	public:
		typedef BASE_BLOCK base_block_t;

		// take all provided params from other and apply to self
		bool overwriteFrom(const self_t& other)
		{
			return static_cast<DERIVED_BLOCK*>(this)->mergeBlock(getBlockDescriptor(), other, true);
		}

		// take all provided params that are not already provided, and apply to self
		bool fillFrom(const self_t& other)
		{
			return static_cast<DERIVED_BLOCK*>(this)->mergeBlock(getBlockDescriptor(), other, false);
		}

		virtual const BlockDescriptor& mostDerivedBlockDescriptor() const { return getBlockDescriptor(); }
		virtual BlockDescriptor& mostDerivedBlockDescriptor() { return getBlockDescriptor(); }

	protected:
		Block()
		{
			//#pragma message("Parsing LLInitParam::Block")
			BaseBlock::init(getBlockDescriptor(), BASE_BLOCK::getBlockDescriptor(), sizeof(DERIVED_BLOCK));
		}

		//
		// Nested classes for declaring parameters
		//
		template <typename T, typename NAME_VALUE_LOOKUP = typename TypeValues<T>::type_value_t >
		class Optional : public TypedParam<T, NAME_VALUE_LOOKUP, false>
		{
			typedef TypedParam<T, NAME_VALUE_LOOKUP, false>		super_t;
			typedef typename super_t::value_t					value_t;
			typedef typename super_t::default_value_t			default_value_t;

		public:
			using super_t::operator();
			using super_t::operator =;
			
			explicit Optional(const char* name = "", const default_value_t& val = defaultValue<default_value_t>())
			:	super_t(DERIVED_BLOCK::getBlockDescriptor(), name, val, NULL, 0, 1)
			{
				//#pragma message("Parsing LLInitParam::Block::Optional")
			}

			Optional& operator =(const value_t& val)
			{
				set(val);
				return *this;
			}

			DERIVED_BLOCK& operator()(const value_t& val)
			{
				super_t::set(val);
				return static_cast<DERIVED_BLOCK&>(Param::enclosingBlock());
			}
		};

		template <typename T, typename NAME_VALUE_LOOKUP = typename TypeValues<T>::type_value_t >
		class Mandatory : public TypedParam<T, NAME_VALUE_LOOKUP, false>
		{
			typedef TypedParam<T, NAME_VALUE_LOOKUP, false>		super_t;
			typedef Mandatory<T, NAME_VALUE_LOOKUP>										self_t;
			typedef typename super_t::value_t					value_t;
			typedef typename super_t::default_value_t			default_value_t;

		public:
			using super_t::operator();
			using super_t::operator =;

			// mandatory parameters require a name to be parseable
			explicit Mandatory(const char* name = "", const default_value_t& val = defaultValue<default_value_t>())
			:	super_t(DERIVED_BLOCK::getBlockDescriptor(), name, val, &validate, 1, 1)
			{}

			Mandatory& operator =(const value_t& val)
			{
				set(val);
				return *this;
			}

			DERIVED_BLOCK& operator()(const value_t& val)
			{
				super_t::set(val);
				return static_cast<DERIVED_BLOCK&>(Param::enclosingBlock());
			}

			static bool validate(const Param* p)
			{
				// valid only if provided
				return static_cast<const self_t*>(p)->isProvided();
			}

		};

		template <typename T, typename RANGE = BaseBlock::AnyAmount, typename NAME_VALUE_LOOKUP = typename TypeValues<T>::type_value_t >
		class Multiple : public TypedParam<T, NAME_VALUE_LOOKUP, true>
		{
			typedef TypedParam<T, NAME_VALUE_LOOKUP, true>	super_t;
			typedef Multiple<T, RANGE, NAME_VALUE_LOOKUP>							self_t;
			typedef typename super_t::container_t									container_t;
			typedef typename super_t::value_t				value_t;

		public:
			typedef typename super_t::iterator										iterator;
			typedef typename super_t::const_iterator								const_iterator;

			explicit Multiple(const char* name = "")
			:	super_t(DERIVED_BLOCK::getBlockDescriptor(), name, container_t(), &validate, RANGE::minCount, RANGE::maxCount)
			{}

			Multiple& operator =(const container_t& val)
			{
				set(val);
				return *this;
			}

			DERIVED_BLOCK& operator()(const container_t& val)
			{
				super_t::set(val);
				return static_cast<DERIVED_BLOCK&>(Param::enclosingBlock());
			}

			static bool validate(const Param* paramp) 
			{
				U32 num_valid = ((super_t*)paramp)->numValidElements();
				return RANGE::minCount <= num_valid && num_valid <= RANGE::maxCount;
			}
		};

		class Deprecated : public Param
		{
		public:
			explicit Deprecated(const char* name)
			:	Param(DERIVED_BLOCK::getBlockDescriptor().mCurrentBlockPtr)
			{
				BlockDescriptor& block_descriptor = DERIVED_BLOCK::getBlockDescriptor();
				if (LL_UNLIKELY(block_descriptor.mInitializationState == BlockDescriptor::INITIALIZING))
				{
					ParamDescriptorPtr param_descriptor = ParamDescriptorPtr(new ParamDescriptor(
													block_descriptor.mCurrentBlockPtr->getHandleFromParam(this),
													NULL,
													&deserializeParam,
													NULL,
													NULL,
													NULL, 
													0, S32_MAX));
					block_descriptor.addParam(param_descriptor, name);
				}
			}
			
			static bool deserializeParam(Param& param, Parser& parser, const Parser::name_stack_range_t& name_stack_range, bool new_name)
			{
				if (name_stack_range.first == name_stack_range.second)
				{
					//std::string message = llformat("Deprecated value %s ignored", getName().c_str());
					//parser.parserWarning(message);
					return true;
				}

				return false;
			}
		};

		// different semantics for documentation purposes, but functionally identical
		typedef Deprecated Ignored;

	public:
		static BlockDescriptor& getBlockDescriptor()
		{
			static BlockDescriptor sBlockDescriptor;
			return sBlockDescriptor;
		}

	protected:
		template <typename T, typename NAME_VALUE_LOOKUP, bool multiple, typename is_block>
		void changeDefault(TypedParam<T, NAME_VALUE_LOOKUP, multiple, is_block>& param, 
			const typename TypedParam<T, NAME_VALUE_LOOKUP, multiple, is_block>::value_t& value)
		{
			if (!param.isProvided())
			{
				param.set(value, false);
			}
		}

	};
	
	template<typename T, typename BLOCK_T>
	struct IsBlock<ParamValue<BaseBlock::Lazy<T, BaseBlock::IS_A_BLOCK>, BLOCK_T >, void>
	{
		typedef IS_A_BLOCK value_t;
	};

	template<typename T, typename BLOCK_T>
	struct IsBlock<ParamValue<BaseBlock::Lazy<T, BaseBlock::NOT_A_BLOCK>, BLOCK_T >, void>
	{
		typedef NOT_BLOCK value_t;
	};

	template<typename T, typename BLOCK_IDENTIFIER>
	struct IsBlock<ParamValue<BaseBlock::Atomic<T>, typename IsBlock<BaseBlock::Atomic<T> >::value_t >, BLOCK_IDENTIFIER>
	{
		typedef typename IsBlock<T>::value_t value_t;
	};

	template<typename T, typename BLOCK_IDENTIFIER>
	struct IsBlock<ParamValue<BaseBlock::Sequential<T>, typename IsBlock<BaseBlock::Sequential<T> >::value_t >, BLOCK_IDENTIFIER>
	{
		typedef typename IsBlock<T>::value_t value_t;
	};


	template<typename T>
	struct InnerMostType
	{
		typedef T value_t;
	};

	template<typename T>
	struct InnerMostType<ParamValue<T, NOT_BLOCK> >
	{
		typedef typename InnerMostType<T>::value_t value_t;
	};

	template<typename T>
	struct InnerMostType<ParamValue<T, IS_A_BLOCK> >
	{
		typedef typename InnerMostType<T>::value_t value_t;
	};

	template<typename T, typename BLOCK_T>
	class ParamValue <BaseBlock::Atomic<T>, BLOCK_T>
	{
		typedef ParamValue <BaseBlock::Atomic<T>, BLOCK_T> self_t;

	public:
		typedef typename InnerMostType<T>::value_t	value_t;
		typedef T									default_value_t;

		ParamValue()
		:	mValue(),
			mValidated(false)
		{}

		ParamValue(const default_value_t& value)
		:	mValue(value),
			mValidated(false)
		{}

		void setValue(const value_t& val)
		{
			mValue.setValue(val);
		}

		const value_t& getValue() const
		{
			return mValue.getValue();
		}

		value_t& getValue()
		{
			return mValue.getValue();
		}

		bool deserializeBlock(Parser& p, Parser::name_stack_range_t name_stack_range, bool new_name)
		{
			if (new_name)
			{
				resetToDefault();
			}
			return mValue.deserializeBlock(p, name_stack_range, new_name);
			}

		void serializeBlock(Parser& p, Parser::name_stack_t& name_stack, const self_t* diff_block = NULL) const
		{
			const BaseBlock* base_block = diff_block
				? &(diff_block->mValue)
				: NULL;
			mValue.serializeBlock(p, name_stack, base_block);
		}

		bool inspectBlock(Parser& p, Parser::name_stack_t name_stack = Parser::name_stack_t(), S32 min_count = 0, S32 max_count = S32_MAX) const
		{
			return mValue.inspectBlock(p, name_stack, min_count, max_count);
		}

		bool mergeBlockParam(bool source_provided, bool dst_provided, BlockDescriptor& block_data, const self_t& source, bool overwrite)
		{
			if (overwrite)
			{
				resetToDefault();
				return mValue.mergeBlock(block_data, source.getValue(), overwrite);
			}
			return false;
		}

		bool validateBlock(bool emit_errors = true) const
		{
			return mValue.validateBlock(emit_errors);
		}

		static BlockDescriptor& getBlockDescriptor()
		{
			return value_t::getBlockDescriptor();
		}


		mutable bool 	mValidated; // lazy validation flag

	private:
		void resetToDefault()
		{
			static T default_value;
			mValue = default_value;
		}

		T	mValue;
	};

	template<typename T>
	class ParamValue <BaseBlock::Sequential<T>, IS_A_BLOCK>
	{
		typedef ParamValue <BaseBlock::Sequential<T>, IS_A_BLOCK> self_t;

	public:
		typedef typename InnerMostType<T>::value_t	value_t;
		typedef T									default_value_t;

		ParamValue()
		:	mValue(),
			mValidated(false)
		{
			mCurParam = getBlockDescriptor().mAllParams.begin();
		}

		ParamValue(const default_value_t& value)
		:	mValue(value),
			mValidated(false)
		{
			mCurParam = getBlockDescriptor().mAllParams.begin();
		}

		void setValue(const value_t& val)
		{
			mValue.setValue(val);
		}

		const value_t& getValue() const
		{
			return mValue.getValue();
		}

		value_t& getValue()
		{
			return mValue.getValue();
		}

		bool deserializeBlock(Parser& p, Parser::name_stack_range_t name_stack_range, bool new_name)
		{
			if (new_name)
			{
				mCurParam = getBlockDescriptor().mAllParams.begin();
			}
			if (name_stack_range.first == name_stack_range.second 
				&& mCurParam != getBlockDescriptor().mAllParams.end())
			{
				// deserialize to mCurParam
				ParamDescriptor& pd = *(*mCurParam);
				ParamDescriptor::deserialize_func_t deserialize_func = pd.mDeserializeFunc;
				Param* paramp = mValue.getParamFromHandle(pd.mParamHandle);

				if (deserialize_func 
					&& paramp 
					&& deserialize_func(*paramp, p, name_stack_range, new_name))
				{
					++mCurParam;
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				return mValue.deserializeBlock(p, name_stack_range, new_name);
			}
		}

		void serializeBlock(Parser& p, Parser::name_stack_t& name_stack, const self_t* diff_block = NULL) const
		{
			const BaseBlock* base_block = diff_block
				? &(diff_block->mValue)
				: NULL;
			mValue.serializeBlock(p, name_stack, base_block);
		}

		bool inspectBlock(Parser& p, Parser::name_stack_t name_stack = Parser::name_stack_t(), S32 min_count = 0, S32 max_count = S32_MAX) const
		{
			return mValue.inspectBlock(p, name_stack, min_count, max_count);
		}

		bool mergeBlockParam(bool source_provided, bool dst_provided, BlockDescriptor& block_data, const self_t& source, bool overwrite)
		{
			return mValue.mergeBlock(block_data, source.getValue(), overwrite);
		}

		bool validateBlock(bool emit_errors = true) const
		{
			return mValue.validateBlock(emit_errors);
		}

		static BlockDescriptor& getBlockDescriptor()
		{
			return value_t::getBlockDescriptor();
		}

		mutable bool 	mValidated; // lazy validation flag

	private:

		BlockDescriptor::all_params_list_t::iterator	mCurParam;
		T												mValue;
	};

	template<typename T>
	class ParamValue <BaseBlock::Sequential<T>, NOT_BLOCK>
	: public T
	{
		typedef ParamValue <BaseBlock::Sequential<T>, NOT_BLOCK> self_t;

	public:
		typedef typename InnerMostType<T>::value_t	value_t;
		typedef T									default_value_t;

		ParamValue()
		:	T(),
			mValidated(false)
		{}

		ParamValue(const default_value_t& value)
		:	T(value.getValue()),
			mValidated(false)
		{}

		mutable bool 	mValidated; // lazy validation flag
	};

	template<typename T, typename BLOCK_T>
	class ParamValue <BaseBlock::Lazy<T, IS_A_BLOCK>, BLOCK_T> 
	{
		typedef ParamValue <BaseBlock::Lazy<T, IS_A_BLOCK>, BLOCK_T> self_t;

	public:
		typedef typename InnerMostType<T>::value_t	value_t;
		typedef LazyValue<T>						default_value_t;
	
		ParamValue()
		:	mValue(),
			mValidated(false)
		{}

		ParamValue(const default_value_t& other)
		:	mValue(other),
			mValidated(false)
		{}

		ParamValue(const T& value)
		:	mValue(value),
			mValidated(false)
		{}

		void setValue(const value_t& val)
		{
			mValue.set(val);
		}

		const value_t& getValue() const
		{
			return mValue.get().getValue();
		}

		value_t& getValue()
		{
			return mValue.get().getValue();
		}

		bool deserializeBlock(Parser& p, Parser::name_stack_range_t name_stack_range, bool new_name)
		{
			return mValue.get().deserializeBlock(p, name_stack_range, new_name);
		}

		void serializeBlock(Parser& p, Parser::name_stack_t& name_stack, const self_t* diff_block = NULL) const
		{
			if (mValue.empty()) return;
			
			const BaseBlock* base_block = (diff_block && !diff_block->mValue.empty())
											? &(diff_block->mValue.get().getValue())
											: NULL;
			mValue.get().serializeBlock(p, name_stack, base_block);
		}

		bool inspectBlock(Parser& p, Parser::name_stack_t name_stack = Parser::name_stack_t(), S32 min_count = 0, S32 max_count = S32_MAX) const
		{
			return mValue.get().inspectBlock(p, name_stack, min_count, max_count);
		}

		bool mergeBlockParam(bool source_provided, bool dst_provided, BlockDescriptor& block_data, const self_t& source, bool overwrite)
		{
			return source.mValue.empty() || mValue.get().mergeBlock(block_data, source.getValue(), overwrite);
		}

		bool validateBlock(bool emit_errors = true) const
		{
			return mValue.empty() || mValue.get().validateBlock(emit_errors);
		}

		static BlockDescriptor& getBlockDescriptor()
		{
			return value_t::getBlockDescriptor();
		}

		mutable bool 	mValidated; // lazy validation flag

	private:
		LazyValue<T>	mValue;
	};

	template<typename T, typename BLOCK_T>
	class ParamValue <BaseBlock::Lazy<T, NOT_BLOCK>, BLOCK_T>
		{
		typedef ParamValue <BaseBlock::Lazy<T, NOT_BLOCK>, BLOCK_T> self_t;

	public:
		typedef typename InnerMostType<T>::value_t	value_t;
		typedef LazyValue<T>						default_value_t;

		ParamValue()
		:	mValue(),
			mValidated(false)
		{}

		ParamValue(const default_value_t& other)
		:	mValue(other),
			mValidated(false)
		{}

		ParamValue(const T& value)
		:	mValue(value),
			mValidated(false)
		{}
			
		void setValue(const value_t& val)
		{
			mValue.set(val);
		}

		const value_t& getValue() const
		{
			return mValue.get().getValue();
		}

		value_t& getValue()
		{
			return mValue.get().getValue();
		}

		mutable bool 	mValidated; // lazy validation flag

	private:
		LazyValue<T>	mValue;
	};

	template <>
	class ParamValue <LLSD, NOT_BLOCK>
	:	public BaseBlock
	{
	public:
		typedef LLSD			value_t;
		typedef LLSD			default_value_t;

		ParamValue()
		:	mValidated(false)
		{}

		ParamValue(const default_value_t& other)
		:	mValue(other),
			mValidated(false)
		{}

		void setValue(const value_t& val) { mValue = val; }

		const value_t& getValue() const { return mValue; }
		LLSD& getValue() { return mValue; }

		// block param interface
		bool deserializeBlock(Parser& p, Parser::name_stack_range_t name_stack_range, bool new_name);
		void serializeBlock(Parser& p, Parser::name_stack_t& name_stack, const BaseBlock* diff_block = NULL) const;
		bool inspectBlock(Parser& p, Parser::name_stack_t name_stack = Parser::name_stack_t(), S32 min_count = 0, S32 max_count = S32_MAX) const
		{
			//TODO: implement LLSD params as schema type Any
			return true;
		}

	protected:
		mutable bool 	mValidated; // lazy validation flag

	private:
		static void serializeElement(Parser& p, const LLSD& sd, Parser::name_stack_t& name_stack);

		LLSD mValue;
	};

	template<typename T>
	class CustomParamValue
	:	public Block<ParamValue<T> >
	{
	public:
		typedef enum e_value_age
		{	
			VALUE_NEEDS_UPDATE,		// mValue needs to be refreshed from the block parameters
			VALUE_AUTHORITATIVE,	// mValue holds the authoritative value (which has been replicated to the block parameters via updateBlockFromValue)
			BLOCK_AUTHORITATIVE		// mValue is derived from the block parameters, which are authoritative
		} EValueAge;

		typedef ParamValue<T>			derived_t;
		typedef CustomParamValue<T>				self_t;
		typedef Block<derived_t>				block_t;
		typedef T						default_value_t;
		typedef T								value_t;
		typedef void					baseblock_base_class_t;


		CustomParamValue(const default_value_t& value = T())
		:	mValue(value),
			mValueAge(VALUE_AUTHORITATIVE),
			mValidated(false)
		{}

		bool deserializeBlock(Parser& parser, Parser::name_stack_range_t name_stack_range, bool new_name)
		{
			derived_t& typed_param = static_cast<derived_t&>(*this);
			// try to parse direct value T
			if (name_stack_range.first == name_stack_range.second)
			{
				if(parser.readValue(typed_param.mValue))
				{
					typed_param.mValueAge = VALUE_AUTHORITATIVE;
					typed_param.updateBlockFromValue(false);

					return true;
				}
			}

			// fall back on parsing block components for T
			return typed_param.BaseBlock::deserializeBlock(parser, name_stack_range, new_name);
		}

		void serializeBlock(Parser& parser, Parser::name_stack_t& name_stack, const BaseBlock* diff_block = NULL) const
		{
			const derived_t& typed_param = static_cast<const derived_t&>(*this);
			const derived_t* diff_param = static_cast<const derived_t*>(diff_block);
			
			// then try to serialize value directly
			if (!diff_param || !ParamCompare<T>::equals(typed_param.getValue(), diff_param->getValue()))
            {
				
				if (!parser.writeValue(typed_param.getValue(), name_stack)) 
				{
					//RN: *always* serialize provided components of BlockValue (don't pass diff_param on),
					// since these tend to be viewed as the constructor arguments for the value T.  It seems
					// cleaner to treat the uniqueness of a BlockValue according to the generated value, and
					// not the individual components.  This way <color red="0" green="1" blue="0"/> will not
					// be exported as <color green="1"/>, since it was probably the intent of the user to 
					// be specific about the RGB color values.  This also fixes an issue where we distinguish
					// between rect.left not being provided and rect.left being explicitly set to 0 (same as default)

					if (typed_param.mValueAge == VALUE_AUTHORITATIVE)
					{
						// if the value is authoritative but the parser doesn't accept the value type
						// go ahead and make a copy, and splat the value out to its component params
						// and serialize those params
						derived_t copy(typed_param);
						copy.updateBlockFromValue(true);
						copy.block_t::serializeBlock(parser, name_stack, NULL);
					}
					else
					{
						block_t::serializeBlock(parser, name_stack, NULL);
					}
				}
			}
		}

		bool validateBlock(bool emit_errors = true) const
		{
			if (mValueAge == VALUE_NEEDS_UPDATE)
			{
				if (block_t::validateBlock(emit_errors))
				{
					// clear stale keyword associated with old value
					mValueAge = BLOCK_AUTHORITATIVE;
					static_cast<derived_t*>(const_cast<self_t*>(this))->updateValueFromBlock();
					return true;
				}
				else
				{
					//block value incomplete, so not considered provided
					// will attempt to revalidate on next call to isProvided()
					return false;  
				}
			}
			else
			{
				// we have a valid value in hand
				return true;
			}
		}

 		// propagate change status up to enclosing block
		/*virtual*/ void paramChanged(const Param& changed_param, bool user_provided)
		{ 
			BaseBlock::paramChanged(changed_param, user_provided);
			if (user_provided)
			{
				// a parameter changed, so our value is out of date
				mValueAge = VALUE_NEEDS_UPDATE;
			}
		}
			
		void setValue(const value_t& val)
		{
			// set param version number to be up to date, so we ignore block contents
			mValueAge = VALUE_AUTHORITATIVE;
			mValue = val;
			static_cast<derived_t*>(this)->updateBlockFromValue(false);
		}

		const value_t& getValue() const
		{
			validateBlock(true);
			return mValue;
		}

		T& getValue() 
		{
			validateBlock(true);
			return mValue;
		}

	protected:

		// use this from within updateValueFromBlock() to set the value without making it authoritative
		void updateValue(const value_t& value)
		{
			mValue = value;
		}

		bool mergeBlockParam(bool source_provided, bool dst_provided, BlockDescriptor& block_data, const BaseBlock& source, bool overwrite)
		{
			bool source_override = source_provided && (overwrite || !dst_provided);

			const derived_t& src_typed_param = static_cast<const derived_t&>(source);

			if (source_override && src_typed_param.mValueAge == VALUE_AUTHORITATIVE)
			{
				// copy value over
				setValue(src_typed_param.getValue());
				return true;
			}
			// merge individual parameters into destination
			if (mValueAge == VALUE_AUTHORITATIVE)
			{
				static_cast<derived_t*>(this)->updateBlockFromValue(dst_provided);
			}
			return mergeBlock(block_data, source, overwrite);
		}

		bool mergeBlock(BlockDescriptor& block_data, const BaseBlock& source, bool overwrite)
		{
			return block_t::mergeBlock(block_data, source, overwrite);
		}

		mutable bool 		mValidated; // lazy validation flag

	private:
		mutable T			mValue;
		mutable EValueAge	mValueAge;
	};
}


#endif // LL_LLPARAM_H
