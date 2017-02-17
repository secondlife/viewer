default
{
    state_entry()
    {
        llOwnerSay("driver is starting");
        llSay(-777,"starting");
    }

    touch_start(integer total_number)
    {
        llOwnerSay("Touched, starting tests");

        float test_time = 20.0;
        float pause_time = 20.0;
        list properties = ["materials", "specmap", "normalmap", "point_light", "bump", "alpha", "glow", "shiny"];
        integer length = llGetListLength(properties);
        integer index = 0;
        while (index < length)
        {
            string prop = llList2String(properties, index);
            ++index;
            
            llOwnerSay("Starting " + prop);
            llSay(-777, prop + " start");
            llSleep(test_time);
            llOwnerSay("Done " + prop);
            llSay(-777,prop + " stop");
            llSleep(pause_time);
        }
    }
}

// Local Variables:
// shadow-file-name: "$SW_HOME/arctan/scripts/metrics/lsl/drive_graphics_state.lsl"
// End:
