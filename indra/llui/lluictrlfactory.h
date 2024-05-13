/** 
 * @file lluictrlfactory.h
 * @brief Factory class for creating UI controls
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LLUICTRLFACTORY_H
#define LLUICTRLFACTORY_H

#include "llfasttimer.h"
#include "llinitparam.h"
#include "llregistry.h"
#include "llxuiparser.h"
#include "llstl.h"
#include "lldir.h"
#include "llsingleton.h"
#include "llheteromap.h"

class LLView;
void deleteView(LLView*); // Inside LLView.cpp, avoid having to potentially delete an incomplete type here.

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
	LLSINGLETON_EMPTY_CTOR(LLDefaultChildRegistry);
};

// lookup widget name by type
class LLWidgetNameRegistry 
:	public LLRegistrySingleton<const std::type_info*, std::string, LLWidgetNameRegistry>
{
	LLSINGLETON_EMPTY_CTOR(LLWidgetNameRegistry);
};

// lookup function for generating empty param block by widget type
// this is used for schema generation
//typedef const LLInitParam::BaseBlock& (*empty_param_block_func_t)();
//class LLDefaultParamBlockRegistry
//:	public LLRegistrySingleton<const std::type_info*, empty_param_block_func_t, LLDefaultParamBlockRegistry>
//{
//	LLSINGLETON(LLDefaultParamBlockRegistry);
//};

// Build time optimization, generate this once in .cpp file
#ifndef LLUICTRLFACTORY_CPP
extern template class LLUICtrlFactory* LLSingleton<class LLUICtrlFactory>::getInstance();
#endif

class LLUICtrlFactory : public LLSingleton<LLUICtrlFactory>
{
	LLSINGLETON(LLUICtrlFactory);
	~LLUICtrlFactory();

	// only partial specialization allowed in inner classes, so use extra dummy parameter
	template <typename PARAM_BLOCK, int DUMMY>
	class ParamDefaults
	{
	public:
		ParamDefaults();
		const PARAM_BLOCK& get() { return mPrototype; }

	private:
		PARAM_BLOCK mPrototype;
	};

	// base case for recursion, there are NO base classes of LLInitParam::BaseBlock
	template<int DUMMY>
	class ParamDefaults<LLInitParam::BaseBlock, DUMMY>
	{
	public:
		ParamDefaults();
		const LLInitParam::BaseBlock& get() { return mBaseBlock; }
	private:
		LLInitParam::BaseBlock mBaseBlock;
	};

public:

	// get default parameter block for widget of a specific type
	template<typename T>
	static const typename T::Params& getDefaultParams()
	{
		return instance().mParamDefaultsMap.obtain< ParamDefaults<typename T::Params, 0> >().get();
	}

	// Does what you want for LLFloaters and LLPanels
	// Returns 0 on success
	S32 saveToXML(LLView* viewp, const std::string& filename);

	// filename tracking for debugging info
	std::string getCurFileName();
	void pushFileName(const std::string& name);
	void popFileName();

	template<typename T>
	static T* create(typename T::Params& params, LLView* parent = NULL)
	{
		params.fillFrom(instance().mParamDefaultsMap.obtain<
						ParamDefaults<typename T::Params, 0> >().get());

		T* widget = createWidgetImpl<T>(params, parent);
		if (widget)
		{
			widget->postBuild();
		}

		return widget;
	}

	LLView* createFromXML(LLXMLNodePtr node, LLView* parent, const std::string& filename, const widget_registry_t&, LLXMLNodePtr output_node );

	template<typename T>
	static T* createFromFile(const std::string &filename, LLView *parent, const widget_registry_t& registry)
	{
		T* widget = NULL;

		instance().pushFileName(filename);
		{
			LLXMLNodePtr root_node;

			if (!LLUICtrlFactory::getLayeredXMLNode(filename, root_node))
			{
                LL_WARNS() << "Couldn't parse XUI from path: " << instance().getCurFileName() << ", from filename: " << filename << LL_ENDL;
				goto fail;
			}

			LLView* view = getInstance()->createFromXML(root_node, parent, filename, registry, NULL);
			if (view)
			{
				widget = dynamic_cast<T*>(view);
				// not of right type, so delete it
				if (!widget) 
				{
					LL_WARNS() << "Widget in " << filename << " was of type " << typeid(view).name() << " instead of expected type " << typeid(T).name() << LL_ENDL;

					deleteView(view);
					view = NULL;
				}
			}
		}
fail:
		instance().popFileName();
		return widget;
	}

	template<class T>
	static T* getDefaultWidget(const std::string& name)
	{
		typename T::Params widget_params;
		widget_params.name = name;
		return create<T>(widget_params);
	}

	static void createChildren(LLView* viewp, LLXMLNodePtr node, const widget_registry_t&, LLXMLNodePtr output_node = NULL);

	static bool getLayeredXMLNode(const std::string &filename, LLXMLNodePtr& root,
								  LLDir::ESkinConstraint constraint=LLDir::CURRENT_SKIN);

private:
	//NOTE: both friend declarations are necessary to keep both gcc and msvc happy
	template <typename T> friend class LLChildRegistry;
	template <typename T> template <typename U> friend class LLChildRegistry<T>::Register;

	static void copyName(LLXMLNodePtr src, LLXMLNodePtr dest);

	// helper function for adding widget type info to various registries
	static void registerWidget(const std::type_info* widget_type, const std::type_info* param_block_type, const std::string& tag);

	static void loadWidgetTemplate(const std::string& widget_tag, LLInitParam::BaseBlock& block);

	template<typename T>
	static T* createWidgetImpl(const typename T::Params& params, LLView* parent = NULL)
	{
        LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
		T* widget = NULL;

		if (!params.validateBlock())
		{
			LL_WARNS() << getInstance()->getCurFileName() << ": Invalid parameter block for " << typeid(T).name() << LL_ENDL;
			//return NULL;
		}

		widget = new T(params);	
		
		widget->initFromParams(params);

		if (parent)
		{
			S32 tab_group = params.tab_group.isProvided() ? params.tab_group() : S32_MAX;
			setCtrlParent(widget, parent, tab_group);
		}
		return widget;
	}

	template<typename T>
	static T* defaultBuilder(LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node)
	{
        LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;

		typename T::Params params(getDefaultParams<T>());

		LLXUIParser parser;
		parser.readXUI(node, params, LLUICtrlFactory::getInstance()->getCurFileName());

		if (output_node)
		{
			// We always want to output top-left coordinates
			typename T::Params output_params(params);
			T::setupParamsForExport(output_params, parent);
			copyName(node, output_node);
			parser.writeXUI(output_node, output_params, LLInitParam::default_parse_rules(), &getDefaultParams<T>());
		}

		// Apply layout transformations, usually munging rect
		params.from_xui = true;
		T::applyXUILayout(params, parent);
		T* widget = createWidgetImpl<T>(params, parent);

		typedef typename T::child_registry_t registry_t;

		createChildren(widget, node, registry_t::instance(), output_node);

		if (widget && !widget->postBuild())
		{
			delete widget;
			widget = NULL;
		}

		return widget;
	}


	// this exists to get around dependency on llview
	static void setCtrlParent(LLView* view, LLView* parent, S32 tab_group);

	class LLPanel*		mDummyPanel;
	std::vector<std::string>	mFileNames;

	// store ParamDefaults specializations
	// Each ParamDefaults specialization used to be an LLSingleton in its own
	// right. But the 2016 changes to the LLSingleton mechanism, making
	// LLSingleton instances polymorphic, are incompatible with current
	// LLInitParam::BaseBlock functionality. (Thanks NickyD for spotting
	// that!) Moreover, instances of the private nested ParamDefaults template
	// aren't global resources -- which is what LLSingleton is designed for.
	// This is simply a cache looked up by type. Its lifespan is tied to
	// LLUICtrlFactory. Use LLHeteroMap for this cache.
	LLHeteroMap mParamDefaultsMap;
};

template <typename PARAM_BLOCK, int DUMMY>
LLUICtrlFactory::ParamDefaults<PARAM_BLOCK, DUMMY>::ParamDefaults()
{
	// look up template file for this param block...
	const std::string* param_block_tag = LLWidgetNameRegistry::instance().getValue(&typeid(PARAM_BLOCK));
	if (param_block_tag)
	{	// ...and if it exists, back fill values using the most specific template first
		PARAM_BLOCK params;
		LLUICtrlFactory::loadWidgetTemplate(*param_block_tag, params);
		mPrototype.fillFrom(params);
	}
	// recursively fill from base class param block
	((typename PARAM_BLOCK::base_block_t&)mPrototype).fillFrom(
		LLUICtrlFactory::instance().mParamDefaultsMap.obtain<
		ParamDefaults<typename PARAM_BLOCK::base_block_t, DUMMY> >().get());

}

template <int DUMMY>
LLUICtrlFactory::ParamDefaults<LLInitParam::BaseBlock, DUMMY>::ParamDefaults() {}

// this is here to make gcc happy with reference to LLUICtrlFactory
template<typename DERIVED>
template<typename T> 
LLChildRegistry<DERIVED>::Register<T>::Register(const char* tag, LLWidgetCreatorFunc func)
:	LLChildRegistry<DERIVED>::StaticRegistrar(tag, func.empty() ? (LLWidgetCreatorFunc)&LLUICtrlFactory::defaultBuilder<T> : func)
{
	// add this widget to various registries
	LLUICtrlFactory::instance().registerWidget(&typeid(T), &typeid(typename T::Params), tag);
	
	// since registry_t depends on T, do this in line here
	// TODO: uncomment this for schema generation
	//typedef typename T::child_registry_t registry_t;
	//LLChildRegistryRegistry::instance().defaultRegistrar().add(&typeid(T), registry_t::instance());
}

#endif //LLUICTRLFACTORY_H
