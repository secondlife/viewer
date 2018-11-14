integer listenHandle; 
integer verbose;
integer current_animation_number;
string NowPlaying;

say_if_verbose(integer channel, string message)
{
    if (verbose)
    {
        llSay(channel, message);
    }
}

stop_all_animations()
{
    integer count = llGetInventoryNumber(INVENTORY_ANIMATION);
    string ItemName;
    string NowPlaying;
    while (count--)
    {
        ItemName = llGetInventoryName(INVENTORY_ANIMATION, count);
        say_if_verbose(0, "Stopping " + ItemName);
        llStopObjectAnimation(ItemName);
    }
}

start_cycle_animations()
{
    current_animation_number = llGetInventoryNumber(INVENTORY_ANIMATION);
    next_animation(); // Do first iteration without waiting for timer
    llSetTimerEvent(5.0);
}

next_animation()
{
    string ItemName;
    if (NowPlaying != "")
    {
        say_if_verbose(0, "Stopping " + NowPlaying);
        llStopObjectAnimation(NowPlaying);
    }
    if (current_animation_number--)
    {
        ItemName = llGetInventoryName(INVENTORY_ANIMATION, current_animation_number);
        say_if_verbose(0, "Starting " + ItemName);
        llStartObjectAnimation(ItemName);
        NowPlaying = ItemName;
    }
    else
    {
        // Start again at the top
        current_animation_number = llGetInventoryNumber(INVENTORY_ANIMATION);
    }
}

stop_cycle_animations()
{
    llSetTimerEvent(0);
}

default
{
    state_entry()
    {
        say_if_verbose(0, "Animated Object here");
        listenHandle = llListen(-2001,"","","");  
        verbose = 0;

        stop_all_animations();
    }

    listen(integer channel, string name, key id, string message)
    {
        //llOwnerSay("got message " + name + " " + (string) id + " " + message);
        list words = llParseString2List(message,[" "],[]);
        string command = llList2String(words,0);
        string option = llList2String(words,1);
        if (command=="anim")
        {
            stop_all_animations();
            if (option=="start")
            {
                start_cycle_animations();
            }
            else if (option=="stop")
            {
                stop_cycle_animations();
            }
        }
        if (command=="verbose")
        {
            if (option=="on")
            {
                verbose = 1;
            }
            else if (option=="off")
            {
                verbose = 0;
            }
        }
    }

    timer()
    {
        say_if_verbose(0, "timer triggered");
        next_animation();
    }
    
    touch_start(integer total_number)
    {
        say_if_verbose(0, "Touch started.");
        start_cycle_animations();
    }
}

// Local Variables:
// shadow-file-name: "$SW_HOME/axon/scripts/testing/lsl/cycle_object_animations.lsl"
// End:
