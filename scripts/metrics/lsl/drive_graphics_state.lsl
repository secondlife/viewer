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

        llOwnerSay("Starting alpha");
        llSay(-777,"alpha start");
        llSleep(5.0);
        llOwnerSay("Done alpha");
        llSay(-777,"alpha stop");
    }
}
