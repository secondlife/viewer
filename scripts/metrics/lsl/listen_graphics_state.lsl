integer listenHandle; 
list restore_list;

// Here we're trying to handle attachments on avatars. The case that
// additional avatars may be seated on those attachments, if that's
// even possible, is not addressed.
integer save_gstate(string gstate)
{
    integer link = llGetNumberOfPrims() > 1;
    integer end = link + llGetNumberOfPrims();

    llOwnerSay("llGetNumberOfPrims returns " + (string) llGetNumberOfPrims());
    llOwnerSay("save_gstate " + (string) link + " " + (string) end);
    for(;link < end; ++link)//loop through all prims
    {
        if (gstate=="alpha")
        {
            list color_list =  llGetLinkPrimitiveParams(link, [PRIM_COLOR, ALL_SIDES]);
            llOwnerSay("save_gstate " + (string) link + " color_list " + (string) color_list);
            restore_list += color_list;
        }
        else if (gstate=="glow")
        {
            list glow_list =  llGetLinkPrimitiveParams(link, [PRIM_GLOW, ALL_SIDES]);
            llOwnerSay("save_gstate " + (string) link + " glow_list " + (string) glow_list);
            restore_list += glow_list;
        }
        else if (gstate=="shiny")
        {
            list state_list =  llGetLinkPrimitiveParams(link, [PRIM_BUMP_SHINY, ALL_SIDES]);
            llOwnerSay("save_gstate " + (string) link + " bump_shiny " + (string) state_list);
            restore_list += state_list;
        }
        else if (gstate=="bump")
        {
            list state_list =  llGetLinkPrimitiveParams(link, [PRIM_BUMP_SHINY, ALL_SIDES]);
            llOwnerSay("save_gstate " + (string) link + " bump_shiny " + (string) state_list);
            restore_list += state_list;
        }
        else if (gstate=="point_light")
        {
            // Note this is not per-face
            list state_list =  llGetLinkPrimitiveParams(link, [PRIM_POINT_LIGHT]);
            llOwnerSay("save_gstate " + (string) link + " point_light " + (string) state_list);
            restore_list += state_list;
        }
        else if (gstate=="normalmap")
        {
            list state_list =  llGetLinkPrimitiveParams(link, [PRIM_NORMAL, ALL_SIDES]);
            llOwnerSay("save_gstate " + (string) link + " normalmap " + (string) state_list);
            restore_list += state_list;
        }
        else if (gstate=="specmap")
        {
            list state_list =  llGetLinkPrimitiveParams(link, [PRIM_SPECULAR, ALL_SIDES]);
            llOwnerSay("save_gstate " + (string) link + " specmap " + (string) state_list);
            restore_list += state_list;
        }
        else if (gstate=="materials")
        {
            list state_list = llGetLinkPrimitiveParams(link, [PRIM_TEXTURE, ALL_SIDES]);
            llOwnerSay("save_gstate " + (string) link + " 1 - texture " + (string) state_list);
            restore_list += state_list;
            state_list =  llGetLinkPrimitiveParams(link, [PRIM_NORMAL, ALL_SIDES]);
            llOwnerSay("save_gstate " + (string) link + " 2 - normalmap " + (string) state_list);
            restore_list += state_list;
            state_list =  llGetLinkPrimitiveParams(link, [PRIM_SPECULAR, ALL_SIDES]);
            llOwnerSay("save_gstate " + (string) link + " 3- specmap " + (string) state_list);
            restore_list += state_list;
        }
        else
        {
            llOwnerSay("unknown gstate " + gstate);
        }
    }
    return 0;
}   

restore_all_faces(integer param, integer link, integer nargs)
{
    integer face = 0;
    for (;face < llGetLinkNumberOfSides(link);++face)
    {
        list vals = llList2List(restore_list,0,nargs-1); 
        restore_list = llDeleteSubList(restore_list, 0, nargs-1);
        llOwnerSay("restore param " + (string) param + " vals " + (string) vals);
        list args = [param, face] + vals;
        llSetLinkPrimitiveParamsFast(link, args);
    }
}
integer restore_gstate(string gstate)
{
    integer start = llGetNumberOfPrims() > 1;
    integer end = start + llGetNumberOfPrims();
    integer link;
    integer i = 0;
    for(link=start; link < end; ++link)//loop through all prims
    {
        if (gstate=="alpha")
        {
            restore_all_faces(PRIM_COLOR, link, 2);
        }
        else if (gstate=="glow")
        {
            restore_all_faces(PRIM_GLOW, link, 1);
        }
        else if (gstate=="shiny")
        {
            restore_all_faces(PRIM_BUMP_SHINY, link, 2);
        }
        else if (gstate=="bump")
        {
            restore_all_faces(PRIM_BUMP_SHINY, link, 2);
        }
        else if (gstate=="point_light")
        {
            // Not per-face
            list vals = llList2List(restore_list,0,4); 
            restore_list = llDeleteSubList(restore_list, 0, 4);
            llOwnerSay("restore point_light " + (string) vals);
            list args = [PRIM_POINT_LIGHT] + vals;
            llOwnerSay("args are " + (string) args);
            llSetLinkPrimitiveParamsFast(link, args);
        }
        else if (gstate=="normalmap")
        {
            restore_all_faces(PRIM_NORMAL, link, 4);
        }
        else if (gstate=="specmap")
        {
            restore_all_faces(PRIM_SPECULAR, link, 7);
        }
        else if (gstate=="materials")
        {
            restore_all_faces(PRIM_TEXTURE, link, 4);
            restore_all_faces(PRIM_NORMAL, link, 4);
            restore_all_faces(PRIM_SPECULAR, link, 7);
        }
        else
        {
            llOwnerSay("unknown gstate " + gstate);
        }
    }
    llOwnerSay("after restore, restore_list is " + (string) restore_list);

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
            else if (gstate=="glow")
            {
                float glow = 1.0;
                llOwnerSay("alter link " + (string) link + " face " + (string) face + " glow " + (string) glow);
                llSetLinkPrimitiveParamsFast(link, [PRIM_GLOW, face, glow]);
            }
            else if (gstate=="shiny")
            {
                integer shiny = PRIM_SHINY_HIGH;
                integer bump = PRIM_BUMP_NONE;
                llOwnerSay("alter link " + (string) link + " face " + (string) face + " shiny " + (string) shiny);
                llSetLinkPrimitiveParamsFast(link, [PRIM_BUMP_SHINY, face, shiny, bump]);
            }
            else if (gstate=="bump")
            {
                integer shiny = PRIM_SHINY_NONE;
                integer bump = PRIM_BUMP_BRICKS;
                llOwnerSay("alter link " + (string) link + " face " + (string) face + " bump " + (string) bump);
                llSetLinkPrimitiveParamsFast(link, [PRIM_BUMP_SHINY, face, shiny, bump]);
            }
            else if (gstate=="point_light")
            {
                if (face==0)
                {
                    llOwnerSay("alter link " + (string) link + " face " + (string) face + " point_light on");
                    llSetLinkPrimitiveParamsFast(link, [PRIM_POINT_LIGHT, 1, <0,1,0>, 1.0, 10.0, 0.5]);
                }
            }
            else if (gstate=="normalmap")
            {
                string map_string = "8ae04f05-9e8a-e685-2bff-dc061efe6b34";
                llOwnerSay("alter link " + (string) link + " face " + (string) face + " normalmap " + map_string);
                llSetLinkPrimitiveParamsFast(link, [PRIM_NORMAL, face, map_string, <1,1,0>, <0,0,0>, 0]);
            }
            else if (gstate=="specmap")
            {
                string map_string = "5be7fc03-fb6f-2eba-0417-814b3a6289b3";
                llOwnerSay("alter link " + (string) link + " face " + (string) face + " specmap " + map_string);
                llSetLinkPrimitiveParamsFast(link, [PRIM_SPECULAR, face, map_string, <1,1,0>, <0,0,0>, 0, <1,1,1>, 200, 0]);
            }
            else if (gstate=="materials")
            {
                string map_string = "7cc4fdec-0ff5-933d-6977-2adb13298466";
                llOwnerSay("alter link " + (string) link + " face " + (string) face + " texture " + map_string);
                llSetLinkPrimitiveParamsFast(link, [PRIM_TEXTURE, face, map_string, <1,1,0>, <0,0,0>, 0]);

                map_string = "8ae04f05-9e8a-e685-2bff-dc061efe6b34";
                llOwnerSay("alter link " + (string) link + " face " + (string) face + " normalmap " + map_string);
                llSetLinkPrimitiveParamsFast(link, [PRIM_NORMAL, face, map_string, <1,1,0>, <0,0,0>, 0]);

                map_string = "5be7fc03-fb6f-2eba-0417-814b3a6289b3";
                llOwnerSay("alter link " + (string) link + " face " + (string) face + " specmap " + map_string);
                llSetLinkPrimitiveParamsFast(link, [PRIM_SPECULAR, face, map_string, <1,1,0>, <0,0,0>, 0, <1,1,1>, 200, 0]);
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

// Local Variables:
// shadow-file-name: "$SW_HOME/arctan/scripts/metrics/lsl/listen_graphics_state.lsl"
// End:
