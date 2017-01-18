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

        float test_time = 5.0;
        float pause_time = 5.0;
        
        llOwnerSay("Starting point_light");
        llSay(-777,"point_light start");
        llSleep(test_time);
        llOwnerSay("Done point_light");
        llSay(-777,"point_light stop");
        llSleep(pause_time);

        llOwnerSay("Starting bump");
        llSay(-777,"bump start");
        llSleep(test_time);
        llOwnerSay("Done bump");
        llSay(-777,"bump stop");
        llSleep(pause_time);

        llOwnerSay("Starting alpha");
        llSay(-777,"alpha start");
        llSleep(test_time);
        llOwnerSay("Done alpha");
        llSay(-777,"alpha stop");
        llSleep(pause_time);

        llOwnerSay("Starting glow");
        llSay(-777,"glow start");
        llSleep(test_time);
        llOwnerSay("Done glow");
        llSay(-777,"glow stop");
        llSleep(pause_time);
        
        llOwnerSay("Starting shiny");
        llSay(-777,"shiny start");
        llSleep(test_time);
        llOwnerSay("Done shiny");
        llSay(-777,"shiny stop");
        llSleep(pause_time);
    }
}
