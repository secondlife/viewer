/** 
 * @file lluictrlfactory.h
 * @brief Factory class for creating UI controls
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#ifndef LLUICTRLFACTORY_H
#define LLUICTRLFACTORY_H

#include "llcallbackmap.h"
#include "llinitparam.h"
#include "llregistry.h"
#include "v4color.h"
#include "llfasttimer.h"

#include "llxuiparser.h"

#include <boost/function.hpp>
#include <iosfwd>
#include <stack>
#include <set>

class LLPanel;
class LLFloater;
class LLView;


// sort functor for typeid maps
struct LLCompareTypeID
{
	bool operator()(const std::type_info* lhs, const std::type_info* rhs) const
	{
		return lhs->before(*rhs);
	}
};

// lookup widget constructor funcs by widget name
template <typename DERIVED_TYPE>
class LLChildRegistry : public LLRegistrySingleton<std::string, LLWidgetCreatorFunc, DERIVED_TYPE>
{
public:
	typedef LLRegistrySingleton<std::string, LLWidgetCreatorFunc, DERIVED_TYPE> super_t;
	// local static instance for registering a particular widget
	template<typename T>
	class Register : public super_t::StaticRegistrar
	{
	public:
		// register with either the provided builder, or the generic templated builder
		Register(const char* tag, LLWidgetCreatorFunc func = NULL);
	};

protected:
	LLChildRegistry() {}
};

class LLDefaultChildRegistry : public LLChildRegistry<LLDefaultChildRegistry>
{
protected:
	LLDefaultChildRegistry(){}
	friend class LLSingleton<LLDefaultChildRegistry>;
};

// lookup widget name by type
class LLWidgetNameRegistry 
:	public LLRegistrySingleton<const std::type_info*, std::string, LLWidgetNameRegistry , LLCompareTypeID>
{};

// lookup factory functions for default widget instances by widget type
typedef LLView* (*dummy_widget_creator_func_t)(const std::string&);
class LLDefaultWidgetRegistry
:	public LLRegistrySingleton<const std::type_info*, dummy_widget_creator_func_t, LLDefaultWidgetRegistry, LLCompareTypeID>
{};

// lookup function for generating empty param block by widget type
// this is used for schema generation
//typedef const LLInitParam::BaseBlock& (*empty_param_block_func_t)();
//class LLDefaultParamBlockRegistry
//:	public LLRegistrySingleton<const std::type_info*, empty_param_block_func_t, LLDefaultParamBlockRegistry, LLCompareTypeID>
//{};

extern LLFastTimer::DeclareTimer FTM_WIDGET_SETUP;
extern LLFastTimer::DeclareTimer FTM_WIDGET_CONSTRUCTION;
extern LLFastTimer::DeclareTimer FTM_INIT_FROM_PARAMS;

// Build time optimization, generate this once in .cpp file
#ifndef LLUICTRLFACTORY_CPP
extern template class LLUICtrlFactory* LLSingleton<class LLUICtrlFactory>::getInstance();
#endif

class LLUICtrlFactory : public LLSingleton<LLUICtrlFactory>
{
private:
	friend class LLSingleton<LLUICtrlFactory>;
	LLUICtrlFactory();
	~LLUICtrlFactory();

	// only partial specialization allowed in inner classes, so use extra dummy parameter
	template <typename PARAM_BLOCK, int DUMMY>
	class ParamDefaults : public LLSingleton<ParamDefaults<PARAM_BLOCK, DUMMY> > 
	{
	public:
		ParamDefaults()
		{
			// recursively initialize from base class param block
			((typename PARAM_BLOCK::base_block_t&)mPrototype).fillFrom(ParamDefaults<typename PARAM_BLOCK::base_block_t, DUMMY>::instance().get());
			// after initializing base classes, look up template file for this param block
			const std::string* param_block_tag = getWidgetTag(&typeid(PARAM_BLOCK));
			if (param_block_tag)
			{
				LLUICtrlFactory::loadWidgetTemplate(*param_block_tag, mPrototype);
			}
		}

		const PARAM_BLOCK& get() { return mPrototype; }

	private:
		PARAM_BLOCK mPrototype;
	};

	// base case for recursion, there are NO base classes of LLInitParam::BaseBlock
	template<int DUMMY>
	class ParamDefaults<LLInitParam::BaseBlock, DUMMY> : public LLSingleton<ParamDefaults<LLInitParam::BaseBlock, DUMMY> >
	{
	public:
		const LLInitParam::BaseBlock& get() { return mBaseBlock; }
	private:
		LLInitParam::BaseBlock mBaseBlock;
	};

public:

	// get default parameter block for widget of a specific type
	template<typename T>
	static const typename T::Params& getDefaultParams()
	{
		//#pragma message("Generating ParamDefaults")
		return ParamDefaults<typename T::Params, 0>::instance().get();
	}

	bool buildFloater(LLFloater* floaterp, const std::string &filename, LLXMLNodePtr output_node);
	BOOL buildPanel(LLPanel* panelp, const std::string &filename, LLXMLNodePtr output_node = NULL);

	// Does what you want for LLFloaters and LLPanels
	// Returns 0 on success
	S32 saveToXML(LLView* viewp, const std::string& filename);

	std::string getCurFileName() { return mFileNames.empty() ? "" : mFileNames.back(); }

	static BOOL getAttributeColor(LLXMLNodePtr node, const std::string& name, LLColor4& color);

	LLPanel* createFactoryPanel(const std::string& name);

	void pushFactoryFunctions(const LLCallbackMap::map_t* map);
	void popFactoryFunctions();

	template<typename T>
	static T* createWidget(typename T::Params& params, LLView* parent = NULL)
	{
		T* widget = NULL;

		T::setupParams(params, parent);

		if (!params.validateBlock())
		{
			llwarns << getInstance()->getCurFileName() << ": Invalid parameter block for " << typeid(T).name() << llendl;
			//return NULL;
		}

		{
			LLFastTimer timer(FTM_WIDGET_CONSTRUCTION);
			widget = new T(params);	
		}
		{
			LLFastTimer timer(FTM_INIT_FROM_PARAMS);
			widget->initFromParams(params);
		}

		if (parent)
		{
			S32 tab_group = params.tab_group.isProvided() ? params.tab_group() : S32_MAX;
			setCtrlParent(widget, parent, tab_group);
		}
		return widget;
	}

	template<typename T>
	static T* create(typename T::Params& params, LLView* parent = NULL)
	{
		params.fillFrom(ParamDefaults<typename T::Params, 0>::instance().get());

		T* widget = createWidget<T>(params, parent);
		if (widget)
		{
			widget->postBuild();
		}

		return widget;
	}

	LLView* createFromXML(LLXMLNodePtr node, LLView* parent, const std::string& filename, const widget_registry_t&, LLXMLNodePtr output_node );

	template<typename T>
	static T* createFromFile(const std::string &filename, LLView *parent, const widget_registry_t& registry, LLXMLNodePtr output_node = NULL)
	{
		T* widget = NULL;

		std::string skinned_filename = findSkinnedFilename(filename);
		getInstance()->mFileNames.push_back(skinned_filename);
		{
			LLXMLNodePtr root_node;

			//if exporting, only load the language being exported, 			
			//instead of layering localized version on top of english			
			if (output_node)			
			{					
				if (!LLUICtrlFactory::getLocalizedXMLNode(filename, root_node))				
				{							
					llwarns << "Couldn't parse XUI file: " <<  filename  << llendl;					
					goto fail;				
				}
			}
			else if (!LLUICtrlFactory::getLayeredXMLNode(filename, root_node))
			{
				llwarns << "Couldn't parse XUI file: " << skinned_filename << llendl;
				goto fail;
			}
			
			LLView* view = getInstance()->createFromXML(root_node, parent, filename, registry, output_node);
			if (view)
			{
				widget = dynamic_cast<T*>(view);
				// not of right type, so delete it
				if (!widget) 
				{
					delete view;
					view = NULL;
				}
			
			}
		}
fail:
		getInstance()->mFileNames.pop_back();
		return widget;
	}

	template<class T>
	static T* getDefaultWidget(const std::string& name)
	{
		dummy_widget_creator_func_t* dummy_func = getDefaultWidgetFunc(&typeid(T));
		return dummy_func ? dynamic_cast<T*>((*dummy_func)(name)) : NULL;
	}

	template <class T> 
	static LLView* createDefaultWidget(const std::string& name) 
	{
		typename T::Params params;
		params.name(name);
		
		return create<T>(params);
	}

	static void copyName(LLXMLNodePtr src, LLXMLNodePtr dest);

	template<typename T>
	static T* defaultBuilder(LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node)
	{
		LLFastTimer timer(FTM_WIDGET_SETUP);

		typename T::Params params(getDefaultParams<T>());

		LLXUIParser::instance().readXUI(node, params, LLUICtrlFactory::getInstance()->getCurFileName());

		if (output_node)
		{
			// We always want to output top-left coordinates
			typename T::Params output_params(params);
			T::setupParamsForExport(output_params, parent);
			// Export only the differences between this any default params
			typename T::Params default_params(getDefaultParams<T>());
			copyName(node, output_node);
			LLXUIParser::instance().writeXUI(
				output_node, output_params, &default_params);
		}

		// Apply layout transformations, usually munging rect
		params.from_xui = true;
		T* widget = createWidget<T>(params, parent);

		typedef typename T::child_registry_t registry_t;

		createChildren(widget, node, registry_t::instance(), output_node);

		if (widget && !widget->postBuild())
		{
			delete widget;
			widget = NULL;
		}

		return widget;
	}

	static void createChildren(LLView* viewp, LLXMLNodePtr node, const widget_registry_t&, LLXMLNodePtr output_node = NULL);

	static bool getLayeredXMLNode(const std::string &filename, LLXMLNodePtr& root);
	
	static bool getLocalizedXMLNode(const std::string &xui_filename, LLXMLNodePtr& root);

	static void loadWidgetTemplate(const std::string& widget_tag, LLInitParam::BaseBlock& block);

	// helper function for adding widget type info to various registries
	static void registerWidget(const std::type_info* widget_type, const std::type_info* param_block_type, dummy_widget_creator_func_t creator_func, const std::string& tag);

private:
	// return default widget instance factory func for a given type
	static dummy_widget_creator_func_t* getDefaultWidgetFunc(const std::type_info* widget_type);

	static const std::string* getWidgetTag(const std::type_info* widget_type);

	// this exists to get around dependency on llview
	static void setCtrlParent(LLView* view, LLView* parent, S32 tab_group);

	// Avoid directly using LLUI and LLDir in the template code
	static std::string findSkinnedFilename(const std::string& filename);

	typedef std::deque<const LLCallbackMap::map_t*> factory_stack_t;
	factory_stack_t					mFactoryStack;

	LLPanel*		mDummyPanel;
	std::vector<std::string>	mFileNames;
};

template<typename T>
const LLInitParam::BaseBlock& getEmptyParamBlock()
{
	static typename T::Params params;
	return params;
}

// this is here to make gcc happy with reference to LLUICtrlFactory
template<typename DERIVED>
template<typename T> 
LLChildRegistry<DERIVED>::Register<T>::Register(const char* tag, LLWidgetCreatorFunc func)
:	LLChildRegistry<DERIVED>::StaticRegistrar(tag, func.empty() ? (LLWidgetCreatorFunc)&LLUICtrlFactory::defaultBuilder<T> : func)
{
	// add this widget to various registries
	LLUICtrlFactory::instance().registerWidget(&typeid(T), &typeid(typename T::Params), &LLUICtrlFactory::createDefaultWidget<T>, tag);
	
	// since registry_t depends on T, do this in line here
	typedef typename T::child_registry_t registry_t;
	LLChildRegistryRegistry::instance().defaultRegistrar().add(&typeid(T), registry_t::instance());
}


typedef boost::function<LLPanel* (void)> LLPanelClassCreatorFunc;

// local static instance for registering a particular panel class

class LLRegisterPanelClass
:	public LLSingleton< LLRegisterPanelClass >
{
public:
	// reigister with either the provided builder, or the generic templated builder
	void addPanelClass(const std::string& tag,LLPanelClassCreatorFunc func)
	{
		mPanelClassesNames[tag] = func;
	}

	LLPanel* createPanelClass(const std::string& tag)
	{
		param_name_map_t::iterator iT =  mPanelClassesNames.find(tag);
		if(iT == mPanelClassesNames.end())
			return 0;
		return iT->second();
	}
	template<typename T>
	static T* defaultPanelClassBuilder()
	{
		T* pT = new T();
		return pT;
	}

private:
	typedef std::map< std::string, LLPanelClassCreatorFunc> param_name_map_t;
	
	param_name_map_t mPanelClassesNames;
};


// local static instance for registering a particular panel class
template<typename T>
class LLRegisterPanelClassWrapper
:	public LLRegisterPanelClass
{
public:
	// reigister with either the provided builder, or the generic templated builder
	LLRegisterPanelClassWrapper(const std::string& tag);
};


template<typename T>
LLRegisterPanelClassWrapper<T>::LLRegisterPanelClassWrapper(const std::string& tag) 
{
	LLRegisterPanelClass::instance().addPanelClass(tag,&LLRegisterPanelClass::defaultPanelClassBuilder<T>);
}


#endif //LLUICTRLFACTORY_H
