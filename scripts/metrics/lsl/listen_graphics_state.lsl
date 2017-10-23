integer listenHandle; 
integer verbose;
integer backChannel = -666;

say_if_verbose(string message)
{
    if (verbose)
    {
        llSay(0, message);
    }
}

integer change_textures(string diffuse_prefix, string normal_prefix, string spec_prefix, integer res, integer count)
{
    say_if_verbose("change_textures starts, res " + (string) res + " count " + (string) count);
    integer start = llGetNumberOfPrims() > 1;
    integer end = start + llGetNumberOfPrims();
    integer i = 0;
    integer link;
    for(link=start; link < end; ++link)//loop through all prims
    {
        say_if_verbose("link " + (string) link + " change_textures");
        integer face = 0;
        for (;face < llGetLinkNumberOfSides(link);++face)
        {
            string diffuse_name = diffuse_prefix + "_" + (string) res + "_" + (string) i;
            say_if_verbose("alter link " + (string) link + " face " + (string) face + " diffusemap " + diffuse_name);
            llSetLinkPrimitiveParamsFast(link, [PRIM_TEXTURE, face, diffuse_name, <1,1,0>, <0,0,0>, 0]);
            
            string normal_name = normal_prefix + "_" + (string) res + "_" + (string) i;
            say_if_verbose("alter link " + (string) link + " face " + (string) face + " normalmap " + normal_name);
            llSetLinkPrimitiveParamsFast(link, [PRIM_NORMAL, face, normal_name, <1,1,0>, <0,0,0>, 0]);
            
            string spec_name = spec_prefix + "_" + (string) res + "_" + (string) i;
            say_if_verbose("alter link " + (string) link + " face " + (string) face + " specmap " + spec_name);
            llSetLinkPrimitiveParamsFast(link, [PRIM_SPECULAR, face, spec_name, <1,1,0>, <0,0,0>, 0, <1,1,1>, 200, 0]);

            i = (i+1)%count;
        }
    }
    say_if_verbose("change_textures done");
    return 0;
}

integer change_gstate(string gstate)
{
    if (gstate=="texture_res_128")
    {
        change_textures("cloud_texture","normal_map_rings","spec_map_rings",128,10);
        return 0;
    }
    else if (gstate=="texture_res_1024")
    {
        change_textures("cloud_texture","normal_map_rings","spec_map_rings",1024,10);
        return 0;
    }
    say_if_verbose("change_gstate " + gstate + " starts");
    integer start = llGetNumberOfPrims() > 1;
    integer end = start + llGetNumberOfPrims();
    integer link;
    integer i = 0;
    for(link=start; link < end; ++link)//loop through all prims
    {
        say_if_verbose("link " + (string) link + " change_gstate gstate " + gstate);
        integer face = 0;
        for (;face < llGetLinkNumberOfSides(link);++face)
        {
            if (gstate=="alpha")
            {
                vector color = <1,0,0>;
                float alpha = 0.5;
                say_if_verbose("alter link " + (string) link + " face " + (string) face + " color " + (string) color + " alpha " + (string) alpha);
                llSetLinkPrimitiveParamsFast(link, [PRIM_COLOR, face, color, alpha]);
            }
            else if (gstate=="glow")
            {
                float glow = 1.0;
                say_if_verbose("alter link " + (string) link + " face " + (string) face + " glow " + (string) glow);
                llSetLinkPrimitiveParamsFast(link, [PRIM_GLOW, face, glow]);
            }
            else if (gstate=="shiny")
            {
                integer shiny = PRIM_SHINY_HIGH;
                integer bump = PRIM_BUMP_NONE;
                say_if_verbose("alter link " + (string) link + " face " + (string) face + " shiny " + (string) shiny);
                llSetLinkPrimitiveParamsFast(link, [PRIM_BUMP_SHINY, face, shiny, bump]);
                llSetLinkPrimitiveParams(link, [PRIM_BUMP_SHINY, face, shiny, bump]);
            }
            else if (gstate=="bump")
            {
                integer shiny = PRIM_SHINY_NONE;
                integer bump = PRIM_BUMP_BRICKS;
                say_if_verbose("alter link " + (string) link + " face " + (string) face + " bump " + (string) bump);
                llSetLinkPrimitiveParamsFast(link, [PRIM_BUMP_SHINY, face, shiny, bump]);
                llSetLinkPrimitiveParams(link, [PRIM_BUMP_SHINY, face, shiny, bump]);
            }
            else if (gstate=="point_light")
            {
                if (face==0)
                {
                    say_if_verbose("alter link " + (string) link + " face " + (string) face + " point_light on");
                    llSetLinkPrimitiveParamsFast(link, [PRIM_POINT_LIGHT, 1, <0,1,0>, 1.0, 10.0, 0.5]);
                }
            }
            else if (gstate=="normalmap")
            {
                string map_string = "8ae04f05-9e8a-e685-2bff-dc061efe6b34";
                say_if_verbose("alter link " + (string) link + " face " + (string) face + " normalmap " + map_string);
                llSetLinkPrimitiveParamsFast(link, [PRIM_NORMAL, face, map_string, <1,1,0>, <0,0,0>, 0]);
            }
            else if (gstate=="specmap")
            {
                string map_string = "5be7fc03-fb6f-2eba-0417-814b3a6289b3";
                say_if_verbose("alter link " + (string) link + " face " + (string) face + " specmap " + map_string);
                llSetLinkPrimitiveParamsFast(link, [PRIM_SPECULAR, face, map_string, <1,1,0>, <0,0,0>, 0, <1,1,1>, 200, 0]);
            }
            else if (gstate=="materials")
            {
                //string map_string = "7cc4fdec-0ff5-933d-6977-2adb13298466";
                //llOwnerSay("alter link " + (string) link + " face " + (string) face + " texture " + map_string);
                //llSetLinkPrimitiveParamsFast(link, [PRIM_TEXTURE, face, map_string, <1,1,0>, <0,0,0>, 0]);

                string map_string = "8ae04f05-9e8a-e685-2bff-dc061efe6b34";
                say_if_verbose("alter link " + (string) link + " face " + (string) face + " normalmap " + map_string);
                llSetLinkPrimitiveParamsFast(link, [PRIM_NORMAL, face, map_string, <1,1,0>, <0,0,0>, 0]);

                map_string = "5be7fc03-fb6f-2eba-0417-814b3a6289b3";
                say_if_verbose("alter link " + (string) link + " face " + (string) face + " specmap " + map_string);
                llSetLinkPrimitiveParamsFast(link, [PRIM_SPECULAR, face, map_string, <1,1,0>, <0,0,0>, 0, <1,1,1>, 200, 0]);
            }
            else
            {
                say_if_verbose("ignoring unknown gstate " + gstate);
            }
        }
    }
    say_if_verbose("change_gstate " + gstate + " done");
    return 0;
}

// We don't have enough memory to actually remember the previous
// state, so just set it to a cheap default.
integer clear_gstate(string gstate)
{
    if (gstate=="texture_res_128")
    {
        return 0;
    }
    else if (gstate=="texture_res_1024")
    {
        return 0;
    }
    say_if_verbose("clear_gstate " + gstate + " starts");
    integer start = llGetNumberOfPrims() > 1;
    integer end = start + llGetNumberOfPrims();
    integer link;
    integer i = 0;
    for(link=start; link < end; ++link)//loop through all prims
    {
        say_if_verbose("link " + (string) link + " clear_gstate gstate " + gstate);
        integer face = 0;
        for (;face < llGetLinkNumberOfSides(link);++face)
        {
            if (gstate=="alpha")
            {
                vector color = <1,1,1>;
                float alpha = 1.0;
                say_if_verbose("clear link " + (string) link + " face " + (string) face + " color " + (string) color + " alpha " + (string) alpha);
                llSetLinkPrimitiveParamsFast(link, [PRIM_COLOR, face, color, alpha]);
            }
            else if (gstate=="glow")
            {
                float glow = 0.0;
                say_if_verbose("clear link " + (string) link + " face " + (string) face + " glow " + (string) glow);
                llSetLinkPrimitiveParamsFast(link, [PRIM_GLOW, face, glow]);
            }
            else if (gstate=="shiny")
            {
                integer shiny = PRIM_SHINY_NONE;
                integer bump = PRIM_BUMP_NONE;
                say_if_verbose("clear link " + (string) link + " face " + (string) face + " shiny " + (string) shiny);
                llSetLinkPrimitiveParamsFast(link, [PRIM_BUMP_SHINY, face, shiny, bump]);
            }
            else if (gstate=="bump")
            {
                integer shiny = PRIM_SHINY_NONE;
                integer bump = PRIM_BUMP_NONE;
                say_if_verbose("clear link " + (string) link + " face " + (string) face + " bump " + (string) bump);
                llSetLinkPrimitiveParamsFast(link, [PRIM_BUMP_SHINY, face, shiny, bump]);
            }
            else if (gstate=="point_light")
            {
                if (face==0)
                {
                    say_if_verbose("clear link " + (string) link + " face " + (string) face + " point_light off");
                    llSetLinkPrimitiveParamsFast(link, [PRIM_POINT_LIGHT, 0, <0,1,0>, 1.0, 10.0, 0.5]);
                }
            }
            else if (gstate=="normalmap")
            {
                say_if_verbose("clear link " + (string) link + " face " + (string) face + " normalmap");
                llSetLinkPrimitiveParamsFast(link, [PRIM_NORMAL, face, NULL_KEY, <1,1,0>, <0,0,0>, 0]);
            }
            else if (gstate=="specmap")
            {
                say_if_verbose("clear link " + (string) link + " face " + (string) face + " specmap");
                llSetLinkPrimitiveParamsFast(link, [PRIM_SPECULAR, face, NULL_KEY, <1,1,0>, <0,0,0>, 0, <1,1,1>, 200, 0]);
            }
            else if (gstate=="materials")
            {
                string map_string = NULL_KEY;
                say_if_verbose("clear link " + (string) link + " face " + (string) face + " normalmap " + map_string);
                llSetLinkPrimitiveParamsFast(link, [PRIM_NORMAL, face, map_string, <1,1,0>, <0,0,0>, 0]);

                say_if_verbose("clear link " + (string) link + " face " + (string) face + " specmap " + map_string);
                llSetLinkPrimitiveParamsFast(link, [PRIM_SPECULAR, face, map_string, <1,1,0>, <0,0,0>, 0, <1,1,1>, 200, 0]);
            }
            else
            {
                say_if_verbose("unknown gstate " + gstate);
            }
        }
   }
    say_if_verbose("clear_gstate " + gstate + " done");
    return 0;
}

default
{
    state_entry()
    {
        say_if_verbose("listening");
        listenHandle = llListen(-777,"","","");  
        verbose = 0;
    }

    listen(integer channel, string name, key id, string message)
    {
        say_if_verbose("listen func called");
        say_if_verbose("got message " + name + " " + (string) id + " " + message);
        list words = llParseString2List(message,[" "],[]);
        string gstate = llList2String(words,1);
        string action = llList2String(words,0);
        say_if_verbose("action " + action + " gstate " + gstate);
        if (action=="start_prop")
        {
            string duration = llList2String(words,2);
            say_if_verbose("running change_gstate " + gstate + " duration " + (string) duration);
            say_if_verbose("backChannel sending: started");
            llSay(backChannel,"started");
            say_if_verbose("step 1: running change_gstate");
            change_gstate(gstate);
            
            say_if_verbose("step 2: sleeping for requested duration " + (string) duration);
            llSleep((integer) duration);

            say_if_verbose("step 3: running clear_gstate");
            clear_gstate(gstate);

            say_if_verbose("backChannel sending: done");
            llSay(backChannel,"done");
        }
        else if (message == "verbose on")
        {
            llOwnerSay("setting verbose -> on");
            verbose = 1;
        }
        else if (message == "verbose off")
        {
            llOwnerSay("setting verbose -> off");
            verbose = 0;
        }
        else
        {
            say_if_verbose("ignoring message " + message);
        }
    }
}

// Local Variables:
// shadow-file-name: "$SW_HOME/arctan/scripts/metrics/lsl/listen_graphics_state.lsl"
// End:
