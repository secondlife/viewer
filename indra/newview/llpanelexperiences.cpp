#include "llviewerprecompiledheaders.h"


#include "llpanelprofile.h"
#include "lluictrlfactory.h"
#include "llexperiencecache.h"
#include "llagent.h"

#include "llpanelexperiences.h"
#include "llslurl.h"


static LLRegisterPanelClassWrapper<LLPanelExperiences> register_experiences_panel("experiences_panel");


LLPanelExperiences::LLPanelExperiences(  )
	: mExperiencesList(NULL)
{
    buildFromFile("panel_experiences.xml");
}

BOOL LLPanelExperiences::postBuild( void )
{
	mExperiencesList = getChild<LLFlatListView>("experiences_list");
	if(hasString("no_experiences"))
	{
		mExperiencesList->setNoItemsCommentText(getString("no_experiences"));
	}

	return TRUE;
}



LLExperienceItem* LLPanelExperiences::getSelectedExperienceItem()
{
	LLPanel* selected_item = mExperiencesList->getSelectedItem();
	if (!selected_item) return NULL;

	return dynamic_cast<LLExperienceItem*>(selected_item);
}

void LLPanelExperiences::setExperienceList( const LLSD& experiences )
{
    mExperiencesList->clear();

    LLSD::array_const_iterator it = experiences.beginArray();
    for( /**/ ; it != experiences.endArray(); ++it)
    {
        LLUUID public_key = it->asUUID();
        LLExperienceItem* item = new LLExperienceItem();

        item->init(public_key);
        mExperiencesList->addItem(item, public_key);
    }
}

LLPanelExperiences* LLPanelExperiences::create(const std::string& name)
{
    LLPanelExperiences* panel= new LLPanelExperiences();
    panel->setName(name);
    return panel;
}

void LLPanelExperiences::removeExperiences( const LLSD& ids )
{
    LLSD::array_const_iterator it = ids.beginArray();
    for( /**/ ; it != ids.endArray(); ++it)
    {
        removeExperience(it->asUUID());
    }
}

void LLPanelExperiences::removeExperience( const LLUUID& id )
{
    mExperiencesList->removeItemByUUID(id);
}

void LLPanelExperiences::addExperience( const LLUUID& id )
{
    if(!mExperiencesList->getItemByValue(id))
    {
        LLExperienceItem* item = new LLExperienceItem();

        item->init(id);
        mExperiencesList->addItem(item, id);
    }
}




LLExperienceItem::LLExperienceItem()
{
	buildFromFile("panel_experience_list_item.xml");
}

void LLExperienceItem::init( const LLUUID& id)
{
    getChild<LLUICtrl>("experience_name")->setValue(LLSLURL("experience", id, "profile").getSLURLString());
}


LLExperienceItem::~LLExperienceItem()
{

}

void LLPanelSearchExperiences::doSearch()
{

}

LLPanelSearchExperiences* LLPanelSearchExperiences::create( const std::string& name )
{
    LLPanelSearchExperiences* panel= new LLPanelSearchExperiences();
    panel->getChild<LLPanel>("results")->addChild(LLPanelExperiences::create(name));
    ///XXXif(
    return panel;
}

BOOL LLPanelSearchExperiences::postBuild( void )
{
    childSetAction("search_button", boost::bind(&LLPanelSearchExperiences::doSearch, this));
    return TRUE;
}
