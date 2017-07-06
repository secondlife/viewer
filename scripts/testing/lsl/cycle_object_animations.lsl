cycle_animations()
{
    list AnimationList;
    integer count = llGetInventoryNumber(INVENTORY_ANIMATION);
    string ItemName;
    string NowPlaying;
    while (count--)
    {
        ItemName = llGetInventoryName(INVENTORY_ANIMATION, count);
        if (NowPlaying != "")
        {
            //llSay(0, "Stopping " + NowPlaying);
            llStopObjectAnimation(NowPlaying);
        }
        //llSay(0, "Starting " + ItemName);
        llStartObjectAnimation(ItemName);
        NowPlaying = ItemName;
        llSleep(10);
    }
    if (NowPlaying != "")
    {
        //llSay(0, "Stopping " + NowPlaying);
        llStopObjectAnimation(NowPlaying);
        llSleep(10);
    }
}

default
{
    state_entry()
    {
        llSay(0, "Animated Object here");
    }

    touch_start(integer total_number)
    {
        llSay(0, "Touch started.");
        while (1)
        {
            cycle_animations();
        }

    }
    
    touch_end(integer total_number)
    {
        llSay(0, "Touch ended.");
    }
}

// Local Variables:
// shadow-file-name: "$SW_HOME/axon/scripts/testing/lsl/cycle_object_animations.lsl"
// End:
