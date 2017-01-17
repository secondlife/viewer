integer listenHandle; 
list saved_gstate;

// Here we're trying to handle attachments on avatars. The case that
// additional avatars may be seated on those attachments, if that's
// even possible, is not addressed.
integer save_gstate(string gstate)
{
    integer link = llGetNumberOfPrims() > 1;
    integer end = link + llGetNumberOfPrims();
    saved_gstate = [];

    llOwnerSay("llGetNumberOfPrims returns " + (string) llGetNumberOfPrims());
    llOwnerSay("save_gstate " + (string) link + " " + (string) end);
    for(;link < end; ++link)//loop through all prims
    {
        if (gstate=="alpha")
        {
            list color_list =  llGetLinkPrimitiveParams(link, [PRIM_COLOR, ALL_SIDES]);
            llOwnerSay("save_gstate " + (string) link + " color_list " + (string) color_list);
            saved_gstate += color_list;
        }
        else
        {
            llOwnerSay("unknown gstate " + gstate);
        }
    }
    return 0;
}   

integer restore_gstate(string gstate)
{
    integer start = llGetNumberOfPrims() > 1;
    integer end = start + llGetNumberOfPrims();
    integer link;
    integer i = 0;
    list restore_list = saved_gstate;
    for(link=start; link < end; ++link)//loop through all prims
    {
        llOwnerSay("link " + (string) link + " restore_gstate using " + (string) restore_list);
        integer face = 0;
        for (;face < llGetLinkNumberOfSides(link);++face)
        {
            if (gstate=="alpha")
            {
                vector color = llList2Vector(restore_list, 0);
                float alpha = llList2Float(restore_list, 1);
                llOwnerSay("restore link alpha " + (string) link + " face " + (string) face + " color " + (string) color + " alpha " + (string) alpha);
                restore_list = llDeleteSubList(restore_list, 0, 1);
                llSetLinkPrimitiveParamsFast(link, [PRIM_COLOR, face, color, alpha]);
            }
            else
            {
                llOwnerSay("unknown gstate " + gstate);
            }
        }
   }
    return 0;
}

integer change_gstate(string gstate)
{
    integer start = llGetNumberOfPrims() > 1;
    integer end = start + llGetNumberOfPrims();
    integer link;
    integer i = 0;
    for(link=start; link < end; ++link)//loop through all prims
    {
        llOwnerSay("link " + (string) link + " change_gstate gstate " + gstate);
        integer face = 0;
        for (;face < llGetLinkNumberOfSides(link);++face)
        {
            if (gstate=="alpha")
            {
                vector color = <1,0,0>;
                float alpha = 0.5;
                llOwnerSay("alter link " + (string) link + " face " + (string) face + " color " + (string) color + " alpha " + (string) alpha);
                llSetLinkPrimitiveParamsFast(link, [PRIM_COLOR, face, color, alpha]);
            }
            else
            {
                llOwnerSay("unknown gstate " + gstate);
            }
        }
   }
    return 0;
}

default
{
    state_entry()
    {
        llOwnerSay("listening");
        listenHandle = llListen(-777,"","","");  
    }

    listen(integer channel, string name, key id, string message)
    {
        llOwnerSay("listen func called");
        llOwnerSay("got message " + name + " " + (string) id + " " + message);
        list words = llParseString2List(message,[" "],[]);
        string gstate = llList2String(words,0);
        string action = llList2String(words,1);
        llOwnerSay("gstate " + gstate + " action " + action);
        if (action=="start")
        {
            save_gstate(gstate);
            change_gstate(gstate);
        }
        else if (action=="stop")
        {
            restore_gstate(gstate);
        }
    }
    
    touch_start(integer total_number)
    {
        llOwnerSay( "Touched.");
    }
}
