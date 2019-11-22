list texture_ids = [
{TEXTURE_IDS}
    ];

clear_textures()
{
    integer link = LINK_SET;
    integer face = ALL_SIDES;
    llSetLinkPrimitiveParamsFast(link, [PRIM_TEXTURE, face, NULL_KEY, <1,1,0>, <0,0,0>, 0]);
    llSetLinkPrimitiveParamsFast(link, [PRIM_NORMAL, face, NULL_KEY, <1,1,0>, <0,0,0>, 0]);
    llSetLinkPrimitiveParamsFast(link, [PRIM_SPECULAR, face, NULL_KEY, <1,1,0>, <0,0,0>, 0, <1,1,1>, 200, 0]);
}

set_textures(list ids, string which)
{
    llOwnerSay("set_textures starts, which " + which);
    integer start = llGetNumberOfPrims() > 1;
    integer end = start + llGetNumberOfPrims();
    integer i = 0;
    integer link;
    integer texture_count = llGetListLength(texture_ids);
    for(link=start; link < end && i < texture_count; ++link)//loop through all prims
    {
        string tex = llList2String(texture_ids, i);
        i += 1;
        llOwnerSay("link " + (string) link + " set_textures, " + which + ", id " + (string) tex);
        integer face = ALL_SIDES;
        if (which == "diffuse")
        {
            llSetLinkPrimitiveParamsFast(link, [PRIM_TEXTURE, face, tex, <1,1,0>, <0,0,0>, 0]);
        }
        if (which == "normal" || which == "materials")
        {
            llSetLinkPrimitiveParamsFast(link, [PRIM_NORMAL, face, tex, <1,1,0>, <0,0,0>, 0]);
        }
        if (which == "specular" || which == "materials")
        {
            llSetLinkPrimitiveParamsFast(link, [PRIM_SPECULAR, face, tex, <1,1,0>, <0,0,0>, 0, <1,1,1>, 200, 0]);
        }

    }
    llOwnerSay("setting name {NAME}");
    llSetObjectName("{NAME}");
    llOwnerSay("change_textures done");
}

default
{
    state_entry()
    {
        string which = "{WHICH}";
        llSay(0, "Clearing textures");
        clear_textures();
        llSay(0, "Setting textures of type " + which);
        set_textures(texture_ids, which);
    }

    touch_start(integer total_number)
    {
        llSay(0, "Touched.");
    }
}
